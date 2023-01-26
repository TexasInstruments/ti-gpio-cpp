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
#include "gpio_event.h"
#include "gpio_pin_data.h"
#include "gpio_common.h"
#include "gpio_hw_pwm.h"
#include "gpio_sw_pwm.h"

using namespace GPIO;
using namespace std;

namespace GPIO
{
constexpr auto _export_dir() { return _SYSFS_ROOT "/export"; }
constexpr auto _unexport_dir() { return _SYSFS_ROOT "/unexport"; }

//================================================================================
auto& global = GlobalVariableWrapper::get_instance();

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
    if (need_gpio && is_None(ch_info.gpio_chip_dir))
        throw runtime_error("Channel " + channel + " is not a GPIO");
    if (need_pwm && is_None(ch_info.pwm_chip_dir))
        throw runtime_error("Channel " + channel + " is not a PWM");
    return ch_info;
}

ChannelInfo _channel_to_info(const string& channel, bool need_gpio = false, bool need_pwm = false)
{
    _validate_mode_set();
    return _channel_to_info_lookup(channel, need_gpio, need_pwm);
}

vector<ChannelInfo> _channels_to_infos(const vector<string>& channels, bool need_gpio = false, bool need_pwm = false)
{
    _validate_mode_set();
    vector<ChannelInfo> ch_infos{};
    for (const auto& c : channels) {
        ch_infos.push_back(_channel_to_info_lookup(c, need_gpio, need_pwm));
    }
    return ch_infos;
}

/* Return the current configuration of a channel as reported by sysfs.
   Any of IN, OUT, HARD_PWM, or UNKNOWN may be returned. */
Directions _sysfs_channel_configuration(const ChannelInfo& ch_info)
{
    if (!is_None(ch_info.pwm_chip_dir)) {
        string pwm_dir = ch_info.pwm_chip_dir + "/pwm" + to_string(ch_info.pwm_id);
        if (os_path_exists(pwm_dir))
            return HARD_PWM;
    }

    string gpio_dir = _SYSFS_ROOT + "/gpio"s + to_string(ch_info.gpio);
    if (!os_path_exists(gpio_dir))
        return UNKNOWN; // Originally returns None in TI's GPIO Python Library

    string gpio_direction;
    { // scope for f
        ifstream f_direction(gpio_dir + "/direction");
        stringstream buffer{};
        buffer << f_direction.rdbuf();
        gpio_direction = buffer.str();
        gpio_direction = strip(gpio_direction);
        // lower()
        transform(gpio_direction.begin(), gpio_direction.end(), gpio_direction.begin(),
                  [](unsigned char c) { return tolower(c); });
    } // scope ends

    if (gpio_direction == "in")
        return IN;
    else if (gpio_direction == "out")
        return OUT;
    else
        return UNKNOWN; // Originally returns None in TI's GPIO Python Library
}

/* Return the current configuration of a channel as requested by this
   module in this process. Any of IN, OUT, or UNKNOWN may be returned. */
Directions _app_channel_configuration(const ChannelInfo& ch_info)
{
    if (!is_in(ch_info.channel, global._channel_configuration))
        return UNKNOWN; // Originally returns None in TI's GPIO Python Library
    return global._channel_configuration[ch_info.channel];
}

void _export_gpio(const int gpio)
{
    if (os_path_exists(_SYSFS_ROOT + "/gpio"s + to_string(gpio)))
        return;
    { // scope for f_export
        ofstream f_export(_SYSFS_ROOT + "/export"s);
        f_export << gpio;
    } // scope ends

    string value_path = _SYSFS_ROOT + "/gpio"s + to_string(gpio) + "/value"s;

    int time_count = 0;
    while (!os_access(value_path, R_OK | W_OK)) {
        this_thread::sleep_for(chrono::milliseconds(10));
        if (time_count++ > 100)
            throw runtime_error("Permission denied: path: " + value_path +
                                "\n Please configure permissions or use the root user to run this.");
    }
}

