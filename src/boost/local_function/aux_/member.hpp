
// Copyright (C) 2009-2012 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0
// (see accompanying file LICENSE_1_0.txt or a copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Home at http://www.boost.org/libs/local_function

#ifndef BOOST_LOCAL_FUNCTION_AUX_MEMBER_HPP_
#define BOOST_LOCAL_FUNCTION_AUX_MEMBER_HPP_

namespace boost { namespace local_function { namespace aux {

// Metafunctions to manipulate data members.

template<typename T> struct member_type {
    typedef T& reference;
    typedef T* pointer;
};

template<typename T> struct member_type<T*> {
    typedef T*& reference;
    typedef T* pointer;
};

template<typename T> struct member_type<T* const> {
    typedef T* const& reference;
    typedef T* pointer;
};

template<typename T> struct member_type<T const*> {
    typedef T const*& reference;
    typedef T const* pointer;
};

template<typename T> struct member_type<T const* const> {
    typedef T const* const& reference;
    typedef T const* pointer;
};

// NOTE: Do not add specializations for T const[&/*] (ambiguous on VACPP).
template<typename T> T* member_addr(T& data) { return &data; }
template<typename T> T* member_addr(T* data) { return data; }

// NOTE: Do not add specializations for T const[&/*] (ambiguous on VACPP).
template<typename T> T& member_deref(T& data) { return data; }
template<typename T> T& member_deref(T* data) { return *data; }

} } } // namespace

#endif // #include guard

