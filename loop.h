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

#ifndef __CONNMAN_LOOP_H
#define __CONNMAN_LOOP_H

#define _POSIX_SOURCE

#include <poll.h>
#include <dbus/dbus.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define WATCHEDS_MAX_COUNT 20

#ifdef __cplusplus
extern "C" {
#endif

void loop_init(void);

void loop_run(_Bool poll_stdin);

void loop_quit(void);

void loop_terminate(void);

#ifdef __cplusplus
}
#endif

#endif
