#!/bin/sh

# my emscripten "build system"
emcc src/r6502.cpp src/bus.cpp src/cartridge.cpp src/format.cpp src/main.cpp \
  -std=c++17 \
  -Iinclude/ -I/opt/homebrew/include/ -include common.h \
  --preload-file pixel.ttf \
  -g -O0 -fsanitize=undefined \
  -s USE_SDL=2 \
  -sALLOW_MEMORY_GROWTH -sSTACK_OVERFLOW_CHECK=2 -sSAFE_HEAP=1 \
  -o cmake-build-debug/www/index.html
