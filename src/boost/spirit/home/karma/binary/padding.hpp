//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_KARMA_PADDING_MAY_06_2008_0436PM)
#define BOOST_SPIRIT_KARMA_PADDING_MAY_06_2008_0436PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/support/common_terminals.hpp>
#include <boost/spirit/home/support/info.hpp>

#include <boost/spirit/home/karma/domain.hpp>
#include <boost/spirit/home/karma/meta_compiler.hpp>
#include <boost/spirit/home/karma/delimit_out.hpp>
#include <boost/spirit/home/karma/auxiliary/lazy.hpp>
#include <boost/spirit/home/karma/detail/generate_to.hpp>
#include <boost/spirit/home/support/unused.hpp>
#include <boost/fusion/include/at.hpp>
#include <boost/fusion/include/vector.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit
{
    ///////////////////////////////////////////////////////////////////////////
    // Enablers
    ///////////////////////////////////////////////////////////////////////////

    // enables pad(...)
    template <typename A0>
    struct use_terminal<karma::domain
        , terminal_ex<tag::pad, fusion::vector1<A0> > > 
      : mpl::true_ {};

    // enables lazy pad(...)
    template <>
    struct use_lazy_terminal<karma::domain, tag::pad, 1>
      : mpl::true_ {};

}}

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit { namespace karma
{
#ifndef BOOST_SPIRIT_NO_PREDEFINED_TERMINALS
    using boost::spirit::pad;
#endif
    using boost::spirit::pad_type;

    struct binary_padding_generator 
      : primitive_generator<binary_padding_generator>
    {
        typedef mpl::int_<generator_properties::tracking> properties;

        template <typename Context, typename Unused>
        struct attribute
        {
            typedef unused_type type;
        };

        binary_padding_generator(int numpadbytes)
          : numpadbytes_(numpadbytes)
        {}

        template <
            typename OutputIterator, typename Context, typename Delimiter
          , typename Attribute>
        bool generate(OutputIterator& sink, Context&, Delimiter const& d
          , Attribute const& /*attr*/) const
        {
            std::size_t count = sink.get_out_count() % numpadbytes_;
            if (count)
                count = numpadbytes_ - count;

            bool result = true;
            while (result && count-- != 0)
                result = detail::generate_to(sink, '\0');

            if (result)
                result = karma::delimit_out(sink, d);  // always do post-delimiting
            return result;
        }

        template <typename Context>
        static info what(Context const&)
        {
            return info("pad");
        }

        int numpadbytes_;
    };

    ///////////////////////////////////////////////////////////////////////////
    // Generator generators: make_xxx function (objects)
    ///////////////////////////////////////////////////////////////////////////
    template <typename Modifiers, typename A0>
    struct make_primitive<
        terminal_ex<tag::pad, fusion::vector1<A0> >
      , Modifiers>
    {
        typedef binary_padding_generator result_type;

        template <typename Terminal>
        result_type operator()(Terminal const& term, unused_type) const
        {
            return result_type(fusion::at_c<0>(term.args));
        }
    };

}}}

#endif
