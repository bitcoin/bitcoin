/*=============================================================================
    Copyright (c) 2001-2011 Hartmut Kaiser
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_SPIRIT_MATCH_MANIP_MAY_05_2007_1202PM)
#define BOOST_SPIRIT_MATCH_MANIP_MAY_05_2007_1202PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/qi/parse.hpp>
#include <boost/spirit/home/qi/parser.hpp>
#include <boost/spirit/home/support/unused.hpp>
#include <boost/spirit/home/qi/stream/detail/match_manip.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit { namespace qi
{
    ///////////////////////////////////////////////////////////////////////////
    template <typename Expr>
    inline typename detail::match<Expr>::type
    match(
        Expr const& expr)
    {
        return detail::match<Expr>::call(expr);
    }

    template <typename Expr, typename Attribute>
    inline detail::match_manip<
        Expr, mpl::false_, mpl::false_, unused_type, Attribute
    >
    match(
        Expr const& xpr
      , Attribute& p)
    {
        using qi::detail::match_manip;

        // Report invalid expression error as early as possible.
        // If you got an error_invalid_expression error message here,
        // then the expression (expr) is not a valid spirit qi expression.
        BOOST_SPIRIT_ASSERT_MATCH(qi::domain, Expr);
        return match_manip<Expr, mpl::false_, mpl::false_, unused_type, Attribute>(
            xpr, unused, p);
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename Expr, typename Skipper>
    inline typename detail::phrase_match<Expr, Skipper>::type 
    phrase_match(
        Expr const& expr
      , Skipper const& s
      , BOOST_SCOPED_ENUM(skip_flag) post_skip = skip_flag::postskip)
    {
        return detail::phrase_match<Expr, Skipper>::call(expr, s, post_skip);
    }

    template <typename Expr, typename Skipper, typename Attribute>
    inline detail::match_manip<
        Expr, mpl::false_, mpl::false_, Skipper, Attribute
    >
    phrase_match(
        Expr const& xpr
      , Skipper const& s
      , BOOST_SCOPED_ENUM(skip_flag) post_skip
      , Attribute& p)
    {
        using qi::detail::match_manip;

        // Report invalid expression error as early as possible.
        // If you got an error_invalid_expression error message here,
        // then either the expression (expr) or skipper is not a valid
        // spirit qi expression.
        BOOST_SPIRIT_ASSERT_MATCH(qi::domain, Expr);
        BOOST_SPIRIT_ASSERT_MATCH(qi::domain, Skipper);
        return match_manip<Expr, mpl::false_, mpl::false_, Skipper, Attribute>(
            xpr, s, post_skip, p);
    }

    template <typename Expr, typename Skipper, typename Attribute>
    inline detail::match_manip<
        Expr, mpl::false_, mpl::false_, Skipper, Attribute
    >
    phrase_match(
        Expr const& xpr
      , Skipper const& s
      , Attribute& p)
    {
        using qi::detail::match_manip;

        // Report invalid expression error as early as possible.
        // If you got an error_invalid_expression error message here,
        // then either the expression (expr) or skipper is not a valid
        // spirit qi expression.
        BOOST_SPIRIT_ASSERT_MATCH(qi::domain, Expr);
        BOOST_SPIRIT_ASSERT_MATCH(qi::domain, Skipper);
        return match_manip<Expr, mpl::false_, mpl::false_, Skipper, Attribute>(
            xpr, s, p);
    }

    ///////////////////////////////////////////////////////////////////////////
    template<typename Char, typename Traits, typename Derived>
    inline std::basic_istream<Char, Traits>&
    operator>>(std::basic_istream<Char, Traits>& is, parser<Derived> const& p)
    {
        typedef spirit::basic_istream_iterator<Char, Traits> input_iterator;

        input_iterator f(is);
        input_iterator l;
        if (!p.derived().parse(f, l, unused, unused, unused))
        {
            is.setstate(std::ios_base::failbit);
        }
        return is;
    }

}}}

#endif

