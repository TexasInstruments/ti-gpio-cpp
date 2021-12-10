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

class GlobalVariableWrapper
{
public:
    // -----Global Variables----
    // NOTE: DON'T change the declaration order of fields.
    // declaration order == initialization order

    PinData _pinData;
    const Model _model;
    const PinInfo _BOARD_INFO;
    const map<GPIO::NumberingModes, map<string, ChannelInfo>> _channel_data_by_mode;

    // A map used as lookup tables for pin to linux gpio mapping
    map<string, ChannelInfo> _channel_data;

    bool _gpio_warnings;
    NumberingModes _gpio_mode;
    map<string, Directions> _channel_configuration;
    map<string, bool> _pwm_channels;

    GlobalVariableWrapper(const GlobalVariableWrapper&) = delete;
    GlobalVariableWrapper& operator=(const GlobalVariableWrapper&) = delete;

    static GlobalVariableWrapper& get_instance()
    {
        static GlobalVariableWrapper singleton{};
        return singleton;
    }

    static string get_model()
    {
        auto& instance = get_instance();
        auto ret = ModelToString(instance._model);
        if (is_None(ret))
            throw runtime_error("get_model error");

        return ret;
    }

    static string get_BOARD_INFO()
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

private:
    GlobalVariableWrapper()
    : _pinData(get_data()), // Get GPIO pin data
      _model(_pinData.model),
      _BOARD_INFO(_pinData.pin_info),
      _channel_data_by_mode(_pinData.channel_data),
      _gpio_warnings(true),
      _gpio_mode(NumberingModes::None)
    {
        _CheckPermission();
    }

    void _CheckPermission()
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
};

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

string _pwm_path(const ChannelInfo& ch_info) { return ch_info.pwm_chip_dir + "/pwm" + to_string(ch_info.pwm_id); }

string _pwm_export_path(const ChannelInfo& ch_info) { return ch_info.pwm_chip_dir + "/export"; }

string _pwm_unexport_path(const ChannelInfo& ch_info) { return ch_info.pwm_chip_dir + "/unexport"; }

string _pwm_period_path(const ChannelInfo& ch_info) { return _pwm_path(ch_info) + "/period"; }

string _pwm_duty_cycle_path(const ChannelInfo& ch_info) { return _pwm_path(ch_info) + "/duty_cycle"; }

string _pwm_enable_path(const ChannelInfo& ch_info) { return _pwm_path(ch_info) + "/enable"; }

void _export_pwm(const ChannelInfo& ch_info)
{
    if (os_path_exists(_pwm_path(ch_info)))
        return;

    { // scope for f
        string path = _pwm_export_path(ch_info);
        ofstream f(path);

        if (!f.is_open())
            throw runtime_error("Can't open " + path);

        f << ch_info.pwm_id;
    } // scope ends

    string enable_path = _pwm_enable_path(ch_info);

    int time_count = 0;
    while (!os_access(enable_path, R_OK | W_OK)) {
        this_thread::sleep_for(chrono::milliseconds(10));
        if (time_count++ > 100)
            throw runtime_error("Permission denied: path: " + enable_path +
                                "\n Please configure permissions or use the root user to run this.");
    }
}

void _cleanup_one(const ChannelInfo& ch_info)
{
    Directions app_cfg = global._channel_configuration[ch_info.channel];
    _event_cleanup(ch_info.gpio);
    _unexport_gpio(ch_info.gpio);
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
extern const string GPIO::model = GlobalVariableWrapper::get_model();
extern const string GPIO::BOARD_INFO = GlobalVariableWrapper::get_BOARD_INFO();

/* Function used to enable/disable warnings during setup and cleanup. */
void GPIO::setwarnings(bool state) { global._gpio_warnings = state; }

// Function used to set the pin mumbering mode.
// Possible mode values are BOARD, BCM, and SOC
void GPIO::setmode(NumberingModes mode)
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
    } catch (exception& e) {
        cerr << "[Exception] " << e.what() << " (caught from: setmode())" << endl;
        terminate();
    }
}

// Function used to get the currently set pin numbering mode
NumberingModes GPIO::getmode() { return global._gpio_mode; }

