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

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <dbus/dbus.h>
#include <signal.h>

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
WINDOW *win_header, *win_footer, *win_body;
ITEM **my_items;
MENU *my_menu;
int nb_items = 0;

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
	__renderers_free_home_page();
}

void main_callback(int status, struct json_object *jobj)
{
	struct json_object *data, *cmd_tmp;
	const char *cmd_name;

	assert(status == 0);

	json_object_object_get_ex(jobj, "command_name", &cmd_tmp);
	json_object_object_get_ex(jobj, "data", &data);
	cmd_name = json_object_get_string(cmd_tmp);

	if (strcmp("get_home_page", cmd_name) == 0)
		__renderers_home_page(data);
	else
		assert(1);

	json_object_put(jobj);
}

void __ncurses_action(void) {
	int ch = wgetch(win_body);

	switch (ch) {
		case 'q':
			stop_loop(0);
			break;

		case KEY_UP:
			menu_driver(my_menu, REQ_UP_ITEM);
			break;

		case KEY_DOWN:
			menu_driver(my_menu, REQ_DOWN_ITEM);
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

	win_body = newwin(LINES-4, COLS, 2, 0);
	box(win_body, 0 , 0);
	keypad(win_body, TRUE);

	win_header = newwin(2, COLS, 0, 0);
	win_footer = newwin(2, COLS, LINES-2, 0);

	// Print all windows, according to the man it's more efficient than 3
	// wrefresh()
	wnoutrefresh(win_header);
	wnoutrefresh(win_body);
	wnoutrefresh(win_footer);
	doupdate();

	// get_home_page (and render it)
	__ncurses_print_info_in_footer(false, "asking for 'get_home_page'");
	cmd = json_object_new_object();
	json_object_object_add(cmd, "command", json_object_new_string("get_home_page"));
	__engine_query(cmd);

	loop_run(true);
	loop_terminate();

	delwin(win_header);
	delwin(win_body);
	delwin(win_footer);
	endwin();

	return 0;
}
