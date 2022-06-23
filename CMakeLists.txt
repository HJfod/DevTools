cmake_minimum_required(VERSION 3.3.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(DevTools VERSION 1.0.0)

file(GLOB_RECURSE SOURCES
	src/*.cpp
)

add_library(${PROJECT_NAME} SHARED ${SOURCES})
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")

find_path(GEODE_SDK_PATH
	NAMES Geode.cmake
	PATHS $ENV{GEODE_SUITE}/sdk /Users/Shared/Geode/suite/sdk /usr/local/geode/sdk
	DOC "Geode SDK path."
	REQUIRED
)
include(${GEODE_SDK_PATH}/Geode.cmake)

# imgui

add_library(imgui STATIC
    imgui/imgui.cpp
	imgui/imgui_widgets.cpp
	imgui/imgui_tables.cpp
    imgui/imgui_draw.cpp
    imgui/backends/imgui_impl_win32.cpp
    imgui/backends/imgui_impl_opengl3.cpp
)
set_property(TARGET imgui PROPERTY CXX_STANDARD 17)
target_include_directories(imgui PUBLIC
    "imgui"
)
target_link_libraries(
    imgui PRIVATE
    opengl32
)

target_link_libraries(${PROJECT_NAME} 
	imgui
	${CMAKE_SOURCE_DIR}/GL/glew32.lib
)

target_include_directories(${PROJECT_NAME} PUBLIC 
    imgui
    api/include
)

setup_geode_mod()

