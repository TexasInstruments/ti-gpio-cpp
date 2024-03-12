/*
Copyright (c) 2012-2017 Ben Croston ben@croston.org.
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

// Standard headers
#include <chrono>
#include <stdexcept>

// Interface headers
#include <GPIO.h>

// Local headers
#include "gpio_sw_pwm.h"

using namespace std;

namespace GPIO
{
    GpioPwmIfSw::GpioPwmIfSw( int channel, int frequency_hz )
        : GpioPwmIf( channel, frequency_hz )
    {
        if( frequency_hz <= 0.0 )
        {
            throw runtime_error( "Invalid frequency" );
        }

        m_frequency_hz = frequency_hz;
        m_basetime     = 1000.0 / m_frequency_hz; // ms
        m_slicetime    = m_basetime / 100.0;
        calculate_times( );
    }

    void GpioPwmIfSw::calculate_times( )
    {
        // Time units in micro-seconds
        m_on_time =
            static_cast<long>( ( m_duty_cycle_percent * m_slicetime ) * 1000 );
        m_off_time = static_cast<long>(
            ( ( 100.0 - m_duty_cycle_percent ) * m_slicetime ) * 1000 );
    }

    void GpioPwmIfSw::start( )
    {
        unique_lock<mutex> lock( m_lock );

        if( m_started )
        {
            return;
        }

        m_thread = thread( [this] { pwm_thread( ); } );
    }

    void GpioPwmIfSw::stop( )
    {
        unique_lock<mutex> lock( m_lock );

        if( m_started )
        {
            m_stop_thread = true;
            m_thread.join( );
        }
    }

    void GpioPwmIfSw::pwm_thread( )
    {
        m_started = true;

        while( m_stop_thread == false )
        {
            GPIO::output( m_ch_info.channel, GPIO::HIGH );
            this_thread::sleep_for( chrono::microseconds( m_on_time ) );

            GPIO::output( m_ch_info.channel, GPIO::LOW );
            this_thread::sleep_for( chrono::microseconds( m_off_time ) );
        }

        m_started = false;
    }

    void GpioPwmIfSw::_reconfigure( int frequency_hz, double duty_cycle_percent,
                                    bool start )
    {
        if( ( duty_cycle_percent < 0.0 ) || ( duty_cycle_percent > 100.0 ) )
        {
            throw runtime_error( "invalid duty_cycle_percent" );
        }

        bool freq_change = start || ( m_frequency_hz != frequency_hz );
        bool stop        = m_started && freq_change;

        if( stop )
        {
            this->stop( );
        }

        if( freq_change )
        {
            m_frequency_hz = frequency_hz;
            m_basetime     = 1000.0 / m_frequency_hz; // ms
        }

        m_duty_cycle_percent = duty_cycle_percent;
        calculate_times( );

        if( start || stop )
        {
            this->start( );
        }
    }

    GpioPwmIfSw::~GpioPwmIfSw( )
    {
        stop( );
    }

} // namespace GPIO
