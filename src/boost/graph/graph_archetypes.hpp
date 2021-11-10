//=======================================================================
// Copyright 2002 Indiana University.
// Authors: Andrew Lumsdaine, Lie-Quan Lee, Jeremy G. Siek
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#ifndef BOOST_GRAPH_ARCHETYPES_HPP
#define BOOST_GRAPH_ARCHETYPES_HPP

#include <boost/property_map/property_map.hpp>
#include <boost/concept_archetype.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/properties.hpp>

namespace boost
{ // should use a different namespace for this

namespace detail
{
    struct null_graph_archetype : public null_archetype<>
    {
        struct traversal_category
        {
        };
    };
}

//===========================================================================
template < typename Vertex, typename Directed, typename ParallelCategory,
    typename Base = detail::null_graph_archetype >
struct incidence_graph_archetype : public Base
{
    typedef typename Base::traversal_category base_trav_cat;
    struct traversal_category : public incidence_graph_tag, public base_trav_cat
    {
    };
#if 0
    typedef immutable_graph_tag mutability_category;
#endif
    typedef Vertex vertex_descriptor;
    typedef unsigned int degree_size_type;
    typedef unsigned int vertices_size_type;
    typedef unsigned int edges_size_type;
    struct edge_descriptor
    {
        edge_descriptor() {}
        edge_descriptor(const detail::dummy_constructor&) {}
        bool operator==(const edge_descriptor&) const { return false; }
        bool operator!=(const edge_descriptor&) const { return false; }
    };
    typedef input_iterator_archetype< edge_descriptor > out_edge_iterator;

    typedef Directed directed_category;
    typedef ParallelCategory edge_parallel_category;

    typedef void adjacency_iterator;
    typedef void in_edge_iterator;
    typedef void vertex_iterator;
    typedef void edge_iterator;

    static vertex_descriptor null_vertex() { return vertex_descriptor(); }
};
template < typename V, typename D, typename P, typename B >
V source(
    const typename incidence_graph_archetype< V, D, P, B >::edge_descriptor&,
    const incidence_graph_archetype< V, D, P, B >&)
{
    return V(static_object< detail::dummy_constructor >::get());
}
template < typename V, typename D, typename P, typename B >
V target(
    const typename incidence_graph_archetype< V, D, P, B >::edge_descriptor&,
    const incidence_graph_archetype< V, D, P, B >&)
{
    return V(static_object< detail::dummy_constructor >::get());
}

template < typename V, typename D, typename P, typename B >
std::pair< typename incidence_graph_archetype< V, D, P, B >::out_edge_iterator,
    typename incidence_graph_archetype< V, D, P, B >::out_edge_iterator >
out_edges(const V&, const incidence_graph_archetype< V, D, P, B >&)
{
    typedef typename incidence_graph_archetype< V, D, P, B >::out_edge_iterator
        Iter;
    return std::make_pair(Iter(), Iter());
}

template < typename V, typename D, typename P, typename B >
typename incidence_graph_archetype< V, D, P, B >::degree_size_type out_degree(
    const V&, const incidence_graph_archetype< V, D, P, B >&)
{
    return 0;
}

//===========================================================================
template < typename Vertex, typename Directed, typename ParallelCategory,
    typename Base = detail::null_graph_archetype >
struct adjacency_graph_archetype : public Base
{
    typedef typename Base::traversal_category base_trav_cat;
    struct traversal_category : public adjacency_graph_tag, public base_trav_cat
    {
    };
    typedef Vertex vertex_descriptor;
    typedef unsigned int degree_size_type;
    typedef unsigned int vertices_size_type;
    typedef unsigned int edges_size_type;
    typedef void edge_descriptor;
    typedef input_iterator_archetype< Vertex > adjacency_iterator;

    typedef Directed directed_category;
    typedef ParallelCategory edge_parallel_category;

    typedef void in_edge_iterator;
    typedef void out_edge_iterator;
    typedef void vertex_iterator;
    typedef void edge_iterator;

    static vertex_descriptor null_vertex() { return vertex_descriptor(); }
};

template < typename V, typename D, typename P, typename B >
std::pair< typename adjacency_graph_archetype< V, D, P, B >::adjacency_iterator,
    typename adjacency_graph_archetype< V, D, P, B >::adjacency_iterator >
adjacent_vertices(const V&, const adjacency_graph_archetype< V, D, P, B >&)
{
    typedef typename adjacency_graph_archetype< V, D, P, B >::adjacency_iterator
        Iter;
    return std::make_pair(Iter(), Iter());
}

template < typename V, typename D, typename P, typename B >
typename adjacency_graph_archetype< V, D, P, B >::degree_size_type out_degree(
    const V&, const adjacency_graph_archetype< V, D, P, B >&)
{
    return 0;
}

//===========================================================================
template < typename Vertex, typename Directed, typename ParallelCategory,
    typename Base = detail::null_graph_archetype >
struct vertex_list_graph_archetype : public Base
{
    typedef incidence_graph_archetype< Vertex, Directed, ParallelCategory >
        Incidence;
    typedef adjacency_graph_archetype< Vertex, Directed, ParallelCategory >
        Adjacency;

