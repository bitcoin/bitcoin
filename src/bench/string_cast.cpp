// Copyright (c) 2018-2024 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <tinyformat.h>
#include <util/strencodings.h>
#include <util/string.h>

#include <string>

template <typename T>
std::string NumberToString(T Number){
    std::ostringstream oss;
    oss << Number;
    return oss.str();
}

static void int_atoi(benchmark::Bench& bench)
{
    int value;
    bench.run([&] {
        value = LocaleIndependentAtoi<int>("1");
    });
}

static void strings_1_numberToString(benchmark::Bench& bench)
{
    int i{0};
    bench.run([&] {
        NumberToString(++i);
    });
}

static void strings_1_tostring(benchmark::Bench& bench)
{
    int i{0};
    bench.run([&] {
        ToString(++i);
    });
}

static void strings_2_multi_numberToString(benchmark::Bench& bench)
{
    int i{0};
    bench.run([&] { static_cast<void>(
        NumberToString(i) + NumberToString(i+1) + NumberToString(i+2) + NumberToString(i+3) + NumberToString(i+4));
        ++i;
    });
}

static void strings_2_multi_tostring(benchmark::Bench& bench)
{
    int i{0};
    bench.run([&] { static_cast<void>(
        ToString(i) + ToString(i+1) + ToString(i+2) + ToString(i+3) + ToString(i+4));
        ++i;
    });
}

static void strings_2_strptintf(benchmark::Bench& bench)
{
    int i{0};
    bench.run([&] {
        strprintf("%d|%d|%d|%d|%d", i, i+1, i+2, i+3, i+4);
        ++i;
    });
}

BENCHMARK(int_atoi);
BENCHMARK(strings_1_numberToString);
BENCHMARK(strings_1_tostring);
BENCHMARK(strings_2_multi_numberToString);
BENCHMARK(strings_2_multi_tostring);
BENCHMARK(strings_2_strptintf);
