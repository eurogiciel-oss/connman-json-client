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

#include <dbus/dbus.h>
#include <json/json.h>
#include <stdio.h>
#include <errno.h>
#include <ncurses.h>

#include "dbus_helpers.h"

#include "dbus_json.h"

struct json_object* dbus_basic_json(DBusMessageIter *iter)
{
	int arg_type, i;
	dbus_bool_t b;
	double d;
        char *str;
        struct json_object *res;
        DBusMessageIter entry;

	arg_type = dbus_message_iter_get_arg_type(iter);

	switch (arg_type) {
	case DBUS_TYPE_STRING:
	case DBUS_TYPE_OBJECT_PATH:
		dbus_message_iter_get_basic(iter, &str);
                res = json_object_new_string(str);
		break;

	case DBUS_TYPE_VARIANT:
		dbus_message_iter_recurse(iter, &entry);
		res = dbus_to_json(&entry);
		break;

	case DBUS_TYPE_BOOLEAN:
		dbus_message_iter_get_basic(iter, &b);
		if (!b)
                        res = json_object_new_boolean(0);
		else
                        res = json_object_new_boolean(1);
		break;

	case DBUS_TYPE_BYTE:
	case DBUS_TYPE_INT32:
	case DBUS_TYPE_UINT16:
	case DBUS_TYPE_UINT32:
		dbus_message_iter_get_basic(iter, &i);
                res = json_object_new_int((int32_t) i);
		break;

	case DBUS_TYPE_DOUBLE:
		dbus_message_iter_get_basic(iter, &d);
		res = json_object_new_double((double) d);
		break;

	default:
                fprintf(stderr, "Error on type %d(%c) in"
                "__connman_dbus_basic_json\n", arg_type, (char)arg_type);
                res = NULL;
		break;
	}

        return res;
}

struct json_object* dbus_dict_json(DBusMessageIter *iter)
{
        int arg_type;
        char * str;
        struct json_object *dict, *tmp;
        DBusMessageIter entry, subentry;

        dict = json_object_new_object();

	while ((arg_type = dbus_message_iter_get_arg_type(iter))
			!= DBUS_TYPE_INVALID) {

		switch (arg_type) {
		case DBUS_TYPE_DICT_ENTRY:
                        dbus_message_iter_recurse(iter, &entry);
                        dbus_message_iter_get_basic(&entry, &str);
                        dbus_message_iter_next(&entry);
                        dbus_message_iter_recurse(&entry, &subentry);
                        tmp = dbus_to_json(&subentry);
                        json_object_object_add(dict, str, tmp);
                        break;

                default:
                        fprintf(stderr, "Error on type %d(%c) in "
                        "__connman_dbus_dict_json\n", arg_type, (char)arg_type);
                        break;
		}

		dbus_message_iter_next(iter);
	}

        return dict;
}

struct json_object* dbus_array_json(DBusMessageIter *iter)
{
        struct json_object *jarray, *tmp;
        DBusMessageIter array;

        jarray = json_object_new_array();
        array = *iter;

        while (dbus_message_iter_get_arg_type(&array) != DBUS_TYPE_INVALID ) {
                tmp = dbus_to_json(&array);
                json_object_array_add(jarray, tmp);
                dbus_message_iter_next(&array);
        }

        return jarray;
}

struct json_object* dbus_to_json(DBusMessageIter *iter)
{
        struct json_object *res;
        int arg_type;
        DBusMessageIter subiter;

        switch ((arg_type = dbus_message_iter_get_arg_type(iter))) {
        case DBUS_TYPE_STRUCT:
                dbus_message_iter_recurse(iter, &subiter);
                res = dbus_array_json(&subiter);
                break;

        case DBUS_TYPE_ARRAY:
                dbus_message_iter_recurse(iter, &subiter);

                if (dbus_message_iter_get_arg_type(&subiter) == DBUS_TYPE_DICT_ENTRY) 
                    res = dbus_dict_json(&subiter);
                else
                    res = dbus_array_json(&subiter);
                break;

        case DBUS_TYPE_STRING:
        case DBUS_TYPE_OBJECT_PATH:
        case DBUS_TYPE_VARIANT:
        case DBUS_TYPE_BOOLEAN:
        case DBUS_TYPE_BYTE:
        case DBUS_TYPE_INT32:
        case DBUS_TYPE_UINT16:
        case DBUS_TYPE_UINT32:
        case DBUS_TYPE_DOUBLE:
                res = dbus_basic_json(iter);
                break;

	case DBUS_TYPE_INVALID:
		res = NULL;
		break;
        default:
                fprintf(stderr, "Type not supported in dbus_to_json %d(%c)\n",
                        arg_type, (char)arg_type);
                res = NULL;
        }

        return res;
}

struct json_object* __connman_dbus_to_json(DBusMessageIter *iter)
{
        struct json_object *res = NULL, *tmp = dbus_to_json(iter);

	// This is useful for the TechnologyAdded signal for example
	if (dbus_message_iter_next(iter) == TRUE) {
		res = json_object_new_array();
		json_object_array_add(res, tmp);
		json_object_array_add(res, dbus_to_json(iter));
	}

        return (res == NULL ? tmp : res);
}

void __connman_dbus_json_print(struct json_object *jobj)
{
        fprintf(stdout, "\n%s\n", json_object_to_json_string(jobj));
}

void __connman_dbus_json_print_pretty(struct json_object *jobj)
{
        fprintf(stdout, "\n%s\n", json_object_to_json_string_ext(jobj,
	JSON_C_TO_STRING_PRETTY));
}

int __connman_json_to_dbus_dict(struct json_object *jobj,
			DBusMessageIter *dict)
{
	int res = 0;
	const char *str;

	json_object_object_foreach(jobj, key, val) {
		str = json_object_get_string(val);

		if (!key || key[0] == '\0' || !str || str[0] == '\0') {
			res = -EINVAL;
			break;
		}

		__connman_dbus_append_dict_entry(dict, key, DBUS_TYPE_STRING,
				str);
	}

	return res;
}
