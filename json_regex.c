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

static const char *IPV6_REGEX = "^((([0-9A-Fa-f]{1,4}:){7}[0-9A-Fa-f]{1,4})|(([0-9A-Fa-f]{1,4}:){6}:[0-9A-Fa-f]{1,4})|(([0-9A-Fa-f]{1,4}:){5}:([0-9A-Fa-f]{1,4}:)?[0-9A-Fa-f]{1,4})|(([0-9A-Fa-f]{1,4}:){4}:([0-9A-Fa-f]{1,4}:){0,2}[0-9A-Fa-f]{1,4})|(([0-9A-Fa-f]{1,4}:){3}:([0-9A-Fa-f]{1,4}:){0,3}[0-9A-Fa-f]{1,4})|(([0-9A-Fa-f]{1,4}:){2}:([0-9A-Fa-f]{1,4}:){0,4}[0-9A-Fa-f]{1,4})|(([0-9A-Fa-f]{1,4}:){6}((b((25[0-5])|(1d{2})|(2[0-4]d)|(d{1,2}))b).){3}(b((25[0-5])|(1d{2})|(2[0-4]d)|(d{1,2}))b))|(([0-9A-Fa-f]{1,4}:){0,5}:((b((25[0-5])|(1d{2})|(2[0-4]d)|(d{1,2}))b).){3}(b((25[0-5])|(1d{2})|(2[0-4]d)|(d{1,2}))b))|(::([0-9A-Fa-f]{1,4}:){0,5}((b((25[0-5])|(1d{2})|(2[0-4]d)|(d{1,2}))b).){3}(b((25[0-5])|(1d{2})|(2[0-4]d)|(d{1,2}))b))|([0-9A-Fa-f]{1,4}::([0-9A-Fa-f]{1,4}:){0,5}[0-9A-Fa-f]{1,4})|(::([0-9A-Fa-f]{1,4}:){0,6}[0-9A-Fa-f]{1,4})|(([0-9A-Fa-f]{1,4}:){1,7}:))$";

static const char *IPV4_REGEX = "^(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$";

extern struct json_object *jregex_agent_response;
extern struct json_object *jregex_agent_retry_response;
extern struct json_object *jregex_config_service;

void generate_trusted_json(void)
{
	struct json_object *tmp, *opt, *arr;

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

	jregex_config_service = json_object_new_object();
	opt = json_object_new_object();
	tmp = json_object_new_object();
	json_object_object_add(tmp, "Method", json_object_new_string("^(dhcp|manual|off)$"));
	json_object_object_add(tmp, "Address", json_object_new_string(IPV4_REGEX));
	json_object_object_add(tmp, "Netmask", json_object_new_string(IPV4_REGEX));
	json_object_object_add(tmp, "Gateway", json_object_new_string(IPV4_REGEX));
	json_object_object_add(opt, "IPv4.Configuration", tmp);
	tmp = json_object_new_object();
	json_object_object_add(tmp, "Method", json_object_new_string("^(auto|manual|6to4|off)$"));
	json_object_object_add(tmp, "Address", json_object_new_string(IPV6_REGEX));
	json_object_object_add(tmp, "PrefixLength", json_object_new_int(12));
	json_object_object_add(tmp, "Gateway", json_object_new_string(IPV6_REGEX));
	json_object_object_add(tmp, "Privacy", json_object_new_string("^(auto|disabled|enabled|prefered)$"));
	json_object_object_add(opt, "IPv6.Configuration", tmp);
	tmp = json_object_new_object();
	json_object_object_add(tmp, "Method", json_object_new_string("^(direct|auto|manual)$"));
	arr = json_object_new_array();
	json_object_array_add(arr, json_object_new_string("^([[:print:]]+)$"));
	json_object_object_add(tmp, "URL", arr);
	arr = json_object_new_array();
	json_object_array_add(arr, json_object_new_string("^([[:print:]]+)$"));
	json_object_object_add(tmp, "Servers", arr);
	arr = json_object_new_array();
	json_object_array_add(arr, json_object_new_string("^([[:print:]]+)$"));
	json_object_object_add(tmp, "Excludes", arr);
	json_object_object_add(opt, "Proxy.Configuration", tmp);
	json_object_object_add(opt, "AutoConnect", json_object_new_boolean(TRUE));
	arr = json_object_new_array();
	json_object_array_add(arr, json_object_new_string("^([[:print:]]+)$"));
	json_object_object_add(opt, "Domains.Configuration", arr);
	arr = json_object_new_array();
	json_object_array_add(arr, json_object_new_string("^([[:print:]]+)$"));
	json_object_object_add(opt, "Nameservers.Configuration", arr);
	arr = json_object_new_array();
	json_object_array_add(arr, json_object_new_string("^([[:print:]]+)$"));
	json_object_object_add(opt, "Timeservers.Configuration", arr);
	json_object_object_add(jregex_config_service, "options", opt);
	json_object_object_add(jregex_config_service, key_service, json_object_new_string("(%5C%5C|/|([a-zA-Z]))+"));

}

void free_trusted_json(void)
{
	json_object_put(jregex_agent_response);
	json_object_put(jregex_agent_retry_response);
	json_object_put(jregex_config_service);
}
