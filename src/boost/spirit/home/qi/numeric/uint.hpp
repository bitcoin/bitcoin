/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2011 Bryce Lelbach
    Copyright (c) 2011 Jan Frederick Eick

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef BOOST_SPIRIT_QI_NUMERIC_UINT_HPP
#define BOOST_SPIRIT_QI_NUMERIC_UINT_HPP

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
        struct uint_parser
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
        struct uint_parser
          : spirit::terminal<tag::uint_parser<T, Radix, MinDigits, MaxDigits> >
        {};
    }

    ///////////////////////////////////////////////////////////////////////////
    // Enablers
    ///////////////////////////////////////////////////////////////////////////
    template <> // enables ushort_
    struct use_terminal<qi::domain, tag::ushort_> : mpl::true_ {};

    template <typename A0> // enables lit(n)
    struct use_terminal<qi::domain
        , terminal_ex<tag::lit, fusion::vector1<A0> >
        , typename enable_if<is_same<A0, unsigned short> >::type>
      : mpl::true_ {};

    template <typename A0> // enables ushort_(n)
    struct use_terminal<qi::domain
        , terminal_ex<tag::ushort_, fusion::vector1<A0> > >
      : is_arithmetic<A0> {};

    template <> // enables *lazy* ushort_(n)
    struct use_lazy_terminal<qi::domain, tag::ushort_, 1> : mpl::true_ {};

    ///////////////////////////////////////////////////////////////////////////
    template <> // enables uint_
    struct use_terminal<qi::domain, tag::uint_> : mpl::true_ {};

    template <typename A0> // enables lit(n)
    struct use_terminal<qi::domain
        , terminal_ex<tag::lit, fusion::vector1<A0> >
        , typename enable_if<is_same<A0, unsigned> >::type>
      : mpl::true_ {};

    template <typename A0> // enables uint_(n)
    struct use_terminal<qi::domain
        , terminal_ex<tag::uint_, fusion::vector1<A0> > >
      : is_arithmetic<A0> {};

    template <> // enables *lazy* uint_(n)
    struct use_lazy_terminal<qi::domain, tag::uint_, 1> : mpl::true_ {};

    ///////////////////////////////////////////////////////////////////////////
    template <> // enables ulong_
    struct use_terminal<qi::domain, tag::ulong_> : mpl::true_ {};

    template <typename A0> // enables lit(n)
    struct use_terminal<qi::domain
        , terminal_ex<tag::lit, fusion::vector1<A0> >
        , typename enable_if<is_same<A0, unsigned long> >::type>
      : mpl::true_ {};

    template <typename A0> // enables ulong_(n)
    struct use_terminal<qi::domain
        , terminal_ex<tag::ulong_, fusion::vector1<A0> > >
      : is_arithmetic<A0> {};

    template <> // enables *lazy* ulong_(n)
    struct use_lazy_terminal<qi::domain, tag::ulong_, 1> : mpl::true_ {};

    ///////////////////////////////////////////////////////////////////////////
#ifdef BOOST_HAS_LONG_LONG
    template <> // enables ulong_long
    struct use_terminal<qi::domain, tag::ulong_long> : mpl::true_ {};

    template <typename A0> // enables lit(n)
    struct use_terminal<qi::domain
        , terminal_ex<tag::lit, fusion::vector1<A0> >
        , typename enable_if<is_same<A0, boost::ulong_long_type> >::type>
      : mpl::true_ {};

    template <typename A0> // enables ulong_long(n)
    struct use_terminal<qi::domain
        , terminal_ex<tag::ulong_long, fusion::vector1<A0> > >
      : is_arithmetic<A0> {};

    template <> // enables *lazy* ulong_long(n)
    struct use_lazy_terminal<qi::domain, tag::ulong_long, 1> : mpl::true_ {};
