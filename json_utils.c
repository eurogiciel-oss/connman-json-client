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

#include <json/json.h>
#include <sys/types.h>
#include <regex.h>
#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "json_utils.h"

bool __match_strings(const char *str, const char *trusted)
{
	int regexp_err, regexp_match;
	regex_t preg;

	regexp_err = regcomp(&preg, trusted, REG_NOSUB | REG_EXTENDED);
	assert(regexp_err == 0);
	regexp_match = regexec(&preg, str, 0, NULL, 0);
	regfree(&preg);

	return (regexp_match == 0);
}

static bool json_match_string(struct json_object *jobj,
		struct json_object *jtrusted)
{
	return __match_strings(json_object_get_string(jobj),
			json_object_get_string(jtrusted));
}

static bool json_match_object(struct json_object *jobj,
		struct json_object *jtrusted)
{
	bool res = true;
	struct json_object *tmp_trusted;
	json_bool key_is_trusted;

	json_object_object_foreach(jobj, key, val) {
		key_is_trusted = json_object_object_get_ex(jtrusted, key,
				&tmp_trusted);

		if (key_is_trusted == FALSE)
			return false;

		res = __json_type_dispatch(val, tmp_trusted);

		if (res == false)
			return res;
	}

	return res;
}

static bool json_match_array(struct json_object *jobj,
		struct json_object *jtrusted)
{
	bool res = true;
	int array_len, i;
	struct json_object *elem_trusted;

	array_len = json_object_array_length(jobj);

	if (array_len <= 0)
		return false;

	elem_trusted = json_object_array_get_idx(jtrusted, 0);
	assert(elem_trusted);
	assert(json_object_get_type(elem_trusted) != json_type_null);

	for (i = 0; i < array_len && res == true; i++) {
		res = __json_type_dispatch(
				json_object_array_get_idx(jobj, i),
				elem_trusted);
	}

	return res;
}

bool __json_type_dispatch(struct json_object *jobj,
		struct json_object *jtrusted)
{
	enum json_type type, type_trusted;
	bool res;

	type = json_object_get_type(jobj);
	type_trusted = json_object_get_type(jtrusted);

	if (type != type_trusted)
		return false;

	switch (type_trusted) {

		case json_type_string:
			res = json_match_string(jobj, jtrusted);
			break;

		case json_type_object:
			res = json_match_object(jobj, jtrusted);
			break;

		case json_type_array:
			res = json_match_array(jobj, jtrusted);
			break;

		default:
			res = false;
			break;
	}

	return res;
}

static const char* get_string_from_jobj(struct json_object *jobj)
{
	if (json_object_get_type(jobj) == json_type_string)
		return json_object_get_string(jobj);

	return NULL;
}

static struct json_object* get_jobj_from_key(struct json_object *jobj,
		const char *key_id)
{
	struct json_object *data;
	json_object_object_get_ex(jobj, key_id, &data);

	if (!data)
		return NULL;

	return data;
}

const char* __json_get_command_str(struct json_object *jobj)
{
	struct json_object *cmd;
	cmd = get_jobj_from_key(jobj, "command");

	if (cmd)
		return get_string_from_jobj(cmd);

	return NULL;
}

/*
 * This get the last token ('/') of str.
 * Don't forget to free the return value.
 */
char* __extract_dbus_short_name(const char *str)
{
	char *last_token = strrchr(str, '/'), *res;

	if (!last_token)
		return NULL;

	last_token++;
	res = strdup(last_token);

	return res;
}
