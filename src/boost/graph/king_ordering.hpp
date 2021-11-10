//=======================================================================
// Copyright 1997, 1998, 1999, 2000 University of Notre Dame.
// Copyright 2004, 2005 Trustees of Indiana University
// Authors: Andrew Lumsdaine, Lie-Quan Lee, Jeremy G. Siek,
//          Doug Gregor, D. Kevin McGrath
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================//
#ifndef BOOST_GRAPH_KING_HPP
#define BOOST_GRAPH_KING_HPP

#include <deque>
#include <vector>
#include <algorithm>
#include <boost/config.hpp>
#include <boost/bind/bind.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/graph/detail/sparse_ordering.hpp>
#include <boost/graph/graph_utility.hpp>

/*
  King Algorithm for matrix reordering
*/

namespace boost
{
namespace detail
{
    template < typename OutputIterator, typename Buffer, typename Compare,
        typename PseudoDegreeMap, typename VecMap, typename VertexIndexMap >
    class bfs_king_visitor : public default_bfs_visitor
    {
    public:
        bfs_king_visitor(OutputIterator* iter, Buffer* b, Compare compare,
            PseudoDegreeMap deg, std::vector< int > loc, VecMap color,
            VertexIndexMap vertices)
        : permutation(iter)
        , Qptr(b)
        , degree(deg)
        , comp(compare)
        , Qlocation(loc)
        , colors(color)
        , vertex_map(vertices)
        {
        }

        template < typename Vertex, typename Graph >
        void finish_vertex(Vertex, Graph& g)
        {
            using namespace boost::placeholders;

            typename graph_traits< Graph >::out_edge_iterator ei, ei_end;
            Vertex v, w;

            typedef typename std::deque< Vertex >::reverse_iterator
                reverse_iterator;

            reverse_iterator rend = Qptr->rend() - index_begin;
            reverse_iterator rbegin = Qptr->rbegin();

            // heap the vertices already there
            std::make_heap(rbegin, rend, boost::bind< bool >(comp, _2, _1));

            unsigned i = 0;

            for (i = index_begin; i != Qptr->size(); ++i)
            {
                colors[get(vertex_map, (*Qptr)[i])] = 1;
                Qlocation[get(vertex_map, (*Qptr)[i])] = i;
            }

            i = 0;

            for (; rbegin != rend; rend--)
            {
                percolate_down< Vertex >(i);
                w = (*Qptr)[index_begin + i];
                for (boost::tie(ei, ei_end) = out_edges(w, g); ei != ei_end;
                     ++ei)
                {
                    v = target(*ei, g);
                    put(degree, v, get(degree, v) - 1);

                    if (colors[get(vertex_map, v)] == 1)
                    {
                        percolate_up< Vertex >(get(vertex_map, v), i);
                    }
                }

                colors[get(vertex_map, w)] = 0;
                i++;
            }
        }

        template < typename Vertex, typename Graph >
        void examine_vertex(Vertex u, const Graph&)
        {

            *(*permutation)++ = u;
            index_begin = Qptr->size();
        }

    protected:
        // this function replaces pop_heap, and tracks state information
        template < typename Vertex > void percolate_down(int offset)
        {
            int heap_last = index_begin + offset;
            int heap_first = Qptr->size() - 1;

            // pop_heap functionality:
            // swap first, last
            std::swap((*Qptr)[heap_last], (*Qptr)[heap_first]);

            // swap in the location queue
            std::swap(Qlocation[heap_first], Qlocation[heap_last]);

            // set drifter, children
            int drifter = heap_first;
            int drifter_heap = Qptr->size() - drifter;

            int right_child_heap = drifter_heap * 2 + 1;
            int right_child = Qptr->size() - right_child_heap;

            int left_child_heap = drifter_heap * 2;
            int left_child = Qptr->size() - left_child_heap;

            // check that we are staying in the heap
            bool valid = (right_child < heap_last) ? false : true;

            // pick smallest child of drifter, and keep in mind there might only
            // be left child
            int smallest_child = (valid
                                     && get(degree, (*Qptr)[left_child])
                                         > get(degree, (*Qptr)[right_child]))
                ? right_child
                : left_child;

            while (valid && smallest_child < heap_last
                && comp((*Qptr)[drifter], (*Qptr)[smallest_child]))
            {

                // if smallest child smaller than drifter, swap them
                std::swap((*Qptr)[smallest_child], (*Qptr)[drifter]);
                std::swap(Qlocation[drifter], Qlocation[smallest_child]);

                // update the values, run again, as necessary
                drifter = smallest_child;
                drifter_heap = Qptr->size() - drifter;

                right_child_heap = drifter_heap * 2 + 1;
                right_child = Qptr->size() - right_child_heap;

                left_child_heap = drifter_heap * 2;
                left_child = Qptr->size() - left_child_heap;

                valid = (right_child < heap_last) ? false : true;

                smallest_child = (valid
                                     && get(degree, (*Qptr)[left_child])
                                         > get(degree, (*Qptr)[right_child]))
                    ? right_child
                    : left_child;
            }
        }

