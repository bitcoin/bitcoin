// Copyright 2005 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Alex Breuer
//           Andrew Lumsdaine
#ifndef BOOST_GRAPH_GRAPH_STATS_HPP
#define BOOST_GRAPH_GRAPH_STATS_HPP

#include <map>
#include <list>
#include <boost/graph/iteration_macros.hpp>
#include <boost/assert.hpp>

namespace boost
{
namespace graph
{

    template < typename Graph > struct sort_edge_by_origin
    {
    public:
        typedef typename graph_traits< Graph >::edge_descriptor edge_type;

        explicit sort_edge_by_origin(Graph& g) : g(g) {}

        inline bool operator()(edge_type a, edge_type b)
        {
            return source(a, g) == source(b, g) ? target(a, g) < target(b, g)
                                                : source(a, g) < source(b, g);
        }

    private:
        Graph& g;
    };

    template < typename Graph > struct equal_edge
    {
    public:
        typedef typename graph_traits< Graph >::edge_descriptor edge_type;

        explicit equal_edge(Graph& g) : g(g) {}

        inline bool operator()(edge_type a, edge_type b)
        {
            return source(a, g) == source(b, g) && target(a, g) == target(b, g);
        }

    private:
        Graph& g;
    };

    template < typename Graph > unsigned long num_dup_edges(Graph& g)
    {
        typedef typename graph_traits< Graph >::edge_iterator e_iterator_type;
        typedef typename graph_traits< Graph >::edge_descriptor edge_type;

        std::list< edge_type > all_edges;

        BGL_FORALL_EDGES_T(e, g, Graph) { all_edges.push_back(e); }

        sort_edge_by_origin< Graph > cmp1(g);
        all_edges.sort(cmp1);
        equal_edge< Graph > cmp2(g);
        all_edges.unique(cmp2);

        return num_edges(g) - all_edges.size();
    }

    template < typename Graph >
    std::map< unsigned long, unsigned long > dup_edge_dist(Graph& g)
    {
        std::map< unsigned long, unsigned long > dist;
        typedef
            typename graph_traits< Graph >::adjacency_iterator a_iterator_type;
        typedef typename graph_traits< Graph >::vertex_descriptor vertex_type;

        BGL_FORALL_VERTICES_T(v, g, Graph)
        {
            std::list< vertex_type > front_neighbors;
            a_iterator_type a_iter, a_end;
            for (boost::tie(a_iter, a_end) = adjacent_vertices(v, g);
                 a_iter != a_end; ++a_iter)
            {
                front_neighbors.push_back(*a_iter);
            }

            front_neighbors.sort();
            front_neighbors.unique();
            dist[out_degree(v, g) - front_neighbors.size()] += 1;
        }
        return dist;
    }

    template < typename Graph >
    std::map< unsigned long, unsigned long > degree_dist(Graph& g)
    {
        std::map< unsigned long, unsigned long > dist;
        typedef
            typename graph_traits< Graph >::adjacency_iterator a_iterator_type;
        typedef typename graph_traits< Graph >::vertex_descriptor vertex_type;

        BGL_FORALL_VERTICES_T(v, g, Graph) { dist[out_degree(v, g)] += 1; }

        return dist;
    }

    template < typename Graph >
    std::map< unsigned long, double > weight_degree_dist(Graph& g)
    {
        std::map< unsigned long, double > dist, n;
        typedef
            typename graph_traits< Graph >::adjacency_iterator a_iterator_type;
        typedef typename graph_traits< Graph >::vertex_descriptor vertex_type;
        typedef typename property_map< Graph, edge_weight_t >::const_type
            edge_map_type;
        typedef typename property_traits< edge_map_type >::value_type
            edge_weight_type;

        typename property_map< Graph, edge_weight_t >::type em
            = get(edge_weight, g);

        BGL_FORALL_VERTICES_T(v, g, Graph)
        {
            edge_weight_type tmp = 0;
            BGL_FORALL_OUTEDGES_T(v, e, g, Graph) { tmp += em[e]; }
            n[out_degree(v, g)] += 1.;
            dist[out_degree(v, g)] += tmp;
        }

        for (std::map< unsigned long, double >::iterator iter = dist.begin();
             iter != dist.end(); ++iter)
        {
            BOOST_ASSERT(n[iter->first] != 0);
            dist[iter->first] /= n[iter->first];
        }

        return dist;
    }

}
}

#endif
