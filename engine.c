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

#include "commands.h"
#include "json_utils.h"
#include "loop.h"

#include "engine.h"

DBusConnection *connection;

extern void (*engine_callback)(int status, struct json_object *jobj);

void (*commands_callback)(struct json_object *data, json_bool is_error) = NULL;
void (*commands_signal)(struct json_object *data) = NULL;
void (*agent_callback)(struct json_object *data, struct agent_data *request) = NULL;
void (*agent_error_callback)(struct json_object *data, struct agent_data *request) = NULL;

static enum {INIT_STATE, INIT_TECHNOLOGIES, INIT_SERVICES, INIT_OVER} init_status = 0;

static struct json_object *state, *technologies, *services, *home_page;

void engine_commands_cb(struct json_object *data, json_bool is_error)
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
			engine_callback((is_error ? 1 : 0), data);
			break;
	}

	if (init_status != INIT_OVER)
		loop_quit();
}

void engine_commands_sig(struct json_object *data)
{
	engine_callback(12345, data);
}

void engine_agent_cb(struct json_object *data, struct agent_data *request)
{
	engine_callback(-ENOSYS, NULL);
}

void engine_agent_error_cb(struct json_object *data, struct agent_data *request)
{
	engine_callback(-ENOSYS, NULL);
}

static _Bool technology_or_service_exist(_Bool is_technology, const char *cmd)
{
	int len, i;
	_Bool found = false;
	struct json_object *sub_array, *name, *ressource;
	const char *bus_name;

	if (is_technology)
		ressource = technologies;
	else
		ressource = services;

	len = json_object_array_length(ressource);

	for (i = 0; i < len && !found; i++) {
		sub_array = json_object_array_get_idx(ressource, i);
		name = json_object_array_get_idx(sub_array, 0);
		bus_name = json_object_get_string(name);

		if (strncmp(bus_name, cmd, strnlen(bus_name, 256)) == 0)
			found = true;
	}

	return found;
}

/*
 {
	"command_name": <cmd_name>,
	"data": { <data> }
 }
 */
static struct json_object* coating(const char *cmd_name,
		struct json_object *data)
{
	struct json_object *res = json_object_new_object();
	struct json_object *tmp = json_object_get(data);

	json_object_object_add(res, "command_name",
			json_object_new_string(cmd_name));
	json_object_object_add(res, "data", tmp);

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

static void compose_home_page(void)
{
	home_page = json_object_new_object();
	json_object_object_add(home_page, "state", state);
	json_object_object_add(home_page, "technologies", technologies);
}

/*
 {
 	"command_name": "get_home_page",
	"data": {
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
	assert(home_page != NULL);
	engine_callback(0, coating("get_home_page",
				json_object_get(home_page)));

	return -EINPROGRESS;
}

/*
 "\/net\/connman\/technology\/ethernet" -> { "Name":"Wired", ...  }
 */
static struct json_object* get_dict_of_technology(const char *technology)
{
	struct json_object *array, *tmp;
	int len, i;

	len = json_object_array_length(technologies);

	for (i = 0; i < len; i++) {
		array = json_object_array_get_idx(technologies, i);
		tmp = json_object_array_get_idx(array, 0);

		if (strncmp(json_object_get_string(tmp), technology, 256) == 0)
			return json_object_array_get_idx(array, 1);
	}

