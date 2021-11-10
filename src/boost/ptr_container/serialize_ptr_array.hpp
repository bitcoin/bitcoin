// Copyright Sebastian Ramacher, 2007.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PTR_CONTAINER_SERIALIZE_PTR_ARRAY_HPP
#define BOOST_PTR_CONTAINER_SERIALIZE_PTR_ARRAY_HPP

#include <boost/ptr_container/detail/serialize_reversible_cont.hpp>
#include <boost/ptr_container/ptr_array.hpp>

namespace boost 
{

namespace serialization 
{

template<class Archive, class T, std::size_t N, class CloneAllocator>
void save(Archive& ar, const ptr_array<T, N, CloneAllocator>& c, unsigned int /*version*/)
{
    ptr_container_detail::save_helper(ar, c);
}

template<class Archive, class T, std::size_t N, class CloneAllocator>
void load(Archive& ar, ptr_array<T, N, CloneAllocator>& c, unsigned int /*version*/)
{
    typedef ptr_array<T, N, CloneAllocator> container_type;
    typedef BOOST_DEDUCED_TYPENAME container_type::size_type size_type;
    
    for(size_type i = 0u; i != N; ++i)
    {
        T* p;
        ar >> boost::serialization::make_nvp( ptr_container_detail::item(), p );
        c.replace(i, p);
    }
}

template<class Archive, class T, std::size_t N, class CloneAllocator>
void serialize(Archive& ar, ptr_array<T, N, CloneAllocator>& c, const unsigned int version)
{
   split_free(ar, c, version);
}

} // namespace serialization
} // namespace boost

#endif
