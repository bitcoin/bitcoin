//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_KARMA_FORMAT_MANIP_MAY_03_2007_1424PM)
#define BOOST_SPIRIT_KARMA_FORMAT_MANIP_MAY_03_2007_1424PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <iterator>
#include <string>
#include <boost/spirit/home/karma/generate.hpp>
#include <boost/spirit/home/support/iterators/ostream_iterator.hpp>
#include <boost/mpl/bool.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit { namespace karma { namespace detail
{
    ///////////////////////////////////////////////////////////////////////////
    template <typename Expr
      , typename CopyExpr = mpl::false_, typename CopyAttr = mpl::false_
      , typename Delimiter = unused_type, typename Attribute = unused_type>
    struct format_manip 
    {
        // This assertion makes sure we don't hit the only code path which is 
        // not implemented (because it isn't needed), where both, the 
        // expression and the attribute need to be held as a copy.
        BOOST_SPIRIT_ASSERT_MSG(!CopyExpr::value || !CopyAttr::value
            , error_invalid_should_not_happen, ());

        format_manip(Expr const& xpr, Delimiter const& d, Attribute const& a) 
          : expr(xpr), delim(d), pre(delimit_flag::dont_predelimit), attr(a) {}

        format_manip(Expr const& xpr, Delimiter const& d
            , BOOST_SCOPED_ENUM(delimit_flag) pre_delimit, Attribute const& a) 
          : expr(xpr), delim(d), pre(pre_delimit), attr(a) {}

        Expr const& expr;
        Delimiter const& delim;
        BOOST_SCOPED_ENUM(delimit_flag) const pre;
        Attribute const& attr;

        // silence MSVC warning C4512: assignment operator could not be generated
        BOOST_DELETED_FUNCTION(format_manip& operator= (format_manip const&))
    };

    template <typename Expr, typename Delimiter, typename Attribute>
    struct format_manip<Expr, mpl::false_, mpl::true_, Delimiter, Attribute>
    {
        format_manip(Expr const& xpr, Delimiter const& d, Attribute const& a) 
          : expr(xpr), delim(d), pre(delimit_flag::dont_predelimit), attr(a) {}

        format_manip(Expr const& xpr, Delimiter const& d
            , BOOST_SCOPED_ENUM(delimit_flag) pre_delimit, Attribute const& a) 
          : expr(xpr), delim(d), pre(pre_delimit), attr(a) {}

        Expr const& expr;
        Delimiter const& delim;
        BOOST_SCOPED_ENUM(delimit_flag) const pre;
        Attribute attr;

        // silence MSVC warning C4512: assignment operator could not be generated
        BOOST_DELETED_FUNCTION(format_manip& operator= (format_manip const&))
    };

    template <typename Expr, typename Delimiter, typename Attribute>
    struct format_manip<Expr, mpl::true_, mpl::false_, Delimiter, Attribute>
    {
        format_manip(Expr const& xpr, Delimiter const& d, Attribute const& a) 
          : expr(xpr), delim(d), pre(delimit_flag::dont_predelimit), attr(a) {}

        format_manip(Expr const& xpr, Delimiter const& d
            , BOOST_SCOPED_ENUM(delimit_flag) pre_delimit, Attribute const& a) 
          : expr(xpr), delim(d), pre(pre_delimit), attr(a) {}

        Expr expr;
        Delimiter const& delim;
        BOOST_SCOPED_ENUM(delimit_flag) const pre;
        Attribute const& attr;

        // silence MSVC warning C4512: assignment operator could not be generated
        BOOST_DELETED_FUNCTION(format_manip& operator= (format_manip const&))
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename Expr, typename Enable = void>
    struct format
    {
        // Report invalid expression error as early as possible.
        // If you got an error_invalid_expression error message here,
        // then the expression (Expr) is not a valid spirit karma expression.
        // Did you intend to use the auto_ facilities while forgetting to 
        // #include <boost/spirit/include/karma_format_auto.hpp>?
        BOOST_SPIRIT_ASSERT_MATCH(karma::domain, Expr);
    };

    template <typename Expr>
    struct format<Expr
      , typename enable_if<traits::matches<karma::domain, Expr> >::type>
    {
        typedef format_manip<Expr> type;

        static type call(Expr const& expr)
        {
            return type(expr, unused, unused);
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename Expr, typename Delimiter, typename Enable = void>
    struct format_delimited
    {
        // Report invalid expression error as early as possible.
        // If you got an error_invalid_expression error message here,
        // then the expression (Expr) is not a valid spirit karma expression.
        // Did you intend to use the auto_ facilities while forgetting to 
        // #include <boost/spirit/include/karma_format_auto.hpp>?
        BOOST_SPIRIT_ASSERT_MATCH(karma::domain, Expr);
    };

    template <typename Expr, typename Delimiter>
    struct format_delimited<Expr, Delimiter
      , typename enable_if<traits::matches<karma::domain, Expr> >::type>
    {
        typedef format_manip<Expr, mpl::false_, mpl::false_, Delimiter> type;

        static type call(
            Expr const& expr
          , Delimiter const& delimiter
          , BOOST_SCOPED_ENUM(delimit_flag) pre_delimit)
        {
            // Report invalid expression error as early as possible.
            // If you got an error_invalid_expression error message here,
            // then the delimiter is not a valid spirit karma expression.
            BOOST_SPIRIT_ASSERT_MATCH(karma::domain, Delimiter);
            return type(expr, delimiter, pre_delimit, unused);
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    template<typename Char, typename Traits, typename Expr
      , typename CopyExpr, typename CopyAttr> 
    inline std::basic_ostream<Char, Traits> & 
    operator<< (std::basic_ostream<Char, Traits> &os
      , format_manip<Expr, CopyExpr, CopyAttr> const& fm)
    {
        karma::ostream_iterator<Char, Char, Traits> sink(os);
        if (!karma::generate (sink, fm.expr))
        {
            os.setstate(std::ios_base::failbit);
        }
        return os;
    }

    ///////////////////////////////////////////////////////////////////////////
    template<typename Char, typename Traits, typename Expr
      , typename CopyExpr, typename CopyAttr, typename Attribute> 
    inline std::basic_ostream<Char, Traits> & 
    operator<< (std::basic_ostream<Char, Traits> &os
      , format_manip<Expr, CopyExpr, CopyAttr, unused_type, Attribute> const& fm)
    {
        karma::ostream_iterator<Char, Char, Traits> sink(os);
        if (!karma::generate(sink, fm.expr, fm.attr))
        {
            os.setstate(std::ios_base::failbit);
        }
        return os;
    }

    template<typename Char, typename Traits, typename Expr
      , typename CopyExpr, typename CopyAttr, typename Delimiter> 
    inline std::basic_ostream<Char, Traits> & 
    operator<< (std::basic_ostream<Char, Traits> &os
      , format_manip<Expr, CopyExpr, CopyAttr, Delimiter> const& fm)
    {
        karma::ostream_iterator<Char, Char, Traits> sink(os);
        if (!karma::generate_delimited(sink, fm.expr, fm.delim, fm.pre))
        {
            os.setstate(std::ios_base::failbit);
        }
        return os;
    }

    ///////////////////////////////////////////////////////////////////////////
    template<typename Char, typename Traits, typename Expr
      , typename CopyExpr, typename CopyAttr, typename Delimiter
      , typename Attribute> 
    inline std::basic_ostream<Char, Traits> & 
    operator<< (std::basic_ostream<Char, Traits> &os
      , format_manip<Expr, CopyExpr, CopyAttr, Delimiter, Attribute> const& fm)
    {
        karma::ostream_iterator<Char, Char, Traits> sink(os);
        if (!karma::generate_delimited(sink, fm.expr, fm.delim, fm.pre, fm.attr))
        {
            os.setstate(std::ios_base::failbit);
        }
        return os;
    }
}}}}

#endif
