// Copyright 2004 The Trustees of Indiana University.

// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Douglas Gregor
//           Andrew Lumsdaine
#ifndef BOOST_GRAPH_BRANDES_BETWEENNESS_CENTRALITY_HPP
#define BOOST_GRAPH_BRANDES_BETWEENNESS_CENTRALITY_HPP

#include <stack>
#include <vector>
#include <boost/graph/overloading.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/graph/breadth_first_search.hpp>
#include <boost/graph/relax.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/type_traits/is_convertible.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/mpl/if.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/graph/named_function_params.hpp>
#include <algorithm>

namespace boost
{

namespace detail
{
    namespace graph
    {

        /**
         * Customized visitor passed to Dijkstra's algorithm by Brandes'
         * betweenness centrality algorithm. This visitor is responsible for
         * keeping track of the order in which vertices are discovered, the
         * predecessors on the shortest path(s) to a vertex, and the number
         * of shortest paths.
         */
        template < typename Graph, typename WeightMap, typename IncomingMap,
            typename DistanceMap, typename PathCountMap >
        struct brandes_dijkstra_visitor : public bfs_visitor<>
        {
            typedef typename graph_traits< Graph >::vertex_descriptor
                vertex_descriptor;
            typedef
                typename graph_traits< Graph >::edge_descriptor edge_descriptor;

            brandes_dijkstra_visitor(
                std::stack< vertex_descriptor >& ordered_vertices,
                WeightMap weight, IncomingMap incoming, DistanceMap distance,
                PathCountMap path_count)
            : ordered_vertices(ordered_vertices)
            , weight(weight)
            , incoming(incoming)
            , distance(distance)
            , path_count(path_count)
            {
            }

            /**
             * Whenever an edge e = (v, w) is relaxed, the incoming edge list
             * for w is set to {(v, w)} and the shortest path count of w is set
             * to the number of paths that reach {v}.
             */
            void edge_relaxed(edge_descriptor e, const Graph& g)
            {
                vertex_descriptor v = source(e, g), w = target(e, g);
                incoming[w].clear();
                incoming[w].push_back(e);
                put(path_count, w, get(path_count, v));
            }

            /**
             * If an edge e = (v, w) was not relaxed, it may still be the case
             * that we've found more equally-short paths, so include {(v, w)} in
             * the incoming edges of w and add all of the shortest paths to v to
             * the shortest path count of w.
             */
            void edge_not_relaxed(edge_descriptor e, const Graph& g)
            {
                typedef typename property_traits< WeightMap >::value_type
                    weight_type;
                typedef typename property_traits< DistanceMap >::value_type
                    distance_type;
                vertex_descriptor v = source(e, g), w = target(e, g);
                distance_type d_v = get(distance, v), d_w = get(distance, w);
                weight_type w_e = get(weight, e);

                closed_plus< distance_type > combine;
                if (d_w == combine(d_v, w_e))
                {
                    put(path_count, w, get(path_count, w) + get(path_count, v));
                    incoming[w].push_back(e);
                }
            }

            /// Keep track of vertices as they are reached
            void examine_vertex(vertex_descriptor w, const Graph&)
            {
                ordered_vertices.push(w);
            }

        private:
            std::stack< vertex_descriptor >& ordered_vertices;
            WeightMap weight;
            IncomingMap incoming;
            DistanceMap distance;
            PathCountMap path_count;
        };

        /**
         * Function object that calls Dijkstra's shortest paths algorithm
         * using the Dijkstra visitor for the Brandes betweenness centrality
         * algorithm.
         */
        template < typename WeightMap > struct brandes_dijkstra_shortest_paths
        {
            brandes_dijkstra_shortest_paths(WeightMap weight_map)
            : weight_map(weight_map)
            {
            }

            template < typename Graph, typename IncomingMap,
                typename DistanceMap, typename PathCountMap,
                typename VertexIndexMap >
            void operator()(Graph& g,
                typename graph_traits< Graph >::vertex_descriptor s,
                std::stack< typename graph_traits< Graph >::vertex_descriptor >&
                    ov,
                IncomingMap incoming, DistanceMap distance,
                PathCountMap path_count, VertexIndexMap vertex_index)
            {
                typedef brandes_dijkstra_visitor< Graph, WeightMap, IncomingMap,
                    DistanceMap, PathCountMap >
                    visitor_type;
                visitor_type visitor(
                    ov, weight_map, incoming, distance, path_count);

                dijkstra_shortest_paths(g, s,
                    boost::weight_map(weight_map)
                        .vertex_index_map(vertex_index)
                        .distance_map(distance)
                        .visitor(visitor));
            }

        private:
            WeightMap weight_map;
        };

