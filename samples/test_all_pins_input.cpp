/*
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
#include <iostream>
#include <map>

// Interface headers
#include <GPIO.h>

using namespace std;

struct TestPinData
{
    initializer_list<int>       board_pins;
    initializer_list<int>       bcm_pins;
    initializer_list<string>    soc_pins;
};

const TestPinData j721e_pin_data =
{
    {7,8,10,11,12,13,15,16,18,19,21,22,23,24,26,35,36,37,38,40},
    {4,14,15,17,18,27,22,23,24,10,9,25,11,8,7,19,16,26,20,21},
    {"GPIO0_7","GPIO0_70","GPIO0_81","GPIO0_71","GPIO0_1","GPIO0_82",
     "GPIO0_11","GPIO0_5","GPIO0_12","GPIO0_101","GPIO0_107","GPIO0_8",
     "GPIO0_103","GPIO0_102","GPIO0_108","GPIO0_2","GPIO0_97","GPIO0_115",
     "GPIO0_3","GPIO0_4"
    }
};

const TestPinData am68a_pin_data =
{
    {8,10,11,12,13,16,18,35,37,38,40},
    {14,15,17,18,27,23,24,19,26,20,21},
    {"GPIO0_1","GPIO0_2","GPIO0_42","GPIO0_46","GPIO0_36","GPIO0_3",
     "GPIO0_13","GPIO0_47","GPIO0_27","GPIO0_48","GPIO0_45"
    }
};

const TestPinData am69a_pin_data =
{
    {8,10,11,12,13,16,18,35,37,38,40},
    {14,15,17,18,27,23,24,19,26,20,21},
    {"GPIO0_1","GPIO0_2","GPIO0_42","GPIO0_46","GPIO0_36","GPIO0_3",
     "GPIO0_13","GPIO0_47","GPIO0_27","GPIO0_48","GPIO0_45"
    }
};

const TestPinData am62a_pin_data =
{
    {13,15,16,18,29,31,32,37},
    {27,22,23,24,5,6,12,26},
    {"GPIO0_42","GPIO1_22","GPIO0_38","GPIO0_39",
     "GPIO0_36","GPIO0_33","GPIO0_40","GPIO0_41"
    }
};

const TestPinData am62p_pin_data =
{
    {8,10,11,13,15,16,18,19,21,22,23,29,31,32,37},
    {14,15,17,27,22,23,24,10,9,25,11,5,6,12,26},
    {"GPIO1_25","GPIO1_24","GPIO1_11","GPIO0_42","GPIO1_22",
     "GPIO0_38","GPIO0_39","GPIO1_18","GPIO1_19","GPIO0_14",
     "GPIO1_17","GPIO0_36","GPIO0_33","GPIO0_40","GPIO0_41"
    }
};

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

const map<string, const TestPinData*> gPin_data =
{
    {"J721E_SK", &j721e_pin_data},
    {"AM68_SK",  &am68a_pin_data},
    {"AM69_SK",  &am69a_pin_data},
    {"AM62A_SK", &am62a_pin_data},
    {"AM62P_SK", &am62p_pin_data}
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

const auto get_pin_data(const map<string, const TestPinData*> &pinData)
{
	if (pinData.find(GPIO::model) == pinData.end())
	{
		cerr << "Not supported on this board\n";
		terminate();
	}

	return pinData.at(GPIO::model);
}

int main()
{
    int status = 0;

    cout << "model: " << GPIO::model << endl;
    cout << "lib version: " << GPIO::VERSION << endl;
    cout << GPIO::BOARD_INFO << endl;

    const auto &pins = get_pin_data(gPin_data);

    status |= run_test(GPIO::BOARD, pins->board_pins);
    status |= run_test(GPIO::BCM,   pins->bcm_pins);
    status |= run_test(GPIO::SOC,   pins->soc_pins);

    cout << "end" << endl;
    return status;
}
