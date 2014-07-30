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

#include "json_regex.h"
#include "keys.h"

extern struct json_object *jregex_agent_response;
extern struct json_object *jregex_agent_retry_response;

void generate_trusted_json(void)
{
	jregex_agent_response = json_object_new_object();
	json_object_object_add(jregex_agent_response, "Name", json_object_new_string("^([[:print:]]+)$"));
	json_object_object_add(jregex_agent_response, "SSID", json_object_new_string("^([[:xdigit:]]+)$"));
	json_object_object_add(jregex_agent_response, "Identity", json_object_new_string("^([[:print:]]+)$"));
	json_object_object_add(jregex_agent_response, "Passphrase", json_object_new_string("^([[:print:]]*)$"));
	json_object_object_add(jregex_agent_response, "PreviousPassphrase", json_object_new_string("^([[:print:]]*)$"));
	json_object_object_add(jregex_agent_response, "WPS", json_object_new_string("^([[:digit:]]*)$"));
	json_object_object_add(jregex_agent_response, "Username", json_object_new_string("^([[:print:]]*)$"));
	json_object_object_add(jregex_agent_response, "Password", json_object_new_string("^([[:print:]]*)$"));

	jregex_agent_retry_response = json_object_new_boolean(TRUE);
}

void free_trusted_json(void)
{
	json_object_put(jregex_agent_response);
	json_object_put(jregex_agent_retry_response);
}
