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

const char key_state[] = "state";
const char key_technologies[] = "technologies";
const char key_technology[] = "technology";
const char key_service[] = "service";
const char key_services[] = "services";
const char key_options[] = "options";

const char key_command[] = "command";
const char key_command_data[] = "cmd_data";
const char key_command_path[] = "cmd_path";
const char key_command_interface[] = "cmd_interface";

const char key_engine_get_state[] = "get_state";
const char key_engine_get_services[] = "get_services";
const char key_engine_get_technologies[] = "get_technologies";
const char key_engine_get_home_page[] = "get_home_page";
const char key_engine_get_services_from_tech[] = "get_services_from_tech";
const char key_engine_connect[] = "connect";
const char key_engine_disconnect[] = "disconnect";
const char key_engine_agent_response[] = "agent_response";
const char key_engine_agent_retry[] = "agent_retry";
const char key_engine_scan_tech[] = "scan_tech";
const char key_engine_config_service[] = "config_service";

const char key_dbus_json_success_key[] = "OK";
const char key_dbus_json_error_key[] = "ERROR";
const char key_dbus_json_agent_error_key[] = "ERROR Agent";
const char key_dbus_json_agent_msg_key[] = "MSG Agent";
const char key_dbus_json_signal_key[] = "SIGNAL";
const char key_connman_service[] = "net.connman";
const char key_connman_path[] = "/";

char key_agent_interface[] = "net.connman.Agent";
const char key_agent_error_message[] = "agent_error_message";

const char key_scan_return[] = "scan_return";
const char key_connect_return[] = "connect_return";
const char key_return_force_refresh[] = "return_force_refresh";
