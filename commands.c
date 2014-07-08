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

#include "commands.h"

extern DBusConnection *connection;

extern void (*commands_callback)(struct json_object *data, json_bool is_error);

extern void (*commands_signal)(struct json_object *data);

static bool check_dbus_name(const char *name)
{
	/*
	 * Valid dbus chars should be [A-Z][a-z][0-9]_
	 * and should not start with number.
	 */
	unsigned int i;

	if (!name || name[0] == '\0')
		return false;

	for (i = 0; name[i] != '\0'; i++)
		if (!((name[i] >= 'A' && name[i] <= 'Z') ||
					(name[i] >= 'a' && name[i] <= 'z') ||
					(name[i] >= '0' && name[i] <= '9') ||
					name[i] == '_'))
			return false;

	return true;
}

static char* get_path(char *full_path)
{
	char *path;

	path = strrchr(full_path, '/');
	if (path && *path != '\0')
		path++;
	else
		path = full_path;

	return path;
}

static void call_return_list(DBusMessageIter *iter, const char *error,
		void *user_data)
{
	struct json_object *res, *array;
	json_bool jerror;

	if (error) {
		res = json_object_new_object();
		array = json_object_new_array();

		if (user_data)
			json_object_array_add(array, json_object_new_string(user_data));

		json_object_array_add(array, json_object_new_string(error));
		json_object_object_add(res, DBUS_JSON_ERROR_KEY, array);
		jerror = TRUE;

	} else {
		res = __connman_dbus_to_json(iter);
		jerror = FALSE;
	}

	commands_callback(res, jerror);
}

static void call_return_list_free(DBusMessageIter *iter,
		const char *error, void *user_data)
{
	call_return_list(iter, error, get_path(user_data));
	free(user_data);
}

/*
 	"valid_technology" | offline
 */
static int cmd_enable(struct json_object *jobj)
{
	char *tech;
	const char *arg = json_object_get_string(jobj);
	dbus_bool_t b = TRUE;

	if (check_dbus_name(arg) == false)
		return -EINVAL;

	if (strcmp(arg, "offline") == 0) {
		tech = strndup(arg, JSON_COMMANDS_STRING_SIZE_SMALL);
		return __connman_dbus_set_property(connection, "/",
				"net.connman.Manager", call_return_list_free,
				tech, "OfflineMode", DBUS_TYPE_BOOLEAN, &b);
	}

	tech = malloc(JSON_COMMANDS_STRING_SIZE_MEDIUM + 1);
	snprintf(tech, JSON_COMMANDS_STRING_SIZE_MEDIUM,
			"/net/connman/technology/%s", arg);
	tech[JSON_COMMANDS_STRING_SIZE_MEDIUM] = '\0';

	return __connman_dbus_set_property(connection, tech,
			"net.connman.Technology", call_return_list_free, tech,
			"Powered", DBUS_TYPE_BOOLEAN, &b);
}

/*
 	"valid_technology" | offline
 */
static int cmd_disable(struct json_object *jobj)
{
	char *tech;
	const char *arg = json_object_get_string(jobj);
	dbus_bool_t b = FALSE;

	if (check_dbus_name(arg) == false)
		return -EINVAL;

	if (strcmp(arg, "offline") == 0) {
		tech = strndup(arg, JSON_COMMANDS_STRING_SIZE_SMALL);
		return __connman_dbus_set_property(connection, "/",
				"net.connman.Manager", call_return_list_free,
				tech, "OfflineMode", DBUS_TYPE_BOOLEAN, &b);
	}

	tech = malloc(JSON_COMMANDS_STRING_SIZE_MEDIUM + 1);
	snprintf(tech, JSON_COMMANDS_STRING_SIZE_MEDIUM,
			"/net/connman/technology/%s", arg);
	tech[JSON_COMMANDS_STRING_SIZE_MEDIUM] = '\0';

	return __connman_dbus_set_property(connection, tech,
			"net.connman.Technology", call_return_list_free, tech,
			"Powered", DBUS_TYPE_BOOLEAN, &b);
}

