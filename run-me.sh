#!/bin/bash

autoreconf -i

./configure --disable-optimization --enable-debug

make
