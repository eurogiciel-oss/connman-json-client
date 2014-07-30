/*
 *  connman-json-client
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <ncurses.h>

#include "commands.h"
#include "json_utils.h"
#include "loop.h"
#include "dbus_json.h"
#include "keys.h"
#include "json_regex.h"

#include "engine.h"

DBusConnection *connection;
struct json_object *jregex_agent_response;
struct json_object *jregex_agent_retry_response;

static DBusConnection *agent_dbus_conn;

void (*engine_callback)(int status, struct json_object *jobj) = NULL;

/* state for the initialisation */
static enum {INIT_STATE, INIT_TECHNOLOGIES, INIT_SERVICES, INIT_OVER} init_status = INIT_STATE;

/* the recorded state as given by connman-json */
static struct json_object *state;

/* the recorded technologies as given by connman-json */
static struct json_object *technologies;

/* the recorded services as given by connman-json */
static struct json_object *services;

static void react_to_sig_service(struct json_object *interface,
			struct json_object *path, struct json_object *data,
			const char *sig_name);
static void react_to_sig_technology(struct json_object *interface,
			struct json_object *path, struct json_object *data,
			const char *sig_name);
static void react_to_sig_manager(struct json_object *interface,
			struct json_object *path, struct json_object *data,
			const char *sig_name);

static struct {
	void (*react_to_sig)(struct json_object *interface,
			struct json_object *path, struct json_object *data,
			const char *sig_name);
} subscribed_to[] = {
	{ react_to_sig_service },	// Service
	{ react_to_sig_technology },	// Technology
	{ react_to_sig_manager },	// Manager
};

static struct agent_data *agent_data_cache;

static void engine_commands_cb(struct json_object *data, json_bool is_error)
{
	switch (init_status) {
		case INIT_STATE:
			state = data;
			break;

		case INIT_TECHNOLOGIES:
			technologies = data;
			break;

		case INIT_SERVICES:
			services = data;
			break;

		default:
			if (data)
				engine_callback((is_error ? -1 : 0), data);
			break;
	}

	if (init_status != INIT_OVER)
		loop_quit();
}

static void engine_agent_cb(struct json_object *data, struct agent_data *request)
{
	struct json_object *res;

	if (agent_data_cache != NULL) {
		res = json_object_new_object();
		json_object_object_add(res, key_dbus_json_agent_error_key,
				json_object_new_string("Already handling an "
				"agent request"));
		engine_callback(-ENOMEM, res);
		return;
	}

	agent_data_cache = request;
	engine_callback(0, data);
}

static void engine_agent_error_cb(struct json_object *data)
{
	engine_callback(-1, data);
}

static int agent_response(struct json_object *data)
{
	int res;

	res = json_to_agent_response(data, agent_data_cache);
	agent_data_cache = NULL;

	return res;
}

static int agent_cancel(struct json_object *data)
{
	agent_cancel_request();
	agent_data_cache = NULL;

	return -EINPROGRESS;
}

/*
 * data: bool
 */
static int agent_error_response(struct json_object *data)
{
	report_error_return(data, agent_data_cache);
	agent_data_cache = NULL;

	return -EINPROGRESS;
}

/*
 * dbus_name in technologies or services
 * dbus_name -> [ dbus_name, { dict } ]
 */
static struct json_object *search_technology_or_service(struct json_object *ressource,
                                       const char *cmd)
{
	int len, i;
	bool found = false;
	struct json_object *sub_array, *name;
	const char *bus_name;

	len = json_object_array_length(ressource);

	for (i = 0; i < len && !found; i++) {
		sub_array = json_object_array_get_idx(ressource, i);
		name = json_object_array_get_idx(sub_array, 0);
		bus_name = json_object_get_string(name);

		if (strncmp(bus_name, cmd, 256) == 0)
			return sub_array;
	}

	return 0;
}

static struct json_object *get_technology(const char *cmd)
{
	return search_technology_or_service(technologies, cmd);
}

static struct json_object *get_service(const char *cmd)
{
	return search_technology_or_service(services, cmd);
}

static bool has_service(const char *cmd)
{
	return !!get_service(cmd);
}

/*
 {
	"command": <cmd_name>,
	"cmd_data": <data>
 }
 */
static struct json_object* coating(const char *cmd_name,
		struct json_object *data)
{
	struct json_object *res = json_object_new_object();
	struct json_object *tmp = json_object_get(data);

