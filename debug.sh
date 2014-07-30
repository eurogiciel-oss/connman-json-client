#!/bin/bash

pid=$(ps aux |grep connman_json |head -n 1 | awk '{print $2}')
gdb -q -p $pid "$(pwd)/connman_json"
