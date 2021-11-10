//=======================================================================
// Copyright (c) Aaron Windsor 2007
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#ifndef __PLANAR_FACE_TRAVERSAL_HPP__
#define __PLANAR_FACE_TRAVERSAL_HPP__

#include <vector>
#include <set>
#include <map>
#include <boost/next_prior.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/properties.hpp>

namespace boost
{

struct planar_face_traversal_visitor
{
    void begin_traversal() {}

    void begin_face() {}

    template < typename Edge > void next_edge(Edge) {}

    template < typename Vertex > void next_vertex(Vertex) {}

    void end_face() {}

    void end_traversal() {}
};

template < typename Graph, typename PlanarEmbedding, typename Visitor,
    typename EdgeIndexMap >
void planar_face_traversal(const Graph& g, PlanarEmbedding embedding,
    Visitor& visitor, EdgeIndexMap em)
{
    typedef typename graph_traits< Graph >::vertex_descriptor vertex_t;
    typedef typename graph_traits< Graph >::edge_descriptor edge_t;
    typedef typename graph_traits< Graph >::vertex_iterator vertex_iterator_t;
    typedef typename graph_traits< Graph >::edge_iterator edge_iterator_t;
    typedef typename property_traits< PlanarEmbedding >::value_type
        embedding_value_t;
    typedef typename embedding_value_t::const_iterator embedding_iterator_t;

    typedef typename std::vector< std::set< vertex_t > >
        distinguished_edge_storage_t;
    typedef typename std::vector< std::map< vertex_t, edge_t > >
        distinguished_edge_to_edge_storage_t;

    typedef typename boost::iterator_property_map<
        typename distinguished_edge_storage_t::iterator, EdgeIndexMap >
        distinguished_edge_map_t;

    typedef typename boost::iterator_property_map<
        typename distinguished_edge_to_edge_storage_t::iterator, EdgeIndexMap >
        distinguished_edge_to_edge_map_t;

    distinguished_edge_storage_t visited_vector(num_edges(g));
    distinguished_edge_to_edge_storage_t next_edge_vector(num_edges(g));

    distinguished_edge_map_t visited(visited_vector.begin(), em);
    distinguished_edge_to_edge_map_t next_edge(next_edge_vector.begin(), em);

    vertex_iterator_t vi, vi_end;
    typename std::vector< edge_t >::iterator ei, ei_end;
    edge_iterator_t fi, fi_end;
    embedding_iterator_t pi, pi_begin, pi_end;

    visitor.begin_traversal();

    // Initialize the next_edge property map. This map is initialized from the
    // PlanarEmbedding so that get(next_edge, e)[v] is the edge that comes
    // after e in the clockwise embedding around vertex v.

    for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi)
    {
        vertex_t v(*vi);
        pi_begin = embedding[v].begin();
        pi_end = embedding[v].end();
        for (pi = pi_begin; pi != pi_end; ++pi)
        {
            edge_t e(*pi);
            std::map< vertex_t, edge_t > m = get(next_edge, e);
            m[v] = boost::next(pi) == pi_end ? *pi_begin : *boost::next(pi);
            put(next_edge, e, m);
        }
    }

    // Take a copy of the edges in the graph here, since we want to accomodate
    // face traversals that add edges to the graph (for triangulation, in
    // particular) and don't want to use invalidated edge iterators.
    // Also, while iterating over all edges in the graph, we single out
    // any self-loops, which need some special treatment in the face traversal.

    std::vector< edge_t > self_loops;
    std::vector< edge_t > edges_cache;
    std::vector< vertex_t > vertices_in_edge;

    for (boost::tie(fi, fi_end) = edges(g); fi != fi_end; ++fi)
    {
        edge_t e(*fi);
        edges_cache.push_back(e);
        if (source(e, g) == target(e, g))
            self_loops.push_back(e);
    }

    // Iterate over all edges in the graph
    ei_end = edges_cache.end();
    for (ei = edges_cache.begin(); ei != ei_end; ++ei)
    {

        edge_t e(*ei);
        vertices_in_edge.clear();
        vertices_in_edge.push_back(source(e, g));
        vertices_in_edge.push_back(target(e, g));

        typename std::vector< vertex_t >::iterator vi, vi_end;
        vi_end = vertices_in_edge.end();

        // Iterate over both vertices in the current edge
        for (vi = vertices_in_edge.begin(); vi != vi_end; ++vi)
        {

            vertex_t v(*vi);
            std::set< vertex_t > e_visited = get(visited, e);
            typename std::set< vertex_t >::iterator e_visited_found
                = e_visited.find(v);

            if (e_visited_found == e_visited.end())
                visitor.begin_face();

            while (e_visited.find(v) == e_visited.end())
            {
                visitor.next_vertex(v);
                visitor.next_edge(e);
                e_visited.insert(v);
                put(visited, e, e_visited);
                v = source(e, g) == v ? target(e, g) : source(e, g);
                e = get(next_edge, e)[v];
                e_visited = get(visited, e);
            }

            if (e_visited_found == e_visited.end())
                visitor.end_face();
        }
    }

    // Iterate over all self-loops, visiting them once separately
    // (they've already been visited once, this visitation is for
    // the "inside" of the self-loop)

    ei_end = self_loops.end();
    for (ei = self_loops.begin(); ei != ei_end; ++ei)
    {
        visitor.begin_face();
        visitor.next_edge(*ei);
        visitor.next_vertex(source(*ei, g));
        visitor.end_face();
    }

    visitor.end_traversal();
}

template < typename Graph, typename PlanarEmbedding, typename Visitor >
inline void planar_face_traversal(
    const Graph& g, PlanarEmbedding embedding, Visitor& visitor)
{
    planar_face_traversal(g, embedding, visitor, get(edge_index, g));
}

} // namespace boost

#endif //__PLANAR_FACE_TRAVERSAL_HPP__
