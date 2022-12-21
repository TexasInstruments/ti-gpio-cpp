/*
Copyright (c) 2012-2017 Ben Croston ben@croston.org.
Copyright (c) 2019, NVIDIA CORPORATION.
Copyright (c) 2019 Jueon Park(pjueon) bluegbg@gmail.com.
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
#include <string>
#include <stdexcept>
#include <vector>
#include <map>
#include <set>
#include <fstream>
#include <sstream>
#include <cctype>

#include <algorithm>
#include <iterator>

// Interface headers
#include <GPIO.h>

// Local headers
#include "gpio_pin_data.h"
#include "python_functions.h"

using namespace std;
using namespace std::string_literals; // enables s-suffix for std::string literals

namespace GPIO
{
// Global variables are wrapped in singleton pattern in order to avoid
// initialization order of global variables in different compilation units problem
class EntirePinData
{
private:
    EntirePinData();

public:
    /* These vectors contain all the relevant GPIO data for each Platform.
    The values are use to generate dictionaries that map the corresponding pin
    mode numbers to the Linux GPIO pin number and GPIO chip directory */
    const vector<PinDefinition> J721E_SK_PIN_DEFS;
    const vector<string> compats_j721e;

    const vector<PinDefinition> AM68_SK_PIN_DEFS;
    const vector<string> compats_am68sk;

    const map<Model, vector<PinDefinition>> PIN_DEFS_MAP;
    const map<Model, PinInfo> DEVICE_INFO_MAP;

    EntirePinData(const EntirePinData &) = delete;
    EntirePinData &operator=(const EntirePinData &) = delete;
    ~EntirePinData() = default;

    static EntirePinData &get_instance()
    {
        static EntirePinData singleton{};
        return singleton;
    }
};

        {  4, "600000.gpio",    "3",   "2", "GPIO0_4",       "None", -1},
        {  5, "600000.gpio",    "5",   "3", "GPIO0_5",       "None", -1},
        { 66, "42110000.gpio",  "7",   "4", "WKUP_GPIO0_66", "None", -1},
        {  1, "600000.gpio",    "8",  "14", "GPIO0_1",       "None", -1},
        {  2, "600000.gpio",   "10",  "15", "GPIO0_2",       "None", -1},
        { 42, "600000.gpio",   "11",  "17", "GPIO0_42",      "None", -1},
        { 46, "600000.gpio",   "12",  "18", "GPIO0_46",      "None", -1},
        { 36, "600000.gpio",   "13",  "27", "GPIO0_36",      "None", -1},
        { 49, "42110000.gpio", "15",  "22", "WKUP_GPIO0_49", "None", -1},
        {  3, "600000.gpio",   "16",  "23", "GPIO0_3",       "None", -1},
        { 13, "600000.gpio",   "18",  "24", "GPIO0_13",      "None", -1},
        {  1, "42110000.gpio", "19",  "10", "WKUP_GPIO0_1",  "None", -1},
        {  2, "42110000.gpio", "21",   "9", "WKUP_GPIO0_2",  "None", -1},
        { 67, "42110000.gpio", "22",  "25", "WKUP_GPIO0_67", "None", -1},
        {  0, "42110000.gpio", "23",  "11", "WKUP_GPIO0_0",  "None", -1},
        {  3, "42110000.gpio", "24",   "8", "WKUP_GPIO0_3",  "None", -1},
        { 15, "42110000.gpio", "26",   "7", "WKUP_GPIO0_15", "None", -1},
        { 56, "42110000.gpio", "29",   "5", "WKUP_GPIO0_56", "None", -1},
        { 57, "42110000.gpio", "31",   "6", "WKUP_GPIO0_57", "None", -1},
        { 35, "600000.gpio",   "32",  "12", "GPIO0_35",      "None", -1},
        { 51, "600000.gpio",   "33",  "13", "GPIO0_51",      "None", -1},
        { 47, "600000.gpio",   "35",  "19", "GPIO0_47",      "None", -1},
        { 41, "600000.gpio",   "36",  "16", "GPIO0_41",      "None", -1},
        { 27, "600000.gpio",   "37",  "26", "GPIO0_27",      "None", -1},
        { 48, "600000.gpio",   "38",  "20", "GPIO0_48",      "None", -1},
        { 45, "600000.gpio",   "40",  "21", "GPIO0_45",      "None", -1}

