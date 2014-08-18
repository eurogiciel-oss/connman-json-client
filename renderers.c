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

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <json.h>
#include <errno.h>

#include "ncurses_utils.h"
#include "json_utils.h"
#include "string_utils.h"
#include "keys.h"

#include "renderers.h"

/*
 * This file handle the "rendering" work: create and free ncurses elements.
 */

/*
 * See main.c repos_cursor.
 */
extern void repos_cursor(void);

// We keep the number of items and fields for convenience.
int nb_items = 0;
int nb_fields = 0;

// Current position in y and x for field positions.
static int cur_y, cur_x;

/*
 * Windows (ncurses term) used,

 +-----------------------------------+
 |  win_header                       | <-- state area
 +-----------------------------------+
 |                                   |
 |                                   |
 |                                   |
 |  win_body                         |
 |                                   |
 |                                   |
 |                                   |
 +-----------------------------------+
 |  win_footer                       | <-- infos/errors area
 +-----------------------------------+

 */
WINDOW *win_header, *win_footer, *win_body, *inner;

// number of lines in win_body
int win_body_lines;

// Items (ncurses term), used by main_menu
ITEM **main_items;

// Menu (ncurses term)
MENU *main_menu;

// Field (ncurses term), used by main_form
FIELD **main_fields;

// Form (ncurses term)
FORM *main_form;

// Number of pages the form is displayed on (if the window is too small for the
// form, a scrolling form is created.
int nb_pages;

// Holds informations on the current context, see renderers.h.
struct context_info context;

// Allowed string values for service configuration fields
static char *true_false_enum[] = { "true", "false", NULL };
static char *ipv4_method_enum[] = { "dhcp", "manual", "off", NULL };
static char *ipv6_method_enum[] = { "auto", "manual", "off", "6to4", NULL };
static char *ipv6_privacy_enum[] = { "auto", "disabled", "enabled", "prefered", NULL };
static char *proxy_method_enum[] = { "direct", "manual", "auto", NULL };

// Used to tag service configuration fields for repos_cursor
static char str_field[2];

/*
 * This is useful to mark main_fields with a value for repos_cursor
 */
static char* get_str_key() {
	int cur_ch;
	cur_ch = str_field[0];

	if (cur_ch == '\0')
		cur_ch = 'a';
	else
		cur_ch++;

	str_field[0] = cur_ch;
	str_field[1] = '\0';

	return str_field;
}

/*
 * Create a menu of technologies: a selectable list of technologies.
 * @param jobj format of the json object:
 [
	 [
		 "\/net\/connman\/technology\/wifi",
		 {
			 "Name":"WiFi",
			 "Type":"wifi",
			 "Powered":true,
			 "Connected":false,
			 "Tethering":false
		 }
	 ],
	 [
		 "\/net\/connman\/technology\/ethernet",
		 {
			 "Name":"Wired",
			 "Type":"ethernet",
			 "Powered":true,
			 "Connected":true,
			 "Tethering":false
		 }
	 ]
 ]
 */
static void renderers_technologies(struct json_object *jobj)
{
	int i;
	char *desc_base = "%-20s Powered %-5s          Connected %-5s";
	char desc_base_sub[30];
	const char *k_name, *k_type, *k_powered, *k_connected;
	char *desc, *tech_short_name;
	struct json_object *sub_array, *dbus_tech_name, *tech_dict;
	struct userptr_data *data;

	nb_items = json_object_array_length(jobj);
	main_items = malloc(sizeof(ITEM*) * (nb_items+1));
	assert(main_items != NULL && nb_items > 0);

	for (i = 0; i < nb_items; i++) {
		sub_array = json_object_array_get_idx(jobj, i);

		if (!sub_array)
			continue;

		dbus_tech_name = json_object_array_get_idx(sub_array, 0);
		tech_dict = json_object_array_get_idx(sub_array, 1);

		json_object_object_foreach(tech_dict, key, val) {

			if (strcmp("Name", key) == 0)
				k_name = json_object_get_string(val);

			else if (strcmp("Type", key) == 0)
				k_type = json_object_get_string(val);

			else if (strcmp("Powered", key) == 0)
				k_powered = json_object_to_json_string(val);

			else if (strcmp("Connected", key) == 0)
				k_connected = json_object_to_json_string(val);
		}

		snprintf(desc_base_sub, 30, "%s (%s)", k_name, k_type);
		desc_base_sub[29] = '\0';

		desc = malloc(RENDERERS_STRING_MAX_LEN);
		assert(desc != NULL);
		snprintf(desc, RENDERERS_STRING_MAX_LEN-1, desc_base,
				desc_base_sub, k_powered, k_connected);
		desc[RENDERERS_STRING_MAX_LEN-1] = '\0';
		tech_short_name =
			extract_dbus_short_name(json_object_get_string(dbus_tech_name));
		main_items[i] = new_item(tech_short_name, desc);

		data = malloc(sizeof(struct userptr_data));
		assert(data != NULL);
		data->dbus_name = strdup(json_object_get_string(dbus_tech_name));
		data->pretty_name = strdup(k_name);
		set_item_userptr(main_items[i], data);
	}

	main_items[nb_items] = NULL;
	main_menu = new_menu(main_items);
	set_menu_win(main_menu, win_body);
	set_menu_sub(main_menu, derwin(win_body, win_body_lines-2, COLS-4, 3, 2));
	set_menu_mark(main_menu, "");
	set_menu_format(main_menu, win_body_lines-3, 1);
	assert(post_menu(main_menu) == E_OK);

	refresh_home_msg();
	repos_cursor();
	wnoutrefresh(win_header);
	wnoutrefresh(win_body);
	wnoutrefresh(win_footer);
	doupdate();
}

