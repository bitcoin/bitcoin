//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_DETAIL_STRING_HPP
#define BOOST_BEAST_DETAIL_STRING_HPP

#include <boost/beast/core/string_type.hpp>

namespace boost {
namespace beast {

namespace detail {

// Pulling in the UDL directly breaks in some places on MSVC,
// so introduce a namespace for this purprose.
namespace string_literals {

inline
string_view
operator"" _sv(char const* p, std::size_t n)
{
    return string_view{p, n};
}

} // string_literals

inline
char
ascii_tolower(char c)
{
    return ((static_cast<unsigned>(c) - 65U) < 26) ?
        c + 'a' - 'A' : c;
}
} // detail
} // beast
} // boost

#endif