	json_object_object_add(res, key_command,
			json_object_new_string(cmd_name));
	json_object_object_add(res, key_command_data, tmp);

	return res;
}

static int get_state(struct json_object *jobj)
{
	return __cmd_state();
}

static int get_services(struct json_object *jobj)
{
	return __cmd_services();
}

static int get_technologies(struct json_object *jobj)
{
	return __cmd_technologies();
}

/*
 {
	"command": "get_home_page",
	"cmd_data": {
		"state": {
			...
		},
		"technologies": {
			...
		}
	}
 }
 */
static int get_home_page(struct json_object *jobj)
{
	struct json_object *res;

	res = json_object_new_object();
	json_object_object_add(res, key_state, json_object_get(state));
	json_object_object_add(res, key_technologies, json_object_get(technologies));

	engine_callback(0, coating("get_home_page", res));

	// coating increment ref count of res, but creating a new object already
	// increment the ref count of res
	json_object_put(res);

	return -EINPROGRESS;
}

static struct json_object* get_services_matching_tech_type(const char
		*technology, bool is_connected)
{
	struct json_object *array_serv, *serv_dict, *serv_type, *res;
	struct json_object *serv_state;
	int len, i;
	bool is_online, is_ready;

	res = json_object_new_array();
	len = json_object_array_length(services);

	for (i = 0; i < len; i++) {
		array_serv = json_object_array_get_idx(services, i);
		serv_dict = json_object_array_get_idx(array_serv, 1);
		json_object_object_get_ex(serv_dict, "Type", &serv_type);

		if (strncmp(json_object_get_string(serv_type), technology, 256) == 0) {
			json_object_object_get_ex(serv_dict, "State", &serv_state);

			is_online = strncmp(json_object_get_string(serv_state),
					"online", 256) == 0;
			is_ready = strncmp(json_object_get_string(serv_state),
					"ready", 256) == 0;

			// Do we look for something we are connected to ?
			//	Yes -> is the service online / ready ?
			//		No -> continue search
			//		Yes -> remember the service
			if (is_connected && !(is_online || is_ready))
				continue;

			json_object_array_add(res, json_object_get(array_serv));
		}
	}

	return res;
}

static int get_services_from_tech(struct json_object *jobj)
{
	struct json_object *tmp, *res, *res_serv, *res_tech, *tech_dict,
			   *jtech_type, *tech_co;
	const char *tech_dbus_name, *tech_type;

	json_object_object_get_ex(jobj, key_technology, &tmp);
	tech_dbus_name = json_object_get_string(tmp);
	res_tech = get_technology(tech_dbus_name);

	if (res_tech == NULL)
		return -EINVAL;

	json_object_get(res_tech);
	tech_dict = json_object_array_get_idx(res_tech, 1);
	json_object_object_get_ex(tech_dict, "Type", &jtech_type);
	tech_type = json_object_get_string(jtech_type);
	json_object_object_get_ex(tech_dict, "Connected", &tech_co);

	res_serv = get_services_matching_tech_type(tech_type,
			(json_object_get_boolean(tech_co) ? true : false));

	res = json_object_new_object();
	json_object_object_add(res, key_services, res_serv);
	json_object_object_add(res, key_technology, res_tech);
	engine_callback(0, coating("get_services_from_tech", res));
	json_object_put(res);

	return -EINPROGRESS;
}

static int connect_to_service(struct json_object *jobj)
{
	struct json_object *tmp;
	const char *serv_dbus_name;

	json_object_object_get_ex(jobj, key_service, &tmp);
	serv_dbus_name = json_object_get_string(tmp);

	if (!has_service(serv_dbus_name))
		return -EINVAL;

	return __cmd_connect(serv_dbus_name);
}

static int disconnect_technology(struct json_object *jobj)
{
	struct json_object *tmp, *serv, *tech_dict, *tech_type;
	const char *tech_dbus_name, *tech_type_str, *serv_dbus_name;

	json_object_object_get_ex(jobj, key_technology, &tmp);
	tech_dbus_name = json_object_get_string(tmp);
	tmp = get_technology(tech_dbus_name);

	if (tmp == NULL)
		return -EINVAL;

	tech_dict = json_object_array_get_idx(tmp, 1);
	json_object_object_get_ex(tech_dict, "Type", &tech_type);
	tech_type_str = json_object_get_string(tech_type);
	json_object_object_get_ex(tech_dict, "Connected", &tmp);

	serv = get_services_matching_tech_type(tech_type_str,
			(json_object_get_boolean(tmp) ? true : false));

	if (serv == NULL || json_object_array_length(serv) != 1)
		return -EINVAL;

	tmp = json_object_array_get_idx(serv, 0);
	serv_dbus_name = json_object_get_string(
			json_object_array_get_idx(tmp, 0));

	json_object_put(serv);

	return __cmd_disconnect(serv_dbus_name);
}

