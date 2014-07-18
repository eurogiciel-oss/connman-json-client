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

#ifndef __CONNMAN_AGENT_H
#define __CONNMAN_AGENT_H

#include <stdbool.h>

#include "dbus_helpers.h"

#ifdef __cplusplus
extern "C" {
#endif

struct agent_data {
	char *interface;
	bool registered;
	DBusMessage *message;
	DBusMessage *reply;
	DBusMethodFunction pending_function;
};

struct agent_data;

extern void (*agent_callback)(struct json_object *data, struct agent_data *request);
extern void (*agent_error_callback)(struct json_object *data);

int __connman_agent_register(DBusConnection *connection);

void __connman_agent_unregister(DBusConnection *connection, void *user_data);

void __connman_json_to_agent_response(struct json_object *jobj,
		struct agent_data *request);

#ifdef __cplusplus
}
#endif

#endif
