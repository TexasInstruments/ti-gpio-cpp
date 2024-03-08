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
#include <thread>
#include <atomic>

// Interface headers
#include <GPIO.h>

// Local headers
#include "model.h"
#include "python_functions.h"
#include "gpio_pin_data.h"
#include "gpio_common.h"
#include "gpio_hw_pwm.h"
#include "gpio_sw_pwm.h"

#define MAX_EVENTS 20
#define BLOCK_FOREVER -1

using namespace GPIO;
using namespace std;

namespace GPIO
{

    //================================================================================
    auto& global = GlobalVariableWrapper::get_instance();
    typedef  void (*cb) (int channel);
    std::thread* event_handler = nullptr;
    std::atomic_bool _run_loop;
    std::atomic_bool end_wait_event = false;

    gpiod_chip  *chip;
    gpiod_line_info *line_info;
    gpiod_line_direction gpio_direction = GPIOD_LINE_DIRECTION_AS_IS;
    gpiod_line_config *line_config;
    gpiod_line_request *line_req;
    gpiod_line_settings *line_settings;
    gpiod_edge_event_buffer* event_buffer;

    std::set <gpiod_chip *> chips_open;
    std::map <const int, gpiod_line_info *> channelLineInfo;
    std::map <const int, gpiod_line_config *> channelLineConfig;
    std::map <const int, gpiod_line_settings *> channelLineSettings;
    std::map <const int, gpiod_line_request *> channelLineRequest;
    std::map <const int, gpiod_edge_event_buffer *> channelEventBuffer;
    std::vector<Callback> event_callbacks;
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

