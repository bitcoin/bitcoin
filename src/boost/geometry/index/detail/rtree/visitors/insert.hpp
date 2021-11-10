// Boost.Geometry Index
//
// R-tree inserting visitor implementation
//
// Copyright (c) 2011-2015 Adam Wulkiewicz, Lodz, Poland.
//
// This file was modified by Oracle on 2019-2020.
// Modifications copyright (c) 2019-2020 Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle
//
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_INDEX_DETAIL_RTREE_VISITORS_INSERT_HPP
#define BOOST_GEOMETRY_INDEX_DETAIL_RTREE_VISITORS_INSERT_HPP

#ifdef BOOST_GEOMETRY_INDEX_EXPERIMENTAL_ENLARGE_BY_EPSILON
#include <type_traits>
#endif

#include <boost/geometry/algorithms/detail/expand_by_epsilon.hpp>
#include <boost/geometry/core/static_assert.hpp>
#include <boost/geometry/util/condition.hpp>

#include <boost/geometry/index/detail/algorithms/bounds.hpp>
#include <boost/geometry/index/detail/algorithms/content.hpp>

#include <boost/geometry/index/detail/rtree/node/subtree_destroyer.hpp>

namespace boost { namespace geometry { namespace index {

namespace detail { namespace rtree {

// Default choose_next_node
template
<
    typename MembersHolder,
    typename ChooseNextNodeTag = typename MembersHolder::options_type::choose_next_node_tag
>
class choose_next_node;

template <typename MembersHolder>
class choose_next_node<MembersHolder, choose_by_content_diff_tag>
{
public:
    typedef typename MembersHolder::box_type box_type;
    typedef typename MembersHolder::parameters_type parameters_type;

    typedef typename MembersHolder::node node;
    typedef typename MembersHolder::internal_node internal_node;
    typedef typename MembersHolder::leaf leaf;

    typedef typename rtree::elements_type<internal_node>::type children_type;

    typedef typename index::detail::default_content_result<box_type>::type content_type;