void _unexport_gpio(const int gpio)
{
    if (!os_path_exists(_SYSFS_ROOT + "/gpio"s + to_string(gpio)))
        return;

    ofstream f_unexport(_SYSFS_ROOT + "/unexport"s);
    f_unexport << gpio;
}

void _output_one(const string gpio, const int value)
{
    ofstream value_file(_SYSFS_ROOT + "/gpio"s + gpio + "/value"s);
    value_file << int(bool(value));
}

void _output_one(const int gpio, const int value) { _output_one(to_string(gpio), value); }

void _setup_single_out(const ChannelInfo& ch_info, int initial = -1)
{
    _export_gpio(ch_info.gpio);

    string gpio_dir_path = _SYSFS_ROOT + "/gpio"s + to_string(ch_info.gpio) + "/direction"s;
    { // scope for direction_file
        ofstream direction_file(gpio_dir_path);
        direction_file << "out";
    } // scope ends

    if (initial != -1)
        _output_one(ch_info.gpio, initial);

    global._channel_configuration[ch_info.channel] = OUT;
}

void _setup_single_in(const ChannelInfo& ch_info)
{
    _export_gpio(ch_info.gpio);

    string gpio_dir_path = _SYSFS_ROOT + "/gpio"s + to_string(ch_info.gpio) + "/direction"s;
    { // scope for direction_file
        ofstream direction_file(gpio_dir_path);
        direction_file << "in";
    } // scope ends

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
        _event_cleanup(ch_info.gpio);
        _unexport_gpio(ch_info.gpio);
    }

    global._channel_configuration.erase(ch_info.channel);
}

void _cleanup_all()
{
    auto copied = global._channel_configuration;
    for (const auto& _pair : copied) {
        const auto& channel = _pair.first;
        ChannelInfo ch_info = _channel_to_info(channel);
        _cleanup_one(ch_info);
    }
    global._gpio_mode = NumberingModes::None;
}

//==================================================================================
// APIs

// The reason that model and BOARD_INFO are not wrapped by
// GlobalVariableWrapper is because they belong to API

// GlobalVariableWrapper singleton object must be initialized before
// model and BOARD_INFO.
extern const string model = GlobalVariableWrapper::get_model();
extern const string BOARD_INFO = GlobalVariableWrapper::get_BOARD_INFO();

/* Function used to enable/disable warnings during setup and cleanup. */
void setwarnings(bool state) { global._gpio_warnings = state; }

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
NumberingModes getmode() { return global._gpio_mode; }

/* Function used to setup individual pins or lists/tuples of pins as
   Input or Output. direction must be IN or OUT, initial must be
   HIGH or LOW and is only valid when direction is OUT  */
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
            Directions sysfs_cfg = _sysfs_channel_configuration(ch_info);
            Directions app_cfg = _app_channel_configuration(ch_info);

            if (app_cfg == UNKNOWN && sysfs_cfg != UNKNOWN)
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

void setup(int channel, Directions direction, int initial) { setup(to_string(channel), direction, initial); }

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

/* Function used to cleanup channels at the end of the program.
   If no channel is provided, all channels are cleaned */
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

/* Function used to return the current value of the specified channel.
   Function returns either HIGH or LOW */
