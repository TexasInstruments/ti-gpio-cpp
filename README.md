# ti-gpio-cpp - A Linux based CPP library for TI GPIO RPi header enabled platforms
ti-gpio-cpp is an C++ port of the **TI.GPIO Python library**(https://github.com/TexasInstruments/ti-gpio-py).

TI J72e SK development board contain a 40 pin GPIO
header, similar to the 40 pin header in the Raspberry Pi. These GPIOs can be
controlled for digital input and output using the cpp library provided in the
ti-gpio-cpp Library package. The library provides almost same APIs as the TI.GPIO Python library.

This document walks through what is contained in The ti-gpio-cpp library
package, how to build, install, and run the provided sample applications,
and the library API.

# Installation
Clone this repository, build it, and install it.
```
git clone https://github.com/TexasInstruments/ti-gpio-cpp
mkdir build
cd build
cmake ..
make
make install
```
The above installs the library and header files under /usr dirctory. The headers and library will be placed as follows

- **Headers**: /usr/**include**/
- **Library**: /usr/**lib**/

The desired install location can be specified as follows

```
cmake -DCMAKE_INSTALL_PREFIX=<path/to/install> ..
make -j2
make install
```

- **Headers**: path/to/install/**include**/
- **Library**: path/to/install/**lib**/

## Cross-Compilation for the target
The library can be cross-compiled on an x86_64 machine for the target. Here are the steps for cross-compilation.
```
git clone https://github.com/TexasInstruments/ti-gpio-cpp
cd ti-gpio-cpp
# Update cmake/setup_cross_compile.sh to specify tool paths and settings
mkdir build
cd build
source ../cmake/setup_cross_compile.sh
cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/cross_compile_aarch64.cmake ..
make -j2
make install DESTDIR=<path/to/targetfs>
```
The library and headers will be placed under the following directory structire. Here we use the default
'/usr' install prefix and we prepend the tarfetfs directory location

- **Headers**: path/to/targetfs/usr/**include**/
- **Library**: path/to/targetfs/usr/**lib**/

# Library API

The library provides almost same APIs as the the TI's TI.GPIO Python library.
The following discusses the use of each API:

#### 1. Include the libary

To include the GPIO use:
```cpp
#include <GPIO.h>
```

All public APIs are declared in namespace "GPIO". If you want to make your code shorter, you can use:
```cpp
using namespace GPIO; // optional
```

To compile your program use:
```
g++ -o your_program_name your_source_code.cpp -I<path/to/install>/include -L<path/to/install>/lib -lpthread -lti_gpio
```


#### 2. Pin numbering

The TI.GPIO library provides three ways of numbering the I/O pins. The first
two correspond to the modes provided by the RPi.GPIO library, i.e BOARD and BCM
which refer to the pin number of the 40 pin GPIO header and the Broadcom SoC
GPIO numbers respectively. The last mode, SOC, uses strings instead of numbers
which correspond to the signam nameson the SOC.

To specify which mode you are using (mandatory), use the following function
call:
```cpp
GPIO::setmode(GPIO::BOARD);
// or
GPIO::setmode(GPIO::BCM);
// or
GPIO::setmode(GPIO::SOC);
```

To check which mode has be set, you can call:
```cpp
GPIO::NumberingModes mode = GPIO::getmode();
```
This function returns an instance of enum class GPIO::NumberingModes.
The mode must be one of GPIO::BOARD(GPIO::NumberingModes::BOARD),
GPIO::BCM(GPIO::NumberingModes::BCM), GPIO::BCM(GPIO::NumberingModes::SOC),
or GPIO::NumberingModes::None.

#### 3. Warnings

It is possible that the GPIO you are trying to use is already being used
external to the current application. In such a condition, the TI.GPIO
library will warn you if the GPIO being used is configured to anything but the
default direction (input). It will also warn you if you try cleaning up before
setting up the mode and channels. To disable warnings, call:
```cpp
GPIO::setwarnings(false);
```

#### 4. Set up a channel

The GPIO channel must be set up before use as input or output. To configure
the channel as input, call:
```cpp
// (where channel is based on the pin numbering mode discussed above)
GPIO::setup(channel, GPIO::IN); // channel must be int or std::string
```

To set up a channel as output, call:
```cpp
GPIO::setup(channel, GPIO::OUT);
```

It is also possible to specify an initial value for the output channel:
```cpp
GPIO::setup(channel, GPIO::OUT, GPIO::HIGH);
```

When setting up a channel as output, it is also possible to set up more than one
channel at once:
```cpp
# add as many as channels as needed. You can also use tuples: (18,12,13)
std::initializer_list<int> channels = {18, 12, 13};
GPIO::setup(channels, GPIO::OUT);
```

#### 5. Input

To read the value of a channel, use:

```cpp
int value = GPIO::input(channel);
```

This will return either GPIO::LOW(== 0) or GPIO::HIGH(== 1).

#### 6. Output

To set the value of a pin configured as output, use:

```cpp
GPIO::output(channel, state);
```

where state can be GPIO::LOW(== 0) or GPIO::HIGH(== 1).

```cpp
std::initializer_list<int> channels = {18, 12, 13};
GPIO::setup(channels, GPIO::OUT);
// set all channel to HIGH
GPIO.output(channels, GPIO.HIGH);
// set first channel to HIGH and rest to LOW
std::initializer_list<int> values = {GPIO::HIGH, GPIO::LOW, GPIO::LOW};
GPIO.output(channels, values);
```


#### 7. Clean up

At the end of the program, it is good to clean up the PWM and event detected GPIO channels so that all pins
are set in their default state. To clean up all PWM channels used, call:

```cpp
GPIO::cleanup();
```

If you don't want to clean all channels, it is also possible to clean up
individual channels:

```cpp
GPIO::cleanup(chan1); // cleanup only chan1
```

#### 8. Board Information and library version

To get information about the module, use/read:

```cpp
std::string info = GPIO::BOARD_INFO;
```

To get the model name of your device, use/read:

```cpp
std::string model = GPIO::model;
```

To get information about the library version, use/read:

```cpp
std::string version = GPIO::VERSION;
```

This provides a string with the X.Y.Z version format.

#### 9. Interrupts

Aside from busy-polling, the library provides three additional ways of monitoring an input event:

__The wait_for_edge() function__

This function blocks the calling thread until the provided edge(s) is detected. The function can be called as follows:

```cpp
GPIO::wait_for_edge(channel, GPIO::RISING);
```
The second parameter specifies the edge to be detected and can be GPIO::RISING, GPIO::FALLING or GPIO::BOTH. If you only want to limit the wait to a specified amount of time, a timeout can be optionally set:

```cpp
// timeout is in milliseconds__
// debounce_time set to 10ms
GPIO::wait_for_edge(channel, GPIO::RISING, 10, 500);
```

The function returns the channel for which the edge was detected or 0 if a timeout occurred.


__The event_detected() function__

This function can be used to periodically check if an event occurred since the last call. The function can be set up and called as follows:

```cpp
// set rising edge detection on the channel
GPIO::add_event_detect(channel, GPIO::RISING);
run_other_code();
if(GPIO::event_detected(channel))
    do_something();
```

As before, you can detect events for GPIO::RISING, GPIO::FALLING or GPIO::BOTH.

__A callback function run when an edge is detected__

This feature can be used to run a second thread for callback functions. Hence, the callback function can be run concurrent to your main program in response to an edge. This feature can be used as follows:

```cpp
// define callback function
void callback_fn(int channel) {
    std::cout << "Callback called from channel " << channel << std::endl;
}

// add rising edge detection
GPIO::add_event_detect(channel, GPIO::RISING, callback_fn);
```

Any object that satisfies the following requirements can be used as callback functions.

- Callable (argument type: int, return type: void)
- Copy-constructible
- Equality-comparable with same type (ex> func0 == func1)

Here is a user-defined type callback example:

```cpp
// define callback object
class MyCallback
{
public:
    MyCallback(const std::string& name) : name(name) {}
    MyCallback(const MyCallback&) = default; // Copy-constructible

    void operator()(int channel) // Callable
    {
        std::cout << "A callback named " << name;
        std::cout << " called from channel " << channel << std::endl;
    }

    bool operator==(const MyCallback& other) const // Equality-comparable
    {
        return name == other.name;
    }

private:
    std::string name;
};

// create callback object
MyCallback my_callback("foo");
// add rising edge detection
GPIO::add_event_detect(channel, GPIO::RISING, my_callback);
```


More than one callback can also be added if required as follows:

```cpp
void callback_one(int channel) {
    std::cout << "First Callback" << std::endl;
}

void callback_two(int channel) {
    std::cout << "Second Callback" << std::endl;
}

GPIO::add_event_detect(channel, GPIO::RISING);
GPIO::add_event_callback(channel, callback_one);
GPIO::add_event_callback(channel, callback_two);
```

The two callbacks in this case are run sequentially, not concurrently since there is only one event thread running all callback functions.

In order to prevent multiple calls to the callback functions by collapsing multiple events in to a single one, a debounce time can be optionally set:

```cpp
// bouncetime set in milliseconds
GPIO::add_event_detect(channel, GPIO::RISING, callback_fn, 200);
```

If one of the callbacks are no longer required it may then be removed:

```cpp
GPIO::remove_event_callback(channel, callback_two);
```

Similarly, if the edge detection is no longer required it can be removed as follows:

```cpp
GPIO::remove_event_detect(channel);
```

#### 10. Check function of GPIO channels

This feature allows you to check the function of the provided GPIO channel:

```cpp
GPIO::Directions direction = GPIO::gpio_function(channel);
```

The function returns either GPIO::IN(GPIO::Directions::IN) or GPIO::OUT(GPIO::Directions::OUT) which are the instances of enum class GPIO::Directions.

#### 11. PWM

See `samples/simple_pwm.cpp` for details on how to use PWM channels.


# Documentation

Documentation on the TI starter kit can be found at the following link:

* [SK-TDA4VM](https://www.ti.com/tool/SK-TDA4VM);
For the SW package search for "Software Development Kit for DRA829 & TDA4VM Jacinto™ processors"


A good FAQ on handling Pinmux changes can be found at the following link

* [TDA4VM Pinmux guide](https://e2e.ti.com/support/processors-group/processors/f/processors-forum/927526/faq-ccs-tda4vm-pinmux-guide-for-jacinto-processors).
