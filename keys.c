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
const char key_engine_toggle_tech_power[] = "toggle_tech_power";
const char key_engine_toggle_offline_mode[] = "toggle_offline_mode";
const char key_engine_remove_service[] = "remove_service";

const char key_success[] = "OK";
const char key_error[] = "ERROR";
const char key_agent_error[] = "ERROR Agent";
const char key_agent_msg[] = "MSG Agent";
const char key_signal[] = "SIGNAL";
const char key_connman_service[] = "net.connman";
const char key_connman_path[] = "/";
const char key_manager_interface[] = "net.connman.Manager";
const char key_service_interface[] = "net.connman.Service";
const char key_technology_interface[] = "net.connman.Technology";
char key_agent_interface[] = "net.connman.Agent";

const char key_agent_error_message[] = "agent_error_message";
const char key_agent_error_callback[] = "agent_error_callback";
const char key_agent_msg_data[] = "agent_msg_data";
const char key_agent_request_browser[] = "Agent RequestBrowser";
const char key_agent_request_input[] = "Input Requested";

const char key_scan_return[] = "scan_return";
const char key_connect_return[] = "connect_return";
const char key_return_force_refresh[] = "return_force_refresh";

const char key_serv_state[] = "State";
const char key_serv_error[] = "Error";
const char key_serv_name[] = "Name";
const char key_serv_type[] = "Type";
const char key_serv_security[] = "Security";
const char key_serv_strength[] = "Strength";
const char key_serv_favorite[] = "Favorite";
const char key_serv_immutable[] = "Immutable";
const char key_serv_roaming[] = "Roaming";
const char key_serv_autoconnect[] = "AutoConnect";
const char key_serv_ethernet[] = "Ethernet";
const char key_serv_eth_method[] = "Method";
const char key_serv_eth_interface[] = "Interface";
const char key_serv_eth_address[] = "Address";
const char key_serv_eth_mtu[] = "MTU";
const char key_serv_eth_speed[] = "Speed";
const char key_serv_eth_duplex[] = "Duplex";
const char key_serv_ipv4[] = "IPv4";
const char key_serv_ipv4_netmask[] = "Netmask";
const char key_serv_ipv4_gateway[] = "Gateway";
const char key_serv_ipv4_config[] = "IPv4.Configuration";
const char key_serv_ipv6[] = "IPv6";
const char key_serv_ipv6_prefixlength[] = "PrefixLength";
const char key_serv_ipv6_privacy[] = "Privacy";
const char key_serv_ipv6_config[] = "IPv6.Configuration";
const char key_serv_nameservers[] = "Nameservers";
const char key_serv_nameservers_config[] = "Nameservers.Configuration";
const char key_serv_timeservers[] = "Timeservers";
const char key_serv_timeservers_config[] = "Timeservers.Configuration";
const char key_serv_domains[] = "Domains";
const char key_serv_domains_config[] = "Domains.Configuration";
const char key_serv_proxy[] = "Proxy";
const char key_serv_proxy_url[] = "URL";
const char key_serv_proxy_servers[] = "Servers";
const char key_serv_proxy_excludes[] = "Excludes";
const char key_serv_proxy_config[] = "Proxy.Configuration";
const char key_serv_prov[] = "Provider";
const char key_serv_prov_host[] = "Host";
const char key_serv_prov_domain[] = "Domain";
const char key_serv_prov_name[] = "Name";
const char key_serv_prov_type[] = "Type";

const char key_sig_serv_changed[]= "ServicesChanged";
const char key_sig_prop_changed[] = "PropertyChanged";
const char key_sig_tech_added[] = "TechnologyAdded";
const char key_sig_tech_removed[] = "TechnologyRemoved";
const char key_sig_unsupported[] = "Signal unsupported";
