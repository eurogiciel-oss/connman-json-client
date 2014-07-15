/*
 *  connman-json-client
 *
 *  This file is meant for testing.
 *  A ncurses mode is implemented, just uncomment the code.
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

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <dbus/dbus.h>
#include <signal.h>
#include <assert.h>

#include "engine.h"
#include "loop.h"
#include "ncurses_utils.h"
#include "renderers.h"

/*

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
extern int nb_items;
extern int nb_fields;

void print_services_for_tech(struct userptr_data *data);
void action_on_service_config(struct userptr_data *data);

void print_home_page(void);

static struct {
	void (*func)(struct userptr_data *data);
	void (*func_free)();
	void (*func_back)();
} context_actions[] = {
	{ print_services_for_tech, __renderers_free_home_page, print_home_page }, // CONTEXT_HOME
	{ NULL, __renderers_free_service_config, print_home_page}, // CONTEXT_SERVICE_CONFIG
	{ NULL, NULL, NULL }, // CONTEXT_SERVICES
};

void stop_loop(int signum);
void (*engine_callback)(int status, struct json_object *jobj) = NULL;
void main_callback(int status, struct json_object *jobj);

void __connman_callback_ended(void)
{
	// This method is mandatory, and might be of use somehow
	return;
}

void stop_loop(int signum)
{
	loop_quit();

	if (context_actions[current_context].func_free)
		context_actions[current_context].func_free();
}

void print_home_page(void)
{
	struct json_object *cmd;

	__ncurses_print_info_in_footer(false, "asking for 'get_home_page'\n");
	cmd = json_object_new_object();
	json_object_object_add(cmd, ENGINE_KEY_COMMAND, json_object_new_string("get_home_page"));
	__engine_query(cmd);
}

void main_callback(int status, struct json_object *jobj)
{
	struct json_object *data, *cmd_tmp;
	const char *cmd_name;

	assert(status == 0);

	/* get the main object items */
	json_object_object_get_ex(jobj, "command_name", &cmd_tmp);
	assert(cmd_tmp != NULL);
	json_object_object_get_ex(jobj, "data", &data);
	cmd_name = json_object_get_string(cmd_tmp);
	assert(cmd_name != NULL);

	werase(win_body);
	box(win_body, 0, 0);

	/* dispatch according to the command name */
	if (strcmp("get_home_page", cmd_name) == 0)
		__renderers_home_page(data);
	else if (strcmp("get_services_from_tech", cmd_name) == 0)
		__renderers_services(data);
	else
		assert(1);

	/* release the memory of the json object now */
	json_object_put(jobj);
}

void print_services_for_tech(struct userptr_data *data)
{
	struct json_object *cmd, *tmp;

	cmd = json_object_new_object();
	tmp = json_object_new_object();

	json_object_object_add(cmd, ENGINE_KEY_COMMAND,
			json_object_new_string("get_services_from_tech"));
	json_object_object_add(tmp, "technology",
			json_object_new_string(data->dbus_name));
	json_object_object_add(cmd, ENGINE_KEY_CMD_DATA, tmp);

	mvwprintw(win_footer, 1, 1, "[INFO] 'Esc' to get back\n");
	__ncurses_print_info_in_footer(false, "asking for "
			"'get_services_from_tech'\n");

	if (__engine_query(cmd) == -EINVAL)
		__ncurses_print_info_in_footer(true, "@print_services_for_tech:"
				"invalid argument/value");
}

void exec_action(struct userptr_data *data)
{
	struct userptr_data tmp = *data;
	tmp.dbus_name = strdup(data->dbus_name);

	context_actions[current_context].func_free();
	context_actions[current_context].func(&tmp);

	free(tmp.dbus_name);
}

void exec_back(void)
{
	context_actions[current_context].func_free();
	context_actions[current_context].func_back();
}

void exec_action_context_home(int ch)
{
	ITEM *item;

	switch (ch) {
		case KEY_UP:
			menu_driver(my_menu, REQ_UP_ITEM);
			break;

		case KEY_DOWN:
			menu_driver(my_menu, REQ_DOWN_ITEM);
			break;

		case KEY_ENTER:
		case 10:
			item = current_item(my_menu);
			exec_action(item_userptr(item));
			break;

		case 27: // Escape
			__ncurses_print_info_in_footer(false, "^C to quit\n");
			break;
	}
}

void exec_action_context_services_config(int ch)
{
	FIELD *field;
	int cur_page = form_page(my_form);

	switch (ch) {
		case KEY_DOWN:
			form_driver(my_form, REQ_NEXT_FIELD);
			form_driver(my_form, REQ_END_LINE);
			break;

		case KEY_UP:
			form_driver(my_form, REQ_PREV_FIELD);
			form_driver(my_form, REQ_END_LINE);
			break;

		case KEY_NPAGE:
			form_driver(my_form, REQ_NEXT_PAGE);
			set_form_page(my_form, ++cur_page);
			__renderers_services_config_paging();
			break;

		case KEY_PPAGE:
			form_driver(my_form, REQ_PREV_PAGE);
			set_form_page(my_form, --cur_page);
			__renderers_services_config_paging();
			break;

		case KEY_ENTER:
		case 10:
			field = current_field(my_form);
			__ncurses_print_info_in_footer(false,
					field_buffer(field, 0));
			break;

		case 27: // Escape
			exec_back();
			break;
	}
}

void __ncurses_action(void)
{
	int ch = wgetch(win_body);

	switch (current_context) {
		case CONTEXT_HOME:
			exec_action_context_home(ch);
			break;

		case CONTEXT_SERVICE_CONFIG:
			exec_action_context_services_config(ch);
			break;

		case CONTEXT_SERVICES:
			__ncurses_print_info_in_footer(true, "not handled yet");
			break;
	}

	wrefresh(win_body);
}

int main(void)
{
	struct json_object *cmd;
	engine_callback = main_callback;

	if (__engine_init() < 0)
		exit(1);

	signal(SIGINT, stop_loop);
	loop_init();

	initscr();
	cbreak();
	noecho();
	keypad(stdscr, TRUE);

	win_body_lines = LINES - 8;
	
	// If you don't do this, the service config form won't be displayed
	while (win_body_lines % 4 != 3)
		win_body_lines--;

	win_body = newwin(win_body_lines + 2, COLS, 2, 0);
	box(win_body, 0 , 0);
	keypad(win_body, TRUE);

	win_header = newwin(2, COLS, 0, 0);
	win_footer = newwin(2, COLS, LINES-2, 0);

	// Print all windows, according to the man it's more efficient than 3
	wnoutrefresh(win_header);
	wnoutrefresh(win_body);
	wnoutrefresh(win_footer);
	doupdate();

	current_context = CONTEXT_HOME;

	// get_home_page (and render it)
	__ncurses_print_info_in_footer(false, "asking for 'get_home_page'\n");
	cmd = json_object_new_object();
	json_object_object_add(cmd, ENGINE_KEY_COMMAND, json_object_new_string("get_home_page"));
	__engine_query(cmd);

	loop_run(true);
	loop_terminate();

	delwin(win_header);
	delwin(win_body);
	delwin(win_footer);
	endwin();

	return 0;
}
