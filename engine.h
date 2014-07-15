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

#ifndef __CONNMAN_ENGINE_H
#define __CONNMAN_ENGINE_H

#include "agent.h"

#define ENGINE_KEY_COMMAND "command"
#define ENGINE_KEY_CMD_DATA "cmd_data"

#ifdef __cplusplus
extern "C" {
#endif

extern DBusConnection *connection;

extern void (*commands_callback)(struct json_object *data, json_bool is_error);
extern void (*commands_signal)(struct json_object *data);
extern void (*agent_callback)(struct json_object *data, struct agent_data *request);
extern void (*agent_error_callback)(struct json_object *data, struct agent_data *request);

extern void (*engine_callback)(int status, struct json_object *jobj);

int __engine_query(struct json_object *jobj);

int __engine_init(void);

#ifdef __cplusplus
}
#endif

#endif
