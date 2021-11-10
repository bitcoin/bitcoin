// Copyright (C) 2005 Arkadiy Vertleyb, Peder Holt.
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TYPEOF_STD_set_hpp_INCLUDED
#define BOOST_TYPEOF_STD_set_hpp_INCLUDED

#include <set>
#include <boost/typeof/typeof.hpp>
#include <boost/typeof/std/memory.hpp>
#include <boost/typeof/std/functional.hpp>

#include BOOST_TYPEOF_INCREMENT_REGISTRATION_GROUP()

BOOST_TYPEOF_REGISTER_TEMPLATE(std::set, 1)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::set, 2)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::set, 3)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::multiset, 1)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::multiset, 2)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::multiset, 3)

#endif//BOOST_TYPEOF_STD_set_hpp_INCLUDED
