// Boost.Geometry Index
//
// R-tree R*-tree insert algorithm implementation
//
// Copyright (c) 2011-2015 Adam Wulkiewicz, Lodz, Poland.
//
// This file was modified by Oracle on 2019-2021.
// Modifications copyright (c) 2019-2021 Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle
//
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_INDEX_DETAIL_RTREE_RSTAR_INSERT_HPP
#define BOOST_GEOMETRY_INDEX_DETAIL_RTREE_RSTAR_INSERT_HPP

#include <type_traits>

#include <boost/core/ignore_unused.hpp>

#include <boost/geometry/index/detail/algorithms/content.hpp>

namespace boost { namespace geometry { namespace index {

namespace detail { namespace rtree { namespace visitors {

namespace rstar {

// Utility to distinguish between default and non-default index strategy
template <typename Point1, typename Point2, typename Strategy>
struct comparable_distance
{
    typedef typename geometry::comparable_distance_result
        <
            Point1, Point2, Strategy
        >::type result_type;

    static inline result_type call(Point1 const& p1, Point2 const& p2, Strategy const& s)
    {
        return geometry::comparable_distance(p1, p2, s);
    }
};

template <typename Point1, typename Point2>
struct comparable_distance<Point1, Point2, default_strategy>
{
    typedef typename geometry::default_comparable_distance_result
        <
            Point1, Point2
        >::type result_type;

    static inline result_type call(Point1 const& p1, Point2 const& p2, default_strategy const& )
    {
        return geometry::comparable_distance(p1, p2);
    }
};

template <typename MembersHolder>
class remove_elements_to_reinsert
{
public:
    typedef typename MembersHolder::box_type box_type;
    typedef typename MembersHolder::parameters_type parameters_type;
    typedef typename MembersHolder::translator_type translator_type;
    typedef typename MembersHolder::allocators_type allocators_type;

    typedef typename MembersHolder::node node;
    typedef typename MembersHolder::internal_node internal_node;
    typedef typename MembersHolder::leaf leaf;

    //typedef typename Allocators::internal_node_pointer internal_node_pointer;
    typedef internal_node * internal_node_pointer;

