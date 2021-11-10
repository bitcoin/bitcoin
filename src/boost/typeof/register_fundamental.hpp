// Copyright (C) 2004 Arkadiy Vertleyb
// Copyright (C) 2004 Peder Holt
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TYPEOF_REGISTER_FUNDAMENTAL_HPP_INCLUDED
#define BOOST_TYPEOF_REGISTER_FUNDAMENTAL_HPP_INCLUDED

#include <boost/typeof/typeof.hpp>

#include BOOST_TYPEOF_INCREMENT_REGISTRATION_GROUP()

BOOST_TYPEOF_REGISTER_TYPE(unsigned char)
BOOST_TYPEOF_REGISTER_TYPE(unsigned short)
BOOST_TYPEOF_REGISTER_TYPE(unsigned int)
BOOST_TYPEOF_REGISTER_TYPE(unsigned long)

BOOST_TYPEOF_REGISTER_TYPE(signed char)
BOOST_TYPEOF_REGISTER_TYPE(signed short)
BOOST_TYPEOF_REGISTER_TYPE(signed int)
BOOST_TYPEOF_REGISTER_TYPE(signed long)

BOOST_TYPEOF_REGISTER_TYPE(bool)
BOOST_TYPEOF_REGISTER_TYPE(char)

BOOST_TYPEOF_REGISTER_TYPE(float)
BOOST_TYPEOF_REGISTER_TYPE(double)
BOOST_TYPEOF_REGISTER_TYPE(long double)

#ifndef BOOST_NO_INTRINSIC_WCHAR_T
// If the following line fails to compile and you're using the Intel
// compiler, see http://lists.boost.org/MailArchives/boost-users/msg06567.php,
// and define BOOST_NO_INTRINSIC_WCHAR_T on the command line.
BOOST_TYPEOF_REGISTER_TYPE(wchar_t)
#endif

#if (defined(BOOST_INTEL_CXX_VERSION) && defined(_MSC_VER) && (BOOST_INTEL_CXX_VERSION <= 600)) \
    || (defined(BOOST_BORLANDC) && (BOOST_BORLANDC == 0x600) && (_MSC_VER == 1200))
BOOST_TYPEOF_REGISTER_TYPE(unsigned __int8)
BOOST_TYPEOF_REGISTER_TYPE(__int8)
BOOST_TYPEOF_REGISTER_TYPE(unsigned __int16)
BOOST_TYPEOF_REGISTER_TYPE(__int16)
BOOST_TYPEOF_REGISTER_TYPE(unsigned __int32)
BOOST_TYPEOF_REGISTER_TYPE(__int32)
#ifdef BOOST_BORLANDC
BOOST_TYPEOF_REGISTER_TYPE(unsigned __int64)
BOOST_TYPEOF_REGISTER_TYPE(__int64)
#endif
#endif

# if defined(BOOST_HAS_LONG_LONG)
BOOST_TYPEOF_REGISTER_TYPE(::boost::ulong_long_type)
BOOST_TYPEOF_REGISTER_TYPE(::boost::long_long_type)
#elif defined(BOOST_HAS_MS_INT64)
BOOST_TYPEOF_REGISTER_TYPE(unsigned __int64)
BOOST_TYPEOF_REGISTER_TYPE(__int64)
#endif

BOOST_TYPEOF_REGISTER_TYPE(void)

#endif//BOOST_TYPEOF_REGISTER_FUNDAMENTAL_HPP_INCLUDED
