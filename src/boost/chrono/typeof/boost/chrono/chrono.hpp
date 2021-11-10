//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Vicente J. Botet Escriba 20010.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or
// copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Based on the unique_threader/unique_joiner design from of Kevlin Henney (n1883)
//
// See http://www.boost.org/libs/chrono for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_CHRONO_TYPEOF_CHRONO_HPP
#define BOOST_CHRONO_TYPEOF_CHRONO_HPP

#include <boost/chrono/chrono.hpp>
#include <boost/typeof/typeof.hpp>

#include BOOST_TYPEOF_INCREMENT_REGISTRATION_GROUP()

BOOST_TYPEOF_REGISTER_TEMPLATE(boost::chrono::duration, (typename)(typename))
BOOST_TYPEOF_REGISTER_TEMPLATE(boost::chrono::time_point, (typename)(typename))
#if 0
BOOST_TYPEOF_REGISTER_TYPE(boost::chrono::system_clock)
#ifdef BOOST_CHRONO_HAS_CLOCK_STEADY
BOOST_TYPEOF_REGISTER_TYPE(boost::chrono::steady_clock)
#endif
BOOST_TYPEOF_REGISTER_TYPE(boost::chrono::high_resolution_clock)

#endif
#endif
