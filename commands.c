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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/types.h>
#include <unistd.h>

#include "dbus_helpers.h"
#include "agent.h"
#include "dbus_json.h"
#include "engine.h"
#include "keys.h"
#include "string_utils.h"

#include "commands.h"

/*
 * This file is a facilitator between the engine and then connman dbus service.
 */

extern DBusConnection *connection;

// Callback called when a command return.
extern void (*commands_callback)(struct json_object *data, json_bool is_error);
void (*commands_callback)(struct json_object *data, json_bool is_error) = NULL;

// Callback called when a signal is emitted.
extern void (*commands_signal)(struct json_object *data);
void (*commands_signal)(struct json_object *data) = NULL;

/*
 * Get called when a connman dbus method return. This function format parameters
 * and "forward" the callback to commands_callback.
 *
 * Format of the callback:
 *	- error: { key_error: [ "error string" ] }
 *	- success: { key_command: "command string", ... }
 *	- with user data: { ... , key_return_force_refresh: "user data string" }
 *
 * Note: setting user data will force refresh in context CONTEXT_SERVICES.
 *
 * @param iter answer to the command
 * @param error if an error occured, this will be filled with the appropriate
 *		error message
 * @param user_data data passed by the user while calling the method
 */
static void call_return_list(DBusMessageIter *iter, const char *error,
		void *user_data)
{
	struct json_object *res, *array;
	json_bool jerror;

	if (error) {
		res = json_object_new_object();
		array = json_object_new_array();

		json_object_array_add(array, json_object_new_string(error));
		json_object_object_add(res, key_error, array);
		jerror = TRUE;

	} else {
		res = dbus_to_json(iter);
		jerror = FALSE;

		if (!res)
			res = json_object_new_object();
	}

	if (user_data)
		json_object_object_add(res, key_return_force_refresh,
			json_object_new_string((const char*)user_data));

	commands_callback(res, jerror);
}

/*
 * Invoke call_return_list then free user_data.
 */
static void call_return_list_free(DBusMessageIter *iter,
		const char *error, void *user_data)
{
	char *dbus_short_name = extract_dbus_short_name(user_data);

	call_return_list(iter, error, dbus_short_name);

	if (user_data)
		free(user_data);

	if (dbus_short_name)
		free(dbus_short_name);
}

/*
 * Call the Manager GetProperties method.
 */
int __cmd_state(void)
{
	return dbus_method_call(connection, key_connman_service,
			key_connman_path, key_manager_interface, "GetProperties",
			call_return_list, NULL, NULL, NULL);
}

/*
 * Call the Manager GetServices method.
 */
int __cmd_services(void)
{
	return dbus_method_call(connection, key_connman_service,
			key_connman_path, key_manager_interface, "GetServices",
			call_return_list, NULL, NULL, NULL);
}

/*
 * Call the Manager GetTechnologies method.
 */
int __cmd_technologies(void)
{
	return dbus_method_call(connection, key_connman_service,
			key_connman_path, key_manager_interface, "GetTechnologies",
			call_return_list, NULL,	NULL, NULL);
}

/*
 * Call the Service Connect method.
 * This method call will force refresh (key_connect_return is set in user_data
 * attribute of the command callback).
 * @param serv_dbus_name the dbus name of the service
 */
int __cmd_connect(const char *serv_dbus_name)
{
	return dbus_method_call(connection, key_connman_service,
			serv_dbus_name, key_service_interface, "Connect",
			call_return_list_free, strdup(key_connect_return), NULL,
			NULL);
}

/*
 * Call the Service Disconnect method.
 * @param serv_dbus_name the dbus name of the service
 */
int __cmd_disconnect(const char *serv_dbus_name)
{
	return dbus_method_call(connection, key_connman_service,
			serv_dbus_name, key_service_interface, "Disconnect",
			call_return_list, NULL, NULL, NULL);
}

/*
 * Call the Technology Scan method, note that it will send an error callback if
 * the technology can't be scanned.
 * @param tech_dbus_name the dbus name of the technology
 */
int __cmd_scan(const char *tech_dbus_name)
{
	return dbus_method_call(connection, key_connman_service,
			tech_dbus_name, key_technology_interface, "Scan",
			call_return_list_free, strdup(key_scan_return), NULL, NULL);
}

/*
 * Toogle the Powered state of the technology.
 * @param tech_dbus_name the dbus name of the technology
 * @param set_power_to the new power state to set
 */
int __cmd_toogle_tech_power(const char *tech_dbus_name, bool set_power_to)
{
	dbus_bool_t dbus_bool;

	dbus_bool = set_power_to ? TRUE : FALSE;

	return dbus_set_property(connection, tech_dbus_name,
			"net.connman.Technology", call_return_list_free,
			strdup(tech_dbus_name), "Powered",
			DBUS_TYPE_BOOLEAN, &dbus_bool);
}

/*
 * Append the IPv4 json object to iter.
 * @param iter valid dbus message iteration
 * @param jobj IPv4 settings
 */
