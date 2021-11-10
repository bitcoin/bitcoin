//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_UNIT_TEST_GLOBAL_SUITES_HPP
#define BOOST_BEAST_UNIT_TEST_GLOBAL_SUITES_HPP

#include <boost/beast/_experimental/unit_test/suite_list.hpp>

namespace boost {
namespace beast {
namespace unit_test {

namespace detail {

/// Holds test suites registered during static initialization.
inline
suite_list&
global_suites()
{
    static suite_list s;
    return s;
}

template<class Suite>
struct insert_suite
{
    insert_suite(char const* name, char const* module,
        char const* library, bool manual)
    {
        global_suites().insert<Suite>(
            name, module, library, manual);
    }
};

} // detail

/// Holds test suites registered during static initialization.
inline
suite_list const&
global_suites()
{
    return detail::global_suites();
}

} // unit_test
} // beast
} // boost

#endif