/*
 * Fill win_header with a title, state and OfflineMode informations.
 * @param jobj the state object of engine:
 {
	 "State": "ready",
	 "OfflineMode": false,
	 "SessionMode": false
 }
 */
void __renderers_state(struct json_object *jobj)
{
	struct json_object *state, *offline_mode;
	const char *state_str;

	json_object_object_get_ex(jobj, "State", &state);
	state_str = json_object_get_string(state);
	json_object_object_get_ex(jobj, "OfflineMode", &offline_mode);

	mvwprintw(win_header, 0, 1, "Connman ncurses UI");
	// 38 = len(string) + 1
	mvwprintw(win_header, 0, COLS-38, "State: %-6s%-6sOfflineMode: %-5s\n",
			state_str, "", json_object_get_string(offline_mode));
	redrawwin(win_header);
	wrefresh(win_header);
}

/*
 * The home page is a list of technologies and the general status of connman.
 * Thus, this function call renderers_technologies and __renderers_state.
 * @param jobj A json object with technologies and state:
 {
 	key_technologies: { ... },
	key_state: { ... }
 }
 */
void __renderers_home_page(struct json_object *jobj)
{
	struct json_object *tech, *state;

	json_object_object_get_ex(jobj, key_technologies, &tech);
	json_object_object_get_ex(jobj, key_state, &state);

	if (tech == NULL || state == NULL)
		return;

	werase(win_body);
	box(win_body, 0, 0);
	mvwprintw(win_body, 1, 2, "Technologies:");

	__renderers_state(state);
	renderers_technologies(tech);

	context.current_context = CONTEXT_HOME;
}

/*
 * Free the menu and items memory.
 */
void __renderers_free_home_page(void)
{
	int i;
	struct userptr_data *data;

	if (!main_menu)
		return;

	unpost_menu(main_menu);

	for (i = 0; i < nb_items; i++) {
		free((void *) main_items[i]->description.str);
		free((void *) main_items[i]->name.str);

		data = item_userptr(main_items[i]);
		free((void *) data->dbus_name);
		free((void *) data->pretty_name);
		free(data);

		free_item(main_items[i]);
	}

	free_menu(main_menu);
	free(main_items);
	nb_items = 0;
	main_menu = NULL;
	main_items = NULL;
}

/*
 * Return true if jobj has a Connected attribute set to true, false otherwise.
 * @param jobj A json object service dict
 */
static bool tech_is_connected(struct json_object *jobj)
{
	struct json_object *jbool;

	json_object_object_get_ex(jobj, "Connected", &jbool);

	return ((jbool && json_object_get_boolean(jbool)) ? true : false);
}

/*
 * This is used to compute how many fields are needed to display the service
 * configuration.
 * @param jobj The service dictionary
 */