int __cmd_state(void)
{
	return __connman_dbus_method_call(connection, CONNMAN_SERVICE,
			CONNMAN_PATH, "net.connman.Manager", "GetProperties",
			call_return_list, NULL, NULL, NULL);
}

int __cmd_services(void)
{
	return __connman_dbus_method_call(connection, CONNMAN_SERVICE,
			CONNMAN_PATH, "net.connman.Manager", "GetServices",
			call_return_list, NULL, NULL, NULL);
}

int __cmd_technologies(void)
{
	return __connman_dbus_method_call(connection, CONNMAN_SERVICE,
			CONNMAN_PATH, "net.connman.Manager", "GetTechnologies",
			call_return_list, NULL,	NULL, NULL);
}

/*
 	"valid_technology"
 */
static int cmd_scan(struct json_object *jobj)
{
	char *path;
	const char *arg = json_object_get_string(jobj);

	path = malloc(JSON_COMMANDS_STRING_SIZE_MEDIUM + 1);
	snprintf(path, JSON_COMMANDS_STRING_SIZE_MEDIUM,
			"/net/connman/technology/%s", arg);
	path[JSON_COMMANDS_STRING_SIZE_MEDIUM] = '\0';

	return __connman_dbus_method_call(connection, CONNMAN_SERVICE,
			path, "net.connman.Technology", "Scan",
			call_return_list_free, path, NULL, NULL);
}

/*
 	"valid_service"
 */
static int cmd_connect(struct json_object *jobj)
{
	char *path;
	const char *arg = json_object_get_string(jobj);

	if (check_dbus_name(arg) == false)
		return -EINVAL;

	path = malloc(JSON_COMMANDS_STRING_SIZE_MEDIUM + 1);
	snprintf(path, JSON_COMMANDS_STRING_SIZE_MEDIUM,
			"/net/connman/service/%s", arg);
	path[JSON_COMMANDS_STRING_SIZE_MEDIUM] = '\0';

	return __connman_dbus_method_call(connection, CONNMAN_SERVICE, path,
			"net.connman.Service", "Connect", call_return_list_free,
			path, NULL, NULL);
}

/*
 	"valid_service"
 */
static int cmd_disconnect(struct json_object *jobj)
{
	char *path;
	const char *arg = json_object_get_string(jobj);

	if (check_dbus_name(arg) == false)
		return -EINVAL;

	path = malloc(JSON_COMMANDS_STRING_SIZE_MEDIUM + 1);
	snprintf(path, JSON_COMMANDS_STRING_SIZE_MEDIUM,
			"/net/connman/service/%s", arg);
	path[JSON_COMMANDS_STRING_SIZE_MEDIUM] = '\0';

	return __connman_dbus_method_call(connection, CONNMAN_SERVICE, path,
			"net.connman.Service", "Disconnect",
			call_return_list_free, path, NULL, NULL);
}

/*
 	"valid_service"
 */
static int cmd_remove(struct json_object *jobj)
{
	char *path;
	const char *arg = json_object_get_string(jobj);

	if (check_dbus_name(arg) == false)
		return -EINVAL;

	path = malloc(JSON_COMMANDS_STRING_SIZE_MEDIUM + 1);
	snprintf(path, JSON_COMMANDS_STRING_SIZE_MEDIUM,
			"/net/connman/service/%s", arg);
	path[JSON_COMMANDS_STRING_SIZE_MEDIUM] = '\0';

	return __connman_dbus_method_call(connection, CONNMAN_SERVICE, path,
			"net.connman.Service", "Remove", call_return_list_free,
			path, NULL, NULL);
}

static void config_append_ipv4(DBusMessageIter *iter,
		struct json_object *jobj)
{
	const char *str;
	char *buf;

	json_object_object_foreach(jobj, key, val) {
		str = json_object_get_string(val);
		buf = malloc(sizeof(char) * JSON_COMMANDS_STRING_SIZE_SMALL);
		strncpy(buf, str, JSON_COMMANDS_STRING_SIZE_SMALL);

		__connman_dbus_append_dict_entry(iter, key, DBUS_TYPE_STRING,
				&buf);
		free(buf);
	}
}

