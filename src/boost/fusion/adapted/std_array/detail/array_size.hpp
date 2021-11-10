/*=============================================================================
    Copyright (c) 2013 Mateusz Loskot

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_FUSION_STD_ARRAY_ARRAY_SIZE_01062013_1700)
#define BOOST_FUSION_STD_ARRAY_ARRAY_SIZE_01062013_1700

#include <cstddef>
#include <boost/type_traits/integral_constant.hpp>

namespace boost { namespace fusion
{
    namespace extension
    {
        template<class T>
        struct std_array_size;

        template<template<typename, std::size_t> class Array, typename T, std::size_t N>
        struct std_array_size<Array<T, N> > : boost::integral_constant<std::size_t, N> {};
    }
}}

#endif
