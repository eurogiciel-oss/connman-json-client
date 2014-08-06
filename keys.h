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

#if !defined(_KEYS_H_)
#define _KEYS_H_

extern const char key_state[];
extern const char key_technologies[];
extern const char key_technology[];
extern const char key_service[];
extern const char key_services[];
extern const char key_options[];

extern const char key_command[];
extern const char key_command_data[];
extern const char key_command_path[];
extern const char key_command_interface[];

extern const char key_engine_get_state[];
extern const char key_engine_get_services[];
extern const char key_engine_get_technologies[];
extern const char key_engine_get_home_page[];
extern const char key_engine_get_services_from_tech[];
extern const char key_engine_connect[];
extern const char key_engine_disconnect[];
extern const char key_engine_agent_response[];
extern const char key_engine_agent_error_response[];
extern const char key_engine_agent_retry[];
extern const char key_engine_scan_tech[];
extern const char key_engine_config_service[];

extern const char key_dbus_json_success_key[];
extern const char key_dbus_json_error_key[];
extern const char key_dbus_json_agent_error_key[];
extern const char key_dbus_json_agent_msg_key[];
extern const char key_dbus_json_signal_key[];
extern const char key_connman_service[];
extern const char key_connman_path[];

extern char key_agent_interface[];
extern const char key_agent_error_message[];
extern const char key_agent_error_callback[];
extern const char key_agent_msg_data[];

extern const char key_scan_return[];
extern const char key_connect_return[];
extern const char key_return_force_refresh[];

#endif