static void config_append_ipv6(DBusMessageIter *iter,
		struct json_object *jobj)
{
	struct json_object *methobj;
	const char *method, *str;
	char *buf;

	if (!json_object_object_get_ex(jobj, "Method", &methobj)) {
		call_return_list(NULL, "No 'Method' set", "");
		return;
	}
	method = json_object_get_string(methobj);

	if (strcmp("6to4", method) == 0) {
		call_return_list(NULL, "Cannot be set by user", "Service set"
				" properties.");
		return;
	}

	json_object_object_foreach(jobj, key, val) {
		if (strcmp("PrefixLength", key) == 0) {
			int tmp = json_object_get_int(val);
			__connman_dbus_append_dict_entry(iter, key,
					DBUS_TYPE_BYTE, &tmp);
		} else {
			str = json_object_get_string(val);
			buf = malloc(sizeof(char) *
					JSON_COMMANDS_STRING_SIZE_SMALL);
			strncpy(buf, str, JSON_COMMANDS_STRING_SIZE_SMALL);

			__connman_dbus_append_dict_entry(iter, key,
					DBUS_TYPE_STRING, &buf);

			free(buf);
		}
	}
}

static void config_append_json_array_of_strings(DBusMessageIter *iter,
		struct json_object *jobj)
{
	struct json_object *strobj;
	int i, len;
	const char *str;
	char *buf;

	if (jobj && json_object_is_type(jobj, json_type_array)) {
		len = json_object_array_length(jobj);
		for (i = 0; i < len; i++) {
			strobj = json_object_array_get_idx(jobj, i);
			if (strobj) {
				str = json_object_get_string(strobj);
				buf = malloc(sizeof(char) *
						JSON_COMMANDS_STRING_SIZE_SMALL);
				strncpy(buf, str,
						JSON_COMMANDS_STRING_SIZE_SMALL);
				dbus_message_iter_append_basic(iter,
						DBUS_TYPE_STRING,
						&buf);
				free(buf);
			}
		}
	}
}

static void config_append_proxy(DBusMessageIter *iter,
		struct json_object *jobj)
{
	struct json_object *methobj, *urlobj, *tmpobj;
	const char *method, *url;
	char *buf;
	int method_len;

	if (!json_object_object_get_ex(jobj, "Method", &methobj)) {
		call_return_list(NULL, "No 'Method' set", "");
		return;
	}
	method = json_object_get_string(methobj);

	if (strcmp(method, "manual") == 0) {
		if (!json_object_object_get_ex(jobj, "Servers", &tmpobj))
			tmpobj = 0;
		__connman_dbus_append_dict_string_array(iter, "Servers",
				config_append_json_array_of_strings, tmpobj);

		if (!json_object_object_get_ex(jobj, "Excludes", &tmpobj))
			tmpobj = 0;
		__connman_dbus_append_dict_string_array(iter, "Excludes",
				config_append_json_array_of_strings, tmpobj);
	} else if (strcmp(method, "auto") == 0) {
		if (json_object_object_get_ex(jobj, "URL", &urlobj)) {
			url = json_object_get_string(urlobj);
			if (url)
				__connman_dbus_append_dict_entry(iter, "URL",
						DBUS_TYPE_STRING, url);
		}
	} else if (strcmp(method, "direct") != 0)
		return;

	method_len = strlen(method) + 1;
	buf = malloc(sizeof(char) * method_len);
	strncpy(buf, method, method_len);

	__connman_dbus_append_dict_entry(iter, "Method", DBUS_TYPE_STRING,
			&buf);
	free(buf);
}

