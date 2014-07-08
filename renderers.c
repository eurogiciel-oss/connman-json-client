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

#include "renderers.h"

extern WINDOW *win_header, *win_footer, *win_body;
extern MENU *my_menu;
extern ITEM **my_items;
extern int nb_items;

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
	char *desc_base = "%s (%s)      Powered %-5s      Connected %-6s";
	const char *k_name, *k_type, *k_powered, *k_connected;
	char *desc;
	struct json_object *sub_array, *tech_name, *tech_dict;

	nb_items = json_object_array_length(jobj);
	my_items = calloc(nb_items+1, sizeof(ITEM*));
	assert(my_items != NULL);

	for (i = 0; i < nb_items; i++) {
		sub_array = json_object_array_get_idx(jobj, i);
		tech_name = json_object_array_get_idx(sub_array, 0);
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

		desc = malloc(RENDERERS_STRING_MAX_LEN);
		snprintf(desc, RENDERERS_STRING_MAX_LEN-1, desc_base, k_name,
				k_type, k_powered, k_connected);
		desc[RENDERERS_STRING_MAX_LEN-1] = '\0';
		my_items[i] = new_item(json_object_get_string(tech_name),
				desc);
	}

	my_menu = new_menu(my_items);
	//set_menu_format(my_menu, LINES-6, 1);
	set_menu_win(my_menu, win_body);
	set_menu_sub(my_menu, derwin(win_body, LINES-10, COLS-4, 2, 3));
	post_menu(my_menu);

	wnoutrefresh(win_header);
	wnoutrefresh(win_body);
	wnoutrefresh(win_footer);
	doupdate();
}

static void free_technologies(void)
{
	int i;

	unpost_menu(my_menu);
	free_menu(my_menu);

	for (i = 0; i < nb_items; i++) {
		free((void *) my_items[i]->description.str);
		free_item(my_items[i]);
	}

	free(my_items);
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

	mvwprintw(win_header, 1, 1, "Connman ncurses UI");
	// 38 = len(string) + 1
	mvwprintw(win_header, 1, COLS-38, "State: %-6s%-6sOfflineMode: %-5s",
			state_str, "", json_object_to_json_string(offline_mode));
	wrefresh(win_header);
}

void __renderers_home_page(struct json_object *jobj)
{
	struct json_object *tech, *state;

	json_object_object_get_ex(jobj, "technologies", &tech);
	json_object_object_get_ex(jobj, "state", &state);

	renderers_technologies(tech);
	renderers_state(state);
}

void __renderers_free_home_page(void)
{
	free_technologies();
}
