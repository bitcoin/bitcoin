// Boost.Geometry Index
//
// R-tree initial packing
//
// Copyright (c) 2011-2017 Adam Wulkiewicz, Lodz, Poland.
// Copyright (c) 2020 Caian Benedicto, Campinas, Brazil.
//
// This file was modified by Oracle on 2019.
// Modifications copyright (c) 2019 Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle
//
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_INDEX_DETAIL_RTREE_PACK_CREATE_HPP
#define BOOST_GEOMETRY_INDEX_DETAIL_RTREE_PACK_CREATE_HPP

#include <boost/core/ignore_unused.hpp>

#include <boost/geometry/algorithms/expand.hpp>
#include <boost/geometry/index/detail/algorithms/bounds.hpp>
#include <boost/geometry/index/detail/algorithms/nth_element.hpp>
#include <boost/geometry/index/detail/rtree/node/subtree_destroyer.hpp>

#include <boost/geometry/algorithms/detail/expand_by_epsilon.hpp>

namespace boost { namespace geometry { namespace index { namespace detail { namespace rtree {

namespace pack_utils {

template <std::size_t Dimension>
struct biggest_edge
{
    BOOST_STATIC_ASSERT(0 < Dimension);
    template <typename Box>
    static inline void apply(Box const& box, typename coordinate_type<Box>::type & length, std::size_t & dim_index)
    {
        biggest_edge<Dimension-1>::apply(box, length, dim_index);
        typename coordinate_type<Box>::type curr
            = geometry::get<max_corner, Dimension-1>(box) - geometry::get<min_corner, Dimension-1>(box);
        if ( length < curr )
        {
            dim_index = Dimension - 1;
            length = curr;
        }
    }
};

template <>
struct biggest_edge<1>
{
    template <typename Box>
    static inline void apply(Box const& box, typename coordinate_type<Box>::type & length, std::size_t & dim_index)
    {
        dim_index = 0;
        length = geometry::get<max_corner, 0>(box) - geometry::get<min_corner, 0>(box);
    }
};

template <std::size_t I>
struct point_entries_comparer
{
    template <typename PointEntry>
    bool operator()(PointEntry const& e1, PointEntry const& e2) const
    {
        return geometry::get<I>(e1.first) < geometry::get<I>(e2.first);
    }
};

template <std::size_t I, std::size_t Dimension>
struct nth_element_and_half_boxes
{
    template <typename EIt, typename Box>
    static inline void apply(EIt first, EIt median, EIt last, Box const& box, Box & left, Box & right, std::size_t dim_index)
    {
        if ( I == dim_index )
        {
            index::detail::nth_element(first, median, last, point_entries_comparer<I>());

            geometry::convert(box, left);
            geometry::convert(box, right);
            typename coordinate_type<Box>::type edge_len
                = geometry::get<max_corner, I>(box) - geometry::get<min_corner, I>(box);
            typename coordinate_type<Box>::type median
                = geometry::get<min_corner, I>(box) + edge_len / 2;
            geometry::set<max_corner, I>(left, median);
            geometry::set<min_corner, I>(right, median);
        }
        else
            nth_element_and_half_boxes<I+1, Dimension>::apply(first, median, last, box, left, right, dim_index);
    }
};

template <std::size_t Dimension>
struct nth_element_and_half_boxes<Dimension, Dimension>
{
    template <typename EIt, typename Box>
    static inline void apply(EIt , EIt , EIt , Box const& , Box & , Box & , std::size_t ) {}
};

} // namespace pack_utils

// STR leafs number are calculated as rcount/max
// and the number of splitting planes for each dimension as (count/max)^(1/dimension)
// <-> for dimension==2 -> sqrt(count/max)
//
// The main flaw of this algorithm is that the resulting tree will have bad structure for:
// 1. non-uniformly distributed elements
//      Statistic check could be performed, e.g. based on variance of lengths of elements edges for each dimension
// 2. elements distributed mainly along one axis
//      Calculate bounding box of all elements and then number of dividing planes for a dimension
//      from the length of BB edge for this dimension (more or less assuming that elements are uniformly-distributed squares)
//
// Another thing is that the last node may have less elements than Max or even Min.
// The number of splitting planes must be chosen more carefully than count/max
//
// This algorithm is something between STR and TGS
// it is more similar to the top-down recursive kd-tree creation algorithm
// using object median split and split axis of greatest BB edge
// BB is only used as a hint (assuming objects are distributed uniformly)
//
// Implemented algorithm guarantees that the number of elements in nodes will be between Min and Max
// and that nodes are packed as tightly as possible
// e.g. for 177 values Max = 5 and Min = 2 it will construct the following tree:
// ROOT                 177
// L1          125               52
// L2  25  25  25  25  25   25  17    10
// L3  5x5 5x5 5x5 5x5 5x5  5x5 3x5+2 2x5

template <typename MembersHolder>
class pack
{
    typedef typename MembersHolder::node node;
    typedef typename MembersHolder::internal_node internal_node;
    typedef typename MembersHolder::leaf leaf;

