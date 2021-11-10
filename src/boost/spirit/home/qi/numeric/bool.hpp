/*=============================================================================
    Copyright (c) 2001-2011 Hartmut Kaiser
    Copyright (c) 2011      Bryce Lelbach

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef BOOST_SPIRIT_QI_NUMERIC_BOOL_HPP
#define BOOST_SPIRIT_QI_NUMERIC_BOOL_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/qi/skip_over.hpp>
#include <boost/spirit/home/qi/detail/enable_lit.hpp>
#include <boost/spirit/home/qi/meta_compiler.hpp>
#include <boost/spirit/home/qi/parser.hpp>
#include <boost/spirit/home/qi/numeric/bool_policies.hpp>
#include <boost/spirit/home/support/common_terminals.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/detail/workaround.hpp>
#include <boost/type_traits/is_same.hpp>

namespace boost { namespace spirit
{
    namespace qi
    {
        ///////////////////////////////////////////////////////////////////////
        // forward declaration only
        template <typename T>
        struct bool_policies;

        ///////////////////////////////////////////////////////////////////////
        // This is the class that the user can instantiate directly in
        // order to create a customized bool parser
        template <typename T, typename BoolPolicies = bool_policies<T> >
        struct bool_parser
          : spirit::terminal<tag::stateful_tag<BoolPolicies, tag::bool_, T> >
        {
            typedef tag::stateful_tag<BoolPolicies, tag::bool_, T> tag_type;

            bool_parser() {}
            bool_parser(BoolPolicies const& data)
              : spirit::terminal<tag_type>(data) {}
        };
    }

    ///////////////////////////////////////////////////////////////////////////
    // Enablers
    ///////////////////////////////////////////////////////////////////////////
    template <> // enables bool_
    struct use_terminal<qi::domain, tag::bool_>
      : mpl::true_ {};

    template <> // enables true_
    struct use_terminal<qi::domain, tag::true_>
      : mpl::true_ {};

    template <> // enables false_
    struct use_terminal<qi::domain, tag::false_>
      : mpl::true_ {};

    ///////////////////////////////////////////////////////////////////////////
    template <typename A0> // enables lit(...)
    struct use_terminal<qi::domain
        , terminal_ex<tag::lit, fusion::vector1<A0> >
        , typename enable_if<is_same<A0, bool> >::type>
      : mpl::true_ {};

    ///////////////////////////////////////////////////////////////////////////
    template <typename A0> // enables bool_(...)
    struct use_terminal<qi::domain
      , terminal_ex<tag::bool_, fusion::vector1<A0> >
    > : mpl::true_ {};

    template <> // enables *lazy* bool_(...)
    struct use_lazy_terminal<qi::domain, tag::bool_, 1>
      : mpl::true_ {};

    ///////////////////////////////////////////////////////////////////////////
    // enables any custom bool_parser
    template <typename T, typename BoolPolicies>
    struct use_terminal<qi::domain
        , tag::stateful_tag<BoolPolicies, tag::bool_, T> >
      : mpl::true_ {};

    // enables any custom bool_parser(...)
    template <typename T, typename BoolPolicies, typename A0>
    struct use_terminal<qi::domain
        , terminal_ex<tag::stateful_tag<BoolPolicies, tag::bool_, T>
        , fusion::vector1<A0> > >
      : mpl::true_ {};

    // enables *lazy* custom bool_parser(...)
    template <typename T, typename BoolPolicies>
    struct use_lazy_terminal<
        qi::domain
      , tag::stateful_tag<BoolPolicies, tag::bool_, T>
      , 1 // arity
    > : mpl::true_ {};
}}

namespace boost { namespace spirit { namespace qi
{
#ifndef BOOST_SPIRIT_NO_PREDEFINED_TERMINALS
    using spirit::bool_;
    using spirit::true_;
    using spirit::false_;
    using spirit::lit;    // lit(true) is equivalent to true
#endif
    using spirit::bool_type;
    using spirit::true_type;
    using spirit::false_type;
    using spirit::lit_type;

    namespace detail
    {
        template <typename T, typename BoolPolicies>
        struct bool_impl
        {
            template <typename Iterator, typename Attribute>
            static bool parse(Iterator& first, Iterator const& last
              , Attribute& attr, BoolPolicies const& p, bool allow_true = true
              , bool disallow_false = false)
            {
                if (first == last)
                    return false;

#if BOOST_WORKAROUND(BOOST_MSVC, BOOST_TESTED_AT(1600))
                p; // suppresses warning: C4100: 'p' : unreferenced formal parameter
#endif
                return (allow_true && p.parse_true(first, last, attr)) ||
                       (!disallow_false && p.parse_false(first, last, attr));
            }
        };
    }

    ///////////////////////////////////////////////////////////////////////////
    // This actual boolean parser
    ///////////////////////////////////////////////////////////////////////////
    template <typename T, typename BoolPolicies = bool_policies<T> >
    struct any_bool_parser
      : primitive_parser<any_bool_parser<T, BoolPolicies> >
    {
        template <typename Context, typename Iterator>
        struct attribute
        {
            typedef T type;
        };

        template <typename Iterator, typename Context
          , typename Skipper, typename Attribute>
        bool parse(Iterator& first, Iterator const& last
          , Context& /*context*/, Skipper const& skipper
          , Attribute& attr_) const
        {
            typedef detail::bool_impl<T, BoolPolicies> extract;
            qi::skip_over(first, last, skipper);
            return extract::parse(first, last, attr_, BoolPolicies());
        }

        template <typename Context>
        info what(Context& /*context*/) const
        {
            return info("boolean");
        }
    };

    template <typename T, typename BoolPolicies = bool_policies<T>
            , bool no_attribute = true>
    struct literal_bool_parser
      : primitive_parser<literal_bool_parser<T, BoolPolicies, no_attribute> >
    {
        template <typename Value>
        literal_bool_parser(Value const& n) : n_(n) {}

        template <typename Context, typename Iterator>
        struct attribute
          : mpl::if_c<no_attribute, unused_type, T>
        {};

        template <typename Iterator, typename Context
          , typename Skipper, typename Attribute>
        bool parse(Iterator& first, Iterator const& last
          , Context& /*context*/, Skipper const& skipper
          , Attribute& attr_) const
        {
            typedef detail::bool_impl<T, BoolPolicies> extract;
            qi::skip_over(first, last, skipper);
            return extract::parse(first, last, attr_, BoolPolicies(), n_, n_);
        }

        template <typename Context>
        info what(Context& /*context*/) const
        {
            return info("boolean");
        }

        T n_;
    };

    ///////////////////////////////////////////////////////////////////////////
    // Parser generators: make_xxx function (objects)
    ///////////////////////////////////////////////////////////////////////////
    template <typename T, typename Modifiers
            , typename Policies = bool_policies<T> >
    struct make_bool
    {
        typedef has_modifier<Modifiers, tag::char_code_base<tag::no_case> >
            no_case;

        typedef typename mpl::if_<
            mpl::and_<
                no_case
              , is_same<bool_policies<T>, Policies>
            >
          , any_bool_parser<T, no_case_bool_policies<T> >
          , any_bool_parser<T, Policies> >::type
        result_type;

        result_type operator()(unused_type, unused_type) const
        {
            return result_type();
        }
    };

    template <typename T, typename Modifiers
            , typename Policies = bool_policies<T> >
    struct make_direct_bool
    {
        typedef has_modifier<Modifiers, tag::char_code_base<tag::no_case> >
            no_case;

        typedef typename mpl::if_<
            mpl::and_<
                no_case
              , is_same<bool_policies<T>, Policies>
            >
          , literal_bool_parser<T, no_case_bool_policies<T>, false>
          , literal_bool_parser<T, Policies, false> >::type
        result_type;

        template <typename Terminal>
        result_type operator()(Terminal const& term, unused_type) const
        {
            return result_type(fusion::at_c<0>(term.args));
        }
    };

    template <typename T, typename Modifiers, bool b
            , typename Policies = bool_policies<T> >
    struct make_predefined_direct_bool
    {
        typedef has_modifier<Modifiers, tag::char_code_base<tag::no_case> >
            no_case;

        typedef typename mpl::if_<
            mpl::and_<
                no_case
              , is_same<bool_policies<T>, Policies>
            >
          , literal_bool_parser<T, no_case_bool_policies<T>, false>
          , literal_bool_parser<T, Policies, false> >::type
        result_type;

        result_type operator()(unused_type, unused_type) const
        {
            return result_type(b);
        }
    };

    template <typename T, typename Modifiers
            , typename Policies = bool_policies<T> >
    struct make_literal_bool
    {
        typedef has_modifier<Modifiers, tag::char_code_base<tag::no_case> >
            no_case;

        typedef typename mpl::if_<
            mpl::and_<
                no_case
              , is_same<bool_policies<T>, Policies>
            >
          , literal_bool_parser<T, no_case_bool_policies<T> >
          , literal_bool_parser<T, Policies> >::type
        result_type;

        template <typename Terminal>
        result_type operator()(Terminal const& term, unused_type) const
        {
            return result_type(fusion::at_c<0>(term.args));
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename Modifiers, typename A0>
    struct make_primitive<
          terminal_ex<tag::lit, fusion::vector1<A0> >
        , Modifiers, typename enable_if<is_same<A0, bool> >::type>
      : make_literal_bool<bool, Modifiers> {};

    ///////////////////////////////////////////////////////////////////////////
    template <typename Modifiers>
    struct make_primitive<tag::false_, Modifiers>
      : make_predefined_direct_bool<bool, Modifiers, false>
    {};

    template <typename Modifiers>
    struct make_primitive<tag::true_, Modifiers>
      : make_predefined_direct_bool<bool, Modifiers, true>
    {};

    ///////////////////////////////////////////////////////////////////////////
    template <typename T, typename Policies, typename Modifiers>
    struct make_primitive<
        tag::stateful_tag<Policies, tag::bool_, T>, Modifiers>
      : make_bool<T, Modifiers, Policies> {};

    template <typename T, typename Policies, typename A0, typename Modifiers>
    struct make_primitive<
        terminal_ex<tag::stateful_tag<Policies, tag::bool_, T>
      , fusion::vector1<A0> >, Modifiers>
      : make_direct_bool<T, Modifiers, Policies> {};

    ///////////////////////////////////////////////////////////////////////////
    template <typename Modifiers>
    struct make_primitive<tag::bool_, Modifiers>
      : make_bool<bool, Modifiers> {};

    template <typename Modifiers, typename A0>
    struct make_primitive<
        terminal_ex<tag::bool_
      , fusion::vector1<A0> >, Modifiers>
      : make_direct_bool<bool, Modifiers> {};
}}}

#endif
