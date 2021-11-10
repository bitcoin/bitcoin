// Boost name_generator_md5.hpp header file  ------------------------//

// Copyright 2017 James E. King III

// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
//  https://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UUID_NAME_GENERATOR_MD5_HPP
#define BOOST_UUID_NAME_GENERATOR_MD5_HPP

#include <boost/uuid/basic_name_generator.hpp>
#include <boost/uuid/detail/md5.hpp>

namespace boost {
namespace uuids {

//! \brief MD5 hashing is defined in RFC 4122 however it is not commonly
//!        used; the definition is provided for backwards compatibility.
typedef basic_name_generator<detail::md5> name_generator_md5;

} // uuids
} // boost

#endif // BOOST_UUID_NAME_GENERATOR_MD5_HPP
