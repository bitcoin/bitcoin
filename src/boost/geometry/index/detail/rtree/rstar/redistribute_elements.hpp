// Boost.Geometry Index
//
// R-tree R*-tree split algorithm implementation
//
// Copyright (c) 2011-2017 Adam Wulkiewicz, Lodz, Poland.
//
// This file was modified by Oracle on 2019-2020.
// Modifications copyright (c) 2019-2020 Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle
//
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_INDEX_DETAIL_RTREE_RSTAR_REDISTRIBUTE_ELEMENTS_HPP
#define BOOST_GEOMETRY_INDEX_DETAIL_RTREE_RSTAR_REDISTRIBUTE_ELEMENTS_HPP

#include <boost/core/ignore_unused.hpp>

#include <boost/geometry/core/static_assert.hpp>

#include <boost/geometry/index/detail/algorithms/intersection_content.hpp>
#include <boost/geometry/index/detail/algorithms/margin.hpp>
#include <boost/geometry/index/detail/algorithms/nth_element.hpp>
#include <boost/geometry/index/detail/algorithms/union_content.hpp>

#include <boost/geometry/index/detail/bounded_view.hpp>

#include <boost/geometry/index/detail/rtree/node/node.hpp>
#include <boost/geometry/index/detail/rtree/visitors/insert.hpp>
#include <boost/geometry/index/detail/rtree/visitors/is_leaf.hpp>

namespace boost { namespace geometry { namespace index {

namespace detail { namespace rtree {

namespace rstar {

template <typename Element, typename Parameters, typename Translator, typename Tag, size_t Corner, size_t AxisIndex>
class element_axis_corner_less
{
    typedef typename rtree::element_indexable_type<Element, Translator>::type indexable_type;
    typedef typename geometry::point_type<indexable_type>::type point_type;
    typedef geometry::model::box<point_type> bounds_type;
    typedef typename index::detail::strategy_type<Parameters>::type strategy_type;
    typedef index::detail::bounded_view
        <
            indexable_type, bounds_type, strategy_type
        > bounded_view_type;

public:
    element_axis_corner_less(Translator const& tr, strategy_type const& strategy)
        : m_tr(tr), m_strategy(strategy)
    {}

    bool operator()(Element const& e1, Element const& e2) const
    {
        bounded_view_type bounded_ind1(rtree::element_indexable(e1, m_tr), m_strategy);
        bounded_view_type bounded_ind2(rtree::element_indexable(e2, m_tr), m_strategy);

        return geometry::get<Corner, AxisIndex>(bounded_ind1)
            < geometry::get<Corner, AxisIndex>(bounded_ind2);
    }

private:
    Translator const& m_tr;
    strategy_type const& m_strategy;
};

template <typename Element, typename Parameters, typename Translator, size_t Corner, size_t AxisIndex>
class element_axis_corner_less<Element, Parameters, Translator, box_tag, Corner, AxisIndex>
{
    typedef typename index::detail::strategy_type<Parameters>::type strategy_type;

public:
    element_axis_corner_less(Translator const& tr, strategy_type const&)
        : m_tr(tr)
    {}

    bool operator()(Element const& e1, Element const& e2) const
    {
        return geometry::get<Corner, AxisIndex>(rtree::element_indexable(e1, m_tr))
            < geometry::get<Corner, AxisIndex>(rtree::element_indexable(e2, m_tr));
    }

private:
    Translator const& m_tr;
};

template <typename Element, typename Parameters, typename Translator, size_t Corner, size_t AxisIndex>
class element_axis_corner_less<Element, Parameters, Translator, point_tag, Corner, AxisIndex>
{
    typedef typename index::detail::strategy_type<Parameters>::type strategy_type;

public:
    element_axis_corner_less(Translator const& tr, strategy_type const& )
        : m_tr(tr)
    {}

