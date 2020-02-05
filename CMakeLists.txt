cmake_minimum_required(VERSION 3.15)
project(INFOMCV_calibration)

set(CMAKE_CXX_STANDARD 20)

add_executable(INFOMCV_calibration main.cpp)

find_package(SDL2 REQUIRED)

target_link_libraries(INFOMCV_calibration PRIVATE ${SDL2_LIBRARIES})

target_compile_definitions(INFOMCV_calibration PRIVATE SDL_MAIN_HANDLED)