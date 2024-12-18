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

#pragma once
#ifndef MODEL_H
#define MODEL_H

#include <string>

// SOC Models
enum class Model
{
    J721E_SK,
    AM68_SK,
    AM69_SK,
    AM62A_SK,
    AM62P_SK,
    J722S_EVM
};

// alias
constexpr Model    J721E_SK = Model::J721E_SK;
constexpr Model    AM68_SK  = Model::AM68_SK;
constexpr Model    AM69_SK  = Model::AM69_SK;
constexpr Model    AM62A_SK = Model::AM62A_SK;
constexpr Model    AM62P_SK = Model::AM62P_SK;
constexpr Model    J722S_EVM = Model::J722S_EVM;

static std::string ModelToString( Model m )
{
    if( m == Model::J721E_SK )
    {
        return "J721E_SK";
    }
    else if( m == Model::AM68_SK )
    {
        return "AM68_SK";
    }
    else if( m == Model::AM69_SK )
    {
        return "AM69_SK";
    }
    else if( m == Model::AM62A_SK )
    {
        return "AM62A_SK";
    }
    else if( m == Model::AM62P_SK )
    {
        return "AM62P_SK";
    }
    else if( m == Model::J722S_EVM )
    {
        return "J722S_EVM";
    }
    else
    {
        return "None";
    }
}

#endif
