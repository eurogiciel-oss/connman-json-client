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

#ifndef __CONNMAN_RENDERERS_H
#define __CONNMAN_RENDERERS_H

#include <ncurses.h>
#include <ncurses/form.h>
#include <ncurses/menu.h>

#define RENDERERS_STRING_MAX_LEN 100

#ifdef __cplusplus
extern "C" {
#endif

extern WINDOW *win_header, *win_footer, *win_body, *inner;
extern ITEM **my_items;
extern MENU *my_menu;
extern FIELD **field;
extern FORM *my_form;
extern int win_body_lines;
extern int nb_pages;

struct userptr_data {
	char *dbus_name;	// e.g. /net/connman/wifi_XXXXXX_YYYY_none
	char *pretty_name;	// e.g. MySuperWifiESSID
};

// Different contexts we can be in
typedef enum {CONTEXT_HOME, CONTEXT_SERVICE_CONFIG, CONTEXT_SERVICES} context_t;

// This keep track of the execution context, the cursor position and current
// service and technology user pointers.
struct context_info {
	context_t current_context;
	struct userptr_data *serv;
	struct userptr_data *tech;
	char *cursor_id;		// tag-like, see main.c repos_cursor()
};

void __renderers_home_page(struct json_object *jobj);

void __renderers_free_home_page(void);

void __renderers_services_config_paging(void);

void __renderers_services(struct json_object *jobj);

void __renderers_free_service_config(void);

void __renderers_free_services(void);

#ifdef __cplusplus
}
#endif

#endif
