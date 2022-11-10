cmake_minimum_required(VERSION 3.21)
project(nes)

set(CMAKE_CXX_STANDARD 17)

include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(nes src/main.cpp src/r6502.cpp include/r6502.h src/bus.cpp include/bus.h include/common.h src/test_r6502.cpp)
include_directories(nes PUBLIC include)
include_directories(nes PUBLIC /opt/homebrew/include)

enable_testing()

add_executable(
        nes_test
        src/main.cpp src/r6502.cpp include/r6502.h src/bus.cpp include/bus.h include/common.h src/test_r6502.cpp
)
target_link_libraries(
        nes_test
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(nes_test)