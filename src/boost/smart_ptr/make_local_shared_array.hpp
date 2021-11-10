/*
Copyright 2017 Peter Dimov
Copyright 2017-2019 Glen Joseph Fernandes
(glenjofe@gmail.com)

Distributed under the Boost Software License, Version 1.0.
(http://www.boost.org/LICENSE_1_0.txt)
*/
#ifndef BOOST_SMART_PTR_MAKE_LOCAL_SHARED_ARRAY_HPP
#define BOOST_SMART_PTR_MAKE_LOCAL_SHARED_ARRAY_HPP

#include <boost/core/default_allocator.hpp>
#include <boost/smart_ptr/allocate_local_shared_array.hpp>

namespace boost {

template<class T>
inline typename enable_if_<is_bounded_array<T>::value,
    local_shared_ptr<T> >::type
make_local_shared()
{
    return boost::allocate_local_shared<T>(boost::default_allocator<typename
        detail::sp_array_element<T>::type>());
}

template<class T>
inline typename enable_if_<is_bounded_array<T>::value,
    local_shared_ptr<T> >::type
make_local_shared(const typename remove_extent<T>::type& value)
{
    return boost::allocate_local_shared<T>(boost::default_allocator<typename
        detail::sp_array_element<T>::type>(), value);
}

template<class T>
inline typename enable_if_<is_unbounded_array<T>::value,
    local_shared_ptr<T> >::type
make_local_shared(std::size_t size)
{
    return boost::allocate_local_shared<T>(boost::default_allocator<typename
        detail::sp_array_element<T>::type>(), size);
}

template<class T>
inline typename enable_if_<is_unbounded_array<T>::value,
    local_shared_ptr<T> >::type
make_local_shared(std::size_t size,
    const typename remove_extent<T>::type& value)
{
    return boost::allocate_local_shared<T>(boost::default_allocator<typename
        detail::sp_array_element<T>::type>(), size, value);
}

template<class T>
inline typename enable_if_<is_bounded_array<T>::value,
    local_shared_ptr<T> >::type
make_local_shared_noinit()
{
    return boost::allocate_local_shared_noinit<T>(boost::
        default_allocator<typename detail::sp_array_element<T>::type>());
}

template<class T>
inline typename enable_if_<is_unbounded_array<T>::value,
    local_shared_ptr<T> >::type
make_local_shared_noinit(std::size_t size)
{
    return boost::allocate_local_shared_noinit<T>(boost::
        default_allocator<typename detail::sp_array_element<T>::type>(), size);
}

} /* boost */

#endif
