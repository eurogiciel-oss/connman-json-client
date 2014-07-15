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

#ifndef __CONNMAN_DBUS_JSON_H
#define __CONNMAN_DBUS_JSON_H

#define DBUS_JSON_SUCCESS_KEY "OK"
#define DBUS_JSON_ERROR_KEY "ERROR"
#define DBUS_JSON_AGENT_ERROR_KEY "ERROR Agent"
#define DBUS_JSON_AGENT_MSG_KEY "MSG Agent"
#define DBUS_JSON_SIGNAL_KEY "SIGNAL"
#define CONNMAN_SERVICE   "net.connman"
#define CONNMAN_PATH      "/"

#ifdef __cplusplus
extern "C" {
#endif

struct json_object* dbus_basic_json(DBusMessageIter *iter);

struct json_object* dbus_dict_json(DBusMessageIter *iter);

struct json_object* dbus_array_json(DBusMessageIter *iter);

struct json_object* dbus_to_json(DBusMessageIter *iter);

struct json_object* __connman_dbus_to_json(DBusMessageIter *iter);

void __connman_dbus_json_print(struct json_object *jobj);

void __connman_dbus_json_print_pretty(struct json_object *jobj);

int __connman_json_to_dbus_dict(struct json_object *jobj,
		DBusMessageIter *dict);

#ifdef __cplusplus
}
#endif

#endif
