/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_SPIRIT_GRAMMAR_FEBRUARY_19_2007_0236PM)
#define BOOST_SPIRIT_GRAMMAR_FEBRUARY_19_2007_0236PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/support/unused.hpp>
#include <boost/spirit/home/support/info.hpp>
#include <boost/spirit/home/support/assert_msg.hpp>
#include <boost/spirit/home/qi/domain.hpp>
#include <boost/spirit/home/qi/nonterminal/rule.hpp>
#include <boost/spirit/home/qi/nonterminal/nonterminal_fwd.hpp>
#include <boost/spirit/home/qi/reference.hpp>
#include <boost/noncopyable.hpp>
#include <boost/proto/extends.hpp>
#include <boost/proto/traits.hpp>
#include <boost/type_traits/is_same.hpp>

namespace boost { namespace spirit { namespace qi
{
    template <
        typename Iterator, typename T1, typename T2, typename T3
      , typename T4>
    struct grammar
      : proto::extends<
            typename proto::terminal<
                reference<rule<Iterator, T1, T2, T3, T4> const>
            >::type
          , grammar<Iterator, T1, T2, T3, T4>
        >
      , parser<grammar<Iterator, T1, T2, T3, T4> >
      , noncopyable
    {
        typedef Iterator iterator_type;
        typedef rule<Iterator, T1, T2, T3, T4> start_type;
        typedef typename start_type::sig_type sig_type;
        typedef typename start_type::locals_type locals_type;
        typedef typename start_type::skipper_type skipper_type;
        typedef typename start_type::encoding_type encoding_type;
        typedef grammar<Iterator, T1, T2, T3, T4> base_type;
        typedef reference<start_type const> reference_;
        typedef typename proto::terminal<reference_>::type terminal;

        static size_t const params_size = start_type::params_size;

        template <typename Context, typename Iterator_>
        struct attribute
        {
            typedef typename start_type::attr_type type;
        };

        grammar(
            start_type const& start
          , std::string const& name = "unnamed-grammar")
        : proto::extends<terminal, base_type>(terminal::make(reference_(start)))
        , name_(name)
        {}

        // This constructor is used to catch if the start rule is not
        // compatible with the grammar.
        template <typename Iterator_,
            typename T1_, typename T2_, typename T3_, typename T4_>
        grammar(
            rule<Iterator_, T1_, T2_, T3_, T4_> const&
          , std::string const& = "unnamed-grammar")
        {
            // If you see the assertion below failing then the start rule
            // passed to the constructor of the grammar is not compatible with
            // the grammar (i.e. it uses different template parameters).
            BOOST_SPIRIT_ASSERT_MSG(
                (is_same<start_type, rule<Iterator_, T1_, T2_, T3_, T4_> >::value)
              , incompatible_start_rule, (rule<Iterator_, T1_, T2_, T3_, T4_>));
        }

        std::string name() const
        {
            return name_;
        }

        void name(std::string const& str)
        {
            name_ = str;
        }

        template <typename Context, typename Skipper, typename Attribute>
        bool parse(Iterator& first, Iterator const& last
          , Context& context, Skipper const& skipper
          , Attribute& attr_) const
        {
            return this->proto_base().child0.parse(
                first, last, context, skipper, attr_);
        }

        template <typename Context>
        info what(Context&) const
        {
            return info(name_);
        }

        // bring in the operator() overloads
        start_type const& get_parameterized_subject() const
        { return this->proto_base().child0.ref.get(); }
        typedef start_type parameterized_subject_type;
        #include <boost/spirit/home/qi/nonterminal/detail/fcall.hpp>

        std::string name_;

    };
}}}

namespace boost { namespace spirit { namespace traits
{
    ///////////////////////////////////////////////////////////////////////////
    template <
        typename IteratorA, typename IteratorB, typename Attribute
      , typename Context, typename T1, typename T2, typename T3, typename T4>
    struct handles_container<
        qi::grammar<IteratorA, T1, T2, T3, T4>, Attribute, Context, IteratorB>
      : traits::is_container<
          typename attribute_of<
              qi::grammar<IteratorA, T1, T2, T3, T4>, Context, IteratorB
          >::type
        >
    {};
}}}

#endif
