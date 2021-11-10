//=======================================================================
// Copyright (c) Aaron Windsor 2007
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#ifndef __FACE_ITERATORS_HPP__
#define __FACE_ITERATORS_HPP__

#include <boost/iterator/iterator_facade.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/graph/graph_traits.hpp>

namespace boost
{

// tags for defining traversal properties

// VisitorType
struct lead_visitor
{
};
struct follow_visitor
{
};

// TraversalType
struct single_side
{
};
struct both_sides
{
};

// TraversalSubType
struct first_side
{
}; // for single_side
struct second_side
{
}; // for single_side
struct alternating
{
}; // for both_sides

// Time
struct current_iteration
{
};
struct previous_iteration
{
};

// Why TraversalType AND TraversalSubType? TraversalSubType is a function
// template parameter passed in to the constructor of the face iterator,
// whereas TraversalType is a class template parameter. This lets us decide
// at runtime whether to move along the first or second side of a bicomp (by
// assigning a face_iterator that has been constructed with TraversalSubType
// = first_side or second_side to a face_iterator variable) without any of
// the virtual function overhead that comes with implementing this
// functionality as a more structured form of type erasure. It also allows
// a single face_iterator to be the end iterator of two iterators traversing
// both sides of a bicomp.

// ValueType is either graph_traits<Graph>::vertex_descriptor
// or graph_traits<Graph>::edge_descriptor

// forward declaration (defining defaults)
template < typename Graph, typename FaceHandlesMap, typename ValueType,
    typename BicompSideToTraverse = single_side,
    typename VisitorType = lead_visitor, typename Time = current_iteration >
class face_iterator;

template < typename Graph, bool StoreEdge > struct edge_storage
{
};

template < typename Graph > struct edge_storage< Graph, true >
{
    typename graph_traits< Graph >::edge_descriptor value;
};

// specialization for TraversalType = traverse_vertices
template < typename Graph, typename FaceHandlesMap, typename ValueType,
    typename TraversalType, typename VisitorType, typename Time >

class face_iterator : public boost::iterator_facade<
                          face_iterator< Graph, FaceHandlesMap, ValueType,
                              TraversalType, VisitorType, Time >,
                          ValueType, boost::forward_traversal_tag, ValueType >
{
public:
    typedef typename graph_traits< Graph >::vertex_descriptor vertex_t;
    typedef typename graph_traits< Graph >::edge_descriptor edge_t;
    typedef face_iterator< Graph, FaceHandlesMap, ValueType, TraversalType,
        VisitorType, Time >
        self;
    typedef typename FaceHandlesMap::value_type face_handle_t;

    face_iterator()
    : m_lead(graph_traits< Graph >::null_vertex())
    , m_follow(graph_traits< Graph >::null_vertex())
    {
    }

    template < typename TraversalSubType >
    face_iterator(face_handle_t anchor_handle, FaceHandlesMap face_handles,
        TraversalSubType traversal_type)
    : m_follow(anchor_handle.get_anchor()), m_face_handles(face_handles)
    {
        set_lead_dispatch(anchor_handle, traversal_type);
    }

    template < typename TraversalSubType >
    face_iterator(vertex_t anchor, FaceHandlesMap face_handles,
        TraversalSubType traversal_type)
    : m_follow(anchor), m_face_handles(face_handles)
    {
        set_lead_dispatch(m_face_handles[anchor], traversal_type);
    }

private:
    friend class boost::iterator_core_access;

    inline vertex_t get_first_vertex(
        face_handle_t anchor_handle, current_iteration)
    {
        return anchor_handle.first_vertex();
    }

    inline vertex_t get_second_vertex(
        face_handle_t anchor_handle, current_iteration)
    {
        return anchor_handle.second_vertex();
    }

    inline vertex_t get_first_vertex(
        face_handle_t anchor_handle, previous_iteration)
    {
        return anchor_handle.old_first_vertex();
    }

    inline vertex_t get_second_vertex(
        face_handle_t anchor_handle, previous_iteration)
    {
        return anchor_handle.old_second_vertex();
    }

    inline void set_lead_dispatch(face_handle_t anchor_handle, first_side)
    {
        m_lead = get_first_vertex(anchor_handle, Time());
        set_edge_to_first_dispatch(anchor_handle, ValueType(), Time());
    }

    inline void set_lead_dispatch(face_handle_t anchor_handle, second_side)
    {
        m_lead = get_second_vertex(anchor_handle, Time());
        set_edge_to_second_dispatch(anchor_handle, ValueType(), Time());
    }

    inline void set_edge_to_first_dispatch(
        face_handle_t anchor_handle, edge_t, current_iteration)
    {
        m_edge.value = anchor_handle.first_edge();
    }

    inline void set_edge_to_second_dispatch(
        face_handle_t anchor_handle, edge_t, current_iteration)
    {
        m_edge.value = anchor_handle.second_edge();
    }

    inline void set_edge_to_first_dispatch(
        face_handle_t anchor_handle, edge_t, previous_iteration)
    {
        m_edge.value = anchor_handle.old_first_edge();
    }

    inline void set_edge_to_second_dispatch(
        face_handle_t anchor_handle, edge_t, previous_iteration)
    {
        m_edge.value = anchor_handle.old_second_edge();
    }

    template < typename T >
    inline void set_edge_to_first_dispatch(face_handle_t, vertex_t, T)
    {
    }

    template < typename T >
    inline void set_edge_to_second_dispatch(face_handle_t, vertex_t, T)
    {
    }

    void increment()
    {
        face_handle_t curr_face_handle(m_face_handles[m_lead]);
        vertex_t first = get_first_vertex(curr_face_handle, Time());
        vertex_t second = get_second_vertex(curr_face_handle, Time());
        if (first == m_follow)
        {
            m_follow = m_lead;
            set_edge_to_second_dispatch(curr_face_handle, ValueType(), Time());
            m_lead = second;
        }
        else if (second == m_follow)
        {
            m_follow = m_lead;
            set_edge_to_first_dispatch(curr_face_handle, ValueType(), Time());
            m_lead = first;
        }
        else
            m_lead = m_follow = graph_traits< Graph >::null_vertex();
    }

    bool equal(self const& other) const
    {
        return m_lead == other.m_lead && m_follow == other.m_follow;
    }

    ValueType dereference() const
    {
        return dereference_dispatch(VisitorType(), ValueType());
    }

    inline ValueType dereference_dispatch(lead_visitor, vertex_t) const
    {
        return m_lead;
    }

    inline ValueType dereference_dispatch(follow_visitor, vertex_t) const
    {
        return m_follow;
    }

    inline ValueType dereference_dispatch(lead_visitor, edge_t) const
    {
        return m_edge.value;
    }

    inline ValueType dereference_dispatch(follow_visitor, edge_t) const
    {
        return m_edge.value;
    }

    vertex_t m_lead;
    vertex_t m_follow;
    edge_storage< Graph, boost::is_same< ValueType, edge_t >::value > m_edge;
    FaceHandlesMap m_face_handles;
};

template < typename Graph, typename FaceHandlesMap, typename ValueType,
    typename VisitorType, typename Time >
class face_iterator< Graph, FaceHandlesMap, ValueType, both_sides, VisitorType,
    Time >
: public boost::iterator_facade< face_iterator< Graph, FaceHandlesMap,
                                     ValueType, both_sides, VisitorType, Time >,
      ValueType, boost::forward_traversal_tag, ValueType >
{
public:
    typedef face_iterator< Graph, FaceHandlesMap, ValueType, both_sides,
        VisitorType, Time >
        self;
    typedef typename graph_traits< Graph >::vertex_descriptor vertex_t;
    typedef typename FaceHandlesMap::value_type face_handle_t;

    face_iterator() {}

    face_iterator(face_handle_t anchor_handle, FaceHandlesMap face_handles)
    : first_itr(anchor_handle, face_handles, first_side())
    , second_itr(anchor_handle, face_handles, second_side())
    , first_is_active(true)
    , first_increment(true)
    {
    }

    face_iterator(vertex_t anchor, FaceHandlesMap face_handles)
    : first_itr(face_handles[anchor], face_handles, first_side())
    , second_itr(face_handles[anchor], face_handles, second_side())
    , first_is_active(true)
    , first_increment(true)
    {
    }

private:
    friend class boost::iterator_core_access;

    typedef face_iterator< Graph, FaceHandlesMap, ValueType, single_side,
        follow_visitor, Time >
        inner_itr_t;

    void increment()
    {
        if (first_increment)
        {
            ++first_itr;
            ++second_itr;
            first_increment = false;
        }
        else if (first_is_active)
            ++first_itr;
        else
            ++second_itr;
        first_is_active = !first_is_active;
    }

    bool equal(self const& other) const
    {
        // Want this iterator to be equal to the "end" iterator when at least
        // one of the iterators has reached the root of the current bicomp.
        // This isn't ideal, but it works.

        return (first_itr == other.first_itr || second_itr == other.second_itr);
    }

    ValueType dereference() const
    {
        return first_is_active ? *first_itr : *second_itr;
    }

    inner_itr_t first_itr;
    inner_itr_t second_itr;
    inner_itr_t face_end;
    bool first_is_active;
    bool first_increment;
};

} /* namespace boost */

#endif //__FACE_ITERATORS_HPP__
