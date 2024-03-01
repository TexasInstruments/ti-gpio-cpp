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

#pragma once
#ifndef _GPIO_H
#define _GPIO_H

//standard headers
#include <memory> // for pImpl
#include <map>
#include <set>

// library headers
#include <gpiod.h>

namespace GPIO
{
    constexpr auto VERSION = "2.0.0";

    // Pin Numbering Modes
    enum class NumberingModes { BOARD, BCM, SOC, None };

    // GPIO::BOARD, GPIO::BCM, GPIO::SOC
    constexpr NumberingModes BOARD = NumberingModes::BOARD;
    constexpr NumberingModes BCM = NumberingModes::BCM;
    constexpr NumberingModes SOC = NumberingModes::SOC;

    /*
    Pull up/down options are removed because they are unused in
    TI's original python libarary.
    */
    constexpr int HIGH = 1;
    constexpr int LOW = 0;

    /*
    GPIO directions.
    UNKNOWN constant is for gpios that are not yet setup
    If the user uses UNKNOWN or HARD_PWM as a parameter to
    GPIO::setmode function, An exception will occur
    */
    enum class Directions { UNKNOWN, OUT, IN, HARD_PWM };

    // GPIO::IN, GPIO::OUT
    constexpr Directions IN = Directions::IN;
    constexpr Directions OUT = Directions::OUT;

    // GPIO Event Types
    enum class Edge { UNKNOWN, NONE, RISING, FALLING, BOTH };

    constexpr Edge NO_EDGE = Edge::NONE;
    constexpr Edge RISING = Edge::RISING;
    constexpr Edge FALLING = Edge::FALLING;
    constexpr Edge BOTH = Edge::BOTH;

    /*
    Function used to set the pin mumbering mode.
    Possible mode values are BOARD, BCM, and SOC
    */
    void setmode(NumberingModes mode);

    // Function used to get the currently set pin numbering mode
    NumberingModes getmode();

    /*
    Function used to setup individual pins as Input or Output.
    direction must be IN or OUT, initial must be
    HIGH or LOW and is only valid when direction is OUT
    */
    void setup(const std::string& channel, Directions direction, int initial = -1);
    void setup(int channel, Directions direction, int initial = -1);

    /*
    Function used to cleanup channels at the end of the program.
    If no channel is provided, all channels are cleaned
    */
    void cleanup();

    /*
    Function used to return the current value of the specified channel.
    Function returns either HIGH or LOW
    */
    int input(int channel);
    int input(const std::string& channel);

    /*
    Function used to set a value to a channel.
    Values must be either HIGH or LOW
    */
    void output(int channel, int value);
    void output(const std::string& channel, int value);

    /*
    Function used to check the currently set function
    of the channel specified.
    */
    Directions gpio_function(int channel);
    Directions gpio_function(const std::string& channel);

    //--------------EVENTS--------------------------------

    /*
    Function used to add threaded event detection for a specified gpio channel.
    @gpio must be an integer specifying the channel
    @edge must be a member of GPIO::Edge
    @callback (optional) may be a callback function to be called when the event is detected (or nullptr)
    @bouncetime (optional) a button-bounce signal ignore time (in milliseconds, default=none)
    */

    void add_event_detect(const std::string& channel, Edge edge, const Callback& callback = nullptr,
                        unsigned long bounce_time = 0);
    void add_event_detect(int channel, Edge edge, const Callback& callback = nullptr, unsigned long bounce_time = 0);

    /* Function used to remove event detection for channel */
    void remove_event_detect(const std::string& channel);
    void remove_event_detect(int channel);


    //--------------PWM---------------------------------------
    class GpioPwmIf;
    class PWM
    {
        public:
            PWM(int channel, int frequency_hz);
            PWM(PWM&& other);
            PWM& operator=(PWM&& other);
            PWM(const PWM&) = delete;            // Can't create duplicate PWM objects
            PWM& operator=(const PWM&) = delete; // Can't create duplicate PWM objects
            ~PWM();
            void start(double duty_cycle_percent);
            void stop();
            void ChangeFrequency(int frequency_hz);
            void ChangeDutyCycle(double duty_cycle_percent);

        private:
            GpioPwmIf *pImpl{nullptr};
    };

} // namespace GPIO

#endif // _GPIO_H
