// Copyright (C) 2005 Arkadiy Vertleyb, Peder Holt.
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TYPEOF_STD_string_hpp_INCLUDED
#define BOOST_TYPEOF_STD_string_hpp_INCLUDED

#include <string>
#include <boost/typeof/typeof.hpp>
#include <boost/typeof/std/memory.hpp>

#include BOOST_TYPEOF_INCREMENT_REGISTRATION_GROUP()

BOOST_TYPEOF_REGISTER_TEMPLATE(std::char_traits, 1)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::basic_string, 1)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::basic_string, 2)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::basic_string, 3)

#ifndef BOOST_BORLANDC
//Borland chokes on this "double definition" of string
BOOST_TYPEOF_REGISTER_TYPE(std::string)
#endif

#endif//BOOST_TYPEOF_STD_string_hpp_INCLUDED
