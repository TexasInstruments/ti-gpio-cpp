/*
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

#pragma once
#ifndef GPIO_COMMON_H
#define GPIO_COMMON_H

// Standard headers
#include <string>
#include <map>

// Local headers
#include "model.h"
#include "python_functions.h"
#include "gpio_pin_data.h"

// Interface headers
#include <GPIO.h>

namespace GPIO
{
// These are only for implementation
constexpr Directions UNKNOWN = Directions::UNKNOWN;
constexpr Directions HARD_PWM = Directions::HARD_PWM;

//================================================================================
// All global variables are wrapped in a singleton class except for public APIs,
// in order to avoid initialization order problem among global variables in
// different compilation units.

class GlobalVariableWrapper
{
    public:
        // -----Global Variables----
        // NOTE: DON'T change the declaration order of fields.
        // declaration order == initialization order

        PinData _pinData;
        const Model _model;
        const PinInfo _BOARD_INFO;
        const std::map<GPIO::NumberingModes, std::map<std::string, ChannelInfo>> _channel_data_by_mode;

        // A map used as lookup tables for pin to linux gpio mapping
        std::map<std::string, ChannelInfo> _channel_data;

        bool _gpio_warnings;
        NumberingModes _gpio_mode;
        std::map<std::string, Directions> _channel_configuration;
        std::map<std::string, bool> _pwm_channels;

        GlobalVariableWrapper(const GlobalVariableWrapper&) = delete;
        GlobalVariableWrapper& operator=(const GlobalVariableWrapper&) = delete;

        static GlobalVariableWrapper& get_instance();

        static std::string get_model();

        static std::string get_BOARD_INFO();

    private:
        GlobalVariableWrapper();

        void _CheckPermission();
};

void _cleanup_all();
Directions _app_channel_configuration(const ChannelInfo& ch_info);
Directions _sysfs_channel_configuration(const ChannelInfo& ch_info);

} // namespace GPIO

#endif /* GPIO_COMMON_H */
