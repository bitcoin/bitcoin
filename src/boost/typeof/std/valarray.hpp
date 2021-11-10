// Copyright (C) 2005 Arkadiy Vertleyb, Peder Holt.
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TYPEOF_STD_valarray_hpp_INCLUDED
#define BOOST_TYPEOF_STD_valarray_hpp_INCLUDED

#include <valarray>
#include <boost/typeof/typeof.hpp>

#include BOOST_TYPEOF_INCREMENT_REGISTRATION_GROUP()

BOOST_TYPEOF_REGISTER_TEMPLATE(std::valarray, 1)
BOOST_TYPEOF_REGISTER_TYPE(std::slice)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::slice_array, 1)
BOOST_TYPEOF_REGISTER_TYPE(std::gslice)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::gslice_array, 1)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::mask_array, 1)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::indirect_array, 1)

#endif//BOOST_TYPEOF_STD_valarray_hpp_INCLUDED
