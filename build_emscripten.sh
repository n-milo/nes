#!/bin/sh

# my emscripten "build system"
emcc src/r6502.cpp src/bus.cpp src/cartridge.cpp src/format.cpp src/main.cpp \
  -std=c++17 \
  -Iinclude/ -I/opt/homebrew/include/ -include common.h \
  -g -O0 -fsanitize=undefined \
  -s USE_SDL=2 \
  -sALLOW_MEMORY_GROWTH -sSTACK_OVERFLOW_CHECK=2 -sSAFE_HEAP=1 \
  -o www/emulator.html
