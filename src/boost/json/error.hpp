//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/json
//

#ifndef BOOST_JSON_ERROR_HPP
#define BOOST_JSON_ERROR_HPP

#include <boost/json/detail/config.hpp>
#ifndef BOOST_JSON_STANDALONE
# include <boost/system/error_code.hpp>
# include <boost/system/system_error.hpp>
#else
# include <system_error>
#endif

BOOST_JSON_NS_BEGIN

#ifdef BOOST_JSON_DOCS

/** The type of error code used by the library.

    This type alias is set depending
    on how the library is configured:

    @par Use with Boost

    If the macro `BOOST_JSON_STANDALONE` is
    not defined, this type will be an alias
    for `boost::system::error_code`.
    Compiling a program using the library will
    require Boost, and a compiler conforming
    to C++11 or later.

    @par Use without Boost

    If the macro `BOOST_JSON_STANDALONE` is
    defined, this type will be an alias
    for `std::error_code`.
    Compiling a program using the library will
    require only a compiler conforming to C++17
    or later.

    @see https://en.cppreference.com/w/cpp/error/error_code
*/
using error_code = __see_below__;

/** The type of error category used by the library.

    This type alias is set depending
    on how the library is configured:

    @par Use with Boost

    If the macro `BOOST_JSON_STANDALONE` is
    not defined, this type will be an alias
    for `boost::system::error_category`.
    Compiling a program using the library will
    require Boost, and a compiler conforming
    to C++11 or later.

    @par Use without Boost

    If the macro `BOOST_JSON_STANDALONE` is
    defined, this type will be an alias
    for `std::error_category`.
    Compiling a program using the library will
    require only a compiler conforming to C++17
    or later.

    @see https://en.cppreference.com/w/cpp/error/error_category
*/
using error_category = __see_below__;

/** The type of error condition used by the library.

    This type alias is set depending
    on how the library is configured:

    @par Use with Boost

    If the macro `BOOST_JSON_STANDALONE` is
    not defined, this type will be an alias
    for `boost::system::error_condition`.
    Compiling a program using the library will
    require Boost, and a compiler conforming
    to C++11 or later.

    @par Use without Boost

    If the macro `BOOST_JSON_STANDALONE` is
    defined, this type will be an alias
    for `std::error_condition`.
    Compiling a program using the library will
    require only a compiler conforming to C++17
    or later.

    @see https://en.cppreference.com/w/cpp/error/error_condition
*/
using error_condition = __see_below__;

/** The type of system error thrown by the library.

    This type alias is set depending
    on how the library is configured:

    @par Use with Boost

    If the macro `BOOST_JSON_STANDALONE` is
    not defined, this type will be an alias
    for `boost::system::system_error`.
    Compiling a program using the library will
    require Boost, and a compiler conforming
    to C++11 or later.

    @par Use without Boost

    If the macro `BOOST_JSON_STANDALONE` is
    defined, this type will be an alias
    for `std::system_error`.
    Compiling a program using the library will
    require only a compiler conforming to C++17
    or later.

    @see https://en.cppreference.com/w/cpp/error/system_error
*/
using system_error = __see_below__;

/// Returns the generic error category used by the library.
error_category const&
generic_category();

#elif ! defined(BOOST_JSON_STANDALONE)

using error_code = boost::system::error_code;
using error_category = boost::system::error_category;
using error_condition = boost::system::error_condition;
using system_error = boost::system::system_error;
using boost::system::generic_category;

#else

using error_code = std::error_code;
using error_category = std::error_category;
using error_condition = std::error_condition;
using system_error = std::system_error;
using std::generic_category;

#endif

/** Error codes returned by JSON operations

*/
enum class error
{
    //
    // parse errors
    //

    /// syntax error
    syntax = 1,

    /// extra data
    extra_data,

    /// incomplete JSON
    incomplete,

    /// exponent too large
    exponent_overflow,

    /// too deep
    too_deep,

    /// illegal leading surrogate
    illegal_leading_surrogate,

    /// illegal trailing surrogate
    illegal_trailing_surrogate,

    /// expected hex digit
    expected_hex_digit,

    /// expected utf16 escape
    expected_utf16_escape,

    /// An object contains too many elements
    object_too_large,

    /// An array contains too many elements
    array_too_large,

    /// A key is too large
    key_too_large,

    /// A string is too large
    string_too_large,

    /// The parser encountered an exception and must be reset
    exception,

    //----------------------------------

    /// not a number
    not_number,

    /// number cast is not exact
    not_exact,

    /// test failure
    test_failure,
};

/** Error conditions corresponding to JSON errors
*/
enum class condition
{
    /// A parser-related error
    parse_error = 1,

    /// An error on assignment to or from a JSON value
    assign_error
};

BOOST_JSON_NS_END

#include <boost/json/impl/error.hpp>

#endif
