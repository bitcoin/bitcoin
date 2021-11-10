//=======================================================================
// Copyright 2007 Aaron Windsor
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================
#ifndef __MAKE_MAXIMAL_PLANAR_HPP__
#define __MAKE_MAXIMAL_PLANAR_HPP__

#include <boost/config.hpp>
#include <boost/tuple/tuple.hpp> //for tie
#include <boost/graph/biconnected_components.hpp>
#include <boost/property_map/property_map.hpp>
#include <vector>
#include <iterator>
#include <algorithm>

#include <boost/graph/planar_face_traversal.hpp>
#include <boost/graph/planar_detail/add_edge_visitors.hpp>

namespace boost
{

template < typename Graph, typename VertexIndexMap, typename AddEdgeVisitor >
struct triangulation_visitor : public planar_face_traversal_visitor
{

    typedef typename graph_traits< Graph >::vertex_descriptor vertex_t;
    typedef typename graph_traits< Graph >::edge_descriptor edge_t;
    typedef typename graph_traits< Graph >::vertices_size_type v_size_t;
    typedef typename graph_traits< Graph >::degree_size_type degree_size_t;
    typedef typename graph_traits< Graph >::edge_iterator edge_iterator_t;
    typedef typename graph_traits< Graph >::vertex_iterator vertex_iterator_t;
    typedef
        typename graph_traits< Graph >::adjacency_iterator adjacency_iterator_t;
    typedef typename std::vector< vertex_t > vertex_vector_t;
    typedef typename std::vector< v_size_t > v_size_vector_t;
    typedef typename std::vector< degree_size_t > degree_size_vector_t;
    typedef iterator_property_map< typename v_size_vector_t::iterator,
        VertexIndexMap >
        vertex_to_v_size_map_t;
    typedef iterator_property_map< typename degree_size_vector_t::iterator,
        VertexIndexMap >
        vertex_to_degree_size_map_t;
    typedef typename vertex_vector_t::iterator face_iterator;

    triangulation_visitor(Graph& arg_g, VertexIndexMap arg_vm,
        AddEdgeVisitor arg_add_edge_visitor)
    : g(arg_g)
    , vm(arg_vm)
    , add_edge_visitor(arg_add_edge_visitor)
    , timestamp(0)
    , marked_vector(num_vertices(g), timestamp)
    , degree_vector(num_vertices(g), 0)
    , marked(marked_vector.begin(), vm)
    , degree(degree_vector.begin(), vm)
    {
        vertex_iterator_t vi, vi_end;
        for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi)
            put(degree, *vi, out_degree(*vi, g));
    }

    template < typename Vertex > void next_vertex(Vertex v)
    {
        // Self-loops will appear as consecutive vertices in the list of
        // vertices on a face. We want to skip these.
        if (!vertices_on_face.empty()
            && (vertices_on_face.back() == v || vertices_on_face.front() == v))
            return;

        vertices_on_face.push_back(v);
    }

