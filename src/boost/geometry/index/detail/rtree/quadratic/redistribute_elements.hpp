// Boost.Geometry Index
//
// R-tree quadratic split algorithm implementation
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

#ifndef BOOST_GEOMETRY_INDEX_DETAIL_RTREE_QUADRATIC_REDISTRIBUTE_ELEMENTS_HPP
#define BOOST_GEOMETRY_INDEX_DETAIL_RTREE_QUADRATIC_REDISTRIBUTE_ELEMENTS_HPP

#include <algorithm>

#include <boost/core/ignore_unused.hpp>

#include <boost/geometry/index/detail/algorithms/content.hpp>
#include <boost/geometry/index/detail/algorithms/union_content.hpp>

#include <boost/geometry/index/detail/bounded_view.hpp>

#include <boost/geometry/index/detail/rtree/node/node.hpp>
#include <boost/geometry/index/detail/rtree/visitors/insert.hpp>
#include <boost/geometry/index/detail/rtree/visitors/is_leaf.hpp>

namespace boost { namespace geometry { namespace index {

namespace detail { namespace rtree {

namespace quadratic {

template <typename Box, typename Elements, typename Parameters, typename Translator>
inline void pick_seeds(Elements const& elements,
                       Parameters const& parameters,
                       Translator const& tr,
                       size_t & seed1,
                       size_t & seed2)
{
    typedef typename Elements::value_type element_type;
    typedef typename rtree::element_indexable_type<element_type, Translator>::type indexable_type;
    typedef Box box_type;
    typedef typename index::detail::default_content_result<box_type>::type content_type;
    typedef typename index::detail::strategy_type<Parameters>::type strategy_type;
    typedef index::detail::bounded_view
        <
            indexable_type, box_type, strategy_type
        > bounded_indexable_view;

    const size_t elements_count = parameters.get_max_elements() + 1;
    BOOST_GEOMETRY_INDEX_ASSERT(elements.size() == elements_count, "wrong number of elements");
    BOOST_GEOMETRY_INDEX_ASSERT(2 <= elements_count, "unexpected number of elements");

    strategy_type const& strategy = index::detail::get_strategy(parameters);

    content_type greatest_free_content = 0;
    seed1 = 0;
    seed2 = 1;

    for ( size_t i = 0 ; i < elements_count - 1 ; ++i )
    {
        for ( size_t j = i + 1 ; j < elements_count ; ++j )
        {
            indexable_type const& ind1 = rtree::element_indexable(elements[i], tr);
            indexable_type const& ind2 = rtree::element_indexable(elements[j], tr);

            box_type enlarged_box;
            index::detail::bounds(ind1, enlarged_box, strategy);
            index::detail::expand(enlarged_box, ind2, strategy);

            bounded_indexable_view bounded_ind1(ind1, strategy);
            bounded_indexable_view bounded_ind2(ind2, strategy);
            content_type free_content = ( index::detail::content(enlarged_box)
                                            - index::detail::content(bounded_ind1) )
                                                - index::detail::content(bounded_ind2);
                
            if ( greatest_free_content < free_content )
            {
                greatest_free_content = free_content;
                seed1 = i;
                seed2 = j;
            }
        }
    }

    ::boost::ignore_unused(parameters);
}

} // namespace quadratic

template <typename MembersHolder>
struct redistribute_elements<MembersHolder, quadratic_tag>
{
    typedef typename MembersHolder::box_type box_type;
    typedef typename MembersHolder::parameters_type parameters_type;
    typedef typename MembersHolder::translator_type translator_type;
    typedef typename MembersHolder::allocators_type allocators_type;

    typedef typename MembersHolder::node node;
    typedef typename MembersHolder::internal_node internal_node;
    typedef typename MembersHolder::leaf leaf;

    typedef typename index::detail::default_content_result<box_type>::type content_type;

