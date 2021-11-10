//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_KARMA_FORMAT_MANIP_MAY_01_2007_1211PM)
#define BOOST_SPIRIT_KARMA_FORMAT_MANIP_MAY_01_2007_1211PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/karma/generate.hpp>
#include <boost/spirit/home/karma/generator.hpp>
#include <boost/spirit/home/karma/detail/output_iterator.hpp>
#include <boost/spirit/home/karma/stream/detail/format_manip.hpp>
#include <boost/spirit/home/support/unused.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit { namespace karma 
{
    ///////////////////////////////////////////////////////////////////////////
    template <typename Expr>
    inline typename detail::format<Expr>::type 
    format(Expr const& expr)
    {
        return detail::format<Expr>::call(expr);
    }

    template <typename Expr, typename Attribute>
    inline detail::format_manip<Expr, mpl::false_, mpl::false_, unused_type, Attribute> 
    format(
        Expr const& expr
      , Attribute const& attr)
    {
        using karma::detail::format_manip;

        // Report invalid expression error as early as possible.
        // If you got an error_invalid_expression error message here,
        // then the expression (expr) is not a valid spirit karma expression.
        BOOST_SPIRIT_ASSERT_MATCH(karma::domain, Expr);
        return format_manip<Expr, mpl::false_, mpl::false_, unused_type, Attribute>(
            expr, unused, attr);
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename Expr, typename Delimiter>
    inline typename detail::format_delimited<Expr, Delimiter>::type 
    format_delimited(
        Expr const& expr
      , Delimiter const& d
      , BOOST_SCOPED_ENUM(delimit_flag) pre_delimit = 
            delimit_flag::dont_predelimit)
    {
        return detail::format_delimited<Expr, Delimiter>::call(expr, d, pre_delimit);
    }

    template <typename Expr, typename Delimiter, typename Attribute>
    inline detail::format_manip<Expr, mpl::false_, mpl::false_, Delimiter, Attribute> 
    format_delimited(
        Expr const& xpr
      , Delimiter const& d
      , BOOST_SCOPED_ENUM(delimit_flag) pre_delimit
      , Attribute const& attr)
    {
        using karma::detail::format_manip;

        // Report invalid expression error as early as possible.
        // If you got an error_invalid_expression error message here,
        // then the expression (expr) is not a valid spirit karma expression.
        BOOST_SPIRIT_ASSERT_MATCH(karma::domain, Expr);
        BOOST_SPIRIT_ASSERT_MATCH(karma::domain, Delimiter);
        return format_manip<Expr, mpl::false_, mpl::false_, Delimiter, Attribute>(
            xpr, d, pre_delimit, attr);
    }

    template <typename Expr, typename Delimiter, typename Attribute>
    inline detail::format_manip<Expr, mpl::false_, mpl::false_, Delimiter, Attribute> 
    format_delimited(
        Expr const& xpr
      , Delimiter const& d
      , Attribute const& attr)
    {
        using karma::detail::format_manip;

        // Report invalid expression error as early as possible.
        // If you got an error_invalid_expression error message here,
        // then the expression (expr) is not a valid spirit karma expression.
        BOOST_SPIRIT_ASSERT_MATCH(karma::domain, Expr);
        BOOST_SPIRIT_ASSERT_MATCH(karma::domain, Delimiter);
        return format_manip<Expr, mpl::false_, mpl::false_, Delimiter, Attribute>(
            xpr, d, delimit_flag::dont_predelimit, attr);
    }

    ///////////////////////////////////////////////////////////////////////////
    template<typename Char, typename Traits, typename Derived> 
    inline std::basic_ostream<Char, Traits> & 
    operator<< (std::basic_ostream<Char, Traits> &os, generator<Derived> const& g)
    {
        typedef traits::properties_of<
            typename result_of::compile<karma::domain, Derived>::type
        > properties;
        typedef karma::ostream_iterator<Char, Char, Traits> outiter_type;

        outiter_type target_sink(os);
        karma::detail::output_iterator<outiter_type, properties> sink(target_sink);

        if (!g.derived().generate(sink, unused, unused, unused))
        {
            os.setstate(std::ios_base::failbit);
        }
        return os;
    }
}}}

#endif 

