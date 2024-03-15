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
#ifndef GPIO_HW_PWM_H
#define GPIO_HW_PWM_H

// Standard headers

// Local headers
#include "gpio_pwm_if.h"

namespace GPIO
{
    // HW PWM class ==========================================================
    class GpioPwmIfHw : public GpioPwmIf
    {
      public:
        GpioPwmIfHw( int channel, int frequency_hz );
        void start( ) final;
        void stop( ) final;
        void _reconfigure( int frequency_hz, double duty_cycle_percent,
                           bool start = false ) final;
        ~GpioPwmIfHw( );

      public:
        int m_period_ns{ 0 };
        int m_duty_cycle_ns{ 0 };
    };

    void hw_disable_pwm( const ChannelInfo &ch_info );
    void hw_unexport_pwm( const ChannelInfo &ch_info );

} // namespace GPIO

#endif // GPIO_HW_PWM_H
