#ifndef BOOST_TYPE_TRAITS_TYPE_IDENTITY_HPP_INCLUDED
#define BOOST_TYPE_TRAITS_TYPE_IDENTITY_HPP_INCLUDED

//
//  Copyright 2015 Peter Dimov
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//

#include <boost/config.hpp>

namespace boost
{

template<class T> struct type_identity
{
    typedef T type;
};

#if !defined(BOOST_NO_CXX11_TEMPLATE_ALIASES)

template <class T> using type_identity_t = typename type_identity<T>::type;

#endif


} // namespace boost

#endif // #ifndef BOOST_TYPE_TRAITS_TYPE_IDENTITY_HPP_INCLUDED
