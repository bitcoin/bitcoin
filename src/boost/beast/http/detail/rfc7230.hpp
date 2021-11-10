//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_HTTP_DETAIL_RFC7230_HPP
#define BOOST_BEAST_HTTP_DETAIL_RFC7230_HPP

#include <boost/beast/core/string.hpp>
#include <iterator>
#include <utility>

namespace boost {
namespace beast {
namespace http {
namespace detail {

BOOST_BEAST_DECL
bool
is_digit(char c);

BOOST_BEAST_DECL
char
is_alpha(char c);

BOOST_BEAST_DECL
char
is_text(char c);

BOOST_BEAST_DECL
char
is_token_char(char c);

BOOST_BEAST_DECL
char
is_qdchar(char c);

BOOST_BEAST_DECL
char
is_qpchar(char c);


// converts to lower case,
// returns 0 if not a valid text char
//
BOOST_BEAST_DECL
char
to_value_char(char c);

// VFALCO TODO Make this return unsigned?
BOOST_BEAST_DECL
std::int8_t
unhex(char c);

BOOST_BEAST_DECL
string_view
trim(string_view s);

struct param_iter
{
    using iter_type = string_view::const_iterator;

    iter_type it;
    iter_type first;
    iter_type last;
    std::pair<string_view, string_view> v;

    bool
    empty() const
    {
        return first == it;
    }

    BOOST_BEAST_DECL
    void
    increment();
};

/*
    #token = [ ( "," / token )   *( OWS "," [ OWS token ] ) ]
*/
struct opt_token_list_policy
{
    using value_type = string_view;

    BOOST_BEAST_DECL
    bool
    operator()(value_type& v,
        char const*& it, string_view s) const;
};

} // detail
} // http
} // beast
} // boost

#ifdef BOOST_BEAST_HEADER_ONLY
#include <boost/beast/http/detail/rfc7230.ipp>
#endif

#endif

