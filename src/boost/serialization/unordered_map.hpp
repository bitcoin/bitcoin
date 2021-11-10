#ifndef  BOOST_SERIALIZATION_UNORDERED_MAP_HPP
#define BOOST_SERIALIZATION_UNORDERED_MAP_HPP

// MS compatible compilers support #pragma once
#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// serialization/unordered_map.hpp:
// serialization for stl unordered_map templates

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com .
// (C) Copyright 2014 Jim Bell
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

#include <boost/config.hpp>

#include <unordered_map>

#include <boost/serialization/utility.hpp>
#include <boost/serialization/unordered_collections_save_imp.hpp>
#include <boost/serialization/unordered_collections_load_imp.hpp>
#include <boost/serialization/archive_input_unordered_map.hpp>
#include <boost/serialization/split_free.hpp>

namespace boost {
namespace serialization {

template<
    class Archive,
    class Key,
    class T,
    class HashFcn,
    class EqualKey,
    class Allocator
>
inline void save(
    Archive & ar,
    const std::unordered_map<Key, T, HashFcn, EqualKey, Allocator> &t,
    const unsigned int /*file_version*/
){
    boost::serialization::stl::save_unordered_collection<
        Archive,
        std::unordered_map<Key, T, HashFcn, EqualKey, Allocator>
    >(ar, t);
}

template<
    class Archive,
    class Key,
    class T,
    class HashFcn,
    class EqualKey,
    class Allocator
>
inline void load(
    Archive & ar,
    std::unordered_map<Key, T, HashFcn, EqualKey, Allocator> &t,
    const unsigned int /*file_version*/
){
    boost::serialization::stl::load_unordered_collection<
        Archive,
        std::unordered_map<Key, T, HashFcn, EqualKey, Allocator>,
        boost::serialization::stl::archive_input_unordered_map<
            Archive,
            std::unordered_map<Key, T, HashFcn, EqualKey, Allocator>
        >
    >(ar, t);
}

// split non-intrusive serialization function member into separate
// non intrusive save/load member functions
template<
    class Archive,
    class Key,
    class T,
    class HashFcn,
    class EqualKey,
    class Allocator
>
inline void serialize(
    Archive & ar,
    std::unordered_map<Key, T, HashFcn, EqualKey, Allocator> &t,
    const unsigned int file_version
){
    boost::serialization::split_free(ar, t, file_version);
}

// unordered_multimap
template<
    class Archive,
    class Key,
    class T,
    class HashFcn,
    class EqualKey,
    class Allocator
>
inline void save(
    Archive & ar,
    const std::unordered_multimap<Key, T, HashFcn, EqualKey, Allocator> &t,
    const unsigned int /*file_version*/
){
    boost::serialization::stl::save_unordered_collection<
        Archive,
        std::unordered_multimap<Key, T, HashFcn, EqualKey, Allocator>
    >(ar, t);
}

template<
    class Archive,
    class Key,
    class T,
    class HashFcn,
    class EqualKey,
    class Allocator
>
inline void load(
    Archive & ar,
    std::unordered_multimap<Key, T, HashFcn, EqualKey, Allocator> &t,
    const unsigned int /*file_version*/
){
    boost::serialization::stl::load_unordered_collection<
        Archive,
        std::unordered_multimap<Key, T, HashFcn, EqualKey, Allocator>,
        boost::serialization::stl::archive_input_unordered_multimap<
            Archive,
            std::unordered_multimap<Key, T, HashFcn, EqualKey, Allocator>
        >
    >(ar, t);
}

// split non-intrusive serialization function member into separate
// non intrusive save/load member functions
template<
    class Archive,
    class Key,
    class T,
    class HashFcn,
    class EqualKey,
    class Allocator
>
inline void serialize(
    Archive & ar,
    std::unordered_multimap<Key, T, HashFcn, EqualKey, Allocator> &t,
    const unsigned int file_version
){
    boost::serialization::split_free(ar, t, file_version);
}

} // namespace serialization
} // namespace boost

#endif // BOOST_SERIALIZATION_UNORDERED_MAP_HPP
