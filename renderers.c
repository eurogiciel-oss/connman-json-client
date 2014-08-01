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
#include <json/json.h>

#include "ncurses_utils.h"
#include "json_utils.h"
#include "keys.h"

#include "renderers.h"

extern void repos_cursor(void);

int nb_items = 0;
int nb_fields = 0;

static int cur_y, cur_x;

WINDOW *win_header, *win_footer, *win_body, *inner;
ITEM **my_items;
MENU *my_menu;
FIELD **field;
FORM *my_form;
int win_body_lines;
int nb_pages;
struct context_info context;

char *true_false_enum[] = { "true", "false", NULL };
char str_field[2];

/*
 * This is useful to mark field with a value for repos_cursor()
 */
char* get_str_key() {
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
	my_items = malloc(sizeof(ITEM*) * (nb_items+1));
	assert(my_items != NULL && nb_items > 0);

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
			__extract_dbus_short_name(json_object_get_string(dbus_tech_name));
		my_items[i] = new_item(tech_short_name, desc);

		data = malloc(sizeof(struct userptr_data));
		assert(data != NULL);
		data->dbus_name = strdup(json_object_get_string(dbus_tech_name));
		data->pretty_name = strdup(k_name);
		set_item_userptr(my_items[i], data);
	}

	my_items[nb_items] = NULL;
	my_menu = new_menu(my_items);
	set_menu_win(my_menu, win_body);
	set_menu_sub(my_menu, derwin(win_body, win_body_lines-2, COLS-4, 3, 2));
	set_menu_mark(my_menu, "");
	set_menu_format(my_menu, win_body_lines-3, 1);
	assert(post_menu(my_menu) == E_OK);

	repos_cursor();
	wnoutrefresh(win_header);
	wnoutrefresh(win_body);
	wnoutrefresh(win_footer);
	doupdate();
}

/*
 {
	 "State": "ready",
	 "OfflineMode": false,
	 "SessionMode": false
 }
 */
static void renderers_state(struct json_object *jobj)
{
	struct json_object *state, *offline_mode;
	const char *state_str;

	json_object_object_get_ex(jobj, "State", &state);
	state_str = json_object_get_string(state);
	json_object_object_get_ex(jobj, "OfflineMode", &offline_mode);

	mvwprintw(win_header, 0, 1, "Connman ncurses UI");
	// 38 = len(string) + 1
	mvwprintw(win_header, 0, COLS-38, "State: %-6s%-6sOfflineMode: %-5s",
			state_str, "", json_object_get_string(offline_mode));
	wrefresh(win_header);
}

void __renderers_home_page(struct json_object *jobj)
{
	struct json_object *tech, *state;

	json_object_object_get_ex(jobj, "technologies", &tech);
	json_object_object_get_ex(jobj, "state", &state);

	werase(win_body);
	box(win_body, 0, 0);
	mvwprintw(win_body, 1, 2, "Technologies:");

	renderers_state(state);
	renderers_technologies(tech);

	context.current_context = CONTEXT_HOME;
}

void __renderers_free_home_page(void)
{
	int i;
	struct userptr_data *data;

	unpost_menu(my_menu);

	for (i = 0; i < nb_items; i++) {
		free((void *) my_items[i]->description.str);
		free((void *) my_items[i]->name.str);

		data = item_userptr(my_items[i]);
		free((void *) data->dbus_name);
		free((void *) data->pretty_name);
		free(data);

		free_item(my_items[i]);
	}

	free_menu(my_menu);
	free(my_items);
	nb_items = 0;
	my_menu = NULL;
	my_items = NULL;
}

/*
 { ..., "Connected":true, ...  } -> true | false
 */
static bool tech_is_connected(struct json_object *jobj)
{
	struct json_object *jbool;

	json_object_object_get_ex(jobj, "Connected", &jbool);

	return (json_object_get_boolean(jbool) ? true : false);
}

static int compute_nb_elems_in_service(struct json_object *jobj)
{
	int longest_key_len = 0, tmp_len, tmp_len_obj = 0;

	json_object_object_foreach(jobj, key, val) {

		if (json_object_get_type(val) == json_type_object) {
			nb_fields++; // count 1 for the label of the subsection
			tmp_len_obj = compute_nb_elems_in_service(val);
		} else {
			nb_fields += 2; // count 2 for a label and a field
		}

		if ((tmp_len = strlen(key)) > longest_key_len)
			longest_key_len = tmp_len;
	}

	return longest_key_len > tmp_len_obj ? longest_key_len : tmp_len_obj;
}

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
 * Test if a given string ends with ".Configuration"
 */
static bool string_ends_with_configuration(const char *str)
{
	char *last_token;

	last_token = strrchr(str, '.');

	if (last_token)
		last_token++;

	return (last_token && strcmp(last_token, "Configuration") == 0);
}

static void render_fields_from_jobj(int longest_key_len, int *pos,
		struct json_object *jobj, bool is_modifiable)
{
	bool is_autoconnect = false;
	struct userptr_data *data;

