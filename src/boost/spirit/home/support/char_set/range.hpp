/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_SPIRIT_RANGE_MAY_16_2006_0720_PM)
#define BOOST_SPIRIT_RANGE_MAY_16_2006_0720_PM

#if defined(_MSC_VER)
#pragma once
#endif

namespace boost { namespace spirit { namespace support { namespace detail
{
    ///////////////////////////////////////////////////////////////////////////
    //  A closed range (first, last)
    ///////////////////////////////////////////////////////////////////////////
    template <typename T>
    struct range
    {
        typedef T value_type;

        range() : first(), last() {}
        range(T first_, T last_) : first(first_), last(last_) {}

        T first;
        T last;
    };
}}}}

#endif
