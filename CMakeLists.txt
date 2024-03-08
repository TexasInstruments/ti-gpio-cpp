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

project(ti_gpio)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/common.cmake)

# Build the library
build_lib(${PROJECT_NAME} STATIC 2.0.0
          src/gpio.cpp
        #   src/gpio_event.cpp
        #   src/gpio_event.h
          src/gpio_pin_data.cpp
          src/gpio_common.cpp
          src/gpio_sw_pwm.cpp
          src/gpio_hw_pwm.cpp
          src/python_functions.cpp)

# Build sample applications
build_app(ti_gpio_test samples/ti_gpio_test.cpp)

build_app(button_event samples/button_event.cpp)

build_app(button_interrupt samples/button_interrupt.cpp)

build_app(simple_out samples/simple_out.cpp)

build_app(simple_pwm samples/simple_pwm.cpp)

build_app(test_all_pins_input samples/test_all_pins_input.cpp)

build_app(test_all_pins_pwm samples/test_all_pins_pwm.cpp)
