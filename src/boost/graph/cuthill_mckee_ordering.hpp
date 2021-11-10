//=======================================================================
// Copyright 1997, 1998, 1999, 2000 University of Notre Dame.
// Copyright 2004, 2005 Trustees of Indiana University
// Authors: Andrew Lumsdaine, Lie-Quan Lee, Jeremy G. Siek,
//          Doug Gregor, D. Kevin McGrath
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================
#ifndef BOOST_GRAPH_CUTHILL_MCKEE_HPP
#define BOOST_GRAPH_CUTHILL_MCKEE_HPP

#include <boost/config.hpp>
#include <boost/graph/detail/sparse_ordering.hpp>
#include <boost/graph/graph_utility.hpp>
#include <algorithm>

/*
  (Reverse) Cuthill-McKee Algorithm for matrix reordering
*/

namespace boost
{

namespace detail
{

    template < typename OutputIterator, typename Buffer, typename DegreeMap >
    class bfs_rcm_visitor : public default_bfs_visitor
    {
    public:
        bfs_rcm_visitor(OutputIterator* iter, Buffer* b, DegreeMap deg)
        : permutation(iter), Qptr(b), degree(deg)
        {
        }
        template < class Vertex, class Graph >
        void examine_vertex(Vertex u, Graph&)
        {
            *(*permutation)++ = u;
            index_begin = Qptr->size();
        }
        template < class Vertex, class Graph >
        void finish_vertex(Vertex, Graph&)
        {
            using std::sort;

            typedef typename property_traits< DegreeMap >::value_type ds_type;

            typedef indirect_cmp< DegreeMap, std::less< ds_type > > Compare;
            Compare comp(degree);

            sort(Qptr->begin() + index_begin, Qptr->end(), comp);
        }

    protected:
        OutputIterator* permutation;
        int index_begin;
        Buffer* Qptr;
        DegreeMap degree;
    };

} // namespace detail

// Reverse Cuthill-McKee algorithm with a given starting Vertex.
//
// If user provides a reverse iterator, this will be a reverse-cuthill-mckee
// algorithm, otherwise it will be a standard CM algorithm

template < class Graph, class OutputIterator, class ColorMap, class DegreeMap >
OutputIterator cuthill_mckee_ordering(const Graph& g,
    std::deque< typename graph_traits< Graph >::vertex_descriptor >
        vertex_queue,
    OutputIterator permutation, ColorMap color, DegreeMap degree)
{

    // create queue, visitor...don't forget namespaces!
    typedef typename graph_traits< Graph >::vertex_descriptor Vertex;
    typedef typename boost::sparse::sparse_ordering_queue< Vertex > queue;
    typedef typename detail::bfs_rcm_visitor< OutputIterator, queue, DegreeMap >
        Visitor;
    typedef typename property_traits< ColorMap >::value_type ColorValue;
    typedef color_traits< ColorValue > Color;

    queue Q;

    // create a bfs_rcm_visitor as defined above
    Visitor vis(&permutation, &Q, degree);

    typename graph_traits< Graph >::vertex_iterator ui, ui_end;

    // Copy degree to pseudo_degree
    // initialize the color map
    for (boost::tie(ui, ui_end) = vertices(g); ui != ui_end; ++ui)
    {
        put(color, *ui, Color::white());
    }

    while (!vertex_queue.empty())
    {
        Vertex s = vertex_queue.front();
        vertex_queue.pop_front();

        // call BFS with visitor
        breadth_first_visit(g, s, Q, vis, color);
    }
    return permutation;
}

// This is the case where only a single starting vertex is supplied.
template < class Graph, class OutputIterator, class ColorMap, class DegreeMap >
OutputIterator cuthill_mckee_ordering(const Graph& g,
    typename graph_traits< Graph >::vertex_descriptor s,
    OutputIterator permutation, ColorMap color, DegreeMap degree)
{

    std::deque< typename graph_traits< Graph >::vertex_descriptor >
        vertex_queue;
    vertex_queue.push_front(s);

    return cuthill_mckee_ordering(g, vertex_queue, permutation, color, degree);
}

// This is the version of CM which selects its own starting vertex
template < class Graph, class OutputIterator, class ColorMap, class DegreeMap >
OutputIterator cuthill_mckee_ordering(const Graph& G,
    OutputIterator permutation, ColorMap color, DegreeMap degree)
{
    if (boost::graph::has_no_vertices(G))
        return permutation;

    typedef typename boost::graph_traits< Graph >::vertex_descriptor Vertex;
    typedef typename property_traits< ColorMap >::value_type ColorValue;
    typedef color_traits< ColorValue > Color;

    std::deque< Vertex > vertex_queue;

    // Mark everything white
    BGL_FORALL_VERTICES_T(v, G, Graph) put(color, v, Color::white());

    // Find one vertex from each connected component
    BGL_FORALL_VERTICES_T(v, G, Graph)
    {
        if (get(color, v) == Color::white())
        {
            depth_first_visit(G, v, dfs_visitor<>(), color);
            vertex_queue.push_back(v);
        }
    }

    // Find starting nodes for all vertices
    // TBD: How to do this with a directed graph?
    for (typename std::deque< Vertex >::iterator i = vertex_queue.begin();
         i != vertex_queue.end(); ++i)
        *i = find_starting_node(G, *i, color, degree);

    return cuthill_mckee_ordering(G, vertex_queue, permutation, color, degree);
}

template < typename Graph, typename OutputIterator, typename VertexIndexMap >
OutputIterator cuthill_mckee_ordering(
    const Graph& G, OutputIterator permutation, VertexIndexMap index_map)
{
    if (boost::graph::has_no_vertices(G))
        return permutation;

    std::vector< default_color_type > colors(num_vertices(G));
    return cuthill_mckee_ordering(G, permutation,
        make_iterator_property_map(&colors[0], index_map, colors[0]),
        make_out_degree_map(G));
}

template < typename Graph, typename OutputIterator >
inline OutputIterator cuthill_mckee_ordering(
    const Graph& G, OutputIterator permutation)
{
    return cuthill_mckee_ordering(G, permutation, get(vertex_index, G));
}
} // namespace boost

#endif // BOOST_GRAPH_CUTHILL_MCKEE_HPP