static int scan_technology(struct json_object *jobj)
{
	struct json_object *tmp;
	const char *tech_dbus_name;

	json_object_object_get_ex(jobj, key_technology, &tmp);
	tech_dbus_name = json_object_get_string(tmp);
	tmp = get_technology(tech_dbus_name);

	if (tmp == NULL)
		return -EINVAL;

	return __cmd_scan(tech_dbus_name);
}

static struct {
	const char *cmd;
	int (*func)(struct json_object *jobj);
	bool trusted_is_json_string;
	/*
	 * We can't only use regex as strings because they are transformed in
	 * json_object and [] {} don't have the same signification in json and
	 * regex. So we use json_object generated in json_regex.{c,h} to
	 * overcome this. See init_cmd_table().
	 */
	union {
		const char *trusted_str;
		struct json_object *trusted_jobj;
	} trusted;
} cmd_table[] = {
	{ key_engine_get_state, get_state, true, { "" } },
	{ key_engine_get_services, get_services, true, { "" } },
	{ key_engine_get_technologies, get_technologies, true, { "" } },
	{ key_engine_get_home_page, get_home_page, true, { "" } },
	{ key_engine_get_services_from_tech, get_services_from_tech, true, {
		"{ \"technology\": \"(%5C%5C|/|([a-zA-Z]))+\" }" } },
	{ key_engine_connect, connect_to_service, true, {
		"{ \"service\": \"(%5C%5C|/|([a-zA-Z]))+\" }" } },
	{ key_engine_disconnect, disconnect_technology, true, {
		"{ \"technology\": \"(%5C%5C|/|([a-zA-Z]))+\" }" } },
	{ key_engine_agent_response, agent_response, false, { "" } },
	{ key_engine_agent_cancel, agent_cancel, true, { "" } },
	{ key_engine_agent_retry, agent_error_response, false, { "" } },
	{ key_engine_scan_tech, scan_technology, true, {
		"{ \"technology\": \"(%5C%5C|/|([a-zA-Z]))+\" }" } },
	{ NULL, }, // this is a sentinel
};

/*
 * We can't set json_objects in the above declaration, so let's cheat:
 * we fill the gaps with json objects (generated in json_regex.{c,h})
 */
static void init_cmd_table(void)
{
	int i;

	for (i = 0; cmd_table[i].cmd; i++) {
		if (!cmd_table[i].trusted_is_json_string) {
			if (strncmp(key_engine_agent_response, cmd_table[i].cmd, 50) == 0)
				cmd_table[i].trusted.trusted_jobj = jregex_agent_response;

			else if (strncmp(key_engine_agent_retry, cmd_table[i].cmd, 50) == 0)
				cmd_table[i].trusted.trusted_jobj = jregex_agent_retry_response;
		}
	}
}

static int command_exist(const char *cmd)
{
	int res = -1, i;

	for (i = 0; ; i++) {

		if (cmd_table[i].cmd == NULL)
			break;

		if (strncmp(cmd_table[i].cmd, cmd, JSON_COMMANDS_STRING_SIZE_SMALL) == 0)
			res = i;
	}

	return res;
}

static bool command_data_is_clean(struct json_object *jobj, int cmd_pos)
{
	struct json_object *jcmd_data;
	bool res = false;

	if (cmd_table[cmd_pos].trusted_is_json_string)
		jcmd_data = json_tokener_parse(cmd_table[cmd_pos].trusted.trusted_str);
	else
		jcmd_data = cmd_table[cmd_pos].trusted.trusted_jobj;

	assert(jcmd_data);

	res = __json_type_dispatch(jobj, jcmd_data);
	json_object_put(jcmd_data);

	return res;
}

