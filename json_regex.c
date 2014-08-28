/*
 *  connman-ncurses
 *
 *  Copyright (C) 2014 Eurogiciel. All rights reserved.
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

#include <json.h>

#include "json_regex.h"
#include "keys.h"

/*
 * Some regex applyed by the engine on input data cannot be translated into json
 * with json_tokener. IPV6_REGEX is a quite good example of why it can't be
 * translated into json as it is.
 */

static const char *IPV6_REGEX = "^((([0-9A-Fa-f]{1,4}:){7}[0-9A-Fa-f]{1,4})|(([0-9A-Fa-f]{1,4}:){6}:[0-9A-Fa-f]{1,4})|(([0-9A-Fa-f]{1,4}:){5}:([0-9A-Fa-f]{1,4}:)?[0-9A-Fa-f]{1,4})|(([0-9A-Fa-f]{1,4}:){4}:([0-9A-Fa-f]{1,4}:){0,2}[0-9A-Fa-f]{1,4})|(([0-9A-Fa-f]{1,4}:){3}:([0-9A-Fa-f]{1,4}:){0,3}[0-9A-Fa-f]{1,4})|(([0-9A-Fa-f]{1,4}:){2}:([0-9A-Fa-f]{1,4}:){0,4}[0-9A-Fa-f]{1,4})|(([0-9A-Fa-f]{1,4}:){6}((b((25[0-5])|(1d{2})|(2[0-4]d)|(d{1,2}))b).){3}(b((25[0-5])|(1d{2})|(2[0-4]d)|(d{1,2}))b))|(([0-9A-Fa-f]{1,4}:){0,5}:((b((25[0-5])|(1d{2})|(2[0-4]d)|(d{1,2}))b).){3}(b((25[0-5])|(1d{2})|(2[0-4]d)|(d{1,2}))b))|(::([0-9A-Fa-f]{1,4}:){0,5}((b((25[0-5])|(1d{2})|(2[0-4]d)|(d{1,2}))b).){3}(b((25[0-5])|(1d{2})|(2[0-4]d)|(d{1,2}))b))|([0-9A-Fa-f]{1,4}::([0-9A-Fa-f]{1,4}:){0,5}[0-9A-Fa-f]{1,4})|(::([0-9A-Fa-f]{1,4}:){0,6}[0-9A-Fa-f]{1,4})|(([0-9A-Fa-f]{1,4}:){1,7}:))$";

static const char *IPV4_REGEX = "^(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$";

// Regex filter for agent response
extern struct json_object *jregex_agent_response;

// Regex filter for agent retry
extern struct json_object *jregex_agent_retry_response;

// Regex filter for service configuration
extern struct json_object *jregex_config_service;

/*
 * Generate json objects used for verification.
 */
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

	// See commands.c __cmd_config_service for a better idea of the format.
	jregex_config_service = json_object_new_object();
	opt = json_object_new_object();
	tmp = json_object_new_object();
	json_object_object_add(tmp, key_serv_ipv4_method, json_object_new_string("^(dhcp|manual|off)$"));
	json_object_object_add(tmp, key_serv_ipv4_address, json_object_new_string(IPV4_REGEX));
	json_object_object_add(tmp, key_serv_ipv4_netmask, json_object_new_string(IPV4_REGEX));
	json_object_object_add(tmp, key_serv_ipv4_gateway, json_object_new_string(IPV4_REGEX));
	json_object_object_add(opt, key_serv_ipv4_config, tmp);
	tmp = json_object_new_object();
	json_object_object_add(tmp, key_serv_ipv6_method, json_object_new_string("^(auto|manual|6to4|off)$"));
	json_object_object_add(tmp, key_serv_ipv6_address, json_object_new_string(IPV6_REGEX));
	json_object_object_add(tmp, key_serv_ipv6_prefixlength, json_object_new_int(12));
	json_object_object_add(tmp, key_serv_ipv6_gateway, json_object_new_string(IPV6_REGEX));
	json_object_object_add(tmp, key_serv_ipv6_privacy, json_object_new_string("^(auto|disabled|enabled|prefered)$"));
	json_object_object_add(opt, key_serv_ipv6_config, tmp);
	tmp = json_object_new_object();
	json_object_object_add(tmp, key_serv_proxy_method, json_object_new_string("^(direct|auto|manual)$"));
	arr = json_object_new_array();
	json_object_array_add(arr, json_object_new_string("^([[:print:]]*)$"));
	json_object_object_add(tmp, key_serv_proxy_url, arr);
	arr = json_object_new_array();
	json_object_array_add(arr, json_object_new_string("^([[:print:]]*)$"));
	json_object_object_add(tmp, key_serv_proxy_servers, arr);
	arr = json_object_new_array();
	json_object_array_add(arr, json_object_new_string("^([[:print:]]*)$"));
	json_object_object_add(tmp, key_serv_proxy_excludes, arr);
	json_object_object_add(opt, key_serv_proxy_config, tmp);
	json_object_object_add(opt, key_serv_autoconnect, json_object_new_boolean(TRUE));
	arr = json_object_new_array();
	json_object_array_add(arr, json_object_new_string("^([[:print:]]*)$"));
	json_object_object_add(opt, key_serv_domains_config, arr);
	arr = json_object_new_array();
	json_object_array_add(arr, json_object_new_string("^([[:print:]]*)$"));
	json_object_object_add(opt, key_serv_nameservers_config, arr);
	arr = json_object_new_array();
	json_object_array_add(arr, json_object_new_string("^([[:print:]]*)$"));
	json_object_object_add(opt, key_serv_timeservers_config, arr);
	json_object_object_add(jregex_config_service, key_options, opt);
	json_object_object_add(jregex_config_service, key_service, json_object_new_string("(%5C%5C|/|([a-zA-Z]))+"));

}

/*
 * Free the json objects.
 */
void free_trusted_json(void)
{
	json_object_put(jregex_agent_response);
	json_object_put(jregex_agent_retry_response);
	json_object_put(jregex_config_service);
}
