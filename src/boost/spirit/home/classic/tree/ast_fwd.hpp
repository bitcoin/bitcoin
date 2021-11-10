/*=============================================================================
    Copyright (c) 2006 Tobias Schwinger
    http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_TREE_AST_FWD_HPP)
#define BOOST_SPIRIT_TREE_AST_FWD_HPP

#include <boost/spirit/home/classic/namespace.hpp>
#include <boost/spirit/home/classic/core/nil.hpp>

namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

    template <
        typename MatchPolicyT, 
        typename NodeFactoryT, 
        typename T = nil_t
    >
    struct ast_tree_policy;

    template <
        typename IteratorT,
        typename NodeFactoryT = node_val_data_factory<nil_t>,
        typename T = nil_t
    >
    struct ast_match_policy;

    template <typename T>
    struct gen_ast_node_parser;

    struct root_node_op;

BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}} // namespace BOOST_SPIRIT_CLASSIC_NS

#endif