static void config_append_ipv4(DBusMessageIter *iter,
		struct json_object *jobj)
{
	const char *str;

	json_object_object_foreach(jobj, key, val) {
		str = json_object_get_string(val);
		dbus_append_dict_entry(iter, key, DBUS_TYPE_STRING, &str);
	}
}

/*
 * Append the IPv6 json object to iter and perform checks, if any error occur
 * the error callback is emitted. The verifications are detailed in
 * connman/doc/services-api.txt.
 * @param iter valid dbus message iteration
 * @param jobj IPv6 setings
 */
static void config_append_ipv6(DBusMessageIter *iter,
		struct json_object *jobj)
{
	struct json_object *methobj;
	const char *method, *str;

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
			dbus_append_dict_entry(iter, key,
					DBUS_TYPE_BYTE, &tmp);
		} else {
			str = json_object_get_string(val);
			dbus_append_dict_entry(iter, key, DBUS_TYPE_STRING, &str);
		}
	}
}

/*
 * Append an array of string to iter.
 * @param iter valid dbus message iteration
 * @param jobj the array of strings (in json objects)
 */
static void config_append_json_array_of_strings(DBusMessageIter *iter,
		struct json_object *jobj)
{
	struct json_object *strobj;
	int i, len;
	const char *str;

	if (jobj && json_object_is_type(jobj, json_type_array)) {
		len = json_object_array_length(jobj);

		for (i = 0; i < len; i++) {
			strobj = json_object_array_get_idx(jobj, i);

			if (!strobj)
				continue;

			str = json_object_get_string(strobj);
			dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &str);
		}
	}
}

/*
 * Append the Proxy json object to iter and perform checks, if any error occur
 * the error callback is emitted. The verifications are detailed in
 * connman/doc/services-api.txt.
 * @param iter valid dbus message iteration
 * @param jobj Proxy settings
 */
static void config_append_proxy(DBusMessageIter *iter,
		struct json_object *jobj)
{
	struct json_object *methobj, *urlobj, *tmpobj;
	const char *method, *url;

	if (!json_object_object_get_ex(jobj, "Method", &methobj)) {
		call_return_list(NULL, "No 'Method' set", "");
		return;
	}

	method = json_object_get_string(methobj);

	if (strcmp(method, "manual") == 0) {
		if (!json_object_object_get_ex(jobj, "Servers", &tmpobj))
			tmpobj = 0;
		dbus_append_dict_string_array(iter, "Servers",
				config_append_json_array_of_strings, tmpobj);

		if (!json_object_object_get_ex(jobj, "Excludes", &tmpobj))
			tmpobj = 0;
		dbus_append_dict_string_array(iter, "Excludes",
				config_append_json_array_of_strings, tmpobj);

	} else if (strcmp(method, "auto") == 0) {
		if (json_object_object_get_ex(jobj, "URL", &urlobj)) {
			url = json_object_get_string(urlobj);
			if (url)
				dbus_append_dict_entry(iter, "URL",
						DBUS_TYPE_STRING, url);
		}

	} else if (strcmp(method, "direct") != 0)
		return;

	dbus_append_dict_entry(iter, "Method", DBUS_TYPE_STRING, &method);
}

/*
 * Note that option names are the same as the ones in connman/doc/services-api.txt
 *
   "options": {
	   "IPv4.Configuration": {
		   "Method": "dhcp"|"manual"|"off",
		   "Address": "xxxx",
		   "Netmask": "xxxx",
		   "Gateway": "xxxx"
	   },
	   "IPv6.Configuration": {
		   "Method": "auto"|"manual"|"6to4"|"off",
		   "Address": "xxxx",
		   "PrefixLength": 12,
		   "Gateway": "xxxx",
		   "Privacy": "auto"|"disabled"|"enabled"|"prefered"
	   },
	   "Proxy.Configuration": {
		   "Method": "direct"|"auto"|"manual",
		   "URL": "proxyurl",
		   "Servers": [ "proxyserver1", "proxyserver1" ],
		   "Excludes": [ "proxyserver1" ]
	   },
	   "AutoConnect": FALSE|TRUE,
	   "Domains.Configuration": [ "domainserver1", "domainserver2" ],
	   "Nameservers.Configuration: [ "nameserver1", "nameserver2" ],
	   "Timeservers.Configuration: [ "timeserver1", "timeserver2" ]
   }
 *
 * @param service_dbus_name dbus name of the service to configure
 * @param options json object, see above for format
 */
