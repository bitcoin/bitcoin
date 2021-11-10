/*=============================================================================
    Copyright (c) 2001-2003 Daniel Nuffer
    Copyright (c) 2001-2007 Hartmut Kaiser
    http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#ifndef BOOST_SPIRIT_CLASSIC_TREE_PARSE_TREE_UTILS_HPP
#define BOOST_SPIRIT_CLASSIC_TREE_PARSE_TREE_UTILS_HPP

#include <utility>                          // for std::pair

#include <boost/spirit/home/classic/tree/parse_tree.hpp> // needed for parse tree generation

///////////////////////////////////////////////////////////////////////////////
namespace boost {
namespace spirit {
BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

///////////////////////////////////////////////////////////////////////////////
//
//  The function 'get_first_leaf' returnes a reference to the first leaf node
//  of the given parsetree.
//
///////////////////////////////////////////////////////////////////////////////
template <typename T>
tree_node<T> const &
get_first_leaf (tree_node<T> const &node);

///////////////////////////////////////////////////////////////////////////////
//
//  The function 'find_node' finds a specified node through recursive search.
//  If the return value is true, the variable to which points the parameter
//  'found_node' will contain the address of the node with the given rule_id.
//
///////////////////////////////////////////////////////////////////////////////
template <typename T>
bool
find_node (tree_node<T> const &node, parser_id node_to_search,
    tree_node<T> const **found_node);

///////////////////////////////////////////////////////////////////////////////
//
//  The function 'get_node_range' return a pair of iterators pointing at the
//  range, which contains the elements of a specified node. It's very useful
//  for locating all information related with a specified node.
//
///////////////////////////////////////////////////////////////////////////////
template <typename T>
bool
get_node_range (tree_node<T> const &node, parser_id node_to_search,
    std::pair<typename tree_node<T>::const_tree_iterator,
        typename tree_node<T>::const_tree_iterator> &nodes);

///////////////////////////////////////////////////////////////////////////////
BOOST_SPIRIT_CLASSIC_NAMESPACE_END
}   // namespace spirit
}   // namespace boost

#include <boost/spirit/home/classic/tree/impl/parse_tree_utils.ipp>

#endif
