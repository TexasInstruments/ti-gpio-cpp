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
#include <chrono>
#include <iostream>
#include <signal.h>
#include <thread>

// Interface headers
#include <GPIO.h>

using namespace std;

static bool end_this_program = false;

inline void delay( int s )
{
    this_thread::sleep_for( chrono::seconds( s ) );
}

void signalHandler( int s )
{
    end_this_program = true;
}

int main( )
{
    // When CTRL+C pressed, signalHandler will be called
    signal( SIGINT, signalHandler );

    // Pin Definitions
    int led_pin = 11;  // BOARD pin 11
    int but_pin = 18; // BOARD pin 18

    // Pin Setup.
    GPIO::setmode( GPIO::BOARD );

    // set pin as an output pin with optional initial state of HIGH
    GPIO::setup( led_pin, GPIO::OUT, GPIO::LOW );
    GPIO::setup( but_pin, GPIO::IN );

    cout << "Starting demo now! Press CTRL+C to exit" << endl;

    while( !end_this_program )
    {
        cout << "Waiting for button event" << endl;
        GPIO::wait_for_edge( but_pin, GPIO::Edge::FALLING );

        // event received when button pressed
        cout << "Button Pressed!" << endl;
        GPIO::output( led_pin, GPIO::HIGH );
        delay( 1 );
        GPIO::output( led_pin, GPIO::LOW );
    }

    return 0;
}