	return NULL;
}

static struct json_object* get_services_matching_tech_type(const char
		*technology, _Bool is_connected)
{
	struct json_object *array_serv, *serv_dict, *serv_type, *res;
	struct json_object *serv_state;
	int len, i;

	res = json_object_new_array();
	len = json_object_array_length(services);

	for (i = 0; i < len; i++) {
		array_serv = json_object_array_get_idx(services, i);
		serv_dict = json_object_array_get_idx(array_serv, 1);
		json_object_object_get_ex(serv_dict, "Type", &serv_type);

		if (strncmp(json_object_get_string(serv_type), technology, 256) == 0) {
			json_object_object_get_ex(serv_dict, "State", &serv_state);

			if (is_connected && strncmp(json_object_get_string(serv_state), "online", 256) != 0)
				break;

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

	json_object_object_get_ex(jobj, "technology", &tmp);
	tech_dbus_name = json_object_get_string(tmp);

	if (!technology_or_service_exist(true, tech_dbus_name))
		return -EINVAL;

	tech_dict = get_dict_of_technology(tech_dbus_name);
	json_object_object_get_ex(tech_dict, "Type", &jtech_type);
	tech_type = json_object_get_string(jtech_type);
	json_object_object_get_ex(tech_dict, "Connected", &tech_co);

	res_serv = get_services_matching_tech_type(tech_type,
			(json_object_get_boolean(tech_co) ? true : false));
	res_tech = json_object_new_array();
	json_object_array_add(res_tech, tmp);
	json_object_array_add(res_tech, tech_dict);

	res = json_object_new_object();
	json_object_object_add(res, "services", res_serv);
	json_object_object_add(res, "technology", res_tech);
	engine_callback(0, coating("get_services_from_tech", res));

	return -EINPROGRESS;
}

static const struct {
	const char *cmd;
	int (*func)(struct json_object *jobj);
	_Bool trusted_is_json_string;
	union {
		const char *trusted_str;
		struct json_object *trusted_jobj;
	} trusted;
} cmd_table[] = {
	{ "get_state", get_state, true, { "" } },
	{ "get_services", get_services, true, { "" } },
	{ "get_technologies", get_technologies, true, { "" } },
	{ "get_home_page", get_home_page, true, { "" } },
	{ "get_services_from_tech", get_services_from_tech, true, {
	"{ \"technology\": \"(%5C%5C|/|([a-zA-Z]))+\" }" } },
	{ NULL, }, // this is a sentinel
};

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

static _Bool command_data_is_clean(struct json_object *jobj, int cmd_pos)
{
	struct json_object *jcmd_data;
	_Bool res = false;

	if (cmd_table[cmd_pos].trusted_is_json_string)
		jcmd_data = json_tokener_parse(cmd_table[cmd_pos].trusted.trusted_str);
	else 
		jcmd_data = cmd_table[cmd_pos].trusted.trusted_jobj;

	assert(jcmd_data);

	res = __json_type_dispatch(jobj, jcmd_data);
	json_object_put(jcmd_data);

	return res;
}

int __engine_query(struct json_object *jobj)
{
	const char *command_str = NULL;
	int res, cmd_pos;
	struct json_object *jcmd_data;

	command_str = __json_get_command_str(jobj);

	if (!command_str || (cmd_pos = command_exist(command_str)) < 0)
		return -EINVAL;
	
	json_object_object_get_ex(jobj, ENGINE_KEY_CMD_DATA, &jcmd_data);

	if (jcmd_data != NULL && !command_data_is_clean(jcmd_data, cmd_pos))
		return -EINVAL;
	
	res = cmd_table[cmd_pos].func(jcmd_data);
	json_object_put(jobj);

	return res;
}

int __engine_init(void)
{
	DBusError dbus_err;
	//struct json_object *jobj, *jarray;
	int res = 0;

	// Getting dbus connection
	dbus_error_init(&dbus_err);
	connection = dbus_bus_get(DBUS_BUS_SYSTEM, &dbus_err);

	if (dbus_error_is_set(&dbus_err)) {
		printf("\n[-] Error getting connection: %s\n", dbus_err.message);
		dbus_error_free(&dbus_err);
		return -1;
	}

	commands_callback = engine_commands_cb;
	commands_signal = engine_commands_sig;
	agent_callback = engine_agent_cb;
	agent_error_callback = engine_agent_error_cb;

	/*
	jobj = json_object_new_object();
	jarray = json_object_new_array();
	json_object_array_add(jarray, json_object_new_string("Manager"));
	json_object_object_add(jobj, "monitor_add", jarray);

	if (__cmd_monitor(jobj) != -EINPROGRESS)
		printf("[-] @__engine_init: Couldn't monitor Manager.\n");
	
	json_object_put(jobj);
	*/
	
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
	compose_home_page();

	return 0;
}

void __engine_terminate(void)
{
	json_object_put(home_page);
	json_object_put(services);
}
