#!/bin/bash

g++ linux-alsa-output.cpp -o linux-x64-alsa-output.node -I../include/napi -shared -fPIC -O0 -lasound -lm -std=c++17
