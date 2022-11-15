cmake_minimum_required(VERSION 3.21)
project(nes)

set(CMAKE_CXX_STANDARD 17)

add_library(nes src/r6502.cpp include/r6502.h src/bus.cpp include/bus.h include/common.h src/cartridge.cpp include/cartridge.h include/mapper.h)
include_directories(nes PUBLIC include)
include_directories(nes PUBLIC /opt/homebrew/include)
target_compile_options(nes PUBLIC -include common.h)

add_executable(nes_frontend src/main.cpp)
target_link_libraries(nes_frontend PRIVATE nes)
target_link_libraries(nes_frontend PRIVATE SDL2)
target_link_libraries(nes_frontend PRIVATE SDL2_ttf)

enable_testing()
find_package(Catch2 3 REQUIRED)
add_executable(nes_test src/test_r6502.cpp)
target_link_libraries(nes_test PRIVATE nes)
target_link_libraries(nes_test PRIVATE Catch2::Catch2WithMain)