int __cmd_config_service(const char *service_dbus_name, struct json_object *options)
{
	int res = 0;
	char *simple_service_conf, *dyn_service_name;
	dbus_bool_t dbus_bool;

	if (!service_dbus_name) {
		call_return_list(NULL, "Wrong service name",
				"Service set properties.");
		return -EINVAL;
	}

	if (!options) {
		call_return_list(NULL, "No options provided for service "
				"configuration", "");
		return -EINVAL;
	}

	json_object_object_foreach(options, key, val) {
		simple_service_conf = NULL;
		dyn_service_name = extract_dbus_short_name(service_dbus_name);

		if (strcmp("IPv4.Configuration", key) == 0) {
			res = dbus_set_property_dict(connection,
					service_dbus_name, key_service_interface,
					call_return_list_free, dyn_service_name,
					"IPv4.Configuration", DBUS_TYPE_STRING,
					config_append_ipv4, val);

		} else if (strcmp("IPv6.Configuration", key) == 0) {
			res = dbus_set_property_dict(connection,
					service_dbus_name, key_service_interface,
					call_return_list_free, dyn_service_name,
					"IPv6.Configuration", DBUS_TYPE_STRING,
					config_append_ipv6, val);

		} else if (strcmp("Proxy.Configuration", key) == 0) {
			res = dbus_set_property_dict(connection,
					service_dbus_name, key_service_interface,
					call_return_list_free, dyn_service_name,
					"Proxy.Configuration", DBUS_TYPE_STRING,
					config_append_proxy, val);

		} else if (strcmp("AutoConnect", key) == 0) {

			if (json_object_get_boolean(val) == TRUE)
				dbus_bool = TRUE;
			else
				dbus_bool = FALSE;

			res = dbus_set_property(connection, service_dbus_name,
					key_service_interface, call_return_list_free,
					dyn_service_name, "AutoConnect",
					DBUS_TYPE_BOOLEAN, &dbus_bool);

		} else if (strcmp("Domains.Configuration", key) == 0)
			simple_service_conf = key;

		else if (strcmp("Nameservers.Configuration", key) == 0)
			simple_service_conf = key;

		else if (strcmp("Timeservers.Configuration", key) == 0)
			simple_service_conf = key;
		else {
			char *str = strndup(key, JSON_COMMANDS_STRING_SIZE_SMALL);
			call_return_list_free(NULL, "Unknown configuration key", str);
			res = -EINVAL;
		}

		if (simple_service_conf != NULL) {
			res = dbus_set_property_array(connection,
					service_dbus_name, key_service_interface,
					call_return_list_free,
					dyn_service_name,
					simple_service_conf, DBUS_TYPE_STRING,
					config_append_json_array_of_strings, val);
		}

		simple_service_conf = NULL;

		if (res < 0 && res != -EINPROGRESS)
			return res;
	}

	return res;
}

/*
 * This is called when some signal have been emitted by the connman dbus
 * service. It will "forward" the signal in the signal callback with the
 * following format:
 *	{
 *		key_signal: "signal name string",
 *		key_command_interface: "interface string",
 *		key_command_path: "path string",
 *		key_command_data: { data of the signal }
 *	}
 */
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

	if (!strcmp(interface, key_agent_interface) ||
			!strcmp(interface, "net.connman.Session") ||
			!strcmp(interface, "net.connman.Notification"))
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

	interface = strrchr(interface, '.');
	if (interface && *interface != '\0')
		interface++;

	path = strrchr(dbus_message_get_path(message), '/');
	if (path && *path != '\0')
		path++;

	if (dbus_message_is_signal(message, key_manager_interface,
				"ServicesChanged")) {
		sig_name = json_object_new_string("ServicesChanged");

	} else if (dbus_message_is_signal(message, key_manager_interface,
				"PropertyChanged")) {
		sig_name = json_object_new_string("PropertyChanged");

	} else if (dbus_message_is_signal(message, key_manager_interface,
				"TechnologyAdded")) {
		path = dbus_message_get_member(message);
		sig_name = json_object_new_string("TechnologyAdded");

	} else if (dbus_message_is_signal(message, key_manager_interface,
				"TechnologyRemoved")) {
		path = dbus_message_get_member(message);
		sig_name = json_object_new_string("TechnologyRemoved");

	} else if (dbus_message_is_signal(message, key_service_interface,
				"PropertyChanged")) {
		sig_name = json_object_new_string("PropertyChanged");

	} else if (dbus_message_is_signal(message, key_technology_interface,
				"PropertyChanged")) {
		sig_name = json_object_new_string("PropertyChanged");

	} else {
		sig_name = json_object_new_string("Signal unsupported");
	}

	dbus_message_iter_init(message, &iter);
	res = json_object_new_object();

	json_object_object_add(res, key_command_interface,
			json_object_new_string(interface));
	json_object_object_add(res, key_command_path,
			json_object_new_string(path));
	json_object_object_add(res, key_command_data, dbus_to_json(&iter));

	json_object_object_add(res, key_signal, sig_name);
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

/*
 * Add a filter on connamn dbus signals.
 * @param interface one of "Service", "Technology", "Manager"
 */
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

/*
 * Remove a filter on connman dbus signals.
 * @param interface one of "Service", "Technology", "Manager"
 */
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
 * Add and remove signal monitoring.
 * @param jobj json object with the following format:
 *	{
 *		"monitor_add": [ "Service" ... ],
 *		"monitor_del": [ "Manager" ... ]
 *	}
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