        /**
         * Function object that invokes breadth-first search for the
         * unweighted form of the Brandes betweenness centrality algorithm.
         */
        struct brandes_unweighted_shortest_paths
        {
            /**
             * Customized visitor passed to breadth-first search, which
             * records predecessor and the number of shortest paths to each
             * vertex.
             */
            template < typename Graph, typename IncomingMap,
                typename DistanceMap, typename PathCountMap >
            struct visitor_type : public bfs_visitor<>
            {
                typedef typename graph_traits< Graph >::edge_descriptor
                    edge_descriptor;
                typedef typename graph_traits< Graph >::vertex_descriptor
                    vertex_descriptor;

                visitor_type(IncomingMap incoming, DistanceMap distance,
                    PathCountMap path_count,
                    std::stack< vertex_descriptor >& ordered_vertices)
                : incoming(incoming)
                , distance(distance)
                , path_count(path_count)
                , ordered_vertices(ordered_vertices)
                {
                }

                /// Keep track of vertices as they are reached
                void examine_vertex(vertex_descriptor v, Graph&)
                {
                    ordered_vertices.push(v);
                }

                /**
                 * Whenever an edge e = (v, w) is labelled a tree edge, the
                 * incoming edge list for w is set to {(v, w)} and the shortest
                 * path count of w is set to the number of paths that reach {v}.
                 */
                void tree_edge(edge_descriptor e, Graph& g)
                {
                    vertex_descriptor v = source(e, g);
                    vertex_descriptor w = target(e, g);
                    put(distance, w, get(distance, v) + 1);

                    put(path_count, w, get(path_count, v));
                    incoming[w].push_back(e);
                }

                /**
                 * If an edge e = (v, w) is not a tree edge, it may still be the
                 * case that we've found more equally-short paths, so include
                 * (v, w) in the incoming edge list of w and add all of the
                 * shortest paths to v to the shortest path count of w.
                 */
                void non_tree_edge(edge_descriptor e, Graph& g)
                {
                    vertex_descriptor v = source(e, g);
                    vertex_descriptor w = target(e, g);
                    if (get(distance, w) == get(distance, v) + 1)
                    {
                        put(path_count, w,
                            get(path_count, w) + get(path_count, v));
                        incoming[w].push_back(e);
                    }
                }

            private:
                IncomingMap incoming;
                DistanceMap distance;
                PathCountMap path_count;
                std::stack< vertex_descriptor >& ordered_vertices;
            };

            template < typename Graph, typename IncomingMap,
                typename DistanceMap, typename PathCountMap,
                typename VertexIndexMap >
            void operator()(Graph& g,
                typename graph_traits< Graph >::vertex_descriptor s,
                std::stack< typename graph_traits< Graph >::vertex_descriptor >&
                    ov,
                IncomingMap incoming, DistanceMap distance,
                PathCountMap path_count, VertexIndexMap vertex_index)
            {
                typedef typename graph_traits< Graph >::vertex_descriptor
                    vertex_descriptor;

                visitor_type< Graph, IncomingMap, DistanceMap, PathCountMap >
                    visitor(incoming, distance, path_count, ov);

                std::vector< default_color_type > colors(num_vertices(g),
                    color_traits< default_color_type >::white());
                boost::queue< vertex_descriptor > Q;
                breadth_first_visit(g, s, Q, visitor,
                    make_iterator_property_map(colors.begin(), vertex_index));
            }
        };

        // When the edge centrality map is a dummy property map, no
        // initialization is needed.
        template < typename Iter >
        inline void init_centrality_map(
            std::pair< Iter, Iter >, dummy_property_map)
        {
        }

        // When we have a real edge centrality map, initialize all of the
        // centralities to zero.
        template < typename Iter, typename Centrality >
        void init_centrality_map(
            std::pair< Iter, Iter > keys, Centrality centrality_map)
        {
            typedef typename property_traits< Centrality >::value_type
                centrality_type;
            while (keys.first != keys.second)
            {
                put(centrality_map, *keys.first, centrality_type(0));
                ++keys.first;
            }
        }

        // When the edge centrality map is a dummy property map, no update
        // is performed.
        template < typename Key, typename T >
        inline void update_centrality(dummy_property_map, const Key&, const T&)
        {
        }

        // When we have a real edge centrality map, add the value to the map
        template < typename CentralityMap, typename Key, typename T >
        inline void update_centrality(
            CentralityMap centrality_map, Key k, const T& x)
        {
            put(centrality_map, k, get(centrality_map, k) + x);
        }

