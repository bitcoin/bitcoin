/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2001-2011 Hartmut Kaiser

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_PARSE_APRIL_16_2006_0442PM)
#define BOOST_SPIRIT_PARSE_APRIL_16_2006_0442PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/support/context.hpp>
#include <boost/spirit/home/support/nonterminal/locals.hpp>
#include <boost/spirit/home/qi/detail/parse.hpp>
#include <boost/iterator/iterator_concepts.hpp>

namespace boost { namespace spirit { namespace qi
{
    ///////////////////////////////////////////////////////////////////////////
    template <typename Iterator, typename Expr>
    inline bool
    parse(
        Iterator& first
      , Iterator last
      , Expr const& expr)
    {
        // Make sure the iterator is at least a readable forward traversal iterator.
        // If you got a compilation error here, then you are using a weaker iterator
        // while calling this function, you need to supply a readable forward traversal
        // iterator instead.
        BOOST_CONCEPT_ASSERT((boost_concepts::ReadableIteratorConcept<Iterator>));
        BOOST_CONCEPT_ASSERT((boost_concepts::ForwardTraversalConcept<Iterator>));

        return detail::parse_impl<Expr>::call(first, last, expr);
    }

    template <typename Iterator, typename Expr>
    inline bool
    parse(
        Iterator const& first_
      , Iterator last
      , Expr const& expr)
    {
        Iterator first = first_;
        return qi::parse(first, last, expr);
    }

    ///////////////////////////////////////////////////////////////////////////
    namespace detail
    {
        template <typename T>
        struct make_context
        {
            typedef context<fusion::cons<T&>, locals<> > type;
        };

        template <>
        struct make_context<unused_type>
        {
            typedef unused_type type;
        };
    }

    template <typename Iterator, typename Expr, typename Attr>
    inline bool
    parse(
        Iterator& first
      , Iterator last
      , Expr const& expr
      , Attr& attr)
    {
        // Make sure the iterator is at least a readable forward traversal iterator.
        // If you got a compilation error here, then you are using a weaker iterator
        // while calling this function, you need to supply a readable forward traversal
        // iterator instead.
        BOOST_CONCEPT_ASSERT((boost_concepts::ReadableIteratorConcept<Iterator>));
        BOOST_CONCEPT_ASSERT((boost_concepts::ForwardTraversalConcept<Iterator>));

        // Report invalid expression error as early as possible.
        // If you got an error_invalid_expression error message here,
        // then the expression (expr) is not a valid spirit qi expression.
        BOOST_SPIRIT_ASSERT_MATCH(qi::domain, Expr);

        typename detail::make_context<Attr>::type context(attr);
        return compile<qi::domain>(expr).parse(first, last, context, unused, attr);
    }

    template <typename Iterator, typename Expr, typename Attr>
    inline bool
    parse(
        Iterator const& first_
      , Iterator last
      , Expr const& expr
      , Attr& attr)
    {
        Iterator first = first_;
        return qi::parse(first, last, expr, attr);
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename Iterator, typename Expr, typename Skipper>
    inline bool
    phrase_parse(
        Iterator& first
      , Iterator last
      , Expr const& expr
      , Skipper const& skipper
      , BOOST_SCOPED_ENUM(skip_flag) post_skip = skip_flag::postskip)
    {
        // Make sure the iterator is at least a readable forward traversal iterator.
        // If you got a compilation error here, then you are using a weaker iterator
        // while calling this function, you need to supply a readable forward traversal
        // iterator instead.
        BOOST_CONCEPT_ASSERT((boost_concepts::ReadableIteratorConcept<Iterator>));
        BOOST_CONCEPT_ASSERT((boost_concepts::ForwardTraversalConcept<Iterator>));

        return detail::phrase_parse_impl<Expr>::call(
            first, last, expr, skipper, post_skip);
    }

    template <typename Iterator, typename Expr, typename Skipper>
    inline bool
    phrase_parse(
        Iterator const& first_
      , Iterator last
      , Expr const& expr
      , Skipper const& skipper
      , BOOST_SCOPED_ENUM(skip_flag) post_skip = skip_flag::postskip)
    {
        Iterator first = first_;
        return qi::phrase_parse(first, last, expr, skipper, post_skip);
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename Iterator, typename Expr, typename Skipper, typename Attr>
    inline bool
    phrase_parse(
        Iterator& first
      , Iterator last
      , Expr const& expr
      , Skipper const& skipper
      , BOOST_SCOPED_ENUM(skip_flag) post_skip
      , Attr& attr)
    {
        // Make sure the iterator is at least a readable forward traversal iterator.
        // If you got a compilation error here, then you are using a weaker iterator
        // while calling this function, you need to supply a readable forward traversal
        // iterator instead.
        BOOST_CONCEPT_ASSERT((boost_concepts::ReadableIteratorConcept<Iterator>));
        BOOST_CONCEPT_ASSERT((boost_concepts::ForwardTraversalConcept<Iterator>));

        // Report invalid expression error as early as possible.
        // If you got an error_invalid_expression error message here,
        // then either the expression (expr) or skipper is not a valid
        // spirit qi expression.
        BOOST_SPIRIT_ASSERT_MATCH(qi::domain, Expr);
        BOOST_SPIRIT_ASSERT_MATCH(qi::domain, Skipper);

        typedef
            typename result_of::compile<qi::domain, Skipper>::type
        skipper_type;
        skipper_type const skipper_ = compile<qi::domain>(skipper);

        typename detail::make_context<Attr>::type context(attr);
        if (!compile<qi::domain>(expr).parse(
                first, last, context, skipper_, attr))
            return false;

        if (post_skip == skip_flag::postskip)
            qi::skip_over(first, last, skipper_);
        return true;
    }

    template <typename Iterator, typename Expr, typename Skipper, typename Attr>
    inline bool
    phrase_parse(
        Iterator const& first_
      , Iterator last
      , Expr const& expr
      , Skipper const& skipper
      , BOOST_SCOPED_ENUM(skip_flag) post_skip
      , Attr& attr)
    {
        Iterator first = first_;
        return qi::phrase_parse(first, last, expr, skipper, post_skip, attr);
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename Iterator, typename Expr, typename Skipper, typename Attr>
    inline bool
    phrase_parse(
        Iterator& first
      , Iterator last
      , Expr const& expr
      , Skipper const& skipper
      , Attr& attr)
    {
        return qi::phrase_parse(first, last, expr, skipper, skip_flag::postskip, attr);
    }

    template <typename Iterator, typename Expr, typename Skipper, typename Attr>
    inline bool
    phrase_parse(
        Iterator const& first_
      , Iterator last
      , Expr const& expr
      , Skipper const& skipper
      , Attr& attr)
    {
        Iterator first = first_;
        return qi::phrase_parse(first, last, expr, skipper, skip_flag::postskip, attr);
    }
}}}

#endif