int input(const string& channel)
{
    try
    {
        ChannelInfo ch_info = _channel_to_info(channel, true);

        Directions app_cfg = _app_channel_configuration(ch_info);

        if (app_cfg != IN && app_cfg != OUT)
            throw runtime_error("You must setup() the GPIO channel first");

        ifstream value(_SYSFS_ROOT + "/gpio"s + to_string(ch_info.gpio) + "/value"s);
        int value_read;
        value >> value_read;
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

int input(int channel) { return input(to_string(channel)); }

/* Function used to set a value to a channel.
   Values must be either HIGH or LOW */
void output(const string& channel, int value)
{
    try
    {
        ChannelInfo ch_info = _channel_to_info(channel, true);
        // check that the channel has been set as output
        if (_app_channel_configuration(ch_info) != OUT)
            throw runtime_error("The GPIO channel has not been set up as an OUTPUT");
        _output_one(ch_info.gpio, value);
    }
    catch (exception& e)
    {
        cerr << "[Exception] " << e.what()
             << " (caught from: output())"
             << endl;
    }
}

void output(int channel, int value) { output(to_string(channel), value); }

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

    for (auto i = 0; i < channels.size(); i++)
    {
        output(*c++, *v++);
    }
}

/* Function used to check the currently set function of the channel specified.
 */
Directions gpio_function(const string& channel)
{
    try {
        ChannelInfo ch_info = _channel_to_info(channel);
        return _sysfs_channel_configuration(ch_info);
    } catch (exception& e) {
        cerr << "[Exception] " << e.what() << " (caught from: gpio_function())" << endl;
        terminate();
    }
}

Directions gpio_function(int channel) { return gpio_function(to_string(channel)); }

//=============================== Events =================================

bool event_detected(const std::string& channel)
{
    ChannelInfo ch_info = _channel_to_info(channel, true);
    try {
        // channel must be setup as input
        Directions app_cfg = _app_channel_configuration(ch_info);
        if (app_cfg != Directions::IN)
            throw runtime_error("You must setup() the GPIO channel as an input first");

        return _edge_event_detected(ch_info.gpio);
    } catch (exception& e) {
        cerr << "[Exception] " << e.what() << " (caught from: GPIO::event_detected())" << endl;
        _cleanup_all();
        terminate();
    }
}

bool event_detected(int channel) { return event_detected(std::to_string(channel)); }

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
        if (_edge_event_exists(ch_info.gpio))
            throw runtime_error("The edge event must have been set via add_event_detect()");

        // Execute
        EventResultCode result = (EventResultCode)_add_edge_callback(ch_info.gpio, callback);
        switch (result) {
        case EventResultCode::None:
            break;
        default: {
            const char* error_msg = event_error_code_to_message[result];
            throw runtime_error(error_msg ? error_msg : "Unknown Error");
        }
        }
    } catch (exception& e) {
        cerr << "[Exception] " << e.what() << " (caught from: GPIO::add_event_callback())" << endl;
        _cleanup_all();
        terminate();
    }
}

void add_event_callback(int channel, const Callback& callback)
{
    add_event_callback(std::to_string(channel), callback);
}

void remove_event_callback(const std::string& channel, const Callback& callback)
{
    ChannelInfo ch_info = _channel_to_info(channel, true);

    _remove_edge_callback(ch_info.gpio, callback);
}

void remove_event_callback(int channel, const Callback& callback)
{
    remove_event_callback(std::to_string(channel), callback);
}

void add_event_detect(const std::string& channel, Edge edge, const Callback& callback, unsigned long bounce_time)
{
    add_event_detect(std::atoi(channel.data()), edge, callback, bounce_time);
}

void add_event_detect(int channel, Edge edge, const Callback& callback, unsigned long bounce_time)
{
    try {
        ChannelInfo ch_info = _channel_to_info(std::to_string(channel), true);

        // channel must be setup as input
        Directions app_cfg = _app_channel_configuration(ch_info);
        if (app_cfg != Directions::IN) {
            throw runtime_error("You must setup() the GPIO channel as an input first");
        }

        // edge provided must be rising, falling or both
        if (edge != Edge::RISING && edge != Edge::FALLING && edge != Edge::BOTH)
            throw invalid_argument("argument 'edge' must be set to RISING, FALLING or BOTH");

        // Execute
        EventResultCode result = (EventResultCode)_add_edge_detect(ch_info.gpio, channel, edge, bounce_time);
        switch (result) {
        case EventResultCode::None:
            break;
        default: {
            const char* error_msg = event_error_code_to_message[result];
            throw runtime_error(error_msg ? error_msg : "Unknown Error");
        }
        }

        if (callback != nullptr) {
            if (_add_edge_callback(ch_info.gpio, callback))
                // Shouldn't happen (--it was just added successfully)
                throw runtime_error("Couldn't add callback due to unknown error with just added event");
        }
    } catch (exception& e) {
        cerr << "[Exception] " << e.what() << " (caught from: GPIO::add_event_detect())" << endl;
        _cleanup_all();
        terminate();
    }
}