    template <typename ResultElements, typename Node>
    static inline void apply(ResultElements & result_elements,
                             Node & n,
                             internal_node_pointer parent,
                             size_t current_child_index,
                             parameters_type const& parameters,
                             translator_type const& translator,
                             allocators_type & allocators)
    {
        typedef typename rtree::elements_type<Node>::type elements_type;
        typedef typename elements_type::value_type element_type;
        typedef typename geometry::point_type<box_type>::type point_type;
        typedef typename index::detail::strategy_type<parameters_type>::type strategy_type;
        // TODO: awulkiew - change second point_type to the point type of the Indexable?
        typedef rstar::comparable_distance
            <
                point_type, point_type, strategy_type
            > comparable_distance_pp;
        typedef typename comparable_distance_pp::result_type comparable_distance_type;

        elements_type & elements = rtree::elements(n);

        const size_t elements_count = parameters.get_max_elements() + 1;
        const size_t reinserted_elements_count = (::std::min)(parameters.get_reinserted_elements(), elements_count - parameters.get_min_elements());

        BOOST_GEOMETRY_INDEX_ASSERT(parent, "node shouldn't be the root node");
        BOOST_GEOMETRY_INDEX_ASSERT(elements.size() == elements_count, "unexpected elements number");
        BOOST_GEOMETRY_INDEX_ASSERT(0 < reinserted_elements_count, "wrong value of elements to reinsert");

        auto const& strategy = index::detail::get_strategy(parameters);

        // calculate current node's center
        point_type node_center;
        geometry::centroid(rtree::elements(*parent)[current_child_index].first, node_center,
                           strategy);

        // fill the container of centers' distances of children from current node's center
        typedef typename index::detail::rtree::container_from_elements_type<
            elements_type,
            std::pair<comparable_distance_type, element_type>
        >::type sorted_elements_type;

        sorted_elements_type sorted_elements;
        // If constructor is used instead of resize() MS implementation leaks here
        sorted_elements.reserve(elements_count);                                                         // MAY THROW, STRONG (V, E: alloc, copy)
        
        for ( typename elements_type::const_iterator it = elements.begin() ;
              it != elements.end() ; ++it )
        {
            point_type element_center;
            geometry::centroid(rtree::element_indexable(*it, translator), element_center,
                               strategy);
            sorted_elements.push_back(std::make_pair(
                comparable_distance_pp::call(node_center, element_center, strategy),
                *it));                                                                                  // MAY THROW (V, E: copy)
        }

        // sort elements by distances from center
        std::partial_sort(
            sorted_elements.begin(),
            sorted_elements.begin() + reinserted_elements_count,
            sorted_elements.end(),
            distances_dsc<comparable_distance_type, element_type>);                                                // MAY THROW, BASIC (V, E: copy)

        // copy elements which will be reinserted
        result_elements.clear();
        result_elements.reserve(reinserted_elements_count);                                             // MAY THROW, STRONG (V, E: alloc, copy)
        for ( typename sorted_elements_type::const_iterator it = sorted_elements.begin() ;
              it != sorted_elements.begin() + reinserted_elements_count ; ++it )
        {
            result_elements.push_back(it->second);                                                      // MAY THROW (V, E: copy)
        }

        BOOST_TRY
        {
            // copy remaining elements to the current node
            elements.clear();
            elements.reserve(elements_count - reinserted_elements_count);                                // SHOULDN'T THROW (new_size <= old size)
            for ( typename sorted_elements_type::const_iterator it = sorted_elements.begin() + reinserted_elements_count;
                  it != sorted_elements.end() ; ++it )
            {
                elements.push_back(it->second);                                                         // MAY THROW (V, E: copy)
            }
        }
        BOOST_CATCH(...)
        {
            elements.clear();

            for ( typename sorted_elements_type::iterator it = sorted_elements.begin() ;
                  it != sorted_elements.end() ; ++it )
            {
                destroy_element<MembersHolder>::apply(it->second, allocators);
            }

            BOOST_RETHROW                                                                                 // RETHROW
        }
        BOOST_CATCH_END

        ::boost::ignore_unused(parameters);
    }

private:
    template <typename Distance, typename El>
    static inline bool distances_asc(
        std::pair<Distance, El> const& d1,
        std::pair<Distance, El> const& d2)
    {
        return d1.first < d2.first;
    }
    
