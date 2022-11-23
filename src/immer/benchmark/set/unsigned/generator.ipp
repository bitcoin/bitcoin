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
    auto operator() (std::size_t runs) const
    {
        assert(runs > 0);
        auto engine = std::default_random_engine{42};
        auto dist = std::uniform_int_distribution<unsigned>{};
        auto r = std::vector<unsigned>(runs);
        std::generate_n(r.begin(), runs, std::bind(dist, engine));
        return r;
    }
};

} // namespace
