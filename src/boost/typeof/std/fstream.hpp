// Copyright (C) 2005 Arkadiy Vertleyb, Peder Holt.
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TYPEOF_STD_fstream_hpp_INCLUDED
#define BOOST_TYPEOF_STD_fstream_hpp_INCLUDED

#include <fstream>
#include <boost/typeof/typeof.hpp>
#include <boost/typeof/std/string.hpp>

#include BOOST_TYPEOF_INCREMENT_REGISTRATION_GROUP()

BOOST_TYPEOF_REGISTER_TEMPLATE(std::basic_filebuf, 1)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::basic_filebuf, 2)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::basic_ifstream, 1)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::basic_ifstream, 2)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::basic_ofstream, 1)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::basic_ofstream, 2)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::basic_fstream, 1)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::basic_fstream, 2)
BOOST_TYPEOF_REGISTER_TYPE(std::filebuf)
BOOST_TYPEOF_REGISTER_TYPE(std::ifstream)
BOOST_TYPEOF_REGISTER_TYPE(std::ofstream)
BOOST_TYPEOF_REGISTER_TYPE(std::fstream)

#endif//BOOST_TYPEOF_STD_fstream_hpp_INCLUDED
