/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c)      2011 Bryce Lelbach

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_SPIRIT_INT_APR_17_2006_0830AM)
#define BOOST_SPIRIT_INT_APR_17_2006_0830AM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/qi/skip_over.hpp>
#include <boost/spirit/home/qi/detail/enable_lit.hpp>
#include <boost/spirit/home/qi/numeric/numeric_utils.hpp>
#include <boost/spirit/home/qi/meta_compiler.hpp>
#include <boost/spirit/home/qi/parser.hpp>
#include <boost/spirit/home/support/common_terminals.hpp>
#include <boost/spirit/home/support/info.hpp>
#include <boost/spirit/home/support/detail/is_spirit_tag.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/type_traits/is_same.hpp>

namespace boost { namespace spirit
{
    namespace tag
    {
        template <typename T, unsigned Radix, unsigned MinDigits
                , int MaxDigits>
        struct int_parser 
        {
            BOOST_SPIRIT_IS_TAG()
        };
    }

    namespace qi
    {
        ///////////////////////////////////////////////////////////////////////
        // This one is the class that the user can instantiate directly in
        // order to create a customized int parser
        template <typename T = int, unsigned Radix = 10, unsigned MinDigits = 1
                , int MaxDigits = -1>
        struct int_parser
          : spirit::terminal<tag::int_parser<T, Radix, MinDigits, MaxDigits> >
        {};
    }

    ///////////////////////////////////////////////////////////////////////////
    // Enablers
    ///////////////////////////////////////////////////////////////////////////
    //[primitive_parsers_enable_short
    template <> // enables short_
    struct use_terminal<qi::domain, tag::short_> : mpl::true_ {};
    //]

    template <typename A0> // enables lit(n)
    struct use_terminal<qi::domain
        , terminal_ex<tag::lit, fusion::vector1<A0> >
        , typename enable_if<is_same<A0, signed short> >::type>
      : mpl::true_ {};

    template <typename A0> // enables short_(n)
    struct use_terminal<qi::domain
        , terminal_ex<tag::short_, fusion::vector1<A0> > >
      : is_arithmetic<A0> {};

    template <> // enables *lazy* short_(n)
    struct use_lazy_terminal<qi::domain, tag::short_, 1> : mpl::true_ {};

    ///////////////////////////////////////////////////////////////////////////
    //[primitive_parsers_enable_int
    template <> // enables int_
    struct use_terminal<qi::domain, tag::int_> : mpl::true_ {};
    //]

    template <typename A0> // enables lit(n)
    struct use_terminal<qi::domain
        , terminal_ex<tag::lit, fusion::vector1<A0> >
        , typename enable_if<is_same<A0, signed> >::type>
      : mpl::true_ {};

    template <typename A0> // enables int_(n)
    struct use_terminal<qi::domain
        , terminal_ex<tag::int_, fusion::vector1<A0> > >
      : is_arithmetic<A0> {};

    template <> // enables *lazy* int_(n)
    struct use_lazy_terminal<qi::domain, tag::int_, 1> : mpl::true_ {};

    ///////////////////////////////////////////////////////////////////////////
    //[primitive_parsers_enable_long
    template <> // enables long_
    struct use_terminal<qi::domain, tag::long_> : mpl::true_ {};
    //]

    template <typename A0> // enables lit(n)
    struct use_terminal<qi::domain
        , terminal_ex<tag::lit, fusion::vector1<A0> >
        , typename enable_if<is_same<A0, signed long> >::type>
      : mpl::true_ {};

    template <typename A0> // enables long_(n)
    struct use_terminal<qi::domain
        , terminal_ex<tag::long_, fusion::vector1<A0> > >
      : is_arithmetic<A0> {};

    template <> // enables *lazy* long_(n)
    struct use_lazy_terminal<qi::domain, tag::long_, 1> : mpl::true_ {};

    ///////////////////////////////////////////////////////////////////////////
#ifdef BOOST_HAS_LONG_LONG
    //[primitive_parsers_enable_long_long
    template <> // enables long_long
    struct use_terminal<qi::domain, tag::long_long> : mpl::true_ {};
    //]

    template <typename A0> // enables lit(n)
    struct use_terminal<qi::domain
        , terminal_ex<tag::lit, fusion::vector1<A0> >
        , typename enable_if<is_same<A0, boost::long_long_type> >::type>
      : mpl::true_ {};

