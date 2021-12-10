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

cmake_minimum_required(VERSION 3.16.0)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

add_compile_options(-std=c++17)

# Get the path of this file
get_filename_component(SELF_DIR    ${CMAKE_CURRENT_LIST_FILE} PATH)
get_filename_component(ROOT_DIR    ${SELF_DIR}/.. ABSOLUTE)
get_filename_component(INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/.. ABSOLUTE)

IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE Release)
ENDIF()

MESSAGE("Build type: " ${CMAKE_BUILD_TYPE})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

message(STATUS "CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE} PROJECT_NAME = ${PROJECT_NAME}")

SET(CMAKE_FIND_LIBRARY_PREFIXES "" "lib")
SET(CMAKE_FIND_LIBRARY_SUFFIXES ".a" ".lib" ".so")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib/${CMAKE_BUILD_TYPE})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE})

# Include directories
include_directories(${PROJECT_SOURCE_DIR}
                    ${ROOT_DIR}/include)

set(COMMON_LINK_LIBS
    ti_gpio
    )

set(SYSTEM_LINK_LIBS
    pthread
    )

# Function for building an application
# ARG0: app name
# ARG1: source list
function(build_app)
    set(app ${ARGV0})
    set(src ${ARGV1})
    add_executable(${app} ${${src}})
    target_link_libraries(${app}
                          -Wl,--start-group
                          ${COMMON_LINK_LIBS}
                          ${SYSTEM_LINK_LIBS}
                          -Wl,--end-group)
endfunction(build_app)

# Function for building a node:
# ARG0: lib name
# ARG1: source list
# ARG2: type (STATIC, SHARED)
function(build_lib)
    set(lib ${ARGV0})
    set(src ${ARGV1})
    set(type ${ARGV2})
    set(version 0.1.0)

    add_library(${lib} ${type} ${${src}})

    set(INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR}/${CMAKE_INSTALL_INCLUDEDIR})

    message("INCLUDE_INSTALL_DIR = " ${INCLUDE_INSTALL_DIR})
    install(TARGETS ${lib}
            EXPORT ${lib}Targets
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}  # Shared Libs
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}  # Static Libs
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}  # Executables, DLLs
            INCLUDES DESTINATION ${CMAKE_INSTALL_LIBDIR}/${CMAKE_INSTALL_INCLUDEDIR}
    )

    FILE(GLOB HDRS
        include/*.h
    )

    # Specify the header files to install
    install(FILES
        ${HDRS}
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/${CMAKE_INSTALL_INCLUDEDIR}
    )

    set(CONFIG_PACKAGE_LOCATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})

    install(EXPORT ${PROJECT_NAME}Targets
        FILE ${PROJECT_NAME}Targets.cmake
        DESTINATION ${CONFIG_PACKAGE_LOCATION}
    )

    export(EXPORT ${PROJECT_NAME}Targets
        FILE ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Targets.cmake
    )

    # Generate the version file for the config file
    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
        VERSION "${version}"
        COMPATIBILITY AnyNewerVersion
    )
    
    # Create config file
    configure_package_config_file(${ROOT_DIR}/cmake/ti_gpio_config.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/${lib}Config.cmake
        INSTALL_DESTINATION ${CONFIG_PACKAGE_LOCATION}
        PATH_VARS INCLUDE_INSTALL_DIR
    )

    # install the package configuration files
    install(FILES 
      ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
      ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
      DESTINATION ${CONFIG_PACKAGE_LOCATION}
    )

endfunction(build_lib)

