cmake_minimum_required(VERSION 3.21)
project(nes)

set(CMAKE_CXX_STANDARD 17)

add_library(nes src/r6502.cpp include/r6502.h src/bus.cpp include/bus.h include/common.h src/cartridge.cpp include/cartridge.h include/mapper.h src/format.cpp src/font.cpp src/ppu.cpp include/ppu.h)
include_directories(nes PUBLIC include)
include_directories(nes PUBLIC /opt/homebrew/include)
target_compile_options(nes PUBLIC -include common.h)

add_library(nes_frontend SHARED src/frontend.cpp src/gfx.cpp include/gfx.h)
target_link_libraries(nes_frontend PUBLIC nes)
target_link_libraries(nes_frontend PUBLIC SDL2)

add_executable(nes_main src/main.cpp)
target_link_libraries(nes_main PUBLIC nes_frontend)
target_compile_options(nes_main PRIVATE -fsanitize=address)
target_link_options(nes_main PRIVATE -fsanitize=address)

enable_testing()
find_package(Catch2 3 REQUIRED)
add_executable(nes_test src/test_r6502.cpp)
target_link_libraries(nes_test PRIVATE nes)
target_link_libraries(nes_test PRIVATE Catch2::Catch2WithMain)

