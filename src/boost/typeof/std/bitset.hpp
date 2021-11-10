// Copyright (C) 2005 Arkadiy Vertleyb, Peder Holt.
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TYPEOF_STD_bitset_hpp_INCLUDED
#define BOOST_TYPEOF_STD_bitset_hpp_INCLUDED

#include <bitset>
#include <boost/typeof/typeof.hpp>

#include BOOST_TYPEOF_INCREMENT_REGISTRATION_GROUP()

BOOST_TYPEOF_REGISTER_TEMPLATE(std::bitset, (BOOST_TYPEOF_INTEGRAL(std::size_t)))

#endif//BOOST_TYPEOF_STD_bitset_hpp_INCLUDED
