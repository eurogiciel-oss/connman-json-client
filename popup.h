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

#ifndef __CONNMAN_POPUP_H
#define __CONNMAN_POPUP_H

#include <form.h>
#include <menu.h>

#ifdef __cplusplus
extern "C" {
#endif

struct popup_actions {
	char *key; // Button name
	void (*func)(); // Execute on 'return'
};

void popup_new(int rows, int cols, int posy, int posx, char **requests,
		char *title);

void popup_delete(void);

void popup_refresh(void);

bool popup_exists(void);

void popup_driver(int ch);

#ifdef __cplusplus
}
#endif

#endif
