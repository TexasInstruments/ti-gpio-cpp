/*
Copyright (c) 2012-2017 Ben Croston ben@croston.org.
Copyright (c) 2019, NVIDIA CORPORATION.
Copyright (c) 2019 Jueon Park(pjueon) bluegbg@gmail.com.
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
#ifndef GPIO_PIN_DATA_H
#define GPIO_PIN_DATA_H

// Standard headers
#include <fstream>
#include <string>
#include <map>
#include <vector>

// Interface headers
#include <GPIO.h>

// Local headers
#include "model.h"

namespace GPIO
{
    struct PinDefinition
    {
        const int LinuxPin;            // Linux GPIO pin number
        const std::string SysfsDir;    // GPIO chip sysfs directory
        const std::string BoardPin;    // Pin number (BOARD mode)
        const std::string BCMPin;      // Pin number (BCM mode)
        const std::string SOCPin;      // Pin name (SOC mode)
        const std::string PWMSysfsDir; // PWM chip sysfs directory
        const int PWMID;               // PWM ID within PWM chip

        std::string PinName(GPIO::NumberingModes key) const
        {
            if (key == GPIO::BOARD)
                return BoardPin;
            else if (key == GPIO::BCM)
                return BCMPin;
            else // SOC
                return SOCPin;
        }
    };

    struct PinInfo
    {
        const int           P1_REVISION;
        const std::string   RAM;
        const std::string   REVISION;
        const std::string   TYPE;
        const std::string   MANUFACTURER;
        const std::string   PROCESSOR;
    };

    struct ChannelInfo
    {
        const std::string   channel;
        const std::string   gpio_chip_dir;
        const int           chip_gpio;
        const int           gpio;
        const std::string   pwm_chip_dir;
        const int           pwm_id;

        std::shared_ptr<std::fstream> f_direction;
        std::shared_ptr<std::fstream> f_value;
        std::shared_ptr<std::fstream> f_duty_cycle;

        ChannelInfo(const std::string  &channel,
                    const std::string  &gpio_chip_dir,
                    int                 chip_gpio,
                    int                 gpio,
                    const std::string  &pwm_chip_dir,
                    int                 pwm_id)
            : channel(channel),
              gpio_chip_dir(gpio_chip_dir),
              chip_gpio(chip_gpio),
              gpio(gpio),
              pwm_chip_dir(pwm_chip_dir),
              pwm_id(pwm_id),
              f_direction(std::make_shared<std::fstream>()),
              f_value(std::make_shared<std::fstream>()),
              f_duty_cycle(std::make_shared<std::fstream>())
        {}
    };

    struct PinData
    {
        Model model;
        PinInfo pin_info;
        std::map<GPIO::NumberingModes, std::map<std::string, ChannelInfo>> channel_data;
    };

    PinData get_data();

} // namespace GPIO

#endif // GPIO_PIN_DATA_H
