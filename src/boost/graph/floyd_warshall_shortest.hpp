// Copyright 2002 Rensselaer Polytechnic Institute

// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Lauren Foutz
//           Scott Hill

/*
  This file implements the functions

  template <class VertexListGraph, class DistanceMatrix,
    class P, class T, class R>
  bool floyd_warshall_initialized_all_pairs_shortest_paths(
    const VertexListGraph& g, DistanceMatrix& d,
    const bgl_named_params<P, T, R>& params)

  AND

  template <class VertexAndEdgeListGraph, class DistanceMatrix,
    class P, class T, class R>
  bool floyd_warshall_all_pairs_shortest_paths(
    const VertexAndEdgeListGraph& g, DistanceMatrix& d,
    const bgl_named_params<P, T, R>& params)
*/

#ifndef BOOST_GRAPH_FLOYD_WARSHALL_HPP
#define BOOST_GRAPH_FLOYD_WARSHALL_HPP

#include <boost/property_map/property_map.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/named_function_params.hpp>
#include <boost/graph/graph_concepts.hpp>
#include <boost/graph/relax.hpp>
#include <boost/concept/assert.hpp>

namespace boost
{
namespace detail
{
    template < typename T, typename BinaryPredicate >
    T min_with_compare(const T& x, const T& y, const BinaryPredicate& compare)
    {
        if (compare(x, y))
            return x;
        else
            return y;
    }

    template < typename VertexListGraph, typename DistanceMatrix,
        typename BinaryPredicate, typename BinaryFunction, typename Infinity,
        typename Zero >
    bool floyd_warshall_dispatch(const VertexListGraph& g, DistanceMatrix& d,
        const BinaryPredicate& compare, const BinaryFunction& combine,
        const Infinity& inf, const Zero& zero)
    {
        typename graph_traits< VertexListGraph >::vertex_iterator i, lasti, j,
            lastj, k, lastk;

        for (boost::tie(k, lastk) = vertices(g); k != lastk; k++)
            for (boost::tie(i, lasti) = vertices(g); i != lasti; i++)
                if (d[*i][*k] != inf)
                    for (boost::tie(j, lastj) = vertices(g); j != lastj; j++)
                        if (d[*k][*j] != inf)
                            d[*i][*j] = detail::min_with_compare(d[*i][*j],
                                combine(d[*i][*k], d[*k][*j]), compare);

        for (boost::tie(i, lasti) = vertices(g); i != lasti; i++)
            if (compare(d[*i][*i], zero))
                return false;
        return true;
    }
}

template < typename VertexListGraph, typename DistanceMatrix,
    typename BinaryPredicate, typename BinaryFunction, typename Infinity,
    typename Zero >
bool floyd_warshall_initialized_all_pairs_shortest_paths(
    const VertexListGraph& g, DistanceMatrix& d, const BinaryPredicate& compare,
    const BinaryFunction& combine, const Infinity& inf, const Zero& zero)
{
    BOOST_CONCEPT_ASSERT((VertexListGraphConcept< VertexListGraph >));

    return detail::floyd_warshall_dispatch(g, d, compare, combine, inf, zero);
}

template < typename VertexAndEdgeListGraph, typename DistanceMatrix,
    typename WeightMap, typename BinaryPredicate, typename BinaryFunction,
    typename Infinity, typename Zero >
bool floyd_warshall_all_pairs_shortest_paths(const VertexAndEdgeListGraph& g,
    DistanceMatrix& d, const WeightMap& w, const BinaryPredicate& compare,
    const BinaryFunction& combine, const Infinity& inf, const Zero& zero)
{
    BOOST_CONCEPT_ASSERT((VertexListGraphConcept< VertexAndEdgeListGraph >));
    BOOST_CONCEPT_ASSERT((EdgeListGraphConcept< VertexAndEdgeListGraph >));
    BOOST_CONCEPT_ASSERT((IncidenceGraphConcept< VertexAndEdgeListGraph >));

    typename graph_traits< VertexAndEdgeListGraph >::vertex_iterator firstv,
        lastv, firstv2, lastv2;
    typename graph_traits< VertexAndEdgeListGraph >::edge_iterator first, last;

    for (boost::tie(firstv, lastv) = vertices(g); firstv != lastv; firstv++)
        for (boost::tie(firstv2, lastv2) = vertices(g); firstv2 != lastv2;
             firstv2++)
            d[*firstv][*firstv2] = inf;

    for (boost::tie(firstv, lastv) = vertices(g); firstv != lastv; firstv++)
        d[*firstv][*firstv] = zero;

    for (boost::tie(first, last) = edges(g); first != last; first++)
    {
        if (d[source(*first, g)][target(*first, g)] != inf)
        {
            d[source(*first, g)][target(*first, g)]
                = detail::min_with_compare(get(w, *first),
                    d[source(*first, g)][target(*first, g)], compare);
        }
        else
            d[source(*first, g)][target(*first, g)] = get(w, *first);
    }

    bool is_undirected = is_same<
        typename graph_traits< VertexAndEdgeListGraph >::directed_category,
        undirected_tag >::value;
    if (is_undirected)
    {
        for (boost::tie(first, last) = edges(g); first != last; first++)
        {
            if (d[target(*first, g)][source(*first, g)] != inf)
                d[target(*first, g)][source(*first, g)]
                    = detail::min_with_compare(get(w, *first),
                        d[target(*first, g)][source(*first, g)], compare);
            else
                d[target(*first, g)][source(*first, g)] = get(w, *first);
        }
    }

    return detail::floyd_warshall_dispatch(g, d, compare, combine, inf, zero);
}

namespace detail
{
    template < class VertexListGraph, class DistanceMatrix, class WeightMap,
        class P, class T, class R >
    bool floyd_warshall_init_dispatch(const VertexListGraph& g,
        DistanceMatrix& d, WeightMap /*w*/,
        const bgl_named_params< P, T, R >& params)
    {
        typedef typename property_traits< WeightMap >::value_type WM;
        WM inf = choose_param(get_param(params, distance_inf_t()),
            std::numeric_limits< WM >::max BOOST_PREVENT_MACRO_SUBSTITUTION());

        return floyd_warshall_initialized_all_pairs_shortest_paths(g, d,
            choose_param(
                get_param(params, distance_compare_t()), std::less< WM >()),
            choose_param(get_param(params, distance_combine_t()),
                closed_plus< WM >(inf)),
            inf, choose_param(get_param(params, distance_zero_t()), WM()));
    }

