// Boost.Geometry Index
//
// R-tree linear split algorithm implementation
//
// Copyright (c) 2008 Federico J. Fernandez.
// Copyright (c) 2011-2014 Adam Wulkiewicz, Lodz, Poland.
//
// This file was modified by Oracle on 2019-2020.
// Modifications copyright (c) 2019-2020 Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle
//
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_INDEX_DETAIL_RTREE_LINEAR_REDISTRIBUTE_ELEMENTS_HPP
#define BOOST_GEOMETRY_INDEX_DETAIL_RTREE_LINEAR_REDISTRIBUTE_ELEMENTS_HPP

#include <type_traits>

#include <boost/core/ignore_unused.hpp>

#include <boost/geometry/core/static_assert.hpp>

#include <boost/geometry/index/detail/algorithms/bounds.hpp>
#include <boost/geometry/index/detail/algorithms/content.hpp>
#include <boost/geometry/index/detail/bounded_view.hpp>

#include <boost/geometry/index/detail/rtree/node/node.hpp>
#include <boost/geometry/index/detail/rtree/visitors/insert.hpp>
#include <boost/geometry/index/detail/rtree/visitors/is_leaf.hpp>

namespace boost { namespace geometry { namespace index {

namespace detail { namespace rtree {

namespace linear {

template <typename R, typename T>
inline R difference_dispatch(T const& from, T const& to, std::false_type /*is_unsigned*/)
{
    return to - from;
}

template <typename R, typename T>
inline R difference_dispatch(T const& from, T const& to, std::true_type /*is_unsigned*/)
{
    return from <= to ? R(to - from) : -R(from - to);
}

template <typename R, typename T>
inline R difference(T const& from, T const& to)
{
    BOOST_GEOMETRY_STATIC_ASSERT((! std::is_unsigned<R>::value),
        "Result can not be an unsigned type.",
        R);

    return difference_dispatch<R>(from, to, std::is_unsigned<T>());
}

// TODO: awulkiew
// In general, all aerial Indexables in the tree with box-like nodes will be analyzed as boxes
// because they must fit into larger box. Therefore the algorithm could be the same for Bounds type.
// E.g. if Bounds type is sphere, Indexables probably should be analyzed as spheres.
// 1. View could be provided to 'see' all Indexables as Bounds type.
//    Not ok in the case of big types like Ring, however it's possible that Rings won't be supported,
//    only simple types. Even then if we consider storing Box inside the Sphere we must calculate
//    the bounding sphere 2x for each box because there are 2 loops. For each calculation this means
//    4-2d or 8-3d expansions or -, / and sqrt().
// 2. Additional container could be used and reused if the Indexable type is other than the Bounds type.

// IMPORTANT!
// Still probably the best way would be providing specialized algorithms for each Indexable-Bounds pair!
// Probably on pick_seeds algorithm level - For Bounds=Sphere seeds would be choosen differently

// TODO: awulkiew
// there are loops inside find_greatest_normalized_separation::apply()
// iteration is done for each DimensionIndex.
// Separations and seeds for all DimensionIndex(es) could be calculated at once, stored, then the greatest would be choosen.

// The following struct/method was adapted for the preliminary version of the R-tree. Then it was called:
// void find_normalized_separations(std::vector<Box> const& boxes, T& separation, unsigned int& first, unsigned int& second) const

template <typename Elements, typename Parameters, typename Translator, typename Tag, size_t DimensionIndex>
struct find_greatest_normalized_separation
{
    typedef typename Elements::value_type element_type;
    typedef typename rtree::element_indexable_type<element_type, Translator>::type indexable_type;
    typedef typename coordinate_type<indexable_type>::type coordinate_type;

    typedef std::conditional_t
        <
            std::is_integral<coordinate_type>::value,
            double,
            coordinate_type
        > separation_type;

    typedef typename geometry::point_type<indexable_type>::type point_type;
    typedef geometry::model::box<point_type> bounds_type;
    typedef index::detail::bounded_view
        <
            indexable_type, bounds_type,
            typename index::detail::strategy_type<Parameters>::type
        > bounded_view_type;

