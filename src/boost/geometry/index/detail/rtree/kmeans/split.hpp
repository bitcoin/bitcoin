// Boost.Geometry Index
//
// R-tree kmeans split algorithm implementation
//
// Copyright (c) 2011-2013 Adam Wulkiewicz, Lodz, Poland.
//
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_INDEX_DETAIL_RTREE_KMEANS_SPLIT_HPP
#define BOOST_GEOMETRY_INDEX_DETAIL_RTREE_KMEANS_SPLIT_HPP

#include <boost/geometry/index/rtree/node/node.hpp>
#include <boost/geometry/index/rtree/visitors/insert.hpp>

namespace boost { namespace geometry { namespace index {

namespace detail { namespace rtree {

namespace kmeans {

// some details

} // namespace kmeans

// split_kmeans_tag
// OR
// split_clusters_tag and redistribute_kmeans_tag - then redistribute will probably slightly different interface
// or some other than "redistribute"

// 1. for this algorithm one probably MUST USE NON-STATIC NODES with node_default_tag
//    or the algorithm must be changed somehow - to not store additional nodes in the current node
//    but return excessive element/elements container instead (possibly pushable_array<1> or std::vector)
//    this would also cause building of smaller trees since +1 element in nodes wouldn't be needed in different redistributing algorithms
// 2. it is probably possible to add e.g. 2 levels of tree in one insert

// Edge case is that every node split generates M + 1 children, in parent containing M nodes
// result is 2M + 1 nodes in parent on this level
// On next level the same, next the same and so on.
// We have Depth*M+1 nodes in the root
// The tree may then gain some > 1 levels in one insert
// split::apply() manages this but special attention is required

// which algorithm should be used to choose current node in traversing while inserting?
// some of the currently used ones or some using mean values as well?

// TODO
// 1. Zmienic troche algorytm zeby przekazywal nadmiarowe elementy do split
//    i pobieral ze split nadmiarowe elementy rodzica
//    W zaleznosci od algorytmu w rozny sposob - l/q/r* powinny zwracac np pushable_array<1>
//    wtedy tez is_overerflow (z R* insert?) bedzie nieportrzebne
//    Dla kmeans zapewne std::vector, jednak w wezlach nadal moglaby byc pushable_array
// 2. Fajnie byloby tez uproscic te wszystkie parametry root,parent,index itd. Mozliwe ze okazalyby sie zbedne
// 3. Sprawdzyc czasy wykonywania i zajetosc pamieci
// 4. Pamietac o parametryzacji kontenera z nadmiarowymi elementami
// PS. Z R* reinsertami moze byc masakra

template <typename Value, typename Options, typename Translator, typename Box, typename Allocators>
class split<Value, Options, Translator, Box, Allocators, split_kmeans_tag>
{
protected:
    typedef typename rtree::node<Value, typename Options::parameters_type, Box, Allocators, typename Options::node_tag>::type node;
    typedef typename rtree::internal_node<Value, typename Options::parameters_type, Box, Allocators, typename Options::node_tag>::type internal_node;
    typedef typename rtree::leaf<Value, typename Options::parameters_type, Box, Allocators, typename Options::node_tag>::type leaf;

    typedef typename Options::parameters_type parameters_type;

public:
    template <typename Node>
    static inline void apply(node* & root_node,
                             size_t & leafs_level,
                             Node & n,
                             internal_node *parent_node,
                             size_t current_child_index,
                             Translator const& tr,
                             Allocators & allocators)
    {

    }
};

}} // namespace detail::rtree

}}} // namespace boost::geometry::index

#endif // BOOST_GEOMETRY_INDEX_DETAIL_RTREE_KMEANS_SPLIT_HPP
