/*=============================================================================
    Copyright (c) 1998-2003 Joel de Guzman
    Copyright (c) 2001 Daniel Nuffer
    Copyright (c) 2002 Hartmut Kaiser
    http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_OPTIONAL_HPP)
#define BOOST_SPIRIT_OPTIONAL_HPP

#include <boost/spirit/home/classic/namespace.hpp>
#include <boost/spirit/home/classic/core/parser.hpp>
#include <boost/spirit/home/classic/core/primitives/primitives.hpp>
#include <boost/spirit/home/classic/core/composite/composite.hpp>
#include <boost/spirit/home/classic/meta/as_parser.hpp>

namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

    ///////////////////////////////////////////////////////////////////////////
    //
    //  optional class
    //
    //      Handles expressions of the form:
    //
    //          !a
    //
    //      where a is a parser. The expression returns a composite
    //      parser that matches its subject zero (0) or one (1) time.
    //
    ///////////////////////////////////////////////////////////////////////////
    struct optional_parser_gen;
    
    template <typename S>
    struct optional
    :   public unary<S, parser<optional<S> > >
    {
        typedef optional<S>                 self_t;
        typedef unary_parser_category       parser_category_t;
        typedef optional_parser_gen         parser_generator_t;
        typedef unary<S, parser<self_t> >   base_t;
    
        optional(S const& a)
        : base_t(a) {}
    
        template <typename ScannerT>
        typename parser_result<self_t, ScannerT>::type
        parse(ScannerT const& scan) const
        {
            typedef typename parser_result<self_t, ScannerT>::type result_t;
            typedef typename ScannerT::iterator_t iterator_t;
            iterator_t save = scan.first;
            if (result_t r = this->subject().parse(scan))
            {
                return r;
            }
            else
            {
                scan.first = save;
                return scan.empty_match();
            }
        }
    };
    
    struct optional_parser_gen
    {
        template <typename S>
        struct result 
        {
            typedef optional<S> type;
        };
    
        template <typename S>
        static optional<S>
        generate(parser<S> const& a)
        {
            return optional<S>(a.derived());
        }
    };
    
    template <typename S>
    optional<S>
    operator!(parser<S> const& a);

BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}} // namespace BOOST_SPIRIT_CLASSIC_NS

#endif

#include <boost/spirit/home/classic/core/composite/impl/optional.ipp>
