#ifndef BOOST_SMART_PTR_DETAIL_SP_OBSOLETE_HPP_INCLUDED
#define BOOST_SMART_PTR_DETAIL_SP_OBSOLETE_HPP_INCLUDED

// MS compatible compilers support #pragma once

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif


// boost/smart_ptr/detail/sp_obsolete.hpp
//
// Copyright 2020 Peter Dimov
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// Defines the BOOST_SP_OBSOLETE macro that emits a deprecation
// message.

#include <boost/config/pragma_message.hpp>

#if !defined( BOOST_SP_NO_OBSOLETE_MESSAGE )

#define BOOST_SP_OBSOLETE() BOOST_PRAGMA_MESSAGE("This platform-specific implementation is presumed obsolete and is slated for removal. If you want it retained, please open an issue in https://github.com/boostorg/smart_ptr.")

#else

#define BOOST_SP_OBSOLETE()

#endif

#endif // #ifndef BOOST_SMART_PTR_DETAIL_SP_OBSOLETE_HPP_INCLUDED
