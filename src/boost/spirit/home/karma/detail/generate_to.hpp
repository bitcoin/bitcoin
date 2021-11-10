//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_KARMA_DETAIL_GENERATE_TO_FEB_20_2007_0417PM)
#define BOOST_SPIRIT_KARMA_DETAIL_GENERATE_TO_FEB_20_2007_0417PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/support/unused.hpp>
#include <boost/spirit/home/support/char_class.hpp>
#include <boost/spirit/home/karma/detail/output_iterator.hpp>

namespace boost { namespace spirit { namespace karma { namespace detail 
{
    ///////////////////////////////////////////////////////////////////////////
    //  These utility functions insert the given parameter into the supplied 
    //  output iterator.
    //  If the attribute is spirit's unused_type, this is a no_op.
    ///////////////////////////////////////////////////////////////////////////
    template <
        typename OutputIterator, typename Attribute, typename CharEncoding
      , typename Tag>
    inline bool 
    generate_to(OutputIterator& sink, Attribute const& p, CharEncoding, Tag)
    {
        *sink = spirit::char_class::convert<CharEncoding>::to(Tag(), p);
        ++sink;
        return detail::sink_is_good(sink);
    }

    template <typename OutputIterator, typename Attribute>
    inline bool 
    generate_to(OutputIterator& sink, Attribute const& p, unused_type, unused_type)
    {
        *sink = p;
        ++sink;
        return detail::sink_is_good(sink);
    }

    template <typename OutputIterator, typename CharEncoding, typename Tag>
    inline bool generate_to(OutputIterator&, unused_type, CharEncoding, Tag)
    {
        return true;
    }

    template <typename OutputIterator, typename Attribute>
    inline bool 
    generate_to(OutputIterator& sink, Attribute const& p)
    {
        *sink = p;
        ++sink;
        return detail::sink_is_good(sink);
    }

    template <typename OutputIterator>
    inline bool generate_to(OutputIterator&, unused_type)
    {
        return true;
    }

}}}}   // namespace boost::spirit::karma::detail

#endif  // KARMA_CORE_DETAIL_INSERT_TO_HPP
