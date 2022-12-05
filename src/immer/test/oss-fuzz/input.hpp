//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include "extra/fuzzer/fuzzer_input.hpp"

#include <array>
#include <fstream>
#include <iostream>
#include <vector>

std::vector<std::uint8_t> load_input(const char* file)
{
    auto fname = std::string{IMMER_OSS_FUZZ_DATA_PATH} + "/" + file;
    auto ifs   = std::ifstream{fname, std::ios::binary};
    ifs.seekg(0, ifs.end);
    auto size = ifs.tellg();
    ifs.seekg(0, ifs.beg);
    auto r = std::vector<std::uint8_t>(size);
    ifs.read(reinterpret_cast<char*>(r.data()), size);
    return r;
}