void remove_event_detect(const std::string& channel)
{
    ChannelInfo ch_info = _channel_to_info(channel, true);

    _remove_edge_detect(ch_info.gpio);
}

void remove_event_detect(int channel) { remove_event_detect(std::to_string(channel)); }

int wait_for_edge(const std::string& channel, Edge edge, uint64_t bounce_time, uint64_t timeout)
{
    return wait_for_edge(std::atoi(channel.data()), edge, bounce_time, timeout);
}

int wait_for_edge(int channel, Edge edge, uint64_t bounce_time, uint64_t timeout)
{
    try {
        ChannelInfo ch_info = _channel_to_info(std::to_string(channel), true);

        // channel must be setup as input
        Directions app_cfg = _app_channel_configuration(ch_info);
        if (app_cfg != Directions::IN) {
            throw runtime_error("You must setup() the GPIO channel as an input first");
        }

        // edge provided must be rising, falling or both
        if (edge != Edge::RISING && edge != Edge::FALLING && edge != Edge::BOTH)
            throw invalid_argument("argument 'edge' must be set to RISING, FALLING or BOTH");

        // Execute
        EventResultCode result =
            (EventResultCode)_blocking_wait_for_edge(ch_info.gpio, channel, edge, bounce_time, timeout);
        switch (result) {
        case EventResultCode::None:
            // Timeout
            return 0;
        case EventResultCode::EdgeDetected:
            // Event Detected
            return channel;
        default: {
            const char* error_msg = event_error_code_to_message[result];
            throw runtime_error(error_msg ? error_msg : "Unknown Error");
        }
        }
    } catch (exception& e) {
        cerr << "[Exception] " << e.what() << " (caught from: GPIO::wait_for_edge())" << endl;
        _cleanup_all();
        terminate();
    }
}

// PWM class ==========================================================
GpioPwmIf::GpioPwmIf(int channel, int frequency_hz):
    m_ch_info(_channel_to_info(to_string(channel), true, false))
{
}

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

        if (global._gpio_warnings) {
            auto sysfs_cfg = _sysfs_channel_configuration(pImpl->m_ch_info);
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
    if (!is_in(pImpl->m_ch_info.channel, global._channel_configuration)) {
        /* The user probably ran cleanup() on the channel already, so avoid
           attempts to repeat the cleanup operations. */
        return;
    }
    try {
        stop();
        global._channel_configuration.erase(pImpl->m_ch_info.channel);
        global._pwm_channels.erase(pImpl->m_ch_info.channel);

    } catch (...) {
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


//=======================================
// Callback
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
//=======================================


//=======================================
struct _cleaner {
private:
    _cleaner() = default;

public:
    _cleaner(const _cleaner&) = delete;
    _cleaner& operator=(const _cleaner&) = delete;

    static _cleaner& get_instance()
    {
        static _cleaner singleton;
        return singleton;
    }

    ~_cleaner()
    {
        try {
            // When the user forgot to call cleanup() at the end of the program,
            // _cleaner object will call it.
            _cleanup_all();
        } catch (exception& e) {
            cerr << "Exception: " << e.what() << endl;
            cerr << "Exception from destructor of _cleaner class." << endl;
        }
    }
};

// Explicit instantiation
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

// AutoCleaner will be destructed at the end of the program, and call
// _cleanup_all(). It COULD cause a problem because at the end of the program,
// global._channel_configuration and global._gpio_mode MUST NOT be destructed
// before AutoCleaner. But the static objects are destructed in the reverse
// order of construction, and objects defined in the same compilation unit will
// be constructed in the order of definition. So it's supposed to work properly.
_cleaner& AutoCleaner = _cleaner::get_instance();

} // namespace GPIO

//=========================================================================
