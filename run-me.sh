#!/bin/bash

autoreconf -i -f

./configure --disable-optimization --enable-debug #--disable-silent-rules

make -B
