include(CMakeParseArguments)
include(GenerateExportHeader)

function(omnia_link_debug target)
    if (NOT target STREQUAL "LibDebug")
        target_link_libraries(${target} PUBLIC LibDebug)
    endif ()
endfunction()

function(omnia_lib target)
    cmake_parse_arguments(OMNIA "" "" "SOURCES;LIBS;THIRD_PARTY;INCLUDE_DIRS" ${ARGN})

    add_library(${target})

    target_sources(${target} PRIVATE ${OMNIA_SOURCES})
    target_link_libraries(${target} PUBLIC Common PRIVATE ${OMNIA_LIBS} ${OMNIA_THIRD_PARTY})
    omnia_link_debug(${target})
    if (OMNIA_LIBS)
        add_dependencies(${target} ${OMNIA_LIBS})
    endif ()

    string(REGEX REPLACE "^Lib" "" short_name ${target})
    string(TOUPPER ${short_name} short_name_upper)

    set(export_dir "${CMAKE_CURRENT_BINARY_DIR}/include/${target}")
    file(MAKE_DIRECTORY "${export_dir}")

    generate_export_header(${target}
            EXPORT_MACRO_NAME ${short_name_upper}_API
            EXPORT_FILE_NAME "${export_dir}/Export.h"
    )

    target_include_directories(${target} PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
            ${OMNIA_INCLUDE_DIRS}
    )
endfunction()

function(omnia_app target)
    cmake_parse_arguments(OMNIA "" "" "SOURCES;LIBS;THIRD_PARTY" ${ARGN})

    add_executable(${target})

    target_sources(${target} PRIVATE ${OMNIA_SOURCES})
    target_link_libraries(${target} PRIVATE Common ${OMNIA_LIBS} ${OMNIA_THIRD_PARTY})
    omnia_link_debug(${target})
    if (OMNIA_LIBS)
        add_dependencies(${target} ${OMNIA_LIBS})
    endif ()
endfunction()

function(omnia_test target)
    cmake_parse_arguments(OMNIA "" "" "SOURCES;LIBS;THIRD_PARTY" ${ARGN})

    string(APPEND target "Test")

    add_executable(${target})

    find_package(GTest CONFIG REQUIRED)
    include(GoogleTest)

    target_sources(${target} PRIVATE ${OMNIA_SOURCES})
    target_link_libraries(${target} PRIVATE Common ${OMNIA_LIBS} ${OMNIA_THIRD_PARTY} GTest::gtest_main)
    omnia_link_debug(${target})
    if (OMNIA_LIBS)
        add_dependencies(${target} ${OMNIA_LIBS})
    endif ()

    gtest_discover_tests(${target})
endfunction()

add_custom_target(
        CopyResources
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/Resources
        ${CMAKE_BINARY_DIR}/Resources
        COMMENT "Copying Resources to output folder"
)