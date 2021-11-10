//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/json
//

#ifndef BOOST_JSON_SERIALIZE_HPP
#define BOOST_JSON_SERIALIZE_HPP

#include <boost/json/detail/config.hpp>
#include <boost/json/value.hpp>
#include <iosfwd>
#include <string>

BOOST_JSON_NS_BEGIN

/** Return a string representing a serialized element.

    This function serializes `t` as JSON and returns
    it as a `std::string`.

    @par Complexity
    Constant or linear in the size of `t`.

    @par Exception Safety
    Strong guarantee.
    Calls to allocate may throw.

    @return The serialized string

    @param t The value to serialize
*/
/** @{ */
BOOST_JSON_DECL
std::string
serialize(value const& t);

BOOST_JSON_DECL
std::string
serialize(array const& t);

BOOST_JSON_DECL
std::string
serialize(object const& t);

BOOST_JSON_DECL
std::string
serialize(string const& t);

BOOST_JSON_DECL
std::string
serialize(string_view t);
/** @} */

/** Serialize an element to an output stream.

    This function serializes the specified element
    as JSON into the output stream.

    @return `os`.

    @par Complexity
    Constant or linear in the size of `t`.

    @par Exception Safety
    Strong guarantee.
    Calls to `memory_resource::allocate` may throw.

    @param os The output stream to serialize to.

    @param t The value to serialize
*/
/** @{ */
BOOST_JSON_DECL
std::ostream&
operator<<(
    std::ostream& os,
    value const& t);

BOOST_JSON_DECL
std::ostream&
operator<<(
    std::ostream& os,
    array const& t);

BOOST_JSON_DECL
std::ostream&
operator<<(
    std::ostream& os,
    object const& t);

BOOST_JSON_DECL
std::ostream&
operator<<(
    std::ostream& os,
    string const& t);
/** @} */

BOOST_JSON_NS_END

#endif