/*
  expected json:
  {
	"interface": STRING
	"path": STRING (dbus short name)
	"SIGNAL": STRING
	"cmd_data": OBJECT
  }
*/
static void engine_commands_sig(struct json_object *jobj)
{
	struct json_object *sig_name, *interface, *data, *path;
	const char *interface_str, *sig_name_str;
	json_bool exist;
	int pos;

	exist = json_object_object_get_ex(jobj, key_command_interface, &interface);
	assert(exist && interface != NULL);
	interface_str = json_object_get_string(interface);
	assert(interface_str != NULL);

	if (strcmp(interface_str, "Service") == 0)
		pos = 0;
	else if (strcmp(interface_str, "Technology") == 0)
		pos = 1;
	else // Manager
		pos = 2;

	json_object_object_get_ex(jobj, key_command_data, &data);
	json_object_object_get_ex(jobj, key_command_path, &path);
	json_object_object_get_ex(jobj, key_dbus_json_signal_key, &sig_name);
	sig_name_str = json_object_get_string(sig_name);

	subscribed_to[pos].react_to_sig(interface, path, data, sig_name_str);

	engine_callback(12345, jobj);
}

static void react_to_sig_service(struct json_object *interface,
			struct json_object *path, struct json_object *data,
			const char *sig_name)
{
	char serv_dbus_name[256];
	struct json_object *serv, *serv_dict, *val;
	const char *key;

	snprintf(serv_dbus_name, 256, "/net/connman/service/%s", json_object_get_string(path));
	serv_dbus_name[255] = '\0';
	serv = search_technology_or_service(services, serv_dbus_name);

	if (!serv)
		return;

	key = json_object_get_string(json_object_array_get_idx(data, 0));
	val = json_object_array_get_idx(data, 1);
	serv_dict = json_object_array_get_idx(serv, 1);

	if (serv_dict && json_object_object_get_ex(serv_dict, key, NULL)) {
		json_object_object_del(serv_dict, key);
		json_object_object_add(serv_dict, key, val);
	}
}

static void react_to_sig_technology(struct json_object *interface,
			struct json_object *path, struct json_object *data,
			const char *sig_name)
{
	char tech_dbus_name[256];
	struct json_object *tech, *tech_dict, *val;
	const char *key;

	snprintf(tech_dbus_name, 256, "/net/connman/technology/%s", json_object_get_string(path));
	tech_dbus_name[255] = '\0';
	tech = search_technology_or_service(technologies, tech_dbus_name);

	if (!tech)
		return;

	key = json_object_get_string(json_object_array_get_idx(data, 0));
	val = json_object_array_get_idx(data, 1);
	tech_dict = json_object_array_get_idx(tech, 1);

	if (tech_dict && json_object_object_get_ex(tech_dict, key, NULL)) {
		json_object_object_del(tech_dict, key);
		json_object_object_add(tech_dict, key, val);
	}
}

static void replace_service_in_services(const char *serv_name,
		struct json_object *serv_dict)
{
	struct json_object *sub_array, *tmp;
	int i, len;
	bool found = false;

	len = json_object_array_length(services);

	for (i = 0; i < len && !found; i++) {
		sub_array = json_object_array_get_idx(services, i);
		tmp = json_object_array_get_idx(sub_array, 0);

		if (strcmp(json_object_get_string(tmp), serv_name) == 0) {
			json_object_array_put_idx(sub_array, 1,
					json_object_get(serv_dict));
			found = true;
		}
	}

	if (!found) {
		tmp = json_object_new_array();
		json_object_array_add(tmp, json_object_new_string(serv_name));
		json_object_array_add(tmp, json_object_get(serv_dict));
		json_object_array_add(services, tmp);
	}
}

