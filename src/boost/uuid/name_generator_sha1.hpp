// Boost name_generator_sha1.hpp header file  ------------------------//

// Copyright 2010 Andy Tompkins.
// Copyright 2017 James E. King III

// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
//  https://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UUID_NAME_GENERATOR_SHA1_HPP
#define BOOST_UUID_NAME_GENERATOR_SHA1_HPP

#include <boost/uuid/basic_name_generator.hpp>
#include <boost/uuid/detail/sha1.hpp>

namespace boost {
namespace uuids {

//! \brief SHA1 hashing is the default method defined in RFC 4122 to be
//!        used when backwards compatibility is not necessary.
typedef basic_name_generator<detail::sha1> name_generator_sha1;

} // uuids
} // boost

#endif // BOOST_UUID_NAME_GENERATOR_SHA1_HPP
