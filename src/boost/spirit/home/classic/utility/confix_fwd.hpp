/*=============================================================================
    Copyright (c) 2006 Tobias Schwinger
    http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_CONFIX_FWD_HPP)
#define BOOST_SPIRIT_CONFIX_FWD_HPP

#include <boost/spirit/home/classic/namespace.hpp>
#include <boost/spirit/home/classic/core/parser.hpp>

namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

    struct is_nested;
    struct non_nested;
    struct is_lexeme;
    struct non_lexeme;

    template <
        typename OpenT, typename ExprT, typename CloseT, 
        typename CategoryT = plain_parser_category,
        typename NestedT = non_nested, typename LexemeT = non_lexeme
    >
    struct confix_parser;

    template<typename OpenT, typename CloseT>
    struct comment_nest_parser;

BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}} // namespace BOOST_SPIRIT_CLASSIC_NS

#endif


