#ifndef BOOST_SERIALIZATION_TRAITS_HPP
#define BOOST_SERIALIZATION_TRAITS_HPP

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// traits.hpp:

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com .
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

// This header is used to apply serialization traits to templates.  The
// standard system can't be used for platforms which don't support
// Partial Templlate Specialization.

// The motivation for this is the Name-Value Pair (NVP) template.
// it has to work the same on all platforms in order for archives
// to be portable accross platforms.

#include <boost/config.hpp>
#include <boost/static_assert.hpp>

#include <boost/mpl/int.hpp>
#include <boost/mpl/bool_fwd.hpp>
#include <boost/serialization/level_enum.hpp>
#include <boost/serialization/tracking_enum.hpp>

namespace boost {
namespace serialization {

// common base class used to detect appended traits class
struct basic_traits {};

template <class T>
struct extended_type_info_impl;

template<
    class T,
    int Level,
    int Tracking,
    unsigned int Version = 0,
    class ETII = extended_type_info_impl< T >,
    class Wrapper = mpl::false_
>
struct traits : public basic_traits {
    BOOST_STATIC_ASSERT(Version == 0 || Level >= object_class_info);
    BOOST_STATIC_ASSERT(Tracking == track_never || Level >= object_serializable);
    typedef typename mpl::int_<Level> level;
    typedef typename mpl::int_<Tracking> tracking;
    typedef typename mpl::int_<Version> version;
    typedef ETII type_info_implementation;
    typedef Wrapper is_wrapper;
};

} // namespace serialization
} // namespace boost

#endif // BOOST_SERIALIZATION_TRAITS_HPP
