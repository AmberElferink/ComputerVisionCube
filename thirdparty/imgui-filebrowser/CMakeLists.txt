add_library(imgui-filebrowser STATIC ImGuiFileBrowser.cpp ImGuiFileBrowser.h)
target_link_libraries(imgui-filebrowser PRIVATE imgui)
target_include_directories(imgui-filebrowser INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
