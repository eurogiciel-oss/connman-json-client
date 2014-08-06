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

#include <string.h>
#include <ctype.h>

#include "string_utils.h"

/*
 * This is useful because ncurses fill fields blanks with spaces.
 */
char* trim_whitespaces(char *str)
{
	char *end;

	// trim leading space
	while(isspace(*str))
		str++;

	if(*str == 0) // all spaces?
		return str;

	// trim trailing space
	end = str + strnlen(str, 128) - 1;

	while(end > str && isspace(*end))
		end--;

	// write new null terminator
	*(end+1) = '\0';

	return str;
}

/*
 * This get the last token ('/') of str.
 * Don't forget to free the return value.
 */
char* extract_dbus_short_name(const char *str)
{
	char *last_token = strrchr(str, '/'), *res;

	if (!last_token)
		return NULL;

	last_token++;
	res = strdup(last_token);

	return res;
}

/*
 * Valid dbus chars should be [A-Z][a-z][0-9]_
 * and should not start with number.
 */
bool check_dbus_name(const char *name)
{
	unsigned int i;

	if (!name || name[0] == '\0')
		return false;

	for (i = 0; name[i] != '\0'; i++)
		if (!((name[i] >= 'A' && name[i] <= 'Z') ||
					(name[i] >= 'a' && name[i] <= 'z') ||
					(name[i] >= '0' && name[i] <= '9') ||
					name[i] == '_'))
			return false;

	return true;
}

/*
 * Test if a given string ends with ".Configuration".
 */
bool string_ends_with_configuration(const char *str)
{
	char *last_token;

	last_token = strrchr(str, '.');

	if (last_token)
		last_token++;

	return (last_token && strcmp(last_token, "Configuration") == 0);
}
