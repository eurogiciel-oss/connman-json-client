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

#include "engine.h"

struct client {
	engine_callback cb;
};

static struct client *engine_clients[ENGINE_MAX_CLIENTS];
static int first_engine_client_available_id = 0;
static int clients_number = 0;

static _Bool check_id_in_bounds(int id) {
	return id > 0 && id < ENGINE_MAX_CLIENTS;
}

static void update_available_client_id(void) {
	int i;

	for (i = 0; i < ENGINE_MAX_CLIENTS && engine_clients[i]; i++) {}

	if (i == ENGINE_MAX_CLIENTS)
		first_engine_client_available_id = -1;
	else
		first_engine_client_available_id = i;
}

int __engine_get(engine_callback cb) {
	struct client *tmp;
	int res;

	if (!cb)
		return -EINVAL;

	if (first_engine_client_available_id < 0)
		return -ENOMEM;

	tmp = malloc(sizeof(struct client));

	if (!tmp)
		return -ENOMEM;

	engine_clients[first_engine_client_available_id] = tmp;
	res = first_engine_client_available_id;
	update_available_client_id();
	clients_number++;

	return res;
}

int __engine_release(int id) {
	
	if (!check_id_in_bounds(id) || !engine_clients[id])
		return -EINVAL;

	free(engine_clients[id]);
	engine_clients[id] = NULL;

	if (id < first_engine_client_available_id)
		update_available_client_id();

	clients_number--;

	return 0;
}

static const char* get_string_from_jobj(struct json_object *jobj) {

	if (json_object_get_type(jobj) == json_type_string)
		return json_object_get_string(jobj);
	
	return NULL;
}

static struct json_object* get_jobj_from_key(struct json_object *jobj,
		const char *key_id) {
	struct json_object *data;

	json_object_object_get_ex(jobj, key_id, &data);
	if (!data)
		return NULL;
	
	return data;
}

static const char* get_command_str(struct json_object *jobj) {
	struct json_object *cmd;

	cmd = get_jobj_from_key(jobj, ENGINE_KEY_COMMAND);
	if (cmd)
		return get_string_from_jobj(cmd);

	return NULL;
}

int __engine_query(int id, struct json_object *jobj, void *user_data) {
	const char *command_str = NULL;

	if (!check_id_in_bounds(id) || !jobj)
		return -EINVAL;

	command_str = get_command_str(jobj);
	if (!command_str)
		return -EINVAL;



	return -ENOSYS;
}
