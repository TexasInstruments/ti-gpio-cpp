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

#pragma once
#ifndef PYTHON_FUNCTIONS_H
#define PYTHON_FUNCTIONS_H

// Standard headers
#include <string>
#include <vector>
#include <map>
#include <set>

bool startswith(const std::string& s, const std::string& prefix);

std::vector<std::string> split(const std::string& s, const char d);

bool os_access(const std::string& path, int mode); // os.access

std::vector<std::string> os_listdir(const std::string& path); // os.listdir

bool os_path_isdir(const std::string& path); // os.path.isdirs

bool os_path_exists(const std::string& path); // os.path.exists

std::string strip(const std::string& s);

bool is_None(const std::string& s);

template<class key_t, class element_t>
bool is_in(const key_t& key, const std::map<key_t, element_t>& dictionary)
{
    return dictionary.find(key) != dictionary.end();
}


template<class key_t>
bool is_in(const key_t& key, const std::set<key_t>& set)
{
    return set.find(key) != set.end();
}

#endif // PYTHON_FUNCTIONS_H