    typedef typename MembersHolder::node_pointer node_pointer;
    typedef typename MembersHolder::size_type size_type;
    typedef typename MembersHolder::parameters_type parameters_type;
    typedef typename MembersHolder::translator_type translator_type;
    typedef typename MembersHolder::allocators_type allocators_type;

    typedef typename MembersHolder::box_type box_type;
    typedef typename geometry::point_type<box_type>::type point_type;
    typedef typename geometry::coordinate_type<point_type>::type coordinate_type;
    typedef typename detail::default_content_result<box_type>::type content_type;
    typedef typename detail::strategy_type<parameters_type>::type strategy_type;
    static const std::size_t dimension = geometry::dimension<point_type>::value;

    typedef typename rtree::container_from_elements_type<
        typename rtree::elements_type<leaf>::type,
        size_type
    >::type values_counts_container;

    typedef typename rtree::elements_type<internal_node>::type internal_elements;
    typedef typename internal_elements::value_type internal_element;

    typedef rtree::subtree_destroyer<MembersHolder> subtree_destroyer;

public:
    // Arbitrary iterators
    template <typename InIt> inline static
    node_pointer apply(InIt first, InIt last,
                       size_type & values_count,
                       size_type & leafs_level,
                       parameters_type const& parameters,
                       translator_type const& translator,
                       allocators_type & allocators)
    {
        return apply(first, last, values_count, leafs_level, parameters, translator,
                     allocators, boost::container::new_allocator<void>());
    }

    template <typename InIt, typename TmpAlloc> inline static
    node_pointer apply(InIt first, InIt last,
                       size_type & values_count,
                       size_type & leafs_level,
                       parameters_type const& parameters,
                       translator_type const& translator,
                       allocators_type & allocators,
                       TmpAlloc const& temp_allocator)
    {
        typedef typename std::iterator_traits<InIt>::difference_type diff_type;
            
        diff_type diff = std::distance(first, last);
        if ( diff <= 0 )
            return node_pointer(0);

        typedef std::pair<point_type, InIt> entry_type;
        typedef typename boost::container::allocator_traits<TmpAlloc>::
            template rebind_alloc<entry_type> temp_entry_allocator_type;

        temp_entry_allocator_type temp_entry_allocator(temp_allocator);
        boost::container::vector<entry_type, temp_entry_allocator_type> entries(temp_entry_allocator);

        values_count = static_cast<size_type>(diff);
        entries.reserve(values_count);

        auto const& strategy = index::detail::get_strategy(parameters);
        
        expandable_box<box_type, strategy_type> hint_box(strategy);
        for ( ; first != last ; ++first )
        {
            // NOTE: support for iterators not returning true references adapted
            // to Geometry concept and default translator returning true reference
            // An alternative would be to dereference the iterator and translate
            // in one expression each time the indexable was needed.
            typename std::iterator_traits<InIt>::reference in_ref = *first;
            typename translator_type::result_type indexable = translator(in_ref);

            // NOTE: added for consistency with insert()
            // CONSIDER: alternative - ignore invalid indexable or throw an exception
            BOOST_GEOMETRY_INDEX_ASSERT(detail::is_valid(indexable), "Indexable is invalid");

            hint_box.expand(indexable);

            point_type pt;
            geometry::centroid(indexable, pt, strategy);
            entries.push_back(std::make_pair(pt, first));
        }

        subtree_elements_counts subtree_counts = calculate_subtree_elements_counts(values_count, parameters, leafs_level);
        internal_element el = per_level(entries.begin(), entries.end(), hint_box.get(), values_count, subtree_counts,
                                        parameters, translator, allocators);

        return el.second;
    }

private:
    template <typename BoxType, typename Strategy>
    class expandable_box
    {
    public:
        explicit expandable_box(Strategy const& strategy)
            : m_strategy(strategy), m_initialized(false)
        {}