        template < typename Iter >
        inline void divide_centrality_by_two(
            std::pair< Iter, Iter >, dummy_property_map)
        {
        }

        template < typename Iter, typename CentralityMap >
        inline void divide_centrality_by_two(
            std::pair< Iter, Iter > keys, CentralityMap centrality_map)
        {
            typename property_traits< CentralityMap >::value_type two(2);
            while (keys.first != keys.second)
            {
                put(centrality_map, *keys.first,
                    get(centrality_map, *keys.first) / two);
                ++keys.first;
            }
        }

        template < typename Graph, typename CentralityMap,
            typename EdgeCentralityMap, typename IncomingMap,
            typename DistanceMap, typename DependencyMap, typename PathCountMap,
            typename VertexIndexMap, typename ShortestPaths >
        void brandes_betweenness_centrality_impl(const Graph& g,
            CentralityMap centrality, // C_B
            EdgeCentralityMap edge_centrality_map,
            IncomingMap incoming, // P
            DistanceMap distance, // d
            DependencyMap dependency, // delta
            PathCountMap path_count, // sigma
            VertexIndexMap vertex_index, ShortestPaths shortest_paths)
        {
            typedef
                typename graph_traits< Graph >::vertex_iterator vertex_iterator;
            typedef typename graph_traits< Graph >::vertex_descriptor
                vertex_descriptor;

            // Initialize centrality
            init_centrality_map(vertices(g), centrality);
            init_centrality_map(edges(g), edge_centrality_map);

            std::stack< vertex_descriptor > ordered_vertices;
            vertex_iterator s, s_end;
            for (boost::tie(s, s_end) = vertices(g); s != s_end; ++s)
            {
                // Initialize for this iteration
                vertex_iterator w, w_end;
                for (boost::tie(w, w_end) = vertices(g); w != w_end; ++w)
                {
                    incoming[*w].clear();
                    put(path_count, *w, 0);
                    put(dependency, *w, 0);
                }
                put(path_count, *s, 1);

                // Execute the shortest paths algorithm. This will be either
                // Dijkstra's algorithm or a customized breadth-first search,
                // depending on whether the graph is weighted or unweighted.
                shortest_paths(g, *s, ordered_vertices, incoming, distance,
                    path_count, vertex_index);

                while (!ordered_vertices.empty())
                {
                    vertex_descriptor w = ordered_vertices.top();
                    ordered_vertices.pop();

                    typedef typename property_traits< IncomingMap >::value_type
                        incoming_type;
                    typedef typename incoming_type::iterator incoming_iterator;
                    typedef
                        typename property_traits< DependencyMap >::value_type
                            dependency_type;

                    for (incoming_iterator vw = incoming[w].begin();
                         vw != incoming[w].end(); ++vw)
                    {
                        vertex_descriptor v = source(*vw, g);
                        dependency_type factor
                            = dependency_type(get(path_count, v))
                            / dependency_type(get(path_count, w));
                        factor *= (dependency_type(1) + get(dependency, w));
                        put(dependency, v, get(dependency, v) + factor);
                        update_centrality(edge_centrality_map, *vw, factor);
                    }

                    if (w != *s)
                    {
                        update_centrality(centrality, w, get(dependency, w));
                    }
                }
            }

            typedef typename graph_traits< Graph >::directed_category
                directed_category;
            const bool is_undirected
                = is_convertible< directed_category*, undirected_tag* >::value;
            if (is_undirected)
            {
                divide_centrality_by_two(vertices(g), centrality);
                divide_centrality_by_two(edges(g), edge_centrality_map);
            }
        }

    }
} // end namespace detail::graph

template < typename Graph, typename CentralityMap, typename EdgeCentralityMap,
    typename IncomingMap, typename DistanceMap, typename DependencyMap,
    typename PathCountMap, typename VertexIndexMap >
void brandes_betweenness_centrality(const Graph& g,
    CentralityMap centrality, // C_B
    EdgeCentralityMap edge_centrality_map,
    IncomingMap incoming, // P
    DistanceMap distance, // d
    DependencyMap dependency, // delta
    PathCountMap path_count, // sigma
    VertexIndexMap vertex_index BOOST_GRAPH_ENABLE_IF_MODELS_PARM(
        Graph, vertex_list_graph_tag))
{
    detail::graph::brandes_unweighted_shortest_paths shortest_paths;

    detail::graph::brandes_betweenness_centrality_impl(g, centrality,
        edge_centrality_map, incoming, distance, dependency, path_count,
        vertex_index, shortest_paths);
}

template < typename Graph, typename CentralityMap, typename EdgeCentralityMap,
    typename IncomingMap, typename DistanceMap, typename DependencyMap,
    typename PathCountMap, typename VertexIndexMap, typename WeightMap >
void brandes_betweenness_centrality(const Graph& g,
    CentralityMap centrality, // C_B
    EdgeCentralityMap edge_centrality_map,
    IncomingMap incoming, // P
    DistanceMap distance, // d
    DependencyMap dependency, // delta
    PathCountMap path_count, // sigma
    VertexIndexMap vertex_index,
    WeightMap weight_map BOOST_GRAPH_ENABLE_IF_MODELS_PARM(
        Graph, vertex_list_graph_tag))
{
    detail::graph::brandes_dijkstra_shortest_paths< WeightMap > shortest_paths(
        weight_map);

    detail::graph::brandes_betweenness_centrality_impl(g, centrality,
        edge_centrality_map, incoming, distance, dependency, path_count,
        vertex_index, shortest_paths);
}

namespace detail
{
    namespace graph
    {
        template < typename Graph, typename CentralityMap,
            typename EdgeCentralityMap, typename WeightMap,
            typename VertexIndexMap >
        void brandes_betweenness_centrality_dispatch2(const Graph& g,
            CentralityMap centrality, EdgeCentralityMap edge_centrality_map,
            WeightMap weight_map, VertexIndexMap vertex_index)
        {
            typedef typename graph_traits< Graph >::degree_size_type
                degree_size_type;
            typedef
                typename graph_traits< Graph >::edge_descriptor edge_descriptor;
            typedef typename mpl::if_c<
                (is_same< CentralityMap, dummy_property_map >::value),
                EdgeCentralityMap, CentralityMap >::type a_centrality_map;
            typedef typename property_traits< a_centrality_map >::value_type
                centrality_type;

            typename graph_traits< Graph >::vertices_size_type V
                = num_vertices(g);

            std::vector< std::vector< edge_descriptor > > incoming(V);
            std::vector< centrality_type > distance(V);
            std::vector< centrality_type > dependency(V);
            std::vector< degree_size_type > path_count(V);

            brandes_betweenness_centrality(g, centrality, edge_centrality_map,
                make_iterator_property_map(incoming.begin(), vertex_index),
                make_iterator_property_map(distance.begin(), vertex_index),
                make_iterator_property_map(dependency.begin(), vertex_index),
                make_iterator_property_map(path_count.begin(), vertex_index),
                vertex_index, weight_map);
        }

