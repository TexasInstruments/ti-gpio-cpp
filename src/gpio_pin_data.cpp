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
#include <cctype>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <algorithm>
#include <iterator>

// Interface headers
#include <GPIO.h>

// Local headers
#include "gpio_pin_data.h"
#include "python_functions.h"

using namespace std;
using namespace std::string_literals; // enables s-suffix for std::string
                                      // literals

namespace GPIO
{
    /*
    Global variables are wrapped in singleton pattern in order to avoid
    initialization order of global variables in different compilation units
    problem
    */

    class EntirePinData
    {
      private:
        EntirePinData( );

      public:
        /*
        These vectors contain all the relevant GPIO data for each Platform.
        The values are use to generate dictionaries that map the corresponding
        pin mode numbers to the Linux GPIO pin number and GPIO chip directory
        */

        const vector<PinDefinition>             J721E_SK_PIN_DEFS;
        const vector<string>                    compats_j721e;

        const vector<PinDefinition>             AM68_SK_PIN_DEFS;
        const vector<string>                    compats_am68sk;

        const vector<PinDefinition>             AM69_SK_PIN_DEFS;
        const vector<string>                    compats_am69sk;

        const vector<PinDefinition>             AM62A_SK_PIN_DEFS;
        const vector<string>                    compats_am62ask;

        const vector<PinDefinition>             AM62P_SK_PIN_DEFS;
        const vector<string>                    compats_am62psk;

        const map<Model, vector<PinDefinition>> PIN_DEFS_MAP;
        const map<Model, PinInfo>               DEVICE_INFO_MAP;

        EntirePinData( const EntirePinData & )            = delete;
        EntirePinData &operator=( const EntirePinData & ) = delete;
        ~EntirePinData( )                                 = default;

        static EntirePinData &get_instance( )
        {
            static EntirePinData singleton{ };
            return singleton;
        }
    };

