cmake_minimum_required(VERSION 3.10)
project(INFOMCV_calibration)

set(CMAKE_CXX_STANDARD 20)

set(SOURCE_FILES
  src/Renderer.cpp
  src/Renderer.h
  src/IndexedMesh.cpp
  src/IndexedMesh.h
  main.cpp
)

add_executable(INFOMCV_calibration ${SOURCE_FILES})

find_package(OpenCV REQUIRED imgcodecs calib3d videoio highgui)
find_package(SDL2 CONFIG REQUIRED)
if (TARGET SDL2::SDL2)
    set(SDL2_LIBRARIES SDL2::SDL2)
endif()
target_include_directories(INFOMCV_calibration PRIVATE src/ ${SDL2_INCLUDE_DIRS})

add_subdirectory(thirdparty/glad)

target_link_libraries(INFOMCV_calibration PRIVATE ${SDL2_LIBRARIES} glad ${OpenCV_LIBS})

target_compile_definitions(INFOMCV_calibration PRIVATE SDL_MAIN_HANDLED)
