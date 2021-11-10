//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_TEST_FAIL_COUNT_HPP
#define BOOST_BEAST_TEST_FAIL_COUNT_HPP

#include <boost/beast/core/detail/config.hpp>
#include <boost/beast/_experimental/test/error.hpp>
#include <cstdlib>

namespace boost {
namespace beast {
namespace test {

/** A countdown to simulated failure

    On the Nth operation, the class will fail with the specified
    error code, or the default error code of @ref error::test_failure.

    Instances of this class may be used to build objects which
    are specifically designed to aid in writing unit tests, for
    interfaces which can throw exceptions or return `error_code`
    values representing failure.
*/
class fail_count
{
    std::size_t n_;
    std::size_t i_ = 0;
    error_code ec_;

public:
    fail_count(fail_count&&) = default;

    /** Construct a counter

        @param n The 0-based index of the operation to fail on or after
        @param ev An optional error code to use when generating a simulated failure
    */
    BOOST_BEAST_DECL
    explicit
    fail_count(
        std::size_t n,
        error_code ev = error::test_failure);

    /// Throw an exception on the Nth failure
    BOOST_BEAST_DECL
    void
    fail();

    /// Set an error code on the Nth failure
    BOOST_BEAST_DECL
    bool
    fail(error_code& ec);
};

} // test
} // beast
} // boost

#ifdef BOOST_BEAST_HEADER_ONLY
#include <boost/beast/_experimental/test/impl/fail_count.ipp>
#endif

#endif
