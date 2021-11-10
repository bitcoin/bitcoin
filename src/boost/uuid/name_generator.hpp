// Boost name_generator.hpp header file  -----------------------------//

// Copyright 2010 Andy Tompkins.
// Copyright 2017 James E. King III

// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
//  https://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UUID_NAME_GENERATOR_HPP
#define BOOST_UUID_NAME_GENERATOR_HPP

#include <boost/config.hpp>
#include <boost/uuid/name_generator_sha1.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {
namespace uuids {

//! \deprecated
//! \brief this provides backwards compatibility with previous boost
//!        releases however it is now deprecated to ensure that once
//!        a new hashing algorithm is defined for name generation that
//!        there is no confusion - at that time this will be removed.
typedef name_generator_sha1 name_generator;

//! \brief this provides the latest name generator hashing algorithm
//!        regardless of boost release; if you do not need stable
//!        name generation across releases then this will suffice
typedef name_generator_sha1 name_generator_latest;

} // uuids
} // boost

#endif // BOOST_UUID_NAME_GENERATOR_HPP