    static inline void apply(Elements const& elements,
                             Parameters const& parameters,
                             Translator const& translator,
                             separation_type & separation,
                             size_t & seed1,
                             size_t & seed2)
    {
        const size_t elements_count = parameters.get_max_elements() + 1;
        BOOST_GEOMETRY_INDEX_ASSERT(elements.size() == elements_count, "unexpected number of elements");
        BOOST_GEOMETRY_INDEX_ASSERT(2 <= elements_count, "unexpected number of elements");

        typename index::detail::strategy_type<Parameters>::type const&
            strategy = index::detail::get_strategy(parameters);

        // find the lowest low, highest high
        bounded_view_type bounded_indexable_0(rtree::element_indexable(elements[0], translator),
                                              strategy);
        coordinate_type lowest_low = geometry::get<min_corner, DimensionIndex>(bounded_indexable_0);
        coordinate_type highest_high = geometry::get<max_corner, DimensionIndex>(bounded_indexable_0);

        // and the lowest high
        coordinate_type lowest_high = highest_high;
        size_t lowest_high_index = 0;
        for ( size_t i = 1 ; i < elements_count ; ++i )
        {
            bounded_view_type bounded_indexable(rtree::element_indexable(elements[i], translator),
                                                strategy);
            coordinate_type min_coord = geometry::get<min_corner, DimensionIndex>(bounded_indexable);
            coordinate_type max_coord = geometry::get<max_corner, DimensionIndex>(bounded_indexable);

            if ( max_coord < lowest_high )
            {
                lowest_high = max_coord;
                lowest_high_index = i;
            }

            if ( min_coord < lowest_low )
                lowest_low = min_coord;

            if ( highest_high < max_coord )
                highest_high = max_coord;
        }

        // find the highest low
        size_t highest_low_index = lowest_high_index == 0 ? 1 : 0;
        bounded_view_type bounded_indexable_hl(rtree::element_indexable(elements[highest_low_index], translator),
                                               strategy);
        coordinate_type highest_low = geometry::get<min_corner, DimensionIndex>(bounded_indexable_hl);
        for ( size_t i = highest_low_index ; i < elements_count ; ++i )
        {
            bounded_view_type bounded_indexable(rtree::element_indexable(elements[i], translator),
                                                strategy);
            coordinate_type min_coord = geometry::get<min_corner, DimensionIndex>(bounded_indexable);
            if ( highest_low < min_coord &&
                 i != lowest_high_index )
            {
                highest_low = min_coord;
                highest_low_index = i;
            }
        }

        coordinate_type const width = highest_high - lowest_low;
        
        // highest_low - lowest_high
        separation = difference<separation_type>(lowest_high, highest_low);
        // BOOST_GEOMETRY_INDEX_ASSERT(0 <= width);
        if ( std::numeric_limits<coordinate_type>::epsilon() < width )
            separation /= width;

        seed1 = highest_low_index;
        seed2 = lowest_high_index;

        ::boost::ignore_unused(parameters);
    }
};

// Version for points doesn't calculate normalized separation since it would always be equal to 1
// It returns two seeds most distant to each other, separation is equal to distance
template <typename Elements, typename Parameters, typename Translator, size_t DimensionIndex>
struct find_greatest_normalized_separation<Elements, Parameters, Translator, point_tag, DimensionIndex>
{
    typedef typename Elements::value_type element_type;
    typedef typename rtree::element_indexable_type<element_type, Translator>::type indexable_type;
    typedef typename coordinate_type<indexable_type>::type coordinate_type;

    typedef coordinate_type separation_type;

    static inline void apply(Elements const& elements,
                             Parameters const& parameters,
                             Translator const& translator,
                             separation_type & separation,
                             size_t & seed1,
                             size_t & seed2)
    {
        const size_t elements_count = parameters.get_max_elements() + 1;
        BOOST_GEOMETRY_INDEX_ASSERT(elements.size() == elements_count, "unexpected number of elements");
        BOOST_GEOMETRY_INDEX_ASSERT(2 <= elements_count, "unexpected number of elements");

        // find the lowest low, highest high
        coordinate_type lowest = geometry::get<DimensionIndex>(rtree::element_indexable(elements[0], translator));
        coordinate_type highest = geometry::get<DimensionIndex>(rtree::element_indexable(elements[0], translator));
        size_t lowest_index = 0;
        size_t highest_index = 0;
        for ( size_t i = 1 ; i < elements_count ; ++i )
        {
            coordinate_type coord = geometry::get<DimensionIndex>(rtree::element_indexable(elements[i], translator));

            if ( coord < lowest )
            {
                lowest = coord;
                lowest_index = i;
            }

            if ( highest < coord )
            {
                highest = coord;
                highest_index = i;
            }
        }

        separation = highest - lowest;
        seed1 = lowest_index;
        seed2 = highest_index;

        if ( lowest_index == highest_index )
            seed2 = (lowest_index + 1) % elements_count; // % is just in case since if this is true lowest_index is 0

        ::boost::ignore_unused(parameters);
    }
};

template <typename Elements, typename Parameters, typename Translator, size_t Dimension>
struct pick_seeds_impl
{
    BOOST_STATIC_ASSERT(0 < Dimension);

