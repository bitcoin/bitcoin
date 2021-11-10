//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
// Copyright (c) 2020 Krystian Stasiowski (sdkrystian@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/json
//

#ifndef BOOST_JSON_PARSE_HPP
#define BOOST_JSON_PARSE_HPP

#include <boost/json/detail/config.hpp>
#include <boost/json/error.hpp>
#include <boost/json/parse_options.hpp>
#include <boost/json/storage_ptr.hpp>
#include <boost/json/string_view.hpp>
#include <boost/json/value.hpp>

BOOST_JSON_NS_BEGIN

/** Return parsed JSON as a @ref value.

    This function parses an entire string in one
    step to produce a complete JSON object, returned
    as a @ref value. If the buffer does not contain a
    complete serialized JSON, an error occurs. In this
    case the returned value will be null, using the
    default memory resource.

    @par Complexity
    Linear in `s.size()`.

    @par Exception Safety
    Strong guarantee.
    Calls to `memory_resource::allocate` may throw.

    @return A value representing the parsed JSON,
    or a null if any error occurred.

    @param s The string to parse.

    @param ec Set to the error, if any occurred.

    @param sp The memory resource that the new value and all
    of its elements will use. If this parameter is omitted,
    the default memory resource is used.

    @param opt The options for the parser. If this parameter
    is omitted, the parser will accept only standard JSON.

    @see
        @ref parse_options,
        @ref stream_parser.
*/
BOOST_JSON_DECL
value
parse(
    string_view s,
    error_code& ec,
    storage_ptr sp = {},
    parse_options const& opt = {});

/** Parse a string of JSON into a @ref value.

    This function parses an entire string in one
    step to produce a complete JSON object, returned
    as a @ref value. If the buffer does not contain a
    complete serialized JSON, an exception is thrown.

    @par Complexity
    Linear in `s.size()`.

    @par Exception Safety
    Strong guarantee.
    Calls to `memory_resource::allocate` may throw.

    @return A value representing the parsed
    JSON upon success.

    @param s The string to parse.

    @param sp The memory resource that the new value and all
    of its elements will use. If this parameter is omitted,
    the default memory resource is used.

    @param opt The options for the parser. If this parameter
    is omitted, the parser will accept only standard JSON.

    @throw system_error Thrown on failure.

    @see
        @ref parse_options,
        @ref stream_parser.
*/
BOOST_JSON_DECL
value
parse(
    string_view s,
    storage_ptr sp = {},
    parse_options const& opt = {});

BOOST_JSON_NS_END

#endif
