#ifndef BOOST_SERIALIZATION_LEVEL_ENUM_HPP
#define BOOST_SERIALIZATION_LEVEL_ENUM_HPP

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// level_enum.hpp:

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com .
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

namespace boost {
namespace serialization {

// for each class used in the program, specify which level
// of serialization should be implemented

// names for each level
enum level_type
{
    // Don't serialize this type. An attempt to do so should
    // invoke a compile time assertion.
    not_serializable = 0,
    // write/read this type directly to the archive. In this case
    // serialization code won't be called.  This is the default
    // case for fundamental types.  It presumes a member function or
    // template in the archive class that can handle this type.
    // there is no runtime overhead associated reading/writing
    // instances of this level
    primitive_type = 1,
    // Serialize the objects of this type using the objects "serialize"
    // function or template. This permits values to be written/read
    // to/from archives but includes no class or version information.
    object_serializable = 2,
    ///////////////////////////////////////////////////////////////////
    // once an object is serialized at one of the above levels, the
    // corresponding archives cannot be read if the implementation level
    // for the archive object is changed.
    ///////////////////////////////////////////////////////////////////
    // Add class information to the archive.  Class information includes
    // implementation level, class version and class name if available
    object_class_info = 3
};

} // namespace serialization
} // namespace boost

#endif // BOOST_SERIALIZATION_LEVEL_ENUM_HPP