    template <typename Node>
    static inline void apply(Node & n,
                             Node & second_node,
                             box_type & box1,
                             box_type & box2,
                             parameters_type const& parameters,
                             translator_type const& translator,
                             allocators_type & allocators)
    {
        typedef typename rtree::elements_type<Node>::type elements_type;
        typedef typename elements_type::value_type element_type;
        typedef typename rtree::element_indexable_type<element_type, translator_type>::type indexable_type;

        elements_type & elements1 = rtree::elements(n);
        elements_type & elements2 = rtree::elements(second_node);
        
        BOOST_GEOMETRY_INDEX_ASSERT(elements1.size() == parameters.get_max_elements() + 1, "unexpected elements number");

        // copy original elements - use in-memory storage (std::allocator)
        // TODO: move if noexcept
        typedef typename rtree::container_from_elements_type<elements_type, element_type>::type
            container_type;
        container_type elements_copy(elements1.begin(), elements1.end());                                   // MAY THROW, STRONG (alloc, copy)
        container_type elements_backup(elements1.begin(), elements1.end());                                 // MAY THROW, STRONG (alloc, copy)
        
        // calculate initial seeds
        size_t seed1 = 0;
        size_t seed2 = 0;
        quadratic::pick_seeds<box_type>(elements_copy, parameters, translator, seed1, seed2);

        // prepare nodes' elements containers
        elements1.clear();
        BOOST_GEOMETRY_INDEX_ASSERT(elements2.empty(), "second node's elements container should be empty");

        BOOST_TRY
        {
            typename index::detail::strategy_type<parameters_type>::type const&
                strategy = index::detail::get_strategy(parameters);

            // add seeds
            elements1.push_back(elements_copy[seed1]);                                                      // MAY THROW, STRONG (copy)
            elements2.push_back(elements_copy[seed2]);                                                      // MAY THROW, STRONG (alloc, copy)

            // calculate boxes
            detail::bounds(rtree::element_indexable(elements_copy[seed1], translator),
                           box1, strategy);
            detail::bounds(rtree::element_indexable(elements_copy[seed2], translator),
                           box2, strategy);

            // remove seeds
            if (seed1 < seed2)
            {
                rtree::move_from_back(elements_copy, elements_copy.begin() + seed2);                        // MAY THROW, STRONG (copy)
                elements_copy.pop_back();
                rtree::move_from_back(elements_copy, elements_copy.begin() + seed1);                        // MAY THROW, STRONG (copy)
                elements_copy.pop_back();
            }
            else
            {
                rtree::move_from_back(elements_copy, elements_copy.begin() + seed1);                        // MAY THROW, STRONG (copy)
                elements_copy.pop_back();
                rtree::move_from_back(elements_copy, elements_copy.begin() + seed2);                        // MAY THROW, STRONG (copy)
                elements_copy.pop_back();
            }

            // initialize areas
            content_type content1 = index::detail::content(box1);
            content_type content2 = index::detail::content(box2);

            size_t remaining = elements_copy.size();

            // redistribute the rest of the elements
            while ( !elements_copy.empty() )
            {
                typename container_type::reverse_iterator el_it = elements_copy.rbegin();
                bool insert_into_group1 = false;

                size_t elements1_count = elements1.size();
                size_t elements2_count = elements2.size();

                // if there is small number of elements left and the number of elements in node is lesser than min_elems
                // just insert them to this node
                if ( elements1_count + remaining <= parameters.get_min_elements() )
                {
                    insert_into_group1 = true;
                }
                else if ( elements2_count + remaining <= parameters.get_min_elements() )
                {
                    insert_into_group1 = false;
                }
                // insert the best element
                else
                {
                    // find element with minimum groups areas increses differences
                    content_type content_increase1 = 0;
                    content_type content_increase2 = 0;
                    el_it = pick_next(elements_copy.rbegin(), elements_copy.rend(),
                                      box1, box2, content1, content2,
                                      translator, strategy,
                                      content_increase1, content_increase2);

                    if ( content_increase1 < content_increase2 ||
                         ( content_increase1 == content_increase2 && ( content1 < content2 ||
                           ( content1 == content2 && elements1_count <= elements2_count ) )
                         ) )
                    {
                        insert_into_group1 = true;
                    }
                    else
                    {
                        insert_into_group1 = false;
                    }
                }

                // move element to the choosen group
                element_type const& elem = *el_it;
                indexable_type const& indexable = rtree::element_indexable(elem, translator);

                if ( insert_into_group1 )
                {
                    elements1.push_back(elem);                                                              // MAY THROW, STRONG (copy)
                    index::detail::expand(box1, indexable, strategy);
                    content1 = index::detail::content(box1);
                }
                else
                {
                    elements2.push_back(elem);                                                              // MAY THROW, STRONG (alloc, copy)
                    index::detail::expand(box2, indexable, strategy);
                    content2 = index::detail::content(box2);
                }

                BOOST_GEOMETRY_INDEX_ASSERT(!elements_copy.empty(), "expected more elements");
                typename container_type::iterator el_it_base = el_it.base();
                rtree::move_from_back(elements_copy, --el_it_base);                                         // MAY THROW, STRONG (copy)
                elements_copy.pop_back();

                BOOST_GEOMETRY_INDEX_ASSERT(0 < remaining, "expected more remaining elements");
                --remaining;
            }
        }
        BOOST_CATCH(...)
        {
            //elements_copy.clear();
            elements1.clear();
            elements2.clear();

            rtree::destroy_elements<MembersHolder>::apply(elements_backup, allocators);
            //elements_backup.clear();

            BOOST_RETHROW                                                                                     // RETHROW, BASIC
        }
        BOOST_CATCH_END
    }

