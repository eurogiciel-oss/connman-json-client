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

#ifndef __CONNMAN_COMMANDS_H
#define __CONNMAN_COMMANDS_H

#include <dbus/dbus.h>
#include <json/json.h>

#define _POSIX_C_SOURCE 200809L

#define JSON_COMMANDS_STRING_SIZE_SMALL 25
#define JSON_COMMANDS_STRING_SIZE_MEDIUM 70

#ifdef __cplusplus
extern "C" {
#endif

int __cmd_state(void);

int __cmd_services(void);

int __cmd_technologies(void);

int __cmd_monitor(struct json_object *jobj);

int __connman_command_dispatcher(DBusConnection *dbus_conn,
	struct json_object *jobj);

#ifdef __cplusplus
}
#endif

#endif
