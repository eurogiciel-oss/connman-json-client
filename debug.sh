#!/bin/bash

# You want gdb *not* to catch ^C (aka SIGINT) execute this in gdb:
# handle SIGINT pass nostop

pid=$(ps aux |grep connman_json |head -n 1 | awk '{print $2}')
gdb -q -p $pid "$(pwd)/connman_json"
