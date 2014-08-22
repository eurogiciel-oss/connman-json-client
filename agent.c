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
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

#include "dbus_helpers.h"
#include "dbus_json.h"
#include "keys.h"
#include "string_utils.h"

#include "agent.h"

/*
 * This is a strip down of the agent.c implementation in connmanctl.
 * GLib dependancies have been removed as well as vpn_agent. Finally, the
 * overall is more simple and only allow a single agent request at any given
 * time.
 */

// This is the generic callback for agent requests / errors.
void (*agent_callback)(struct json_object *data, struct agent_data *request) = NULL;

// agent_error_callback is dedicated to agent register/unregister errors.
void (*agent_error_callback)(struct json_object *data) = NULL;

// The agent have to have his own dbus connection.
static DBusConnection *agent_connection;

// The callback to call to respond to an agent browser requets.
void request_browser_return(struct json_object *connected,
		struct agent_data *request);

// The callback to call to respond to an agent error.
void report_error_return(struct json_object *retry, struct agent_data *request);

/*
 * Format the agent error:
 {
	key_agent_error: error,
	key_service: service
 }
*/
static struct json_object* format_agent_error(const char *error,
		const char *service)
{
	struct json_object *res;

	res = json_object_new_object();
	json_object_object_add(res, key_agent_error,
			json_object_new_string(error));
	json_object_object_add(res, key_service,
			json_object_new_string(service));

	return res;
}

/*
 * Format the agent message and indicate the appropriate callback for the
 * answer. This is done by adding the following attributes to jerror.
 {
 	... ,
 	key_agent_error_message: msg,
 	key_agent_error_callback: cb
 }
*/
static void format_agent_with_callback(struct json_object *jerror,
		const char *msg, const char *cb)
{
	json_object_object_add(jerror, key_agent_error_message,
			json_object_new_string(msg));
	json_object_object_add(jerror, key_agent_error_callback,
			json_object_new_string(cb));
}

/*
 * Format an agent message.
 * @param data Can be NULL, in that case the attribute key_agent_msg_data won't
 * be set.
 {
	key_agent_msg: msg,
	key_service: service,
	key_agent_msg_data: { data }
 }
*/
static struct json_object* format_agent_msg(const char *msg,
		const char *service, struct json_object *data)
{
	struct json_object *res;

	res = json_object_new_object();

	json_object_object_add(res, key_agent_msg,
			json_object_new_string(msg));
	json_object_object_add(res, key_service,
			json_object_new_string(service));

	if (data)
		json_object_object_add(res, key_agent_msg_data, data);

	return res;
}

/*
 * The agent_request is used to keep in a place multiple informations on the
 * agent and the current request. See agent.h for definition.
 */
static struct agent_data agent_request = {
	key_agent_interface,
};

/*
 * Return the custom agent dbus path.
 */
static char *agent_path(void)
{
	static char path[] = "/connman_json_agent";
	return path;
}

static void append_path(DBusMessageIter *iter)
{
	const char *path = agent_path();

	dbus_message_iter_append_basic(iter, DBUS_TYPE_OBJECT_PATH, &path);
}

static void pending_message_remove(struct agent_data *request)
{
	if (request->message) {
		dbus_message_unref(request->message);
		request->message = NULL;
	}

	if (request->reply) {
		dbus_message_unref(request->reply);
		request->reply = NULL;
	}
}

static void pending_command_complete()
{
	struct agent_data *next_request = NULL;
	DBusMessage *pending_message;
	DBusMethodFunction pending_function;

	if (agent_request.message)
		next_request = &agent_request;

	if (!next_request)
		return;

	pending_message = next_request->message;
	pending_function = next_request->pending_function;
	next_request->pending_function = NULL;

	pending_function(agent_connection, next_request->message,
			next_request);

	dbus_message_unref(pending_message);
}

static bool handle_message(DBusMessage *message, struct agent_data *request,
		DBusMethodFunction function)
{
	if (!agent_request.pending_function)
		return true;

	request->message = dbus_message_ref(message);
	request->pending_function = function;

	return false;
}

