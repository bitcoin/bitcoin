// Copyright (C) 2005 Arkadiy Vertleyb, Peder Holt.
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TYPEOF_STD_map_hpp_INCLUDED
#define BOOST_TYPEOF_STD_map_hpp_INCLUDED

#include <map>
#include <boost/typeof/typeof.hpp>
#include <boost/typeof/std/memory.hpp>
#include <boost/typeof/std/functional.hpp>
#include <boost/typeof/std/utility.hpp>

#include BOOST_TYPEOF_INCREMENT_REGISTRATION_GROUP()

BOOST_TYPEOF_REGISTER_TEMPLATE(std::map, 2)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::map, 3)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::map, 4)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::multimap, 2)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::multimap, 3)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::multimap, 4)

#endif//BOOST_TYPEOF_STD_map_hpp_INCLUDED
