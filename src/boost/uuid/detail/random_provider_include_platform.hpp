//
// Copyright (c) 2017 James E. King III
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//   https://www.boost.org/LICENSE_1_0.txt)
//
// Platform-specific random entropy provider platform definition
//

#ifndef BOOST_UUID_DETAIL_RANDOM_PROVIDER_PLATFORM_INCLUDE_HPP
#define BOOST_UUID_DETAIL_RANDOM_PROVIDER_PLATFORM_INCLUDE_HPP

#if   defined(BOOST_UUID_RANDOM_PROVIDER_ARC4RANDOM)
# include <boost/uuid/detail/random_provider_arc4random.ipp>
#elif defined(BOOST_UUID_RANDOM_PROVIDER_BCRYPT)
# include <boost/uuid/detail/random_provider_bcrypt.ipp>
#elif defined(BOOST_UUID_RANDOM_PROVIDER_GETENTROPY)
# include <boost/uuid/detail/random_provider_getentropy.ipp>
#elif defined(BOOST_UUID_RANDOM_PROVIDER_GETRANDOM)
# include <boost/uuid/detail/random_provider_getrandom.ipp>
#elif defined(BOOST_UUID_RANDOM_PROVIDER_POSIX)
# include <boost/uuid/detail/random_provider_posix.ipp>
#elif defined(BOOST_UUID_RANDOM_PROVIDER_WINCRYPT)
# include <boost/uuid/detail/random_provider_wincrypt.ipp>
#endif

#endif // BOOST_UUID_DETAIL_RANDOM_PROVIDER_PLATFORM_INCLUDE_HPP

