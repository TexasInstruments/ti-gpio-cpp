/*
Copyright (c) 2012-2017 Ben Croston ben@croston.org.
Copyright (c) 2019, NVIDIA CORPORATION.
Copyright (c) 2019 Jueon Park(pjueon) bluegbg@gmail.com.
Copyright (c) 2021 Adam Rasburn blackforestcheesecake@protonmail.ch
Copyright (c) 2021, Texas Instruments Incorporated. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/

// Standard headers
#include <dirent.h>
#include <unistd.h>
#include <iostream>
#include <sstream>

// Interface headers
#include <GPIO.h>

// Local headers
#include "gpio_common.h"

using namespace GPIO;
using namespace std;

// The user CAN'T use GPIO::UNKNOW, GPIO::HARD_PWM
// These are only for implementation
constexpr Directions UNKNOWN = Directions::UNKNOWN;
constexpr Directions HARD_PWM = Directions::HARD_PWM;

//================================================================================
// All global variables are wrapped in a singleton class except for public APIs,
// in order to avoid initialization order problem among global variables in
// different compilation units.

GlobalVariableWrapper& GlobalVariableWrapper::get_instance()
{
    static GlobalVariableWrapper singleton{};
    return singleton;
}

string GlobalVariableWrapper::get_model()
{
    auto& instance = get_instance();
    auto ret = ModelToString(instance._model);
    if (is_None(ret))
        throw runtime_error("get_model error");

    return ret;
}

string GlobalVariableWrapper::get_BOARD_INFO()
{
    auto& instance = GlobalVariableWrapper::get_instance();

    stringstream ss{};
    ss << "[BOARD_INFO]\n";
    ss << "P1_REVISION: " << instance._BOARD_INFO.P1_REVISION << endl;
    ss << "RAM: " << instance._BOARD_INFO.RAM << endl;
    ss << "REVISION: " << instance._BOARD_INFO.REVISION << endl;
    ss << "TYPE: " << instance._BOARD_INFO.TYPE << endl;
    ss << "MANUFACTURER: " << instance._BOARD_INFO.MANUFACTURER << endl;
    ss << "PROCESSOR: " << instance._BOARD_INFO.PROCESSOR << endl;
    return ss.str();
}
GlobalVariableWrapper::GlobalVariableWrapper()
    : _pinData(get_data()), // Get GPIO pin data
      _model(_pinData.model),
      _BOARD_INFO(_pinData.pin_info),
      _channel_data_by_mode(_pinData.channel_data),
      _gpio_warnings(true),
      _gpio_mode(NumberingModes::None)
{
    _CheckPermission();
}

void GlobalVariableWrapper::_CheckPermission()
{
    string path1 = _SYSFS_ROOT + "/export"s;
    string path2 = _SYSFS_ROOT + "/unexport"s;
    if (!os_access(path1, W_OK) || !os_access(path2, W_OK)) {
        cerr << "[ERROR] The current user does not have permissions set to access the library functionalites. "
                "Please configure permissions or use the root user to run this."
             << endl;
        throw runtime_error("Permission Denied.");
    }
}

