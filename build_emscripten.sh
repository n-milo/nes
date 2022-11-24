#!/bin/sh

# my emscripten "build system"
emcc src/r6502.cpp src/bus.cpp src/cartridge.cpp src/format.cpp src/main.cpp src/font.cpp src/ppu.cpp src/gfx.cpp \
  -std=c++17 \
  -Iinclude/ -I/opt/homebrew/include/ -include common.h \
  --preload-file monogram-bitmap.json \
  --preload-file roms/ \
  -g -O2 \
  -s USE_SDL=2 \
  -sALLOW_MEMORY_GROWTH \
  -o cmake-build-debug/www/index.html