        // this is like percolate down, but we always compare against the
        // parent, as there is only a single choice
        template < typename Vertex > void percolate_up(int vertex, int offset)
        {

            int child_location = Qlocation[vertex];
            int heap_child_location = Qptr->size() - child_location;
            int heap_parent_location = (int)(heap_child_location / 2);
            unsigned parent_location = Qptr->size() - heap_parent_location;

            bool valid = (heap_parent_location != 0
                && child_location > index_begin + offset
                && parent_location < Qptr->size());

            while (valid
                && comp((*Qptr)[child_location], (*Qptr)[parent_location]))
            {

                // swap in the heap
                std::swap((*Qptr)[child_location], (*Qptr)[parent_location]);

                // swap in the location queue
                std::swap(
                    Qlocation[child_location], Qlocation[parent_location]);

                child_location = parent_location;
                heap_child_location = heap_parent_location;
                heap_parent_location = (int)(heap_child_location / 2);
                parent_location = Qptr->size() - heap_parent_location;
                valid = (heap_parent_location != 0
                    && child_location > index_begin + offset);
            }
        }

        OutputIterator* permutation;
        int index_begin;
        Buffer* Qptr;
        PseudoDegreeMap degree;
        Compare comp;
        std::vector< int > Qlocation;
        VecMap colors;
        VertexIndexMap vertex_map;
    };

} // namespace detail

template < class Graph, class OutputIterator, class ColorMap, class DegreeMap,
    typename VertexIndexMap >
OutputIterator king_ordering(const Graph& g,
    std::deque< typename graph_traits< Graph >::vertex_descriptor >
        vertex_queue,
    OutputIterator permutation, ColorMap color, DegreeMap degree,
    VertexIndexMap index_map)
{
    typedef typename property_traits< DegreeMap >::value_type ds_type;
    typedef typename property_traits< ColorMap >::value_type ColorValue;
    typedef color_traits< ColorValue > Color;
    typedef typename graph_traits< Graph >::vertex_descriptor Vertex;
    typedef iterator_property_map< typename std::vector< ds_type >::iterator,
        VertexIndexMap, ds_type, ds_type& >
        PseudoDegreeMap;
    typedef indirect_cmp< PseudoDegreeMap, std::less< ds_type > > Compare;
    typedef typename boost::sparse::sparse_ordering_queue< Vertex > queue;
    typedef typename detail::bfs_king_visitor< OutputIterator, queue, Compare,
        PseudoDegreeMap, std::vector< int >, VertexIndexMap >
        Visitor;
    typedef
        typename graph_traits< Graph >::vertices_size_type vertices_size_type;
    std::vector< ds_type > pseudo_degree_vec(num_vertices(g));
    PseudoDegreeMap pseudo_degree(pseudo_degree_vec.begin(), index_map);

    typename graph_traits< Graph >::vertex_iterator ui, ui_end;
    queue Q;
    // Copy degree to pseudo_degree
    // initialize the color map
    for (boost::tie(ui, ui_end) = vertices(g); ui != ui_end; ++ui)
    {
        put(pseudo_degree, *ui, get(degree, *ui));
        put(color, *ui, Color::white());
    }

    Compare comp(pseudo_degree);
    std::vector< int > colors(num_vertices(g));

    for (vertices_size_type i = 0; i < num_vertices(g); i++)
        colors[i] = 0;

    std::vector< int > loc(num_vertices(g));

    // create the visitor
    Visitor vis(&permutation, &Q, comp, pseudo_degree, loc, colors, index_map);

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
template < class Graph, class OutputIterator, class ColorMap, class DegreeMap,
    typename VertexIndexMap >
OutputIterator king_ordering(const Graph& g,
    typename graph_traits< Graph >::vertex_descriptor s,
    OutputIterator permutation, ColorMap color, DegreeMap degree,
    VertexIndexMap index_map)
{

    std::deque< typename graph_traits< Graph >::vertex_descriptor >
        vertex_queue;
    vertex_queue.push_front(s);
    return king_ordering(
        g, vertex_queue, permutation, color, degree, index_map);
}

template < class Graph, class OutputIterator, class ColorMap, class DegreeMap,
    class VertexIndexMap >
OutputIterator king_ordering(const Graph& G, OutputIterator permutation,
    ColorMap color, DegreeMap degree, VertexIndexMap index_map)
{
    if (has_no_vertices(G))
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

    return king_ordering(
        G, vertex_queue, permutation, color, degree, index_map);
}

template < typename Graph, typename OutputIterator, typename VertexIndexMap >
OutputIterator king_ordering(
    const Graph& G, OutputIterator permutation, VertexIndexMap index_map)
{
    if (has_no_vertices(G))
        return permutation;

    std::vector< default_color_type > colors(num_vertices(G));
    return king_ordering(G, permutation,
        make_iterator_property_map(&colors[0], index_map, colors[0]),
        make_out_degree_map(G), index_map);
}

template < typename Graph, typename OutputIterator >
inline OutputIterator king_ordering(const Graph& G, OutputIterator permutation)
{
    return king_ordering(G, permutation, get(vertex_index, G));
}

} // namespace boost

#endif // BOOST_GRAPH_KING_HPP