#endif

    ///////////////////////////////////////////////////////////////////////////
    template <> // enables bin
    struct use_terminal<qi::domain, tag::bin> : mpl::true_ {};

    template <typename A0> // enables bin(n)
    struct use_terminal<qi::domain
        , terminal_ex<tag::bin, fusion::vector1<A0> > >
      : is_arithmetic<A0> {};

    template <> // enables *lazy* bin(n)
    struct use_lazy_terminal<qi::domain, tag::bin, 1> : mpl::true_ {};

    ///////////////////////////////////////////////////////////////////////////
    template <> // enables oct
    struct use_terminal<qi::domain, tag::oct> : mpl::true_ {};

    template <typename A0> // enables oct(n)
    struct use_terminal<qi::domain
        , terminal_ex<tag::oct, fusion::vector1<A0> > >
      : is_arithmetic<A0> {};

    template <> // enables *lazy* oct(n)
    struct use_lazy_terminal<qi::domain, tag::oct, 1> : mpl::true_ {};

    ///////////////////////////////////////////////////////////////////////////
    template <> // enables hex
    struct use_terminal<qi::domain, tag::hex> : mpl::true_ {};

    template <typename A0> // enables hex(n)
    struct use_terminal<qi::domain
        , terminal_ex<tag::hex, fusion::vector1<A0> > >
      : is_arithmetic<A0> {};

    template <> // enables *lazy* hex(n)
    struct use_lazy_terminal<qi::domain, tag::hex, 1> : mpl::true_ {};

    ///////////////////////////////////////////////////////////////////////////
    // enables any custom uint_parser
    template <typename T, unsigned Radix, unsigned MinDigits
            , int MaxDigits>
    struct use_terminal<qi::domain
        , tag::uint_parser<T, Radix, MinDigits, MaxDigits> >
      : mpl::true_ {};

    // enables any custom uint_parser(n)
    template <typename T, unsigned Radix, unsigned MinDigits
            , int MaxDigits, typename A0>
    struct use_terminal<qi::domain
        , terminal_ex<tag::uint_parser<T, Radix, MinDigits, MaxDigits>
                  , fusion::vector1<A0> >
    > : mpl::true_ {};

    // enables *lazy* custom uint_parser(n)
    template <typename T, unsigned Radix, unsigned MinDigits
            , int MaxDigits>
    struct use_lazy_terminal<qi::domain
      , tag::uint_parser<T, Radix, MinDigits, MaxDigits>, 1
    > : mpl::true_ {};
}}

namespace boost { namespace spirit { namespace qi
{
#ifndef BOOST_SPIRIT_NO_PREDEFINED_TERMINALS
    using spirit::bin;
    using spirit::oct;
    using spirit::hex;

    using spirit::ushort_;
    using spirit::uint_;
    using spirit::ulong_;
#ifdef BOOST_HAS_LONG_LONG
    using spirit::ulong_long;
#endif
    using spirit::lit; // lit(1) is equivalent to 1
#endif

    using spirit::bin_type;
    using spirit::oct_type;
    using spirit::hex_type;

    using spirit::ushort_type;
    using spirit::uint_type;
    using spirit::ulong_type;
#ifdef BOOST_HAS_LONG_LONG
    using spirit::ulong_long_type;
#endif
    using spirit::lit_type;

    ///////////////////////////////////////////////////////////////////////////
    // This is the actual uint parser
    ///////////////////////////////////////////////////////////////////////////
    template <typename T, unsigned Radix = 10, unsigned MinDigits = 1
            , int MaxDigits = -1>
    struct any_uint_parser
      : primitive_parser<any_uint_parser<T, Radix, MinDigits, MaxDigits> >
    {
        // check template parameter 'Radix' for validity
        BOOST_SPIRIT_ASSERT_MSG(
            Radix >= 2 && Radix <= 36,
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
            typedef extract_uint<T, Radix, MinDigits, MaxDigits> extract;
            qi::skip_over(first, last, skipper);
            return extract::call(first, last, attr_);
        }

        template <typename Context>
        info what(Context& /*context*/) const
        {
            return info("unsigned-integer");
        }
    };
    //]

    template <typename T, unsigned Radix = 10, unsigned MinDigits = 1
            , int MaxDigits = -1, bool no_attribute = true>
    struct literal_uint_parser
      : primitive_parser<literal_uint_parser<T, Radix, MinDigits, MaxDigits
        , no_attribute> >
    {
        // check template parameter 'Radix' for validity
        BOOST_SPIRIT_ASSERT_MSG(
            Radix == 2 || Radix == 8 || Radix == 10 || Radix == 16,
            not_supported_radix, ());

        template <typename Value>
        literal_uint_parser(Value const& n) : n_(n) {}

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
            typedef extract_uint<T, Radix, MinDigits, MaxDigits> extract;
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
            return info("unsigned-integer");
        }

        T n_;
    };

    ///////////////////////////////////////////////////////////////////////////
    // Parser generators: make_xxx function (objects)
    ///////////////////////////////////////////////////////////////////////////
    template <typename T, unsigned Radix = 10, unsigned MinDigits = 1
            , int MaxDigits = -1>
    struct make_uint
    {
        typedef any_uint_parser<T, Radix, MinDigits, MaxDigits> result_type;
        result_type operator()(unused_type, unused_type) const
        {
            return result_type();
        }
    };

    template <typename T, unsigned Radix = 10, unsigned MinDigits = 1
            , int MaxDigits = -1>
    struct make_direct_uint
    {
        typedef literal_uint_parser<T, Radix, MinDigits, MaxDigits, false>
            result_type;
        template <typename Terminal>
        result_type operator()(Terminal const& term, unused_type) const
        {
            return result_type(fusion::at_c<0>(term.args));
        }
    };

