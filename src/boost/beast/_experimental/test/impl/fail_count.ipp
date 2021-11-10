//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_TEST_IMPL_FAIL_COUNT_IPP
#define BOOST_BEAST_TEST_IMPL_FAIL_COUNT_IPP

#include <boost/beast/_experimental/test/fail_count.hpp>
#include <boost/throw_exception.hpp>

namespace boost {
namespace beast {
namespace test {

fail_count::
fail_count(
    std::size_t n,
    error_code ev)
    : n_(n)
    , ec_(ev)
{
}

void
fail_count::
fail()
{
    if(i_ < n_)
        ++i_;
    if(i_ == n_)
        BOOST_THROW_EXCEPTION(system_error{ec_});
}

bool
fail_count::
fail(error_code& ec)
{
    if(i_ < n_)
        ++i_;
    if(i_ == n_)
    {
        ec = ec_;
        return true;
    }
    ec = {};
    return false;
}

} // test
} // beast
} // boost

#endif
