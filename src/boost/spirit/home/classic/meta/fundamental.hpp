/*=============================================================================
    Copyright (c) 2002-2003 Hartmut Kaiser
    http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_FUNDAMENTAL_HPP)
#define BOOST_SPIRIT_FUNDAMENTAL_HPP

#include <boost/spirit/home/classic/namespace.hpp>
#include <boost/spirit/home/classic/meta/impl/fundamental.ipp>

namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

    ///////////////////////////////////////////////////////////////////////////
    //
    //  Helper template for counting the number of nodes contained in a
    //  given parser type.
    //  All parser_category type parsers are counted as nodes.
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename ParserT>
    struct node_count {

        typedef typename ParserT::parser_category_t parser_category_t;
        typedef typename impl::nodes<parser_category_t>
            ::template count<ParserT, mpl::int_<0> > count_t;

        BOOST_STATIC_CONSTANT(int, value = count_t::value);
    };

    ///////////////////////////////////////////////////////////////////////////
    //
    //  Helper template for counting the number of leaf nodes contained in a
    //  given parser type.
    //  Only plain_parser_category type parsers are counted as leaf nodes.
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename ParserT>
    struct leaf_count {

        typedef typename ParserT::parser_category_t parser_category_t;
        typedef typename impl::leafs<parser_category_t>
            ::template count<ParserT, mpl::int_<0> > count_t;

        BOOST_STATIC_CONSTANT(int, value = count_t::value);
    };

BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}} // namespace BOOST_SPIRIT_CLASSIC_NS

#endif // !defined(BOOST_SPIRIT_FUNDAMENTAL_HPP)