    template <typename T, unsigned Radix = 10, unsigned MinDigits = 1
            , int MaxDigits = -1>
    struct make_literal_uint
    {
        typedef literal_uint_parser<T, Radix, MinDigits, MaxDigits> result_type;
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
        , Modifiers, typename enable_if<is_same<A0, unsigned short> >::type>
      : make_literal_uint<unsigned short> {};

    template <typename Modifiers, typename A0>
    struct make_primitive<
          terminal_ex<tag::lit, fusion::vector1<A0> >
        , Modifiers, typename enable_if<is_same<A0, unsigned> >::type>
      : make_literal_uint<unsigned> {};

    template <typename Modifiers, typename A0>
    struct make_primitive<
          terminal_ex<tag::lit, fusion::vector1<A0> >
        , Modifiers, typename enable_if<is_same<A0, unsigned long> >::type>
      : make_literal_uint<unsigned long> {};

#ifdef BOOST_HAS_LONG_LONG
    template <typename Modifiers, typename A0>
    struct make_primitive<
          terminal_ex<tag::lit, fusion::vector1<A0> >
        , Modifiers, typename enable_if<is_same<A0, boost::ulong_long_type> >::type>
      : make_literal_uint<boost::ulong_long_type> {};
#endif

    ///////////////////////////////////////////////////////////////////////////
    template <typename T, unsigned Radix, unsigned MinDigits, int MaxDigits
            , typename Modifiers>
    struct make_primitive<
        tag::uint_parser<T, Radix, MinDigits, MaxDigits>
      , Modifiers>
      : make_uint<T, Radix, MinDigits, MaxDigits> {};

    template <typename T, unsigned Radix, unsigned MinDigits, int MaxDigits
            , typename A0, typename Modifiers>
    struct make_primitive<
        terminal_ex<tag::uint_parser<T, Radix, MinDigits, MaxDigits>
      , fusion::vector1<A0> >, Modifiers>
      : make_direct_uint<T, Radix, MinDigits, MaxDigits> {};

    ///////////////////////////////////////////////////////////////////////////
    template <typename Modifiers>
    struct make_primitive<tag::bin, Modifiers>
      : make_uint<unsigned, 2> {};

    template <typename Modifiers, typename A0>
    struct make_primitive<
        terminal_ex<tag::bin
      , fusion::vector1<A0> > , Modifiers>
      : make_direct_uint<unsigned, 2> {};

    ///////////////////////////////////////////////////////////////////////////
    template <typename Modifiers>
    struct make_primitive<tag::oct, Modifiers>
      : make_uint<unsigned, 8> {};

    template <typename Modifiers, typename A0>
    struct make_primitive<
        terminal_ex<tag::oct
      , fusion::vector1<A0> > , Modifiers>
      : make_direct_uint<unsigned, 8> {};

    ///////////////////////////////////////////////////////////////////////////
    template <typename Modifiers>
    struct make_primitive<tag::hex, Modifiers>
      : make_uint<unsigned, 16> {};

    template <typename Modifiers, typename A0>
    struct make_primitive<
        terminal_ex<tag::hex
      , fusion::vector1<A0> > , Modifiers>
      : make_direct_uint<unsigned, 16> {};

    ///////////////////////////////////////////////////////////////////////////
    template <typename Modifiers>
    struct make_primitive<tag::ushort_, Modifiers>
      : make_uint<unsigned short> {};

    template <typename Modifiers, typename A0>
    struct make_primitive<
        terminal_ex<tag::ushort_
      , fusion::vector1<A0> > , Modifiers>
      : make_direct_uint<unsigned short> {};

    ///////////////////////////////////////////////////////////////////////////
    template <typename Modifiers>
    struct make_primitive<tag::uint_, Modifiers>
      : make_uint<unsigned> {};

    template <typename Modifiers, typename A0>
    struct make_primitive<
        terminal_ex<tag::uint_
      , fusion::vector1<A0> > , Modifiers>
      : make_direct_uint<unsigned> {};

    ///////////////////////////////////////////////////////////////////////////
    template <typename Modifiers>
    struct make_primitive<tag::ulong_, Modifiers>
      : make_uint<unsigned long> {};

    template <typename Modifiers, typename A0>
    struct make_primitive<
        terminal_ex<tag::ulong_
      , fusion::vector1<A0> > , Modifiers>
      : make_direct_uint<unsigned long> {};

    ///////////////////////////////////////////////////////////////////////////
#ifdef BOOST_HAS_LONG_LONG
    template <typename Modifiers>
    struct make_primitive<tag::ulong_long, Modifiers>
      : make_uint<boost::ulong_long_type> {};

    template <typename Modifiers, typename A0>
    struct make_primitive<
        terminal_ex<tag::ulong_long
      , fusion::vector1<A0> > , Modifiers>
      : make_direct_uint<boost::ulong_long_type> {};
#endif
}}}

#endif
