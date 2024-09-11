cmake_minimum_required(VERSION 3.9)

project(mipflooding VERSION 0.2 DESCRIPTION "A library for Mip Flooding, an efficient edge padding technique that improves compressibility, first presented by Sean Feeley at GDC 2019")

set(LIBMIPFLOODING_SRC_FILES
    ${PROJECT_SOURCE_DIR}/src/helpers/helper_functions.cpp
    ${PROJECT_SOURCE_DIR}/src/helpers/channel_set.cpp
    ${PROJECT_SOURCE_DIR}/src/subroutines.cpp
    ${PROJECT_SOURCE_DIR}/src/core_functions.cpp
    ${PROJECT_SOURCE_DIR}/src/libmipflooding_extern_c.cpp
)

set(LIBMIPFLOODING_HEADER_FILES
    ${PROJECT_SOURCE_DIR}/src/helpers/channel_set.h
    ${PROJECT_SOURCE_DIR}/src/libmipflooding.h
    ${PROJECT_SOURCE_DIR}/src/libmipflooding_extern_c.h
)

set(LIBMIPFLOODING_PUBLIC_HEADER_FILES
    ${PROJECT_SOURCE_DIR}/src/libmipflooding.h
    ${PROJECT_SOURCE_DIR}/src/libmipflooding_extern_c.h
)

add_library(mipflooding SHARED ${LIBMIPFLOODING_SRC_FILES} ${LIBMIPFLOODING_HEADER_FILES})

if (DEFINED ${CMAKE_BUILD_TYPE} AND ${CMAKE_BUILD_TYPE} STREQUAL Debug)
    set_target_properties(mipflooding PROPERTIES COMPILE_DEFINITIONS "_DEBUG")
    set_target_properties(mipflooding PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Debug")
else()
    set_target_properties(mipflooding PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Release")
endif()

set_target_properties(mipflooding PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    PUBLIC_HEADER "${LIBMIPFLOODING_PUBLIC_HEADER_FILES}"
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "gcc")
    if (CMAKE_CXX_COMPILER_VERSION LESS 13) # gcc 13 adds support for #pragma region 
        target_compile_options(mipflooding PRIVATE -pedantic -Wall -Wno-unknown-pragmas -O3)
    else()
        target_compile_options(mipflooding PRIVATE -pedantic -Wall -O3)
    endif()
endif()
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(mipflooding PRIVATE -pedantic -Wall -O3)
endif()
