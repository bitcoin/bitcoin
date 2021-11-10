// Copyright Sebastian Ramacher, 2007.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PTR_CONTAINER_SERIALIZE_PTR_MAP_HPP
#define BOOST_PTR_CONTAINER_SERIALIZE_PTR_MAP_HPP

#include <boost/ptr_container/detail/serialize_ptr_map_adapter.hpp>
#include <boost/ptr_container/ptr_map.hpp>

namespace boost 
{

namespace serialization 
{

template<class Archive, class Key, class T, class Compare, class CloneAllocator, class Allocator>
void serialize(Archive& ar, ptr_map<Key, T, Compare, CloneAllocator, Allocator>& c, const unsigned int version)
{
   split_free(ar, c, version);
}

template<class Archive, class Key, class T, class Compare, class CloneAllocator, class Allocator>
void serialize(Archive& ar, ptr_multimap<Key, T, Compare, CloneAllocator, Allocator>& c, const unsigned int version)
{
   split_free(ar, c, version);
}

} // namespace serialization
} // namespace boost

#endif