/* Function used to setup individual pins or lists/tuples of pins as
   Input or Output. direction must be IN or OUT, initial must be
   HIGH or LOW and is only valid when direction is OUT  */
void GPIO::setup(const string& channel, Directions direction, int initial)
{
    try {
        if (global._pwm_channels.find(channel) != global._pwm_channels.end())
        {
            throw runtime_error("Channel " + channel + " already running as PWM.");
        }
    }

    catch (exception& e) {
        cerr << "[Exception] " << e.what() << " (caught from: GPIO::setup())" << endl;
        _cleanup_all();
        terminate();
    }

    try {
        ChannelInfo ch_info = _channel_to_info(channel, true);

        if (global._gpio_warnings) {
            Directions sysfs_cfg = _sysfs_channel_configuration(ch_info);
            Directions app_cfg = _app_channel_configuration(ch_info);

            if (app_cfg == UNKNOWN && sysfs_cfg != UNKNOWN) {
                cerr << "[WARNING] This channel is already in use, continuing anyway. Use GPIO::setwarnings(false) to "
                        "disable warnings.\n";
            }
        }

        if (direction == OUT) {
            _setup_single_out(ch_info, initial);
        } else if (direction == IN) {
            if (initial != -1)
                throw runtime_error("initial parameter is not valid for inputs");
            _setup_single_in(ch_info);
        } else
            throw runtime_error("GPIO direction must be GPIO::IN or GPIO::OUT");
    } catch (exception& e) {
        cerr << "[Exception] " << e.what() << " (caught from: setup())" << endl;
        terminate();
    }
}

void GPIO::setup(int channel, Directions direction, int initial) { setup(to_string(channel), direction, initial); }

template <typename T>
void GPIO::setup(const std::initializer_list<T> &channels, Directions direction, int initial)
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
void GPIO::cleanup(const string& channel)
{
    try {
        // warn if no channel is setup
        if (global._gpio_mode == NumberingModes::None && global._gpio_warnings) {
            cerr << "[WARNING] No channels have been set up yet - nothing to clean up! "
                    "Try cleaning up at the end of your program instead!";
            return;
        }

        // clean all channels if no channel param provided
        if (is_None(channel)) {
            _cleanup_all();
            return;
        }

        ChannelInfo ch_info = _channel_to_info(channel);
        if (is_in(ch_info.channel, global._channel_configuration)) {
            _cleanup_one(ch_info);
        }
    } catch (exception& e) {
        cerr << "[Exception] " << e.what() << " (caught from: cleanup())" << endl;
        terminate();
    }
}

void GPIO::cleanup(int channel)
{
    string str_channel = to_string(channel);
    cleanup(str_channel);
}

/* Function used to return the current value of the specified channel.
   Function returns either HIGH or LOW */
int GPIO::input(const string& channel)
{
    try {
        ChannelInfo ch_info = _channel_to_info(channel, true);

        Directions app_cfg = _app_channel_configuration(ch_info);

        if (app_cfg != IN && app_cfg != OUT)
            throw runtime_error("You must setup() the GPIO channel first");

        { // scope for value
            ifstream value(_SYSFS_ROOT + "/gpio"s + to_string(ch_info.gpio) + "/value"s);
            int value_read;
            value >> value_read;
            return value_read;
        } // scope ends
    } catch (exception& e) {
        cerr << "[Exception] " << e.what() << " (caught from: input())" << endl;
        terminate();
    }
}

int GPIO::input(int channel) { return input(to_string(channel)); }

/* Function used to set a value to a channel.
   Values must be either HIGH or LOW */
void GPIO::output(const string& channel, int value)
{
    try {
        ChannelInfo ch_info = _channel_to_info(channel, true);
        // check that the channel has been set as output
        if (_app_channel_configuration(ch_info) != OUT)
            throw runtime_error("The GPIO channel has not been set up as an OUTPUT");
        _output_one(ch_info.gpio, value);
    } catch (exception& e) {
        cerr << "[Exception] " << e.what() << " (caught from: output())" << endl;
        terminate();
    }
}