    typedef typename Elements::value_type element_type;
    typedef typename rtree::element_indexable_type<element_type, Translator>::type indexable_type;

    typedef find_greatest_normalized_separation<
        Elements, Parameters, Translator,
        typename tag<indexable_type>::type, Dimension - 1
    > find_norm_sep;

    typedef typename find_norm_sep::separation_type separation_type;

    static inline void apply(Elements const& elements,
                             Parameters const& parameters,
                             Translator const& tr,
                             separation_type & separation,
                             size_t & seed1,
                             size_t & seed2)
    {
        pick_seeds_impl<Elements, Parameters, Translator, Dimension - 1>::apply(elements, parameters, tr, separation, seed1, seed2);

        separation_type current_separation;
        size_t s1, s2;
        find_norm_sep::apply(elements, parameters, tr, current_separation, s1, s2);

        // in the old implementation different operator was used: <= (y axis prefered)
        if ( separation < current_separation )
        {
            separation = current_separation;
            seed1 = s1;
            seed2 = s2;
        }
    }
};

template <typename Elements, typename Parameters, typename Translator>
struct pick_seeds_impl<Elements, Parameters, Translator, 1>
{
    typedef typename Elements::value_type element_type;
    typedef typename rtree::element_indexable_type<element_type, Translator>::type indexable_type;
    typedef typename coordinate_type<indexable_type>::type coordinate_type;

    typedef find_greatest_normalized_separation<
        Elements, Parameters, Translator,
        typename tag<indexable_type>::type, 0
    > find_norm_sep;

    typedef typename find_norm_sep::separation_type separation_type;

    static inline void apply(Elements const& elements,
                             Parameters const& parameters,
                             Translator const& tr,
                             separation_type & separation,
                             size_t & seed1,
                             size_t & seed2)
    {
        find_norm_sep::apply(elements, parameters, tr, separation, seed1, seed2);
    }
};

// from void linear_pick_seeds(node_pointer const& n, unsigned int &seed1, unsigned int &seed2) const

template <typename Elements, typename Parameters, typename Translator>
inline void pick_seeds(Elements const& elements,
                       Parameters const& parameters,
                       Translator const& tr,
                       size_t & seed1,
                       size_t & seed2)
{
    typedef typename Elements::value_type element_type;
    typedef typename rtree::element_indexable_type<element_type, Translator>::type indexable_type;

    typedef pick_seeds_impl
        <
            Elements, Parameters, Translator,
            geometry::dimension<indexable_type>::value
        > impl;
    typedef typename impl::separation_type separation_type;

    separation_type separation = 0;
    impl::apply(elements, parameters, tr, separation, seed1, seed2);
}

} // namespace linear

// from void split_node(node_pointer const& n, node_pointer& n1, node_pointer& n2) const

template <typename MembersHolder>
struct redistribute_elements<MembersHolder, linear_tag>
{
    typedef typename MembersHolder::box_type box_type;
    typedef typename MembersHolder::parameters_type parameters_type;
    typedef typename MembersHolder::translator_type translator_type;
    typedef typename MembersHolder::allocators_type allocators_type;

    typedef typename MembersHolder::node node;
    typedef typename MembersHolder::internal_node internal_node;
    typedef typename MembersHolder::leaf leaf;

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
        typedef typename index::detail::default_content_result<box_type>::type content_type;

        typename index::detail::strategy_type<parameters_type>::type const&
            strategy = index::detail::get_strategy(parameters);

        elements_type & elements1 = rtree::elements(n);
        elements_type & elements2 = rtree::elements(second_node);
        const size_t elements1_count = parameters.get_max_elements() + 1;

        BOOST_GEOMETRY_INDEX_ASSERT(elements1.size() == elements1_count, "unexpected number of elements");