static void react_to_sig_manager(struct json_object *interface,
			struct json_object *path, struct json_object *data,
			const char *sig_name)
{
	const char *tmp_str;
	struct json_object *serv_to_del, *serv_to_add, *sub_array, *serv_dict,
			   *tmp, *tmp_array;
	int i, len;

	if (strcmp(sig_name, "ServicesChanged") == 0) {
		// remove services (they disappeared)
		serv_to_del = json_object_array_get_idx(data, 1);
		len = json_object_array_length(serv_to_del);

		for (i = 0; i < len; i++) {
			tmp_str = json_object_get_string(
					json_object_array_get_idx(serv_to_del, i));

			if (json_object_object_get_ex(services, tmp_str, NULL))
				json_object_object_del(services, tmp_str);
		}

		// add new services
		serv_to_add = json_object_array_get_idx(data, 0);
		len = json_object_array_length(serv_to_add);

		for (i = 0; i < len; i++) {
			sub_array = json_object_array_get_idx(serv_to_add, i);
			serv_dict = json_object_array_get_idx(sub_array, 1);

			// if the service have been "modified"
			if (json_object_array_length(serv_dict)) {
				tmp_str = json_object_get_string(
						json_object_array_get_idx(sub_array, 0));
				replace_service_in_services(tmp_str, serv_dict);
			}
		}

	} else if (strcmp(sig_name, "PropertyChanged") == 0) {
		/* state:
		 * {
		 *	"State": "online",
		 *	"OfflineMode": false,
		 *	"SessionMode": false
		 * }
		 */
		tmp_str = json_object_get_string(json_object_array_get_idx(data,
					0));
		json_object_object_del(state, tmp_str);
		json_object_object_add(state, tmp_str,
				json_object_array_get_idx(data, 1));

	} else if (strcmp(sig_name, "TechnologyAdded") == 0) {
		json_object_array_add(technologies, json_object_get(data));

	} else if (strcmp(sig_name, "TechnologyRemoved") == 0) {
		tmp_str = json_object_get_string(data);
		tmp_array = json_object_new_array();
		len = json_object_array_length(technologies);

		for (i = 0; i < len; i++) {
			sub_array = json_object_array_get_idx(technologies, i);
			assert(sub_array != NULL);
			tmp = json_object_array_get_idx(sub_array, 0);
			assert(tmp != NULL);

			if (strcmp(tmp_str, json_object_get_string(tmp)) != 0) {
				json_object_array_add(tmp_array, sub_array);
			}
		}

		//json_object_put(technologies);
		technologies = tmp_array;
		json_object_get(technologies);
	}

	// We ignore PeersChanged: we don't support P2P
}

int engine_query(struct json_object *jobj)
{
	const char *command_str = NULL;
	int res, cmd_pos;
	struct json_object *jcmd_data;

	command_str = __json_get_command_str(jobj);

	if (!command_str || (cmd_pos = command_exist(command_str)) < 0)
		return -EINVAL;

	json_object_object_get_ex(jobj, key_command_data, &jcmd_data);

	if (jcmd_data != NULL && !command_data_is_clean(jcmd_data, cmd_pos))
		return -EINVAL;

	res = cmd_table[cmd_pos].func(jcmd_data);
	json_object_put(jobj);

	return res;
}

int engine_init(void)
{
	DBusError dbus_err;
	struct json_object *jobj, *jarray;
	int res = 0;

	// Getting dbus connection
	dbus_error_init(&dbus_err);
	connection = dbus_bus_get(DBUS_BUS_SYSTEM, &dbus_err);

	if (dbus_error_is_set(&dbus_err)) {
		printf("\n[-] Error getting connection: %s\n", dbus_err.message);
		dbus_error_free(&dbus_err);
		return -1;
	}

	dbus_error_init(&dbus_err);
	agent_dbus_conn = dbus_bus_get(DBUS_BUS_SYSTEM, &dbus_err);

	if (dbus_error_is_set(&dbus_err)) {
		printf("\n[-] Error getting agent_dbus_conn: %s\n", dbus_err.message);
		dbus_error_free(&dbus_err);
		return -1;
	}

	commands_callback = engine_commands_cb;
	commands_signal = engine_commands_sig;
	agent_callback = engine_agent_cb;
	agent_error_callback = engine_agent_error_cb;

	jobj = json_object_new_object();
	jarray = json_object_new_array();
	json_object_array_add(jarray, json_object_new_string("Manager"));
	json_object_array_add(jarray, json_object_new_string("Service"));
	json_object_array_add(jarray, json_object_new_string("Technology"));
	json_object_object_add(jobj, "monitor_add", jarray);

	res = __cmd_monitor(jobj);
	json_object_put(jobj);

	if (res != -EINPROGRESS)
		return res;

	// We need the loop to get callbacks to init our things
	loop_init();
	init_status = INIT_STATE;
	res = get_state(NULL);

	if (res != -EINPROGRESS)
		return res;

	loop_run(false);
	init_status = INIT_TECHNOLOGIES;

	if ((res = get_technologies(NULL)) != -EINPROGRESS)
		return res;

	loop_run(false);
	init_status = INIT_SERVICES;

	if ((res = get_services(NULL)) != -EINPROGRESS)
		return res;

	loop_run(false);
	init_status = INIT_OVER;

	agent_register(agent_dbus_conn);
	agent_data_cache = NULL;
	generate_trusted_json();
	init_cmd_table();

	return 0;
}

void engine_terminate(void)
{
	json_object_put(technologies);
	json_object_put(services);
	agent_unregister(agent_dbus_conn, NULL);
	free_trusted_json();
}

