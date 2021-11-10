// Boost.Geometry Index
//
// R-tree node concept
//
// Copyright (c) 2011-2013 Adam Wulkiewicz, Lodz, Poland.
//
// This file was modified by Oracle on 2020.
// Modifications copyright (c) 2020 Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle
//
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_INDEX_DETAIL_RTREE_NODE_CONCEPT_HPP
#define BOOST_GEOMETRY_INDEX_DETAIL_RTREE_NODE_CONCEPT_HPP

#include <boost/geometry/core/static_assert.hpp>

namespace boost { namespace geometry { namespace index {

namespace detail { namespace rtree {

template <typename Value, typename Parameters, typename Box, typename Allocators, typename Tag>
struct node
{
    BOOST_GEOMETRY_STATIC_ASSERT_FALSE(
        "Not implemented for this Tag type.",
        Value, Parameters, Box, Allocators, Tag);
};

template <typename Value, typename Parameters, typename Box, typename Allocators, typename Tag>
struct internal_node
{
    BOOST_GEOMETRY_STATIC_ASSERT_FALSE(
        "Not implemented for this Tag type.",
        Value, Parameters, Box, Allocators, Tag);
};

template <typename Value, typename Parameters, typename Box, typename Allocators, typename Tag>
struct leaf
{
    BOOST_GEOMETRY_STATIC_ASSERT_FALSE(
        "Not implemented for this Tag type.",
        Value, Parameters, Box, Allocators, Tag);
};

template <typename Value, typename Parameters, typename Box, typename Allocators, typename Tag, bool IsVisitableConst>
struct visitor
{
    BOOST_GEOMETRY_STATIC_ASSERT_FALSE(
        "Not implemented for this Tag type.",
        Value, Parameters, Box, Allocators, Tag);
};

template <typename Allocator, typename Value, typename Parameters, typename Box, typename Tag>
class allocators
{
    BOOST_GEOMETRY_STATIC_ASSERT_FALSE(
        "Not implemented for this Tag type.",
        Allocator, Value, Parameters, Box, Tag);
};

template <typename Allocators, typename Node>
struct create_node
{
    BOOST_GEOMETRY_STATIC_ASSERT_FALSE(
        "Not implemented for this Node type.",
        Allocators, Node);
};

template <typename Allocators, typename Node>
struct destroy_node
{
    BOOST_GEOMETRY_STATIC_ASSERT_FALSE(
        "Not implemented for this Node type.",
        Allocators, Node);
};

}} // namespace detail::rtree

}}} // namespace boost::geometry::index

#endif // BOOST_GEOMETRY_INDEX_DETAIL_RTREE_NODE_CONCEPT_HPP