        // copy original elements - use in-memory storage (std::allocator)
        // TODO: move if noexcept
        typedef typename rtree::container_from_elements_type<elements_type, element_type>::type
            container_type;
        container_type elements_copy(elements1.begin(), elements1.end());                                   // MAY THROW, STRONG (alloc, copy)

        // calculate initial seeds
        size_t seed1 = 0;
        size_t seed2 = 0;
        linear::pick_seeds(elements_copy, parameters, translator, seed1, seed2);

        // prepare nodes' elements containers
        elements1.clear();
        BOOST_GEOMETRY_INDEX_ASSERT(elements2.empty(), "unexpected container state");

        BOOST_TRY
        {
            // add seeds
            elements1.push_back(elements_copy[seed1]);                                                      // MAY THROW, STRONG (copy)
            elements2.push_back(elements_copy[seed2]);                                                      // MAY THROW, STRONG (alloc, copy)

            // calculate boxes
            detail::bounds(rtree::element_indexable(elements_copy[seed1], translator),
                           box1, strategy);
            detail::bounds(rtree::element_indexable(elements_copy[seed2], translator),
                           box2, strategy);

            // initialize areas
            content_type content1 = index::detail::content(box1);
            content_type content2 = index::detail::content(box2);

            BOOST_GEOMETRY_INDEX_ASSERT(2 <= elements1_count, "unexpected elements number");
            size_t remaining = elements1_count - 2;

            // redistribute the rest of the elements
            for ( size_t i = 0 ; i < elements1_count ; ++i )
            {
                if (i != seed1 && i != seed2)
                {
                    element_type const& elem = elements_copy[i];
                    indexable_type const& indexable = rtree::element_indexable(elem, translator);

                    // if there is small number of elements left and the number of elements in node is lesser than min_elems
                    // just insert them to this node
                    if ( elements1.size() + remaining <= parameters.get_min_elements() )
                    {
                        elements1.push_back(elem);                                                          // MAY THROW, STRONG (copy)
                        index::detail::expand(box1, indexable, strategy);
                        content1 = index::detail::content(box1);
                    }
                    else if ( elements2.size() + remaining <= parameters.get_min_elements() )
                    {
                        elements2.push_back(elem);                                                          // MAY THROW, STRONG (alloc, copy)
                        index::detail::expand(box2, indexable, strategy);
                        content2 = index::detail::content(box2);
                    }
                    // choose better node and insert element
                    else
                    {
                        // calculate enlarged boxes and areas
                        box_type enlarged_box1(box1);
                        box_type enlarged_box2(box2);
                        index::detail::expand(enlarged_box1, indexable, strategy);
                        index::detail::expand(enlarged_box2, indexable, strategy);
                        content_type enlarged_content1 = index::detail::content(enlarged_box1);
                        content_type enlarged_content2 = index::detail::content(enlarged_box2);

                        content_type content_increase1 = enlarged_content1 - content1;
                        content_type content_increase2 = enlarged_content2 - content2;

                        // choose group which box content have to be enlarged least or has smaller content or has fewer elements
                        if ( content_increase1 < content_increase2 ||
                                ( content_increase1 == content_increase2 &&
                                    ( content1 < content2 ||
                                        ( content1 == content2 && elements1.size() <= elements2.size() ) ) ) )
                        {
                            elements1.push_back(elem);                                                      // MAY THROW, STRONG (copy)
                            box1 = enlarged_box1;
                            content1 = enlarged_content1;
                        }
                        else
                        {
                            elements2.push_back(elem);                                                      // MAY THROW, STRONG (alloc, copy)
                            box2 = enlarged_box2;
                            content2 = enlarged_content2;
                        }
                    }
                
                    BOOST_GEOMETRY_INDEX_ASSERT(0 < remaining, "unexpected value");
                    --remaining;
                }
            }
        }
        BOOST_CATCH(...)
        {
            elements1.clear();
            elements2.clear();

            rtree::destroy_elements<MembersHolder>::apply(elements_copy, allocators);
            //elements_copy.clear();

            BOOST_RETHROW                                                                                     // RETHROW, BASIC
        }
        BOOST_CATCH_END
    }
};

}} // namespace detail::rtree

}}} // namespace boost::geometry::index

#endif // BOOST_GEOMETRY_INDEX_DETAIL_RTREE_LINEAR_REDISTRIBUTE_ELEMENTS_HPP
