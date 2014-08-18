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

#include <ncurses.h>
#include <assert.h>

#include "special_win.h"

#define WINDOW_EXIT_MSG "'Esc' to close."

/*
 * This is used for help and error windows.
 */

/*
 * This create, fill and print a new window.
 * @param rows see ncurses newwin()
 * @param cols see ncurses newwin()
 * @param posy see ncurses newwin()
 * @param posx see ncurses newwin()
 * @param msg the message to print, every line should start with a space (or the
 *	box will overwrite the first character
 * @param is_error_msg this is used to print the "title" of the window
 */
static WINDOW* win_new(int rows, int cols, int posy, int posx, const char *msg,
		bool is_error_msg)
{
	WINDOW *win;

	win = newwin(rows, cols, posy, posx);
	assert(win != NULL);
	
	if (is_error_msg)
		mvwprintw(win, 1, 1, "Error message:\n\n%s", msg);
	else
		mvwprintw(win, 1, 1, "Help:\n\n%s", msg);

	box(win, 0, 0);
	mvwprintw(win, rows-2, 1, WINDOW_EXIT_MSG);
	win_refresh(win);

	return win;
}

/*
 * Proxy for win_new(). is_error_msg is set to true.
 */
WINDOW* win_error_new(int rows, int cols, int posy, int posx, const char *msg)
{
	return win_new(rows, cols, posy, posx, msg, true);
}

/*
 * Proxy for win_new(). is_error_msg is set to false.
 */
WINDOW* win_help_new(int rows, int cols, int posy, int posx, const char *msg)
{
	return win_new(rows, cols, posy, posx, msg, false);
}

/*
 * Return if the window exist or not.
 */
bool win_exists(WINDOW *win)
{
	return (win != NULL);
}

/*
 * Force the redraw and refresh of the window if it exists.
 */
void win_refresh(WINDOW *win)
{
	if (win) {
		redrawwin(win);
		wrefresh(win);
	}
}

/*
 * Delete the window if ch is 27 ('Esc'), refresh the window otherwise.
 */
void win_driver(WINDOW **win, int ch)
{
	if (ch == 27) {
		delwin(*win);
		*win = NULL;
	} else
		win_refresh(*win);
}

/*
 * Resize the window to the given height and length.
 */
void win_resize(WINDOW *win, int rows, int cols)
{
	if (win) {
		wresize(win, rows, cols);
		box(win, 0, 0);
		mvwprintw(win, rows-2, 1, WINDOW_EXIT_MSG);
		win_refresh(win);
	}
}