EntirePinData::EntirePinData()
    : 
    J721E_SK_PIN_DEFS
    {
    //  OFFSET  Sysfs_dir   BOARD   BCM  SOC_NAME   PWM_SysFs PWM_Id
        { 84, "600000.gpio",  "3",  "2", "GPIO0_84",  "None",        -1},
        { 83, "600000.gpio",  "5",  "3", "GPIO0_83",  "None",        -1},
        {  7, "600000.gpio",  "7",  "4", "GPIO0_7",   "None",        -1},
        { 70, "600000.gpio",  "8", "14", "GPIO0_70",  "None",        -1},
        { 81, "600000.gpio", "10", "15", "GPIO0_81",  "None",        -1},
        { 71, "600000.gpio", "11", "17", "GPIO0_71",  "None",        -1},
        {  1, "600000.gpio", "12", "18", "GPIO0_1",   "None",        -1},
        { 82, "600000.gpio", "13", "27", "GPIO0_82",  "None",        -1},
        { 11, "600000.gpio", "15", "22", "GPIO0_11",  "None",        -1},
        {  5, "600000.gpio", "16", "23", "GPIO0_5",   "None",        -1},
        { 12, "601000.gpio", "18", "24", "GPIO0_12",  "None",        -1},
        {101, "600000.gpio", "19", "10", "GPIO0_101", "None",        -1},
        {107, "600000.gpio", "21",  "9", "GPIO0_107", "None",        -1},
        {  8, "600000.gpio", "22", "25", "GPIO0_8",   "None",        -1},
        {103, "600000.gpio", "23", "11", "GPIO0_103", "None",        -1},
        {102, "600000.gpio", "24",  "8", "GPIO0_102", "None",        -1},
        {108, "600000.gpio", "26",  "7", "GPIO0_108", "None",        -1},
        { 93, "600000.gpio", "29",  "5", "GPIO0_93",  "3020000.pwm",  0},
        { 94, "600000.gpio", "31",  "6", "GPIO0_94",  "3020000.pwm",  1},
        { 98, "600000.gpio", "32", "12", "GPIO0_98",  "3030000.pwm",  0},
        { 99, "600000.gpio", "33", "13", "GPIO0_99",  "3030000.pwm",  1},
        {  2, "600000.gpio", "35", "19", "GPIO0_2",   "None",        -1},
        { 97, "600000.gpio", "36", "16", "GPIO0_97",  "None",        -1},
        {115, "600000.gpio", "37", "26", "GPIO0_115", "None",        -1},
        {  3, "600000.gpio", "38", "20", "GPIO0_3",   "None",        -1},
        {  4, "600000.gpio", "40", "21", "GPIO0_4",   "None",        -1}
    },
    compats_j721e
    {
        "ti,j721e-eaikti",
        "ti,j721e"
    },
    AM68_SK_PIN_DEFS
    {
    //  OFFSET  Sysfs_dir   BOARD   BCM  SOC_NAME   PWM_SysFs PWM_Id
        { 84, "600000.gpio",  "3",  "2", "GPIO0_84",  "None",        -1},
        { 83, "600000.gpio",  "5",  "3", "GPIO0_83",  "None",        -1},
        {  7, "600000.gpio",  "7",  "4", "GPIO0_7",   "None",        -1},
        { 70, "600000.gpio",  "8", "14", "GPIO0_70",  "None",        -1},
        { 81, "600000.gpio", "10", "15", "GPIO0_81",  "None",        -1},
        { 71, "600000.gpio", "11", "17", "GPIO0_71",  "None",        -1},
        {  1, "600000.gpio", "12", "18", "GPIO0_1",   "None",        -1},
        { 82, "600000.gpio", "13", "27", "GPIO0_82",  "None",        -1},
        { 11, "600000.gpio", "15", "22", "GPIO0_11",  "None",        -1},
        {  5, "600000.gpio", "16", "23", "GPIO0_5",   "None",        -1},
        { 12, "601000.gpio", "18", "24", "GPIO0_12",  "None",        -1},
        {101, "600000.gpio", "19", "10", "GPIO0_101", "None",        -1},
        {107, "600000.gpio", "21",  "9", "GPIO0_107", "None",        -1},
        {  8, "600000.gpio", "22", "25", "GPIO0_8",   "None",        -1},
        {103, "600000.gpio", "23", "11", "GPIO0_103", "None",        -1},
        {102, "600000.gpio", "24",  "8", "GPIO0_102", "None",        -1},
        {108, "600000.gpio", "26",  "7", "GPIO0_108", "None",        -1},
        { 93, "600000.gpio", "29",  "5", "GPIO0_93",  "3020000.pwm",  0},
        { 94, "600000.gpio", "31",  "6", "GPIO0_94",  "3020000.pwm",  1},
        { 98, "600000.gpio", "32", "12", "GPIO0_98",  "3030000.pwm",  0},
        { 99, "600000.gpio", "33", "13", "GPIO0_99",  "3030000.pwm",  1},
        {  2, "600000.gpio", "35", "19", "GPIO0_2",   "None",        -1},
        { 97, "600000.gpio", "36", "16", "GPIO0_97",  "None",        -1},
        {115, "600000.gpio", "37", "26", "GPIO0_115", "None",        -1},
        {  3, "600000.gpio", "38", "20", "GPIO0_3",   "None",        -1},
        {  4, "600000.gpio", "40", "21", "GPIO0_4",   "None",        -1}
    },
    compats_am68sk
    {
        "ti,am68-sk",
        "ti,j721s2"
    },
    PIN_DEFS_MAP
    {
        { J721E_SK, J721E_SK_PIN_DEFS },
        { AM68_SK,  AM68_SK_PIN_DEFS }
    },
    DEVICE_INFO_MAP
    {
        { J721E_SK, {1, "8192M", "Unknown", "J721e SK", "TI", "ARM A72"} },
        { AM68_SK,  {1, "8192M", "Unknown", "AM68 SK", "TI", "ARM A72"} }
    }
{};

