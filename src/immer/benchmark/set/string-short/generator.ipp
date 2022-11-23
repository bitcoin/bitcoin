//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#include <random>
#include <vector>
#include <cassert>
#include <functional>
#include <algorithm>

#define GENERATOR_T generate_unsigned

namespace {

struct GENERATOR_T
{
    static constexpr auto char_set   = "_-0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    static constexpr auto max_length = 15;
    static constexpr auto min_length = 4;

    auto operator() (std::size_t runs) const
    {
        assert(runs > 0);
        auto engine = std::default_random_engine{42};
        auto dist = std::uniform_int_distribution<unsigned>{};
        auto gen = std::bind(dist, engine);
        auto r = std::vector<std::string>(runs);
        std::generate_n(r.begin(), runs, [&] {
            auto len = gen() % (max_length - min_length) + min_length;
            auto str = std::string(len, ' ');
            std::generate_n(str.begin(), len, [&] {
                return char_set[gen() % sizeof(char_set)];
            });
            return str;
        });
        return r;
    }
};

} // namespace
