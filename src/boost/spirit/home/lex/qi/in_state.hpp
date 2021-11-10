//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_LEX_IN_STATE_OCT_09_2007_0748PM)
#define BOOST_SPIRIT_LEX_IN_STATE_OCT_09_2007_0748PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/proto/traits.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit { namespace qi
{
    ///////////////////////////////////////////////////////////////////////////
    // The following is a helper template allowing to use the in_state()[] as 
    // a skip parser
    ///////////////////////////////////////////////////////////////////////////
    template <typename Skipper, typename String = char const*>
    struct in_state_skipper
      : proto::subscript<
            typename proto::terminal<
                terminal_ex<tag::in_state, fusion::vector1<String> > 
            >::type
          , Skipper
        >::type {};
}}}

#endif