    bool operator()(Element const& e1, Element const& e2) const
    {
        return geometry::get<AxisIndex>(rtree::element_indexable(e1, m_tr))
            < geometry::get<AxisIndex>(rtree::element_indexable(e2, m_tr));
    }

private:
    Translator const& m_tr;
};

template <typename Box, size_t Corner, size_t AxisIndex>
struct choose_split_axis_and_index_for_corner
{
    typedef typename index::detail::default_margin_result<Box>::type margin_type;
    typedef typename index::detail::default_content_result<Box>::type content_type;

    template <typename Elements, typename Parameters, typename Translator>
    static inline void apply(Elements const& elements,
                             size_t & choosen_index,
                             margin_type & sum_of_margins,
                             content_type & smallest_overlap,
                             content_type & smallest_content,
                             Parameters const& parameters,
                             Translator const& translator)
    {
        typedef typename Elements::value_type element_type;
        typedef typename rtree::element_indexable_type<element_type, Translator>::type indexable_type;
        typedef typename tag<indexable_type>::type indexable_tag;

        BOOST_GEOMETRY_INDEX_ASSERT(elements.size() == parameters.get_max_elements() + 1, "wrong number of elements");

        typename index::detail::strategy_type<Parameters>::type const&
            strategy = index::detail::get_strategy(parameters);

        // copy elements
        Elements elements_copy(elements);                                                                       // MAY THROW, STRONG (alloc, copy)
        
        size_t const index_first = parameters.get_min_elements();
        size_t const index_last = parameters.get_max_elements() - parameters.get_min_elements() + 2;

        // sort elements
        element_axis_corner_less
            <
                element_type, Parameters, Translator, indexable_tag, Corner, AxisIndex
            > elements_less(translator, strategy);
        std::sort(elements_copy.begin(), elements_copy.end(), elements_less);                                   // MAY THROW, BASIC (copy)
//        {
//            typename Elements::iterator f = elements_copy.begin() + index_first;
//            typename Elements::iterator l = elements_copy.begin() + index_last;
//            // NOTE: for stdlibc++ shipped with gcc 4.8.2 std::nth_element is replaced with std::sort anyway
//            index::detail::nth_element(elements_copy.begin(), f, elements_copy.end(), elements_less);                // MAY THROW, BASIC (copy)
//            index::detail::nth_element(f, l, elements_copy.end(), elements_less);                                    // MAY THROW, BASIC (copy)
//            std::sort(f, l, elements_less);                                                                   // MAY THROW, BASIC (copy)
//        }

        // init outputs
        choosen_index = index_first;
        sum_of_margins = 0;
        smallest_overlap = (std::numeric_limits<content_type>::max)();
        smallest_content = (std::numeric_limits<content_type>::max)();

        // calculate sum of margins for all distributions
        for ( size_t i = index_first ; i < index_last ; ++i )
        {
            // TODO - awulkiew: may be optimized - box of group 1 may be initialized with
            // box of min_elems number of elements and expanded for each iteration by another element

            Box box1 = rtree::elements_box<Box>(elements_copy.begin(), elements_copy.begin() + i,
                                                translator, strategy);
            Box box2 = rtree::elements_box<Box>(elements_copy.begin() + i, elements_copy.end(),
                                                translator, strategy);
            
            sum_of_margins += index::detail::comparable_margin(box1) + index::detail::comparable_margin(box2);

            content_type ovl = index::detail::intersection_content(box1, box2, strategy);
            content_type con = index::detail::content(box1) + index::detail::content(box2);

            // TODO - shouldn't here be < instead of <= ?
            if ( ovl < smallest_overlap || (ovl == smallest_overlap && con <= smallest_content) )
            {
                choosen_index = i;
                smallest_overlap = ovl;
                smallest_content = con;
            }
        }

        ::boost::ignore_unused(parameters);
    }
};

//template <typename Box, size_t AxisIndex, typename ElementIndexableTag>
//struct choose_split_axis_and_index_for_axis
//{
//    BOOST_GEOMETRY_STATIC_ASSERT_FALSE("Not implemented for this Tag type.", ElementIndexableTag);
//};

template <typename Box, size_t AxisIndex, typename ElementIndexableTag>
struct choose_split_axis_and_index_for_axis
{
    typedef typename index::detail::default_margin_result<Box>::type margin_type;
    typedef typename index::detail::default_content_result<Box>::type content_type;

