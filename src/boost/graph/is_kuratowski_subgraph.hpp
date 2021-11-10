//=======================================================================
// Copyright 2007 Aaron Windsor
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================
#ifndef __IS_KURATOWSKI_SUBGRAPH_HPP__
#define __IS_KURATOWSKI_SUBGRAPH_HPP__

#include <boost/config.hpp>
#include <boost/tuple/tuple.hpp> //for tie
#include <boost/property_map/property_map.hpp>
#include <boost/graph/properties.hpp>
#include <boost/graph/isomorphism.hpp>
#include <boost/graph/adjacency_list.hpp>

#include <algorithm>
#include <vector>
#include <set>

namespace boost
{

namespace detail
{

    template < typename Graph > Graph make_K_5()
    {
        typename graph_traits< Graph >::vertex_iterator vi, vi_end, inner_vi;
        Graph K_5(5);
        for (boost::tie(vi, vi_end) = vertices(K_5); vi != vi_end; ++vi)
            for (inner_vi = next(vi); inner_vi != vi_end; ++inner_vi)
                add_edge(*vi, *inner_vi, K_5);
        return K_5;
    }

    template < typename Graph > Graph make_K_3_3()
    {
        typename graph_traits< Graph >::vertex_iterator vi, vi_end,
            bipartition_start, inner_vi;
        Graph K_3_3(6);
        bipartition_start = next(next(next(vertices(K_3_3).first)));
        for (boost::tie(vi, vi_end) = vertices(K_3_3); vi != bipartition_start;
             ++vi)
            for (inner_vi = bipartition_start; inner_vi != vi_end; ++inner_vi)
                add_edge(*vi, *inner_vi, K_3_3);
        return K_3_3;
    }

    template < typename AdjacencyList, typename Vertex >
    void contract_edge(AdjacencyList& neighbors, Vertex u, Vertex v)
    {
        // Remove u from v's neighbor list
        neighbors[v].erase(
            std::remove(neighbors[v].begin(), neighbors[v].end(), u),
            neighbors[v].end());

        // Replace any references to u with references to v
        typedef
            typename AdjacencyList::value_type::iterator adjacency_iterator_t;

        adjacency_iterator_t u_neighbor_end = neighbors[u].end();
        for (adjacency_iterator_t u_neighbor_itr = neighbors[u].begin();
             u_neighbor_itr != u_neighbor_end; ++u_neighbor_itr)
        {
            Vertex u_neighbor(*u_neighbor_itr);
            std::replace(neighbors[u_neighbor].begin(),
                neighbors[u_neighbor].end(), u, v);
        }

        // Remove v from u's neighbor list
        neighbors[u].erase(
            std::remove(neighbors[u].begin(), neighbors[u].end(), v),
            neighbors[u].end());

        // Add everything in u's neighbor list to v's neighbor list
        std::copy(neighbors[u].begin(), neighbors[u].end(),
            std::back_inserter(neighbors[v]));

        // Clear u's neighbor list
        neighbors[u].clear();
    }

