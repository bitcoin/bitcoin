
// Copyright (C) 2009-2012 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0
// (see accompanying file LICENSE_1_0.txt or a copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Home at http://www.boost.org/libs/local_function

#ifndef BOOST_LOCAL_FUNCTION_AUX_ADD_POINTED_CONST_HPP_
#define BOOST_LOCAL_FUNCTION_AUX_ADD_POINTED_CONST_HPP_

namespace boost { namespace local_function { namespace aux {

// Metafunction to add const to pointed type `T` (i.e. converts
// `T* [const]` to `T const* [const]`). `boost::add_const<>` cannot be used 
// instead because only adds outer const.

template<typename T> struct add_pointed_const { typedef T type; };

template<typename T> struct add_pointed_const<T*> { typedef T const* type; };

template<typename T> struct add_pointed_const<T const*>
    { typedef T const* type; };

template<typename T> struct add_pointed_const<T* const>
    { typedef T const* const type; };

template<typename T> struct add_pointed_const<T const* const>
    { typedef T const* const type; };

} } } // namespace

#endif //#include guard

