/*=============================================================================
    Copyright (c) 2006 Tobias Schwinger
    http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_TREE_COMMON_FWD_HPP)
#define BOOST_SPIRIT_TREE_COMMON_FWD_HPP

#include <boost/spirit/home/classic/namespace.hpp>
#include <boost/spirit/home/classic/core/nil.hpp>

namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

    template <typename T>
    struct tree_node;

    template <typename IteratorT = char const*, typename ValueT = nil_t>
    struct node_iter_data;

    template <typename ValueT = nil_t>
    class node_iter_data_factory;

    template <typename ValueT = nil_t>
    class node_val_data_factory;

    template <typename ValueT = nil_t>
    class node_all_val_data_factory;

    template <
        typename IteratorT,
        typename NodeFactoryT = node_val_data_factory<nil_t>,
        typename T = nil_t
    >
    class tree_match;

    struct tree_policy;

    template <
        typename MatchPolicyT,
        typename IteratorT,
        typename NodeFactoryT,
        typename TreePolicyT, 
        typename T = nil_t
    >
    struct common_tree_match_policy;

    template <typename MatchPolicyT, typename NodeFactoryT>
    struct common_tree_tree_policy;

    template <typename T>
    struct no_tree_gen_node_parser;

    template <typename T>
    struct leaf_node_parser;

    template <typename T, typename NodeParserT>
    struct node_parser;

    struct discard_node_op;
    struct reduced_node_op;
    struct infix_node_op;
    struct discard_first_node_op;
    struct discard_last_node_op;
    struct inner_node_op;

    template <typename T, typename ActionParserT>
    struct action_directive_parser;

    struct access_match_action
    {
        template <typename ParserT, typename ActionT>
        struct action;
    };

    struct access_node_action
    {
        template <typename ParserT, typename ActionT>
        struct action;
    };

    template <
        typename IteratorT = char const *,
        typename NodeFactoryT = node_val_data_factory<nil_t>,
        typename T = nil_t
    >
    struct tree_parse_info;

BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}} // namespace BOOST_SPIRIT_CLASSIC_NS

#endif
