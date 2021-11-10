// Copyright (C) 2005 Arkadiy Vertleyb, Peder Holt.
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TYPEOF_STD_sstream_hpp_INCLUDED
#define BOOST_TYPEOF_STD_sstream_hpp_INCLUDED

#include <sstream>
#include <boost/typeof/typeof.hpp>
#include <boost/typeof/std/string.hpp>
#include <boost/typeof/std/memory.hpp>

#include BOOST_TYPEOF_INCREMENT_REGISTRATION_GROUP()

BOOST_TYPEOF_REGISTER_TEMPLATE(std::basic_stringbuf, 1)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::basic_stringbuf, 2)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::basic_stringbuf, 3)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::basic_istringstream, 1)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::basic_istringstream, 2)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::basic_istringstream, 3)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::basic_ostringstream, 1)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::basic_ostringstream, 2)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::basic_ostringstream, 3)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::basic_stringstream, 1)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::basic_stringstream, 2)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::basic_stringstream, 3)
BOOST_TYPEOF_REGISTER_TYPE(std::stringbuf)
BOOST_TYPEOF_REGISTER_TYPE(std::istringstream)
BOOST_TYPEOF_REGISTER_TYPE(std::ostringstream)
BOOST_TYPEOF_REGISTER_TYPE(std::stringstream)

#endif//BOOST_TYPEOF_STD_sstream_hpp_INCLUDED
