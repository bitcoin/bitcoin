// Copyright (C) 2004, 2005 Arkadiy Vertleyb
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (http://www.boost.org/LICENSE_1_0.txt)

// Inclusion of this file increments BOOST_TYPEOF_REGISTRATION_GROUP 
// This method was suggested by Paul Mensonides

#ifdef BOOST_TYPEOF_EMULATION
#   undef BOOST_TYPEOF_REGISTRATION_GROUP

#   include <boost/preprocessor/slot/counter.hpp>
#   include BOOST_PP_UPDATE_COUNTER()
#   define BOOST_TYPEOF_REGISTRATION_GROUP BOOST_PP_COUNTER
#endif
