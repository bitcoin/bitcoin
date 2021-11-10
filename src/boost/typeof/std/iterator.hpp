// Copyright (C) 2005 Arkadiy Vertleyb, Peder Holt.
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TYPEOF_STD_iterator_hpp_INCLUDED
#define BOOST_TYPEOF_STD_iterator_hpp_INCLUDED

#include <iterator>
#include <boost/typeof/typeof.hpp>
#include <boost/typeof/std/string.hpp>

#include BOOST_TYPEOF_INCREMENT_REGISTRATION_GROUP()

BOOST_TYPEOF_REGISTER_TEMPLATE(std::iterator_traits, 1)
#if BOOST_WORKAROUND(BOOST_DINKUMWARE_STDLIB, == 1)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::iterator, 2)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::iterator, 3)
#else
BOOST_TYPEOF_REGISTER_TEMPLATE(std::iterator, 2)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::iterator, 3)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::iterator, 4)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::iterator, 5)
#endif//BOOST_WORKAROUND(BOOST_DINKUMWARE_STDLIB, == 1)
BOOST_TYPEOF_REGISTER_TYPE(std::input_iterator_tag)
BOOST_TYPEOF_REGISTER_TYPE(std::output_iterator_tag)
BOOST_TYPEOF_REGISTER_TYPE(std::forward_iterator_tag)
BOOST_TYPEOF_REGISTER_TYPE(std::bidirectional_iterator_tag)
BOOST_TYPEOF_REGISTER_TYPE(std::random_access_iterator_tag)
#if BOOST_WORKAROUND(BOOST_DINKUMWARE_STDLIB, == 1)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::reverse_iterator, 2)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::reverse_iterator, 3)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::reverse_iterator, 4)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::reverse_iterator, 5)
#else
BOOST_TYPEOF_REGISTER_TEMPLATE(std::reverse_iterator, 1)
#endif//BOOST_WORKAROUND(BOOST_DINKUMWARE_STDLIB, == 1)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::back_insert_iterator, 1)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::front_insert_iterator, 1)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::insert_iterator, 1)
#if BOOST_WORKAROUND(BOOST_DINKUMWARE_STDLIB, == 1)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::istream_iterator, 1)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::istream_iterator, 2)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::istream_iterator, 3)
#else
BOOST_TYPEOF_REGISTER_TEMPLATE(std::istream_iterator, 1)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::istream_iterator, 2)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::istream_iterator, 3)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::istream_iterator, 4)
#endif//BOOST_WORKAROUND(BOOST_DINKUMWARE_STDLIB, == 1)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::ostream_iterator, 1)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::ostream_iterator, 2)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::ostream_iterator, 3)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::istreambuf_iterator, 1)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::istreambuf_iterator, 2)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::ostreambuf_iterator, 1)
BOOST_TYPEOF_REGISTER_TEMPLATE(std::ostreambuf_iterator, 2)

#endif//BOOST_TYPEOF_STD_iterator_hpp_INCLUDED
