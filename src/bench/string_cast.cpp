// Copyright (c) 2018-2023 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <tinyformat.h>
#include <util/strencodings.h>
#include <util/string.h>

#include <boost/lexical_cast.hpp>
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

static void int_lexical_cast(benchmark::Bench& bench)
{
    bench.run([&] {
        boost::lexical_cast<int>("1");
    });
}

static void strings_1_lexical_cast(benchmark::Bench& bench)
{
    int i{0};
    bench.run([&] {
        boost::lexical_cast<std::string>(++i);
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

static void strings_2_multi_lexical_cast(benchmark::Bench& bench)
{
    int i{0};
    bench.run([&] { static_cast<void>(
        boost::lexical_cast<std::string>(i) +
        boost::lexical_cast<std::string>(i+1) +
        boost::lexical_cast<std::string>(i+2) +
        boost::lexical_cast<std::string>(i+3) +
        boost::lexical_cast<std::string>(i+4));
        ++i;
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
BENCHMARK(int_lexical_cast);
BENCHMARK(strings_1_lexical_cast);
BENCHMARK(strings_1_numberToString);
BENCHMARK(strings_1_tostring);
BENCHMARK(strings_2_multi_lexical_cast);
BENCHMARK(strings_2_multi_numberToString);
BENCHMARK(strings_2_multi_tostring);
BENCHMARK(strings_2_strptintf);
