
// (C) Copyright Steve Cleary, Beman Dawes, Howard Hinnant & John Maddock 2000.
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).
//
//  See http://www.boost.org/libs/type_traits for most recent version including documentation.

#ifndef BOOST_TT_ADD_POINTER_HPP_INCLUDED
#define BOOST_TT_ADD_POINTER_HPP_INCLUDED

#include <boost/type_traits/remove_reference.hpp>

namespace boost {

#if defined(BOOST_BORLANDC) && (BOOST_BORLANDC < 0x5A0)
//
// For some reason this implementation stops Borlands compiler
// from dropping cv-qualifiers, it still fails with references
// to arrays for some reason though (shrug...) (JM 20021104)
//
template <typename T>
struct add_pointer
{
    typedef T* type;
};
template <typename T>
struct add_pointer<T&>
{
    typedef T* type;
};
template <typename T>
struct add_pointer<T&const>
{
    typedef T* type;
};
template <typename T>
struct add_pointer<T&volatile>
{
    typedef T* type;
};
template <typename T>
struct add_pointer<T&const volatile>
{
    typedef T* type;
};

#else

template <typename T>
struct add_pointer
{
    typedef typename remove_reference<T>::type no_ref_type;
    typedef no_ref_type* type;
};

#endif

#if !defined(BOOST_NO_CXX11_TEMPLATE_ALIASES)

   template <class T> using add_pointer_t = typename add_pointer<T>::type;

#endif

} // namespace boost

#endif // BOOST_TT_ADD_POINTER_HPP_INCLUDED