    enum target_graph_t
    {
        tg_k_3_3,
        tg_k_5
    };

} // namespace detail

template < typename Graph, typename ForwardIterator, typename VertexIndexMap >
bool is_kuratowski_subgraph(const Graph& g, ForwardIterator begin,
    ForwardIterator end, VertexIndexMap vm)
{

    typedef typename graph_traits< Graph >::vertex_descriptor vertex_t;
    typedef typename graph_traits< Graph >::vertex_iterator vertex_iterator_t;
    typedef typename graph_traits< Graph >::edge_descriptor edge_t;
    typedef typename graph_traits< Graph >::edges_size_type e_size_t;
    typedef typename graph_traits< Graph >::vertices_size_type v_size_t;
    typedef typename std::vector< vertex_t > v_list_t;
    typedef typename v_list_t::iterator v_list_iterator_t;
    typedef iterator_property_map< typename std::vector< v_list_t >::iterator,
        VertexIndexMap >
        vertex_to_v_list_map_t;

    typedef adjacency_list< vecS, vecS, undirectedS > small_graph_t;

    detail::target_graph_t target_graph
        = detail::tg_k_3_3; // unless we decide otherwise later

    static small_graph_t K_5(detail::make_K_5< small_graph_t >());

    static small_graph_t K_3_3(detail::make_K_3_3< small_graph_t >());

    v_size_t n_vertices(num_vertices(g));
    v_size_t max_num_edges(3 * n_vertices - 5);

    std::vector< v_list_t > neighbors_vector(n_vertices);
    vertex_to_v_list_map_t neighbors(neighbors_vector.begin(), vm);

    e_size_t count = 0;
    for (ForwardIterator itr = begin; itr != end; ++itr)
    {

        if (count++ > max_num_edges)
            return false;

        edge_t e(*itr);
        vertex_t u(source(e, g));
        vertex_t v(target(e, g));

        neighbors[u].push_back(v);
        neighbors[v].push_back(u);
    }

    for (v_size_t max_size = 2; max_size < 5; ++max_size)
    {

        vertex_iterator_t vi, vi_end;
        for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi)
        {
            vertex_t v(*vi);

            // a hack to make sure we don't contract the middle edge of a path
            // of four degree-3 vertices
            if (max_size == 4 && neighbors[v].size() == 3)
            {
                if (neighbors[neighbors[v][0]].size()
                        + neighbors[neighbors[v][1]].size()
                        + neighbors[neighbors[v][2]].size()
                    < 11 // so, it has two degree-3 neighbors
                )
                    continue;
            }

            while (neighbors[v].size() > 0 && neighbors[v].size() < max_size)
            {
                // Find one of v's neighbors u such that v and u
                // have no neighbors in common. We'll look for such a
                // neighbor with a naive cubic-time algorithm since the
                // max size of any of the neighbor sets we'll consider
                // merging is 3

                bool neighbor_sets_intersect = false;

                vertex_t min_u = graph_traits< Graph >::null_vertex();
                vertex_t u;
                v_list_iterator_t v_neighbor_end = neighbors[v].end();
                for (v_list_iterator_t v_neighbor_itr = neighbors[v].begin();
                     v_neighbor_itr != v_neighbor_end; ++v_neighbor_itr)
                {
                    neighbor_sets_intersect = false;
                    u = *v_neighbor_itr;
                    v_list_iterator_t u_neighbor_end = neighbors[u].end();
                    for (v_list_iterator_t u_neighbor_itr
                         = neighbors[u].begin();
                         u_neighbor_itr != u_neighbor_end
                         && !neighbor_sets_intersect;
                         ++u_neighbor_itr)
                    {
                        for (v_list_iterator_t inner_v_neighbor_itr
                             = neighbors[v].begin();
                             inner_v_neighbor_itr != v_neighbor_end;
                             ++inner_v_neighbor_itr)
                        {
                            if (*u_neighbor_itr == *inner_v_neighbor_itr)
                            {
                                neighbor_sets_intersect = true;
                                break;
                            }
                        }
                    }
                    if (!neighbor_sets_intersect
                        && (min_u == graph_traits< Graph >::null_vertex()
                            || neighbors[u].size() < neighbors[min_u].size()))
                    {
                        min_u = u;
                    }
                }

                if (min_u == graph_traits< Graph >::null_vertex())
                    // Exited the loop without finding an appropriate neighbor
                    // of v, so v must be a lost cause. Move on to other
                    // vertices.
                    break;
                else
                    u = min_u;

                detail::contract_edge(neighbors, u, v);

            } // end iteration over v's neighbors

        } // end iteration through vertices v

        if (max_size == 3)
        {
            // check to see whether we should go on to find a K_5
            for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi)
                if (neighbors[*vi].size() == 4)
                {
                    target_graph = detail::tg_k_5;
                    break;
                }

            if (target_graph == detail::tg_k_3_3)
                break;
        }

    } // end iteration through max degree 2,3, and 4

    // Now, there should only be 5 or 6 vertices with any neighbors. Find them.

    v_list_t main_vertices;
    vertex_iterator_t vi, vi_end;

    for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi)
    {
        if (!neighbors[*vi].empty())
            main_vertices.push_back(*vi);
    }

    // create a graph isomorphic to the contracted graph to test
    // against K_5 and K_3_3
    small_graph_t contracted_graph(main_vertices.size());
    std::map< vertex_t,
        typename graph_traits< small_graph_t >::vertex_descriptor >
        contracted_vertex_map;

    typename v_list_t::iterator itr, itr_end;
    itr_end = main_vertices.end();
    typename graph_traits< small_graph_t >::vertex_iterator si
        = vertices(contracted_graph).first;

    for (itr = main_vertices.begin(); itr != itr_end; ++itr, ++si)
    {
        contracted_vertex_map[*itr] = *si;
    }

    typename v_list_t::iterator jtr, jtr_end;
    for (itr = main_vertices.begin(); itr != itr_end; ++itr)
    {
        jtr_end = neighbors[*itr].end();
        for (jtr = neighbors[*itr].begin(); jtr != jtr_end; ++jtr)
        {
            if (get(vm, *itr) < get(vm, *jtr))
            {
                add_edge(contracted_vertex_map[*itr],
                    contracted_vertex_map[*jtr], contracted_graph);
            }
        }
    }

    if (target_graph == detail::tg_k_5)
    {
        return boost::isomorphism(K_5, contracted_graph);
    }
    else // target_graph == tg_k_3_3
    {
        return boost::isomorphism(K_3_3, contracted_graph);
    }
}

template < typename Graph, typename ForwardIterator >
bool is_kuratowski_subgraph(
    const Graph& g, ForwardIterator begin, ForwardIterator end)
{
    return is_kuratowski_subgraph(g, begin, end, get(vertex_index, g));
}

}

#endif //__IS_KURATOWSKI_SUBGRAPH_HPP__
