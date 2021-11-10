#ifndef BOOST_SERIALIZATION_STRONG_TYPEDEF_HPP
#define BOOST_SERIALIZATION_STRONG_TYPEDEF_HPP

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// strong_typedef.hpp:

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com .
// (C) Copyright 2016 Ashish Sadanandan
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/serialization for updates, documentation, and revision history.

// macro used to implement a strong typedef.  strong typedef
// guarentees that two types are distinguised even though the
// share the same underlying implementation.  typedef does not create
// a new type.  BOOST_STRONG_TYPEDEF(T, D) creates a new type named D
// that operates as a type T.

#include <boost/config.hpp>
#include <boost/operators.hpp>
#include <boost/type_traits/has_nothrow_assign.hpp>
#include <boost/type_traits/has_nothrow_constructor.hpp>
#include <boost/type_traits/has_nothrow_copy.hpp>

#define BOOST_STRONG_TYPEDEF(T, D)                                                                               \
struct D                                                                                                         \
    : boost::totally_ordered1< D                                                                                 \
    , boost::totally_ordered2< D, T                                                                              \
    > >                                                                                                          \
{                                                                                                                \
    T t;                                                                                                         \
    explicit D(const T& t_) BOOST_NOEXCEPT_IF(boost::has_nothrow_copy_constructor<T>::value) : t(t_) {}          \
    D() BOOST_NOEXCEPT_IF(boost::has_nothrow_default_constructor<T>::value) : t() {}                             \
    D(const D & t_) BOOST_NOEXCEPT_IF(boost::has_nothrow_copy_constructor<T>::value) : t(t_.t) {}                \
    D& operator=(const D& rhs) BOOST_NOEXCEPT_IF(boost::has_nothrow_assign<T>::value) {t = rhs.t; return *this;} \
    D& operator=(const T& rhs) BOOST_NOEXCEPT_IF(boost::has_nothrow_assign<T>::value) {t = rhs; return *this;}   \
    operator const T&() const {return t;}                                                                        \
    operator T&() {return t;}                                                                                    \
    bool operator==(const D& rhs) const {return t == rhs.t;}                                                     \
    bool operator<(const D& rhs) const {return t < rhs.t;}                                                       \
};

#endif // BOOST_SERIALIZATION_STRONG_TYPEDEF_HPP
