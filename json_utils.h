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

#ifndef __CONNMAN_JSON_UTILS_H
#define __CONNMAN_JSON_UTILS_H

#include <json/json.h>
#include <sys/types.h>
#include <regex.h>
#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>

//TODO remove
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

_Bool __json_type_dispatch(struct json_object *jobj,
		struct json_object *jtrusted);

#ifdef __cplusplus
}
#endif

#endif