    template <typename Distance, typename El>
    static inline bool distances_dsc(
        std::pair<Distance, El> const& d1,
        std::pair<Distance, El> const& d2)
    {
        return d1.first > d2.first;
    }
};

template
<
    size_t InsertIndex,
    typename Element,
    typename MembersHolder,
    bool IsValue = std::is_same<Element, typename MembersHolder::value_type>::value
>
struct level_insert_elements_type
{
    typedef typename rtree::elements_type<
        typename rtree::internal_node<
            typename MembersHolder::value_type,
            typename MembersHolder::parameters_type,
            typename MembersHolder::box_type,
            typename MembersHolder::allocators_type,
            typename MembersHolder::node_tag
        >::type
    >::type type;
};

template <typename Value, typename MembersHolder>
struct level_insert_elements_type<0, Value, MembersHolder, true>
{
    typedef typename rtree::elements_type<
        typename rtree::leaf<
            typename MembersHolder::value_type,
            typename MembersHolder::parameters_type,
            typename MembersHolder::box_type,
            typename MembersHolder::allocators_type,
            typename MembersHolder::node_tag
        >::type
    >::type type;
};

template <size_t InsertIndex, typename Element, typename MembersHolder>
struct level_insert_base
    : public detail::insert<Element, MembersHolder>
{
    typedef detail::insert<Element, MembersHolder> base;
    typedef typename base::node node;
    typedef typename base::internal_node internal_node;
    typedef typename base::leaf leaf;

    typedef typename level_insert_elements_type<InsertIndex, Element, MembersHolder>::type elements_type;
    typedef typename index::detail::rtree::container_from_elements_type<
        elements_type,
        typename elements_type::value_type
    >::type result_elements_type;

    typedef typename MembersHolder::box_type box_type;
    typedef typename MembersHolder::parameters_type parameters_type;
    typedef typename MembersHolder::translator_type translator_type;
    typedef typename MembersHolder::allocators_type allocators_type;

    typedef typename allocators_type::node_pointer node_pointer;
    typedef typename allocators_type::size_type size_type;

    inline level_insert_base(node_pointer & root,
                             size_type & leafs_level,
                             Element const& element,
                             parameters_type const& parameters,
                             translator_type const& translator,
                             allocators_type & allocators,
                             size_type relative_level)
        : base(root, leafs_level, element, parameters, translator, allocators, relative_level)
        , result_relative_level(0)
    {}

    template <typename Node>
    inline void handle_possible_reinsert_or_split_of_root(Node &n)
    {
        BOOST_GEOMETRY_INDEX_ASSERT(result_elements.empty(), "reinsert should be handled only once for level");

        result_relative_level = base::m_leafs_level - base::m_traverse_data.current_level;

        // overflow
        if ( base::m_parameters.get_max_elements() < rtree::elements(n).size() )
        {
            // node isn't root node
            if ( !base::m_traverse_data.current_is_root() )
            {
                // NOTE: exception-safety
                // After an exception result_elements may contain garbage, don't use it
                rstar::remove_elements_to_reinsert<MembersHolder>::apply(
                    result_elements, n,
                    base::m_traverse_data.parent, base::m_traverse_data.current_child_index,
                    base::m_parameters, base::m_translator, base::m_allocators);                            // MAY THROW, BASIC (V, E: alloc, copy)
            }
            // node is root node
            else
            {
                BOOST_GEOMETRY_INDEX_ASSERT(&n == &rtree::get<Node>(*base::m_root_node), "node should be the root node");
                base::split(n);                                                                             // MAY THROW (V, E: alloc, copy, N: alloc)
            }
        }
    }

    template <typename Node>
    inline void handle_possible_split(Node &n) const
    {
        // overflow
        if ( base::m_parameters.get_max_elements() < rtree::elements(n).size() )
        {
            base::split(n);                                                                                 // MAY THROW (V, E: alloc, copy, N: alloc)
        }
    }

    template <typename Node>
    inline void recalculate_aabb_if_necessary(Node const& n) const
    {
        if ( !result_elements.empty() && !base::m_traverse_data.current_is_root() )
        {
            // calulate node's new box
            recalculate_aabb(n);
        }
    }

    template <typename Node>
    inline void recalculate_aabb(Node const& n) const
    {
        base::m_traverse_data.current_element().first =
            elements_box<box_type>(rtree::elements(n).begin(), rtree::elements(n).end(),
                                   base::m_translator,
                                   index::detail::get_strategy(base::m_parameters));
    }

    inline void recalculate_aabb(leaf const& n) const
    {
        base::m_traverse_data.current_element().first =
            values_box<box_type>(rtree::elements(n).begin(), rtree::elements(n).end(),
                                 base::m_translator,
                                 index::detail::get_strategy(base::m_parameters));
    }

    size_type result_relative_level;
    result_elements_type result_elements;
};

template
<
    size_t InsertIndex,
    typename Element,
    typename MembersHolder,
    bool IsValue = std::is_same<Element, typename MembersHolder::value_type>::value
>
struct level_insert
    : public level_insert_base<InsertIndex, Element, MembersHolder>
{
    typedef level_insert_base<InsertIndex, Element, MembersHolder> base;
    typedef typename base::node node;
    typedef typename base::internal_node internal_node;
    typedef typename base::leaf leaf;

    typedef typename base::parameters_type parameters_type;
    typedef typename base::translator_type translator_type;
    typedef typename base::allocators_type allocators_type;

    typedef typename base::node_pointer node_pointer;
    typedef typename base::size_type size_type;

    inline level_insert(node_pointer & root,
                        size_type & leafs_level,
                        Element const& element,
                        parameters_type const& parameters,
                        translator_type const& translator,
                        allocators_type & allocators,
                        size_type relative_level)
        : base(root, leafs_level, element, parameters, translator, allocators, relative_level)
    {}

    inline void operator()(internal_node & n)
    {
        BOOST_GEOMETRY_INDEX_ASSERT(base::m_traverse_data.current_level < base::m_leafs_level, "unexpected level");

        if ( base::m_traverse_data.current_level < base::m_level )
        {
            // next traversing step
            base::traverse(*this, n);                                                                       // MAY THROW (E: alloc, copy, N: alloc)

            // further insert
            if ( 0 < InsertIndex )
            {
                BOOST_GEOMETRY_INDEX_ASSERT(0 < base::m_level, "illegal level value, level shouldn't be the root level for 0 < InsertIndex");

                if ( base::m_traverse_data.current_level == base::m_level - 1 )
                {
                    base::handle_possible_reinsert_or_split_of_root(n);                                     // MAY THROW (E: alloc, copy, N: alloc)
                }
            }
        }
        else
        {
            BOOST_GEOMETRY_INDEX_ASSERT(base::m_level == base::m_traverse_data.current_level, "unexpected level");

            BOOST_TRY
            {
                // push new child node
                rtree::elements(n).push_back(base::m_element);                                              // MAY THROW, STRONG (E: alloc, copy)
            }
            BOOST_CATCH(...)
            {
                // NOTE: exception-safety
                // if the insert fails above, the element won't be stored in the tree, so delete it

                rtree::visitors::destroy<MembersHolder>::apply(base::m_element.second, base::m_allocators);

                BOOST_RETHROW                                                                                 // RETHROW
            }
            BOOST_CATCH_END

            // first insert
            if ( 0 == InsertIndex )
            {
                base::handle_possible_reinsert_or_split_of_root(n);                                         // MAY THROW (E: alloc, copy, N: alloc)
            }
            // not the first insert
            else
            {
                base::handle_possible_split(n);                                                             // MAY THROW (E: alloc, N: alloc)
            }
        }

        base::recalculate_aabb_if_necessary(n);
    }

    inline void operator()(leaf &)
    {
        BOOST_GEOMETRY_INDEX_ASSERT(false, "this visitor can't be used for a leaf");
    }
};

template <size_t InsertIndex, typename Value, typename MembersHolder>
struct level_insert<InsertIndex, Value, MembersHolder, true>
    : public level_insert_base<InsertIndex, typename MembersHolder::value_type, MembersHolder>
{
    typedef level_insert_base<InsertIndex, typename MembersHolder::value_type, MembersHolder> base;
    typedef typename base::node node;
    typedef typename base::internal_node internal_node;
    typedef typename base::leaf leaf;

    typedef typename MembersHolder::value_type value_type;
    typedef typename base::parameters_type parameters_type;
    typedef typename base::translator_type translator_type;
    typedef typename base::allocators_type allocators_type;

    typedef typename base::node_pointer node_pointer;
    typedef typename base::size_type size_type;

    inline level_insert(node_pointer & root,
                        size_type & leafs_level,
                        value_type const& v,
                        parameters_type const& parameters,
                        translator_type const& translator,
                        allocators_type & allocators,
                        size_type relative_level)
        : base(root, leafs_level, v, parameters, translator, allocators, relative_level)
    {}

    inline void operator()(internal_node & n)
    {
        BOOST_GEOMETRY_INDEX_ASSERT(base::m_traverse_data.current_level < base::m_leafs_level, "unexpected level");
        BOOST_GEOMETRY_INDEX_ASSERT(base::m_traverse_data.current_level < base::m_level, "unexpected level");

        // next traversing step
        base::traverse(*this, n);                                                                       // MAY THROW (V, E: alloc, copy, N: alloc)

        BOOST_GEOMETRY_INDEX_ASSERT(0 < base::m_level, "illegal level value, level shouldn't be the root level for 0 < InsertIndex");
        
        if ( base::m_traverse_data.current_level == base::m_level - 1 )
        {
            base::handle_possible_reinsert_or_split_of_root(n);                                         // MAY THROW (E: alloc, copy, N: alloc)
        }

        base::recalculate_aabb_if_necessary(n);
    }

    inline void operator()(leaf & n)
    {
        BOOST_GEOMETRY_INDEX_ASSERT(base::m_traverse_data.current_level == base::m_leafs_level,
                                    "unexpected level");
        BOOST_GEOMETRY_INDEX_ASSERT(base::m_level == base::m_traverse_data.current_level ||
                                    base::m_level == (std::numeric_limits<size_t>::max)(),
                                    "unexpected level");
        
        rtree::elements(n).push_back(base::m_element);                                                  // MAY THROW, STRONG (V: alloc, copy)

        base::handle_possible_split(n);                                                                 // MAY THROW (V: alloc, copy, N: alloc)
    }
};

template <typename Value, typename MembersHolder>
struct level_insert<0, Value, MembersHolder, true>
    : public level_insert_base<0, typename MembersHolder::value_type, MembersHolder>
{
    typedef level_insert_base<0, typename MembersHolder::value_type, MembersHolder> base;
    typedef typename base::node node;
    typedef typename base::internal_node internal_node;
    typedef typename base::leaf leaf;

    typedef typename MembersHolder::value_type value_type;
    typedef typename base::parameters_type parameters_type;
    typedef typename base::translator_type translator_type;
    typedef typename base::allocators_type allocators_type;

    typedef typename base::node_pointer node_pointer;
    typedef typename base::size_type size_type;

    inline level_insert(node_pointer & root,
                        size_type & leafs_level,
                        value_type const& v,
                        parameters_type const& parameters,
                        translator_type const& translator,
                        allocators_type & allocators,
                        size_type relative_level)
        : base(root, leafs_level, v, parameters, translator, allocators, relative_level)
    {}

    inline void operator()(internal_node & n)
    {
        BOOST_GEOMETRY_INDEX_ASSERT(base::m_traverse_data.current_level < base::m_leafs_level,
                                    "unexpected level");
        BOOST_GEOMETRY_INDEX_ASSERT(base::m_traverse_data.current_level < base::m_level,
                                    "unexpected level");

        // next traversing step
        base::traverse(*this, n);                                                                       // MAY THROW (V: alloc, copy, N: alloc)

        base::recalculate_aabb_if_necessary(n);
    }

    inline void operator()(leaf & n)
    {
        BOOST_GEOMETRY_INDEX_ASSERT(base::m_traverse_data.current_level == base::m_leafs_level,
                                    "unexpected level");
        BOOST_GEOMETRY_INDEX_ASSERT(base::m_level == base::m_traverse_data.current_level ||
                                    base::m_level == (std::numeric_limits<size_t>::max)(),
                                    "unexpected level");

        rtree::elements(n).push_back(base::m_element);                                                  // MAY THROW, STRONG (V: alloc, copy)

        base::handle_possible_reinsert_or_split_of_root(n);                                             // MAY THROW (V: alloc, copy, N: alloc)
        
        base::recalculate_aabb_if_necessary(n);
    }
};

} // namespace rstar

// R*-tree insert visitor
// After passing the Element to insert visitor the Element is managed by the tree
// I.e. one should not delete the node passed to the insert visitor after exception is thrown
// because this visitor may delete it
template <typename Element, typename MembersHolder>
class insert<Element, MembersHolder, insert_reinsert_tag>
    : public MembersHolder::visitor
{
    typedef typename MembersHolder::parameters_type parameters_type;
    typedef typename MembersHolder::translator_type translator_type;
    typedef typename MembersHolder::allocators_type allocators_type;

    typedef typename MembersHolder::node node;
    typedef typename MembersHolder::internal_node internal_node;
    typedef typename MembersHolder::leaf leaf;

    typedef typename allocators_type::node_pointer node_pointer;
    typedef typename allocators_type::size_type size_type;

public:
    inline insert(node_pointer & root,
                  size_type & leafs_level,
                  Element const& element,
                  parameters_type const& parameters,
                  translator_type const& translator,
                  allocators_type & allocators,
                  size_type relative_level = 0)
        : m_root(root), m_leafs_level(leafs_level), m_element(element)
        , m_parameters(parameters), m_translator(translator)
        , m_relative_level(relative_level), m_allocators(allocators)
    {}

    inline void operator()(internal_node & n)
    {
        boost::ignore_unused(n);
        BOOST_GEOMETRY_INDEX_ASSERT(&n == &rtree::get<internal_node>(*m_root), "current node should be the root");

        // Distinguish between situation when reinserts are required and use adequate visitor, otherwise use default one
        if ( m_parameters.get_reinserted_elements() > 0 )
        {
            rstar::level_insert<0, Element, MembersHolder> lins_v(
                m_root, m_leafs_level, m_element, m_parameters, m_translator, m_allocators, m_relative_level);

            rtree::apply_visitor(lins_v, *m_root);                                                              // MAY THROW (V, E: alloc, copy, N: alloc)

            if ( !lins_v.result_elements.empty() )
            {
                recursive_reinsert(lins_v.result_elements, lins_v.result_relative_level);                       // MAY THROW (V, E: alloc, copy, N: alloc)
            }
        }
        else
        {
            visitors::insert<Element, MembersHolder, insert_default_tag> ins_v(
                m_root, m_leafs_level, m_element, m_parameters, m_translator, m_allocators, m_relative_level);

            rtree::apply_visitor(ins_v, *m_root); 
        }
    }

    inline void operator()(leaf & n)
    {
        boost::ignore_unused(n);
        BOOST_GEOMETRY_INDEX_ASSERT(&n == &rtree::get<leaf>(*m_root), "current node should be the root");

        // Distinguish between situation when reinserts are required and use adequate visitor, otherwise use default one
        if ( m_parameters.get_reinserted_elements() > 0 )
        {
            rstar::level_insert<0, Element, MembersHolder> lins_v(
                m_root, m_leafs_level, m_element, m_parameters, m_translator, m_allocators, m_relative_level);

            rtree::apply_visitor(lins_v, *m_root);                                                              // MAY THROW (V, E: alloc, copy, N: alloc)

            // we're in the root, so root should be split and there should be no elements to reinsert
            BOOST_GEOMETRY_INDEX_ASSERT(lins_v.result_elements.empty(), "unexpected state");
        }
        else
        {
            visitors::insert<Element, MembersHolder, insert_default_tag> ins_v(
                m_root, m_leafs_level, m_element, m_parameters, m_translator, m_allocators, m_relative_level);

            rtree::apply_visitor(ins_v, *m_root); 
        }
    }

private:
    template <typename Elements>
    inline void recursive_reinsert(Elements & elements, size_t relative_level)
    {
        typedef typename Elements::value_type element_type;

        // reinsert children starting from the minimum distance
        typename Elements::reverse_iterator it = elements.rbegin();
        for ( ; it != elements.rend() ; ++it)
        {
            rstar::level_insert<1, element_type, MembersHolder> lins_v(
                m_root, m_leafs_level, *it, m_parameters, m_translator, m_allocators, relative_level);

            BOOST_TRY
            {
                rtree::apply_visitor(lins_v, *m_root);                                                          // MAY THROW (V, E: alloc, copy, N: alloc)
            }
            BOOST_CATCH(...)
            {
                ++it;
                for ( ; it != elements.rend() ; ++it)
                    rtree::destroy_element<MembersHolder>::apply(*it, m_allocators);
                BOOST_RETHROW                                                                                     // RETHROW
            }
            BOOST_CATCH_END

            BOOST_GEOMETRY_INDEX_ASSERT(relative_level + 1 == lins_v.result_relative_level, "unexpected level");

            // non-root relative level
            if ( lins_v.result_relative_level < m_leafs_level && !lins_v.result_elements.empty())
            {
                recursive_reinsert(lins_v.result_elements, lins_v.result_relative_level);                   // MAY THROW (V, E: alloc, copy, N: alloc)
            }
        }
    }

    node_pointer & m_root;
    size_type & m_leafs_level;
    Element const& m_element;

    parameters_type const& m_parameters;
    translator_type const& m_translator;

    size_type m_relative_level;

    allocators_type & m_allocators;
};

}}} // namespace detail::rtree::visitors

}}} // namespace boost::geometry::index

#endif // BOOST_GEOMETRY_INDEX_DETAIL_RTREE_RSTAR_INSERT_HPP