        template <typename Indexable>
        explicit expandable_box(Indexable const& indexable, Strategy const& strategy)
            : m_strategy(strategy), m_initialized(true)
        {
            detail::bounds(indexable, m_box, m_strategy);
        }

        template <typename Indexable>
        void expand(Indexable const& indexable)
        {
            if ( !m_initialized )
            {
                // it's guaranteed that the Box will be initialized
                // only for Points, Boxes and Segments but that's ok
                // since only those Geometries can be stored
                detail::bounds(indexable, m_box, m_strategy);
                m_initialized = true;
            }
            else
            {
                detail::expand(m_box, indexable, m_strategy);
            }
        }

        void expand_by_epsilon()
        {
            geometry::detail::expand_by_epsilon(m_box);
        }

        BoxType const& get() const
        {
            BOOST_GEOMETRY_INDEX_ASSERT(m_initialized, "uninitialized envelope accessed");
            return m_box;
        }

    private:
        BoxType m_box;
        Strategy m_strategy;
        bool m_initialized;
    };

    struct subtree_elements_counts
    {
        subtree_elements_counts(size_type ma, size_type mi) : maxc(ma), minc(mi) {}
        size_type maxc;
        size_type minc;
    };

    template <typename EIt> inline static
    internal_element per_level(EIt first, EIt last,
                               box_type const& hint_box,
                               size_type values_count,
                               subtree_elements_counts const& subtree_counts,
                               parameters_type const& parameters,
                               translator_type const& translator,
                               allocators_type & allocators)
    {
        BOOST_GEOMETRY_INDEX_ASSERT(0 < std::distance(first, last) && static_cast<size_type>(std::distance(first, last)) == values_count,
                                    "unexpected parameters");

        if ( subtree_counts.maxc <= 1 )
        {
            // ROOT or LEAF
            BOOST_GEOMETRY_INDEX_ASSERT(values_count <= parameters.get_max_elements(),
                                        "too big number of elements");
            // if !root check m_parameters.get_min_elements() <= count

            // create new leaf node
            node_pointer n = rtree::create_node<allocators_type, leaf>::apply(allocators);                       // MAY THROW (A)
            subtree_destroyer auto_remover(n, allocators);
            leaf & l = rtree::get<leaf>(*n);

            // reserve space for values
            rtree::elements(l).reserve(values_count);                                                       // MAY THROW (A)

            // calculate values box and copy values
            //   initialize the box explicitly to avoid GCC-4.4 uninitialized variable warnings with O2
            expandable_box<box_type, strategy_type> elements_box(translator(*(first->second)),
                                                                 detail::get_strategy(parameters));
            rtree::elements(l).push_back(*(first->second));                                                 // MAY THROW (A?,C)
            for ( ++first ; first != last ; ++first )
            {
                // NOTE: push_back() must be called at the end in order to support move_iterator.
                //       The iterator is dereferenced 2x (no temporary reference) to support
                //       non-true reference types and move_iterator without boost::forward<>.
                elements_box.expand(translator(*(first->second)));
                rtree::elements(l).push_back(*(first->second));                                             // MAY THROW (A?,C)
            }

#ifdef BOOST_GEOMETRY_INDEX_EXPERIMENTAL_ENLARGE_BY_EPSILON
            // Enlarge bounds of a leaf node.
            // It's because Points and Segments are compared WRT machine epsilon
            // This ensures that leafs bounds correspond to the stored elements
            // NOTE: this is done only if the Indexable is a different kind of Geometry
            //   than the bounds (only Box for now). Spatial predicates are checked
            //   the same way for Geometry of the same kind.
            if ( BOOST_GEOMETRY_CONDITION((
                    ! index::detail::is_bounding_geometry
                        <
                            typename indexable_type<translator_type>::type
                        >::value )) )
            {
                elements_box.expand_by_epsilon();
            }
#endif

            auto_remover.release();
            return internal_element(elements_box.get(), n);
        }

        // calculate next max and min subtree counts
        subtree_elements_counts next_subtree_counts = subtree_counts;
        next_subtree_counts.maxc /= parameters.get_max_elements();
        next_subtree_counts.minc /= parameters.get_max_elements();

        // create new internal node
        node_pointer n = rtree::create_node<allocators_type, internal_node>::apply(allocators);                  // MAY THROW (A)
        subtree_destroyer auto_remover(n, allocators);
        internal_node & in = rtree::get<internal_node>(*n);

        // reserve space for values
        size_type nodes_count = calculate_nodes_count(values_count, subtree_counts);
        rtree::elements(in).reserve(nodes_count);                                                           // MAY THROW (A)
        // calculate values box and copy values
        expandable_box<box_type, strategy_type> elements_box(detail::get_strategy(parameters));
        
        per_level_packets(first, last, hint_box, values_count, subtree_counts, next_subtree_counts,
                          rtree::elements(in), elements_box,
                          parameters, translator, allocators);

        auto_remover.release();
        return internal_element(elements_box.get(), n);
    }