    typedef typename Base::traversal_category base_trav_cat;
    struct traversal_category : public vertex_list_graph_tag,
                                public base_trav_cat
    {
    };
#if 0
    typedef immutable_graph_tag mutability_category;
#endif
    typedef Vertex vertex_descriptor;
    typedef unsigned int degree_size_type;
    typedef typename Incidence::edge_descriptor edge_descriptor;
    typedef typename Incidence::out_edge_iterator out_edge_iterator;
    typedef typename Adjacency::adjacency_iterator adjacency_iterator;

    typedef input_iterator_archetype< Vertex > vertex_iterator;
    typedef unsigned int vertices_size_type;
    typedef unsigned int edges_size_type;

    typedef Directed directed_category;
    typedef ParallelCategory edge_parallel_category;

    typedef void in_edge_iterator;
    typedef void edge_iterator;

    static vertex_descriptor null_vertex() { return vertex_descriptor(); }
};

template < typename V, typename D, typename P, typename B >
std::pair< typename vertex_list_graph_archetype< V, D, P, B >::vertex_iterator,
    typename vertex_list_graph_archetype< V, D, P, B >::vertex_iterator >
vertices(const vertex_list_graph_archetype< V, D, P, B >&)
{
    typedef typename vertex_list_graph_archetype< V, D, P, B >::vertex_iterator
        Iter;
    return std::make_pair(Iter(), Iter());
}

template < typename V, typename D, typename P, typename B >
typename vertex_list_graph_archetype< V, D, P, B >::vertices_size_type
num_vertices(const vertex_list_graph_archetype< V, D, P, B >&)
{
    return 0;
}

// ambiguously inherited from incidence graph and adjacency graph
template < typename V, typename D, typename P, typename B >
typename vertex_list_graph_archetype< V, D, P, B >::degree_size_type out_degree(
    const V&, const vertex_list_graph_archetype< V, D, P, B >&)
{
    return 0;
}

//===========================================================================

struct property_graph_archetype_tag
{
};

template < typename GraphArchetype, typename Property, typename ValueArch >
struct property_graph_archetype : public GraphArchetype
{
    typedef property_graph_archetype_tag graph_tag;
    typedef ValueArch vertex_property_type;
    typedef ValueArch edge_property_type;
};

struct choose_edge_property_map_archetype
{
    template < typename Graph, typename Property, typename Tag > struct bind_
    {
        typedef mutable_lvalue_property_map_archetype<
            typename Graph::edge_descriptor, Property >
            type;
        typedef lvalue_property_map_archetype< typename Graph::edge_descriptor,
            Property >
            const_type;
    };
};
template <> struct edge_property_selector< property_graph_archetype_tag >
{
    typedef choose_edge_property_map_archetype type;
};

struct choose_vertex_property_map_archetype
{
    template < typename Graph, typename Property, typename Tag > struct bind_
    {
        typedef mutable_lvalue_property_map_archetype<
            typename Graph::vertex_descriptor, Property >
            type;
        typedef lvalue_property_map_archetype<
            typename Graph::vertex_descriptor, Property >
            const_type;
    };
};

template <> struct vertex_property_selector< property_graph_archetype_tag >
{
    typedef choose_vertex_property_map_archetype type;
};

template < typename G, typename P, typename V >
typename property_map< property_graph_archetype< G, P, V >, P >::type get(
    P, property_graph_archetype< G, P, V >&)
{
    typename property_map< property_graph_archetype< G, P, V >, P >::type pmap;
    return pmap;
}

template < typename G, typename P, typename V >
typename property_map< property_graph_archetype< G, P, V >, P >::const_type get(
    P, const property_graph_archetype< G, P, V >&)
{
    typename property_map< property_graph_archetype< G, P, V >, P >::const_type
        pmap;
    return pmap;
}

template < typename G, typename P, typename K, typename V >
typename property_traits< typename property_map<
    property_graph_archetype< G, P, V >, P >::const_type >::value_type
get(P p, const property_graph_archetype< G, P, V >& g, K k)
{
    return get(get(p, g), k);
}

template < typename G, typename P, typename V, typename Key >
void put(
    P p, property_graph_archetype< G, P, V >& g, const Key& key, const V& value)
{
    typedef typename boost::property_map< property_graph_archetype< G, P, V >,
        P >::type Map;
    Map pmap = get(p, g);
    put(pmap, key, value);
}

struct color_value_archetype
{
    color_value_archetype() {}
    color_value_archetype(detail::dummy_constructor) {}
    bool operator==(const color_value_archetype&) const { return true; }
    bool operator!=(const color_value_archetype&) const { return true; }
};
template <> struct color_traits< color_value_archetype >
{
    static color_value_archetype white()
    {
        return color_value_archetype(
            static_object< detail::dummy_constructor >::get());
    }
    static color_value_archetype gray()
    {
        return color_value_archetype(
            static_object< detail::dummy_constructor >::get());
    }
    static color_value_archetype black()
    {
        return color_value_archetype(
            static_object< detail::dummy_constructor >::get());
    }
};

template < typename T > class buffer_archetype
{
public:
    void push(const T&) {}
    void pop() {}
    T& top() { return static_object< T >::get(); }
    const T& top() const { return static_object< T >::get(); }
    bool empty() const { return true; }
};

} // namespace boost

#endif // BOOST_GRAPH_ARCHETYPES_HPP