    template <typename Indexable>
    static inline size_t apply(internal_node & n,
                               Indexable const& indexable,
                               parameters_type const& parameters,
                               size_t /*node_relative_level*/)
    {
        children_type & children = rtree::elements(n);

        BOOST_GEOMETRY_INDEX_ASSERT(!children.empty(), "can't choose the next node if children are empty");

        size_t children_count = children.size();

        // choose index with smallest content change or smallest content
        size_t choosen_index = 0;
        content_type smallest_content_diff = (std::numeric_limits<content_type>::max)();
        content_type smallest_content = (std::numeric_limits<content_type>::max)();

        // caculate areas and areas of all nodes' boxes
        for ( size_t i = 0 ; i < children_count ; ++i )
        {
            typedef typename children_type::value_type child_type;
            child_type const& ch_i = children[i];

            // expanded child node's box
            box_type box_exp(ch_i.first);
            index::detail::expand(box_exp, indexable,
                                  index::detail::get_strategy(parameters));

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

// ----------------------------------------------------------------------- //

// Not implemented here
template
<
    typename MembersHolder,
    typename RedistributeTag = typename MembersHolder::options_type::redistribute_tag
>
struct redistribute_elements
{
    BOOST_GEOMETRY_STATIC_ASSERT_FALSE(
        "Not implemented for this RedistributeTag type.",
        MembersHolder, RedistributeTag);
};

// ----------------------------------------------------------------------- //

// Split algorithm
template
<
    typename MembersHolder,
    typename SplitTag = typename MembersHolder::options_type::split_tag
>
class split
{
    BOOST_GEOMETRY_STATIC_ASSERT_FALSE(
        "Not implemented for this SplitTag type.",
        MembersHolder, SplitTag);
};

// Default split algorithm
template <typename MembersHolder>
class split<MembersHolder, split_default_tag>
{
protected:
    typedef typename MembersHolder::parameters_type parameters_type;
    typedef typename MembersHolder::box_type box_type;
    typedef typename MembersHolder::translator_type translator_type;
    typedef typename MembersHolder::allocators_type allocators_type;
    typedef typename MembersHolder::size_type size_type;

    typedef typename MembersHolder::node node;
    typedef typename MembersHolder::internal_node internal_node;
    typedef typename MembersHolder::leaf leaf;

    typedef typename MembersHolder::node_pointer node_pointer;

public:
    typedef index::detail::varray<
        typename rtree::elements_type<internal_node>::type::value_type,
        1
    > nodes_container_type;

    template <typename Node>
    static inline void apply(nodes_container_type & additional_nodes,
                             Node & n,
                             box_type & n_box,
                             parameters_type const& parameters,
                             translator_type const& translator,
                             allocators_type & allocators)
    {
        // TODO - consider creating nodes always with sufficient memory allocated

        // create additional node, use auto destroyer for automatic destruction on exception
        node_pointer n2_ptr = rtree::create_node<allocators_type, Node>::apply(allocators);                  // MAY THROW, STRONG (N: alloc)
        // create reference to the newly created node
        Node & n2 = rtree::get<Node>(*n2_ptr);

        BOOST_TRY
        {
            // NOTE: thread-safety
            // After throwing an exception by redistribute_elements the original node may be not changed or
            // both nodes may be empty. In both cases the tree won't be valid r-tree.
            // The alternative is to create 2 (or more) additional nodes here and store backup info
            // in the original node, then, if exception was thrown, the node would always have more than max
            // elements.
            // The alternative is to use moving semantics in the implementations of redistribute_elements,
            // it will be possible to throw from boost::move() in the case of e.g. static size nodes.

            // redistribute elements
            box_type box2;
            redistribute_elements<MembersHolder>
                ::apply(n, n2, n_box, box2, parameters, translator, allocators);                                   // MAY THROW (V, E: alloc, copy, copy)

            // check numbers of elements
            BOOST_GEOMETRY_INDEX_ASSERT(parameters.get_min_elements() <= rtree::elements(n).size() &&
                rtree::elements(n).size() <= parameters.get_max_elements(),
                "unexpected number of elements");
            BOOST_GEOMETRY_INDEX_ASSERT(parameters.get_min_elements() <= rtree::elements(n2).size() &&
                rtree::elements(n2).size() <= parameters.get_max_elements(),
                "unexpected number of elements");

            // return the list of newly created nodes (this algorithm returns one)
            additional_nodes.push_back(rtree::make_ptr_pair(box2, n2_ptr));                                  // MAY THROW, STRONG (alloc, copy)
        }
        BOOST_CATCH(...)
        {
            // NOTE: This code is here to prevent leaving the rtree in a state
            //  after an exception is thrown in which pushing new element could
            //  result in assert or putting it outside the memory of node elements.
            typename rtree::elements_type<Node>::type & elements = rtree::elements(n);
            size_type const max_size = parameters.get_max_elements();
            if (elements.size() > max_size)
            {
                rtree::destroy_element<MembersHolder>::apply(elements[max_size], allocators);
                elements.pop_back();
            }

            rtree::visitors::destroy<MembersHolder>::apply(n2_ptr, allocators);

            BOOST_RETHROW
        }
        BOOST_CATCH_END
    }
};

// ----------------------------------------------------------------------- //

namespace visitors { namespace detail {

template <typename InternalNode, typename InternalNodePtr, typename SizeType>
struct insert_traverse_data
{
    typedef typename rtree::elements_type<InternalNode>::type elements_type;
    typedef typename elements_type::value_type element_type;
    typedef typename elements_type::size_type elements_size_type;
    typedef SizeType size_type;

    insert_traverse_data()
        : parent(0), current_child_index(0), current_level(0)
    {}

    void move_to_next_level(InternalNodePtr new_parent,
                            elements_size_type new_child_index)
    {
        parent = new_parent;
        current_child_index = new_child_index;
        ++current_level;
    }

    bool current_is_root() const
    {
        return 0 == parent;
    }

    elements_type & parent_elements() const
    {
        BOOST_GEOMETRY_INDEX_ASSERT(parent, "null pointer");
        return rtree::elements(*parent);
    }

    element_type & current_element() const
    {
        BOOST_GEOMETRY_INDEX_ASSERT(parent, "null pointer");
        return rtree::elements(*parent)[current_child_index];
    }

    InternalNodePtr parent;
    elements_size_type current_child_index;
    size_type current_level;
};

// Default insert visitor
template <typename Element, typename MembersHolder>
class insert
    : public MembersHolder::visitor
{
protected:
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

    //typedef typename allocators_type::internal_node_pointer internal_node_pointer;
    typedef internal_node * internal_node_pointer;

    inline insert(node_pointer & root,
                  size_type & leafs_level,
                  Element const& element,
                  parameters_type const& parameters,
                  translator_type const& translator,
                  allocators_type & allocators,
                  size_type relative_level = 0
    )
        : m_element(element)
        , m_parameters(parameters)
        , m_translator(translator)
        , m_relative_level(relative_level)
        , m_level(leafs_level - relative_level)
        , m_root_node(root)
        , m_leafs_level(leafs_level)
        , m_traverse_data()
        , m_allocators(allocators)
    {
        BOOST_GEOMETRY_INDEX_ASSERT(m_relative_level <= leafs_level, "unexpected level value");
        BOOST_GEOMETRY_INDEX_ASSERT(m_level <= m_leafs_level, "unexpected level value");
        BOOST_GEOMETRY_INDEX_ASSERT(0 != m_root_node, "there is no root node");
        // TODO
        // assert - check if Box is correct

        // When a value is inserted, during the tree traversal bounds of nodes
        // on a path from the root to a leaf must be expanded. So prepare
        // a bounding object at the beginning to not do it later for each node.
        // NOTE: This is actually only needed because conditionally the bounding
        //       object may be expanded below. Otherwise the indexable could be
        //       directly used instead
        index::detail::bounds(rtree::element_indexable(m_element, m_translator),
                              m_element_bounds,
                              index::detail::get_strategy(m_parameters));

#ifdef BOOST_GEOMETRY_INDEX_EXPERIMENTAL_ENLARGE_BY_EPSILON
        // Enlarge it in case if it's not bounding geometry type.
        // It's because Points and Segments are compared WRT machine epsilon
        // This ensures that leafs bounds correspond to the stored elements
        if (BOOST_GEOMETRY_CONDITION((
                std::is_same<Element, value_type>::value
             && ! index::detail::is_bounding_geometry
                    <
                        typename indexable_type<translator_type>::type
                    >::value )) )
        {
            geometry::detail::expand_by_epsilon(m_element_bounds);
        }
#endif
    }

    template <typename Visitor>
    inline void traverse(Visitor & visitor, internal_node & n)
    {
        // choose next node
        size_t choosen_node_index = rtree::choose_next_node<MembersHolder>
            ::apply(n, rtree::element_indexable(m_element, m_translator),
                    m_parameters,
                    m_leafs_level - m_traverse_data.current_level);

        // expand the node to contain value
        index::detail::expand(
            rtree::elements(n)[choosen_node_index].first,
            m_element_bounds,
            index::detail::get_strategy(m_parameters));

        // next traversing step
        traverse_apply_visitor(visitor, n, choosen_node_index);                                                 // MAY THROW (V, E: alloc, copy, N:alloc)
    }

    // TODO: awulkiew - change post_traverse name to handle_overflow or overflow_treatment?

    template <typename Node>
    inline void post_traverse(Node &n)
    {
        BOOST_GEOMETRY_INDEX_ASSERT(m_traverse_data.current_is_root() ||
                                    &n == &rtree::get<Node>(*m_traverse_data.current_element().second),
                                    "if node isn't the root current_child_index should be valid");

        // handle overflow
        if ( m_parameters.get_max_elements() < rtree::elements(n).size() )
        {
            // NOTE: If the exception is thrown current node may contain more than MAX elements or be empty.
            // Furthermore it may be empty root - internal node.
            split(n);                                                                                           // MAY THROW (V, E: alloc, copy, N:alloc)
        }
    }

    template <typename Visitor>
    inline void traverse_apply_visitor(Visitor & visitor, internal_node &n, size_t choosen_node_index)
    {
        // save previous traverse inputs and set new ones
        insert_traverse_data<internal_node, internal_node_pointer, size_type>
            backup_traverse_data = m_traverse_data;

        // calculate new traverse inputs
        m_traverse_data.move_to_next_level(&n, choosen_node_index);

        // next traversing step
        rtree::apply_visitor(visitor, *rtree::elements(n)[choosen_node_index].second);                          // MAY THROW (V, E: alloc, copy, N:alloc)

        // restore previous traverse inputs
        m_traverse_data = backup_traverse_data;
    }

    // TODO: consider - split result returned as OutIter is faster than reference to the container. Why?

    template <typename Node>
    inline void split(Node & n) const
    {
        typedef rtree::split<MembersHolder> split_algo;

        typename split_algo::nodes_container_type additional_nodes;
        box_type n_box;

        split_algo::apply(additional_nodes, n, n_box, m_parameters, m_translator, m_allocators);                // MAY THROW (V, E: alloc, copy, N:alloc)

        BOOST_GEOMETRY_INDEX_ASSERT(additional_nodes.size() == 1, "unexpected number of additional nodes");

        // TODO add all additional nodes
        // For kmeans algorithm:
        // elements number may be greater than node max elements count
        // split and reinsert must take node with some elements count
        // and container of additional elements (std::pair<Box, node*>s or Values)
        // and translator + allocators
        // where node_elements_count + additional_elements > node_max_elements_count
        // What with elements other than std::pair<Box, node*> ?
        // Implement template <node_tag> struct node_element_type or something like that

        // for exception safety
        subtree_destroyer additional_node_ptr(additional_nodes[0].second, m_allocators);

#ifdef BOOST_GEOMETRY_INDEX_EXPERIMENTAL_ENLARGE_BY_EPSILON
        // Enlarge bounds of a leaf node.
        // It's because Points and Segments are compared WRT machine epsilon
        // This ensures that leafs' bounds correspond to the stored elements.
        if (BOOST_GEOMETRY_CONDITION((
                std::is_same<Node, leaf>::value
             && ! index::detail::is_bounding_geometry
                    <
                        typename indexable_type<translator_type>::type
                    >::value )))
        {
            geometry::detail::expand_by_epsilon(n_box);
            geometry::detail::expand_by_epsilon(additional_nodes[0].first);
        }
#endif

        // node is not the root - just add the new node
        if ( !m_traverse_data.current_is_root() )
        {
            // update old node's box
            m_traverse_data.current_element().first = n_box;
            // add new node to parent's children
            m_traverse_data.parent_elements().push_back(additional_nodes[0]);                                     // MAY THROW, STRONG (V, E: alloc, copy)
        }
        // node is the root - add level
        else
        {
            BOOST_GEOMETRY_INDEX_ASSERT(&n == &rtree::get<Node>(*m_root_node), "node should be the root");

            // create new root and add nodes
            subtree_destroyer new_root(rtree::create_node<allocators_type, internal_node>::apply(m_allocators), m_allocators); // MAY THROW, STRONG (N:alloc)

            BOOST_TRY
            {
                rtree::elements(rtree::get<internal_node>(*new_root)).push_back(rtree::make_ptr_pair(n_box, m_root_node));  // MAY THROW, STRONG (E:alloc, copy)
                rtree::elements(rtree::get<internal_node>(*new_root)).push_back(additional_nodes[0]);                 // MAY THROW, STRONG (E:alloc, copy)
            }
            BOOST_CATCH(...)
            {
                // clear new root to not delete in the ~subtree_destroyer() potentially stored old root node
                rtree::elements(rtree::get<internal_node>(*new_root)).clear();
                BOOST_RETHROW                                                                                           // RETHROW
            }
            BOOST_CATCH_END

            m_root_node = new_root.get();
            ++m_leafs_level;

            new_root.release();
        }

        additional_node_ptr.release();
    }

    // TODO: awulkiew - implement dispatchable split::apply to enable additional nodes creation

    Element const& m_element;
    box_type m_element_bounds;
    parameters_type const& m_parameters;
    translator_type const& m_translator;
    size_type const m_relative_level;
    size_type const m_level;

    node_pointer & m_root_node;
    size_type & m_leafs_level;

    // traversing input parameters
    insert_traverse_data<internal_node, internal_node_pointer, size_type> m_traverse_data;

    allocators_type & m_allocators;
};

} // namespace detail

// Insert visitor forward declaration
template
<
    typename Element,
    typename MembersHolder,
    typename InsertTag = typename MembersHolder::options_type::insert_tag
>
class insert;

// Default insert visitor used for nodes elements
// After passing the Element to insert visitor the Element is managed by the tree
// I.e. one should not delete the node passed to the insert visitor after exception is thrown
// because this visitor may delete it
template <typename Element, typename MembersHolder>
class insert<Element, MembersHolder, insert_default_tag>
    : public detail::insert<Element, MembersHolder>
{
public:
    typedef detail::insert<Element, MembersHolder> base;

    typedef typename base::parameters_type parameters_type;
    typedef typename base::translator_type translator_type;
    typedef typename base::allocators_type allocators_type;

    typedef typename base::node node;
    typedef typename base::internal_node internal_node;
    typedef typename base::leaf leaf;

    typedef typename base::node_pointer node_pointer;
    typedef typename base::size_type size_type;

    inline insert(node_pointer & root,
                  size_type & leafs_level,
                  Element const& element,
                  parameters_type const& parameters,
                  translator_type const& translator,
                  allocators_type & allocators,
                  size_type relative_level = 0
    )
        : base(root, leafs_level, element, parameters, translator, allocators, relative_level)
    {}

    inline void operator()(internal_node & n)
    {
        BOOST_GEOMETRY_INDEX_ASSERT(base::m_traverse_data.current_level < base::m_leafs_level, "unexpected level");

        if ( base::m_traverse_data.current_level < base::m_level )
        {
            // next traversing step
            base::traverse(*this, n);                                                                           // MAY THROW (E: alloc, copy, N: alloc)
        }
        else
        {
            BOOST_GEOMETRY_INDEX_ASSERT(base::m_level == base::m_traverse_data.current_level, "unexpected level");

            BOOST_TRY
            {
                // push new child node
                rtree::elements(n).push_back(base::m_element);                                                  // MAY THROW, STRONG (E: alloc, copy)
            }
            BOOST_CATCH(...)
            {
                // if the insert fails above, the element won't be stored in the tree

                rtree::visitors::destroy<MembersHolder>::apply(base::m_element.second, base::m_allocators);

                BOOST_RETHROW                                                                                     // RETHROW
            }
            BOOST_CATCH_END
        }

        base::post_traverse(n);                                                                                 // MAY THROW (E: alloc, copy, N: alloc)
    }

    inline void operator()(leaf &)
    {
        BOOST_GEOMETRY_INDEX_ASSERT(false, "this visitor can't be used for a leaf");
    }
};

// Default insert visitor specialized for Values elements
template <typename MembersHolder>
class insert<typename MembersHolder::value_type, MembersHolder, insert_default_tag>
    : public detail::insert<typename MembersHolder::value_type, MembersHolder>
{
public:
    typedef detail::insert<typename MembersHolder::value_type, MembersHolder> base;

    typedef typename base::value_type value_type;
    typedef typename base::parameters_type parameters_type;
    typedef typename base::translator_type translator_type;
    typedef typename base::allocators_type allocators_type;

    typedef typename base::node node;
    typedef typename base::internal_node internal_node;
    typedef typename base::leaf leaf;

    typedef typename base::node_pointer node_pointer;
    typedef typename base::size_type size_type;

    inline insert(node_pointer & root,
                  size_type & leafs_level,
                  value_type const& value,
                  parameters_type const& parameters,
                  translator_type const& translator,
                  allocators_type & allocators,
                  size_type relative_level = 0
    )
        : base(root, leafs_level, value, parameters, translator, allocators, relative_level)
    {}

    inline void operator()(internal_node & n)
    {
        BOOST_GEOMETRY_INDEX_ASSERT(base::m_traverse_data.current_level < base::m_leafs_level, "unexpected level");
        BOOST_GEOMETRY_INDEX_ASSERT(base::m_traverse_data.current_level < base::m_level, "unexpected level");

        // next traversing step
        base::traverse(*this, n);                                                                                   // MAY THROW (V, E: alloc, copy, N: alloc)

        base::post_traverse(n);                                                                                     // MAY THROW (E: alloc, copy, N: alloc)
    }

    inline void operator()(leaf & n)
    {
        BOOST_GEOMETRY_INDEX_ASSERT(base::m_traverse_data.current_level == base::m_leafs_level, "unexpected level");
        BOOST_GEOMETRY_INDEX_ASSERT(base::m_level == base::m_traverse_data.current_level ||
                                    base::m_level == (std::numeric_limits<size_t>::max)(), "unexpected level");
        
        rtree::elements(n).push_back(base::m_element);                                                              // MAY THROW, STRONG (V: alloc, copy)

        base::post_traverse(n);                                                                                     // MAY THROW (V: alloc, copy, N: alloc)
    }
};

}}} // namespace detail::rtree::visitors

}}} // namespace boost::geometry::index

#endif // BOOST_GEOMETRY_INDEX_DETAIL_RTREE_VISITORS_INSERT_HPP
