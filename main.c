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

#include "dbus_helpers.h"
#include "dbus_json.h"
#include "commands.h"
#include "agent.h"
#include "loop.h"

static DBusConnection *connection;
//static WINDOW *my_win;

void (*commands_callback)(struct json_object *data, json_bool is_error) = NULL;
void (*commands_signal)(struct json_object *data) = NULL;
void (*agent_callback)(struct json_object *data, struct agent_data *request) =
		NULL;
void (*agent_error_callback)(struct json_object *data,
		struct agent_data *request) = NULL;


void __connman_callback_ended(void)
{
	//wprintw(my_win, "\r[-] __connman_callback_ended\n");
	//wrefresh(my_win);
	printf("\n[-] __connman_callback_ended\n");
}

void stop_loop(int signum);

void stop_loop(int signum)
{
	//wprintw(my_win, "\n[*] ^C detected\n");
	//wrefresh(my_win);
	printf("\n[*] ^C detected\n");
	loop_quit();
}

void main_commands_cb(struct json_object *data, json_bool is_error);
void main_commands_sig(struct json_object *data);
void main_agent_cb(struct json_object *data, struct agent_data *request);
void main_agent_error_cb(struct json_object *data, struct agent_data *request);

void main_commands_cb(struct json_object *data, json_bool is_error)
{
	if (!data)
		return;

	if (is_error)
		//wprintw(my_win, "\n\r[ERROR]:");
		printf("\n[ERROR]:");
	else
		//wprintw(my_win, "\n\r[INFO]:");
		printf("\n[INFO]:");

	///wrefresh(my_win);
	//__connman_dbus_json_print_pretty_curses(my_win, data);
	__connman_dbus_json_print_pretty(data);
	json_object_put(data);
	//wrefresh(my_win);
}

void main_commands_sig(struct json_object *data)
{
	//wprintw(my_win, "\n[SIGNAL]:");
	//wrefresh(my_win);
	//__connman_dbus_json_print_pretty_curses(my_win, data);
	printf("\n[SIGNAL]:");
	__connman_dbus_json_print_pretty(data);
	json_object_put(data);
	//wrefresh(my_win);
}

void main_agent_cb(struct json_object *data, struct agent_data *request)
{
	struct json_object *passphrase = json_object_new_object();
	struct json_object *tmp;
	//wprintw(my_win, "\n\r[AGENT]:");
	//wrefresh(my_win);
	//__connman_dbus_json_print_pretty_curses(my_win, data);
	json_object_object_get_ex(data, DBUS_JSON_AGENT_MSG_KEY, &tmp);

	if (tmp) {
			json_object_object_add(passphrase, "Passphrase",
				json_object_new_string("PASSWORD"));
			__connman_json_to_agent_response(passphrase, request);
			printf("\n\r[AGENT]: passphrase sent\n");
	}

	printf("\n[AGENT]:");
	__connman_dbus_json_print_pretty(data);
	json_object_put(data);
	//wprintw(my_win, "\n\r[-] agent_callback is not fully implemented yet\n");
	//wrefresh(my_win);

	//wprintw(my_win, "\n\r[AGENT]: passphrase sent\n");
}

void main_agent_error_cb(struct json_object *data, struct agent_data *request)
{
	//wprintw(my_win, "\n\r[AGENT ERROR]:");
	//wrefresh(my_win);
	//__connman_dbus_json_print_pretty_curses(my_win, data);
	printf("\n[AGENT ERROR]:");
	__connman_dbus_json_print_pretty(data);
	json_object_put(data);
	//wprintw(my_win, "\n\r[-] agent_error_callback is not fully implemented yet\n");
	//wrefresh(my_win);
	printf("\n[-] agent_error_callback is not fully implemented yet\n");
}

int main(int argc, char *argv[])
{
	//int startx, starty, width, height;
	int return_code;
	struct json_object *cmd = json_object_new_object();
	DBusError dbus_err;

	commands_callback = main_commands_cb;
	commands_signal = main_commands_sig;
	agent_callback = main_agent_cb;
	agent_error_callback = main_agent_error_cb;

	/*
	 * ncurses stuff
	initscr();
	cbreak();
	keypad(stdscr, TRUE);
	height = 30;
	width = 100;
	starty = startx = 0;
	refresh();
	my_win = newwin(height, width, starty, startx);
	box(my_win, 0 , 0);
	wrefresh(my_win);
	 */

	/*
	 * Getting dbus connection
	 */
	dbus_error_init(&dbus_err);
	connection = dbus_bus_get(DBUS_BUS_SYSTEM, &dbus_err);

	if (dbus_error_is_set(&dbus_err)) {
		//wprintw(my_win, "\n[-] Error getting connection: %s\n", dbus_err.message);
		//wrefresh(my_win);
		printf("\n[-] Error getting connection: %s\n", dbus_err.message);
		dbus_error_free(&dbus_err);
		return 1;
	}

	json_object_object_add(cmd, "command",
			//json_object_new_string("technologies"));
			//json_object_new_string("services"));
			json_object_new_string("state"));
			//json_object_new_string("disconnect"));
			//json_object_new_string("connect"));
			//json_object_new_string("enable"));
			//json_object_new_string("disable"));
			//json_object_new_string("monitor"));
			//json_object_new_string("agent_register"));
			//json_object_new_string("agent_unregister"));
			//json_object_new_string("config"));

	// loop initialization
	loop_init(connection);

	// react to ^C
	signal(SIGINT, stop_loop);

	return_code = - __connman_command_dispatcher(connection, cmd);

	//wprintw(my_win, "\n\r[*] Return (1) code is %d: %s\n", return_code,
	//		strerror(return_code));
	//wrefresh(my_win);
	printf("\n[*] Return (1) code is %d: %s\n", return_code,
			strerror(return_code));

	cmd = json_object_new_object();
	json_object_object_add(cmd, "command",
			json_object_new_string("connect"));

	json_object_object_add(cmd, "data",
			json_object_new_string("wifi_XXXXXXXXXXXX_YYYYYYYYYYYYYYYYYY_managed_psk"));

	return_code = - __connman_command_dispatcher(connection, cmd);

	//wprintw(my_win, "\n\r[*] Return (2) code is %d: %s\n", return_code,
	//		strerror(return_code));
	//wrefresh(my_win);
	printf("\n[*] Return (2) code is %d: %s\n", return_code,
			strerror(return_code));

	//loop_run(true);
	loop_run(false);

	cmd = json_object_new_object();
	json_object_object_add(cmd, "command",
		json_object_new_string("agent_unregister"));

	return_code = - __connman_command_dispatcher(connection, cmd);

	printf("\n[*] Return (3) code is %d: %s\n", return_code,
			strerror(return_code));

	// terminate the loop
	loop_terminate();

	/*
	delwin(my_win);
	endwin();
	*/

	return 0;
}