	json_object_object_foreach(jobj, key, val) {
		field[*pos] = render_label(longest_key_len, key);
		assert(field[*pos] != NULL);
		(*pos)++;

		is_modifiable = string_ends_with_configuration(key);

		if (json_object_get_type(val) == json_type_object) {
			move_field(field[(*pos)-1], ++cur_y, cur_x);
			cur_y++;
			render_fields_from_jobj(longest_key_len, pos, val,
					is_modifiable);
			is_modifiable = false;
		} else {
			// insert the page delimiter
			if (cur_y+1 >= win_body_lines-2) {
				cur_y = 1;
				set_new_page(field[(*pos)-1], TRUE);
				move_field(field[(*pos)-1], cur_y, cur_x);
				nb_pages++;
			}

			field[*pos] = render_field(longest_key_len, val);
			assert(field[*pos] != NULL);
			is_autoconnect = strcmp(key, "AutoConnect") == 0;

			if (is_modifiable || is_autoconnect) {
				field_opts_on(field[*pos], O_EDIT);
				set_field_back(field[*pos], A_UNDERLINE);
			} else
				field_opts_off(field[*pos], O_EDIT);

			field_opts_on(field[*pos], O_NULLOK);
			data = malloc(sizeof(struct userptr_data));
			data->dbus_name = strdup(get_str_key());
			data->pretty_name = NULL;
			set_field_userptr(field[*pos], data);

			(*pos)++;
		}

		cur_y++;
	}
}

static void renderers_service_config(struct json_object *tech_array,
		struct json_object *serv_array)
{
	struct json_object *serv_dict;
	int longest_key_len, i;

	nb_fields = 0;
	cur_y = 1;
	cur_x = 1;
	serv_dict = json_object_array_get_idx(serv_array, 1);

	// We compute how many fields we will need
	longest_key_len = compute_nb_elems_in_service(serv_dict);

	field = malloc(sizeof(ITEM *) * (nb_fields + 1));
	longest_key_len += 4; // For padding
	i = 0;

	str_field[0] = '\0';
	render_fields_from_jobj(longest_key_len, &i, serv_dict, false);

	field[nb_fields] = NULL;

	my_form = new_form(field);
	assert(my_form);
	set_form_win(my_form, win_body);
	inner = derwin(win_body, win_body_lines-1, COLS-2, 2, 1);
	box(inner, 0, 0);
	assert(inner);
	set_form_sub(my_form, inner);

	post_form(my_form);
	__renderers_services_config_paging();
	repos_cursor();
}

void __renderers_services_config_paging(void)
{
	if (!nb_pages)
		mvwprintw(win_body, 1, 2, "Service configuration :");
	else
		mvwprintw(win_body, 1, 2, "Service configuration (page %d/%d, use "
				"page_up/page_down to change page):",
				form_page(my_form)+1, my_form->maxpage);

	pos_form_cursor(my_form);
}

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

		my_items[i] = new_item(desc, "");

		data = malloc(sizeof(struct userptr_data *));
		assert(data != NULL);
		data->dbus_name = strdup(dbus_name_str);
		data->pretty_name = strdup(name_str);
		set_item_userptr(my_items[i], data);
	}
}

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

		my_items[i] = new_item(desc, "");

		data = malloc(sizeof(struct userptr_data));
		assert(data != NULL);
		data->dbus_name = strdup(serv_name_str);
		data->pretty_name = strdup(essid_str);
		set_item_userptr(my_items[i], data);
	}
}

static void renderers_services(struct json_object *jobj)
{
	char *dbus_short_name;
	struct json_object *array, *dbus_long_name;

	nb_items = json_object_array_length(jobj);

	if (nb_items == 0) {
		mvwprintw(win_body, 1, 2, "No suitable services found for this"
				" technology");
		wrefresh(win_body);
		return;
	}

	array = json_object_array_get_idx(jobj, 0);
	dbus_long_name = json_object_array_get_idx(array, 0);

	dbus_short_name = __extract_dbus_short_name(json_object_get_string(dbus_long_name));
	my_items = malloc(sizeof(ITEM *) * (nb_items+1));
	assert(my_items != NULL);

	if (strncmp(dbus_short_name, "ethernet_", 9) == 0)
		renderers_services_ethernet(jobj);
	else if (strncmp(dbus_short_name, "wifi_", 5) == 0)
		renderers_services_wifi(jobj);
	else
		assert(true);

	free(dbus_short_name);

	mvwprintw(win_body, 1, 2, "Choose a network to connect to:");

	my_items[nb_items] = NULL;
	my_menu = new_menu(my_items);
	set_menu_win(my_menu, win_body);
	set_menu_sub(my_menu, derwin(win_body, win_body_lines-3, COLS-4, 4, 2));
	menu_opts_off(my_menu, O_SHOWDESC);
	set_menu_mark(my_menu, "");
	set_menu_format(my_menu, win_body_lines-3, 1);
	assert(post_menu(my_menu) == E_OK);

	repos_cursor();
	wrefresh(win_body);
}

void __renderers_free_services(void)
{
	int i;
	struct userptr_data *data;

	if (nb_items == 0)
		return;

	unpost_menu(my_menu);

	for (i = 0; i < nb_items; i++) {
		data = item_userptr(my_items[i]);
		free((void *) data->dbus_name);
		free((void *) data->pretty_name);
		free(data);
		free_item(my_items[i]);
	}

	free_menu(my_menu);
	free(my_items);
	nb_items = 0;
}

void __renderers_services(struct json_object *jobj)
{
	struct json_object *tech_array, *tech_dict, *serv_array;

	json_object_object_get_ex(jobj, key_technology, &tech_array);
	json_object_object_get_ex(jobj, key_services, &serv_array);

	tech_dict = json_object_array_get_idx(tech_array, 1);
	nb_pages = 0;
	werase(win_body);
	box(win_body, 0, 0);

	if (tech_is_connected(tech_dict)) {
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

void __renderers_free_service_config(void)
{
	int i;
	struct userptr_data *tmp;

	unpost_form(my_form);

	for (i = 0; i < nb_fields; i++) {
		tmp = field_userptr(field[i]);

		if (tmp) {
			if (tmp->dbus_name)
				free(tmp->dbus_name);

			if (tmp->pretty_name)
				free(tmp->pretty_name);

			free(tmp);
		}

		free_field(field[i]);
	}

	free_form(my_form);
	free(field);
	nb_fields = 0;
}
