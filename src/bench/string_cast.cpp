// Copyright (c) 2018-2020 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <tinyformat.h>
#include <util/strencodings.h>

#include <boost/lexical_cast.hpp>
#include <string>

template <typename T>
std::string NumberToString(T Number){
    std::ostringstream oss;
    oss << Number;
    return oss.str();
}

static void int_atoi(benchmark::State& state)
{
    while (state.KeepRunning())
        atoi("1");
}

static void int_lexical_cast(benchmark::State& state)
{
    while (state.KeepRunning())
        boost::lexical_cast<int>("1");
}

static void strings_1_itostr(benchmark::State& state)
{
    int i{0};
    while (state.KeepRunning())
        itostr(++i);
}

static void strings_1_lexical_cast(benchmark::State& state)
{
    int i{0};
    while (state.KeepRunning())
        boost::lexical_cast<std::string>(++i);
}

static void strings_1_numberToString(benchmark::State& state)
{
    int i{0};
    while (state.KeepRunning())
        NumberToString(++i);
}

static void strings_1_to_string(benchmark::State& state)
{
    int i{0};
    while (state.KeepRunning())
        std::to_string(++i);
}

static void strings_2_multi_itostr(benchmark::State& state)
{
    int i{0};
    while (state.KeepRunning()) {
        itostr(i) + itostr(i+1) + itostr(i+2) + itostr(i+3) + itostr(i+4);
        ++i;
    }
}

static void strings_2_multi_lexical_cast(benchmark::State& state)
{
    int i{0};
    while (state.KeepRunning()) {
        boost::lexical_cast<std::string>(i) +
        boost::lexical_cast<std::string>(i+1) +
        boost::lexical_cast<std::string>(i+2) +
        boost::lexical_cast<std::string>(i+3) +
        boost::lexical_cast<std::string>(i+4);
        ++i;
    }
}

static void strings_2_multi_numberToString(benchmark::State& state)
{
    int i{0};
    while (state.KeepRunning()) {
        NumberToString(i) + NumberToString(i+1) + NumberToString(i+2) + NumberToString(i+3) + NumberToString(i+4);
        ++i;
    }
}

static void strings_2_multi_to_string(benchmark::State& state)
{
    int i{0};
    while (state.KeepRunning()) {
        std::to_string(i) + std::to_string(i+1) + std::to_string(i+2) + std::to_string(i+3) + std::to_string(i+4);
        ++i;
    }
}

static void strings_2_strptintf(benchmark::State& state)
{
    int i{0};
    while (state.KeepRunning()) {
        strprintf("%d|%d|%d|%d|%d", i, i+1, i+2, i+3, i+4);
        ++i;
    }
}

BENCHMARK(int_atoi, 700 * 1000 * 1000);
BENCHMARK(int_lexical_cast, 40 * 1000 * 1000);
BENCHMARK(strings_1_itostr, 3 * 1000 * 1000);
BENCHMARK(strings_1_lexical_cast, 20 * 1000 * 1000);
BENCHMARK(strings_1_numberToString, 3 * 1000 * 1000);
BENCHMARK(strings_1_to_string, 8 * 1000 * 1000);
BENCHMARK(strings_2_multi_itostr, 500 * 1000);
BENCHMARK(strings_2_multi_lexical_cast, 2500 * 1000);
BENCHMARK(strings_2_multi_numberToString, 600 * 1000);
BENCHMARK(strings_2_multi_to_string, 1500 * 1000);
BENCHMARK(strings_2_strptintf, 700 * 1000);