    template <typename A0> // enables long_long(n)
    struct use_terminal<qi::domain
        , terminal_ex<tag::long_long, fusion::vector1<A0> > >
      : is_arithmetic<A0> {};

    template <> // enables *lazy* long_long(n)
    struct use_lazy_terminal<qi::domain, tag::long_long, 1> : mpl::true_ {};
#endif

    ///////////////////////////////////////////////////////////////////////////
    // enables any custom int_parser
    template <typename T, unsigned Radix, unsigned MinDigits
            , int MaxDigits>
    struct use_terminal<qi::domain
        , tag::int_parser<T, Radix, MinDigits, MaxDigits> >
      : mpl::true_ {};

    // enables any custom int_parser(n)
    template <typename T, unsigned Radix, unsigned MinDigits
            , int MaxDigits, typename A0>
    struct use_terminal<qi::domain
        , terminal_ex<tag::int_parser<T, Radix, MinDigits, MaxDigits>
                  , fusion::vector1<A0> >
    > : mpl::true_ {};

    // enables *lazy* custom int_parser(n)
    template <typename T, unsigned Radix, unsigned MinDigits
            , int MaxDigits>
    struct use_lazy_terminal<qi::domain
      , tag::int_parser<T, Radix, MinDigits, MaxDigits>, 1
    > : mpl::true_ {};
}}

namespace boost { namespace spirit { namespace qi
{
#ifndef BOOST_SPIRIT_NO_PREDEFINED_TERMINALS
    using spirit::short_;
    using spirit::int_;
    using spirit::long_;
#ifdef BOOST_HAS_LONG_LONG
    using spirit::long_long;
#endif
    using spirit::lit;    // lit(1) is equivalent to 1
#endif
    using spirit::short_type;
    using spirit::int_type;
    using spirit::long_type;
    using spirit::lit_type;
#ifdef BOOST_HAS_LONG_LONG
    using spirit::long_long_type;
#endif
    using spirit::lit_type;

    ///////////////////////////////////////////////////////////////////////////
    // This is the actual int parser
    ///////////////////////////////////////////////////////////////////////////
    //[primitive_parsers_int_parser
    template <
        typename T
      , unsigned Radix = 10
      , unsigned MinDigits = 1
      , int MaxDigits = -1>
    struct any_int_parser
      : primitive_parser<any_int_parser<T, Radix, MinDigits, MaxDigits> >
    {
        // check template parameter 'Radix' for validity
        BOOST_SPIRIT_ASSERT_MSG(
            Radix == 2 || Radix == 8 || Radix == 10 || Radix == 16,
            not_supported_radix, ());

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
            typedef extract_int<T, Radix, MinDigits, MaxDigits> extract;
            qi::skip_over(first, last, skipper);
            return extract::call(first, last, attr_);
        }

        template <typename Context>
        info what(Context& /*context*/) const
        {
            return info("integer");
        }
    };
    //]

    template <typename T, unsigned Radix = 10, unsigned MinDigits = 1
            , int MaxDigits = -1, bool no_attribute = true>
    struct literal_int_parser
      : primitive_parser<literal_int_parser<T, Radix, MinDigits, MaxDigits
        , no_attribute> >
    {
        // check template parameter 'Radix' for validity
        BOOST_SPIRIT_ASSERT_MSG(
            Radix == 2 || Radix == 8 || Radix == 10 || Radix == 16,
            not_supported_radix, ());

        template <typename Value>
        literal_int_parser(Value const& n) : n_(n) {}

        template <typename Context, typename Iterator>
        struct attribute
          : mpl::if_c<no_attribute, unused_type, T>
        {};

        template <typename Iterator, typename Context
          , typename Skipper, typename Attribute>
        bool parse(Iterator& first, Iterator const& last
          , Context& /*context*/, Skipper const& skipper
          , Attribute& attr_param) const
        {
            typedef extract_int<T, Radix, MinDigits, MaxDigits> extract;
            qi::skip_over(first, last, skipper);

            Iterator save = first;
            T attr_;

            if (extract::call(first, last, attr_) && (attr_ == n_))
            {
                traits::assign_to(attr_, attr_param);
                return true;
            }

            first = save;
            return false;
        }

        template <typename Context>
        info what(Context& /*context*/) const
        {
            return info("integer");
        }

        T n_;
    };

