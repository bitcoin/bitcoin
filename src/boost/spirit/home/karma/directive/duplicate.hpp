//  Copyright (c) 2001-2011 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_SPIRIT_KARMA_DIRECTIVE_DUPLICATE_HPP
#define BOOST_SPIRIT_KARMA_DIRECTIVE_DUPLICATE_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/karma/meta_compiler.hpp>
#include <boost/spirit/home/karma/generator.hpp>
#include <boost/spirit/home/karma/domain.hpp>
#include <boost/spirit/home/karma/detail/attributes.hpp>
#include <boost/spirit/home/support/unused.hpp>
#include <boost/spirit/home/support/info.hpp>
#include <boost/spirit/home/support/common_terminals.hpp>
#include <boost/spirit/home/support/assert_msg.hpp>
#include <boost/spirit/home/support/has_semantic_action.hpp>
#include <boost/spirit/home/support/handles_container.hpp>
#include <boost/fusion/include/cons.hpp>
#include <boost/fusion/include/make_cons.hpp>
#include <boost/fusion/include/vector.hpp>
#include <boost/fusion/include/at_c.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/mpl/bool.hpp>

namespace boost { namespace spirit
{
    ///////////////////////////////////////////////////////////////////////////
    // Enablers
    ///////////////////////////////////////////////////////////////////////////
    template <>
    struct use_directive<karma::domain, tag::duplicate> // enables duplicate
      : mpl::true_ {};
}}

namespace boost { namespace spirit { namespace karma
{
#ifndef BOOST_SPIRIT_NO_PREDEFINED_TERMINALS
    using spirit::duplicate;
#endif
    using spirit::duplicate_type;

    ///////////////////////////////////////////////////////////////////////////
    namespace detail
    {
        ///////////////////////////////////////////////////////////////////////
        template <typename T
          , bool IsSequence = fusion::traits::is_sequence<T>::value>
        struct attribute_count
          : fusion::result_of::size<T>
        {};

        template <>
        struct attribute_count<unused_type, false>
          : mpl::int_<0>
        {};

        template <typename T>
        struct attribute_count<T, false>
          : mpl::int_<1>
        {};

        ///////////////////////////////////////////////////////////////////////
        template <typename T
          , bool IsSequence = fusion::traits::is_sequence<T>::value>
        struct first_attribute_of_subject
          : remove_reference<typename fusion::result_of::at_c<T, 0>::type>
        {};

        template <typename T>
        struct first_attribute_of_subject<T, false>
          : mpl::identity<T>
        {};

        template <typename T, typename Context, typename Iterator>
        struct first_attribute_of
          : first_attribute_of_subject<
                typename traits::attribute_of<T, Context, Iterator>::type>
        {};

        ///////////////////////////////////////////////////////////////////////
        template <typename Attribute, typename T, int N>
        struct duplicate_sequence_attribute
        {
            typedef typename fusion::result_of::make_cons<
                reference_wrapper<T const>
              , typename duplicate_sequence_attribute<Attribute, T, N-1>::type
            >::type type;

            static type call(T const& t)
            {
                return fusion::make_cons(boost::cref(t)
                  , duplicate_sequence_attribute<Attribute, T, N-1>::call(t));
            }
        };

        template <typename Attribute, typename T>
        struct duplicate_sequence_attribute<Attribute, T, 1>
        {
            typedef typename fusion::result_of::make_cons<
                reference_wrapper<T const> >::type type;

            static type call(T const& t)
            {
                return fusion::make_cons(boost::cref(t));
            }
        };

        ///////////////////////////////////////////////////////////////////////
        template <typename Attribute, typename T
          , int N = attribute_count<Attribute>::value
          , bool IsSequence = fusion::traits::is_sequence<Attribute>::value>
        struct duplicate_attribute
        {
            BOOST_SPIRIT_ASSERT_MSG(N > 0, invalid_duplication_count, (Attribute));

            typedef typename duplicate_sequence_attribute<Attribute, T, N>::type
                cons_type;
            typedef typename fusion::result_of::as_vector<cons_type>::type type;

            static type call(T const& t)
            {
                return fusion::as_vector(
                    duplicate_sequence_attribute<Attribute, T, N>::call(t));
            }
        };

        template <typename Attribute, typename T>
        struct duplicate_attribute<Attribute, T, 0, false>
        {
            typedef unused_type type;

            static type call(T const&)
            {
                return unused;
            }
        };

        template <typename Attribute, typename T, int N>
        struct duplicate_attribute<Attribute, T, N, false>
        {
            typedef Attribute const& type;

            static type call(T const& t)
            {
                return t;
            }
        };
    }

    template <typename Attribute, typename T>
    inline typename detail::duplicate_attribute<Attribute, T>::type
    duplicate_attribute(T const& t)
    {
        return detail::duplicate_attribute<Attribute, T>::call(t);
    }

    ///////////////////////////////////////////////////////////////////////////
    // duplicate_directive duplicate its attribute for all elements of the
    // subject generator without generating anything itself
    ///////////////////////////////////////////////////////////////////////////
    template <typename Subject>
    struct duplicate_directive : unary_generator<duplicate_directive<Subject> >
    {
        typedef Subject subject_type;
        typedef typename subject_type::properties properties;

        duplicate_directive(Subject const& subject)
          : subject(subject) {}

        template <typename Context, typename Iterator = unused_type>
        struct attribute
          : detail::first_attribute_of<Subject, Context, Iterator>
        {};

        template <typename OutputIterator, typename Context, typename Delimiter
          , typename Attribute>
        bool generate(OutputIterator& sink, Context& ctx, Delimiter const& d
          , Attribute const& attr) const
        {
            typedef typename traits::attribute_of<Subject, Context>::type
                subject_attr_type;
            return subject.generate(sink, ctx, d
              , duplicate_attribute<subject_attr_type>(attr));
        }

        template <typename Context>
        info what(Context& context) const
        {
            return info("duplicate", subject.what(context));
        }

        Subject subject;
    };

    ///////////////////////////////////////////////////////////////////////////
    // Generator generators: make_xxx function (objects)
    ///////////////////////////////////////////////////////////////////////////
    template <typename Subject, typename Modifiers>
    struct make_directive<tag::duplicate, Subject, Modifiers>
    {
        typedef duplicate_directive<Subject> result_type;
        result_type operator()(unused_type, Subject const& subject
          , unused_type) const
        {
            return result_type(subject);
        }
    };
}}}

namespace boost { namespace spirit { namespace traits
{
    ///////////////////////////////////////////////////////////////////////////
    template <typename Subject>
    struct has_semantic_action<karma::duplicate_directive<Subject> >
      : unary_has_semantic_action<Subject> {};

    ///////////////////////////////////////////////////////////////////////////
    template <typename Subject, typename Attribute, typename Context
        , typename Iterator>
    struct handles_container<karma::duplicate_directive<Subject>, Attribute
        , Context, Iterator>
      : unary_handles_container<Subject, Attribute, Context, Iterator> {};
}}}

#endif
