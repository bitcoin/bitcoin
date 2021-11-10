/*=============================================================================
    Copyright (c) 1998-2003 Joel de Guzman
    Copyright (c) 2001 Daniel Nuffer
    Copyright (c) 2002 Hartmut Kaiser
    http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_EXCLUSIVE_OR_HPP)
#define BOOST_SPIRIT_EXCLUSIVE_OR_HPP

#include <boost/spirit/home/classic/namespace.hpp>
#include <boost/spirit/home/classic/core/parser.hpp>
#include <boost/spirit/home/classic/core/primitives/primitives.hpp>
#include <boost/spirit/home/classic/core/composite/composite.hpp>
#include <boost/spirit/home/classic/meta/as_parser.hpp>

namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

    ///////////////////////////////////////////////////////////////////////////
    //
    //  exclusive_or class
    //
    //      Handles expressions of the form:
    //
    //          a ^ b
    //
    //      where a and b are parsers. The expression returns a composite
    //      parser that matches a or b but not both. One (not both) of the
    //      operands may be a literal char, wchar_t or a primitive string
    //      char const*, wchar_t const*.
    //
    ///////////////////////////////////////////////////////////////////////////
    struct exclusive_or_parser_gen;

    template <typename A, typename B>
    struct exclusive_or
    :   public binary<A, B, parser<exclusive_or<A, B> > >
    {
        typedef exclusive_or<A, B>              self_t;
        typedef binary_parser_category          parser_category_t;
        typedef exclusive_or_parser_gen         parser_generator_t;
        typedef binary<A, B, parser<self_t> >   base_t;

        exclusive_or(A const& a, B const& b)
        : base_t(a, b) {}

        template <typename ScannerT>
        typename parser_result<self_t, ScannerT>::type
        parse(ScannerT const& scan) const
        {
            typedef typename parser_result<self_t, ScannerT>::type result_t;
            typedef typename ScannerT::iterator_t iterator_t;

            iterator_t save = scan.first;
            result_t l = this->left().parse(scan);
            std::swap(save, scan.first);
            result_t r = this->right().parse(scan);

            if (l ? !bool(r) : bool(r))
            {
                if (l)
                    scan.first = save;
                return l ? l : r;
            }

            return scan.no_match();
        }
    };

    struct exclusive_or_parser_gen
    {
        template <typename A, typename B>
        struct result
        {
            typedef
                exclusive_or<
                    typename as_parser<A>::type
                  , typename as_parser<B>::type
                >
            type;
        };

        template <typename A, typename B>
        static exclusive_or<
            typename as_parser<A>::type
          , typename as_parser<B>::type
        >
        generate(A const& a, B const& b)
        {
            return exclusive_or<BOOST_DEDUCED_TYPENAME as_parser<A>::type,
                BOOST_DEDUCED_TYPENAME as_parser<B>::type>
                    (as_parser<A>::convert(a), as_parser<B>::convert(b));
        }
    };

    template <typename A, typename B>
    exclusive_or<A, B>
    operator^(parser<A> const& a, parser<B> const& b);

    template <typename A>
    exclusive_or<A, chlit<char> >
    operator^(parser<A> const& a, char b);

    template <typename B>
    exclusive_or<chlit<char>, B>
    operator^(char a, parser<B> const& b);

    template <typename A>
    exclusive_or<A, strlit<char const*> >
    operator^(parser<A> const& a, char const* b);

    template <typename B>
    exclusive_or<strlit<char const*>, B>
    operator^(char const* a, parser<B> const& b);

    template <typename A>
    exclusive_or<A, chlit<wchar_t> >
    operator^(parser<A> const& a, wchar_t b);

    template <typename B>
    exclusive_or<chlit<wchar_t>, B>
    operator^(wchar_t a, parser<B> const& b);

    template <typename A>
    exclusive_or<A, strlit<wchar_t const*> >
    operator^(parser<A> const& a, wchar_t const* b);

    template <typename B>
    exclusive_or<strlit<wchar_t const*>, B>
    operator^(wchar_t const* a, parser<B> const& b);

BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}} // namespace BOOST_SPIRIT_CLASSIC_NS

#endif

#include <boost/spirit/home/classic/core/composite/impl/exclusive_or.ipp>
