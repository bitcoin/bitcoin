// Boost.Geometry Index
//
// R-tree destroying visitor implementation
//
// Copyright (c) 2011-2014 Adam Wulkiewicz, Lodz, Poland.
//
// This file was modified by Oracle on 2019.
// Modifications copyright (c) 2019 Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle
//
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_INDEX_DETAIL_RTREE_VISITORS_DELETE_HPP
#define BOOST_GEOMETRY_INDEX_DETAIL_RTREE_VISITORS_DELETE_HPP

namespace boost { namespace geometry { namespace index {

namespace detail { namespace rtree { namespace visitors {

template <typename MembersHolder>
class destroy
    : public MembersHolder::visitor
{
public:
    typedef typename MembersHolder::node node;
    typedef typename MembersHolder::internal_node internal_node;
    typedef typename MembersHolder::leaf leaf;

    typedef typename MembersHolder::allocators_type allocators_type;
    typedef typename MembersHolder::node_pointer node_pointer;

    inline destroy(node_pointer node, allocators_type & allocators)
        : m_current_node(node)
        , m_allocators(allocators)
    {}

    inline void operator()(internal_node & n)
    {
        BOOST_GEOMETRY_INDEX_ASSERT(&n == &rtree::get<internal_node>(*m_current_node), "invalid pointers");

        node_pointer node_to_destroy = m_current_node;

        typedef typename rtree::elements_type<internal_node>::type elements_type;
        elements_type & elements = rtree::elements(n);

        for (typename elements_type::iterator it = elements.begin();
             it != elements.end(); ++it)
        {
            m_current_node = it->second;
            rtree::apply_visitor(*this, *m_current_node);
            it->second = 0;
        }

        rtree::destroy_node<allocators_type, internal_node>::apply(m_allocators, node_to_destroy);
    }

    inline void operator()(leaf & l)
    {
        boost::ignore_unused(l);
        BOOST_GEOMETRY_INDEX_ASSERT(&l == &rtree::get<leaf>(*m_current_node), "invalid pointers");

        rtree::destroy_node<allocators_type, leaf>::apply(m_allocators, m_current_node);
    }

    static inline void apply(node_pointer node, allocators_type & allocators)
    {
        destroy v(node, allocators);
        rtree::apply_visitor(v, *node);
    }

private:
    node_pointer m_current_node;
    allocators_type & m_allocators;
};

}}} // namespace detail::rtree::visitors

}}} // namespace boost::geometry::index

#endif // BOOST_GEOMETRY_INDEX_DETAIL_RTREE_VISITORS_DELETE_HPP
