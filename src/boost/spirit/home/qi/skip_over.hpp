/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_SPIRIT_SKIP_APRIL_16_2006_0625PM)
#define BOOST_SPIRIT_SKIP_APRIL_16_2006_0625PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/support/unused.hpp>
#include <boost/spirit/home/qi/detail/unused_skipper.hpp>

namespace boost { namespace spirit { namespace qi
{
    ///////////////////////////////////////////////////////////////////////////
    // Move the /first/ iterator to the first non-matching position
    // given a skip-parser. The function is a no-op if unused_type is
    // passed as the skip-parser.
    ///////////////////////////////////////////////////////////////////////////
    template <typename Iterator, typename T>
    inline void skip_over(Iterator& first, Iterator const& last, T const& skipper)
    {
        while (skipper.parse(first, last, unused, unused, unused))
            /***/;
    }

    template <typename Iterator>
    inline void skip_over(Iterator&, Iterator const&, unused_type)
    {
    }

    template <typename Iterator, typename Skipper>
    inline void skip_over(Iterator&, Iterator const&
      , detail::unused_skipper<Skipper> const&)
    {
    }

}}}

#endif