    template < class VertexAndEdgeListGraph, class DistanceMatrix,
        class WeightMap, class P, class T, class R >
    bool floyd_warshall_noninit_dispatch(const VertexAndEdgeListGraph& g,
        DistanceMatrix& d, WeightMap w,
        const bgl_named_params< P, T, R >& params)
    {
        typedef typename property_traits< WeightMap >::value_type WM;

        WM inf = choose_param(get_param(params, distance_inf_t()),
            std::numeric_limits< WM >::max BOOST_PREVENT_MACRO_SUBSTITUTION());
        return floyd_warshall_all_pairs_shortest_paths(g, d, w,
            choose_param(
                get_param(params, distance_compare_t()), std::less< WM >()),
            choose_param(get_param(params, distance_combine_t()),
                closed_plus< WM >(inf)),
            inf, choose_param(get_param(params, distance_zero_t()), WM()));
    }

} // namespace detail

template < class VertexListGraph, class DistanceMatrix, class P, class T,
    class R >
bool floyd_warshall_initialized_all_pairs_shortest_paths(
    const VertexListGraph& g, DistanceMatrix& d,
    const bgl_named_params< P, T, R >& params)
{
    return detail::floyd_warshall_init_dispatch(g, d,
        choose_const_pmap(get_param(params, edge_weight), g, edge_weight),
        params);
}

template < class VertexListGraph, class DistanceMatrix >
bool floyd_warshall_initialized_all_pairs_shortest_paths(
    const VertexListGraph& g, DistanceMatrix& d)
{
    bgl_named_params< int, int > params(0);
    return detail::floyd_warshall_init_dispatch(
        g, d, get(edge_weight, g), params);
}

template < class VertexAndEdgeListGraph, class DistanceMatrix, class P, class T,
    class R >
bool floyd_warshall_all_pairs_shortest_paths(const VertexAndEdgeListGraph& g,
    DistanceMatrix& d, const bgl_named_params< P, T, R >& params)
{
    return detail::floyd_warshall_noninit_dispatch(g, d,
        choose_const_pmap(get_param(params, edge_weight), g, edge_weight),
        params);
}

template < class VertexAndEdgeListGraph, class DistanceMatrix >
bool floyd_warshall_all_pairs_shortest_paths(
    const VertexAndEdgeListGraph& g, DistanceMatrix& d)
{
    bgl_named_params< int, int > params(0);
    return detail::floyd_warshall_noninit_dispatch(
        g, d, get(edge_weight, g), params);
}

} // namespace boost

#endif
