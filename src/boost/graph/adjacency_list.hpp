//=======================================================================
// Copyright 1997, 1998, 1999, 2000 University of Notre Dame.
// Copyright 2010 Thomas Claveirole
// Authors: Andrew Lumsdaine, Lie-Quan Lee, Jeremy G. Siek, Thomas Claveirole
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#ifndef BOOST_GRAPH_ADJACENCY_LIST_HPP
#define BOOST_GRAPH_ADJACENCY_LIST_HPP

#include <boost/config.hpp>

#include <vector>
#include <list>
#include <set>

#include <boost/unordered_set.hpp>

#include <boost/scoped_ptr.hpp>

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/graph_mutability_traits.hpp>
#include <boost/graph/graph_selectors.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/and.hpp>
#include <boost/mpl/not.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/graph/detail/edge.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/detail/workaround.hpp>
#include <boost/graph/properties.hpp>
#include <boost/graph/named_graph.hpp>

namespace boost
{

//===========================================================================
// Selectors for the VertexList and EdgeList template parameters of
// adjacency_list, and the container_gen traits class which is used
// to map the selectors to the container type used to implement the
// graph.

struct vecS
{
};
struct listS
{
};
struct setS
{
};
struct mapS
{
};
struct multisetS
{
};
struct multimapS
{
};
struct hash_setS
{
};
struct hash_mapS
{
};
struct hash_multisetS
{
};
struct hash_multimapS
{
};

template < class Selector, class ValueType > struct container_gen
{
};

template < class ValueType > struct container_gen< listS, ValueType >
{
    typedef std::list< ValueType > type;
};

template < class ValueType > struct container_gen< vecS, ValueType >
{
    typedef std::vector< ValueType > type;
};

template < class ValueType > struct container_gen< mapS, ValueType >
{
    typedef std::set< ValueType > type;
};

template < class ValueType > struct container_gen< setS, ValueType >
{
    typedef std::set< ValueType > type;
};

template < class ValueType > struct container_gen< multisetS, ValueType >
{
    typedef std::multiset< ValueType > type;
};

template < class ValueType > struct container_gen< multimapS, ValueType >
{
    typedef std::multiset< ValueType > type;
};

template < class ValueType > struct container_gen< hash_setS, ValueType >
{
    typedef boost::unordered_set< ValueType > type;
};

template < class ValueType > struct container_gen< hash_mapS, ValueType >
{
    typedef boost::unordered_set< ValueType > type;
};

template < class ValueType > struct container_gen< hash_multisetS, ValueType >
{
    typedef boost::unordered_multiset< ValueType > type;
};

template < class ValueType > struct container_gen< hash_multimapS, ValueType >
{
    typedef boost::unordered_multiset< ValueType > type;
};

template < class StorageSelector > struct parallel_edge_traits
{
};

template <> struct parallel_edge_traits< vecS >
{
    typedef allow_parallel_edge_tag type;
};

template <> struct parallel_edge_traits< listS >
{
    typedef allow_parallel_edge_tag type;
};

template <> struct parallel_edge_traits< setS >
{
    typedef disallow_parallel_edge_tag type;
};

template <> struct parallel_edge_traits< multisetS >
{
    typedef allow_parallel_edge_tag type;
};

template <> struct parallel_edge_traits< hash_setS >
{
    typedef disallow_parallel_edge_tag type;
};

// mapS is obsolete, replaced with setS
template <> struct parallel_edge_traits< mapS >
{
    typedef disallow_parallel_edge_tag type;
};

template <> struct parallel_edge_traits< hash_mapS >
{
    typedef disallow_parallel_edge_tag type;
};

template <> struct parallel_edge_traits< hash_multisetS >
{
    typedef allow_parallel_edge_tag type;
};

template <> struct parallel_edge_traits< hash_multimapS >
{
    typedef allow_parallel_edge_tag type;
};

namespace detail
{
    template < class Directed > struct is_random_access
    {
        enum
        {
            value = false
        };
        typedef mpl::false_ type;
    };
    template <> struct is_random_access< vecS >
    {
        enum
        {
            value = true
        };
        typedef mpl::true_ type;
    };

} // namespace detail

template < typename Selector > struct is_distributed_selector : mpl::false_
{
};

//===========================================================================
// The adjacency_list_traits class, which provides a way to access
// some of the associated types of an adjacency_list type without
// having to first create the adjacency_list type. This is useful
// when trying to create interior vertex or edge properties who's
// value type is a vertex or edge descriptor.

template < class OutEdgeListS = vecS, class VertexListS = vecS,
    class DirectedS = directedS, class EdgeListS = listS >
struct adjacency_list_traits
{
    typedef
        typename detail::is_random_access< VertexListS >::type is_rand_access;
    typedef typename DirectedS::is_bidir_t is_bidir;
    typedef typename DirectedS::is_directed_t is_directed;