static DBusMessage *agent_release(DBusConnection *connection,
		DBusMessage *message, void *user_data)
{
	if (handle_message(message, &agent_request, agent_release) == false)
		return NULL;

	agent_unregister(connection, NULL);

	pending_message_remove(&agent_request);

	pending_command_complete();

	return dbus_message_new_method_return(message);
}

static DBusMessage *agent_cancel(DBusConnection *connection,
		DBusMessage *message, void *user_data)
{
	struct json_object *res;

	if (handle_message(message, &agent_request, agent_cancel) == false)
		return NULL;

	pending_message_remove(&agent_request);

	pending_command_complete();

	res = format_agent_msg("Agent canceled", "", NULL);
	agent_callback(res, &agent_request);

	return dbus_message_new_method_return(message);
}

void request_browser_return(struct json_object *connected,
		struct agent_data *request)
{
	if (json_object_get_boolean(connected) == TRUE)
		request->reply = dbus_message_new_method_return(request->message);

	else
		request->reply = dbus_message_new_error(request->message,
				"net.connman.Agent.Error.Canceled", NULL);

	if (request->reply)
		dbus_connection_send(agent_connection, request->reply, NULL);

	pending_message_remove(request);
	pending_command_complete();
}

static DBusMessage *agent_request_browser(DBusConnection *connection,
		DBusMessage *message, void *user_data)
{
	DBusMessageIter iter;
	char *service, *url;
	struct json_object *tmp;

	if (handle_message(message, &agent_request, agent_request_browser) == false)
		return NULL;

	dbus_message_iter_init(message, &iter);

	dbus_message_iter_get_basic(&iter, &service);
	dbus_message_iter_next(&iter);
	dbus_message_iter_get_basic(&iter, &url);

	tmp = format_agent_msg(key_agent_request_browser, service,
		json_object_new_string(url));
	format_agent_with_callback(tmp, "Connected ?",
		"request_browser_return");

	agent_request.message = message;
	agent_callback(tmp, &agent_request);

	return NULL;
}

void report_error_return(struct json_object *retry, struct agent_data *request)
{
	if (!request || !retry)
		return;

	if (json_object_get_boolean(retry) == TRUE)
		request->reply = dbus_message_new_error(request->message,
				"net.connman.Agent.Error.Retry", NULL);
	else
		request->reply = dbus_message_new_method_return(request->message);

	if (request->reply)
		dbus_connection_send(agent_connection, request->reply, NULL);

	pending_message_remove(request);
	pending_command_complete();
}

static DBusMessage *agent_report_error(DBusConnection *connection,
		DBusMessage *message, void *user_data)
{
	DBusMessageIter iter;
	char *path, *service, *error;
	struct json_object *tmp;

	if (handle_message(message, &agent_request, agent_report_error) == false)
		return NULL;

	dbus_message_iter_init(message, &iter);

	dbus_message_iter_get_basic(&iter, &path);
	service = extract_dbus_short_name(path);

	dbus_message_iter_next(&iter);
	dbus_message_iter_get_basic(&iter, &error);

	tmp = format_agent_error(error, service);
	format_agent_with_callback(tmp, "Retry ?", "report_error_return");
	free(service);

	agent_request.message = dbus_message_ref(message);
	agent_callback(tmp, &agent_request);

	return NULL;
}

static DBusMessage *agent_request_input(DBusConnection *connection,
		DBusMessage *message, void *user_data)
{
	DBusMessageIter iter;
	char *service, *str;
	struct json_object *res;

	if (handle_message(message, &agent_request, agent_request_input) == false)
		return NULL;

	dbus_message_iter_init(message, &iter);

	dbus_message_iter_get_basic(&iter, &str);
	service = extract_dbus_short_name(str);

	dbus_message_iter_next(&iter);

	res = format_agent_msg(key_agent_request_input, service,
			dbus_to_json(&iter));
	free(service);

	agent_request.message = message;
        agent_callback(res, &agent_request);

	return NULL;
}

