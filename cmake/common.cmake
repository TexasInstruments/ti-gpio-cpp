# Copyright (c) 2021, Texas Instruments Incorporated. All rights reserved.
# 
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.

cmake_minimum_required(VERSION 3.10.2)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

add_compile_options(-std=c++17)

# Get the path of this file
get_filename_component(ROOT_DIR    ${CMAKE_SOURCE_DIR} ABSOLUTE)

IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE Release)
ENDIF()

MESSAGE("Build type: " ${CMAKE_BUILD_TYPE})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

message(STATUS "CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE} PROJECT_NAME = ${PROJECT_NAME}")

SET(CMAKE_FIND_LIBRARY_PREFIXES "" "lib")
SET(CMAKE_FIND_LIBRARY_SUFFIXES ".a" ".lib" ".so")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib/${CMAKE_BUILD_TYPE})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib/${CMAKE_BUILD_TYPE})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE})
set(CMAKE_INSTALL_LIBDIR lib)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX /usr CACHE PATH "Installation Prefix" FORCE)
endif()

# Include directories
include_directories(${PROJECT_SOURCE_DIR} ${ROOT_DIR}/include)

set(COMMON_LINK_LIBS ti_gpio)

set(SYSTEM_LINK_LIBS pthread)

# Function for building a application:
# app_name: app name
# ${ARGN} expands everything after the last named argument to the end
# usage: build_app(app_name a.c b.c....)
function(build_app app_name)
    add_executable(${app_name} ${ARGN})
    target_link_libraries(${app_name}
                          ${COMMON_LINK_LIBS}
                          ${TARGET_LINK_LIBS}
                          ${SYSTEM_LINK_LIBS}
                         )
endfunction()

# Function for building a node:
# lib_name: Name of the library
# lib_type: (STATIC, SHARED)
# lib_ver: Version string of the library
# sub_dir: optional directory under include and lib directories to place the content
# ${ARGN} expands everything after the last named argument to the end
# usage: build_lib(lib_name lib_type lib_ver a.c b.c ....)
function(build_lib lib_name lib_type lib_ver)
    add_library(${lib_name} ${lib_type} ${ARGN})

    set(INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR})

    message("INCLUDE_INSTALL_DIR = " ${INCLUDE_INSTALL_DIR})
    install(TARGETS ${lib_name}
            EXPORT ${lib_name}Targets
            LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}  # Shared Libs
            ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}  # Static Libs
            RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}  # Executables, DLLs
            INCLUDES DESTINATION ${INCLUDE_INSTALL_DIR}
    )

    FILE(GLOB HDRS ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h)

    # Specify the header files to install
    install(FILES ${HDRS} DESTINATION ${INCLUDE_INSTALL_DIR})

    set(CONFIG_PACKAGE_LOCATION ${CMAKE_INSTALL_PREFIX}/cmake/${lib_name})

    # Create config file
    configure_package_config_file(${ROOT_DIR}/cmake/ti_gpio_config.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/${lib_name}Config.cmake
        INSTALL_DESTINATION ${CONFIG_PACKAGE_LOCATION}
        PATH_VARS INCLUDE_INSTALL_DIR
    )

    install(EXPORT ${lib_name}Targets
        FILE ${lib_name}Targets.cmake
        DESTINATION ${CONFIG_PACKAGE_LOCATION}
    )

    export(EXPORT ${lib_name}Targets
        FILE ${CMAKE_CURRENT_BINARY_DIR}/${lib_name}Targets.cmake
    )

    # Generate the version file for the config file
    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/${lib_name}ConfigVersion.cmake
        VERSION "${lib_ver}"
        COMPATIBILITY AnyNewerVersion
    )
    
    # install the package configuration files
    install(FILES 
      ${CMAKE_CURRENT_BINARY_DIR}/${lib_name}Config.cmake
      ${CMAKE_CURRENT_BINARY_DIR}/${lib_name}ConfigVersion.cmake
      DESTINATION ${CONFIG_PACKAGE_LOCATION}
    )

endfunction()

