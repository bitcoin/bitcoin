//  Copyright (c) 2001-2011 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_KARMA_MAXWIDTH_MAR_18_2009_0827AM)
#define BOOST_SPIRIT_KARMA_MAXWIDTH_MAR_18_2009_0827AM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/karma/meta_compiler.hpp>
#include <boost/spirit/home/karma/generator.hpp>
#include <boost/spirit/home/karma/domain.hpp>
#include <boost/spirit/home/karma/detail/output_iterator.hpp>
#include <boost/spirit/home/karma/detail/default_width.hpp>
#include <boost/spirit/home/karma/delimit_out.hpp>
#include <boost/spirit/home/karma/auxiliary/lazy.hpp>
#include <boost/spirit/home/support/unused.hpp>
#include <boost/spirit/home/support/common_terminals.hpp>
#include <boost/spirit/home/support/has_semantic_action.hpp>
#include <boost/spirit/home/support/handles_container.hpp>
#include <boost/spirit/home/karma/detail/attributes.hpp>
#include <boost/spirit/home/support/info.hpp>
#include <boost/spirit/home/support/unused.hpp>
#include <boost/fusion/include/at.hpp>
#include <boost/fusion/include/vector.hpp>
#include <boost/detail/workaround.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit
{
    ///////////////////////////////////////////////////////////////////////////
    // Enablers
    ///////////////////////////////////////////////////////////////////////////

    // enables maxwidth[]
    template <>
    struct use_directive<karma::domain, tag::maxwidth>
      : mpl::true_ {};

    // enables maxwidth(w)[g], where w provides a maxwidth
    template <typename T>
    struct use_directive<karma::domain
          , terminal_ex<tag::maxwidth, fusion::vector1<T> > > 
      : mpl::true_ {};

    // enables *lazy* maxwidth(w)[g], where w provides a maxwidth
    template <>
    struct use_lazy_directive<karma::domain, tag::maxwidth, 1> 
      : mpl::true_ {};

    // enables maxwidth(w, r)[g], where w provides a maxwidth and r is an output
    // iterator used to receive the rest of the output not fitting into the 
    // maxwidth limit
    template <typename T, typename RestIter>
    struct use_directive<karma::domain
          , terminal_ex<tag::maxwidth, fusion::vector2<T, RestIter> > > 
      : mpl::true_ {};

    // enables *lazy* maxwidth(w, r)[g], where w provides a maxwidth and r is 
    // an output iterator used to receive the rest of the output not fitting 
    // into the maxwidth limit
    template <>
    struct use_lazy_directive<karma::domain, tag::maxwidth, 2> 
      : mpl::true_ {};

}}

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit { namespace karma
{
#ifndef BOOST_SPIRIT_NO_PREDEFINED_TERMINALS
    using spirit::maxwidth;
#endif
    using spirit::maxwidth_type;

    namespace detail
    {
        ///////////////////////////////////////////////////////////////////////
        template <typename OutputIterator, typename RestIterator>
        bool buffer_copy_rest(detail::enable_buffering<OutputIterator>& buff
          , std::size_t start_at, RestIterator& dest)
        {
            return buff.buffer_copy_rest(dest, start_at);
        }

        template <typename OutputIterator>
        bool buffer_copy_rest(detail::enable_buffering<OutputIterator>&
          , std::size_t, unused_type)
        {
            return true;
        }

        ///////////////////////////////////////////////////////////////////////
        //  The maxwidth_generate template function is used for all the 
        //  different flavors of the maxwidth[] directive. 
        ///////////////////////////////////////////////////////////////////////
        template <typename OutputIterator, typename Context, typename Delimiter, 
            typename Attribute, typename Embedded, typename Rest>
        inline static bool 
        maxwidth_generate(OutputIterator& sink, Context& ctx, 
            Delimiter const& d, Attribute const& attr, Embedded const& e, 
            unsigned int const maxwidth, Rest& restdest) 
        {
#if BOOST_WORKAROUND(BOOST_MSVC, BOOST_TESTED_AT(1600))
            e; // suppresses warning: C4100: 'e' : unreferenced formal parameter
#endif
            // wrap the given output iterator to allow buffering, but disable 
            // counting
            detail::enable_buffering<OutputIterator> buffering(sink);

            // generate the underlying output and copy the embedded 
            // output to the target output iterator applying the given 
            // maxwidth
            bool r = false;
            {
                detail::disable_counting<OutputIterator> nocounting(sink);
                r = e.generate(sink, ctx, d, attr);
            }   // re-enable counting

            return r && buffering.buffer_copy(maxwidth) &&
                   buffer_copy_rest(buffering, maxwidth, restdest);
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    //  The maxwidth directive is used for maxwidth[...]
    //  generators. It uses default values for the generated width (defined via
    //  the BOOST_KARMA_DEFAULT_FIELD_MAXWIDTH constant).
    // 
    //  The maxwidth with width directive, is used for generators
    //  like maxwidth(width)[...]. 
    ///////////////////////////////////////////////////////////////////////////
    template <typename Subject, typename Width = detail::default_max_width
      , typename Rest = unused_type>
    struct maxwidth_width
      : unary_generator<maxwidth_width<Subject, Width, Rest> >
    {
        typedef Subject subject_type;

        typedef mpl::int_<
            generator_properties::countingbuffer | subject_type::properties::value
        > properties;

        template <typename Context, typename Iterator>
        struct attribute
          : traits::attribute_of<subject_type, Context, Iterator>
        {};

        maxwidth_width(Subject const& subject, Width const& w = Width()
            , Rest const& r = Rest())
          : subject(subject), width(w), rest(r) {}

        template <typename OutputIterator, typename Context, typename Delimiter
          , typename Attribute>
        bool generate(OutputIterator& sink, Context& ctx, Delimiter const& d
          , Attribute const& attr) const
        {
            return detail::maxwidth_generate(sink, ctx, d, attr, subject
              , width, rest);
        }

        template <typename Context>
        info what(Context& context) const
        {
            return info("maxwidth", subject.what(context));
        }

        Subject subject;
        Width width;
        Rest rest;
    };

    ///////////////////////////////////////////////////////////////////////////
    // Generator generators: make_xxx function (objects)
    ///////////////////////////////////////////////////////////////////////////

    // creates maxwidth[] directive generator
    template <typename Subject, typename Modifiers>
    struct make_directive<tag::maxwidth, Subject, Modifiers>
    {
        typedef maxwidth_width<Subject> result_type;
        result_type operator()(unused_type, Subject const& subject
          , unused_type) const
        {
            return result_type(subject);
        }
    };

    // creates maxwidth(width)[] directive generator
    template <typename T, typename Subject, typename Modifiers>
    struct make_directive<
        terminal_ex<tag::maxwidth, fusion::vector1<T> >
      , Subject, Modifiers>
    {
        typedef maxwidth_width<Subject, T> result_type;

        template <typename Terminal>
        result_type operator()(Terminal const& term, Subject const& subject
          , unused_type) const
        {
            return result_type(subject, fusion::at_c<0>(term.args), unused);
        }
    };

    // creates maxwidth(width, restiter)[] directive generator
    template <
        typename T, typename RestIter, typename Subject, typename Modifiers>
    struct make_directive<
        terminal_ex<tag::maxwidth, fusion::vector2<T, RestIter> >
      , Subject, Modifiers>
    {
        typedef maxwidth_width<Subject, T, RestIter> result_type;

        template <typename Terminal>
        result_type operator()(Terminal const& term, Subject const& subject
          , unused_type) const
        {
            return result_type(subject, fusion::at_c<0>(term.args)
              , fusion::at_c<1>(term.args));
        }
    };

}}} // namespace boost::spirit::karma

namespace boost { namespace spirit { namespace traits
{
    ///////////////////////////////////////////////////////////////////////////
    template <typename Subject, typename Width, typename Rest>
    struct has_semantic_action<karma::maxwidth_width<Subject, Width, Rest> >
      : unary_has_semantic_action<Subject> {};

    ///////////////////////////////////////////////////////////////////////////
    template <typename Subject, typename Attribute, typename Context
        , typename Iterator>
    struct handles_container<karma::maxwidth_width<Subject>, Attribute
        , Context, Iterator>
      : unary_handles_container<Subject, Attribute, Context, Iterator> {};
}}}

#endif