static bool ids_warned = false;

PinData get_data()
{
    try
    {
        EntirePinData& _DATA = EntirePinData::get_instance();

        const string compatible_path = "/proc/device-tree/compatible";
        const string ids_path = "/proc/device-tree/chosen/plugin-manager/ids";

        set<string> compatibles{};

        { // scope for f:
            ifstream f(compatible_path);
            stringstream buffer{};

            buffer << f.rdbuf();
            string tmp_str = buffer.str();
            vector<string> _vec_compatibles(split(tmp_str, '\x00'));
            // convert to std::set
            copy(_vec_compatibles.begin(), _vec_compatibles.end(), inserter(compatibles, compatibles.end()));
        } // scope ends

        auto matches = [&compatibles](const vector<string>& vals) 
        {
            for(const auto& v : vals)
            {
                if(is_in(v, compatibles)) 
                    return true;
            }
            return false;
        };

        auto find_pmgr_board = [&](const string& prefix) -> string
        {
            if (!os_path_exists(ids_path))
            {
                    if (ids_warned == false)
                    {
                        ids_warned = true;
                        string msg = "WARNING: Plugin manager information missing from device tree.\n"
                                     "WARNING: Cannot determine whether the expected board is present.";
                        cerr << msg;
                    }

                    return "None";
            }

            for (const auto& file : os_listdir(ids_path))
            {
                if (startswith(file, prefix))
                    return file;
            }

            return "None";
        };

        auto warn_if_not_carrier_board = [&find_pmgr_board](const vector<string>& carrier_boards)
        {
            auto found = false;

            for (auto&& b : carrier_boards)
            {
                found = !is_None(find_pmgr_board(b + "-"s));
                if (found)
                    break;
            }

            if (found == false)
            {
                string msg = "WARNING: Carrier board is not from a Developer Kit.\n"
                             "WARNNIG: TI.GPIO library has not been verified with this carrier board,\n"
                             "WARNING: and in fact is unlikely to work correctly.";
                cerr << msg << endl;
            }
        };

        Model model{};

        if (matches(_DATA.compats_j721e))
        {
            model = J721E_SK;
        }
        else if (matches(_DATA.compats_am68sk))
        {
            model = AM68_SK;
        }
        else
        {
            throw runtime_error("Could not determine SOC model");
        }

        vector<PinDefinition> pin_defs = _DATA.PIN_DEFS_MAP.at(model);
        PinInfo board_info = _DATA.DEVICE_INFO_MAP.at(model);

        map<string, string> gpio_chip_dirs{};
        map<string, int> gpio_chip_base{};
        map<string, string> pwm_dirs{};

        vector<string> sysfs_prefixes = {"/sys/devices/",
                                         "/sys/devices/platform/",
                                         "/sys/devices/platform/bus@100000/",
                                         "/sys/devices/platform/bus@100000/bus@100000:bus@28380000/"};

        // Get the gpiochip offsets
        set<string> gpio_chip_names{};
        for (const auto& pin_def : pin_defs)
        {
            if(!is_None(pin_def.SysfsDir))
                gpio_chip_names.insert(pin_def.SysfsDir);
        }

        for (const auto& gpio_chip_name : gpio_chip_names)
        {
            string gpio_chip_dir = "None";
            for (const auto& prefix : sysfs_prefixes)
            {
                auto d = prefix + gpio_chip_name;
                if(os_path_isdir(d))
                {
                    gpio_chip_dir = d;
                    break;
                }
            }

            if (is_None(gpio_chip_dir))
                throw runtime_error("Cannot find GPIO chip " + gpio_chip_name);

            gpio_chip_dirs[gpio_chip_name] = gpio_chip_dir;
            string gpio_chip_gpio_dir = gpio_chip_dir + "/gpio";
            auto files = os_listdir(gpio_chip_gpio_dir);
            for (const auto& fn : files)
            {
                if (!startswith(fn, "gpiochip"))
                    continue;

                string gpiochip_fn = gpio_chip_gpio_dir + "/" + fn + "/base";
                { // scope for f
                    ifstream f(gpiochip_fn);
                    stringstream buffer;
                    buffer << f.rdbuf();
                    gpio_chip_base[gpio_chip_name] = stoi(strip(buffer.str()));
                    break;
                } // scope ends
            }
        }

        auto global_gpio_id = [&gpio_chip_base](string gpio_chip_name, int chip_relative_id) -> int
        {
            if (is_None(gpio_chip_name) ||
                !is_in(gpio_chip_name, gpio_chip_base) || 
                chip_relative_id == -1)
                return -1;
            return gpio_chip_base[gpio_chip_name] + chip_relative_id;
        };

        set<string> pwm_chip_names{};
        for(const auto& x : pin_defs)
        {
            if(!is_None(x.PWMSysfsDir))
                pwm_chip_names.insert(x.PWMSysfsDir);
        }

        for(const auto& pwm_chip_name : pwm_chip_names)
        {
            string pwm_chip_dir = "None";
            for (const auto& prefix : sysfs_prefixes)
            {
                auto d = prefix + pwm_chip_name;
                if(os_path_isdir(d))
                {
                    pwm_chip_dir = d;
                    break;
                }

            }
            /* Some PWM controllers aren't enabled in all versions of the DT. In
            this case, just hide the PWM function on this pin, but let all other
            aspects of the library continue to work. */
            if (is_None(pwm_chip_dir))
                continue;

            auto chip_pwm_dir = pwm_chip_dir + "/pwm";
            if (!os_path_exists(chip_pwm_dir))
                continue;

            for (const auto& fn : os_listdir(chip_pwm_dir))
            {
                if (!startswith(fn, "pwmchip"))
                    continue;

                string chip_pwm_pwmchip_dir = chip_pwm_dir + "/" + fn;
                pwm_dirs[pwm_chip_name] = chip_pwm_pwmchip_dir;
                break;
            }
        }

        auto model_data = [&global_gpio_id, &pwm_dirs, &gpio_chip_dirs]
                          (NumberingModes key, const auto& pin_defs)
        {
            auto get_or = [](const auto& dictionary, const string& x, const string& defaultValue) -> string
            {
                return is_in(x, dictionary) ? dictionary.at(x) : defaultValue;
            };

            map<string, ChannelInfo> ret{};

            for (const auto& x : pin_defs)
            {
                string pinName = x.PinName(key);
                
                if(!is_in(x.SysfsDir, gpio_chip_dirs))
                    throw std::runtime_error("[model_data]"s + x.SysfsDir + " is not in gpio_chip_dirs"s);

                ret.insert(
                    { 
                        pinName,
                        ChannelInfo
                        { 
                            pinName,
                            gpio_chip_dirs.at(x.SysfsDir),
                            x.LinuxPin,
                            global_gpio_id(x.SysfsDir, x.LinuxPin),
                            get_or(pwm_dirs, x.PWMSysfsDir, "None"),
                            x.PWMID 
                        }
                    }
                );
            }
            return ret;
        };

        map<NumberingModes, map<string, ChannelInfo>> channel_data =
        {
            { BOARD, model_data(BOARD, pin_defs) },
            { BCM, model_data(BCM, pin_defs) },
            { SOC, model_data(SOC, pin_defs) }
        };

	    return {model, board_info, channel_data};
    }
    catch(exception& e)
    {
        cerr << "[Exception] " << e.what() << " (catched from: get_data())" << endl;
      	throw false;
    }
}

} // namespace GPIO
