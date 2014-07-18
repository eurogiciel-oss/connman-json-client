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

#include "agent.h"

void (*agent_callback)(struct json_object *data, struct agent_data *request) = NULL;
void (*agent_error_callback)(struct json_object *data) = NULL;

static DBusConnection *agent_connection;

/*
 * agent_error_callback is dedicated to agent register/unregister errors.
 */

void request_browser_return(struct json_object *connected,
		struct agent_data *request);

void report_error_return(struct json_object *retry, struct agent_data *request);

/*
{
	"ERROR Agent": "erreur_msg...",
	"service": "/net/connman..."
}
*/
static struct json_object* format_agent_error(const char *error,
				const char *service)
{
	struct json_object *res;

	res = json_object_new_object();

	json_object_object_add(res, key_dbus_json_agent_error_key,
			json_object_new_string(error));
	json_object_object_add(res, "service", json_object_new_string(service));

	return res;
}

/*
{
	...,
	"msg": "Retry ?",
	"cb": "respond_error_cb"
}
*/
static void format_agent_with_callback(struct json_object *jerror,
		const char *msg, const char *cb)
{
	json_object_object_add(jerror, "msg", json_object_new_string(msg));
	json_object_object_add(jerror, "callback", json_object_new_string(cb));
}

/*
{
	"MSG Agent": "some info",
	"service": "/net/connman...",
	"data": {...}|json_object_null
}
*/
static struct json_object* format_agent_msg(const char *msg,
				const char *service, struct json_object *data)
{
	struct json_object *res;

	res = json_object_new_object();

	json_object_object_add(res, key_dbus_json_agent_msg_key,
		json_object_new_string(msg));
	json_object_object_add(res, "service", json_object_new_string(service));
	json_object_object_add(res, "data", data);

	return res;
}

static struct agent_data agent_request = {
	key_agent_interface,
};

static char *strip_path(char *path)
{
	char *name = strrchr(path, '/');
	if (name)
		name++;
	else
		name = path;

	return name;
}

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

	__connman_agent_unregister(connection, NULL);

	pending_message_remove(&agent_request);

	pending_command_complete();

	return dbus_message_new_method_return(message);
}

static DBusMessage *agent_cancel(DBusConnection *connection,
		DBusMessage *message, void *user_data)
{
	if (handle_message(message, &agent_request, agent_cancel) == false)
		return NULL;

	pending_message_remove(&agent_request);

	pending_command_complete();

	return dbus_message_new_method_return(message);
}

void request_browser_return(struct json_object *connected,
		struct agent_data *request)
{
	if (json_object_get_boolean(connected) == TRUE)
		__connman_dbus_send_reply(agent_connection, request->message,
				DBUS_TYPE_INVALID);
	else
		__connman_dbus_send_error(agent_connection, request->message,
				"net.connman.Agent.Error.Canceled", NULL);

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

	tmp = format_agent_msg("Agent RequestBrowser", service,
		json_object_new_string(url));
	format_agent_with_callback(tmp, "Connected ?",
		"request_browser_return");

	agent_request.message = message;
	agent_callback(tmp, &agent_request);

	return NULL;
}

void report_error_return(struct json_object *retry, struct agent_data *request)
{
	if (json_object_get_boolean(retry) == TRUE)
		__connman_dbus_send_error(agent_connection, request->message,
				"net.connman.Agent.Error.Retry", NULL);
	else
		__connman_dbus_send_reply(agent_connection, request->message,
				DBUS_TYPE_INVALID);

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
	service = strip_path(path);

	dbus_message_iter_next(&iter);
	dbus_message_iter_get_basic(&iter, &error);

	tmp = format_agent_error(error, service);
	format_agent_with_callback(tmp, "Retry ?", "report_error_return");

	agent_request.message = message;
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
	service = strip_path(str);

	dbus_message_iter_next(&iter);

	res = format_agent_msg("Input Requested", service,
			__connman_dbus_to_json(&iter));

	agent_request.message = message;
        agent_callback(res, &agent_request);

	return NULL;
}

static void agent_unregister_return(DBusMessageIter *iter,
		const char *error,
		void *user_data)
{
	if (error)
		agent_error_callback(format_agent_error(error, "0xdeadbeef"));
	else
		agent_request.registered = false;
}

void __connman_agent_unregister(DBusConnection *connection, void *user_data)
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
			"net.connman.Manager", "UnregisterAgent");

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
	printf("\n[~] @message_handler, method = %s\n", method);

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
	.unregister_function = __connman_agent_unregister,
	.message_function = message_handler,
};

static void agent_register_return(DBusMessageIter *iter,
		const char *error, void *user_data)
{
	DBusConnection *connection = user_data;

	if (error) {
		__connman_agent_unregister(connection, NULL);
		agent_error_callback(format_agent_error(error, ""));

	} else
		agent_request.registered = true;
}

int __connman_agent_register(DBusConnection *connection)
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
			"net.connman.Manager", "RegisterAgent");

	if (!msg)
		return -ENOMEM;

	dbus_message_iter_init_append(msg, &iter);
	append_path(&iter);

	res = send_method_call(connection, msg, agent_register_return,
			NULL);

	if (res != -EINPROGRESS) {
		__connman_agent_unregister(connection, NULL);
		agent_error_callback(format_agent_error("Failed to register"
			" Agent", ""));
	} else
		agent_request.registered = true;

	agent_connection = connection;

	return res;
}

/*
 * Called with the result of the input
 */
void __connman_json_to_agent_response(struct json_object *jobj,
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

	res = __connman_json_to_dbus_dict(jobj, &dict);
	json_object_put(jobj);

	dbus_message_iter_close_container(&iter, &dict);

	if (res == 0) {
		__connman_dbus_send_message(agent_connection, request->reply);
		request->reply = NULL;
	}

	pending_message_remove(request);
	pending_command_complete();
}