    // TODO: awulkiew - change following function to static member of the pick_next class?

    template <typename It>
    static inline It pick_next(It first, It last,
                               box_type const& box1, box_type const& box2,
                               content_type const& content1, content_type const& content2,
                               translator_type const& translator,
                               typename index::detail::strategy_type<parameters_type>::type const& strategy,
                               content_type & out_content_increase1, content_type & out_content_increase2)
    {
        typedef typename boost::iterator_value<It>::type element_type;
        typedef typename rtree::element_indexable_type<element_type, translator_type>::type indexable_type;

        content_type greatest_content_incrase_diff = 0;
        It out_it = first;
        out_content_increase1 = 0;
        out_content_increase2 = 0;
        
        // find element with greatest difference between increased group's boxes areas
        for ( It el_it = first ; el_it != last ; ++el_it )
        {
            indexable_type const& indexable = rtree::element_indexable(*el_it, translator);

            // calculate enlarged boxes and areas
            box_type enlarged_box1(box1);
            box_type enlarged_box2(box2);
            index::detail::expand(enlarged_box1, indexable, strategy);
            index::detail::expand(enlarged_box2, indexable, strategy);
            content_type enlarged_content1 = index::detail::content(enlarged_box1);
            content_type enlarged_content2 = index::detail::content(enlarged_box2);

            content_type content_incrase1 = (enlarged_content1 - content1);
            content_type content_incrase2 = (enlarged_content2 - content2);

            content_type content_incrase_diff = content_incrase1 < content_incrase2 ?
                content_incrase2 - content_incrase1 : content_incrase1 - content_incrase2;

            if ( greatest_content_incrase_diff < content_incrase_diff )
            {
                greatest_content_incrase_diff = content_incrase_diff;
                out_it = el_it;
                out_content_increase1 = content_incrase1;
                out_content_increase2 = content_incrase2;
            }
        }

        return out_it;
    }
};

}} // namespace detail::rtree

}}} // namespace boost::geometry::index

#endif // BOOST_GEOMETRY_INDEX_DETAIL_RTREE_QUADRATIC_REDISTRIBUTE_ELEMENTS_HPP