void GPIO::output(int channel, int value) { output(to_string(channel), value); }

template <typename T>
void GPIO::output(const std::initializer_list<T> &channels, int value)
{
    for (const auto &c : channels)
    {
        output(c, value);
    }
}

template <typename T>
void GPIO::output(const std::initializer_list<T> &channels, const std::initializer_list<int> &values)
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
Directions GPIO::gpio_function(const string& channel)
{
    try {
        ChannelInfo ch_info = _channel_to_info(channel);
        return _sysfs_channel_configuration(ch_info);
    } catch (exception& e) {
        cerr << "[Exception] " << e.what() << " (caught from: gpio_function())" << endl;
        terminate();
    }
}

Directions GPIO::gpio_function(int channel) { return gpio_function(to_string(channel)); }

//=============================== Events =================================

bool GPIO::event_detected(const std::string& channel)
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

bool GPIO::event_detected(int channel) { return event_detected(std::to_string(channel)); }

void GPIO::add_event_callback(const std::string& channel, const Callback& callback)
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

void GPIO::add_event_callback(int channel, const Callback& callback)
{
    add_event_callback(std::to_string(channel), callback);
}

void GPIO::remove_event_callback(const std::string& channel, const Callback& callback)
{
    ChannelInfo ch_info = _channel_to_info(channel, true);

    _remove_edge_callback(ch_info.gpio, callback);
}

void GPIO::remove_event_callback(int channel, const Callback& callback)
{
    remove_event_callback(std::to_string(channel), callback);
}

void GPIO::add_event_detect(const std::string& channel, Edge edge, const Callback& callback, unsigned long bounce_time)
{
    add_event_detect(std::atoi(channel.data()), edge, callback, bounce_time);
}

void GPIO::add_event_detect(int channel, Edge edge, const Callback& callback, unsigned long bounce_time)
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

void GPIO::remove_event_detect(const std::string& channel)
{
    ChannelInfo ch_info = _channel_to_info(channel, true);

    _remove_edge_detect(ch_info.gpio);
}

void GPIO::remove_event_detect(int channel) { remove_event_detect(std::to_string(channel)); }

int GPIO::wait_for_edge(const std::string& channel, Edge edge, uint64_t bounce_time, uint64_t timeout)
{
    return wait_for_edge(std::atoi(channel.data()), edge, bounce_time, timeout);
}

int GPIO::wait_for_edge(int channel, Edge edge, uint64_t bounce_time, uint64_t timeout)
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
struct GPIO::PWM::Impl {
    ChannelInfo m_ch_info;
    bool        m_started{false};
    bool        m_stop_thread{false};
    int         m_frequency_hz{0};
    double      m_duty_cycle_percent{0.0};
    double      m_basetime{0.0};
    double      m_slicetime{0.0};
    long        m_on_time{0L};
    long        m_off_time{0L};
    thread      m_thread;
    mutex       m_lock;

    Impl(int channel, int frequency_hz);
    void start();
    void stop();
    void pwm_thread();
    void _reconfigure(int frequency_hz, double duty_cycle_percent, bool start = false);
    void calculate_times();
    ~Impl();
};

GPIO::PWM::Impl::Impl(int channel, int frequency_hz):
    m_ch_info(_channel_to_info(to_string(channel), true, false))
{
    if (frequency_hz <= 0.0)
    {
        throw runtime_error("Invalid frequency");
    }

    m_frequency_hz = frequency_hz;
    m_basetime     = 1000.0/m_frequency_hz; // ms
    m_slicetime    = m_basetime/100.0;
    calculate_times();
}

void GPIO::PWM::Impl::calculate_times()
{
    // Time units in micro-seconds
    m_on_time  = static_cast<long>((m_duty_cycle_percent * m_slicetime)*1000);
    m_off_time = static_cast<long>(((100.0 - m_duty_cycle_percent) * m_slicetime)*1000);
}

void GPIO::PWM::Impl::start()
{
    unique_lock<mutex> lock(m_lock);

    if (m_started)
    {
        return;
    }

    m_thread = thread([this]{pwm_thread();});
}