    typedef typename mpl::if_< is_bidir, bidirectional_tag,
        typename mpl::if_< is_directed, directed_tag,
            undirected_tag >::type >::type directed_category;

    typedef typename parallel_edge_traits< OutEdgeListS >::type
        edge_parallel_category;

    typedef std::size_t vertices_size_type;
    typedef void* vertex_ptr;
    typedef typename mpl::if_< is_rand_access, vertices_size_type,
        vertex_ptr >::type vertex_descriptor;
    typedef detail::edge_desc_impl< directed_category, vertex_descriptor >
        edge_descriptor;

private:
    // Logic to figure out the edges_size_type
    struct dummy
    {
    };
    typedef typename container_gen< EdgeListS, dummy >::type EdgeContainer;
    typedef typename DirectedS::is_bidir_t BidirectionalT;
    typedef typename DirectedS::is_directed_t DirectedT;
    typedef typename mpl::and_< DirectedT,
        typename mpl::not_< BidirectionalT >::type >::type on_edge_storage;

public:
    typedef typename mpl::if_< on_edge_storage, std::size_t,
        typename EdgeContainer::size_type >::type edges_size_type;
};

} // namespace boost

#include <boost/graph/detail/adjacency_list.hpp>

namespace boost
{

//===========================================================================
// The adjacency_list class.
//

template < class OutEdgeListS = vecS, // a Sequence or an AssociativeContainer
    class VertexListS = vecS, // a Sequence or a RandomAccessContainer
    class DirectedS = directedS, class VertexProperty = no_property,
    class EdgeProperty = no_property, class GraphProperty = no_property,
    class EdgeListS = listS >
class adjacency_list
: public detail::adj_list_gen<
      adjacency_list< OutEdgeListS, VertexListS, DirectedS, VertexProperty,
          EdgeProperty, GraphProperty, EdgeListS >,
      VertexListS, OutEdgeListS, DirectedS, VertexProperty, EdgeProperty,
      GraphProperty, EdgeListS >::type,
  // Support for named vertices
  public graph::maybe_named_graph<
      adjacency_list< OutEdgeListS, VertexListS, DirectedS, VertexProperty,
          EdgeProperty, GraphProperty, EdgeListS >,
      typename adjacency_list_traits< OutEdgeListS, VertexListS, DirectedS,
          EdgeListS >::vertex_descriptor,
      VertexProperty >
{
public:
    typedef GraphProperty graph_property_type;
    typedef typename lookup_one_property< GraphProperty, graph_bundle_t >::type
        graph_bundled;

    typedef VertexProperty vertex_property_type;
    typedef
        typename lookup_one_property< VertexProperty, vertex_bundle_t >::type
            vertex_bundled;

    typedef EdgeProperty edge_property_type;
    typedef typename lookup_one_property< EdgeProperty, edge_bundle_t >::type
        edge_bundled;

private:
    typedef adjacency_list self;
    typedef typename detail::adj_list_gen< self, VertexListS, OutEdgeListS,
        DirectedS, vertex_property_type, edge_property_type, GraphProperty,
        EdgeListS >::type Base;

public:
    typedef typename Base::stored_vertex stored_vertex;
    typedef typename Base::vertices_size_type vertices_size_type;
    typedef typename Base::edges_size_type edges_size_type;
    typedef typename Base::degree_size_type degree_size_type;
    typedef typename Base::vertex_descriptor vertex_descriptor;
    typedef typename Base::edge_descriptor edge_descriptor;
    typedef OutEdgeListS out_edge_list_selector;
    typedef VertexListS vertex_list_selector;
    typedef DirectedS directed_selector;
    typedef EdgeListS edge_list_selector;

    adjacency_list(const GraphProperty& p = GraphProperty())
    : m_property(new graph_property_type(p))
    {
    }

    adjacency_list(const adjacency_list& x)
    : Base(x), m_property(new graph_property_type(*x.m_property))
    {
    }

    adjacency_list& operator=(const adjacency_list& x)
    {
        // TBD: probably should give the strong guarantee
        if (&x != this)
        {
            Base::operator=(x);

            // Copy/swap the ptr since we can't just assign it...
            property_ptr p(new graph_property_type(*x.m_property));
            m_property.swap(p);
        }
        return *this;
    }

    // Required by Mutable Graph
    adjacency_list(vertices_size_type num_vertices,
        const GraphProperty& p = GraphProperty())
    : Base(num_vertices), m_property(new graph_property_type(p))
    {
    }

    // Required by Iterator Constructible Graph
    template < class EdgeIterator >
    adjacency_list(EdgeIterator first, EdgeIterator last, vertices_size_type n,
        edges_size_type = 0, const GraphProperty& p = GraphProperty())
    : Base(n, first, last), m_property(new graph_property_type(p))
    {
    }

    template < class EdgeIterator, class EdgePropertyIterator >
    adjacency_list(EdgeIterator first, EdgeIterator last,
        EdgePropertyIterator ep_iter, vertices_size_type n, edges_size_type = 0,
        const GraphProperty& p = GraphProperty())
    : Base(n, first, last, ep_iter), m_property(new graph_property_type(p))
    {
    }

    void swap(adjacency_list& x)
    {
        // Is there a more efficient way to do this?
        adjacency_list tmp(x);
        x = *this;
        *this = tmp;
    }

    void clear()
    {
        this->clearing_graph();
        Base::clear();
    }

#ifndef BOOST_GRAPH_NO_BUNDLED_PROPERTIES
    // Directly access a vertex or edge bundle
    vertex_bundled& operator[](vertex_descriptor v)
    {
        return get(vertex_bundle, *this)[v];
    }

    const vertex_bundled& operator[](vertex_descriptor v) const
    {
        return get(vertex_bundle, *this)[v];
    }

    edge_bundled& operator[](edge_descriptor e)
    {
        return get(edge_bundle, *this)[e];
    }

    const edge_bundled& operator[](edge_descriptor e) const
    {
        return get(edge_bundle, *this)[e];
    }

    graph_bundled& operator[](graph_bundle_t) { return get_property(*this); }

    graph_bundled const& operator[](graph_bundle_t) const
    {
        return get_property(*this);
    }
#endif

    //  protected:  (would be protected if friends were more portable)
    typedef scoped_ptr< graph_property_type > property_ptr;
    property_ptr m_property;
};

#define ADJLIST_PARAMS                                               \
    typename OEL, typename VL, typename D, typename VP, typename EP, \
        typename GP, typename EL
#define ADJLIST adjacency_list< OEL, VL, D, VP, EP, GP, EL >

template < ADJLIST_PARAMS, typename Tag, typename Value >
inline void set_property(ADJLIST& g, Tag tag, Value const& value)
{
    get_property_value(*g.m_property, tag) = value;
}

template < ADJLIST_PARAMS, typename Tag >
inline typename graph_property< ADJLIST, Tag >::type& get_property(
    ADJLIST& g, Tag tag)
{
    return get_property_value(*g.m_property, tag);
}

template < ADJLIST_PARAMS, typename Tag >
inline typename graph_property< ADJLIST, Tag >::type const& get_property(
    ADJLIST const& g, Tag tag)
{
    return get_property_value(*g.m_property, tag);
}

// dwa 09/25/00 - needed to be more explicit so reverse_graph would work.
template < class Directed, class Vertex, class OutEdgeListS, class VertexListS,
    class DirectedS, class VertexProperty, class EdgeProperty,
    class GraphProperty, class EdgeListS >
inline Vertex source(const detail::edge_base< Directed, Vertex >& e,
    const adjacency_list< OutEdgeListS, VertexListS, DirectedS, VertexProperty,
        EdgeProperty, GraphProperty, EdgeListS >&)
{
    return e.m_source;
}

template < class Directed, class Vertex, class OutEdgeListS, class VertexListS,
    class DirectedS, class VertexProperty, class EdgeProperty,
    class GraphProperty, class EdgeListS >
inline Vertex target(const detail::edge_base< Directed, Vertex >& e,
    const adjacency_list< OutEdgeListS, VertexListS, DirectedS, VertexProperty,
        EdgeProperty, GraphProperty, EdgeListS >&)
{
    return e.m_target;
}

// Mutability Traits
template < ADJLIST_PARAMS > struct graph_mutability_traits< ADJLIST >
{
    typedef mutable_property_graph_tag category;
};

// Can't remove vertices from adjacency lists with VL==vecS
template < typename OEL, typename D, typename VP, typename EP, typename GP,
    typename EL >
struct graph_mutability_traits< adjacency_list< OEL, vecS, D, VP, EP, GP, EL > >
{
    typedef add_only_property_graph_tag category;
};
#undef ADJLIST_PARAMS
#undef ADJLIST

} // namespace boost

#endif // BOOST_GRAPH_ADJACENCY_LIST_HPP
