//  Copyright (c) 2001-2011 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_KARMA_DELIMIT_FEB_20_2007_1208PM)
#define BOOST_SPIRIT_KARMA_DELIMIT_FEB_20_2007_1208PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/support/unused.hpp>
#include <boost/spirit/home/karma/detail/unused_delimiter.hpp>

namespace boost { namespace spirit { namespace karma
{
    ///////////////////////////////////////////////////////////////////////////
    //  Do delimiting. This is equivalent to p << d. The function is a
    //  no-op if spirit::unused is passed as the delimiter-generator.
    ///////////////////////////////////////////////////////////////////////////
    template <typename OutputIterator, typename Delimiter>
    inline bool delimit_out(OutputIterator& sink, Delimiter const& d)
    {
        return d.generate(sink, unused, unused, unused);
    }

    template <typename OutputIterator>
    inline bool delimit_out(OutputIterator&, unused_type)
    {
        return true;
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename OutputIterator, typename Delimiter>
    inline bool delimit_out(OutputIterator&
      , detail::unused_delimiter<Delimiter> const&)
    {
        return true;
    }

}}}

#endif

