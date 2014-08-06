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
#include <stdarg.h>
#include <ctype.h>
#include <string.h>
#include <ncurses/form.h>
#include <ncurses/menu.h>

#include "ncurses_utils.h"

/*
 * This file aim to provide simple functions to print informations in win_footer
 * and keep footer messages.
 */

extern WINDOW *win_footer;

static void print_in_footer(bool is_error, int line, const char *msg,
		va_list args)
{
	mvwprintw(win_footer, line, 0, (is_error ? " [ERROR] " : " [INFO] "));
	vwprintw(win_footer, msg, args);
	wprintw(win_footer, "\n");
	wrefresh(win_footer);
}

void print_info_in_footer(bool is_error, const char* msg, ...)
{
	va_list args;

	va_start(args, msg);
	print_in_footer(is_error, 0, msg, args);
	va_end(args);
}

void print_info_in_footer2(bool is_error, const char* msg, ...)
{
	va_list args;

	va_start(args, msg);
	print_in_footer(is_error, 1, msg, args);
	va_end(args);
}

void refresh_home_msg(void)
{
	print_info_in_footer(false, "'d' to disconnect, 'Return' for"
			" details, 'F5' to force refresh");
	print_info_in_footer2(false, "^C to quit");
}

void refresh_services_msg(void)
{
	print_info_in_footer(false, "'F5' to refresh network list, "
			"'F6' to force a scan");
	print_info_in_footer2(false, "'Esc' to get back");
}

void refresh_service_config_msg(void)
{
	print_info_in_footer(false, "'F5' to force refresh, "
			"'F7' to submit settings");
	print_info_in_footer2(false, "'Esc' to get back");
}
