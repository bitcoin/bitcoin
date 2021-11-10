///////////////////////////////////////////////////////////////////////////////
// never_true.hpp
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_XPRESSIVE_DETAIL_UTILITY_NEVER_TRUE_HPP_EAN_10_04_2005
#define BOOST_XPRESSIVE_DETAIL_UTILITY_NEVER_TRUE_HPP_EAN_10_04_2005

#include <boost/mpl/bool.hpp>

namespace boost { namespace xpressive { namespace detail
{

    // for use in static asserts
    template<typename T>
    struct never_true
      : mpl::false_
    {
    };

}}}

#endif