static int compute_nb_elems_in_service(struct json_object *jobj)
{
	int longest_key_len = 0, tmp_len, tmp_len_obj = 0;

	json_object_object_foreach(jobj, key, val) {

		if (json_object_get_type(val) == json_type_object) {
			nb_fields++; // count 1 for the label of the subsection
			tmp_len_obj = compute_nb_elems_in_service(val);

		} else
			nb_fields += 2; // count 2 for a label and a field

		if (key && (tmp_len = strlen(key)) > longest_key_len)
			longest_key_len = tmp_len;
	}

	return longest_key_len > tmp_len_obj ? longest_key_len : tmp_len_obj;
}

/*
 * Create a field and fill the buffer with the string representation of val.
 * This field can be visited by the cursor, it also dynamicaly resize (in buffer
 * length).
 * @param longest_key_len What is the longest length of a label field, this
 *	information is used to place the field on the x axis
 * @param val The json object representation to print in the field buffer
 */
static FIELD* render_field(int longest_key_len, struct json_object *val)
{
	FIELD *result;

	result = new_field(1, 30, cur_y, longest_key_len, 0, 0);

	if (result) {
		field_opts_off(result, O_AUTOSKIP);
		field_opts_off(result, O_STATIC);
		set_field_buffer(result, 0, json_object_get_string(val));
	}

	return result;
}

/*
 * Create a field and fill the buffer with label_str. This field cannot be
 * modified nor visited by the cursor.
 * @param longest_key_len What is the longest length of a label field, this
 *	information is used to place the field on the x axis
 * @param label_str The string to fill the label buffer with
 */
static FIELD* render_label(int longest_key_len, const char *label_str)
{
	FIELD *result;

	result = new_field(1, longest_key_len-4, cur_y, cur_x, 0, 0);

	if (result) {
		field_opts_on(result, O_VISIBLE);
		field_opts_off(result, O_EDIT);
		field_opts_off(result, O_ACTIVE);
		set_field_buffer(result, 0, label_str);
	}

	return result;
}

/*
 * This sets basic form validation, for example IPv4 Method can be one of
 * ipv4_method_enum.
 * @param pos The index of the main_fields in the global variable
 * @param is_autoconnect Indicate if this is the "AutoConnect" field.
 * @param obj_str The string of the label surrounding main_fields[pos]
 * @param key The string of the current field
 */
static void config_fields_type(int pos, bool is_autoconnect, const char *obj_str,
		const char *key)
{
	if (is_autoconnect) {
		set_field_type(main_fields[pos], TYPE_ENUM, true_false_enum, 0, 1);
		return;
	}

	if (strcmp(key, "Privacy") == 0) {
		set_field_type(main_fields[pos], TYPE_ENUM, ipv6_privacy_enum, 0, 1);
		return;
	}

	// Only "Method" attributes below this point.
	if (strcmp(key, "Method") != 0 || obj_str == NULL)
		return;

	if (strcmp(obj_str, "IPv4.Configuration") == 0) {
		set_field_type(main_fields[pos], TYPE_ENUM, ipv4_method_enum, 0, 1);
		return;
	}

	if (strcmp(obj_str, "IPv6.Configuration") == 0) {
		set_field_type(main_fields[pos], TYPE_ENUM, ipv6_method_enum, 0, 1);
		return;
	}

	if (strcmp(obj_str, "Proxy.Configuration") == 0) {
		set_field_type(main_fields[pos], TYPE_ENUM, proxy_method_enum, 0, 1);
		return;
	}
}

/*
 * Recursive function responsible to create fields, position them and setting
 * the modifiable fields. All of this from a service dictionary.
 * To keep the cursor from moving on signal, a string is affected to mark each
 * field. This is used by repos_cursor().
 * @param longest_key_len The longuest length for a label
 * @param pos The index in main_fields[]
 * @param jobj The service dictionary
 * @param is_obj_modifiable Use to set the whole object as modifiable (usefull
 *	for IPv4.Configuration for example)
 * @param obj_str The string representing the "hash" of a surrounding object
 */
