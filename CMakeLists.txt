cmake_minimum_required(VERSION 3.20)

project(omnia
        VERSION 0.1.0
        LANGUAGES C CXX
        DESCRIPTION "Omnia Game Engine"
)

if (APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "15.0")
endif()

set(OMNIA_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
list(APPEND CMAKE_MODULE_PATH "${OMNIA_SOURCE_DIR}/Meta/CMake")

include(BaseOptions)
include(BaseCompile)
include(BaseTargets)

enable_testing()

add_subdirectory(Common)
add_subdirectory(Libraries)
add_subdirectory(Tests)
add_subdirectory(Tools)