    Directions _channel_configuration(const ChannelInfo& ch_info)
    {
        if (!is_None(ch_info.pwm_chip_dir))
        {
            string pwm_dir = ch_info.pwm_chip_dir + "/pwm" + to_string(ch_info.pwm_id);
            if (os_path_exists(pwm_dir))
                return HARD_PWM;
        }
        else
        {
            if (chip != NULL)
            {
                line_info = gpiod_chip_get_line_info(chip, ch_info.gpio);
                if (line_info != NULL)
                {
                    channelLineInfo[ch_info.gpio] = line_info;
                    gpio_direction = gpiod_line_info_get_direction(line_info);
                }
            }
        }

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

    void _setup_single_out(const ChannelInfo& ch_info, int initial)
    {
        std::string gpiochipX = "/dev/gpiochip" + to_string(ch_info.chip_gpio);
        chip = gpiod_chip_open(gpiochipX.c_str());
        if (chip == NULL)
        {
            throw runtime_error("GPIO open chip failed\n");
        }

        if (chips_open.find(chip) == chips_open.end())
        {
            chips_open.insert(chip);
        }

        line_config = gpiod_line_config_new();
        if (line_config == NULL)
        {
            gpiod_line_info_free(line_info);
            gpiod_chip_close(chip);

            throw runtime_error("failed to get line config\n");
        }

        line_settings = gpiod_line_settings_new();

        gpiod_line_value val;

        if (initial == 1)
        {
            val = GPIOD_LINE_VALUE_ACTIVE;
        }
        else
        {
            val = GPIOD_LINE_VALUE_INACTIVE;
        }

        int status = gpiod_line_settings_set_output_value(line_settings, val);
        if (status == -1)
        {
            gpiod_line_settings_free(line_settings);
            gpiod_line_config_free(line_config);
            gpiod_line_info_free(line_info);
            gpiod_chip_close(chip);

            throw runtime_error("failed to set the output value the given GPIO line\n");
        }

        status = gpiod_line_settings_set_direction (line_settings, GPIOD_LINE_DIRECTION_OUTPUT);
        if (status == -1)
        {
            gpiod_line_settings_free(line_settings);
            gpiod_line_config_free(line_config);
            gpiod_line_info_free(line_info);
            gpiod_chip_close(chip);

            throw runtime_error("failed to set the direction for the given GPIO line\n");
        }

        status = gpiod_line_config_add_line_settings(line_config, &ch_info.gpio, 1, line_settings);
        if (status == -1)
        {
            gpiod_line_settings_free(line_settings);
            gpiod_line_config_free(line_config);
            gpiod_line_info_free(line_info);
            gpiod_chip_close(chip);

            throw runtime_error("failed to configure the GPIO line\n");
        }

        if (channelLineRequest.find(ch_info.gpio) != channelLineRequest.end())
        {
            gpiod_line_request_release(channelLineRequest[ch_info.gpio]);
            channelLineRequest.erase(ch_info.gpio);
        }

        line_req = gpiod_chip_request_lines(chip, NULL, line_config);

        if (line_req == NULL)
        {
            gpiod_line_settings_free(line_settings);
            gpiod_line_config_free(line_config);
            gpiod_line_info_free(line_info);
            gpiod_chip_close(chip);

            throw runtime_error("failed to get the requested GPIO line\n");
        }

        channelLineConfig[ch_info.gpio] = line_config;
        channelLineRequest[ch_info.gpio] = line_req;
        channelLineSettings[ch_info.gpio] = line_settings;

        global._channel_configuration[ch_info.channel] = OUT;
    }

    void _setup_single_in(const ChannelInfo& ch_info)
    {
        std::string gpiochipX = "/dev/gpiochip" + to_string(ch_info.chip_gpio);
        chip = gpiod_chip_open(gpiochipX.c_str());
        if (chip == NULL)
        {
            throw runtime_error("GPIO open chip failed\n");
        }

        if (chips_open.find(chip) == chips_open.end())
        {
            chips_open.insert(chip);
        }

        line_config = gpiod_line_config_new();
        if (line_config == NULL)
        {
            gpiod_line_info_free(line_info);
            gpiod_chip_close(chip);

            throw runtime_error("failed to get line config\n");
        }

        line_settings = gpiod_line_settings_new();

        int status = gpiod_line_settings_set_direction (line_settings, GPIOD_LINE_DIRECTION_INPUT);
        if (status == -1)
        {
            gpiod_line_settings_free(line_settings);
            gpiod_line_config_free(line_config);
            gpiod_line_info_free(line_info);
            gpiod_chip_close(chip);

            throw runtime_error("failed to set the direction for the given GPIO line\n");
        }

        status = gpiod_line_config_add_line_settings(line_config, &ch_info.gpio, 1, line_settings);
        if (status == -1)
        {
            gpiod_line_settings_free(line_settings);
            gpiod_line_config_free(line_config);
            gpiod_line_info_free(line_info);
            gpiod_chip_close(chip);

            throw runtime_error("failed to configure the GPIO line\n");
        }

        if (channelLineRequest.find(ch_info.gpio) != channelLineRequest.end())
        {
            gpiod_line_request_release(channelLineRequest[ch_info.gpio]);
            channelLineRequest.erase(ch_info.gpio);
        }

        line_req = gpiod_chip_request_lines (chip, NULL, line_config);
        if (line_req == NULL)
        {
            gpiod_line_settings_free(line_settings);
            gpiod_line_config_free(line_config);
            gpiod_line_info_free(line_info);
            gpiod_chip_close(chip);

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
            hw_disable_pwm(ch_info);
            hw_unexport_pwm(ch_info);
        }
        else
        {
            event_cleanup(ch_info.gpio);
            gpiod_line_request_release(channelLineRequest[ch_info.gpio]);
            gpiod_line_settings_free(channelLineSettings[ch_info.gpio]);
            gpiod_line_config_free(channelLineConfig[ch_info.gpio]);
            gpiod_line_info_free(channelLineInfo[ch_info.gpio]);
        }

        channelLineRequest.erase(ch_info.gpio);
        channelLineSettings.erase(ch_info.gpio);
        channelLineConfig.erase(ch_info.gpio);
        channelLineInfo.erase(ch_info.gpio);

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

        for (auto it = chips_open.begin(); it != chips_open.end(); it++)
        {
            gpiod_chip_close(*it);
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
            if (global._pwm_channels.find(channel) != global._pwm_channels.end())
            {
                throw runtime_error("Channel " + channel + " already running as PWM.");
            }
        }

        catch (exception& e)
        {
            cerr << "[Exception] " << e.what() << " (caught from: GPIO::setup())" << endl;
            _cleanup_all();
            terminate();
        }

        try
        {
            ChannelInfo ch_info = _channel_to_info(channel, true);
            if (global._gpio_warnings)
            {
                Directions gpiod_cfg = _channel_configuration(ch_info);
                Directions app_cfg = _app_channel_configuration(ch_info);

                if (app_cfg == UNKNOWN && gpiod_cfg != UNKNOWN)
                {
                    cerr << "[WARNING] This channel is already in use, continuing anyway. "
                            "Use GPIO::setwarnings(false) to "
                            "disable warnings.\n";
                }
            }
            if (is_None(ch_info.pwm_chip_dir))
            {
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

    template <typename T>
    void setup(const std::initializer_list<T> &channels, Directions direction, int initial)
    {
        if ((direction == IN) && (initial != -1))
        {
            throw runtime_error("initial parameter is not valid for inputs");
        }

        for (const auto &c : channels)
        {
            setup(c, direction, initial);
        }
    }

    /*
    Function used to cleanup channels at the end of the program.
    If no channel is provided, all channels are cleaned
    */
    void cleanup(const string& channel)
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
            if (is_None(channel))
            {
                _cleanup_all();
                return;
            }

            ChannelInfo ch_info = _channel_to_info(channel);
            if (is_in(ch_info.channel, global._channel_configuration))
            {
                _cleanup_one(ch_info);
            }
        }

        catch (exception& e)
        {
        cerr << "[Exception] " << e.what()
             << " (caught from: cleanup())"
             << endl;
        }
    }

    void cleanup(int channel)
    {
        string str_channel = to_string(channel);
        cleanup(str_channel);
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

    template <typename T>
    void output(const std::initializer_list<T> &channels, int value)
    {
        for (const auto &c : channels)
        {
            output(c, value);
        }
    }

    template <typename T>
    void output(const std::initializer_list<T> &channels, const std::initializer_list<int> &values)
    {
        if (channels.size() != values.size())
        {
            throw runtime_error("Number of values != number of channels");
        }

        auto c = channels.begin();
        auto v = values.begin();

        for (auto it = channels.begin(); it != channels.end(); it++)
        {
            output(*c++, *v++);
        }
    }

    /*
    Function used to check the currently set function of the channel specified.
    */

    Directions gpio_function(const string& channel)
    {
        try
        {
            ChannelInfo ch_info = _channel_to_info(channel);
            return _channel_configuration(ch_info);
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

    //=============================== EVENTS =================================

    void add_event_callback(const std::string& channel, const Callback& callback)
    {
        try {
            // Argument Check
            if (callback == nullptr) {
                throw invalid_argument("callback cannot be null");
            }

            ChannelInfo ch_info = _channel_to_info(channel, true);

            // channel must be setup as input
            Directions app_cfg = _app_channel_configuration(ch_info);
            if (app_cfg != Directions::IN) {
                throw runtime_error("You must setup() the GPIO channel as an input first");
            }

            // edge event must already exist
            if (gpiod_line_settings_get_edge_detection(line_settings) == GPIOD_LINE_EDGE_NONE)
                throw runtime_error("The edge event must have been set via add_event_detect()");

            // Execute
            event_callbacks.push_back(callback);
        }
        catch (exception& e)
        {
            cerr << "[Exception] " << e.what() << " (caught from: GPIO::add_event_callback())" << endl;
            _cleanup_all();
            terminate();
        }
    }

    void add_event_callback(int channel, const Callback& callback)
    {
        add_event_callback(std::to_string(channel), callback);
    }

    void add_event_detect(int channel, Edge edge, const Callback& callback, unsigned long bounce_time)
    {
        try {
            ChannelInfo ch_info = _channel_to_info(std::to_string(channel), true);

            // channel must be setup as input
            Directions app_cfg = _app_channel_configuration(ch_info);
            if (app_cfg != Directions::IN)
            {
                throw runtime_error("You must setup() the GPIO channel as an input first");
            }

            // edge provided must be rising, falling or both
            gpiod_line_edge gpiod_edge_val = GPIOD_LINE_EDGE_NONE;
            if (edge != Edge::RISING && edge != Edge::FALLING && edge != Edge::BOTH)
                throw invalid_argument("argument 'edge' must be set to RISING, FALLING or BOTH");
            else
            {
                if (edge == Edge::RISING )
                {
                    gpiod_edge_val = GPIOD_LINE_EDGE_RISING;
                }
                else if (edge == Edge::FALLING)
                {
                    gpiod_edge_val = GPIOD_LINE_EDGE_FALLING;
                }
                else
                {
                    gpiod_edge_val = GPIOD_LINE_EDGE_BOTH;
                }
            }

            int status = gpiod_line_settings_set_edge_detection (channelLineSettings[ch_info.gpio],
                                                                 gpiod_edge_val);
            if (status == -1)
            {
                gpiod_line_settings_free(channelLineSettings[ch_info.gpio]);
                gpiod_line_config_free(channelLineConfig[ch_info.gpio]);
                gpiod_line_info_free(channelLineInfo[ch_info.gpio]);
                gpiod_chip_close(chip);

                throw runtime_error("failed to configure edge on the requested line\n");
            }

            gpiod_line_settings_set_debounce_period_us (channelLineSettings[ch_info.gpio],
                                                        bounce_time);

            status = gpiod_line_config_add_line_settings (channelLineConfig[ch_info.gpio],
                                                        &ch_info.gpio,
                                                        1,
                                                        channelLineSettings[ch_info.gpio]);
            if (status == -1)
            {
                gpiod_line_settings_free(channelLineSettings[ch_info.gpio]);
                gpiod_line_config_free(channelLineConfig[ch_info.gpio]);
                gpiod_line_info_free(channelLineInfo[ch_info.gpio]);
                gpiod_chip_close(chip);

                throw runtime_error("failed to configure the GPIO line for event\n");

            }

            status = gpiod_line_request_reconfigure_lines(channelLineRequest[ch_info.gpio],
                                                        channelLineConfig[ch_info.gpio]);
            if (status == -1)
            {
                gpiod_line_settings_free(channelLineSettings[ch_info.gpio]);
                gpiod_line_config_free(channelLineConfig[ch_info.gpio]);
                gpiod_line_info_free(channelLineInfo[ch_info.gpio]);
                gpiod_chip_close(chip);

                throw runtime_error("Lines could not be reconfigured for edge events\n");
            }

            // Execute
            if (callback != nullptr)
            {
                add_event_callback(channel, callback);
            }

            event_buffer = gpiod_edge_event_buffer_new(MAX_EVENTS);
            if (event_buffer == NULL)
            {
                throw runtime_error("Create Buffer Error Occured\n");
            }
            channelEventBuffer[ch_info.gpio] = event_buffer;

            start_thread(channel);
        }
        catch (exception& e)
        {
            cerr << "[Exception] " << e.what() << " (caught from: GPIO::add_event_detect())" << endl;
            _cleanup_all();
            terminate();
        }
    }

    void wait_for_edge(const std::string& channel, Edge edge, unsigned long bounce_time, int64_t timeout)
    {
        return wait_for_edge(std::atoi(channel.data()), edge, bounce_time, timeout);
    }

    void wait_for_edge(int channel, Edge edge, unsigned long bounce_time, int64_t timeout)
    {
        try {
            ChannelInfo ch_info = _channel_to_info(std::to_string(channel), true);

            // channel must be setup as input
            Directions app_cfg = _app_channel_configuration(ch_info);
            if (app_cfg != Directions::IN) {
                throw runtime_error("You must setup() the GPIO channel as an input first");
            }

            // edge provided must be rising, falling or both
            gpiod_line_edge gpiod_edge_val = GPIOD_LINE_EDGE_NONE;
            if (edge != Edge::RISING && edge != Edge::FALLING && edge != Edge::BOTH)
            {
                throw invalid_argument("argument 'edge' must be set to RISING, FALLING or BOTH");
            }
            else
            {
                if (edge == Edge::RISING )
                {
                    gpiod_edge_val = GPIOD_LINE_EDGE_RISING;
                }
                else if (edge == Edge::FALLING)
                {
                    gpiod_edge_val = GPIOD_LINE_EDGE_FALLING;
                }
                else
                {
                    gpiod_edge_val = GPIOD_LINE_EDGE_BOTH;
                }
            }

            int status = gpiod_line_settings_set_edge_detection (channelLineSettings[ch_info.gpio],
                                                                 gpiod_edge_val);
            if (status == -1)
            {
                gpiod_line_settings_free(channelLineSettings[ch_info.gpio]);
                gpiod_line_config_free(channelLineConfig[ch_info.gpio]);
                gpiod_line_info_free(channelLineInfo[ch_info.gpio]);
                gpiod_chip_close(chip);

                throw runtime_error("failed to configure edge on the requested line\n");
            }

            gpiod_line_settings_set_debounce_period_us (channelLineSettings[ch_info.gpio],
                                                        bounce_time);

            status = gpiod_line_config_add_line_settings (channelLineConfig[ch_info.gpio],
                                                        &ch_info.gpio,
                                                        1,
                                                        channelLineSettings[ch_info.gpio]);
            if (status == -1)
            {
                gpiod_line_settings_free(channelLineSettings[ch_info.gpio]);
                gpiod_line_config_free(channelLineConfig[ch_info.gpio]);
                gpiod_line_info_free(channelLineInfo[ch_info.gpio]);
                gpiod_chip_close(chip);

                throw runtime_error("failed to configure the GPIO line for event\n");

            }

            status = gpiod_line_request_reconfigure_lines(channelLineRequest[ch_info.gpio],
                                                        channelLineConfig[ch_info.gpio]);
            if (status == -1)
            {
                gpiod_line_settings_free(channelLineSettings[ch_info.gpio]);
                gpiod_line_config_free(channelLineConfig[ch_info.gpio]);
                gpiod_line_info_free(channelLineInfo[ch_info.gpio]);
                gpiod_chip_close(chip);

                throw runtime_error("Lines could not be reconfigured for edge events\n");
            }

            // Execute
            int no_events;
            event_buffer = gpiod_edge_event_buffer_new(MAX_EVENTS);
            if (event_buffer == NULL)
            {
                throw runtime_error("Create Buffer Error Occured\n");
            }

            channelEventBuffer[ch_info.gpio] = event_buffer;

            status = gpiod_line_request_wait_edge_events (channelLineRequest[ch_info.gpio], timeout);
            end_wait_event = true;

            if (status == -1 && end_wait_event != true)
            {
                std::cout<<end_wait_event<<"\n";
                std::cout<<"FDFDFDF";
                throw runtime_error("Wait Event Error Occured\n");
            }
            else if (status == 0)
            {
                std::cout<<"Wait event timeout occured\n";
            }
            else if (status == 1)
            {
                no_events = gpiod_line_request_read_edge_events(channelLineRequest[ch_info.gpio],
                                                                channelEventBuffer[ch_info.gpio],
                                                                MAX_EVENTS);

                std::cout<<"Events Pending: "<<no_events<<"\n";
            }
            else {}

        }
        catch (exception& e)
        {
            cerr << "[Exception] " << e.what() << " (caught from: GPIO::wait_for_edge())" << endl;
            _cleanup_all();
            terminate();
        }
    }

    void start_thread(int channel)
    {
        _run_loop = true;
        event_handler = new std::thread(callback_handler, channel, event_callbacks);
        event_handler->detach();
    }

    void callback_handler(int channel, vector<Callback> event_callbacks)
    {
        ChannelInfo ch_info = _channel_to_info(std::to_string(channel), true);

        while (_run_loop)
        {
            int status = gpiod_line_request_wait_edge_events (channelLineRequest[ch_info.gpio],
                                                            BLOCK_FOREVER);

            if (status == -1)
            {
                throw runtime_error("Wait Event Error Occured\n");
            }
            else if (status == 0)
            {
                std::cout<<"Wait event timeout occured\n";
            }
            else
            {
                gpiod_line_request_read_edge_events(channelLineRequest[ch_info.gpio],
                                                    channelEventBuffer[ch_info.gpio],
                                                    MAX_EVENTS);
                std::cout<<"Event Pending"<<"\n";

                for (auto cb : event_callbacks)
                {
                    cb(channel);
                }
            }
        }
    }

    void event_cleanup(unsigned int channel)
    {
        event_callbacks.clear();
        if (!channelEventBuffer.empty() && channelEventBuffer[channel] != NULL)
            gpiod_edge_event_buffer_free(channelEventBuffer[channel]);

        stop_thread();
    }

    void stop_thread()
    {
        _run_loop = false;
        event_handler = nullptr;
    }

    //=============================== PWM =================================
    GpioPwmIf::GpioPwmIf(int channel, int frequency_hz):
                    m_ch_info(_channel_to_info(to_string(channel), true, false))
    {}

    PWM::PWM(int channel, int frequency_hz)
    {
        try
        {
            if (global._pwm_channels.find(to_string(channel)) != global._pwm_channels.end())
            {
                throw runtime_error("Channel " + to_string(channel) + " already running as PWM.");
            }
        }

        catch (exception& e)
        {
            cerr << "[Exception] " << e.what() << " (caught from: PWM::PWM())" << endl;
            _cleanup_all();
            terminate();
        }

        /* Get the channel information and check if it has PWM
        * directory information populated. The idea is that if
        * it has a PWM directory information populated then it
        * supports HW PWM functionality.
        */
        ChannelInfo ch_info = _channel_to_info(to_string(channel), false, false);

        if (!is_None(ch_info.pwm_chip_dir))
        {
            pImpl = new GpioPwmIfHw(channel, frequency_hz);
        }
        else
        {
            pImpl = new GpioPwmIfSw(channel, frequency_hz);
        }

        try
        {
            Directions app_cfg = _app_channel_configuration(pImpl->m_ch_info);

            if (global._gpio_warnings)
            {
                auto sysfs_cfg = _channel_configuration(pImpl->m_ch_info);
                app_cfg = _app_channel_configuration(pImpl->m_ch_info);

                // warn if channel has been setup external to current program
                if (app_cfg == UNKNOWN && sysfs_cfg != UNKNOWN) {
                    cerr << "[WARNING] This channel is already in use, continuing "
                            "anyway. "
                            "Use GPIO::setwarnings(false) to disable warnings"
                        << endl;
                }
            }

            pImpl->_reconfigure(frequency_hz, 0.0);
            global._channel_configuration[to_string(channel)] = GPIO::OUT;
            global._pwm_channels[to_string(channel)] = true;
        }

        catch (exception& e)
        {
            cerr << "[Exception] " << e.what() << " (caught from: PWM::PWM())" << endl;
            _cleanup_all();
            terminate();
        }
    }

    PWM::~PWM()
    {
        if (!is_in(pImpl->m_ch_info.channel, global._channel_configuration))
        {
            /*
            The user probably ran cleanup() on the channel already, so avoid
            attempts to repeat the cleanup operations.
            */
            return;
        }
        try
        {
            stop();
            global._channel_configuration.erase(pImpl->m_ch_info.channel);
            global._pwm_channels.erase(pImpl->m_ch_info.channel);

        }
        catch (...)
        {
            cerr << "[Exception] ~PWM Exception! shut down the program." << endl;
            _cleanup_all();
            terminate();
        }

        delete pImpl;
    }

    // move construct
    PWM::PWM(PWM&& other) = default;

    // move assign
    PWM& PWM::operator=(PWM&& other)
    {
        if (this == &other)
            return *this;

        pImpl = std::move(other.pImpl);
        return *this;
    }

    void PWM::start(double duty_cycle_percent)
    {
        try
        {
            pImpl->_reconfigure(pImpl->m_frequency_hz, duty_cycle_percent, true);
        }

        catch (exception& e)
        {
            cerr << "[Exception] " << e.what() << " (caught from: PWM::start())" << endl;
            _cleanup_all();
            terminate();
        }
    }

    void PWM::ChangeFrequency(int frequency_hz)
    {
        try
        {
            pImpl->_reconfigure(frequency_hz, pImpl->m_duty_cycle_percent);
        }

        catch (exception& e)
        {
            cerr << "[Exception] " << e.what()
                << " (caught from: PWM::ChangeFrequency())"
                << endl;

            terminate();
        }
    }

    void PWM::ChangeDutyCycle(double duty_cycle_percent)
    {
        try
        {
            pImpl->_reconfigure(pImpl->m_frequency_hz, duty_cycle_percent);
        }

        catch (exception& e)
        {
            cerr << "[Exception] " << e.what()
                << " (caught from: PWM::ChangeDutyCycle())"
                << endl;

            terminate();
        }
    }

    void PWM::stop()
    {
        try
        {
            pImpl->stop();
        }

        catch (exception& e)
        {
            cerr << "[Exception] " << e.what()
                 << " (caught from: PWM::stop())"
                 << endl;

            throw runtime_error("Exeception from PWM::stop");
        }
    }

    //===================================== EXPLICIT INSTANTIATION ================================
    template
    void setup<int>(const std::initializer_list<int> &channels, Directions direction, int initial);
    template
    void output<int>(const std::initializer_list<int> &channels, int value);
    template
    void output<int>(const std::initializer_list<int> &channels, const std::initializer_list<int> &values);

    template
    void setup<string>(const std::initializer_list<string> &channels, Directions direction, int initial);
    template
    void output<string>(const std::initializer_list<string> &channels, int value);
    template
    void output<string>(const std::initializer_list<string> &channels, const std::initializer_list<int> &values);

    //======================================= CALLBACK ==============================================

    void Callback::operator()(int input) const
    {
        if (function != nullptr)
            function(input);
    }


    bool operator==(const Callback& A, const Callback& B)
    {
        return A.comparer(A.function, B.function);
    }


    bool operator!=(const Callback& A, const Callback& B)
    {
        return !(A == B);
    }

}