static void render_fields_from_jobj(int longest_key_len, int *pos,
		struct json_object *jobj, bool is_obj_modifiable,
		const char *obj_str)
{
	bool is_autoconnect = false, is_modifiable = false;
	struct userptr_data *data;

	json_object_object_foreach(jobj, key, val) {
		main_fields[*pos] = render_label(longest_key_len, key);
		assert(main_fields[*pos] != NULL);
		(*pos)++;

		is_modifiable = string_ends_with_configuration(key);
		is_modifiable |= is_obj_modifiable;

		if (json_object_get_type(val) == json_type_object) {
			move_field(main_fields[(*pos)-1], ++cur_y, cur_x);
			cur_y++;
			render_fields_from_jobj(longest_key_len, pos, val,
					is_modifiable, key);
			is_modifiable = false;
		} else {
			// insert the page delimiter
			if (cur_y >= win_body_lines-2) {
				cur_y = 1;
				set_new_page(main_fields[(*pos)-1], TRUE);
				move_field(main_fields[(*pos)-1], cur_y, cur_x);
				nb_pages++;
			}

			main_fields[*pos] = render_field(longest_key_len, val);
			assert(main_fields[*pos] != NULL);
			is_autoconnect = strcmp(key, "AutoConnect") == 0;

			if (is_modifiable || is_autoconnect) {
				field_opts_on(main_fields[*pos], O_EDIT);
				field_opts_off(main_fields[*pos], O_BLANK);
				set_field_back(main_fields[*pos], A_UNDERLINE);
			} else
				field_opts_off(main_fields[*pos], O_EDIT);

			// Specific operations on fields
			config_fields_type(*pos, is_autoconnect, obj_str, key);

			field_opts_on(main_fields[*pos], O_NULLOK);
			data = malloc(sizeof(struct userptr_data));
			data->dbus_name = strdup(get_str_key());
			data->pretty_name = NULL;
			set_field_userptr(main_fields[*pos], data);

			(*pos)++;
		}

		cur_y++;
	}
}

/*
 * Compute the number of needed fields, allocate the memory, render fields and
 * print them. The result is a form (html-like) displaying every information
 * connman has on a service, some of the settings can be modified. See
 * connman/doc/services-api.txt for more informations.
 * The first label contain the service name in the user pointer.
 * @param tech_array Array of json objects representing technologies
 * @param serv_array Array of json objects representing services
 */
static void renderers_service_config(struct json_object *tech_array,
		struct json_object *serv_array)
{
	struct json_object *serv_dict, *tmp, *tmp_val;
	int longest_key_len, i, k;
	struct userptr_data *data;
	const char *serv_dbus_name;
	const char *keys[] = { key_serv_state, key_serv_error, key_serv_name,
		key_serv_type, key_serv_security, key_serv_strength,
		key_serv_favorite, key_serv_immutable, key_serv_roaming,
		key_serv_autoconnect, key_serv_ethernet, key_serv_ipv4,
		key_serv_ipv4_config, key_serv_ipv6, key_serv_ipv6_config,
		key_serv_nameservers, key_serv_nameservers_config,
		key_serv_timeservers, key_serv_timeservers_config,
		key_serv_domains, key_serv_domains_config, key_serv_proxy,
		key_serv_proxy_config, key_serv_prov, NULL };

	nb_fields = 0;
	cur_y = 1;
	cur_x = 1;
	serv_dict = json_object_array_get_idx(serv_array, 1);
	serv_dbus_name = json_object_get_string(
			json_object_array_get_idx(serv_array, 0));

	// We compute how many fields we will need
	longest_key_len = compute_nb_elems_in_service(serv_dict);

	main_fields = malloc(sizeof(ITEM *) * (nb_fields + 1));
	longest_key_len += 4; // For padding
	i = 0;

	str_field[0] = '\0';

	for (k = 0; keys[k] != NULL && i < nb_fields; k++) {
		if (json_object_object_get_ex(serv_dict, keys[k], &tmp_val) == TRUE) {
			tmp = json_object_new_object();
			json_object_object_add(tmp, keys[k], json_object_get(tmp_val));
			render_fields_from_jobj(longest_key_len, &i, tmp, false, NULL);
			json_object_put(tmp);
		}
	}

	data = malloc(sizeof(struct userptr_data));
	data->dbus_name = strdup(serv_dbus_name);
	data->pretty_name = NULL;
	set_field_userptr(main_fields[0], data);
	main_fields[i] = NULL;

	main_form = new_form(main_fields);
	assert(main_form != NULL);
	set_form_win(main_form, win_body);
	inner = derwin(win_body, win_body_lines-1, COLS-2, 2, 1);
	box(inner, 0, 0);
	assert(inner != NULL);
	set_form_sub(main_form, inner);

	post_form(main_form);
	__renderers_services_config_paging();
	refresh_service_config_msg();
	repos_cursor();
}

