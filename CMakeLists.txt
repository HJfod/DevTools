cmake_minimum_required(VERSION 3.0)

project(DevTools VERSION 1.0.0 LANGUAGES C CXX)

set(GEODE_BUILD_CODEGEN On)
set(GEODE_NO_PRECOMPILED_HEADERS On)

add_subdirectory(imgui)

file(GLOB CORE_SOURCES
    src/*.cpp
)

set(SOURCE_FILES 
	${CORE_SOURCES}
)
set(INCLUDE_DIRECTORIES
	"imgui/imgui"
)
set(LINK_DIRECTORIES)
set(LINK_LIBRARIES imgui-hook ${CMAKE_SOURCE_DIR}/GL/glew32.lib)

include(sdk/Geode.cmake)
set_target_properties(DevTools PROPERTIES PREFIX "" OUTPUT_NAME "DevTools")


