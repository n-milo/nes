cmake_minimum_required(VERSION 3.21)
project(nes)

set(CMAKE_CXX_STANDARD 17)

add_executable(nes src/main.cpp src/r6502.cpp include/r6502.h src/bus.cpp include/bus.h include/common.h src/test_r6502.cpp)
include_directories(nes PUBLIC include)
include_directories(nes PUBLIC /opt/homebrew/include)

enable_testing()

find_package(Catch2 3 REQUIRED)
add_executable(
        nes_test
        src/main.cpp src/r6502.cpp include/r6502.h src/bus.cpp include/bus.h include/common.h src/test_r6502.cpp
)
target_link_libraries(nes_test PRIVATE Catch2::Catch2WithMain)
