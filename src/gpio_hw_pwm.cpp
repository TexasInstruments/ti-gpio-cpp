/*
Copyright (c) 2012-2017 Ben Croston ben@croston.org.
Copyright (c) 2019, NVIDIA CORPORATION.
Copyright (c) 2019 Jueon Park(pjueon) bluegbg@gmail.com.
Copyright (c) 2021 Adam Rasburn blackforestcheesecake@protonmail.ch
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

#include <dirent.h>
#include <unistd.h>

// Standard headers
#include <chrono>
#include <stdexcept>
#include <iostream>
#include <sstream>

// Interface headers
#include <GPIO.h>

// Local headers
#include "gpio_common.h"
#include "gpio_hw_pwm.h"

using namespace std;

namespace GPIO
{
string hw_pwm_path(const ChannelInfo& ch_info) { return ch_info.pwm_chip_dir + "/pwm" + to_string(ch_info.pwm_id); }

string hw_pwm_export_path(const ChannelInfo& ch_info) { return ch_info.pwm_chip_dir + "/export"; }

string hw_pwm_unexport_path(const ChannelInfo& ch_info) { return ch_info.pwm_chip_dir + "/unexport"; }

string hw_pwm_period_path(const ChannelInfo& ch_info) { return hw_pwm_path(ch_info) + "/period"; }

string hw_pwm_duty_cycle_path(const ChannelInfo& ch_info) { return hw_pwm_path(ch_info) + "/duty_cycle"; }

string hw_pwm_enable_path(const ChannelInfo& ch_info) { return hw_pwm_path(ch_info) + "/enable"; }

void hw_export_pwm(const ChannelInfo& ch_info)
{
    if (!os_path_exists(hw_pwm_path(ch_info)))
    { // scope for f
        string path = hw_pwm_export_path(ch_info);
        ofstream f(path);

        if (!f.is_open())
            throw runtime_error("Can't open " + path);

        f << ch_info.pwm_id;
    } // scope ends

    string enable_path = hw_pwm_enable_path(ch_info);

    int time_count = 0;
    while (!os_access(enable_path, R_OK | W_OK)) {
        this_thread::sleep_for(chrono::milliseconds(10));
        if (time_count++ > 100)
            throw runtime_error("Permission denied: path: " + enable_path +
                                "\n Please configure permissions or use the root user to run this.");
    }

    ch_info.f_duty_cycle->open(hw_pwm_duty_cycle_path(ch_info), std::ios::in | std::ios::out);
}

void hw_unexport_pwm(const ChannelInfo& ch_info)
{
    ch_info.f_duty_cycle->close();

    ofstream f(hw_pwm_unexport_path(ch_info));
    f << ch_info.pwm_id;
}

void hw_set_pwm_period(const ChannelInfo& ch_info, const int period_ns)
{
    ofstream f(hw_pwm_period_path(ch_info));
    f << period_ns;
}

void hw_set_pwm_duty_cycle(const ChannelInfo& ch_info, const int duty_cycle_ns)
{
    // On boot, both period and duty cycle are both 0. In this state, the period
    // must be set first; any configuration change made while period==0 is
    // rejected. This is fine if we actually want a duty cycle of 0. Later, once
    // any period has been set, we will always be able to set a duty cycle of 0.
    // The code could be written to always read the current value, and only
    // write the value if the desired value is different. However, we enable
    // this check only for the 0 duty cycle case, to avoid having to read the
    // current value every time the duty cycle is set.
    if (duty_cycle_ns == 0) {
        ch_info.f_duty_cycle->seekg(0, std::ios::beg);
        stringstream buffer{};
        buffer << ch_info.f_duty_cycle->rdbuf();
        auto cur = buffer.str();
        cur = strip(cur);

        if (cur == "0")
            return;
    }

    ch_info.f_duty_cycle->seekg(0, std::ios::beg);
    *ch_info.f_duty_cycle << duty_cycle_ns;
    ch_info.f_duty_cycle->flush();
}

void hw_enable_pwm(const ChannelInfo& ch_info)
{
    ofstream f(hw_pwm_enable_path(ch_info));
    f << 1;
}

void hw_disable_pwm(const ChannelInfo& ch_info)
{
    ofstream f(hw_pwm_enable_path(ch_info));
    f << 0;
}

GpioPwmIfHw::GpioPwmIfHw(int channel, int frequency_hz):
    GpioPwmIf(channel, frequency_hz)
{
    if (frequency_hz <= 0.0)
    {
        throw runtime_error("Invalid frequency");
    }

    auto &global = GlobalVariableWrapper::get_instance();

    try
    {
        Directions app_cfg = _app_channel_configuration(m_ch_info);
        if (app_cfg == HARD_PWM)
            throw runtime_error("Can't create duplicate PWM objects");
        /*
        Apps typically set up channels as GPIO before making them be PWM,
        because RPi.GPIO does soft-PWM. We must undo the GPIO export to
        allow HW PWM to run on the pin.
        */
        if (app_cfg == IN || app_cfg == OUT)
            cleanup(channel);

        if (global._gpio_warnings)
        {
            auto sysfs_cfg = _channel_configuration(m_ch_info);
            app_cfg = _app_channel_configuration(m_ch_info);

            // warn if channel has been setup external to current program
            if (app_cfg == UNKNOWN && sysfs_cfg != UNKNOWN)
            {
                cerr << "[WARNING] This channel is already in use, continuing "
                        "anyway. "
                        "Use GPIO::setwarnings(false) to disable warnings. "
                     << "channel: " << channel << endl;
            }
        }

        hw_export_pwm(m_ch_info);
        hw_set_pwm_duty_cycle(m_ch_info, 0);
        // Anything that doesn't match new frequency_hz
        m_frequency_hz = -1 * frequency_hz;
        _reconfigure(frequency_hz, 0.0);
        global._channel_configuration[to_string(channel)] = HARD_PWM;
    }

    catch (exception& e)
    {
        _cleanup_all();
        throw e;
    }
}

void GpioPwmIfHw::start()
{
    try
    {
        _reconfigure(m_frequency_hz, m_duty_cycle_percent, true);
    }

    catch (exception& e)
    {
        _cleanup_all();
        throw e;
    }
}

void GpioPwmIfHw::stop()
{
    try
    {
        if (!m_started)
            return;
        hw_disable_pwm(m_ch_info);

    }

    catch (exception& e)
    {
        cout<<"STOP 4"<<std::endl;
        _cleanup_all();
        throw e;
    }
}

void GpioPwmIfHw::_reconfigure(int frequency_hz, double duty_cycle_percent, bool start)
{
    if (duty_cycle_percent < 0.0 || duty_cycle_percent > 100.0)
        throw runtime_error("invalid duty_cycle_percent");

    bool freq_change = start || (frequency_hz != m_frequency_hz);
    bool stop = m_started && freq_change;

    if (stop)
    {
        m_started = false;
        hw_disable_pwm(m_ch_info);
    }

    if (freq_change)
    {
        m_frequency_hz = frequency_hz;
        m_period_ns = int(1000000000.0 / frequency_hz);
        hw_set_pwm_period(m_ch_info, m_period_ns);
    }

    m_duty_cycle_percent = duty_cycle_percent;
    m_duty_cycle_ns = int(m_period_ns * (duty_cycle_percent / 100.0));

    if (stop || start)
    {
        hw_enable_pwm(m_ch_info);
        m_started = true;
    }

    hw_set_pwm_duty_cycle(m_ch_info, m_duty_cycle_ns);
}

GpioPwmIfHw::~GpioPwmIfHw()
{
    stop();
}

} // namespace GPIO
