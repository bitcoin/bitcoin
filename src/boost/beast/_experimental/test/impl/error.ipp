//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_TEST_IMPL_ERROR_IPP
#define BOOST_BEAST_TEST_IMPL_ERROR_IPP

#include <boost/beast/_experimental/test/error.hpp>

namespace boost {
namespace beast {
namespace test {

namespace detail {

class error_codes : public error_category
{
public:
    BOOST_BEAST_DECL
    const char*
    name() const noexcept override
    {
        return "boost.beast.test";
    }

    BOOST_BEAST_DECL
    std::string
    message(int ev) const override
    {
        switch(static_cast<error>(ev))
        {
        default:
        case error::test_failure: return
            "An automatic unit test failure occurred";
        }
    }

    BOOST_BEAST_DECL
    error_condition
    default_error_condition(int ev) const noexcept override
    {
        return error_condition{ev, *this};
    }
};

} // detail

error_code
make_error_code(error e) noexcept
{
    static detail::error_codes const cat{};
    return error_code{static_cast<
        std::underlying_type<error>::type>(e), cat};
}

} // test
} // beast
} // boost

#endif