    template <typename Elements, typename Parameters, typename Translator>
    static inline void apply(Elements const& elements,
                             size_t & choosen_corner,
                             size_t & choosen_index,
                             margin_type & sum_of_margins,
                             content_type & smallest_overlap,
                             content_type & smallest_content,
                             Parameters const& parameters,
                             Translator const& translator)
    {
        size_t index1 = 0;
        margin_type som1 = 0;
        content_type ovl1 = (std::numeric_limits<content_type>::max)();
        content_type con1 = (std::numeric_limits<content_type>::max)();

        choose_split_axis_and_index_for_corner<Box, min_corner, AxisIndex>
            ::apply(elements, index1,
                    som1, ovl1, con1,
                    parameters, translator);                                                                // MAY THROW, STRONG

        size_t index2 = 0;
        margin_type som2 = 0;
        content_type ovl2 = (std::numeric_limits<content_type>::max)();
        content_type con2 = (std::numeric_limits<content_type>::max)();

        choose_split_axis_and_index_for_corner<Box, max_corner, AxisIndex>
            ::apply(elements, index2,
                    som2, ovl2, con2,
                    parameters, translator);                                                                // MAY THROW, STRONG

        sum_of_margins = som1 + som2;

        if ( ovl1 < ovl2 || (ovl1 == ovl2 && con1 <= con2) )
        {
            choosen_corner = min_corner;
            choosen_index = index1;
            smallest_overlap = ovl1;
            smallest_content = con1;
        }
        else
        {
            choosen_corner = max_corner;
            choosen_index = index2;
            smallest_overlap = ovl2;
            smallest_content = con2;
        }
    }
};

template <typename Box, size_t AxisIndex>
struct choose_split_axis_and_index_for_axis<Box, AxisIndex, point_tag>
{
    typedef typename index::detail::default_margin_result<Box>::type margin_type;
    typedef typename index::detail::default_content_result<Box>::type content_type;

    template <typename Elements, typename Parameters, typename Translator>
    static inline void apply(Elements const& elements,
                             size_t & choosen_corner,
                             size_t & choosen_index,
                             margin_type & sum_of_margins,
                             content_type & smallest_overlap,
                             content_type & smallest_content,
                             Parameters const& parameters,
                             Translator const& translator)
    {
        choose_split_axis_and_index_for_corner<Box, min_corner, AxisIndex>
            ::apply(elements, choosen_index,
                    sum_of_margins, smallest_overlap, smallest_content,
                    parameters, translator);                                                                // MAY THROW, STRONG

        choosen_corner = min_corner;
    }
};

template <typename Box, size_t Dimension>
struct choose_split_axis_and_index
{
    BOOST_STATIC_ASSERT(0 < Dimension);

    typedef typename index::detail::default_margin_result<Box>::type margin_type;
    typedef typename index::detail::default_content_result<Box>::type content_type;

    template <typename Elements, typename Parameters, typename Translator>
    static inline void apply(Elements const& elements,
                             size_t & choosen_axis,
                             size_t & choosen_corner,
                             size_t & choosen_index,
                             margin_type & smallest_sum_of_margins,
                             content_type & smallest_overlap,
                             content_type & smallest_content,
                             Parameters const& parameters,
                             Translator const& translator)
    {
        typedef typename rtree::element_indexable_type<typename Elements::value_type, Translator>::type element_indexable_type;

        choose_split_axis_and_index<Box, Dimension - 1>
            ::apply(elements, choosen_axis, choosen_corner, choosen_index,
                    smallest_sum_of_margins, smallest_overlap, smallest_content,
                    parameters, translator);                                                                // MAY THROW, STRONG

        margin_type sum_of_margins = 0;

        size_t corner = min_corner;
        size_t index = 0;

        content_type overlap_val = (std::numeric_limits<content_type>::max)();
        content_type content_val = (std::numeric_limits<content_type>::max)();

        choose_split_axis_and_index_for_axis<
            Box,
            Dimension - 1,
            typename tag<element_indexable_type>::type
        >::apply(elements, corner, index, sum_of_margins, overlap_val, content_val, parameters, translator); // MAY THROW, STRONG

        if ( sum_of_margins < smallest_sum_of_margins )
        {
            choosen_axis = Dimension - 1;
            choosen_corner = corner;
            choosen_index = index;
            smallest_sum_of_margins = sum_of_margins;
            smallest_overlap = overlap_val;
            smallest_content = content_val;
        }
    }
};

template <typename Box>
struct choose_split_axis_and_index<Box, 1>
{
    typedef typename index::detail::default_margin_result<Box>::type margin_type;
    typedef typename index::detail::default_content_result<Box>::type content_type;

