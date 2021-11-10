// (C) Copyright Jonathan Turkanis 2004-2005.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// Contains the definition of move_ptrs::default_deleter, the default
// Deleter template argument to move_ptr. Uses a technique of Daniel
// Wallin to capture the type of a pointer at the time the deleter 
// is constructed, so that move_ptrs can delete objects of incomplete 
// type by default.

#ifndef BOOST_MOVE_PTR_DEFAULT_DELETER_HPP_INCLUDED
#define BOOST_MOVE_PTR_DEFAULT_DELETER_HPP_INCLUDED 

#include <boost/checked_delete.hpp>
#include <boost/mpl/if.hpp>
#include <boost/type_traits/is_array.hpp>
#include <boost/type_traits/remove_bounds.hpp>

namespace boost { namespace ptr_container_detail { namespace move_ptrs {

namespace ptr_container_detail {

template<typename T>
struct deleter_base {
    typedef void (*deleter)(T*);
    deleter_base(deleter d) { delete_ = d; }
    void operator() (T* t) const { delete_(t); }
    static deleter delete_;
};

template<class T>
typename deleter_base<T>::deleter 
deleter_base<T>::delete_;

template<typename T>
struct scalar_deleter : deleter_base<T> {
    typedef deleter_base<T> base;
    scalar_deleter() : base(do_delete) { }
    static void do_delete(T* t) { checked_delete(t); }
};

template<typename T>
struct array_deleter 
    : deleter_base<typename remove_bounds<T>::type>
{
    typedef typename remove_bounds<T>::type element_type;
    typedef deleter_base<element_type> base;
    array_deleter() : base(do_delete) { }
    static void do_delete(element_type* t) { checked_array_delete(t); }
};

} // End namespace ptr_container_detail.

template<typename T>
struct default_deleter
    : mpl::if_<
          is_array<T>,
          ptr_container_detail::array_deleter<T>,
          ptr_container_detail::scalar_deleter<T>
      >::type
{ 
    default_deleter() { }
    template<typename TT>
    default_deleter(default_deleter<TT>) { }
};

} } } // End namespaces ptr_container_detail, move_ptrs, boost.

#endif // #ifndef BOOST_MOVE_PTR_DEFAULT_DELETER_HPP_INCLUDED
