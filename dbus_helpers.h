/*
 *  connman-ncurses
 *
 *  Copyright (C) 2014 Eurogiciel. All rights reserved.
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

#ifndef __CONNMAN_DBUS_HELPERS_H
#define __CONNMAN_DBUS_HELPERS_H

#include <dbus/dbus.h>
#include <json.h>

#define TIMEOUT           60000

#ifdef __cplusplus
extern "C" {
#endif

typedef DBusMessage * (* DBusMethodFunction) (DBusConnection *connection,
		DBusMessage *message, void *user_data);

/*
 * This function is called when a connman dbus method have been called.
 */
extern void callback_ended(void);

typedef void (*connman_dbus_method_return_func_t)(DBusMessageIter *iter,
		const char *error, void *user_data);

typedef void (*connman_dbus_append_func_t)(DBusMessageIter *iter,
		struct json_object *append_json_object);

int dbus_method_call(DBusConnection *connection,
		const char *service, const char *path, const char *interface,
		const char *method, connman_dbus_method_return_func_t cb,
		void * user_data, connman_dbus_append_func_t append_fn,
		struct json_object *append_json_object);

int dbus_set_property(DBusConnection *connection,
		const char *path, const char *interface,
		connman_dbus_method_return_func_t cb, void * user_data,
		const char *property, int type, const void *value);

void dbus_append_dict(DBusMessageIter *iter,
		connman_dbus_append_func_t append_fn,
		struct json_object *append_json_object);

void dbus_append_dict_entry(DBusMessageIter *iter,
		const char *property, int type, const void *value);

int dbus_set_property_dict(DBusConnection *connection,
		const char *path, const char *interface,
		connman_dbus_method_return_func_t cb, void * user_data,
		const char *property, int type,
		connman_dbus_append_func_t append_fn,
		struct json_object *append_json_object);

void dbus_append_dict_string_array(DBusMessageIter *iter,
		const char *property, connman_dbus_append_func_t append_fn,
		struct json_object *append_json_object);

int dbus_set_property_array(DBusConnection *connection,
		const char *path, const char *interface,
		connman_dbus_method_return_func_t cb, void *user_data,
		const char *property, int type,
		connman_dbus_append_func_t append_fn,
		struct json_object *append_json_object);

int send_method_call(DBusConnection *connection,
		DBusMessage *message, connman_dbus_method_return_func_t cb,
		void *user_data);

dbus_bool_t dbus_send_message(DBusConnection *connection,
		DBusMessage *message);

dbus_bool_t dbus_send_reply_valist(DBusConnection *connection,
		DBusMessage *message, int type, va_list args);

dbus_bool_t dbus_send_reply(DBusConnection *connection,
		DBusMessage *message, int type, ...);

dbus_bool_t dbus_send_error_valist(DBusConnection *connection,
		DBusMessage *message, const char *name, const char *format,
		va_list args);

dbus_bool_t dbus_send_error(DBusConnection *connection, DBusMessage *message,
		const char *name, const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif
