AM_MAKEFLAGS = --no-print-directory
AM_CFLAGS = @DBUS_CFLAGS@ @JSON_CFLAGS@ -Wall -Werror

noinst_PROGRAMS = connman_ncurses

connman_ncurses_SOURCES = dbus_helpers.h dbus_helpers.c \
				  commands.h commands.c \
				  agent.h agent.c \
				  dbus_json.h dbus_json.c \
				  loop.h loop.c \
				  json_utils.h json_utils.c \
				  engine.h engine.c \
				  ncurses_utils.h ncurses_utils.c \
				  renderers.h renderers.c \
				  keys.h keys.c \
				  popup.h popup.c \
				  json_regex.h json_regex.c \
				  string_utils.h string_utils.c \
				  special_win.h special_win.c \
				  main.c


connman_ncurses_LDADD = @DBUS_LIBS@ @JSON_LIBS@ -ldl -lform -lmenu -lncurses
connman_ncurses_LDFLAGS = -Wl,--warn-common
