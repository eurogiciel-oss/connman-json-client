#!/bin/bash

FLAGS="-ljson -Wall -std=c99 -g -O0"
CC="gcc"

# test_json_utils
#$CC $FLAGS -o test_json_utils test_json_utils.c json_utils.o keys.o

# main_simple_commands
#$CC $FLAGS -o main_simple_commands main_simple_commands.c -I/usr/include/dbus-1.0/ -I/usr/lib64/dbus-1.0/include -ldbus-1 -ljson -lncurses loop.o engine.o commands.o dbus_helpers.o json_utils.o dbus_json.o agent.o keys.o

# test_regex
$CC $FLAGS -o test_regexp test_regexp.c json_utils.o keys.o
