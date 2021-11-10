//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_IMPL_ERROR_IPP
#define BOOST_BEAST_IMPL_ERROR_IPP

#include <boost/beast/core/error.hpp>

namespace boost {
namespace beast {

namespace detail {

class error_codes : public error_category
{
public:
    const char*
    name() const noexcept override
    {
        return "boost.beast";
    }

    BOOST_BEAST_DECL
    std::string
    message(int ev) const override
    {
        switch(static_cast<error>(ev))
        {
        default:
        case error::timeout: return
            "The socket was closed due to a timeout";
        }
    }

    BOOST_BEAST_DECL
    error_condition
    default_error_condition(int ev) const noexcept override
    {
        switch(static_cast<error>(ev))
        {
        default:
    //        return {ev, *this};
        case error::timeout:
            return condition::timeout;
        }
    }
};

class error_conditions : public error_category
{
public:
    BOOST_BEAST_DECL
    const char*
    name() const noexcept override
    {
        return "boost.beast";
    }

    BOOST_BEAST_DECL
    std::string
    message(int cv) const override
    {
        switch(static_cast<condition>(cv))
        {
        default:
        case condition::timeout:
            return "The operation timed out";
        }
    }
};

} // detail

error_code
make_error_code(error e)
{
    static detail::error_codes const cat{};
    return error_code{static_cast<
        std::underlying_type<error>::type>(e), cat};
}

error_condition
make_error_condition(condition c)
{
    static detail::error_conditions const cat{};
    return error_condition{static_cast<
        std::underlying_type<condition>::type>(c), cat};
}

} // beast
} // boost

#endif
