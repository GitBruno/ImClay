cmake_minimum_required(VERSION 3.20)
project(ImClay LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG docking
)
FetchContent_MakeAvailable(imgui)

add_library(imgui_core STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
)
target_include_directories(imgui_core PUBLIC
    ${imgui_SOURCE_DIR}
)
target_compile_definitions(imgui_core PUBLIC IMGUI_DEFINE_MATH_OPERATORS)

add_library(ImClay STATIC src/ImClay.cpp)
target_include_directories(ImClay PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/clay
)
target_link_libraries(ImClay PUBLIC imgui_core)
