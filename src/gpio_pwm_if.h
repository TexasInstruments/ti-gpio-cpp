/*
Copyright (c) 2012-2017 Ben Croston ben@croston.org.
Copyright (c) 2019, NVIDIA CORPORATION.
Copyright (c) 2019 Jueon Park(pjueon) bluegbg@gmail.com.
Copyright (c) 2022, Texas Instruments Incorporated. All rights reserved.

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
#ifndef GPIO_PWM_IF_H
#define GPIO_PWM_IF_H

// Standard headers
#include <string>
#include <thread>
#include <mutex>

// Interface headers
#include <GPIO.h>

// Local headers
#include "gpio_pin_data.h"

namespace GPIO
{
// PWM class ==========================================================
class GpioPwmIf {
    public:
        GpioPwmIf(int channel, int frequency_hz);
        virtual void start() = 0;
        virtual void stop() = 0;
        virtual void _reconfigure(int frequency_hz, double duty_cycle_percent, bool start = false) = 0;
        virtual ~GpioPwmIf(){};

    public:
        ChannelInfo m_ch_info;
        int         m_frequency_hz{0};
        double      m_duty_cycle_percent{0.0};
        bool        m_started{false};
};

} // namespace GPIO

#endif // GPIO_PWM_IF_H
