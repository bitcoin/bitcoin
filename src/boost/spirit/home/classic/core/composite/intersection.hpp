/*=============================================================================
    Copyright (c) 1998-2003 Joel de Guzman
    Copyright (c) 2001 Daniel Nuffer
    Copyright (c) 2002 Hartmut Kaiser
    http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_INTERSECTION_HPP)
#define BOOST_SPIRIT_INTERSECTION_HPP

#include <boost/spirit/home/classic/namespace.hpp>
#include <boost/spirit/home/classic/core/parser.hpp>
#include <boost/spirit/home/classic/core/primitives/primitives.hpp>
#include <boost/spirit/home/classic/core/composite/composite.hpp>
#include <boost/spirit/home/classic/meta/as_parser.hpp>

namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

    ///////////////////////////////////////////////////////////////////////////
    //
    //  intersection class
    //
    //      Handles expressions of the form:
    //
    //          a & b
    //
    //      where a and b are parsers. The expression returns a composite
    //      parser that matches a and b. One (not both) of the operands may
    //      be a literal char, wchar_t or a primitive string char const*,
    //      wchar_t const*.
    //
    //      The expression is short circuit evaluated. b is never touched
    //      when a is returns a no-match.
    //
    ///////////////////////////////////////////////////////////////////////////
    struct intersection_parser_gen;
    
    template <typename A, typename B>
    struct intersection
    :   public binary<A, B, parser<intersection<A, B> > >
    {
        typedef intersection<A, B>              self_t;
        typedef binary_parser_category          parser_category_t;
        typedef intersection_parser_gen         parser_generator_t;
        typedef binary<A, B, parser<self_t> >   base_t;
    
        intersection(A const& a, B const& b)
        : base_t(a, b) {}
    
        template <typename ScannerT>
        typename parser_result<self_t, ScannerT>::type
        parse(ScannerT const& scan) const
        {
            typedef typename parser_result<self_t, ScannerT>::type result_t;
            typedef typename ScannerT::iterator_t iterator_t;
            iterator_t save = scan.first;
            if (result_t hl = this->left().parse(scan))
            {
                ScannerT bscan(scan.first, scan.first, scan);
                scan.first = save;
                result_t hr = this->right().parse(bscan);
                if (hl.length() == hr.length())
                    return hl;
            }
    
            return scan.no_match();
        }
    };
    
    struct intersection_parser_gen
    {
        template <typename A, typename B>
        struct result 
        {
            typedef 
                intersection<
                    typename as_parser<A>::type
                  , typename as_parser<B>::type
                >
            type;
        };
    
        template <typename A, typename B>
        static intersection<
            typename as_parser<A>::type
          , typename as_parser<B>::type
        >
        generate(A const& a, B const& b)
        {
            return intersection<BOOST_DEDUCED_TYPENAME as_parser<A>::type,
                BOOST_DEDUCED_TYPENAME as_parser<B>::type>
                    (as_parser<A>::convert(a), as_parser<B>::convert(b));
        }
    };
    
    template <typename A, typename B>
    intersection<A, B>
    operator&(parser<A> const& a, parser<B> const& b);
    
    template <typename A>
    intersection<A, chlit<char> >
    operator&(parser<A> const& a, char b);
    
    template <typename B>
    intersection<chlit<char>, B>
    operator&(char a, parser<B> const& b);
    
    template <typename A>
    intersection<A, strlit<char const*> >
    operator&(parser<A> const& a, char const* b);
    
    template <typename B>
    intersection<strlit<char const*>, B>
    operator&(char const* a, parser<B> const& b);
    
    template <typename A>
    intersection<A, chlit<wchar_t> >
    operator&(parser<A> const& a, wchar_t b);
    
    template <typename B>
    intersection<chlit<wchar_t>, B>
    operator&(wchar_t a, parser<B> const& b);
    
    template <typename A>
    intersection<A, strlit<wchar_t const*> >
    operator&(parser<A> const& a, wchar_t const* b);
    
    template <typename B>
    intersection<strlit<wchar_t const*>, B>
    operator&(wchar_t const* a, parser<B> const& b);
    
BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}} // namespace BOOST_SPIRIT_CLASSIC_NS

#endif

#include <boost/spirit/home/classic/core/composite/impl/intersection.ipp>
