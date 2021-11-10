#ifndef BOOST_SERIALIZATION_TRACKING_ENUM_HPP
#define BOOST_SERIALIZATION_TRACKING_ENUM_HPP

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// tracking_enum.hpp:

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com .
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

namespace boost {
namespace serialization {

// addresses of serialized objects may be tracked to avoid saving/loading
// redundant copies.  This header defines a class trait that can be used
// to specify when objects should be tracked

// names for each tracking level
enum tracking_type
{
    // never track this type
    track_never = 0,
    // track objects of this type if the object is serialized through a
    // pointer.
    track_selectively = 1,
    // always track this type
    track_always = 2
};

} // namespace serialization
} // namespace boost

#endif // BOOST_SERIALIZATION_TRACKING_ENUM_HPP
