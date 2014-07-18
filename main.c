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
#include "keys.h"

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
extern struct context_info context;

void print_services_for_tech();
void connect_to_service();

void print_home_page(void);
void exec_refresh(void);

static struct {
	void (*func)(); // What to do on action
	void (*func_free)(); // Free elements for the current_context
	void (*func_back)(); // What to do on 'Esc'
	void (*func_refresh)(); // Refresh the current_context
} context_actions[] = {
	{ print_services_for_tech, __renderers_free_home_page, print_home_page,
		print_home_page }, // CONTEXT_HOME
	{ NULL, __renderers_free_service_config, print_home_page,
		print_services_for_tech }, // CONTEXT_SERVICE_CONFIG
	{ connect_to_service, __renderers_free_services, print_home_page,
		print_services_for_tech }, // CONTEXT_SERVICES
};

void __connman_callback_ended(void)
{
	// This method is mandatory, and might be of use somehow
	return;
}

void stop_loop(int signum)
{
	loop_quit();

	if (context_actions[context.current_context].func_free)
		context_actions[context.current_context].func_free();

	engine_terminate();
	free(context.serv);
	free(context.tech);
}

void exec_refresh()
{
	context_actions[context.current_context].func_free();
	context_actions[context.current_context].func_refresh();
}

void exec_action(struct userptr_data *data)
{
	switch (context.current_context) {
		case CONTEXT_SERVICES:
			context.serv->dbus_name = strdup(data->dbus_name);
			break;

		case CONTEXT_HOME:
			context.tech->dbus_name = strdup(data->dbus_name);
			break;

		default:
			break;
	}

	context_actions[context.current_context].func_free();
	context_actions[context.current_context].func();
}

void exec_back(void)
{
	if (context.serv && context.serv->dbus_name)
		free(context.serv->dbus_name);

	if (context.tech && context.tech->dbus_name)
		free(context.tech->dbus_name);

	context.serv->dbus_name = NULL;
	context.tech->dbus_name = NULL;
	context_actions[context.current_context].func_free();
	context_actions[context.current_context].func_back();
}

void action_on_signal(struct json_object *jobj)
{
	struct json_object *sig_path, *sig_data;
	bool is_tech_removed, is_current_tech;

	json_object_object_get_ex(jobj, key_command_path, &sig_path);
	json_object_object_get_ex(jobj, key_command_data, &sig_data);
	
	is_tech_removed = strncmp("TechnologyRemoved", json_object_get_string(sig_path), 50) == 0;

	if (context.tech->dbus_name == NULL)
		is_current_tech = false;
	else
		is_current_tech = strncmp(context.tech->dbus_name, json_object_get_string(sig_data), 256) == 0;

	if (is_tech_removed && is_current_tech)
		exec_back();
	else
		exec_refresh();
}

void main_callback(int status, struct json_object *jobj)
{
	struct json_object *data, *cmd_tmp, *error;
	const char *cmd_name;

	json_object_object_get_ex(jobj, "ERROR", &error);
	if (error) {
		__ncurses_print_info_in_footer(true, json_object_get_string(
					json_object_array_get_idx(error, 0)));
		json_object_put(jobj);
		return;
	}

	/* get the main object items */
	json_object_object_get_ex(jobj, key_command, &cmd_tmp);
	json_object_object_get_ex(jobj, key_command_data, &data);
	cmd_name = json_object_get_string(cmd_tmp);

	werase(win_body);
	box(win_body, 0, 0);

	if (!cmd_name) {
		action_on_signal(jobj);
	} else {

		/* dispatch according to the command name */
		if (strcmp("get_home_page", cmd_name) == 0)
			__renderers_home_page(data);
		else if (strcmp("get_services_from_tech", cmd_name) == 0)
			__renderers_services(data);
		else {
			mvwprintw(win_footer, 0, 1, "[INFO]: unknown command called"
					" back: status %d\tjobj %s\n",
					json_object_get_string(jobj));
			wrefresh(win_footer);
		}
	}

	/* release the memory of the json object now */
	json_object_put(jobj);
}

void print_home_page(void)
{
	struct json_object *cmd;

	werase(win_footer);
	cmd = json_object_new_object();
	json_object_object_add(cmd, key_command, json_object_new_string("get_home_page"));
	engine_query(cmd);
}

void print_services_for_tech()
{
	struct json_object *cmd, *tmp;

	cmd = json_object_new_object();
	tmp = json_object_new_object();

	json_object_object_add(cmd, key_command,
			json_object_new_string("get_services_from_tech"));
	json_object_object_add(tmp, "technology",
			json_object_new_string(context.tech->dbus_name));
	json_object_object_add(cmd, key_command_data, tmp);
	__ncurses_print_info_in_footer(false, "'Esc' to get back\n");

	if (engine_query(cmd) == -EINVAL)
		__ncurses_print_info_in_footer(true, "@print_services_for_tech:"
				"invalid argument/value");
}

void connect_to_service()
{
	struct json_object *cmd, *tmp;

	cmd = json_object_new_object();
	tmp = json_object_new_object();

	json_object_object_add(cmd, key_command,
			json_object_new_string("connect"));
	json_object_object_add(tmp, "service",
			json_object_new_string(context.serv->dbus_name));
	json_object_object_add(cmd, key_command_data, tmp);

	if (engine_query(cmd) == -EINVAL)
		__ncurses_print_info_in_footer(true, "@connect_to_service:"
				"invalid argument/value");
	else
		context.current_context = CONTEXT_SERVICE_CONFIG;
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
	}
}

void exec_action_context_service_config(int ch)
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
	}
}

void exec_action_context_services(int ch)
{
	ITEM *item;

	switch (ch) {
		case KEY_DOWN:
			menu_driver(my_menu, REQ_DOWN_ITEM);
			break;

		case KEY_UP:
			menu_driver(my_menu, REQ_UP_ITEM);
			break;

		case KEY_ENTER:
		case 10:
			item = current_item(my_menu);
			exec_action(item_userptr(item));
			break;
	}
}

void ncurses_action(void)
{
	int ch = wgetch(win_body);

	if (ch == KEY_F(1)) {
		stop_loop(0);
		return;
	}

	if (ch == 27) {

		if (context.current_context == CONTEXT_HOME)
			__ncurses_print_info_in_footer(false, "^C to quit\n");
		else
			exec_back();

		return;
	}

	if (ch == KEY_F(5)) {
		exec_refresh();
		return;
	}

	switch (context.current_context) {
		case CONTEXT_HOME:
			exec_action_context_home(ch);
			break;

		case CONTEXT_SERVICE_CONFIG:
			exec_action_context_service_config(ch);
			break;

		case CONTEXT_SERVICES:
			exec_action_context_services(ch);
			break;
	}

	wrefresh(win_body);
}

int main(void)
{
	struct json_object *cmd;
	engine_callback = main_callback;

	if (engine_init() < 0)
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

	context.current_context = CONTEXT_HOME;
	context.serv = malloc(sizeof(struct userptr_data));
	assert(context.serv != NULL);
	context.tech = malloc(sizeof(struct userptr_data));
	assert(context.tech != NULL);

	// get_home_page (and render it)
	cmd = json_object_new_object();
	json_object_object_add(cmd, key_command, json_object_new_string("get_home_page"));
	engine_query(cmd);

	loop_run(true);
	loop_terminate();

	delwin(win_header);
	delwin(win_body);
	delwin(win_footer);
	endwin();

	return 0;
}
