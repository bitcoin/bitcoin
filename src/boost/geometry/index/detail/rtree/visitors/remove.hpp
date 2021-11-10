// Boost.Geometry Index
//
// R-tree removing visitor implementation
//
// Copyright (c) 2011-2017 Adam Wulkiewicz, Lodz, Poland.
//
// This file was modified by Oracle on 2019.
// Modifications copyright (c) 2019 Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle
//
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_INDEX_DETAIL_RTREE_VISITORS_REMOVE_HPP
#define BOOST_GEOMETRY_INDEX_DETAIL_RTREE_VISITORS_REMOVE_HPP

#include <boost/geometry/index/detail/rtree/visitors/destroy.hpp>
#include <boost/geometry/index/detail/rtree/visitors/is_leaf.hpp>

#include <boost/geometry/algorithms/detail/covered_by/interface.hpp>

namespace boost { namespace geometry { namespace index {

namespace detail { namespace rtree { namespace visitors {

// Default remove algorithm
template <typename MembersHolder>
class remove
    : public MembersHolder::visitor
{
    typedef typename MembersHolder::box_type box_type;
    typedef typename MembersHolder::value_type value_type;
    typedef typename MembersHolder::parameters_type parameters_type;
    typedef typename MembersHolder::translator_type translator_type;
    typedef typename MembersHolder::allocators_type allocators_type;

    typedef typename MembersHolder::node node;
    typedef typename MembersHolder::internal_node internal_node;
    typedef typename MembersHolder::leaf leaf;

    typedef rtree::subtree_destroyer<MembersHolder> subtree_destroyer;
    typedef typename allocators_type::node_pointer node_pointer;
    typedef typename allocators_type::size_type size_type;

    typedef typename rtree::elements_type<internal_node>::type::size_type internal_size_type;

    //typedef typename Allocators::internal_node_pointer internal_node_pointer;
    typedef internal_node * internal_node_pointer;

public:
    inline remove(node_pointer & root,
                  size_type & leafs_level,
                  value_type const& value,
                  parameters_type const& parameters,
                  translator_type const& translator,
                  allocators_type & allocators)
        : m_value(value)
        , m_parameters(parameters)
        , m_translator(translator)
        , m_allocators(allocators)
        , m_root_node(root)
        , m_leafs_level(leafs_level)
        , m_is_value_removed(false)
        , m_parent(0)
        , m_current_child_index(0)
        , m_current_level(0)
        , m_is_underflow(false)
    {
        // TODO
        // assert - check if Value/Box is correct
    }

    inline void operator()(internal_node & n)
    {
        typedef typename rtree::elements_type<internal_node>::type children_type;
        children_type & children = rtree::elements(n);

        // traverse children which boxes intersects value's box
        internal_size_type child_node_index = 0;
        for ( ; child_node_index < children.size() ; ++child_node_index )
        {
            if ( index::detail::covered_by_bounds(m_translator(m_value),
                                                  children[child_node_index].first,
                                                  index::detail::get_strategy(m_parameters)) )
            {
                // next traversing step
                traverse_apply_visitor(n, child_node_index);                                                            // MAY THROW

                if ( m_is_value_removed )
                    break;
            }
        }

        // value was found and removed
        if ( m_is_value_removed )
        {
            typedef typename rtree::elements_type<internal_node>::type elements_type;
            typedef typename elements_type::iterator element_iterator;
            elements_type & elements = rtree::elements(n);

            // underflow occured - child node should be removed
            if ( m_is_underflow )
            {
                element_iterator underfl_el_it = elements.begin() + child_node_index;
                size_type relative_level = m_leafs_level - m_current_level;

                // move node to the container - store node's relative level as well and return new underflow state
                // NOTE: if the min elements number is 1, then after an underflow
                //       here the child elements count is 0, so it's not required to store this node,
                //       it could just be destroyed
                m_is_underflow = store_underflowed_node(elements, underfl_el_it, relative_level);                       // MAY THROW (E: alloc, copy)
            }

            // n is not root - adjust aabb
            if ( 0 != m_parent )
            {
                // underflow state should be ok here
                // note that there may be less than min_elems elements in root
                // so this condition should be checked only here
                BOOST_GEOMETRY_INDEX_ASSERT((elements.size() < m_parameters.get_min_elements()) == m_is_underflow, "unexpected state");

                rtree::elements(*m_parent)[m_current_child_index].first
                    = rtree::elements_box<box_type>(elements.begin(), elements.end(), m_translator,
                                                    index::detail::get_strategy(m_parameters));
            }
            // n is root node
            else
            {
                BOOST_GEOMETRY_INDEX_ASSERT(&n == &rtree::get<internal_node>(*m_root_node), "node must be the root");

                // reinsert elements from removed nodes (underflows)
                reinsert_removed_nodes_elements();                                                                  // MAY THROW (V, E: alloc, copy, N: alloc)

                // shorten the tree
                // NOTE: if the min elements number is 1, then after underflow
                //       here the number of elements may be equal to 0
                //       this can occur only for the last removed element
                if ( rtree::elements(n).size() <= 1 )
                {
                    node_pointer root_to_destroy = m_root_node;
                    if ( rtree::elements(n).size() == 0 )
                        m_root_node = 0;
                    else
                        m_root_node = rtree::elements(n)[0].second;
                    --m_leafs_level;

                    rtree::destroy_node<allocators_type, internal_node>::apply(m_allocators, root_to_destroy);
                }
            }
        }
    }

    inline void operator()(leaf & n)
    {
        typedef typename rtree::elements_type<leaf>::type elements_type;
        elements_type & elements = rtree::elements(n);
        
        // find value and remove it
        for ( typename elements_type::iterator it = elements.begin() ; it != elements.end() ; ++it )
        {
            if ( m_translator.equals(*it, m_value, index::detail::get_strategy(m_parameters)) )
            {
                rtree::move_from_back(elements, it);                                                           // MAY THROW (V: copy)
                elements.pop_back();
                m_is_value_removed = true;
                break;
            }
        }

        // if value was removed
        if ( m_is_value_removed )
        {
            BOOST_GEOMETRY_INDEX_ASSERT(0 < m_parameters.get_min_elements(), "min number of elements is too small");

            // calc underflow
            m_is_underflow = elements.size() < m_parameters.get_min_elements();

            // n is not root - adjust aabb
            if ( 0 != m_parent )
            {
                rtree::elements(*m_parent)[m_current_child_index].first
                    = rtree::values_box<box_type>(elements.begin(), elements.end(), m_translator,
                                                  index::detail::get_strategy(m_parameters));
            }
        }
    }

    bool is_value_removed() const
    {
        return m_is_value_removed;
    }

private:

    typedef std::vector< std::pair<size_type, node_pointer> > underflow_nodes;

    void traverse_apply_visitor(internal_node &n, internal_size_type choosen_node_index)
    {
        // save previous traverse inputs and set new ones
        internal_node_pointer parent_bckup = m_parent;
        internal_size_type current_child_index_bckup = m_current_child_index;
        size_type current_level_bckup = m_current_level;

        m_parent = &n;
        m_current_child_index = choosen_node_index;
        ++m_current_level;

        // next traversing step
        rtree::apply_visitor(*this, *rtree::elements(n)[choosen_node_index].second);                    // MAY THROW (V, E: alloc, copy, N: alloc)

        // restore previous traverse inputs
        m_parent = parent_bckup;
        m_current_child_index = current_child_index_bckup;
        m_current_level = current_level_bckup;
    }

    bool store_underflowed_node(
            typename rtree::elements_type<internal_node>::type & elements,
            typename rtree::elements_type<internal_node>::type::iterator underfl_el_it,
            size_type relative_level)
    {
        // move node to the container - store node's relative level as well
        m_underflowed_nodes.push_back(std::make_pair(relative_level, underfl_el_it->second));           // MAY THROW (E: alloc, copy)

        BOOST_TRY
        {
            // NOTE: those are elements of the internal node which means that copy/move shouldn't throw
            // Though it's safer in case if the pointer type could throw in copy ctor.
            // In the future this try-catch block could be removed.
            rtree::move_from_back(elements, underfl_el_it);                                             // MAY THROW (E: copy)
            elements.pop_back();
        }
        BOOST_CATCH(...)
        {
            m_underflowed_nodes.pop_back();
            BOOST_RETHROW                                                                                 // RETHROW
        }
        BOOST_CATCH_END

        // calc underflow
        return elements.size() < m_parameters.get_min_elements();
    }

    static inline bool is_leaf(node const& n)
    {
        visitors::is_leaf<MembersHolder> ilv;
        rtree::apply_visitor(ilv, n);
        return ilv.result;
    }

    void reinsert_removed_nodes_elements()
    {
        typename underflow_nodes::reverse_iterator it = m_underflowed_nodes.rbegin();

        BOOST_TRY
        {
            // reinsert elements from removed nodes
            // begin with levels closer to the root
            for ( ; it != m_underflowed_nodes.rend() ; ++it )
            {
                // it->first is an index of a level of a node, not children
                // counted from the leafs level
                bool const node_is_leaf = it->first == 1;
                BOOST_GEOMETRY_INDEX_ASSERT(node_is_leaf == is_leaf(*it->second), "unexpected condition");
                if ( node_is_leaf )
                {
                    reinsert_node_elements(rtree::get<leaf>(*it->second), it->first);                        // MAY THROW (V, E: alloc, copy, N: alloc)

                    rtree::destroy_node<allocators_type, leaf>::apply(m_allocators, it->second);
                }
                else
                {
                    reinsert_node_elements(rtree::get<internal_node>(*it->second), it->first);               // MAY THROW (V, E: alloc, copy, N: alloc)

                    rtree::destroy_node<allocators_type, internal_node>::apply(m_allocators, it->second);
                }
            }

            //m_underflowed_nodes.clear();
        }
        BOOST_CATCH(...)
        {
            // destroy current and remaining nodes
            for ( ; it != m_underflowed_nodes.rend() ; ++it )
            {
                rtree::visitors::destroy<MembersHolder>::apply(it->second, m_allocators);
            }

            //m_underflowed_nodes.clear();

            BOOST_RETHROW                                                                                      // RETHROW
        }
        BOOST_CATCH_END
    }

    template <typename Node>
    void reinsert_node_elements(Node &n, size_type node_relative_level)
    {
        typedef typename rtree::elements_type<Node>::type elements_type;
        elements_type & elements = rtree::elements(n);

        typename elements_type::iterator it = elements.begin();
        BOOST_TRY
        {
            for ( ; it != elements.end() ; ++it )
            {
                visitors::insert<typename elements_type::value_type, MembersHolder>
                    insert_v(m_root_node, m_leafs_level, *it,
                             m_parameters, m_translator, m_allocators,
                             node_relative_level - 1);

                rtree::apply_visitor(insert_v, *m_root_node);                                               // MAY THROW (V, E: alloc, copy, N: alloc)
            }
        }
        BOOST_CATCH(...)
        {
            ++it;
            rtree::destroy_elements<MembersHolder>::apply(it, elements.end(), m_allocators);
            elements.clear();
            BOOST_RETHROW                                                                                     // RETHROW
        }
        BOOST_CATCH_END
    }

    value_type const& m_value;
    parameters_type const& m_parameters;
    translator_type const& m_translator;
    allocators_type & m_allocators;

    node_pointer & m_root_node;
    size_type & m_leafs_level;

    bool m_is_value_removed;
    underflow_nodes m_underflowed_nodes;

    // traversing input parameters
    internal_node_pointer m_parent;
    internal_size_type m_current_child_index;
    size_type m_current_level;

    // traversing output parameters
    bool m_is_underflow;
};

}}} // namespace detail::rtree::visitors

}}} // namespace boost::geometry::index

#endif // BOOST_GEOMETRY_INDEX_DETAIL_RTREE_VISITORS_REMOVE_HPP
