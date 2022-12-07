//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#include <immer/vector.hpp>

#include <iostream>
#include <string>

// include:fizzbuzz/start
immer::vector<std::string>
fizzbuzz(immer::vector<std::string> v, int first, int last)
{
    for (auto i = first; i < last; ++i)
        v = std::move(v).push_back(
            i % 15 == 0 ? "FizzBuzz"
                        : i % 5 == 0 ? "Bizz"
                                     : i % 3 == 0 ? "Fizz" :
                                                  /* else */ std::to_string(i));
    return v;
}
// include:fizzbuzz/end

int main()
{
    auto v = fizzbuzz({}, 0, 100);
    std::copy(v.begin(),
              v.end(),
              std::ostream_iterator<std::string>{std::cout, "\n"});
}