    template <typename EIt, typename ExpandableBox> inline static
    void per_level_packets(EIt first, EIt last,
                           box_type const& hint_box,
                           size_type values_count,
                           subtree_elements_counts const& subtree_counts,
                           subtree_elements_counts const& next_subtree_counts,
                           internal_elements & elements,
                           ExpandableBox & elements_box,
                           parameters_type const& parameters,
                           translator_type const& translator,
                           allocators_type & allocators)
    {
        BOOST_GEOMETRY_INDEX_ASSERT(0 < std::distance(first, last) && static_cast<size_type>(std::distance(first, last)) == values_count,
                                    "unexpected parameters");

        BOOST_GEOMETRY_INDEX_ASSERT(subtree_counts.minc <= values_count,
                                    "too small number of elements");

        // only one packet
        if ( values_count <= subtree_counts.maxc )
        {
            // the end, move to the next level
            internal_element el = per_level(first, last, hint_box, values_count, next_subtree_counts,
                                            parameters, translator, allocators);

            // in case if push_back() do throw here
            // and even if this is not probable (previously reserved memory, nonthrowing pairs copy)
            // this case is also tested by exceptions test.
            subtree_destroyer auto_remover(el.second, allocators);
            // this container should have memory allocated, reserve() called outside
            elements.push_back(el);                                                 // MAY THROW (A?,C) - however in normal conditions shouldn't
            auto_remover.release();

            elements_box.expand(el.first);
            return;
        }
        
        size_type median_count = calculate_median_count(values_count, subtree_counts);
        EIt median = first + median_count;

        coordinate_type greatest_length;
        std::size_t greatest_dim_index = 0;
        pack_utils::biggest_edge<dimension>::apply(hint_box, greatest_length, greatest_dim_index);
        box_type left, right;
        pack_utils::nth_element_and_half_boxes<0, dimension>
            ::apply(first, median, last, hint_box, left, right, greatest_dim_index);
        
        per_level_packets(first, median, left,
                          median_count, subtree_counts, next_subtree_counts,
                          elements, elements_box,
                          parameters, translator, allocators);
        per_level_packets(median, last, right,
                          values_count - median_count, subtree_counts, next_subtree_counts,
                          elements, elements_box,
                          parameters, translator, allocators);
    }

