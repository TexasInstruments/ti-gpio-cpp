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

project(ti_gpio)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/common.cmake)

# Get the files that form the core library
set(TI_GPIO_LIB_SRCS
    src/gpio.cpp
    src/gpio_event.cpp
    src/gpio_event.h
    src/gpio_pin_data.cpp
    src/python_functions.cpp)

# Build the library
build_lib(${PROJECT_NAME} TI_GPIO_LIB_SRCS STATIC)

# Build sample applications
set(TI_GPIO_TEST_SRCS samples/ti_gpio_test.cpp)
build_app(ti_gpio_test TI_GPIO_TEST_SRCS)

set(BUTTON_EVT_TEST_SRCS samples/button_event.cpp)
build_app(button_event BUTTON_EVT_TEST_SRCS)

set(BUTTON_INT_TEST_SRCS samples/button_interrupt.cpp)
build_app(button_interrupt BUTTON_INT_TEST_SRCS)

set(SAMPLE_OUT_TEST_SRCS samples/simple_out.cpp)
build_app(simple_out SAMPLE_OUT_TEST_SRCS)

set(SAMPLE_PWM_TEST_SRCS samples/simple_pwm.cpp)
build_app(simple_pwm SAMPLE_PWM_TEST_SRCS)

set(ALL_PINS_INPUT_TEST_SRCS samples/test_all_pins_input.cpp)
build_app(test_all_pins_input ALL_PINS_INPUT_TEST_SRCS)

