//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_KARMA_DETAIL_GENERATE_AUTO_DEC_01_2009_0743PM)
#define BOOST_SPIRIT_KARMA_DETAIL_GENERATE_AUTO_DEC_01_2009_0743PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/karma/generate.hpp>
#include <boost/spirit/home/karma/auto/create_generator.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/mpl/not.hpp>
#include <boost/mpl/and.hpp>

namespace boost { namespace spirit { namespace karma { namespace detail
{
    ///////////////////////////////////////////////////////////////////////////
    template <typename Expr>
    struct generate_impl<Expr
      , typename enable_if<
            mpl::and_<
                traits::meta_create_exists<karma::domain, Expr>
              , mpl::not_<traits::matches<karma::domain, Expr> > >
        >::type>
    {
        template <typename OutputIterator>
        static bool call(
            OutputIterator& sink
          , Expr const& expr)
        {
            return karma::generate(sink, create_generator<Expr>(), expr);
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename Expr>
    struct generate_delimited_impl<Expr
      , typename enable_if<
            mpl::and_<
                traits::meta_create_exists<karma::domain, Expr>
              , mpl::not_<traits::matches<karma::domain, Expr> > >
        >::type>
    {
        template <typename OutputIterator, typename Delimiter>
        static bool call(
            OutputIterator& sink
          , Expr const& expr
          , Delimiter const& delimiter
          , BOOST_SCOPED_ENUM(delimit_flag) pre_delimit)
        {
            return karma::generate_delimited(
                sink, create_generator<Expr>(), delimiter, pre_delimit, expr);
        }
    };

}}}}

#endif

