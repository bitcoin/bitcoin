///////////////////////////////////////////////////////////////////////////////
// dont_care.hpp
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_XPRESSIVE_DETAIL_UTILITY_DONT_CARE_HPP_EAN_10_04_2005
#define BOOST_XPRESSIVE_DETAIL_UTILITY_DONT_CARE_HPP_EAN_10_04_2005

namespace boost { namespace xpressive { namespace detail
{
    ///////////////////////////////////////////////////////////////////////////////
    // for function arguments we don't care about
    struct dont_care
    {
        dont_care() {}

        template<typename T>
        dont_care(T const &) {}
    };

}}}

#endif
