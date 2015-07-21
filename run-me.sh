#!/bin/bash

autoreconf -i -f

./configure --disable-optimization --enable-debug #--enable-asan --disable-silent-rules

make -B