/*
   {
   "service": "wifi_8888_8888_none",
   "options": {
   "IPv4": {
   "Method": "dhcp"|"manual"|"off",
   "Address": "xxxx",
   "Netmask": "xxxx",
   "Gateway": "xxxx"
   },
   "IPv6": {
   "Method": "auto"|"manual"|"6to4"|"off",
   "Address": "xxxx",
   "PrefixLength": 12,
   "Gateway": "xxxx",
   "Privacy": "auto"|"disabled"|"enabled"|"prefered"
   },
   "Proxy":{
   "Method": "direct"|"auto"|"manual",
   "URL": "proxyurl",
   "Servers": [ "proxyserver1", "proxyserver1" ],
   "Excludes": [ "proxyserver1" ]
   },
   "AutoConnect": FALSE|TRUE,
   "Domains": [ "domainserver1", "domainserver2" ],
   "Nameservers": [ "nameserver1", "nameserver2" ],
   "Timeservers": [ "timeserver1", "timeserver2" ]
   }
   }
 *
 * Note that option names are the same as the ones in the doc/services-api.txt
 */
static int cmd_config(struct json_object *jobj)
{
	int res = 0;
	const char *service_name;
	char *path, *simple_service_conf, *dyn_service_name;
	dbus_bool_t dbus_bool;
	struct json_object *options, *srvobj;

	if (!json_object_object_get_ex(jobj, "service", &srvobj)) {
		call_return_list(NULL, "No 'service' set", "");
		return -EINVAL;
	}

	service_name = json_object_get_string(srvobj);
	if (!service_name || check_dbus_name(service_name) == false) {
		call_return_list(NULL, "Wrong service name",
				"Service set properties.");
		return -EINVAL;
	}

	if (!json_object_object_get_ex(jobj, "options", &options)) {
		call_return_list(NULL, "No options provided for service "
				"configuration", "");
		return -EINVAL;
	}

	json_object_object_foreach(options, key, val) {
		simple_service_conf = NULL;
		path = malloc(JSON_COMMANDS_STRING_SIZE_MEDIUM + 1);
		snprintf(path, JSON_COMMANDS_STRING_SIZE_MEDIUM,
				"/net/connman/service/%s", service_name);
		path[JSON_COMMANDS_STRING_SIZE_MEDIUM] = '\0';

		dyn_service_name = strndup(service_name,
				JSON_COMMANDS_STRING_SIZE_MEDIUM);

		if (strcmp("IPv4", key) == 0) {
			res = __connman_dbus_set_property_dict(connection,
					path, "net.connman.Service",
					call_return_list_free, dyn_service_name,
					"IPv4.Configuration", DBUS_TYPE_STRING,
					config_append_ipv4, val);

		} else if (strcmp("IPv6", key) == 0) {
			res = __connman_dbus_set_property_dict(connection,
					path, "net.connman.Service",
					call_return_list_free, dyn_service_name,
					"IPv6.Configuration", DBUS_TYPE_STRING,
					config_append_ipv6, val);

		} else if (strcmp("Proxy", key) == 0) {
			res = __connman_dbus_set_property_dict(connection,
					path, "net.connman.Service",
					call_return_list_free, dyn_service_name,
					"Proxy.Configuration", DBUS_TYPE_STRING,
					config_append_proxy, val);

		} else if (strcmp("AutoConnect", key) == 0) {

			if (json_object_get_boolean(val) == TRUE)
				dbus_bool = TRUE;
			else
				dbus_bool = FALSE;

			res = __connman_dbus_set_property(connection, path,
					"net.connman.Service", call_return_list_free,
					dyn_service_name, "AutoConnect",
					DBUS_TYPE_BOOLEAN, &dbus_bool);

		} else if (strcmp("Domains", key) == 0)
			simple_service_conf = "Domains.Configuration";

		else if (strcmp("Nameservers", key) == 0)
			simple_service_conf = "Nameservers.Configuration";

		else if (strcmp("Timeservers", key) == 0)
			simple_service_conf = "Timeservers.Configuration";
		else {
			char *str = strndup(key, JSON_COMMANDS_STRING_SIZE_SMALL);
			call_return_list_free(NULL, "Unknown configuration key", str);
			res = -EINVAL;
		}

		if (simple_service_conf != NULL) {
			res = __connman_dbus_set_property_array(connection,
					path, "net.connman.Service",
					call_return_list_free,
					dyn_service_name,
					simple_service_conf, DBUS_TYPE_STRING,
					config_append_json_array_of_strings, val);
		}

		free(path);
		simple_service_conf = NULL;

		if (res < 0 && res != -EINPROGRESS)
			return res;
	}

	return res;
}

