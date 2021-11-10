//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/json
//

#ifndef BOOST_JSON_STRING_VIEW_HPP
#define BOOST_JSON_STRING_VIEW_HPP

#include <boost/json/detail/config.hpp>
#ifndef BOOST_JSON_STANDALONE
# include <boost/utility/string_view.hpp>
# ifndef BOOST_NO_CXX17_HDR_STRING_VIEW
#  include <string_view>
# endif
#else
# if __has_include(<string_view>)
#  include <string_view>
#  if __cpp_lib_string_view < 201603L
#   error Support for std::string_view is required to use Boost.JSON standalone
#  endif
# else
#  error Header <string_view> is required to use Boost.JSON standalone
# endif
#endif
#include <type_traits>

BOOST_JSON_NS_BEGIN

#ifdef BOOST_JSON_DOCS

/** The type of string view used by the library.

    This type alias is set depending
    on how the library is configured:

    @par Use with Boost

    If the macro `BOOST_JSON_STANDALONE` is
    not defined, this type will be an alias
    for `boost::string_view`.
    Compiling a program using the library will
    require Boost, and a compiler conforming
    to C++11 or later.

    @par Use without Boost

    If the macro `BOOST_JSON_STANDALONE` is
    defined, this type will be an alias
    for `std::string_view`.
    Compiling a program using the library will
    require only a compiler conforming to C++17
    or later.

    @see https://en.cppreference.com/w/cpp/string/basic_string_view
*/
using string_view = __see_below__;

#elif ! defined(BOOST_JSON_STANDALONE)

using string_view = boost::string_view;

#else

using string_view = std::string_view;

#endif

namespace detail {

template<class T>
using is_string_viewish = typename std::enable_if<
    std::is_convertible<
        T const&, string_view>::value &&
    ! std::is_convertible<
        T const&, char const*>::value
            >::type;

} // detail

BOOST_JSON_NS_END

#endif
