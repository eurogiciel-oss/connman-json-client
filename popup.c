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

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "popup.h"

static WINDOW *win_body;
static WINDOW *win_form;
static WINDOW *win_menu;
static bool is_on_button;
static MENU *popup_menu;
static ITEM **popup_items;

FORM *popup_form;
FIELD **popup_fields;
struct popup_actions **popup_btn_action;

void popup_new(int rows, int cols, int posy, int posx, char **requests,
		char *title)
{
	int i, cury = 0, curx = 1, tmp, nb_buttons, nb_fields;
	WINDOW *inner;

	win_body = newwin(rows, cols, posy, posx);
	assert(win_body != NULL && popup_btn_action != NULL);
	box(win_body, 0, 0);

	for (nb_buttons = 0; popup_btn_action[nb_buttons]; nb_buttons++);

	popup_items = calloc(nb_buttons+1, sizeof(ITEM *));
	assert(popup_items != NULL);
	assert(popup_btn_action != NULL);

	for (i = 0; popup_btn_action[i]; i++) {
		popup_items[i] = new_item(popup_btn_action[i]->key, "");
		assert(popup_items[i] != NULL);
	}

	popup_items[i] = NULL;
	popup_menu = new_menu(popup_items);
	win_menu = derwin(win_body, 3, cols-2, rows-4, 1);
	assert(popup_menu != NULL && win_menu != NULL);
	box(win_menu, 0, 0);
	set_menu_win(popup_menu, win_menu);
	set_menu_format(popup_menu, 1, nb_buttons);
	tmp = popup_menu->fcols * (popup_menu->namelen + popup_menu->spc_rows);
	tmp--;
	inner = derwin(win_menu, 1, tmp, 1, (cols-3-tmp)/2);
	assert(inner != NULL);
	set_menu_sub(popup_menu, inner);
	set_menu_mark(popup_menu, "");
	assert(post_menu(popup_menu) == E_OK);

	mvwprintw(win_body, 1, 2, "%s", title);

	for (nb_fields = 0; requests && requests[nb_fields]; nb_fields++);

	if (nb_fields == 0) {
		popup_fields = NULL;
		popup_form = NULL;
		is_on_button = true;
		return;
	}

	popup_fields = calloc(nb_fields+1, sizeof(FIELD *));
	assert(popup_fields != NULL);

	for (i = 0; i < nb_fields && requests[i]; i++) {

		if (i % 2 == 1) {
			popup_fields[i] = new_field(1, 41, cury, curx, 0, 0);
			assert(popup_fields[i] != NULL);
			set_field_buffer(popup_fields[i], 0, requests[i]);
			cury = cury+1;
			curx = 1;
			field_opts_on(popup_fields[i], O_ACTIVE);
			field_opts_on(popup_fields[i], O_EDIT);
			field_opts_off(popup_fields[i], O_STATIC);
			set_field_back(popup_fields[i], A_UNDERLINE); 
			/*
			 * Only allow input characters that can be displayed.
			 * More advanced verification is possible but if the
			 * user type something wrong, he won't be allowed to
			 * quit the field until he corrected his mistake. Of
			 * course no information whatsoever point towards the
			 * mistake.
			 */
			set_field_type(popup_fields[i], TYPE_REGEXP, "^([[:print:]]*)$");
		} else {
			popup_fields[i] = new_field(1, 45, cury, curx, 0, 0);
			assert(popup_fields[i] != NULL);
			set_field_buffer(popup_fields[i], 0, requests[i]);
			curx = strlen(requests[i]) + 2;
			field_opts_off(popup_fields[i], O_ACTIVE);
			field_opts_off(popup_fields[i], O_EDIT);
		}
	}

	popup_fields[i] = NULL;
	popup_form = new_form(popup_fields);
	assert(popup_form != NULL);
	win_form = derwin(win_body, rows-6, cols-2, 2, 1);
	box(win_form, 0, 0);
	assert(popup_form != NULL && win_form != NULL);
	set_form_win(popup_form, win_form);
	inner = derwin(win_form, popup_form->rows+1, popup_form->cols+1, 1, 1);
	assert(inner != NULL);
	set_form_sub(popup_form, inner);

	assert(post_form(popup_form) == E_OK);
	is_on_button = false;
	set_menu_fore(popup_menu, A_NORMAL); // "hide" the button
	pos_form_cursor(popup_form);
}

void popup_delete(void)
{
	int i;

	if (popup_form)
		unpost_form(popup_form);

	unpost_menu(popup_menu);

	if (popup_form) {
		for (i = 0; popup_fields[i] != NULL; i++)
			free_field(popup_fields[i]);

		free_form(popup_form);
	}

	for (i = 0; popup_items[i] != NULL; i++)
		free_item(popup_items[i]);

	free_menu(popup_menu);
	delwin(win_form);
	delwin(win_menu);
	delwin(win_body);
	win_body = NULL;
	popup_btn_action = NULL;
}

static void driver_buttons(ITEM *item)
{
	const char *name = item_name(item);
	int i = 0;

	while (popup_btn_action[i] && strcmp(popup_btn_action[i]->key, name) != 0)
		i++;

	if (popup_btn_action[i])
		popup_btn_action[i]->func();
}

static void switch_to_buttons(void)
{
	// Those 2 lines allow the field buffer to be set
	form_driver(popup_form, REQ_PREV_FIELD);
	form_driver(popup_form, REQ_NEXT_FIELD);

	menu_driver(popup_menu, REQ_FIRST_ITEM);
	is_on_button = true;
	set_menu_fore(popup_menu, A_REVERSE); // "show" the button
}

void popup_driver(int ch)
{
	switch (ch) {
		case KEY_DOWN:
			if (is_on_button || !popup_form)
				break;

			if (popup_form->current == popup_fields[popup_form->maxfield-1])
				switch_to_buttons();
			else
				form_driver(popup_form, REQ_NEXT_FIELD);
			break;

		case KEY_UP:
			if (is_on_button) {
				if (!popup_form)
					break;

				is_on_button = false;
				set_menu_fore(popup_menu, A_NORMAL); // "hide" the button
			} else
				form_driver(popup_form, REQ_PREV_FIELD);
			break;

		case KEY_LEFT:
			if (is_on_button)
				menu_driver(popup_menu, REQ_LEFT_ITEM);
			else
				form_driver(popup_form, REQ_PREV_CHAR);
			break;

		case KEY_RIGHT:
			if (is_on_button)
				menu_driver(popup_menu, REQ_RIGHT_ITEM);
			else
				form_driver(popup_form, REQ_NEXT_CHAR);
			break;

		case 10:
			if (is_on_button)
				driver_buttons(current_item(popup_menu));
			else
				switch_to_buttons();

			break;

		// Delete the char before cursor
		case KEY_BACKSPACE:
		case 127:
			if (!is_on_button)
				form_driver(popup_form, REQ_DEL_PREV);
			break;

		// Delete the char under the cursor
		case KEY_DC:
			if (!is_on_button)
				form_driver(popup_form, REQ_DEL_CHAR);
			break;

		default:
			if (!is_on_button)
				form_driver(popup_form, ch);

			break;

	}

	if (is_on_button)
		pos_menu_cursor(popup_menu);
	else
		pos_form_cursor(popup_form);

	wrefresh(win_body);
}

void popup_refresh(void)
{
	box(win_body, 0, 0);
	wrefresh(win_body);
}

bool popup_exists(void)
{
	return (win_body != NULL);
}
