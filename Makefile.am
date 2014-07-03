AM_MAKEFLAGS = --no-print-directory
AM_CFLAGS = @DBUS_CFLAGS@ -Wall -Werror -std=c99

noinst_PROGRAMS = connman_json

connman_json_SOURCES = dbus_helpers.h dbus_helpers.c \
				  commands.h commands.c \
				  agent.h agent.c \
				  dbus_json.h dbus_json.c \
				  loop.h loop.c \
				  main.c

connman_json_LDADD = @DBUS_LIBS@ -ldl -ljson -lncurses
