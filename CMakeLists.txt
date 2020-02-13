cmake_minimum_required(VERSION 3.10)
project(INFOMCV_calibration)

set(CMAKE_CXX_STANDARD 20)

#list(APPEND CMAKE_MODULE_PATH cmake/)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(SOURCE_FILES
  src/Calibration.cpp
  src/Calibration.h
  src/Renderer.cpp
  src/Renderer.h
  src/IndexedMesh.cpp
  src/IndexedMesh.h
  src/Pipeline.cpp
  src/Pipeline.h
  src/RenderPass.cpp
  src/RenderPass.h
  src/Texture.cpp
  src/Texture.h
  src/Ui.cpp
  src/Ui.h
  main.cpp
)

add_executable(INFOMCV_calibration ${SOURCE_FILES})

find_package(OpenCV REQUIRED calib3d videoio)
find_package(SDL2 CONFIG)
if (WIN32 AND MINGW)
    set(SDL2_INCLUDE_DIRS /mingw64/include/SDL2)
elseif (TARGET SDL2::SDL2)
    set(SDL2_LIBRARIES SDL2::SDL2)
endif()

target_include_directories(INFOMCV_calibration PRIVATE src/ ${SDL2_INCLUDE_DIRS})

add_subdirectory(thirdparty/glad)
add_subdirectory(thirdparty/imgui-1.75)
add_subdirectory(thirdparty/imgui-filebrowser)

target_link_libraries(INFOMCV_calibration
  PRIVATE
    ${SDL2_LIBRARIES}
    glad
    imgui
    imgui-filebrowser
    ${OpenCV_LIBS}
)

target_compile_definitions(INFOMCV_calibration PRIVATE SDL_MAIN_HANDLED)
