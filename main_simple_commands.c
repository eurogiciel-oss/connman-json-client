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
#include <ncurses.h>
#include <ncurses/form.h>
#include <ncurses/menu.h>

#include "engine.h"
#include "loop.h"
#include "ncurses_utils.h"

void __connman_callback_ended(void)
{
	printf("\n[-] __connman_callback_ended\n");
}

void stop_loop(int signum);

void stop_loop(int signum)
{
	printf("\n[*] ^C detected\n");
	loop_quit();
	loop_terminate();
}

void (*engine_callback)(int status, struct json_object *jobj) = NULL;
void main_callback(int status, struct json_object *jobj);

/*
void render_technologies(struct json_object *jobj)
{
	int len, i;
	struct json_object *tech_name, *tech_dict;

	len = json_object_array_length(jobj);

	for (i = 0; i < len; i++) {
		
	}
}

void free_render_technologies()
{
	//TODO
}
*/

void main_callback(int status, struct json_object *jobj)
{
	printf("[*] cb: status %s (%d)\n", strerror(status), status);
	__connman_dbus_json_print_pretty(jobj);
}

void __ncurses_action(int ch) {
	return;
}

int main()
{
	int return_code;
	struct json_object *cmd;

	engine_callback = main_callback;

	if (__engine_init() < 0)
		exit(1);

	signal(SIGINT, stop_loop);
	loop_init();

	// get_state
	/*
	mvprintw(1, 5, "get_state: ");
	refresh();
	//waddstr(my_win, "[*] get_state:\n");
	cmd = json_object_new_object();
	json_object_object_add(cmd, "command", json_object_new_string("get_state"));
	return_code = - __engine_query(cmd);

	// get_services
	wprintw(my_win, "[*] get_services:\n");
	cmd = json_object_new_object();
	json_object_object_add(cmd, "command", json_object_new_string("get_services"));
	return_code = - __engine_query(cmd);
	*/

	// get_technologies
	printf("[*] get_technologies:\n");
	cmd = json_object_new_object();
	json_object_object_add(cmd, "command", json_object_new_string("get_technologies"));
	return_code = - __engine_query(cmd);

	loop_run(true);

	printf("[*] just to trick the compiler : %d\r\n", return_code);
	loop_terminate();

	return 0;
}
