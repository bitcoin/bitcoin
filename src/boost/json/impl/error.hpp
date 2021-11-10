//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/json
//

#ifndef BOOST_JSON_IMPL_ERROR_HPP
#define BOOST_JSON_IMPL_ERROR_HPP

#include <type_traits>

#ifndef BOOST_JSON_STANDALONE

namespace boost {
namespace system {
template<>
struct is_error_code_enum< ::boost::json::error >
{
    static bool const value = true;
};
template<>
struct is_error_condition_enum< ::boost::json::condition >
{
    static bool const value = true;
};
} // system
} // boost

#else

namespace std {
template<>
struct is_error_code_enum< ::boost::json::error >
{
    static bool const value = true;
};
template<>
struct is_error_condition_enum< ::boost::json::condition >
{
    static bool const value = true;
};
} // std

#endif

BOOST_JSON_NS_BEGIN

BOOST_JSON_DECL
error_code
make_error_code(error e);

BOOST_JSON_DECL
error_condition
make_error_condition(condition c);

BOOST_JSON_NS_END

#endif
