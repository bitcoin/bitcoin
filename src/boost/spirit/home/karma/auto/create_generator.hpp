//  Copyright (c) 2001-2011 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_KARMA_CREATE_NOV_21_2009_0340PM)
#define BOOST_SPIRIT_KARMA_CREATE_NOV_21_2009_0340PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/karma/auto/meta_create.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit { namespace result_of
{
    ///////////////////////////////////////////////////////////////////////////
    template <typename T>
    struct create_generator
      : spirit::traits::meta_create<karma::domain, T> {};
}}}

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit { namespace karma
{
    // Main API function for generator creation from data type
    template <typename T>
    typename result_of::create_generator<T>::type
    create_generator()
    {
        return spirit::traits::meta_create<karma::domain, T>::call();
    }
}}}

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit { namespace traits
{
    // Meta function returning true if create_generator does return a valid
    // generator for the given type T.
    template <typename T>
    struct create_generator_exists
      : meta_create_exists<karma::domain, T> {};
}}}

#endif
