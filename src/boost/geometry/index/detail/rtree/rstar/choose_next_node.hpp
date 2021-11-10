// Boost.Geometry Index
//
// R-tree R*-tree next node choosing algorithm implementation
//
// Copyright (c) 2011-2019 Adam Wulkiewicz, Lodz, Poland.
//
// This file was modified by Oracle on 2019.
// Modifications copyright (c) 2019 Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle
//
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_INDEX_DETAIL_RTREE_RSTAR_CHOOSE_NEXT_NODE_HPP
#define BOOST_GEOMETRY_INDEX_DETAIL_RTREE_RSTAR_CHOOSE_NEXT_NODE_HPP

#include <algorithm>

#include <boost/core/ignore_unused.hpp>

#include <boost/geometry/algorithms/expand.hpp>

#include <boost/geometry/index/detail/algorithms/content.hpp>
#include <boost/geometry/index/detail/algorithms/intersection_content.hpp>
#include <boost/geometry/index/detail/algorithms/nth_element.hpp>
#include <boost/geometry/index/detail/algorithms/union_content.hpp>

#include <boost/geometry/index/detail/rtree/node/node.hpp>
#include <boost/geometry/index/detail/rtree/visitors/is_leaf.hpp>

namespace boost { namespace geometry { namespace index {

namespace detail { namespace rtree {

template <typename MembersHolder>
class choose_next_node<MembersHolder, choose_by_overlap_diff_tag>
{
    typedef typename MembersHolder::box_type box_type;
    typedef typename MembersHolder::parameters_type parameters_type;

    typedef typename MembersHolder::node node;
    typedef typename MembersHolder::internal_node internal_node;
    typedef typename MembersHolder::leaf leaf;

    typedef typename rtree::elements_type<internal_node>::type children_type;
    typedef typename children_type::value_type child_type;

    typedef typename index::detail::default_content_result<box_type>::type content_type;

public:
    template <typename Indexable>
    static inline size_t apply(internal_node & n,
                               Indexable const& indexable,
                               parameters_type const& parameters,
                               size_t node_relative_level)
    {
        ::boost::ignore_unused(parameters);

        children_type & children = rtree::elements(n);
        
        // children are leafs
        if ( node_relative_level <= 1 )
        {
            return choose_by_minimum_overlap_cost(children, indexable,
                                                  parameters.get_overlap_cost_threshold(),
                                                  index::detail::get_strategy(parameters));
        }
        // children are internal nodes
        else
        {
            return choose_by_minimum_content_cost(children, indexable,
                                                  index::detail::get_strategy(parameters));
        }
    }

private:
    struct child_contents
    {
        content_type content_diff;
        content_type content;
        size_t i;

        void set(size_t i_, content_type const& content_, content_type const& content_diff_)
        {
            i = i_;
            content = content_;
            content_diff = content_diff_;
        }
    };

    template <typename Indexable, typename Strategy>
    static inline size_t choose_by_minimum_overlap_cost(children_type const& children,
                                                        Indexable const& indexable,
                                                        size_t overlap_cost_threshold,
                                                        Strategy const& strategy)
    {
        const size_t children_count = children.size();

        content_type min_content_diff = (std::numeric_limits<content_type>::max)();
        content_type min_content = (std::numeric_limits<content_type>::max)();
        size_t choosen_index = 0;

        // create container of children sorted by content enlargement needed to include the new value
        typename rtree::container_from_elements_type<children_type, child_contents>::type
            children_contents(children_count);

        for ( size_t i = 0 ; i < children_count ; ++i )
        {
            child_type const& ch_i = children[i];

            // expanded child node's box
            box_type box_exp(ch_i.first);
            index::detail::expand(box_exp, indexable, strategy);

            // areas difference
            content_type content = index::detail::content(box_exp);
            content_type content_diff = content - index::detail::content(ch_i.first);

            children_contents[i].set(i, content, content_diff);

            if ( content_diff < min_content_diff ||
                 (content_diff == min_content_diff && content < min_content) )
            {
                min_content_diff = content_diff;
                min_content = content;
                choosen_index = i;
            }
        }

        // is this assumption ok? if min_content_diff == 0 there is no overlap increase?

        if ( min_content_diff < -std::numeric_limits<double>::epsilon() || std::numeric_limits<double>::epsilon() < min_content_diff )
        {
            size_t first_n_children_count = children_count;
            if ( 0 < overlap_cost_threshold && overlap_cost_threshold < children.size() )
            {
                first_n_children_count = overlap_cost_threshold;
                // rearrange by content_diff
                // in order to calculate nearly minimum overlap cost
                index::detail::nth_element(children_contents.begin(), children_contents.begin() + first_n_children_count, children_contents.end(), content_diff_less);
            }

            // calculate minimum or nearly minimum overlap cost
            choosen_index = choose_by_minimum_overlap_cost_first_n(children, indexable,
                                                                   first_n_children_count,
                                                                   children_count,
                                                                   children_contents,
                                                                   strategy);
        }

        return choosen_index;
    }

