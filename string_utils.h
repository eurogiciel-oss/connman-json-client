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

#ifndef __CONNMAN_STRING_UTILS_H
#define __CONNMAN_STRING_UTILS_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

char* trim_whitespaces(char *str);

char* extract_dbus_short_name(const char *str);

bool check_dbus_name(const char *name);

bool string_ends_with_configuration(const char *str);

#ifdef __cplusplus
}
#endif

#endif