    inline static
    subtree_elements_counts calculate_subtree_elements_counts(size_type elements_count, parameters_type const& parameters, size_type & leafs_level)
    {
        boost::ignore_unused(parameters);

        subtree_elements_counts res(1, 1);
        leafs_level = 0;

        size_type smax = parameters.get_max_elements();
        for ( ; smax < elements_count ; smax *= parameters.get_max_elements(), ++leafs_level )
            res.maxc = smax;

        res.minc = parameters.get_min_elements() * (res.maxc / parameters.get_max_elements());

        return res;
    }

    inline static
    size_type calculate_nodes_count(size_type count,
                                    subtree_elements_counts const& subtree_counts)
    {
        size_type n = count / subtree_counts.maxc;
        size_type r = count % subtree_counts.maxc;

        if ( 0 < r && r < subtree_counts.minc )
        {
            size_type count_minus_min = count - subtree_counts.minc;
            n = count_minus_min / subtree_counts.maxc;
            r = count_minus_min % subtree_counts.maxc;
            ++n;
        }

        if ( 0 < r )
            ++n;

        return n;
    }

    inline static
    size_type calculate_median_count(size_type count,
                                     subtree_elements_counts const& subtree_counts)
    {
        // e.g. for max = 5, min = 2, count = 52, subtree_max = 25, subtree_min = 10

        size_type n = count / subtree_counts.maxc; // e.g. 52 / 25 = 2
        size_type r = count % subtree_counts.maxc; // e.g. 52 % 25 = 2
        size_type median_count = (n / 2) * subtree_counts.maxc; // e.g. 2 / 2 * 25 = 25

        if ( 0 != r ) // e.g. 0 != 2
        {
            if ( subtree_counts.minc <= r ) // e.g. 10 <= 2 == false
            {
                //BOOST_GEOMETRY_INDEX_ASSERT(0 < n, "unexpected value");
                median_count = ((n+1)/2) * subtree_counts.maxc; // if calculated ((2+1)/2) * 25 which would be ok, but not in all cases
            }
            else // r < subtree_counts.second  // e.g. 2 < 10 == true
            {
                size_type count_minus_min = count - subtree_counts.minc; // e.g. 52 - 10 = 42
                n = count_minus_min / subtree_counts.maxc; // e.g. 42 / 25 = 1
                r = count_minus_min % subtree_counts.maxc; // e.g. 42 % 25 = 17
                if ( r == 0 )                               // e.g. false
                {
                    // n can't be equal to 0 because then there wouldn't be any element in the other node
                    //BOOST_GEOMETRY_INDEX_ASSERT(0 < n, "unexpected value");
                    median_count = ((n+1)/2) * subtree_counts.maxc;     // if calculated ((1+1)/2) * 25 which would be ok, but not in all cases
                }
                else
                {
                    if ( n == 0 )                                        // e.g. false
                        median_count = r;                                // if calculated -> 17 which is wrong!
                    else
                        median_count = ((n+2)/2) * subtree_counts.maxc; // e.g. ((1+2)/2) * 25 = 25
                }
            }
        }

        return median_count;
    }
};

}}}}} // namespace boost::geometry::index::detail::rtree

#endif // BOOST_GEOMETRY_INDEX_DETAIL_RTREE_PACK_CREATE_HPP