    static inline bool content_diff_less(child_contents const& p1, child_contents const& p2)
    {
        return p1.content_diff < p2.content_diff
            || (p1.content_diff == p2.content_diff && (p1.content) < (p2.content));
    }

    template <typename Indexable, typename ChildrenContents, typename Strategy>
    static inline size_t choose_by_minimum_overlap_cost_first_n(children_type const& children,
                                                                Indexable const& indexable,
                                                                size_t const first_n_children_count,
                                                                size_t const children_count,
                                                                ChildrenContents const& children_contents,
                                                                Strategy const& strategy)
    {
        BOOST_GEOMETRY_INDEX_ASSERT(first_n_children_count <= children_count, "unexpected value");
        BOOST_GEOMETRY_INDEX_ASSERT(children_contents.size() == children_count, "unexpected number of elements");

        // choose index with smallest overlap change value, or content change or smallest content
        size_t choosen_index = 0;
        content_type smallest_overlap_diff = (std::numeric_limits<content_type>::max)();
        content_type smallest_content_diff = (std::numeric_limits<content_type>::max)();
        content_type smallest_content = (std::numeric_limits<content_type>::max)();

        // for each child node
        for (size_t first_i = 0 ; first_i < first_n_children_count ; ++first_i)
        {
            size_t i = children_contents[first_i].i;
            content_type const& content = children_contents[first_i].content;
            content_type const& content_diff = children_contents[first_i].content_diff;

            child_type const& ch_i = children[i];

            box_type box_exp(ch_i.first);
            // calculate expanded box of child node ch_i
            index::detail::expand(box_exp, indexable, strategy);

            content_type overlap_diff = 0;

            // calculate overlap
            for ( size_t j = 0 ; j < children_count ; ++j )
            {
                if ( i != j )
                {
                    child_type const& ch_j = children[j];

                    content_type overlap_exp = index::detail::intersection_content(box_exp, ch_j.first, strategy);
                    if ( overlap_exp < -std::numeric_limits<content_type>::epsilon() || std::numeric_limits<content_type>::epsilon() < overlap_exp )
                    {
                        overlap_diff += overlap_exp - index::detail::intersection_content(ch_i.first, ch_j.first, strategy);
                    }
                }
            }

            // update result
            if ( overlap_diff < smallest_overlap_diff ||
                ( overlap_diff == smallest_overlap_diff && ( content_diff < smallest_content_diff ||
                ( content_diff == smallest_content_diff && content < smallest_content ) )
                ) )
            {
                smallest_overlap_diff = overlap_diff;
                smallest_content_diff = content_diff;
                smallest_content = content;
                choosen_index = i;
            }
        }

        return choosen_index;
    }

    template <typename Indexable, typename Strategy>
    static inline size_t choose_by_minimum_content_cost(children_type const& children,
                                                        Indexable const& indexable,
                                                        Strategy const& strategy)
    {
        size_t children_count = children.size();

        // choose index with smallest content change or smallest content
        size_t choosen_index = 0;
        content_type smallest_content_diff = (std::numeric_limits<content_type>::max)();
        content_type smallest_content = (std::numeric_limits<content_type>::max)();

        // choose the child which requires smallest box expansion to store the indexable
        for ( size_t i = 0 ; i < children_count ; ++i )
        {
            child_type const& ch_i = children[i];

            // expanded child node's box
            box_type box_exp(ch_i.first);
            index::detail::expand(box_exp, indexable, strategy);

            // areas difference
            content_type content = index::detail::content(box_exp);
            content_type content_diff = content - index::detail::content(ch_i.first);

            // update the result
            if ( content_diff < smallest_content_diff ||
                ( content_diff == smallest_content_diff && content < smallest_content ) )
            {
                smallest_content_diff = content_diff;
                smallest_content = content;
                choosen_index = i;
            }
        }

        return choosen_index;
    }
};

}} // namespace detail::rtree

}}} // namespace boost::geometry::index

#endif // BOOST_GEOMETRY_INDEX_DETAIL_RTREE_RSTAR_CHOOSE_NEXT_NODE_HPP
