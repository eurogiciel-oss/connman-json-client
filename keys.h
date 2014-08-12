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
extern const char key_engine_toogle_tech_power[];

extern const char key_success[];
extern const char key_error[];
extern const char key_agent_error[];
extern const char key_agent_msg[];
extern const char key_signal[];
extern const char key_connman_service[];
extern const char key_connman_path[];
extern const char key_manager_interface[];
extern const char key_service_interface[];
extern const char key_technology_interface[];
extern char key_agent_interface[];

extern const char key_agent_error_message[];
extern const char key_agent_error_callback[];
extern const char key_agent_msg_data[];

extern const char key_scan_return[];
extern const char key_connect_return[];
extern const char key_return_force_refresh[];

extern const char key_serv_state[];
extern const char key_serv_error[];
extern const char key_serv_name[];
extern const char key_serv_type[];
extern const char key_serv_security[];
extern const char key_serv_strength[];
extern const char key_serv_favorite[];
extern const char key_serv_immutable[];
extern const char key_serv_roaming[];
extern const char key_serv_autoconnect[];
extern const char key_serv_ethernet[];
extern const char key_serv_eth_method[];
extern const char key_serv_eth_interface[];
extern const char key_serv_eth_address[];
extern const char key_serv_eth_mtu[];
extern const char key_serv_eth_speed[];
extern const char key_serv_eth_duplex[];
extern const char key_serv_ipv4[];
#define key_serv_ipv4_method key_serv_eth_method
#define key_serv_ipv4_address key_serv_eth_address
extern const char key_serv_ipv4_netmask[];
extern const char key_serv_ipv4_gateway[];
extern const char key_serv_ipv4_config[];
extern const char key_serv_ipv6[];
#define key_serv_ipv6_method key_serv_eth_method
#define key_serv_ipv6_address key_serv_eth_address
extern const char key_serv_ipv6_prefixlength[];
#define key_serv_ipv6_gateway key_serv_ipv4_gateway
extern const char key_serv_ipv6_privacy[];
extern const char key_serv_ipv6_config[];
extern const char key_serv_nameservers[];
extern const char key_serv_nameservers_config[];
extern const char key_serv_timeservers[];
extern const char key_serv_timeservers_config[];
extern const char key_serv_domains[];
extern const char key_serv_domains_config[];
extern const char key_serv_proxy[];
#define key_serv_proxy_method key_serv_eth_method
extern const char key_serv_proxy_url[];
extern const char key_serv_proxy_servers[];
extern const char key_serv_proxy_excludes[];
extern const char key_serv_proxy_config[];
extern const char key_serv_prov[];
extern const char key_serv_prov_host[];
extern const char key_serv_prov_domain[];
extern const char key_serv_prov_name[];
extern const char key_serv_prov_type[];

#endif
