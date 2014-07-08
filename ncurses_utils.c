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

#include "ncurses_utils.h"

extern WINDOW *win_header, *win_footer, *win_body;

void __ncurses_better_field(FIELD *label, FIELD *field, const char *label_txt,
		const char *field_txt)
{
	char *tmp_label_txt, *tmp_field_txt;

	tmp_label_txt = malloc(sizeof(char) * NCURSES_MAX_FIELD_LEN);

	if (!tmp_label_txt)
		return;

	tmp_field_txt = malloc(sizeof(char) * NCURSES_MAX_FIELD_LEN);

	if (!tmp_field_txt) {
		free(tmp_label_txt);
		return;
	}

	snprintf(tmp_label_txt, NCURSES_MAX_FIELD_LEN, "%s", label_txt);
	tmp_label_txt[NCURSES_MAX_FIELD_LEN-1] = '\0';
	snprintf(tmp_field_txt, NCURSES_MAX_FIELD_LEN, "%s", field_txt);
	tmp_field_txt[NCURSES_MAX_FIELD_LEN-1] = '\0';

	set_field_buffer(label, 0, tmp_label_txt);
	set_field_buffer(field, 0, tmp_field_txt);
}

void __ncurses_better_field_free(FIELD *label, FIELD *field)
{
	free(field_buffer(label, 0));
	free(field_buffer(field, 0));
}

void __ncurses_print_info_in_footer(_Bool is_error, const char* msg)
{
	mvwprintw(win_footer, 0, 1, "[%s] %s",
			(is_error ? "ERROR" : "INFO"), msg);
	wrefresh(win_footer);
}
