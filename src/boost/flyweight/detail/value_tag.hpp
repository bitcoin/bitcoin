/* Copyright 2006-2008 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/flyweight for library home page.
 */

#ifndef BOOST_FLYWEIGHT_DETAIL_VALUE_TAG_HPP
#define BOOST_FLYWEIGHT_DETAIL_VALUE_TAG_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/config.hpp> /* keep it first to prevent nasty warns in MSVC */
#include <boost/parameter/parameters.hpp>
#include <boost/type_traits/is_base_and_derived.hpp>

namespace boost{

namespace flyweights{

namespace detail{

/* Three ways to indicate that a given class T is a value policy:
 *   1. Make it derived from value_marker.
 *   2. Specialize is_value to evaluate to boost::mpl::true_.
 *   3. Pass it as value<T> when defining a flyweight type.
 *
 * For the time being the interface of value policies is not public.
 */

struct value_marker{};

template<typename T>
struct is_value:is_base_and_derived<value_marker,T>
{};

template<typename T=parameter::void_>
struct value:parameter::template_keyword<value<>,T>
{};

} /* namespace flyweights::detail */

} /* namespace flyweights */

} /* namespace boost */

#endif