        template < typename Graph, typename CentralityMap,
            typename EdgeCentralityMap, typename VertexIndexMap >
        void brandes_betweenness_centrality_dispatch2(const Graph& g,
            CentralityMap centrality, EdgeCentralityMap edge_centrality_map,
            VertexIndexMap vertex_index)
        {
            typedef typename graph_traits< Graph >::degree_size_type
                degree_size_type;
            typedef
                typename graph_traits< Graph >::edge_descriptor edge_descriptor;
            typedef typename mpl::if_c<
                (is_same< CentralityMap, dummy_property_map >::value),
                EdgeCentralityMap, CentralityMap >::type a_centrality_map;
            typedef typename property_traits< a_centrality_map >::value_type
                centrality_type;

            typename graph_traits< Graph >::vertices_size_type V
                = num_vertices(g);

            std::vector< std::vector< edge_descriptor > > incoming(V);
            std::vector< centrality_type > distance(V);
            std::vector< centrality_type > dependency(V);
            std::vector< degree_size_type > path_count(V);

            brandes_betweenness_centrality(g, centrality, edge_centrality_map,
                make_iterator_property_map(incoming.begin(), vertex_index),
                make_iterator_property_map(distance.begin(), vertex_index),
                make_iterator_property_map(dependency.begin(), vertex_index),
                make_iterator_property_map(path_count.begin(), vertex_index),
                vertex_index);
        }

        template < typename WeightMap >
        struct brandes_betweenness_centrality_dispatch1
        {
            template < typename Graph, typename CentralityMap,
                typename EdgeCentralityMap, typename VertexIndexMap >
            static void run(const Graph& g, CentralityMap centrality,
                EdgeCentralityMap edge_centrality_map,
                VertexIndexMap vertex_index, WeightMap weight_map)
            {
                brandes_betweenness_centrality_dispatch2(g, centrality,
                    edge_centrality_map, weight_map, vertex_index);
            }
        };

        template <>
        struct brandes_betweenness_centrality_dispatch1< param_not_found >
        {
            template < typename Graph, typename CentralityMap,
                typename EdgeCentralityMap, typename VertexIndexMap >
            static void run(const Graph& g, CentralityMap centrality,
                EdgeCentralityMap edge_centrality_map,
                VertexIndexMap vertex_index, param_not_found)
            {
                brandes_betweenness_centrality_dispatch2(
                    g, centrality, edge_centrality_map, vertex_index);
            }
        };