/*
 * This function update the message on the service configuration view.
 * It's realy meaningful when the configuration form can't be displayed entirely
 * on the screen.
 */
void __renderers_services_config_paging(void)
{
	if (!main_form)
		return;

	if (!nb_pages)
		mvwprintw(win_body, 1, 2, "Service configuration :");
	else
		mvwprintw(win_body, 1, 2, "Service configuration (page %d/%d, use "
				"page_up/page_down to change page):",
				form_page(main_form)+1, main_form->maxpage);

	pos_form_cursor(main_form);
}

/*
 * Render the services items for ethernet technology.
 * Only the "Name" (connman term) is displayed.
 * @param jobj The services array
 */
static void renderers_services_ethernet(struct json_object *jobj)
{
	int i;
	// Name
	char *desc_base = "%-33s", *desc;
	const char *name_str, *dbus_name_str;
	struct json_object *sub_array, *serv_name, *serv_dict, *tmp;
	struct userptr_data *data;

	mvwprintw(win_body, 3, 2, "%-33s", "Name");

	for (i = 0; i < nb_items; i++) {
		sub_array = json_object_array_get_idx(jobj, i);
		serv_name = json_object_array_get_idx(sub_array, 0);
		serv_dict = json_object_array_get_idx(sub_array, 1);

		json_object_object_get_ex(serv_dict, "Name", &tmp);
		name_str = json_object_get_string(tmp);

		desc = malloc(RENDERERS_STRING_MAX_LEN);
		assert(desc != NULL);
		snprintf(desc, RENDERERS_STRING_MAX_LEN-1, desc_base, name_str);
		desc[RENDERERS_STRING_MAX_LEN-1] = '\0';

		dbus_name_str = json_object_get_string(serv_name);

		main_items[i] = new_item(desc, "");

		data = malloc(sizeof(struct userptr_data));
		assert(data != NULL);
		data->dbus_name = strdup(dbus_name_str);
		data->pretty_name = strdup(name_str);
		set_item_userptr(main_items[i], data);
	}
}

/*
 * Render the services items for wifi technology.
 * eSSID, State, Security and Signal strengh are displayed.
 * @param jobj The services array
 */
static void renderers_services_wifi(struct json_object *jobj)
{
	int i;
	// eSSID  State  Security  Signal strengh
	char *desc_base = "%-33s%-17s%-19s%u%%", *desc;
	const char *essid_str, *state_str, *security_str, *serv_name_str;
	uint8_t signal_strength;
	struct json_object *sub_array, *serv_name, *serv_dict, *tmp;
	struct userptr_data *data;

	mvwprintw(win_body, 3, 2, "%-33s%-17s%-10s%17s", "eSSID", "State", "Security", "Signal"
			" Strength");

	for (i = 0; i < nb_items; i++) {
		sub_array = json_object_array_get_idx(jobj, i);
		serv_name = json_object_array_get_idx(sub_array, 0);
		serv_dict = json_object_array_get_idx(sub_array, 1);

		json_object_object_get_ex(serv_dict, "Name", &tmp);

		// hidden wifi
		if (tmp)
			essid_str = json_object_get_string(tmp);
		else
			essid_str = "[hidden]";

		json_object_object_get_ex(serv_dict, "Security", &tmp);
		assert(tmp != NULL);
		security_str = json_object_get_string(tmp);

		json_object_object_get_ex(serv_dict, "Strength", &tmp);
		assert(tmp != NULL);
		signal_strength = (uint8_t) json_object_get_int(tmp);

		json_object_object_get_ex(serv_dict, "State", &tmp);
		assert(tmp != NULL);
		state_str = json_object_get_string(tmp);

		desc = malloc(RENDERERS_STRING_MAX_LEN);
		assert(desc != NULL);
		snprintf(desc, RENDERERS_STRING_MAX_LEN-1, desc_base, essid_str,
			state_str, security_str, signal_strength);
		desc[RENDERERS_STRING_MAX_LEN-1] = '\0';

		serv_name_str = json_object_get_string(serv_name);

		main_items[i] = new_item(desc, "");

		data = malloc(sizeof(struct userptr_data));
		assert(data != NULL);
		data->dbus_name = strdup(serv_name_str);
		data->pretty_name = strdup(essid_str);
		set_item_userptr(main_items[i], data);
	}
}

/*
 * Allocate memory, render the items and print them. The result is a menu of
 * compatible services the user can connect to.
 * The switch between wifi and ethernet technoloy is done here.
 * @param jobj The services array
 */
