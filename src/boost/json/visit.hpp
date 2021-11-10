//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/json
//

#ifndef BOOST_JSON_VISIT_HPP
#define BOOST_JSON_VISIT_HPP

#include <boost/json/detail/config.hpp>
#include <boost/json/value.hpp>
#include <type_traits>
#include <utility>

BOOST_JSON_NS_BEGIN

/** Invoke a function object with the contents of a @ref value

    @return The value returned by Visitor.

    @param v The visitation function to invoke

    @param jv The value to visit.
*/
/** @{ */
template<class Visitor>
auto
visit(
    Visitor&& v,
    value& jv) -> decltype(
        std::declval<Visitor>()(nullptr));

template<class Visitor>
auto
visit(
    Visitor &&v,
    value const &jv) -> decltype(
        std::declval<Visitor>()(nullptr));
/** @} */

BOOST_JSON_NS_END

#include <boost/json/impl/visit.hpp>

#endif
