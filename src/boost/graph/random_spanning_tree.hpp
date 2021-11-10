// Copyright 2010 The Trustees of Indiana University.

// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Jeremiah Willcock
//           Andrew Lumsdaine

#ifndef BOOST_GRAPH_RANDOM_SPANNING_TREE_HPP
#define BOOST_GRAPH_RANDOM_SPANNING_TREE_HPP

#include <vector>
#include <boost/assert.hpp>
#include <boost/graph/loop_erased_random_walk.hpp>
#include <boost/graph/random.hpp>
#include <boost/graph/iteration_macros.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/config.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/graph_concepts.hpp>
#include <boost/graph/properties.hpp>
#include <boost/graph/named_function_params.hpp>

namespace boost
{

namespace detail
{
    // Use Wilson's algorithm (based on loop-free random walks) to generate a
    // random spanning tree.  The distribution of edges used is controlled by
    // the next_edge() function, so this version allows either weighted or
    // unweighted selection of trees.
    // Algorithm is from http://en.wikipedia.org/wiki/Uniform_spanning_tree
    template < typename Graph, typename PredMap, typename ColorMap,
        typename NextEdge >
    void random_spanning_tree_internal(const Graph& g,
        typename graph_traits< Graph >::vertex_descriptor s, PredMap pred,
        ColorMap color, NextEdge next_edge)
    {
        typedef
            typename graph_traits< Graph >::vertex_descriptor vertex_descriptor;

        BOOST_ASSERT(num_vertices(g)
            >= 1); // g must also be undirected (or symmetric) and connected

        typedef color_traits< typename property_traits< ColorMap >::value_type >
            color_gen;
        BGL_FORALL_VERTICES_T(v, g, Graph) put(color, v, color_gen::white());

        std::vector< vertex_descriptor > path;

        put(color, s, color_gen::black());
        put(pred, s, graph_traits< Graph >::null_vertex());

        BGL_FORALL_VERTICES_T(v, g, Graph)
        {
            if (get(color, v) != color_gen::white())
                continue;
            loop_erased_random_walk(g, v, next_edge, color, path);
            for (typename std::vector<
                     vertex_descriptor >::const_reverse_iterator i
                 = path.rbegin();
                 boost::next(i)
                 != (typename std::vector<
                     vertex_descriptor >::const_reverse_iterator)path.rend();
                 ++i)
            {
                typename std::vector<
                    vertex_descriptor >::const_reverse_iterator j
                    = i;
                ++j;
                BOOST_ASSERT(get(color, *j) == color_gen::gray());
                put(color, *j, color_gen::black());
                put(pred, *j, *i);
            }
        }
    }
}

// Compute a uniformly-distributed spanning tree on a graph.  Use Wilson's
// algorithm:
// @inproceedings{wilson96generating,
//    author = {Wilson, David Bruce},
//    title = {Generating random spanning trees more quickly than the cover
//    time}, booktitle = {STOC '96: Proceedings of the twenty-eighth annual ACM
//    symposium on Theory of computing}, year = {1996}, isbn = {0-89791-785-5},
//    pages = {296--303},
//    location = {Philadelphia, Pennsylvania, United States},
//    doi = {http://doi.acm.org/10.1145/237814.237880},
//    publisher = {ACM},
//    address = {New York, NY, USA},
//  }
//
template < typename Graph, typename Gen, typename PredMap, typename ColorMap >
void random_spanning_tree(const Graph& g, Gen& gen,
    typename graph_traits< Graph >::vertex_descriptor root, PredMap pred,
    static_property_map< double >, ColorMap color)
{
    unweighted_random_out_edge_gen< Graph, Gen > random_oe(gen);
    detail::random_spanning_tree_internal(g, root, pred, color, random_oe);
}

// Compute a weight-distributed spanning tree on a graph.
template < typename Graph, typename Gen, typename PredMap, typename WeightMap,
    typename ColorMap >
void random_spanning_tree(const Graph& g, Gen& gen,
    typename graph_traits< Graph >::vertex_descriptor root, PredMap pred,
    WeightMap weight, ColorMap color)
{
    weighted_random_out_edge_gen< Graph, WeightMap, Gen > random_oe(
        weight, gen);
    detail::random_spanning_tree_internal(g, root, pred, color, random_oe);
}

template < typename Graph, typename Gen, typename P, typename T, typename R >
void random_spanning_tree(
    const Graph& g, Gen& gen, const bgl_named_params< P, T, R >& params)
{
    using namespace boost::graph::keywords;
    typedef bgl_named_params< P, T, R > params_type;
    BOOST_GRAPH_DECLARE_CONVERTED_PARAMETERS(params_type, params)
    typedef typename graph_traits< Graph >::vertex_descriptor vertex_descriptor;
    vertex_descriptor default_vertex = *vertices(g).first;
    vertex_descriptor start_vertex = arg_pack[_root_vertex | default_vertex];
    typename boost::parameter::binding< arg_pack_type,
        boost::graph::keywords::tag::predecessor_map >::type pred_map
        = arg_pack[_predecessor_map];
    static_property_map< double > default_weight_map(1.);
    typename boost::parameter::value_type< arg_pack_type,
        boost::graph::keywords::tag::weight_map,
        static_property_map< double > >::type e_w_map
        = arg_pack[_weight_map | default_weight_map];
    typename boost::detail::map_maker< Graph, arg_pack_type,
        boost::graph::keywords::tag::color_map,
        boost::default_color_type >::map_type c_map
        = boost::detail::make_color_map_from_arg_pack(g, arg_pack);
    random_spanning_tree(g, gen, start_vertex, pred_map, e_w_map, c_map);
}
}

#include <boost/graph/iteration_macros_undef.hpp>

#endif // BOOST_GRAPH_RANDOM_SPANNING_TREE_HPP
