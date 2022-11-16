#!/bin/sh

# my emscripten "build system"
emcc src/r6502.cpp src/bus.cpp src/cartridge.cpp src/format.cpp src/main.cpp \
  -std=c++17 \
  -Iinclude/ -I/opt/homebrew/include/ -include common.h \
  -s USE_SDL=2 \
  -o cmake-build-debug/www/index.html