void GPIO::PWM::Impl::stop()
{
    unique_lock<mutex> lock(m_lock);

    if (m_started)
    {
        m_stop_thread = true;
        m_thread.join();
    }

}

void GPIO::PWM::Impl::pwm_thread()
{
    m_started = true;

    while (m_stop_thread == false)
    {
        GPIO::output(m_ch_info.channel, GPIO::HIGH);
        this_thread::sleep_for(chrono::microseconds(m_on_time));

        GPIO::output(m_ch_info.channel, GPIO::LOW);
        this_thread::sleep_for(chrono::microseconds(m_off_time));
    }

    m_started = false;
}

void GPIO::PWM::Impl::_reconfigure(int frequency_hz, double duty_cycle_percent, bool start)
{
    if ((duty_cycle_percent < 0.0) || (duty_cycle_percent > 100.0))
    {
        throw runtime_error("invalid duty_cycle_percent");
    }

    bool freq_change = start || (m_frequency_hz != frequency_hz);
    bool stop = m_started && freq_change;

    if (stop) {
        this->stop();
    }

    if (freq_change)
    {
        m_frequency_hz = frequency_hz;
        m_basetime     = 1000.0/m_frequency_hz; // ms
    }

    m_duty_cycle_percent = duty_cycle_percent;
    calculate_times();

    if (start || stop) {
        this->start();
    }
}

GPIO::PWM::Impl::~Impl()
{
    stop();
}

GPIO::PWM::PWM(int channel, int frequency_hz)
{
    try {

        if (global._pwm_channels.find(to_string(channel)) != global._pwm_channels.end())
        {
            throw runtime_error("Channel " + to_string(channel) + " already running as PWM.");
        }
    }

    catch (exception& e) {
        cerr << "[Exception] " << e.what() << " (caught from: PWM::PWM())" << endl;
        _cleanup_all();
        terminate();
    }

    pImpl = new Impl(channel, frequency_hz);

    try {
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
    
    catch (exception& e) {
        cerr << "[Exception] " << e.what() << " (caught from: PWM::PWM())" << endl;
        _cleanup_all();
        terminate();
    }
}

GPIO::PWM::~PWM()
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
GPIO::PWM::PWM(GPIO::PWM&& other) = default;

// move assign
GPIO::PWM& GPIO::PWM::operator=(GPIO::PWM&& other)
{
    if (this == &other)
        return *this;

    pImpl = std::move(other.pImpl);
    return *this;
}

void GPIO::PWM::start(double duty_cycle_percent)
{
    try {
        pImpl->_reconfigure(pImpl->m_frequency_hz, duty_cycle_percent, true);
    } catch (exception& e) {
        cerr << "[Exception] " << e.what() << " (caught from: PWM::start())" << endl;
        _cleanup_all();
        terminate();
    }
}

void GPIO::PWM::ChangeFrequency(int frequency_hz)
{
    try {
        pImpl->_reconfigure(frequency_hz, pImpl->m_duty_cycle_percent);
    } catch (exception& e) {
        cerr << "[Exception] " << e.what() << " (caught from: PWM::ChangeFrequency())" << endl;
        terminate();
    }
}

void GPIO::PWM::ChangeDutyCycle(double duty_cycle_percent)
{
    try {
        pImpl->_reconfigure(pImpl->m_frequency_hz, duty_cycle_percent);
    } catch (exception& e) {
        cerr << "[Exception] " << e.what() << " (caught from: PWM::ChangeDutyCycle())" << endl;
        terminate();
    }
}

void GPIO::PWM::stop()
{
    try {
        pImpl->stop();

    } catch (exception& e) {
        cerr << "[Exception] " << e.what() << " (caught from: PWM::stop())" << endl;
        throw runtime_error("Exeception from GPIO::PWM::stop");
    }
}


//=======================================
// Callback
void GPIO::Callback::operator()(int input) const
{
	if (function != nullptr)
		function(input);
}


bool GPIO::operator==(const GPIO::Callback& A, const GPIO::Callback& B)
{
	return A.comparer(A.function, B.function);
}


bool GPIO::operator!=(const GPIO::Callback& A, const GPIO::Callback& B)
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

//=========================================================================
