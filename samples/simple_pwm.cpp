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

// for signal handling
#include <signal.h>

// Interface headers
#include <GPIO.h>

using namespace std;
const map<string, int> output_pins{ { "J721E_SK", 29 },
                                    { "AM68_SK", 32 },
                                    { "AM69_SK", 32 },
                                    { "AM62A_SK", 12 },
                                    { "AM62P_SK", 12 } };

int                    get_output_pin( )
{
    if( output_pins.find( GPIO::model ) == output_pins.end( ) )
    {
        cerr << "PWM not supported on this board\n";
        terminate( );
    }

    return output_pins.at( GPIO::model );
}

inline void delay( int s )
{
    this_thread::sleep_for( chrono::seconds( s ) );
}

static bool end_this_program = false;

void        signalHandler( int s )
{
    end_this_program = true;
}

int main( )
{
    // Pin Definitions
    int output_pin = get_output_pin( );

    // When CTRL+C pressed, signalHandler will be called
    signal( SIGINT, signalHandler );

    // Pin Setup.
    // Board pin-numbering scheme
    GPIO::setmode( GPIO::BOARD );

    // set pin as an output pin with optional initial state of HIGH
    GPIO::setup( output_pin, GPIO::OUT, GPIO::HIGH );
    GPIO::PWM p( output_pin, 50 );
    auto      val = 25.0;
    p.start( val );

    cout << "PWM running. Press CTRL+C to exit." << endl;

    while( !end_this_program )
    {
        delay( 1 );
        p.ChangeDutyCycle( 10 );
        delay( 1 );
        p.ChangeDutyCycle( 75 );
        delay( 1 );
        p.ChangeDutyCycle( val );
    }

    p.stop( );
    GPIO::cleanup( );

    return 0;
}
