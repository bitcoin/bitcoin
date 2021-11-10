//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_KARMA_FORMAT_MANIP_AUTO_DEC_02_2009_1246PM)
#define BOOST_SPIRIT_KARMA_FORMAT_MANIP_AUTO_DEC_02_2009_1246PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/karma/stream/detail/format_manip.hpp>
#include <boost/spirit/home/karma/auto/create_generator.hpp>
#include <boost/utility/enable_if.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit { namespace karma { namespace detail
{
    ///////////////////////////////////////////////////////////////////////////
    template <typename Expr>
    struct format<Expr
      , typename enable_if<traits::meta_create_exists<karma::domain, Expr> >::type>
    {
        typedef typename result_of::create_generator<Expr>::type expr_type;
        typedef format_manip<
            expr_type, mpl::true_, mpl::false_, unused_type, Expr
        > type;

        static type call(Expr const& expr)
        {
            return type(create_generator<Expr>(), unused, expr);
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename Expr, typename Delimiter>
    struct format_delimited<Expr, Delimiter
      , typename enable_if<traits::meta_create_exists<karma::domain, Expr> >::type>
    {
        typedef typename result_of::create_generator<Expr>::type expr_type;
        typedef format_manip<
            expr_type, mpl::true_, mpl::false_, Delimiter, Expr
        > type;

        static type call(Expr const& expr
          , Delimiter const& delimiter
          , BOOST_SCOPED_ENUM(delimit_flag) pre_delimit)
        {
            return type(create_generator<Expr>(), delimiter, pre_delimit, expr);
        }
    };

}}}}

#endif
