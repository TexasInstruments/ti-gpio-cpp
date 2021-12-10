/*
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
#include <iostream>
#include <map>

// Interface headers
#include <GPIO.h>

using namespace std;

const initializer_list<int> board_pins =
{7, 8, 10, 11, 12, 13, 15, 16, 18, 19, 21, 22, 23, 24, 26, 29, 31, 32, 33,
 35, 36, 37, 38, 40};

const initializer_list<int> bcm_pins =
{4, 14, 15, 17, 18, 27, 22, 23, 24, 10, 9, 25, 11, 8, 7, 5, 6, 12, 13, 19,
 16, 26, 20, 21};

const initializer_list<string> soc_pins =
{"GPIO0_7", "GPIO0_70", "GPIO0_81", "GPIO0_71", "GPIO0_1", "GPIO0_82",
 "GPIO0_11", "GPIO0_5", "GPIO0_12", "GPIO0_101", "GPIO0_107", "GPIO0_8",
 "GPIO0_103", "GPIO0_102", "GPIO0_108", "GPIO0_93", "GPIO0_94", "GPIO0_98",
 "GPIO0_99", "GPIO0_2", "GPIO0_97", "GPIO0_115", "GPIO0_3", "GPIO0_4"};

static map<GPIO::NumberingModes, string> board_mode =
{
    {GPIO::BOARD, "GPIO::BOARD"},
    {GPIO::BCM,   "GPIO::BCM"},
    {GPIO::SOC,   "GPIO::SOC"}
};

static map<int, string> pin_status =
{
    {GPIO::HIGH, "GPIO::HIGH"},
    {GPIO::LOW,  "GPIO::LOW"}
};

template <typename T>
int run_test(GPIO::NumberingModes mode, const initializer_list<T> pins)
{
    int status = 0;

    cout << "Testing the pins in [" + board_mode[mode] + "] mode" << endl;

    for (const auto &v : {GPIO::LOW, GPIO::HIGH})
    {
        GPIO::setmode(mode);
        GPIO::setup(pins, GPIO::OUT);
        GPIO::output(pins, v);

        cout << "    Setting all pins to " + pin_status[v] << endl;
        for (auto p : pins)
        {
            GPIO::setmode(mode);
            GPIO::setup(p, GPIO::IN);
            auto value = GPIO::input(p);

            if (value != v)
            {
                cout << "******* FAILED: Pin " << p << " value check. "
                     << "Expecting " << pin_status[v] << " but got "
                     << pin_status[value] <<  " *******" << endl;
                status = -1;
            }
            else
            {
                cout << "        PASSED: Pin " << p << " value check. "
                     << "Expecting " << pin_status[v] << " and got "
                     << pin_status[value] << endl;
            }

            GPIO::cleanup();
        }
    }

    return status;
}

int main()
{
    int status = 0;

    cout << "model: " << GPIO::model << endl;
    cout << "lib version: " << GPIO::VERSION << endl;
    cout << GPIO::BOARD_INFO << endl;

    status |= run_test(GPIO::BOARD, board_pins);
    status |= run_test(GPIO::BCM, bcm_pins);
    status |= run_test(GPIO::SOC, soc_pins);

    cout << "end" << endl;
    return status;
}