    void end_face()
    {
        ++timestamp;

        if (vertices_on_face.size() <= 3)
        {
            // At most three vertices on this face - don't need to triangulate
            vertices_on_face.clear();
            return;
        }

        // Find vertex on face of minimum degree
        degree_size_t min_degree = num_vertices(g);
        typename vertex_vector_t::iterator min_degree_vertex_itr;
        face_iterator fi_end = vertices_on_face.end();
        for (face_iterator fi = vertices_on_face.begin(); fi != fi_end; ++fi)
        {
            degree_size_t deg = get(degree, *fi);
            if (deg < min_degree)
            {
                min_degree_vertex_itr = fi;
                min_degree = deg;
            }
        }

        // To simplify some of the manipulations, we'll re-arrange
        // vertices_on_face so that it still contains the same
        // (counter-clockwise) order of the vertices on this face, but now the
        // min_degree_vertex is the first element in vertices_on_face.
        vertex_vector_t temp_vector;
        std::copy(min_degree_vertex_itr, vertices_on_face.end(),
            std::back_inserter(temp_vector));
        std::copy(vertices_on_face.begin(), min_degree_vertex_itr,
            std::back_inserter(temp_vector));
        vertices_on_face.swap(temp_vector);

        // Mark all of the min degree vertex's neighbors
        adjacency_iterator_t ai, ai_end;
        for (boost::tie(ai, ai_end)
             = adjacent_vertices(vertices_on_face.front(), g);
             ai != ai_end; ++ai)
        {
            put(marked, *ai, timestamp);
        }

        typename vertex_vector_t::iterator marked_neighbor
            = vertices_on_face.end();

        // The iterator manipulations on the next two lines are safe because
        // vertices_on_face.size() > 3 (from the first test in this function)
        fi_end = prior(vertices_on_face.end());
        for (face_iterator fi
             = boost::next(boost::next(vertices_on_face.begin()));
             fi != fi_end; ++fi)
        {
            if (get(marked, *fi) == timestamp)
            {
                marked_neighbor = fi;
                break;
            }
        }

        if (marked_neighbor == vertices_on_face.end())
        {
            add_edge_range(vertices_on_face[0],
                boost::next(boost::next(vertices_on_face.begin())),
                prior(vertices_on_face.end()));
        }
        else
        {
            add_edge_range(vertices_on_face[1], boost::next(marked_neighbor),
                vertices_on_face.end());

            add_edge_range(*boost::next(marked_neighbor),
                boost::next(boost::next(vertices_on_face.begin())),
                marked_neighbor);
        }

        // reset for the next face
        vertices_on_face.clear();
    }

private:
    void add_edge_range(vertex_t anchor, face_iterator fi, face_iterator fi_end)
    {
        for (; fi != fi_end; ++fi)
        {
            vertex_t v(*fi);
            add_edge_visitor.visit_vertex_pair(anchor, v, g);
            put(degree, anchor, get(degree, anchor) + 1);
            put(degree, v, get(degree, v) + 1);
        }
    }

    Graph& g;
    VertexIndexMap vm;
    AddEdgeVisitor add_edge_visitor;
    v_size_t timestamp;
    vertex_vector_t vertices_on_face;
    v_size_vector_t marked_vector;
    degree_size_vector_t degree_vector;
    vertex_to_v_size_map_t marked;
    vertex_to_degree_size_map_t degree;
};

template < typename Graph, typename PlanarEmbedding, typename VertexIndexMap,
    typename EdgeIndexMap, typename AddEdgeVisitor >
void make_maximal_planar(Graph& g, PlanarEmbedding embedding, VertexIndexMap vm,
    EdgeIndexMap em, AddEdgeVisitor& vis)
{
    triangulation_visitor< Graph, VertexIndexMap, AddEdgeVisitor > visitor(
        g, vm, vis);
    planar_face_traversal(g, embedding, visitor, em);
}

template < typename Graph, typename PlanarEmbedding, typename VertexIndexMap,
    typename EdgeIndexMap >
void make_maximal_planar(
    Graph& g, PlanarEmbedding embedding, VertexIndexMap vm, EdgeIndexMap em)
{
    default_add_edge_visitor vis;
    make_maximal_planar(g, embedding, vm, em, vis);
}

template < typename Graph, typename PlanarEmbedding, typename VertexIndexMap >
void make_maximal_planar(Graph& g, PlanarEmbedding embedding, VertexIndexMap vm)
{
    make_maximal_planar(g, embedding, vm, get(edge_index, g));
}

template < typename Graph, typename PlanarEmbedding >
void make_maximal_planar(Graph& g, PlanarEmbedding embedding)
{
    make_maximal_planar(g, embedding, get(vertex_index, g));
}

} // namespace boost

#endif //__MAKE_MAXIMAL_PLANAR_HPP__
