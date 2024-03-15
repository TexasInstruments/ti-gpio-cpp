/*
Copyright (c) 2012-2017 Ben Croston ben@croston.org.
Copyright (c) 2019, NVIDIA CORPORATION.
Copyright (c) 2019 Jueon Park(pjueon) bluegbg@gmail.com.
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
// for delay function.
#include <chrono>
#include <map>
#include <string>
#include <thread>
#include <vector>

// for signal handling
#include <signal.h>

// Interface headers
#include <GPIO.h>

using namespace std;
/* PWM pin details:
 *
 * J721E_SK:
 *  Pins 11,12: GPIO pins used for SW PWM.
 *  Any GPIO pins can be used for SW PWM.
 *  Pins 29,31,32,33: HW PWM pins
 *
 * AM68_SK:
 *  Pins 7,15,19,21,22,23,24,26,29,31: GPIO pins used for SW PWM.
 *  Any GPIO pins can be used for SW PWM.
 *  Pins 32,33,36: HW PWM pins
 *
 * AM69_SK:
 *  Pins 7,15,19,21,22,23,24,26,29,31: GPIO pins used for SW PWM.
 *  Any GPIO pins can be used for SW PWM.
 *  Pins 32,33,36: HW PWM pins
 */
const map<string, vector<int>> all_pwm_pins{
    { "J721E_SK", { 11, 12, 29, 31, 32, 33 } },
    { "AM68_SK", { 7, 15, 19, 21, 22, 23, 24, 26, 29, 31, 32, 33, 36 } },
    { "AM69_SK", { 7, 15, 19, 21, 22, 23, 24, 26, 29, 31, 32, 33, 36 } },
    { "AM62A_SK", { 12, 13, 15, 16, 18, 22, 29, 31, 32, 33, 35, 36, 37 } },
    { "AM62P_SK",
      { 8, 10, 11, 13, 15, 16, 18, 19, 21, 22, 23, 24, 26, 29, 31, 32, 33, 36,
        37 } },

};

vector<int> get_pwn_pins( )
{
    if( all_pwm_pins.find( GPIO::model ) == all_pwm_pins.end( ) )
    {
        cerr << "PWM not supported on this board\n";
        terminate( );
    }

    return all_pwm_pins.at( GPIO::model );
}

inline void delay( int s )
{
    this_thread::sleep_for( chrono::seconds( s ) );
}

int main( )
{
    // Pin Definitions
    vector<int> pwm_pins = get_pwn_pins( );

    GPIO::setwarnings( true );

    for( auto pin : pwm_pins )
    {
        // Pin Setup.
        // Board pin-numbering scheme
        GPIO::setmode( GPIO::BOARD );
        // set pin as an output pin with optional initial state of HIGH
        GPIO::setup( pin, GPIO::OUT, GPIO::HIGH );
        GPIO::PWM p( pin, 50 );
        auto      val = 25.0;
        p.start( val );

        cout << "Testing PWM on pin [" << pin << "]" << endl;

        delay( 1 );
        p.ChangeDutyCycle( 10 );
        delay( 1 );
        p.ChangeDutyCycle( 75 );
        delay( 1 );
        p.ChangeDutyCycle( val );

        p.stop( );
        GPIO::cleanup( );
    }

    return 0;
}
