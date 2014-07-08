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

#ifndef __CONNMAN_RENDERERS_H
#define __CONNMAN_RENDERERS_H

#include <stdlib.h>
#include <json/json.h>
#include <string.h>
#include <assert.h>

#include "ncurses_utils.h"

#define RENDERERS_STRING_MAX_LEN 100

#ifdef __cplusplus
extern "C" {
#endif

void __renderers_home_page(struct json_object *jobj);

void __renderers_free_home_page(void);

#ifdef __cplusplus
}

#endif

#endif
