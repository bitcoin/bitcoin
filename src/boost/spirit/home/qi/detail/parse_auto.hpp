/*=============================================================================
    Copyright (c) 2001-2011 Hartmut Kaiser

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_DETAIL_PARSE_AUTO_DEC_02_2009_0426PM)
#define BOOST_SPIRIT_DETAIL_PARSE_AUTO_DEC_02_2009_0426PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/qi/parse.hpp>
#include <boost/spirit/home/qi/auto/create_parser.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/mpl/not.hpp>
#include <boost/mpl/and.hpp>

namespace boost { namespace spirit { namespace qi { namespace detail
{
    ///////////////////////////////////////////////////////////////////////////
    template <typename Expr>
    struct parse_impl<Expr
      , typename enable_if<
            mpl::and_<
                traits::meta_create_exists<qi::domain, Expr>
              , mpl::not_<traits::matches<qi::domain, Expr> > >
        >::type>
    {
        template <typename Iterator>
        static bool call(Iterator& first, Iterator last, Expr& expr)
        {
            return qi::parse(first, last, create_parser<Expr>(), expr);
        }

        template <typename Iterator>
        static bool call(Iterator& first, Iterator last, Expr const& expr)
        {
            return qi::parse(first, last, create_parser<Expr>()
              , const_cast<Expr&>(expr));
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename Expr>
    struct phrase_parse_impl<Expr
      , typename enable_if<
            mpl::and_<
                traits::meta_create_exists<qi::domain, Expr>
              , mpl::not_<traits::matches<qi::domain, Expr> > >
        >::type>
    {
        template <typename Iterator, typename Skipper>
        static bool call(Iterator& first, Iterator last, Expr& expr
          , Skipper const& skipper, BOOST_SCOPED_ENUM(skip_flag) post_skip)
        {
            return qi::phrase_parse(first, last, create_parser<Expr>()
              , skipper, post_skip, expr);
        }

        template <typename Iterator, typename Skipper>
        static bool call(Iterator& first, Iterator last, Expr const& expr
          , Skipper const& skipper, BOOST_SCOPED_ENUM(skip_flag) post_skip)
        {
            return qi::phrase_parse(first, last, create_parser<Expr>()
              , skipper, post_skip, const_cast<Expr&>(expr));
        }
    };
}}}}

namespace boost { namespace spirit { namespace qi
{
    ///////////////////////////////////////////////////////////////////////////
    template <typename Iterator, typename Expr>
    inline bool
    parse(
        Iterator& first
      , Iterator last
      , Expr& expr)
    {
        // Make sure the iterator is at least a forward_iterator. If you got a 
        // compilation error here, then you are using an input_iterator while
        // calling this function, you need to supply at least a 
        // forward_iterator instead.
        BOOST_CONCEPT_ASSERT((ForwardIterator<Iterator>));

        return detail::parse_impl<Expr>::call(first, last, expr);
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename Iterator, typename Expr, typename Skipper>
    inline bool
    phrase_parse(
        Iterator& first
      , Iterator last
      , Expr& expr
      , Skipper const& skipper
      , BOOST_SCOPED_ENUM(skip_flag) post_skip = skip_flag::postskip)
    {
        // Make sure the iterator is at least a forward_iterator. If you got a 
        // compilation error here, then you are using an input_iterator while
        // calling this function, you need to supply at least a 
        // forward_iterator instead.
        BOOST_CONCEPT_ASSERT((ForwardIterator<Iterator>));

        return detail::phrase_parse_impl<Expr>::call(
            first, last, expr, skipper, post_skip);
    }
}}}

#endif

