AM_MAKEFLAGS = --no-print-directory
AM_CFLAGS = @DBUS_CFLAGS@ -Wall -Werror -pedantic

noinst_PROGRAMS = connman_json

connman_json_SOURCES = dbus_helpers.h dbus_helpers.c \
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
				  main.c


connman_json_LDADD = @DBUS_LIBS@ -ldl -ljson -lform -lmenu -lncurses
connman_json_LDFLAGS = -Wl,--warn-common
