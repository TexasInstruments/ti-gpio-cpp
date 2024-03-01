/*
Copyright (c) 2012-2017 Ben Croston ben@croston.org.
Copyright (c) 2019, NVIDIA CORPORATION.
Copyright (c) 2019 Jueon Park(pjueon) bluegbg@gmail.com.
Copyright (c) 2021 Adam Rasburn blackforestcheesecake@protonmail.ch
Copyright (c) 2021-2023, Texas Instruments Incorporated. All rights reserved.

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

#include <algorithm>
#include <cctype>
#include <chrono>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>
#include <mutex>

// Interface headers
#include <GPIO.h>

// Local headers
#include "model.h"
#include "python_functions.h"
#include "gpio_pin_data.h"
#include "gpio_common.h"

using namespace GPIO;
using namespace std;

namespace GPIO
{

    //================================================================================
    auto& global = GlobalVariableWrapper::get_instance();
    gpiod_chip  *chip;
    gpiod_line_info *line_info;
    gpiod_line_direction gpio_direction;
    gpiod_line_config *line_config;
    gpiod_line_request *line_req;
    gpiod_line_settings *line_settings;

    std::set <gpiod_chip *> gpiochip;
    std::map <const int, gpiod_line_info *> channelLineInfo;
    std::map <const int, gpiod_line_config *> channelLineConfig;
    std::map <const int, gpiod_line_settings *> channelLineSettings;
    std::map <const int, gpiod_line_request *> channelLineRequest;
    //================================================================================

    void _validate_mode_set()
    {
    if (global._gpio_mode == NumberingModes::None)
        throw runtime_error("Please set pin numbering mode using "
                            "GPIO::setmode(GPIO::BOARD), GPIO::setmode(GPIO::BCM), "
                            "or GPIO::setmode(GPIO::SOC)");
    }

    ChannelInfo _channel_to_info_lookup(const string& channel, bool need_gpio, bool need_pwm)
    {
        if (!is_in(channel, global._channel_data))
            throw runtime_error("Channel " + channel + " is invalid");

        ChannelInfo ch_info = global._channel_data.at(channel);
        return ch_info;
    }

    ChannelInfo _channel_to_info(const string& channel, bool need_gpio = false, bool need_pwm = false)
    {
        _validate_mode_set();
        return _channel_to_info_lookup(channel, need_gpio, need_pwm);
    }

    /*
    Return the current configuration of a channel as reported by gpiod.
    Any of IN, OUT, HARD_PWM, or UNKNOWN may be returned.
    */

    Directions _gpiod_channel_configuration(const ChannelInfo& ch_info)
    {
        std::string gpiochipX = "/dev/gpiochip" + to_string(ch_info.chip_gpio);
        chip = gpiod_chip_open(gpiochipX.c_str());
        if (chip == NULL)
        {
            throw runtime_error("GPIO open chip failed\n");
        }

        line_info = gpiod_chip_get_line_info(chip, ch_info.gpio);
        if (line_info == NULL)
        {
            throw runtime_error("failed to get line info\n");
        }

        gpio_direction = gpiod_line_info_get_direction(line_info);

        gpiochip.insert(chip);
        channelLineInfo[ch_info.gpio] = line_info;

        if (gpio_direction == GPIOD_LINE_DIRECTION_INPUT)
            return IN;
        else if (gpio_direction == GPIOD_LINE_DIRECTION_OUTPUT)
            return OUT;
        else
            return UNKNOWN; // Originally returns None in TI's GPIO Python Library
    }

    /*
    Return the current configuration of a channel as requested by this
    module in this process. Any of IN, OUT, or UNKNOWN may be returned.
    */

    Directions _app_channel_configuration(const ChannelInfo& ch_info)
    {
        if (!is_in(ch_info.channel, global._channel_configuration))
            return UNKNOWN; // Originally returns None in TI's GPIO Python Library
        return global._channel_configuration[ch_info.channel];
    }

    void _setup_single_out(const ChannelInfo& ch_info, int initial = -1)
    {
        line_config = gpiod_line_config_new();
        if (line_config == NULL)
        {
            throw runtime_error("failed to get line config\n");
        }

        line_settings = gpiod_line_settings_new();

        gpiod_line_value val;
        if (initial == 0)
        {
            val = GPIOD_LINE_VALUE_INACTIVE;
        }
        else if (initial == 1)
        {
            val = GPIOD_LINE_VALUE_ACTIVE;
        }
        else{}

        int status = gpiod_line_settings_set_output_value(line_settings, val);
        if (status == -1)
        {
            throw runtime_error("failed to set the output value the given GPIO line\n");
        }

        status = gpiod_line_settings_set_direction (line_settings, GPIOD_LINE_DIRECTION_OUTPUT);
        if (status == -1)
        {
            throw runtime_error("failed to set the direction for the given GPIO line\n");
        }

        status = gpiod_line_config_add_line_settings(line_config, &ch_info.gpio, 1, line_settings);
        if (status == -1)
        {
            throw runtime_error("failed to configure the GPIO line\n");
        }

        line_req = gpiod_chip_request_lines (chip, NULL, line_config);
        if (line_req == NULL)
        {
            throw runtime_error("failed to get the requested GPIO line\n");
        }

        channelLineConfig[ch_info.gpio] = line_config;
        channelLineRequest[ch_info.gpio] = line_req;
        channelLineSettings[ch_info.gpio] = line_settings;

        global._channel_configuration[ch_info.channel] = OUT;
    }

    void _setup_single_in(const ChannelInfo& ch_info)
    {

        line_config = gpiod_line_config_new();
        if (line_config == NULL)
        {
            throw runtime_error("failed to get line config\n");
        }

        line_settings = gpiod_line_settings_new();

        int status = gpiod_line_settings_set_direction (line_settings, GPIOD_LINE_DIRECTION_INPUT);
        if (status == -1)
        {
            throw runtime_error("failed to set the direction for the given GPIO line\n");
        }

        status = gpiod_line_config_add_line_settings(line_config, &ch_info.gpio, 1, line_settings);
        if (status == -1)
        {
            throw runtime_error("failed to configure the GPIO line\n");
        }

        line_req = gpiod_chip_request_lines (chip, NULL, line_config);
        if (line_req == NULL)
        {
            throw runtime_error("failed to get the requested GPIO line\n");
        }

        channelLineConfig[ch_info.gpio] = line_config;
        channelLineRequest[ch_info.gpio] = line_req;
        channelLineSettings[ch_info.gpio] = line_settings;

        global._channel_configuration[ch_info.channel] = IN;
    }

    void _cleanup_one(const ChannelInfo& ch_info)
    {
        Directions app_cfg = global._channel_configuration[ch_info.channel];
        if (app_cfg == HARD_PWM)
        {
            // hw_disable_pwm(ch_info);
            // hw_unexport_pwm(ch_info);
        }
        else
        {
            // _event_cleanup(ch_info.gpio);
            gpiod_line_request_release(channelLineRequest[ch_info.gpio]);
            gpiod_line_settings_free(channelLineSettings[ch_info.gpio]);
            gpiod_line_config_free(channelLineConfig[ch_info.gpio]);
            gpiod_line_info_free(channelLineInfo[ch_info.gpio]);
        }

        global._channel_configuration.erase(ch_info.channel);
    }

    void _cleanup_all()
    {
        auto copied = global._channel_configuration;

        for (const auto& _pair : copied)
        {
            const auto& channel = _pair.first;
            ChannelInfo ch_info = _channel_to_info(channel);
            _cleanup_one(ch_info);
        }
        for (auto it = gpiochip.begin(); it != gpiochip.end(); it++)
        {
            gpiod_chip_close (*it);
        }
        global._gpio_mode = NumberingModes::None;
    }

    //==================================================================================
    // APIs

    /*
    The reason that model and BOARD_INFO are not wrapped by
    GlobalVariableWrapper is because they belong to API
    */

    /*
    GlobalVariableWrapper singleton object must be initialized before
    model and BOARD_INFO.
    */

    extern const string model = GlobalVariableWrapper::get_model();
    extern const string BOARD_INFO = GlobalVariableWrapper::get_BOARD_INFO();

    /* Function used to enable/disable warnings during setup and cleanup. */
    void setwarnings(bool state)
    {
        global._gpio_warnings = state;
    }

    // Function used to set the pin mumbering mode.
    // Possible mode values are BOARD, BCM, and SOC
    void setmode(NumberingModes mode)
    {
        try {
            // check if mode is valid
            if (mode == NumberingModes::None)
                throw runtime_error("Pin numbering mode must be GPIO::BOARD, GPIO::BCM, or GPIO::SOC");
            // check if a different mode has been set
            if (global._gpio_mode != NumberingModes::None && mode != global._gpio_mode)
                throw runtime_error("A different mode has already been set!");

            global._channel_data = global._channel_data_by_mode.at(mode);
            global._gpio_mode = mode;
        }

        catch (exception& e)
        {
            cerr << "[Exception] " << e.what()
                << " (caught from: setmode())"
                << endl;
            terminate();
        }
    }

    // Function used to get the currently set pin numbering mode
    NumberingModes getmode()
    {
        return global._gpio_mode;
    }

    /*
    Function used to setup individual pins or lists/tuples of pins as
    Input or Output. direction must be IN or OUT, initial must be
    HIGH or LOW and is only valid when direction is OUT
    */

    void setup(const string& channel, Directions direction, int initial)
    {
        try
        {
            ChannelInfo ch_info = _channel_to_info(channel, true);

            if (global._gpio_warnings)
            {
                Directions gpiod_cfg = _gpiod_channel_configuration(ch_info);
                Directions app_cfg = _app_channel_configuration(ch_info);

                if (app_cfg == UNKNOWN && gpiod_cfg != UNKNOWN)
                {
                    cerr << "[WARNING] This channel is already in use, continuing anyway. "
                            "Use GPIO::setwarnings(false) to "
                            "disable warnings.\n";
                }
            }

            if (direction == OUT)
            {
                _setup_single_out(ch_info, initial);
            }
            else if (direction == IN)
            {
                if (initial != -1)
                    throw runtime_error("initial parameter is not valid for inputs");
                _setup_single_in(ch_info);
            }
            else
                throw runtime_error("GPIO direction must be GPIO::IN or GPIO::OUT");
        }
        catch (exception& e)
        {
            cerr << "[Exception] " << e.what()
                << " (caught from: setup())"
                << endl;
        }

    }

    void setup(int channel, Directions direction, int initial)
    {
        setup(to_string(channel), direction, initial);
    }

    /*
    Function used to cleanup channels at the end of the program.
    */

    void cleanup()
    {
        try
        {
            // warn if no channel is setup
            if (global._gpio_mode == NumberingModes::None && global._gpio_warnings)
            {
                cerr << "[WARNING] No channels have been set up yet - nothing to clean up! "
                        "Try cleaning up at the end of your program instead!";
                return;
            }

            // clean all channels if no channel param provided
            _cleanup_all();
        }

        catch (exception& e)
        {
            cerr << "[Exception] " << e.what()
                << " (caught from: cleanup())"
                << endl;
        }
    }

    /*
    Function used to return the current value of the specified channel.
    Function returns either HIGH or LOW
    */
    // enum return or int return ??

    int input(const string& channel)
    {
        try
        {
            ChannelInfo ch_info = _channel_to_info(channel, true);

            Directions app_cfg = _app_channel_configuration(ch_info);

            if (app_cfg != IN && app_cfg != OUT)
                throw runtime_error("You must setup() the GPIO channel first");

            gpiod_line_value value_read;
            value_read = gpiod_line_request_get_value(channelLineRequest[ch_info.gpio], ch_info.gpio);
            return value_read;
        }

        catch (exception& e)
        {
            cerr << "[Exception] " << e.what()
                << " (caught from: input())"
                << endl;
            terminate();
        }
    }

    int input(int channel)
    {
        return input(to_string(channel));
    }

    /*
    Function used to set a value to a channel.
    Values must be either HIGH or LOW
    */

    void output(const string& channel, int value)
    {
        try
        {
            gpiod_line_value gpio_val;
            ChannelInfo ch_info = _channel_to_info(channel, true);
            // check that the channel has been set as output
            if (_app_channel_configuration(ch_info) != OUT)
                throw runtime_error("The GPIO channel has not been set up as an OUTPUT");

            if (value == 1)
            {
                gpio_val = GPIOD_LINE_VALUE_ACTIVE;
            }
            else
            {
                gpio_val = GPIOD_LINE_VALUE_INACTIVE;
            }

            int status = gpiod_line_request_set_value(  channelLineRequest[ch_info.gpio],
                                                        ch_info.gpio,
                                                        gpio_val);

            if (status == -1)
            {
                throw runtime_error("Could not set the pin to the given value\n");
            }

        }
        catch (exception& e)
        {
            cerr << "[Exception] " << e.what()
                << " (caught from: output())"
                << endl;
        }
    }

    void output(int channel, int value)
    {
        output(to_string(channel), value);
    }

    /*
    Function used to check the currently set function of the channel specified.
    */

    Directions gpio_function(const string& channel)
    {
        try
        {
            ChannelInfo ch_info = _channel_to_info(channel);
            return _gpiod_channel_configuration(ch_info);
        }
        catch (exception& e)
        {
            cerr << "[Exception] " << e.what() << " (caught from: gpio_function())" << endl;
            terminate();
        }
    }

    Directions gpio_function(int channel)
    {
        return gpio_function(to_string(channel));
    }

    //=============================== PWM =================================
}