    EntirePinData::EntirePinData()
    :
    J721E_SK_PIN_DEFS
    {
    //  GPIOCHIP_X  OFFSET Sysfs_dir   BOARD   BCM  SOC_NAME   PWM_SysFs     PWM_Id
        {1, 84, "600000.gpio",  "3",  "2", "GPIO0_84",  "None",        -1},
        {1, 83, "600000.gpio",  "5",  "3", "GPIO0_83",  "None",        -1},
        {1, 7, "600000.gpio",  "7",  "4", "GPIO0_7",   "None",        -1},
        {1, 70, "600000.gpio",  "8", "14", "GPIO0_70",  "None",        -1},
        {1, 81, "600000.gpio", "10", "15", "GPIO0_81",  "None",        -1},
        {1, 71, "600000.gpio", "11", "17", "GPIO0_71",  "None",        -1},
        {1, 1, "600000.gpio", "12", "18", "GPIO0_1",   "None",        -1},
        {1, 82, "600000.gpio", "13", "27", "GPIO0_82",  "None",        -1},
        {1, 11, "600000.gpio", "15", "22", "GPIO0_11",  "None",        -1},
        {1, 5, "600000.gpio", "16", "23", "GPIO0_5",   "None",        -1},
        {2, 12, "601000.gpio", "18", "24", "GPIO0_12",  "None",        -1},
        {1, 101, "600000.gpio", "19", "10", "GPIO0_101", "None",        -1},
        {1, 107, "600000.gpio", "21",  "9", "GPIO0_107", "None",        -1},
        {1, 8, "600000.gpio", "22", "25", "GPIO0_8",   "None",        -1},
        {1, 103, "600000.gpio", "23", "11", "GPIO0_103", "None",        -1},
        {1, 102, "600000.gpio", "24",  "8", "GPIO0_102", "None",        -1},
        {1, 108, "600000.gpio", "26",  "7", "GPIO0_108", "None",        -1},
        {1, 93, "600000.gpio", "29",  "5", "GPIO0_93",  "3020000.pwm",  0},
        {1, 94, "600000.gpio", "31",  "6", "GPIO0_94",  "3020000.pwm",  1},
        {1, 98, "600000.gpio", "32", "12", "GPIO0_98",  "3030000.pwm",  0},
        {1, 99, "600000.gpio", "33", "13", "GPIO0_99",  "3030000.pwm",  1},
        {1, 2, "600000.gpio", "35", "19", "GPIO0_2",   "None",        -1},
        {1, 97, "600000.gpio", "36", "16", "GPIO0_97",  "None",        -1},
        {1, 115, "600000.gpio", "37", "26", "GPIO0_115", "None",        -1},
        {1, 3, "600000.gpio", "38", "20", "GPIO0_3",   "None",        -1},
        {1, 4, "600000.gpio", "40", "21", "GPIO0_4",   "None",        -1}
    },
    compats_j721e
    {
        "ti,j721e-eaikti",
        "ti,j721e"
    },
    AM68_SK_PIN_DEFS
    {
    //  GPIOCHIP_X  OFFSET Sysfs_dir      BOARD   BCM  SOC_NAME      PWM_SysFs PWM_Id
        {4, 4, "600000.gpio",    "3",  "2", "GPIO0_4",       "None", -1},
        {4, 5, "600000.gpio",    "5",  "3", "GPIO0_5",       "None", -1},
        {3, 66, "42110000.gpio",  "7",  "4", "WKUP_GPIO0_66", "None", -1},
        {4, 1, "600000.gpio",    "8", "14", "GPIO0_1",       "None", -1},
        {4, 2, "600000.gpio",   "10", "15", "GPIO0_2",       "None", -1},
        {4, 42, "600000.gpio",   "11", "17", "GPIO0_42",      "None", -1},
        {4, 46, "600000.gpio",   "12", "18", "GPIO0_46",      "None", -1},
        {4, 36, "600000.gpio",   "13", "27", "GPIO0_36",      "None", -1},
        {3, 49, "42110000.gpio", "15", "22", "WKUP_GPIO0_49", "None", -1},
        {4, 3, "600000.gpio",   "16", "23", "GPIO0_3",       "None", -1},
        {4, 13, "600000.gpio",   "18", "24", "GPIO0_13",      "None", -1},
        {3, 1, "42110000.gpio", "19", "10", "WKUP_GPIO0_1",  "None", -1},
        {3, 2, "42110000.gpio", "21",  "9", "WKUP_GPIO0_2",  "None", -1},
        {3, 67, "42110000.gpio", "22", "25", "WKUP_GPIO0_67", "None", -1},
        {3, 0, "42110000.gpio", "23", "11", "WKUP_GPIO0_0",  "None", -1},
        {3, 3, "42110000.gpio", "24",  "8", "WKUP_GPIO0_3",  "None", -1},
        {3, 15, "42110000.gpio", "26",  "7", "WKUP_GPIO0_15", "None", -1},
        {3, 56, "42110000.gpio", "29",  "5", "WKUP_GPIO0_56", "None", -1},
        {3, 57, "42110000.gpio", "31",  "6", "WKUP_GPIO0_57", "None", -1},
        {4, 35, "600000.gpio",   "32", "12", "GPIO0_35",      "3030000.pwm", 0},
        {4, 51, "600000.gpio",   "33", "13", "GPIO0_51",      "3000000.pwm", 0},
        {4, 47, "600000.gpio",   "35", "19", "GPIO0_47",      "None", -1},
        {4, 41, "600000.gpio",   "36", "16", "GPIO0_41",      "3040000.pwm", 0},
        {4, 27, "600000.gpio",   "37", "26", "GPIO0_27",      "None", -1},
        {4, 48, "600000.gpio",   "38", "20", "GPIO0_48",      "None", -1},
        {4, 45, "600000.gpio",   "40", "21", "GPIO0_45",      "None", -1}
    },
    compats_am68sk
    {
        "ti,am68-sk",
        "ti,j721s2"
    },
    AM69_SK_PIN_DEFS
    {
    //  GPIOCHIP_X  OFFSET  Sysfs_dir  BOARD   BCM   SOC_NAME   PWM_SysFs PWM_Id
        {2, 87, "42110000.gpio",  "3",  "2", "WKUP_GPIO0_87", "None", -1},
        {3, 65, "600000.gpio",    "5",  "3", "WKUP_GPIO0_65", "None", -1},
        {2, 66, "42110000.gpio",  "7",  "4", "WKUP_GPIO0_66", "None", -1},
        {3, 1,  "600000.gpio",    "8", "14", "GPIO0_1",       "None", -1},
        {3, 2,  "600000.gpio",   "10", "15", "GPIO0_2",       "None", -1},
        {3, 42, "600000.gpio",   "11", "17", "GPIO0_42",      "None", -1},
        {3, 46, "600000.gpio",   "12", "18", "GPIO0_46",      "None", -1},
        {3, 36, "600000.gpio",   "13", "27", "GPIO0_36",      "None", -1},
        {2, 49, "42110000.gpio", "15", "22", "WKUP_GPIO0_49", "None", -1},
        {3, 3,  "600000.gpio",   "16", "23", "GPIO0_3",       "None", -1},
        {3, 13, "600000.gpio",   "18", "24", "GPIO0_13",      "None", -1},
        {2, 1,  "42110000.gpio", "19", "10", "WKUP_GPIO0_1",  "None", -1},
        {2, 2,  "42110000.gpio", "21",  "9", "WKUP_GPIO0_2",  "None", -1},
        {2, 67, "42110000.gpio", "22", "25", "WKUP_GPIO0_67", "None", -1},
        {2, 0,  "42110000.gpio", "23", "11", "WKUP_GPIO0_0",  "None", -1},
        {2, 3,  "42110000.gpio", "24",  "8", "WKUP_GPIO0_3",  "None", -1},
        {2, 15, "42110000.gpio", "26",  "7", "WKUP_GPIO0_15", "None", -1},
        {2, 56, "42110000.gpio", "29",  "5", "WKUP_GPIO0_56", "None", -1},
        {2, 57, "42110000.gpio", "31",  "6", "WKUP_GPIO0_57", "None", -1},
        {3, 35, "600000.gpio",   "32", "12", "GPIO0_35",      "3030000.pwm", 0},
        {3, 51, "600000.gpio",   "33", "13", "GPIO0_51",      "3000000.pwm", 0},
        {3, 47, "600000.gpio",   "35", "19", "GPIO0_47",      "None", -1},
        {3, 41, "600000.gpio",   "36", "16", "GPIO0_41",      "3040000.pwm", 0},
        {3, 27, "600000.gpio",   "37", "26", "GPIO0_27",      "None", -1},
        {3, 48, "600000.gpio",   "38", "20", "GPIO0_48",      "None", -1},
        {3, 45, "600000.gpio",   "40", "21", "GPIO0_45",      "None", -1}
    },
    compats_am69sk
    {
        "ti,am69-sk",
        "ti,j784s4"
    },
    AM62A_SK_PIN_DEFS
    {
    //  GPIOCHIP_X  OFFSET Sysfs_dir     BOARD   BCM   SOC_NAME      PWM_SysFs PWM_Id
        {2, 44, "600000.gpio",  "3",  "2", "I2C2_SDA", "None", -1},
        {2, 43, "600000.gpio",  "5",  "3", "I2C2_SCL", "None", -1},
        {3, 30, "601000.gpio",  "7",  "4", "GPIO1_30", "None", -1},
        {3, 25, "601000.gpio",  "8", "14", "GPIO1_25", "None", -1},
        {3, 24, "601000.gpio",  "10", "15", "GPIO1_24", "None", -1},
        {3, 11, "601000.gpio",  "11", "17", "GPIO1_11", "None", -1},
        {3, 14, "601000.gpio",  "12", "18", "GPIO1_14", "23000000.pwm", 1},
        {2, 42, "600000.gpio",  "13", "27", "GPIO0_42", "None", -1},
        {3, 22, "601000.gpio",  "15", "22", "GPIO1_22", "None", -1},
        {2, 38, "600000.gpio",  "16", "23", "GPIO0_38", "None", -1},
        {2, 39, "600000.gpio",  "18", "24", "GPIO0_39", "None", -1},
        {3, 18, "601000.gpio",  "19", "10", "GPIO1_18",  "None", -1},
        {3, 19, "601000.gpio",  "21",  "9", "GPIO1_19",  "None", -1},
        {2, 14, "600000.gpio",  "22", "25", "GPIO0_14", "None", -1},
        {3, 17, "601000.gpio",  "23", "11", "GPIO1_17",  "None", -1},
        {3, 15, "601000.gpio",  "24",  "8", "GPIO1_15",  "None", -1},
        {3, 16, "601000.gpio",  "26",  "7", "GPIO1_16", "None", -1},
        {2, 36, "600000.gpio",  "29",  "5", "GPIO0_36", "None", -1},
        {2, 33, "600000.gpio",  "31",  "6", "GPIO0_33", "None", -1},
        {2, 40, "600000.gpio",  "32", "12", "GPIO0_40", "None", -1},
        {3, 10, "601000.gpio",  "33", "13", "GPIO1_10", "23010000.pwm", 1},
        {3, 13, "601000.gpio",  "35", "19", "GPIO1_13", "23000000.pwm", 0},
        {3, 9, "601000.gpio",   "36", "16", "GPIO1_09",  "23010000.pwm", 0},
        {2, 41, "600000.gpio",  "37", "26", "GPIO0_41", "None", -1},
        {3, 8, "601000.gpio",   "38", "20", "GPIO1_08", "None", -1},
        {3, 7, "601000.gpio",   "40", "21", "GPIO1_07", "None", -1}
    },
    compats_am62ask
    {
        "ti,am62a7-sk",
        "ti,am62a7"
    },
    AM62P_SK_PIN_DEFS
    {
    //  GPIOCHIP_X  OFFSET  Sysfs_dir     BOARD   BCM   SOC_NAME      PWM_SysFs PWM_Id
        {1, 44, "600000.gpio",  "3",  "2", "I2C2_SDA", "None", -1},
        {1, 43, "600000.gpio",  "5",  "3", "I2C2_SCL", "None", -1},
        {2, 30, "601000.gpio",  "7",  "4", "GPIO1_30", "None", -1},
        {2, 25, "601000.gpio",  "8", "14", "GPIO1_25", "None", -1},
        {2, 24, "601000.gpio",  "10", "15", "GPIO1_24", "None", -1},
        {2, 11, "601000.gpio",  "11", "17", "GPIO1_11", "None", -1},
        {2, 14, "601000.gpio",  "12", "18", "GPIO1_14", "23000000.pwm", 1},
        {1, 42, "600000.gpio",  "13", "27", "GPIO0_42", "None", -1},
        {2, 22, "601000.gpio",  "15", "22", "GPIO1_22", "None", -1},
        {1, 38, "600000.gpio",  "16", "23", "GPIO0_38", "None", -1},
        {1, 39, "600000.gpio",  "18", "24", "GPIO0_39", "None", -1},
        {2, 18, "601000.gpio",  "19", "10", "GPIO1_18",  "None", -1},
        {2, 19, "601000.gpio",  "21",  "9", "GPIO1_19",  "None", -1},
        {1, 14, "600000.gpio",  "22", "25", "GPIO0_14", "None", -1},
        {2, 17, "601000.gpio",  "23", "11", "GPIO1_17",  "None", -1},
        {2, 15, "601000.gpio",  "24",  "8", "GPIO1_15",  "None", -1},
        {2, 16, "601000.gpio",  "26",  "7", "GPIO1_16", "None", -1},
        {1, 36, "600000.gpio",  "29",  "5", "GPIO0_36", "None", -1},
        {1, 33, "600000.gpio",  "31",  "6", "GPIO0_33", "None", -1},
        {1, 40, "600000.gpio",  "32", "12", "GPIO0_40", "None", -1},
        {2, 10, "601000.gpio",  "33", "13", "GPIO1_10", "23010000.pwm", 1},
        {2, 13, "601000.gpio",  "35", "19", "GPIO1_13", "23000000.pwm", 0},
        {2, 9, "601000.gpio",   "36", "16", "GPIO1_09",  "23010000.pwm", 0},
        {1, 41, "600000.gpio",  "37", "26", "GPIO0_41", "None", -1},
        {2, 8, "601000.gpio",   "38", "20", "GPIO1_08", "None", -1},
        {2, 7, "601000.gpio",   "40", "21", "GPIO1_07", "None", -1}
    },
    compats_am62psk
    {
        "ti,am62p5-sk",
        "ti,am62p5"
    },
    PIN_DEFS_MAP
    {
        { J721E_SK, J721E_SK_PIN_DEFS },
        { AM68_SK,  AM68_SK_PIN_DEFS },
        { AM69_SK,  AM69_SK_PIN_DEFS },
        { AM62A_SK, AM62A_SK_PIN_DEFS },
        { AM62P_SK, AM62P_SK_PIN_DEFS }
    },
    DEVICE_INFO_MAP
    {
        { J721E_SK, {1, "8192M", "Unknown", "J721e SK", "TI", "ARM A72"} },
        { AM68_SK,  {1, "8192M", "Unknown", "AM68 SK", "TI", "ARM A72"} },
        { AM69_SK,  {1, "8192M", "Unknown", "AM69 SK", "TI", "ARM A72"} },
        { AM62A_SK,  {1, "8192M", "Unknown", "AM62A SK", "TI", "ARM A53"} },
        { AM62P_SK,  {1, "8192M", "Unknown", "AM62P SK", "TI", "ARM A53"} }
    }
{};

