#!/bin/bash

autoreconf -i

./configure --disable-optimization --enable-debug #--disable-silent-rules

make -B
