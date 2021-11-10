/* Copyright 2006-2008 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/flyweight for library home page.
 */

#ifndef BOOST_FLYWEIGHT_KEY_VALUE_FWD_HPP
#define BOOST_FLYWEIGHT_KEY_VALUE_FWD_HPP

#if defined(_MSC_VER)
#pragma once
#endif

namespace boost{

namespace flyweights{

struct no_key_from_value;

template<typename Key,typename Value,typename KeyFromValue=no_key_from_value>
struct key_value;

} /* namespace flyweights */

} /* namespace boost */

#endif
