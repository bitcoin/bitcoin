/*=============================================================================
    Copyright (c) 2001-2007 Hartmut Kaiser
    Copyright (c) 2001-2003 Daniel Nuffer
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#ifndef BOOST_SPIRIT_CLASSIC_TREE_IMPL_PARSE_TREE_UTILS_IPP
#define BOOST_SPIRIT_CLASSIC_TREE_IMPL_PARSE_TREE_UTILS_IPP

///////////////////////////////////////////////////////////////////////////////
namespace boost {
namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

///////////////////////////////////////////////////////////////////////////////
//
//  Returnes the first leaf node of the given parsetree.
//
///////////////////////////////////////////////////////////////////////////////
template <typename T>
inline tree_node<T> const &
get_first_leaf (tree_node<T> const &node)
{
    if (node.children.size() > 0)
        return get_first_leaf(*node.children.begin());
    return node;
}

///////////////////////////////////////////////////////////////////////////////
//
//  Find a specified node through recursive search.
//
///////////////////////////////////////////////////////////////////////////////
template <typename T>
inline bool
find_node (tree_node<T> const &node, parser_id node_to_search,
    tree_node<T> const **found_node)
{
    if (node.value.id() == node_to_search) {
        *found_node = &node;
        return true;
    }
    if (node.children.size() > 0) {
        typedef typename tree_node<T>::const_tree_iterator const_tree_iterator;

        const_tree_iterator end = node.children.end();
        for (const_tree_iterator it = node.children.begin(); it != end; ++it)
        {
            if (find_node (*it, node_to_search, found_node))
                return true;
        }
    }
    return false;   // not found here
}

///////////////////////////////////////////////////////////////////////////////
//
//  The functions 'get_node_range' return a pair of iterators pointing at the
//  range, which contains the elements of a specified node.
//
///////////////////////////////////////////////////////////////////////////////
namespace impl {

template <typename T>
inline bool
get_node_range (typename tree_node<T>::const_tree_iterator const &start,
    parser_id node_to_search,
    std::pair<typename tree_node<T>::const_tree_iterator,
        typename tree_node<T>::const_tree_iterator> &nodes)
{
// look at this node first
tree_node<T> const &node = *start;

    if (node.value.id() == node_to_search) {
        if (node.children.size() > 0) {
        // full subrange
            nodes.first = node.children.begin();
            nodes.second = node.children.end();
        }
        else {
        // only this node
            nodes.first = start;
            nodes.second = start;
            std::advance(nodes.second, 1);
        }
        return true;
    }

// look at subnodes now
    if (node.children.size() > 0) {
        typedef typename tree_node<T>::const_tree_iterator const_tree_iterator;

        const_tree_iterator end = node.children.end();
        for (const_tree_iterator it = node.children.begin(); it != end; ++it)
        {
            if (impl::get_node_range<T>(it, node_to_search, nodes))
                return true;
        }
    }
    return false;
}

} // end of namespace impl

template <typename T>
inline bool
get_node_range (tree_node<T> const &node, parser_id node_to_search,
    std::pair<typename tree_node<T>::const_tree_iterator,
        typename tree_node<T>::const_tree_iterator> &nodes)
{
    if (node.children.size() > 0) {
        typedef typename tree_node<T>::const_tree_iterator const_tree_iterator;

        const_tree_iterator end = node.children.end();
        for (const_tree_iterator it = node.children.begin(); it != end; ++it)
        {
            if (impl::get_node_range<T>(it, node_to_search, nodes))
                return true;
        }
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}   // namespace spirit
}   // namespace boost

#endif