    template <typename Elements, typename Parameters, typename Translator>
    static inline void apply(Elements const& elements,
                             size_t & choosen_axis,
                             size_t & choosen_corner,
                             size_t & choosen_index,
                             margin_type & smallest_sum_of_margins,
                             content_type & smallest_overlap,
                             content_type & smallest_content,
                             Parameters const& parameters,
                             Translator const& translator)
    {
        typedef typename rtree::element_indexable_type<typename Elements::value_type, Translator>::type element_indexable_type;

        choosen_axis = 0;

        choose_split_axis_and_index_for_axis<
            Box,
            0,
            typename tag<element_indexable_type>::type
        >::apply(elements, choosen_corner, choosen_index, smallest_sum_of_margins, smallest_overlap, smallest_content, parameters, translator); // MAY THROW
    }
};

template <size_t Corner, size_t Dimension, size_t I = 0>
struct nth_element
{
    BOOST_STATIC_ASSERT(0 < Dimension);
    BOOST_STATIC_ASSERT(I < Dimension);

    template <typename Elements, typename Parameters, typename Translator>
    static inline void apply(Elements & elements, Parameters const& parameters,
                             const size_t axis, const size_t index, Translator const& tr)
    {
        //BOOST_GEOMETRY_INDEX_ASSERT(axis < Dimension, "unexpected axis value");

        if ( axis != I )
        {
            nth_element<Corner, Dimension, I + 1>::apply(elements, parameters, axis, index, tr);                     // MAY THROW, BASIC (copy)
        }
        else
        {
            typedef typename Elements::value_type element_type;
            typedef typename rtree::element_indexable_type<element_type, Translator>::type indexable_type;
            typedef typename tag<indexable_type>::type indexable_tag;

            typename index::detail::strategy_type<Parameters>::type
                strategy = index::detail::get_strategy(parameters);

            element_axis_corner_less
                <
                    element_type, Parameters, Translator, indexable_tag, Corner, I
                > less(tr, strategy);
            index::detail::nth_element(elements.begin(), elements.begin() + index, elements.end(), less);            // MAY THROW, BASIC (copy)
        }
    }
};

template <size_t Corner, size_t Dimension>
struct nth_element<Corner, Dimension, Dimension>
{
    template <typename Elements, typename Parameters, typename Translator>
    static inline void apply(Elements & /*elements*/, Parameters const& /*parameters*/,
                             const size_t /*axis*/, const size_t /*index*/, Translator const& /*tr*/)
    {}
};

} // namespace rstar

template <typename MembersHolder>
struct redistribute_elements<MembersHolder, rstar_tag>
{
    typedef typename MembersHolder::box_type box_type;
    typedef typename MembersHolder::parameters_type parameters_type;
    typedef typename MembersHolder::translator_type translator_type;
    typedef typename MembersHolder::allocators_type allocators_type;

    typedef typename MembersHolder::node node;
    typedef typename MembersHolder::internal_node internal_node;
    typedef typename MembersHolder::leaf leaf;

    static const size_t dimension = geometry::dimension<box_type>::value;

    typedef typename index::detail::default_margin_result<box_type>::type margin_type;
    typedef typename index::detail::default_content_result<box_type>::type content_type;