static void renderers_services(struct json_object *jobj)
{
	char *dbus_short_name;
	struct json_object *array, *dbus_long_name;

	main_menu = NULL;
	nb_items = json_object_array_length(jobj);

	if (nb_items == 0) {
		mvwprintw(win_body, 1, 2, "No suitable services found for this"
				" technology");
		wrefresh(win_body);
		return;
	}

	array = json_object_array_get_idx(jobj, 0);
	dbus_long_name = json_object_array_get_idx(array, 0);

	dbus_short_name = extract_dbus_short_name(json_object_get_string(dbus_long_name));
	main_items = malloc(sizeof(ITEM *) * (nb_items+1));
	assert(main_items != NULL);

	if (strncmp(dbus_short_name, "ethernet_", 9) == 0)
		renderers_services_ethernet(jobj);
	else if (strncmp(dbus_short_name, "wifi_", 5) == 0)
		renderers_services_wifi(jobj);
	else
		assert(true);

	free(dbus_short_name);

	mvwprintw(win_body, 1, 2, "Choose a network to connect to:");

	main_items[nb_items] = NULL;
	main_menu = new_menu(main_items);
	set_menu_win(main_menu, win_body);
	set_menu_sub(main_menu, derwin(win_body, win_body_lines-3, COLS-4, 4, 2));
	menu_opts_off(main_menu, O_SHOWDESC);
	set_menu_mark(main_menu, "");
	set_menu_format(main_menu, win_body_lines-3, 1);
	assert(post_menu(main_menu) == E_OK);

	refresh_services_msg();
	repos_cursor();
	wrefresh(win_body);
}

/*
 * Free the memory allocated for the services view by renderers_services.
 */
void __renderers_free_services(void)
{
	int i;
	struct userptr_data *data;

	if (main_menu == NULL)
		return;

	unpost_menu(main_menu);

	for (i = 0; i < nb_items; i++) {
		data = item_userptr(main_items[i]);
		free((void *) data->dbus_name);
		free((void *) data->pretty_name);
		free(data);
		free_item(main_items[i]);
	}

	free_menu(main_menu);
	free(main_items);
	nb_items = 0;
	main_menu = NULL;
	main_items = NULL;
}

/*
 * Render the service configuration view or the services connection view
 * depending if the technology is connected or not. See renderers_services and
 * renderers_service_config for more information on those.
 * The context global variable is modified here.
 * @param jobj The following json object:
 {
 	key_technology: [ "technology dbus name", { tech dict } ],
	key_services: [ [ "service dbus name", { serv dict } ], ... ]
 }
 */
void __renderers_services(struct json_object *jobj)
{
	struct json_object *tech_array, *tech_dict, *serv_array;

	json_object_object_get_ex(jobj, key_technology, &tech_array);
	json_object_object_get_ex(jobj, key_services, &serv_array);

	if (tech_array == NULL || serv_array == NULL)
		return;

	tech_dict = json_object_array_get_idx(tech_array, 1);
	nb_pages = 0;
	werase(win_body);
	box(win_body, 0, 0);

	if (tech_is_connected(tech_dict)) {
		if (!serv_array || json_object_array_length(serv_array) == 0) {
			main_menu = NULL;
			return;
		}

		// propose modifications of service parameters
		renderers_service_config(tech_array,
				json_object_array_get_idx(serv_array, 0));
		context.current_context = CONTEXT_SERVICE_CONFIG;
		__renderers_services_config_paging();
	} else {
		// propose to connect to one service
		renderers_services(serv_array);
		context.current_context = CONTEXT_SERVICES;
	}
}

/*
 * Free the allocated memory for the service configuration view.
 */
void __renderers_free_service_config(void)
{
	int i;
	struct userptr_data *tmp;

	if (main_form == NULL)
		return;

	unpost_form(main_form);

	for (i = 0; i < nb_fields && main_fields[i] != NULL; i++) {
		tmp = field_userptr(main_fields[i]);

		if (tmp) {
			if (tmp->dbus_name)
				free(tmp->dbus_name);

			if (tmp->pretty_name)
				free(tmp->pretty_name);

			free(tmp);
		}

		free_field(main_fields[i]);
	}

	free_form(main_form);
	free(main_fields);
	nb_fields = 0;
	main_fields = NULL;
	main_form = NULL;
}