static void agent_unregister_return(DBusMessageIter *iter,
		const char *error, void *user_data)
{
	if (error)
		agent_error_callback(format_agent_error(error, "Got error while"
					" uneregistering agent."));
	else
		agent_request.registered = false;
}

void agent_unregister(DBusConnection *connection, void *user_data)
{
	int res;
	DBusMessage *msg;
	DBusMessageIter iter;

	if (agent_request.registered == false) {
		agent_error_callback(format_agent_error("Agent not"
			" registered", ""));
		return;
	}

	msg = dbus_message_new_method_call(key_connman_service, key_connman_path,
			key_manager_interface, "UnregisterAgent");

	if (!msg)
		return;

	dbus_message_iter_init_append(msg, &iter);
	append_path(&iter);

	res = send_method_call(connection, msg, agent_unregister_return,
			NULL);

	if (res != -EINPROGRESS)
		agent_error_callback(format_agent_error("Failed to unregister"
			" Agent", ""));


	return;
}

static DBusHandlerResult message_handler(DBusConnection *conn, DBusMessage *msg,
		void *user_data)
{
	const char *method = dbus_message_get_member(msg);
	msg = dbus_message_ref(msg);

	if (strcmp("Release", method) == 0)
		agent_release(conn, msg, user_data);
	else if (strcmp("ReportError", method) == 0)
		agent_report_error(conn, msg, user_data);
	else if (strcmp("RequestBrowser", method) == 0)
		agent_request_browser(conn, msg, user_data);
	else if (strcmp("RequestInput", method) == 0)
		agent_request_input(conn, msg, user_data);
	else if (strcmp("Cancel", method) == 0)
		agent_cancel(conn, msg, user_data);
	else
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

	return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusObjectPathVTable agent_table = {
	.unregister_function = agent_unregister,
	.message_function = message_handler,
};

static void agent_register_return(DBusMessageIter *iter,
		const char *error, void *user_data)
{
	DBusConnection *connection = user_data;

	if (error) {
		agent_unregister(connection, NULL);
		agent_error_callback(format_agent_error(error, ""));

	} else
		agent_request.registered = true;
}

int agent_register(DBusConnection *connection)
{
	DBusMessage *msg;
	DBusMessageIter iter;
	int res;

	if (agent_request.registered == true) {
		agent_error_callback(format_agent_error("Agent already"
			" registered", ""));
		return -EALREADY;
	}

	if (!dbus_connection_register_object_path(connection, agent_path(),
				&agent_table, NULL))
		return -ENOMEM;

	msg = dbus_message_new_method_call(key_connman_service, key_connman_path,
			key_manager_interface, "RegisterAgent");

	if (!msg)
		return -ENOMEM;

	dbus_message_iter_init_append(msg, &iter);
	append_path(&iter);

	res = send_method_call(connection, msg, agent_register_return,
			NULL);

	if (res != -EINPROGRESS) {
		agent_unregister(connection, NULL);
		agent_error_callback(format_agent_error("Failed to register"
			" Agent", ""));
	} else
		agent_request.registered = true;

	agent_connection = connection;

	return res;
}

/*
 * Called with the result of an agent input request. This reply to the request
 * after json object to dbus message translation.
 */
int json_to_agent_response(struct json_object *jobj,
		struct agent_data *request)
{
	DBusMessageIter iter;
	DBusMessageIter dict;
	int res = 0;

	request->reply = dbus_message_new_method_return(request->message);
	dbus_message_iter_init_append(request->reply, &iter);

	dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY,
                        DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
                        DBUS_TYPE_STRING_AS_STRING DBUS_TYPE_VARIANT_AS_STRING
                        DBUS_DICT_ENTRY_END_CHAR_AS_STRING,
			&dict);

	res = json_to_dbus_dict(jobj, &dict);

	dbus_message_iter_close_container(&iter, &dict);

	if (res == 0) {
		res = dbus_send_message(agent_connection, request->reply);
		request->reply = NULL;
	}

	pending_message_remove(request);
	pending_command_complete();

	return res;
}
