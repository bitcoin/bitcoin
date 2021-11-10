//  Copyright (c) 2001-2011 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_KARMA_RIGHT_ALIGNMENT_FEB_27_2007_1216PM)
#define BOOST_SPIRIT_KARMA_RIGHT_ALIGNMENT_FEB_27_2007_1216PM

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
#include <boost/integer_traits.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/detail/workaround.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit
{
    ///////////////////////////////////////////////////////////////////////////
    // Enablers
    ///////////////////////////////////////////////////////////////////////////

    // enables right_align[]
    template <>
    struct use_directive<karma::domain, tag::right_align>
      : mpl::true_ {};

    // enables right_align(d)[g] and right_align(w)[g], where d is a generator
    // and w is a maximum width
    template <typename T>
    struct use_directive<karma::domain
          , terminal_ex<tag::right_align, fusion::vector1<T> > >
      : mpl::true_ {};

    // enables *lazy* right_align(d)[g], where d provides a generator
    template <>
    struct use_lazy_directive<karma::domain, tag::right_align, 1> 
      : mpl::true_ {};

    // enables right_align(w, d)[g], where d is a generator and w is a maximum 
    // width
    template <typename Width, typename Padding>
    struct use_directive<karma::domain
          , terminal_ex<tag::right_align, fusion::vector2<Width, Padding> > >
      : spirit::traits::matches<karma::domain, Padding> {};

    // enables *lazy* right_align(w, d)[g], where d provides a generator and w 
    // is a maximum width
    template <>
    struct use_lazy_directive<karma::domain, tag::right_align, 2> 
      : mpl::true_ {};

}}

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit { namespace karma
{
#ifndef BOOST_SPIRIT_NO_PREDEFINED_TERMINALS
    using spirit::right_align;
#endif
    using spirit::right_align_type;

    namespace detail
    {
        ///////////////////////////////////////////////////////////////////////
        //  The right_align_generate template function is used for all the 
        //  different flavors of the right_align[] directive. 
        ///////////////////////////////////////////////////////////////////////
        template <typename OutputIterator, typename Context, typename Delimiter, 
            typename Attribute, typename Embedded, typename Padding>
        inline static bool 
        right_align_generate(OutputIterator& sink, Context& ctx, 
            Delimiter const& d, Attribute const& attr, Embedded const& e, 
            unsigned int const width, Padding const& p) 
        {
#if BOOST_WORKAROUND(BOOST_MSVC, BOOST_TESTED_AT(1600))
            e; // suppresses warning: C4100: 'e' : unreferenced formal parameter
#endif
            // wrap the given output iterator to allow left padding
            detail::enable_buffering<OutputIterator> buffering(sink, width);
            bool r = false;

            // first generate the embedded output 
            {
                detail::disable_counting<OutputIterator> nocounting(sink);
                r = e.generate(sink, ctx, d, attr);
            }   // re-enable counting

            buffering.disable();    // do not perform buffering any more

            // generate the left padding
            detail::enable_counting<OutputIterator> counting(sink, buffering.buffer_size());
            while(r && counting.count() < width) 
                r = p.generate(sink, ctx, unused, unused);

            // copy the buffered output to the target output iterator
            if (r) 
                buffering.buffer_copy();
            return r;
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    //  The simple left alignment directive is used for right_align[...]
    //  generators. It uses default values for the generated width (defined via
    //  the BOOST_KARMA_DEFAULT_FIELD_LENGTH constant) and for the padding
    //  generator (always spaces).
    ///////////////////////////////////////////////////////////////////////////
    template <typename Subject, typename Width = detail::default_width>
    struct simple_right_alignment 
      : unary_generator<simple_right_alignment<Subject, Width> >
    {
        typedef Subject subject_type;

        typedef mpl::int_<
            generator_properties::countingbuffer | subject_type::properties::value
        > properties;

        template <typename Context, typename Iterator>
        struct attribute
          : traits::attribute_of<subject_type, Context, Iterator>
        {};

        simple_right_alignment(Subject const& subject, Width width = Width())
          : subject(subject), width(width) {}

        template <typename OutputIterator, typename Context, typename Delimiter
          , typename Attribute>
        bool generate(OutputIterator& sink, Context& ctx, Delimiter const& d
          , Attribute const& attr) const
        {
            return detail::right_align_generate(sink, ctx, d, attr,
                subject, width, compile<karma::domain>(' '));
        }

        template <typename Context>
        info what(Context& context) const
        {
            return info("right_align", subject.what(context));
        }

        Subject subject;
        Width width;
    };

    ///////////////////////////////////////////////////////////////////////////
    //  The left alignment directive with padding, is used for generators like
    //  right_align(padding)[...], where padding is a arbitrary generator
    //  expression. It uses a default value for the generated width (defined
    //  via the BOOST_KARMA_DEFAULT_FIELD_LENGTH constant).
    ///////////////////////////////////////////////////////////////////////////
    template <typename Subject, typename Padding
      , typename Width = detail::default_width>
    struct padding_right_alignment 
      : unary_generator<padding_right_alignment<Subject, Padding, Width> >
    {
        typedef Subject subject_type;
        typedef Padding padding_type;

        typedef mpl::int_<
            generator_properties::countingbuffer |
            subject_type::properties::value | padding_type::properties::value 
        > properties;

        template <typename Context, typename Iterator>
        struct attribute
          : traits::attribute_of<subject_type, Context, Iterator>
        {};

        padding_right_alignment(Subject const& subject, Padding const& padding
              , Width width = Width())
          : subject(subject), padding(padding), width(width) {}

        template <typename OutputIterator, typename Context, typename Delimiter
          , typename Attribute>
        bool generate(OutputIterator& sink, Context& ctx, Delimiter const& d
          , Attribute const& attr) const
        {
            return detail::right_align_generate(sink, ctx, d, attr,
                subject, width, padding);
        }

        template <typename Context>
        info what(Context& context) const
        {
            return info("right_align", subject.what(context));
        }

        Subject subject;
        Padding padding;
        Width width;
    };

    ///////////////////////////////////////////////////////////////////////////
    // Generator generators: make_xxx function (objects)
    ///////////////////////////////////////////////////////////////////////////

    // creates right_align[] directive generator
    template <typename Subject, typename Modifiers>
    struct make_directive<tag::right_align, Subject, Modifiers>
    {
        typedef simple_right_alignment<Subject> result_type;
        result_type operator()(unused_type, Subject const& subject
          , unused_type) const
        {
            return result_type(subject);
        }
    };

    // creates right_align(width)[] directive generator
    template <typename Width, typename Subject, typename Modifiers>
    struct make_directive<
        terminal_ex<tag::right_align, fusion::vector1<Width> >
      , Subject, Modifiers
      , typename enable_if_c< integer_traits<Width>::is_integral >::type>
    {
        typedef simple_right_alignment<Subject, Width> result_type;

        template <typename Terminal>
        result_type operator()(Terminal const& term, Subject const& subject
          , unused_type) const
        {
            return result_type(subject, fusion::at_c<0>(term.args));
        }
    };

    // creates right_align(pad)[] directive generator
    template <typename Padding, typename Subject, typename Modifiers>
    struct make_directive<
        terminal_ex<tag::right_align, fusion::vector1<Padding> >
      , Subject, Modifiers
      , typename enable_if<
            mpl::and_<
                spirit::traits::matches<karma::domain, Padding>,
                mpl::not_<mpl::bool_<integer_traits<Padding>::is_integral> >
            >
        >::type>
    {
        typedef typename
            result_of::compile<karma::domain, Padding, Modifiers>::type
        padding_type;

        typedef padding_right_alignment<Subject, padding_type> result_type;

        template <typename Terminal>
        result_type operator()(Terminal const& term, Subject const& subject
          , Modifiers const& modifiers) const
        {
            return result_type(subject
              , compile<karma::domain>(fusion::at_c<0>(term.args), modifiers));
        }
    };

    // creates right_align(width, pad)[] directive generator
    template <typename Width, typename Padding, typename Subject
      , typename Modifiers>
    struct make_directive<
        terminal_ex<tag::right_align, fusion::vector2<Width, Padding> >
      , Subject, Modifiers>
    {
        typedef typename
            result_of::compile<karma::domain, Padding, Modifiers>::type
        padding_type;

        typedef padding_right_alignment<Subject, padding_type, Width> result_type;

        template <typename Terminal>
        result_type operator()(Terminal const& term, Subject const& subject
          , Modifiers const& modifiers) const
        {
            return result_type(subject
              , compile<karma::domain>(fusion::at_c<1>(term.args), modifiers)
              , fusion::at_c<0>(term.args));
        }
    };

}}} // namespace boost::spirit::karma

namespace boost { namespace spirit { namespace traits
{
    ///////////////////////////////////////////////////////////////////////////
    template <typename Subject, typename Width>
    struct has_semantic_action<karma::simple_right_alignment<Subject, Width> >
      : unary_has_semantic_action<Subject> {};

    template <typename Subject, typename Padding, typename Width>
    struct has_semantic_action<
            karma::padding_right_alignment<Subject, Padding, Width> >
      : unary_has_semantic_action<Subject> {};

    ///////////////////////////////////////////////////////////////////////////
    template <typename Subject, typename Width, typename Attribute
      , typename Context, typename Iterator>
    struct handles_container<
            karma::simple_right_alignment<Subject, Width>
          , Attribute, Context, Iterator>
      : unary_handles_container<Subject, Attribute, Context, Iterator> {};

    template <typename Subject, typename Padding, typename Width
      , typename Attribute, typename Context, typename Iterator>
    struct handles_container<
            karma::padding_right_alignment<Subject, Padding, Width>
          , Attribute, Context, Iterator>
      : unary_handles_container<Subject, Attribute, Context, Iterator> {};
}}}

#endif