    template <typename Node>
    static inline void apply(
        Node & n,
        Node & second_node,
        box_type & box1,
        box_type & box2,
        parameters_type const& parameters,
        translator_type const& translator,
        allocators_type & allocators)
    {
        typedef typename rtree::elements_type<Node>::type elements_type;
        typedef typename elements_type::value_type element_type;
        
        elements_type & elements1 = rtree::elements(n);
        elements_type & elements2 = rtree::elements(second_node);

        // copy original elements - use in-memory storage (std::allocator)
        // TODO: move if noexcept
        typedef typename rtree::container_from_elements_type<elements_type, element_type>::type
            container_type;
        container_type elements_copy(elements1.begin(), elements1.end());                               // MAY THROW, STRONG
        container_type elements_backup(elements1.begin(), elements1.end());                             // MAY THROW, STRONG

        size_t split_axis = 0;
        size_t split_corner = 0;
        size_t split_index = parameters.get_min_elements();
        margin_type smallest_sum_of_margins = (std::numeric_limits<margin_type>::max)();
        content_type smallest_overlap = (std::numeric_limits<content_type>::max)();
        content_type smallest_content = (std::numeric_limits<content_type>::max)();

        // NOTE: this function internally copies passed elements
        //       why not pass mutable elements and use the same container for all axes/corners
        //       and again, the same below calling partial_sort/nth_element
        //       It would be even possible to not re-sort/find nth_element if the axis/corner
        //       was found for the last sorting - last combination of axis/corner
        rstar::choose_split_axis_and_index<box_type, dimension>
            ::apply(elements_copy,
                    split_axis, split_corner, split_index,
                    smallest_sum_of_margins, smallest_overlap, smallest_content,
                    parameters, translator);                                                            // MAY THROW, STRONG

        // TODO: awulkiew - get rid of following static_casts?
        BOOST_GEOMETRY_INDEX_ASSERT(split_axis < dimension, "unexpected value");
        BOOST_GEOMETRY_INDEX_ASSERT(split_corner == static_cast<size_t>(min_corner) || split_corner == static_cast<size_t>(max_corner), "unexpected value");
        BOOST_GEOMETRY_INDEX_ASSERT(parameters.get_min_elements() <= split_index && split_index <= parameters.get_max_elements() - parameters.get_min_elements() + 1, "unexpected value");

        // TODO: consider using nth_element
        if ( split_corner == static_cast<size_t>(min_corner) )
        {
            rstar::nth_element<min_corner, dimension>
                ::apply(elements_copy, parameters, split_axis, split_index, translator);                // MAY THROW, BASIC (copy)
        }
        else
        {
            rstar::nth_element<max_corner, dimension>
                ::apply(elements_copy, parameters, split_axis, split_index, translator);                // MAY THROW, BASIC (copy)
        }

        BOOST_TRY
        {
            typename index::detail::strategy_type<parameters_type>::type const&
                strategy = index::detail::get_strategy(parameters);

            // copy elements to nodes
            elements1.assign(elements_copy.begin(), elements_copy.begin() + split_index);               // MAY THROW, BASIC
            elements2.assign(elements_copy.begin() + split_index, elements_copy.end());                 // MAY THROW, BASIC

            // calculate boxes
            box1 = rtree::elements_box<box_type>(elements1.begin(), elements1.end(),
                                                 translator, strategy);
            box2 = rtree::elements_box<box_type>(elements2.begin(), elements2.end(),
                                                 translator, strategy);
        }
        BOOST_CATCH(...)
        {
            //elements_copy.clear();
            elements1.clear();
            elements2.clear();

            rtree::destroy_elements<MembersHolder>::apply(elements_backup, allocators);
            //elements_backup.clear();

            BOOST_RETHROW                                                                                 // RETHROW, BASIC
        }
        BOOST_CATCH_END
    }
};

}} // namespace detail::rtree

}}} // namespace boost::geometry::index

#endif // BOOST_GEOMETRY_INDEX_DETAIL_RTREE_RSTAR_REDISTRIBUTE_ELEMENTS_HPP
