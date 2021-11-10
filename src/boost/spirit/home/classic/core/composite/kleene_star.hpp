/*=============================================================================
    Copyright (c) 1998-2003 Joel de Guzman
    Copyright (c) 2001 Daniel Nuffer
    Copyright (c) 2002 Hartmut Kaiser
    http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_KLEENE_STAR_HPP)
#define BOOST_SPIRIT_KLEENE_STAR_HPP

#include <boost/spirit/home/classic/namespace.hpp>
#include <boost/spirit/home/classic/core/parser.hpp>
#include <boost/spirit/home/classic/core/primitives/primitives.hpp>
#include <boost/spirit/home/classic/core/composite/composite.hpp>
#include <boost/spirit/home/classic/meta/as_parser.hpp>

namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

    ///////////////////////////////////////////////////////////////////////////
    //
    //  kleene_star class
    //
    //      Handles expressions of the form:
    //
    //          *a
    //
    //      where a is a parser. The expression returns a composite
    //      parser that matches its subject zero (0) or more times.
    //
    ///////////////////////////////////////////////////////////////////////////
    struct kleene_star_parser_gen;
    
#if BOOST_WORKAROUND(BOOST_MSVC, >= 1400)
#pragma warning(push)
#pragma warning(disable:4512) //assignment operator could not be generated
#endif

    template <typename S>
    struct kleene_star
    :   public unary<S, parser<kleene_star<S> > >
    {
        typedef kleene_star<S>              self_t;
        typedef unary_parser_category       parser_category_t;
        typedef kleene_star_parser_gen      parser_generator_t;
        typedef unary<S, parser<self_t> >   base_t;
    
        kleene_star(S const& a)
        : base_t(a) {}
    
        template <typename ScannerT>
        typename parser_result<self_t, ScannerT>::type
        parse(ScannerT const& scan) const
        {
            typedef typename parser_result<self_t, ScannerT>::type result_t;
            typedef typename ScannerT::iterator_t iterator_t;
            result_t hit = scan.empty_match();
    
            for (;;)
            {
                iterator_t save = scan.first;
                if (result_t next = this->subject().parse(scan))
                {
                    scan.concat_match(hit, next);
                }
                else
                {
                    scan.first = save;
                    return hit;
                }
            }
        }
    };
    
#if BOOST_WORKAROUND(BOOST_MSVC, >= 1400)
#pragma warning(pop)
#endif

    struct kleene_star_parser_gen
    {
        template <typename S>
        struct result 
        {
            typedef kleene_star<S> type;
        };
    
        template <typename S>
        static kleene_star<S>
        generate(parser<S> const& a)
        {
            return kleene_star<S>(a.derived());
        }
    };
    
    //////////////////////////////////
    template <typename S>
    kleene_star<S>
    operator*(parser<S> const& a);

BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}} // namespace BOOST_SPIRIT_CLASSIC_NS

#endif

#include <boost/spirit/home/classic/core/composite/impl/kleene_star.ipp>