    PinData get_data( )
    {
        try
        {
            EntirePinData &_DATA           = EntirePinData::get_instance( );

            const string   compatible_path = "/proc/device-tree/compatible";

            set<string>    compatibles{ };

            { // scope for f:
                ifstream     f( compatible_path );
                stringstream buffer{ };

                buffer << f.rdbuf( );
                string         tmp_str = buffer.str( );
                vector<string> _vec_compatibles( split( tmp_str, '\x00' ) );
                // convert to std::set
                copy( _vec_compatibles.begin( ), _vec_compatibles.end( ),
                      inserter( compatibles, compatibles.end( ) ) );
            } // scope ends

            auto matches = [&compatibles]( const vector<string> &vals ) {
                for( const auto &v : vals )
                {
                    if( is_in( v, compatibles ) )
                    {
                        return true;
                    }
                }
                return false;
            };

            Model model{ };

            if( matches( _DATA.compats_j721e ) )
            {
                model = J721E_SK;
            }
            else if( matches( _DATA.compats_am68sk ) )
            {
                model = AM68_SK;
            }
            else if( matches( _DATA.compats_am69sk ) )
            {
                model = AM69_SK;
            }
            else if( matches( _DATA.compats_am62ask ) )
            {
                model = AM62A_SK;
            }
            else if( matches( _DATA.compats_am62psk ) )
            {
                model = AM62P_SK;
            }

            else
            {
                throw runtime_error( "Could not determine SOC model" );
            }

            vector<PinDefinition> pin_defs = _DATA.PIN_DEFS_MAP.at( model );
            PinInfo             board_info = _DATA.DEVICE_INFO_MAP.at( model );

            map<string, string> pwm_dirs{ };

            vector<string>      sysfs_prefixes = {
                "/sys/devices/", "/sys/devices/platform/",
                "/sys/devices/platform/bus@100000/",
                "/sys/devices/platform/bus@100000/bus@100000:bus@28380000/",
                "/sys/devices/platform/bus@f0000/" };

            set<string> pwm_chip_names{ };
            for( const auto &x : pin_defs )
            {
                if( !is_None( x.PWMSysfsDir ) )
                {
                    pwm_chip_names.insert( x.PWMSysfsDir );
                }
            }

            for( const auto &pwm_chip_name : pwm_chip_names )
            {

                string pwm_chip_dir = "None";
                for( const auto &prefix : sysfs_prefixes )
                {
                    auto d = prefix + pwm_chip_name;
                    if( os_path_isdir( d ) )
                    {
                        pwm_chip_dir = d;
                        break;
                    }
                }

                /*
                Some PWM controllers aren't enabled in all versions of the DT.
                In this case, just hide the PWM function on this pin, but let
                all other aspects of the library continue to work.
                */

                if( is_None( pwm_chip_dir ) )
                {
                    continue;
                }

                auto chip_pwm_dir = pwm_chip_dir + "/pwm";
                if( !os_path_exists( chip_pwm_dir ) )
                {
                    continue;
                }

                for( const auto &fn : os_listdir( chip_pwm_dir ) )
                {
                    if( !startswith( fn, "pwmchip" ) )
                    {
                        continue;
                    }

                    string chip_pwm_pwmchip_dir = chip_pwm_dir + "/" + fn;
                    pwm_dirs[pwm_chip_name]     = chip_pwm_pwmchip_dir;
                    break;
                }
            }

            auto model_data = [&pwm_dirs]( NumberingModes key,
                                           const auto    &pin_defs ) {
                auto get_or = []( const auto &dictionary, const string &x,
                                  const string &defaultValue ) -> string {
                    return is_in( x, dictionary ) ? dictionary.at( x )
                                                  : defaultValue;
                };

                map<string, ChannelInfo> ret{ };

                for( const auto &x : pin_defs )
                {

                    string pinName = x.PinName( key );
                    ret.insert(
                        { pinName, ChannelInfo{ pinName, x.gpiochip, x.LinuxPin,
                                                get_or( pwm_dirs, x.PWMSysfsDir,
                                                        "None" ),
                                                x.PWMID } } );
                }
                return ret;
            };

            map<NumberingModes, map<string, ChannelInfo>> channel_data = {
                { BOARD, model_data( BOARD, pin_defs ) },
                { BCM, model_data( BCM, pin_defs ) },
                { SOC, model_data( SOC, pin_defs ) } };

            return { model, board_info, channel_data };
        }
        catch( exception &e )
        {
            cerr << "[Exception] " << e.what( ) << " (catched from: get_data())"
                 << endl;
            throw false;
        }
    }

} // namespace GPIO