        template < typename T > struct is_bgl_named_params
        {
            BOOST_STATIC_CONSTANT(bool, value = false);
        };

        template < typename Param, typename Tag, typename Rest >
        struct is_bgl_named_params< bgl_named_params< Param, Tag, Rest > >
        {
            BOOST_STATIC_CONSTANT(bool, value = true);
        };

    }
} // end namespace detail::graph

template < typename Graph, typename Param, typename Tag, typename Rest >
void brandes_betweenness_centrality(const Graph& g,
    const bgl_named_params< Param, Tag, Rest >& params
        BOOST_GRAPH_ENABLE_IF_MODELS_PARM(Graph, vertex_list_graph_tag))
{
    typedef bgl_named_params< Param, Tag, Rest > named_params;

    typedef typename get_param_type< edge_weight_t, named_params >::type ew;
    detail::graph::brandes_betweenness_centrality_dispatch1< ew >::run(g,
        choose_param(
            get_param(params, vertex_centrality), dummy_property_map()),
        choose_param(get_param(params, edge_centrality), dummy_property_map()),
        choose_const_pmap(get_param(params, vertex_index), g, vertex_index),
        get_param(params, edge_weight));
}

// disable_if is required to work around problem with MSVC 7.1 (it seems to not
// get partial ordering getween this overload and the previous one correct)
template < typename Graph, typename CentralityMap >
typename disable_if< detail::graph::is_bgl_named_params< CentralityMap >,
    void >::type
brandes_betweenness_centrality(const Graph& g,
    CentralityMap centrality BOOST_GRAPH_ENABLE_IF_MODELS_PARM(
        Graph, vertex_list_graph_tag))
{
    detail::graph::brandes_betweenness_centrality_dispatch2(
        g, centrality, dummy_property_map(), get(vertex_index, g));
}

template < typename Graph, typename CentralityMap, typename EdgeCentralityMap >
void brandes_betweenness_centrality(const Graph& g, CentralityMap centrality,
    EdgeCentralityMap edge_centrality_map BOOST_GRAPH_ENABLE_IF_MODELS_PARM(
        Graph, vertex_list_graph_tag))
{
    detail::graph::brandes_betweenness_centrality_dispatch2(
        g, centrality, edge_centrality_map, get(vertex_index, g));
}

/**
 * Converts "absolute" betweenness centrality (as computed by the
 * brandes_betweenness_centrality algorithm) in the centrality map
 * into "relative" centrality. The result is placed back into the
 * given centrality map.
 */
template < typename Graph, typename CentralityMap >
void relative_betweenness_centrality(const Graph& g, CentralityMap centrality)
{
    typedef typename graph_traits< Graph >::vertex_iterator vertex_iterator;
    typedef
        typename property_traits< CentralityMap >::value_type centrality_type;

    typename graph_traits< Graph >::vertices_size_type n = num_vertices(g);
    centrality_type factor
        = centrality_type(2) / centrality_type(n * n - 3 * n + 2);
    vertex_iterator v, v_end;
    for (boost::tie(v, v_end) = vertices(g); v != v_end; ++v)
    {
        put(centrality, *v, factor * get(centrality, *v));
    }
}

// Compute the central point dominance of a graph.
template < typename Graph, typename CentralityMap >
typename property_traits< CentralityMap >::value_type central_point_dominance(
    const Graph& g,
    CentralityMap centrality BOOST_GRAPH_ENABLE_IF_MODELS_PARM(
        Graph, vertex_list_graph_tag))
{
    using std::max;

    typedef typename graph_traits< Graph >::vertex_iterator vertex_iterator;
    typedef
        typename property_traits< CentralityMap >::value_type centrality_type;

    typename graph_traits< Graph >::vertices_size_type n = num_vertices(g);

    // Find max centrality
    centrality_type max_centrality(0);
    vertex_iterator v, v_end;
    for (boost::tie(v, v_end) = vertices(g); v != v_end; ++v)
    {
        max_centrality = (max)(max_centrality, get(centrality, *v));
    }

    // Compute central point dominance
    centrality_type sum(0);
    for (boost::tie(v, v_end) = vertices(g); v != v_end; ++v)
    {
        sum += (max_centrality - get(centrality, *v));
    }
    return sum / (n - 1);
}

} // end namespace boost

#endif // BOOST_GRAPH_BRANDES_BETWEENNESS_CENTRALITY_HPP