    ///////////////////////////////////////////////////////////////////////////
    // Parser generators: make_xxx function (objects)
    ///////////////////////////////////////////////////////////////////////////
    //[primitive_parsers_make_int
    template <
        typename T
      , unsigned Radix = 10
      , unsigned MinDigits = 1
      , int MaxDigits = -1>
    struct make_int
    {
        typedef any_int_parser<T, Radix, MinDigits, MaxDigits> result_type;
        result_type operator()(unused_type, unused_type) const
        {
            return result_type();
        }
    };
    //]

    template <typename T, unsigned Radix = 10, unsigned MinDigits = 1
            , int MaxDigits = -1>
    struct make_direct_int
    {
        typedef literal_int_parser<T, Radix, MinDigits, MaxDigits, false>
            result_type;
        template <typename Terminal>
        result_type operator()(Terminal const& term, unused_type) const
        {
            return result_type(fusion::at_c<0>(term.args));
        }
    };

    template <typename T, unsigned Radix = 10, unsigned MinDigits = 1
            , int MaxDigits = -1>
    struct make_literal_int
    {
        typedef literal_int_parser<T, Radix, MinDigits, MaxDigits> result_type;
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
        , Modifiers, typename enable_if<is_same<A0, signed short> >::type>
      : make_literal_int<signed short> {};

    template <typename Modifiers, typename A0>
    struct make_primitive<
          terminal_ex<tag::lit, fusion::vector1<A0> >
        , Modifiers, typename enable_if<is_same<A0, signed> >::type>
      : make_literal_int<signed> {};

    template <typename Modifiers, typename A0>
    struct make_primitive<
          terminal_ex<tag::lit, fusion::vector1<A0> >
        , Modifiers, typename enable_if<is_same<A0, signed long> >::type>
      : make_literal_int<signed long> {};

#ifdef BOOST_HAS_LONG_LONG
    template <typename Modifiers, typename A0>
    struct make_primitive<
          terminal_ex<tag::lit, fusion::vector1<A0> >
        , Modifiers, typename enable_if<is_same<A0, boost::long_long_type> >::type>
      : make_literal_int<boost::long_long_type> {};
#endif

    ///////////////////////////////////////////////////////////////////////////
    template <typename T, unsigned Radix, unsigned MinDigits, int MaxDigits
            , typename Modifiers>
    struct make_primitive<
        tag::int_parser<T, Radix, MinDigits, MaxDigits>
      , Modifiers>
      : make_int<T, Radix, MinDigits, MaxDigits> {};

    template <typename T, unsigned Radix, unsigned MinDigits, int MaxDigits
            , typename A0, typename Modifiers>
    struct make_primitive<
        terminal_ex<tag::int_parser<T, Radix, MinDigits, MaxDigits>
      , fusion::vector1<A0> >, Modifiers>
      : make_direct_int<T, Radix, MinDigits, MaxDigits> {};

    ///////////////////////////////////////////////////////////////////////////
    //[primitive_parsers_short_primitive
    template <typename Modifiers>
    struct make_primitive<tag::short_, Modifiers>
      : make_int<short> {};
    //]

    template <typename Modifiers, typename A0>
    struct make_primitive<
        terminal_ex<tag::short_
      , fusion::vector1<A0> > , Modifiers>
      : make_direct_int<short> {};

    ///////////////////////////////////////////////////////////////////////////
    //[primitive_parsers_int_primitive
    template <typename Modifiers>
    struct make_primitive<tag::int_, Modifiers>
      : make_int<int> {};
    //]

    template <typename Modifiers, typename A0>
    struct make_primitive<
        terminal_ex<tag::int_
      , fusion::vector1<A0> > , Modifiers>
      : make_direct_int<int> {};

    ///////////////////////////////////////////////////////////////////////////
    //[primitive_parsers_long_primitive
    template <typename Modifiers>
    struct make_primitive<tag::long_, Modifiers>
      : make_int<long> {};
    //]

    template <typename Modifiers, typename A0>
    struct make_primitive<
        terminal_ex<tag::long_
      , fusion::vector1<A0> > , Modifiers>
      : make_direct_int<long> {};

    ///////////////////////////////////////////////////////////////////////////
#ifdef BOOST_HAS_LONG_LONG
    //[primitive_parsers_long_long_primitive
    template <typename Modifiers>
    struct make_primitive<tag::long_long, Modifiers>
      : make_int<boost::long_long_type> {};
    //]

    template <typename Modifiers, typename A0>
    struct make_primitive<
        terminal_ex<tag::long_long
      , fusion::vector1<A0> > , Modifiers>
      : make_direct_int<boost::long_long_type> {};
#endif
}}}

#endif
