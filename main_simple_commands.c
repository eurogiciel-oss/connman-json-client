/*
 *  connman-json-client
 *
 *  This file is meant for testing.
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
#include <ncurses.h>
#include <ncurses/form.h>
#include <ncurses/menu.h>

#include "engine.h"
#include "loop.h"

void stop_loop(int signum);
void (*engine_callback)(int status, struct json_object *jobj) = NULL;
void main_callback(int status, struct json_object *jobj);

void callback_ended(void)
{
	printf("\n[-] callback_ended\n");
}

void stop_loop(int signum)
{
	printf("\n[*] ^C detected\n");
	loop_quit();
}

void main_callback(int status, struct json_object *jobj)
{
	printf("[*] cb: status %s (%d)\n", strerror(status), status);
	dbus_json_print_pretty(jobj);
}

void __ncurses_action(int ch) {
	return;
}

int main()
{
	struct json_object *cmd;

	engine_callback = main_callback;

	if (__engine_init() < 0)
		exit(1);

	signal(SIGINT, stop_loop);
	loop_init();

	// get_state
	/*
	printf("[*] get_state:\n");
	cmd = json_object_new_object();
	json_object_object_add(cmd, "command", json_object_new_string("get_state"));
	__engine_query(cmd);
	*/

	// get_services
	printf("[*] get_services:\n");
	cmd = json_object_new_object();
	json_object_object_add(cmd, "command", json_object_new_string("get_services"));
	__engine_query(cmd);

	/*
	// get_technologies
	printf("[*] get_technologies:\n");
	cmd = json_object_new_object();
	json_object_object_add(cmd, "command", json_object_new_string("get_technologies"));
	__engine_query(cmd);
	*/

	loop_run(false);
	loop_terminate();

	return 0;
}