static DBusHandlerResult monitor_changed(DBusConnection *connection,
		DBusMessage *message, void *user_data)
{
	DBusMessageIter iter;
	const char *interface, *path;
	struct json_object *res, *sig_name;

	interface = dbus_message_get_interface(message);
	if (!interface)
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

	if (strncmp(interface, "net.connman.", 12) != 0)
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

	if (!strcmp(interface, "net.connman.Agent") ||
			!strcmp(interface, "net.connman.Session") ||
			!strcmp(interface, "net.connman.Notification"))
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

	interface = strrchr(interface, '.');
	if (interface && *interface != '\0')
		interface++;

	path = strrchr(dbus_message_get_path(message), '/');
	if (path && *path != '\0')
		path++;

	if (dbus_message_is_signal(message, "net.connman.Manager",
				"ServicesChanged")) {
		sig_name = json_object_new_string("ServicesChanged");

	} else if (dbus_message_is_signal(message, "net.connman.Manager",
				"PropertyChanged")) {
		sig_name = json_object_new_string("PropertyChanged");

	} else if (dbus_message_is_signal(message, "net.connman.Manager",
				"TechnologyAdded")) {
		path = dbus_message_get_member(message);
		sig_name = json_object_new_string("TechnologyAdded");

	} else if (dbus_message_is_signal(message, "net.connman.Manager",
				"TechnologyRemoved")) {
		path = dbus_message_get_member(message);
		sig_name = json_object_new_string("TechnologyRemoved");

	} else if (dbus_message_is_signal(message, "net.connman.Service",
				"PropertyChanged")) {
		path = dbus_message_get_member(message);
		sig_name = json_object_new_string("PropertyChanged");

	} else if (dbus_message_is_signal(message, "net.connman.Technology",
				"PropertyChanged")) {
		path = dbus_message_get_member(message);
		sig_name = json_object_new_string("PropertyChanged");

	} else {
		sig_name = json_object_new_string("Signal unsupported");
	}

	dbus_message_iter_init(message, &iter);
	res = json_object_new_object();

	json_object_object_add(res, "interface",
			json_object_new_string(interface));
	json_object_object_add(res, "path",
			json_object_new_string(path));
	json_object_object_add(res, "data", __connman_dbus_to_json(&iter));

	json_object_object_add(res, DBUS_JSON_SIGNAL_KEY, sig_name);
	json_object_get(res);

	commands_signal(res);

	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static struct {
	char *interface;
	bool enabled;
} monitor[] = {
	{ "Service", false },
	{ "Technology", false },
	{ "Manager", false },
	{ NULL, },
};

static void monitor_add(const char *interface)
{
	bool add_filter = true, found = false;
	int i;
	char *rule;
	DBusError err;

	if (!interface)
		return;

	for (i = 0; monitor[i].interface; i++) {
		if (monitor[i].enabled == true)
			add_filter = false;

		if (strncmp(interface, monitor[i].interface,
					JSON_COMMANDS_STRING_SIZE_SMALL) == 0) {
			if (monitor[i].enabled == true)
				return;

			monitor[i].enabled = true;
			found = true;
		}
	}

	if (found == false)
		return;

	if (add_filter == true)
		dbus_connection_add_filter(connection, monitor_changed,
				NULL, NULL);

	dbus_error_init(&err);

	rule = malloc(JSON_COMMANDS_STRING_SIZE_MEDIUM + 1);
	snprintf(rule, JSON_COMMANDS_STRING_SIZE_MEDIUM,
			"type='signal',interface='net.connman.%s'", interface);
	rule[JSON_COMMANDS_STRING_SIZE_MEDIUM] = '\0';
	dbus_bus_add_match(connection, rule, &err);
	free(rule);

	if (dbus_error_is_set(&err))
		call_return_list(NULL, err.message, "");
}

static void monitor_del(const char *interface)
{
	bool del_filter = true, found = false;
	int i;
	char *rule;
	DBusError err;

	if (!interface)
		return;

	for (i = 0; monitor[i].interface; i++) {
		if (strncmp(interface, monitor[i].interface,
					JSON_COMMANDS_STRING_SIZE_SMALL) == 0) {
			if (monitor[i].enabled == false)
				return;

			monitor[i].enabled = false;
			found = true;
		}

		if (monitor[i].enabled == true)
			del_filter = false;
	}

	if (found == false)
		return;

	dbus_error_init(&err);
	rule = malloc(JSON_COMMANDS_STRING_SIZE_MEDIUM + 1);
	snprintf(rule, JSON_COMMANDS_STRING_SIZE_MEDIUM,
			"type='signal',interface='net.connman.%s'", interface);
	rule[JSON_COMMANDS_STRING_SIZE_MEDIUM] = '\0';
	dbus_bus_remove_match(connection, rule, &err);
	free(rule);

	if (dbus_error_is_set(&err))
		call_return_list(NULL, err.message, "");

	if (del_filter == true)
		dbus_connection_remove_filter(connection, monitor_changed,
				NULL);
}

/*
   {
   "monitor_add": [ "Service" ... ],
   "monitor_del": [ "Manager" ... ]
   }
 */
int __cmd_monitor(struct json_object *jobj)
{
	struct json_object *tmp;
	const char *interface;
	int i;

	if (json_object_object_get_ex(jobj, "monitor_add", &tmp)) {
		for (i = 0; i < json_object_array_length(tmp); i++) {
			interface = json_object_get_string(
					json_object_array_get_idx(tmp, i));
			monitor_add(interface);
		}
	}

	if (json_object_object_get_ex(jobj, "monitor_del", &tmp)) {
		for (i = 0; i < json_object_array_length(tmp); i++) {
			interface = json_object_get_string(
					json_object_array_get_idx(tmp, i));
			monitor_del(interface);
		}
	}

	return -EINPROGRESS;
}

/*
   {
   "command": "monitor",
   "data": {...}
   }
 */
int __connman_command_dispatcher(DBusConnection *dbus_conn,
		struct json_object *jobj)
{

	struct json_object *data, *tmp;
	const char *command;
	int res;

	connection = dbus_conn;

	if (!json_object_object_get_ex(jobj, "command", &tmp)) {
		call_return_list(NULL, "No 'command' set", "");
		json_object_put(jobj);
		return -EINVAL;
	}

	command = json_object_get_string(tmp);

	if (!command || command[0] == '\0') {
		call_return_list(NULL, "Unknown command", "");
		json_object_put(jobj);
		return -EINVAL;
	}

	if (!json_object_object_get_ex(jobj, "data", &data))
		data = NULL;

	if (strcmp(command, "config") == 0)
		res = cmd_config(data);

	else if (strcmp(command, "remove") == 0)
		res = cmd_remove(data);

	else if (strcmp(command, "disconnect") == 0)
		res = cmd_disconnect(data);

	else if (strcmp(command, "connect") == 0)
		res = cmd_connect(data);

	else if (strcmp(command, "scan") == 0)
		res = cmd_scan(data);

	/*
	else if (strcmp(command, "technologies") == 0)
		res = cmd_technologies();

	else if (strcmp(command, "services") == 0)
		res = cmd_services();

	else if (strcmp(command, "state") == 0)
		res = cmd_state();
	*/

	else if (strcmp(command, "disable") == 0)
		res = cmd_disable(data);

	else if (strcmp(command, "enable") == 0)
		res = cmd_enable(data);

	else if (strcmp(command, "agent_register") == 0)
		res = __connman_agent_register(connection);

	else if (strcmp(command, "agent_unregister") == 0) {
		__connman_agent_unregister(connection, NULL);
		res = 0;
	}

	/*
	else if (strcmp(command, "monitor") == 0)
		res = cmd_monitor(data);
	*/

	else {
		res = -EINVAL;
		call_return_list(NULL, "Unknown command", "");
	}

	json_object_put(jobj);

	return res;
}
