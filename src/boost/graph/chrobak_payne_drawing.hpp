//=======================================================================
// Copyright (c) Aaron Windsor 2007
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#ifndef __CHROBAK_PAYNE_DRAWING_HPP__
#define __CHROBAK_PAYNE_DRAWING_HPP__

#include <vector>
#include <list>
#include <stack>
#include <boost/config.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/property_map/property_map.hpp>

namespace boost
{

namespace graph
{
    namespace detail
    {

        template < typename Graph, typename VertexToVertexMap,
            typename VertexTo1DCoordMap >
        void accumulate_offsets(
            typename graph_traits< Graph >::vertex_descriptor v,
            std::size_t offset, const Graph& g, VertexTo1DCoordMap x,
            VertexTo1DCoordMap delta_x, VertexToVertexMap left,
            VertexToVertexMap right)
        {
            typedef typename graph_traits< Graph >::vertex_descriptor
                vertex_descriptor;
            // Suggestion of explicit stack from Aaron Windsor to avoid system
            // stack overflows.
            typedef std::pair< vertex_descriptor, std::size_t > stack_entry;
            std::stack< stack_entry > st;
            st.push(stack_entry(v, offset));
            while (!st.empty())
            {
                vertex_descriptor v = st.top().first;
                std::size_t offset = st.top().second;
                st.pop();
                if (v != graph_traits< Graph >::null_vertex())
                {
                    x[v] += delta_x[v] + offset;
                    st.push(stack_entry(left[v], x[v]));
                    st.push(stack_entry(right[v], x[v]));
                }
            }
        }

    } /*namespace detail*/
} /*namespace graph*/

template < typename Graph, typename PlanarEmbedding, typename ForwardIterator,
    typename GridPositionMap, typename VertexIndexMap >
void chrobak_payne_straight_line_drawing(const Graph& g,
    PlanarEmbedding embedding, ForwardIterator ordering_begin,
    ForwardIterator ordering_end, GridPositionMap drawing, VertexIndexMap vm)
{

    typedef typename graph_traits< Graph >::vertex_descriptor vertex_t;
    typedef typename graph_traits< Graph >::vertex_iterator vertex_iterator_t;
    typedef typename PlanarEmbedding::value_type::const_iterator
        edge_permutation_iterator_t;
    typedef typename graph_traits< Graph >::vertices_size_type v_size_t;
    typedef std::vector< vertex_t > vertex_vector_t;
    typedef std::vector< v_size_t > vsize_vector_t;
    typedef std::vector< bool > bool_vector_t;
    typedef boost::iterator_property_map< typename vertex_vector_t::iterator,
        VertexIndexMap >
        vertex_to_vertex_map_t;
    typedef boost::iterator_property_map< typename vsize_vector_t::iterator,
        VertexIndexMap >
        vertex_to_vsize_map_t;
    typedef boost::iterator_property_map< typename bool_vector_t::iterator,
        VertexIndexMap >
        vertex_to_bool_map_t;

    vertex_vector_t left_vector(
        num_vertices(g), graph_traits< Graph >::null_vertex());
    vertex_vector_t right_vector(
        num_vertices(g), graph_traits< Graph >::null_vertex());
    vsize_vector_t seen_as_right_vector(num_vertices(g), 0);
    vsize_vector_t seen_vector(num_vertices(g), 0);
    vsize_vector_t delta_x_vector(num_vertices(g), 0);
    vsize_vector_t y_vector(num_vertices(g));
    vsize_vector_t x_vector(num_vertices(g), 0);
    bool_vector_t installed_vector(num_vertices(g), false);

    vertex_to_vertex_map_t left(left_vector.begin(), vm);
    vertex_to_vertex_map_t right(right_vector.begin(), vm);
    vertex_to_vsize_map_t seen_as_right(seen_as_right_vector.begin(), vm);
    vertex_to_vsize_map_t seen(seen_vector.begin(), vm);
    vertex_to_vsize_map_t delta_x(delta_x_vector.begin(), vm);
    vertex_to_vsize_map_t y(y_vector.begin(), vm);
    vertex_to_vsize_map_t x(x_vector.begin(), vm);
    vertex_to_bool_map_t installed(installed_vector.begin(), vm);

    v_size_t timestamp = 1;
    vertex_vector_t installed_neighbors;

    ForwardIterator itr = ordering_begin;
    vertex_t v1 = *itr;
    ++itr;
    vertex_t v2 = *itr;
    ++itr;
    vertex_t v3 = *itr;
    ++itr;

    delta_x[v2] = 1;
    delta_x[v3] = 1;

    y[v1] = 0;
    y[v2] = 0;
    y[v3] = 1;

    right[v1] = v3;
    right[v3] = v2;

    installed[v1] = installed[v2] = installed[v3] = true;

    for (ForwardIterator itr_end = ordering_end; itr != itr_end; ++itr)
    {
        vertex_t v = *itr;

        // First, find the leftmost and rightmost neighbor of v on the outer
        // cycle of the embedding.
        // Note: since we're moving clockwise through the edges adjacent to v,
        // we're actually moving from right to left among v's neighbors on the
        // outer face (since v will be installed above them all) looking for
        // the leftmost and rightmost installed neigbhors

        vertex_t leftmost = graph_traits< Graph >::null_vertex();
        vertex_t rightmost = graph_traits< Graph >::null_vertex();

        installed_neighbors.clear();

        vertex_t prev_vertex = graph_traits< Graph >::null_vertex();
        edge_permutation_iterator_t pi, pi_end;
        pi_end = embedding[v].end();
        for (pi = embedding[v].begin(); pi != pi_end; ++pi)
        {
            vertex_t curr_vertex
                = source(*pi, g) == v ? target(*pi, g) : source(*pi, g);

            // Skip any self-loops or parallel edges
            if (curr_vertex == v || curr_vertex == prev_vertex)
                continue;

            if (installed[curr_vertex])
            {
                seen[curr_vertex] = timestamp;

                if (right[curr_vertex] != graph_traits< Graph >::null_vertex())
                {
                    seen_as_right[right[curr_vertex]] = timestamp;
                }
                installed_neighbors.push_back(curr_vertex);
            }

            prev_vertex = curr_vertex;
        }

        typename vertex_vector_t::iterator vi, vi_end;
        vi_end = installed_neighbors.end();
        for (vi = installed_neighbors.begin(); vi != vi_end; ++vi)
        {
            if (right[*vi] == graph_traits< Graph >::null_vertex()
                || seen[right[*vi]] != timestamp)
                rightmost = *vi;
            if (seen_as_right[*vi] != timestamp)
                leftmost = *vi;
        }

        ++timestamp;

        // stretch gaps
        ++delta_x[right[leftmost]];
        ++delta_x[rightmost];

        // adjust offsets
        std::size_t delta_p_q = 0;
        vertex_t stopping_vertex = right[rightmost];
        for (vertex_t temp = right[leftmost]; temp != stopping_vertex;
             temp = right[temp])
        {
            delta_p_q += delta_x[temp];
        }

        delta_x[v] = ((y[rightmost] + delta_p_q) - y[leftmost]) / 2;
        y[v] = y[leftmost] + delta_x[v];
        delta_x[rightmost] = delta_p_q - delta_x[v];

        bool leftmost_and_rightmost_adjacent = right[leftmost] == rightmost;
        if (!leftmost_and_rightmost_adjacent)
            delta_x[right[leftmost]] -= delta_x[v];

        // install v
        if (!leftmost_and_rightmost_adjacent)
        {
            left[v] = right[leftmost];
            vertex_t next_to_rightmost;
            for (vertex_t temp = leftmost; temp != rightmost;
                 temp = right[temp])
            {
                next_to_rightmost = temp;
            }

            right[next_to_rightmost] = graph_traits< Graph >::null_vertex();
        }
        else
        {
            left[v] = graph_traits< Graph >::null_vertex();
        }

        right[leftmost] = v;
        right[v] = rightmost;
        installed[v] = true;
    }

    graph::detail::accumulate_offsets(
        *ordering_begin, 0, g, x, delta_x, left, right);

    vertex_iterator_t vi, vi_end;
    for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi)
    {
        vertex_t v(*vi);
        drawing[v].x = x[v];
        drawing[v].y = y[v];
    }
}

template < typename Graph, typename PlanarEmbedding, typename ForwardIterator,
    typename GridPositionMap >
inline void chrobak_payne_straight_line_drawing(const Graph& g,
    PlanarEmbedding embedding, ForwardIterator ord_begin,
    ForwardIterator ord_end, GridPositionMap drawing)
{
    chrobak_payne_straight_line_drawing(
        g, embedding, ord_begin, ord_end, drawing, get(vertex_index, g));
}

} // namespace boost

#endif //__CHROBAK_PAYNE_DRAWING_HPP__
