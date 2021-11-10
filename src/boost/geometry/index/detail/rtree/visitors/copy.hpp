// Boost.Geometry Index
//
// R-tree deep copying visitor implementation
//
// Copyright (c) 2011-2015 Adam Wulkiewicz, Lodz, Poland.
//
// This file was modified by Oracle on 2019.
// Modifications copyright (c) 2019 Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle
//
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_INDEX_DETAIL_RTREE_VISITORS_COPY_HPP
#define BOOST_GEOMETRY_INDEX_DETAIL_RTREE_VISITORS_COPY_HPP

#include <boost/geometry/index/detail/rtree/node/subtree_destroyer.hpp>

namespace boost { namespace geometry { namespace index {

namespace detail { namespace rtree { namespace visitors {

template <typename MembersHolder>
class copy
    : public MembersHolder::visitor
{
    typedef typename MembersHolder::allocators_type allocators_type;

    typedef typename MembersHolder::node node;
    typedef typename MembersHolder::internal_node internal_node;
    typedef typename MembersHolder::leaf leaf;

    typedef rtree::subtree_destroyer<MembersHolder> subtree_destroyer;
    typedef typename allocators_type::node_pointer node_pointer;

public:
    explicit inline copy(allocators_type & allocators)
        : result(0)
        , m_allocators(allocators)
    {}

    inline void operator()(internal_node & n)
    {
        node_pointer raw_new_node = rtree::create_node<allocators_type, internal_node>::apply(m_allocators);      // MAY THROW, STRONG (N: alloc)
        subtree_destroyer new_node(raw_new_node, m_allocators);

        typedef typename rtree::elements_type<internal_node>::type elements_type;
        elements_type & elements = rtree::elements(n);

        elements_type & elements_dst = rtree::elements(rtree::get<internal_node>(*new_node));

        for (typename elements_type::iterator it = elements.begin();
            it != elements.end(); ++it)
        {
            rtree::apply_visitor(*this, *it->second);                                                   // MAY THROW (V, E: alloc, copy, N: alloc) 

            // for exception safety
            subtree_destroyer auto_result(result, m_allocators);

            elements_dst.push_back( rtree::make_ptr_pair(it->first, result) );                          // MAY THROW, STRONG (E: alloc, copy)

            auto_result.release();
        }

        result = new_node.get();
        new_node.release();
    }

    inline void operator()(leaf & l)
    {
        node_pointer raw_new_node = rtree::create_node<allocators_type, leaf>::apply(m_allocators);                // MAY THROW, STRONG (N: alloc)
        subtree_destroyer new_node(raw_new_node, m_allocators);

        typedef typename rtree::elements_type<leaf>::type elements_type;
        elements_type & elements = rtree::elements(l);

        elements_type & elements_dst = rtree::elements(rtree::get<leaf>(*new_node));

        for (typename elements_type::iterator it = elements.begin();
            it != elements.end(); ++it)
        {
            elements_dst.push_back(*it);                                                                // MAY THROW, STRONG (V: alloc, copy)
        }

        result = new_node.get();
        new_node.release();
    }

    node_pointer result;

private:
    allocators_type & m_allocators;
};

}}} // namespace detail::rtree::visitors

}}} // namespace boost::geometry::index

#endif // BOOST_GEOMETRY_INDEX_DETAIL_RTREE_VISITORS_COPY_HPP
