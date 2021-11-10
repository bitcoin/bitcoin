//
//=======================================================================
// Copyright 2002 Marc Wintermantel (wintermantel@even-ag.ch)
// ETH Zurich, Center of Structure Technologies
// (https://web.archive.org/web/20050307090307/http://www.structures.ethz.ch/)
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================
//

#ifndef BOOST_GRAPH_WAVEFRONT_HPP
#define BOOST_GRAPH_WAVEFRONT_HPP

#include <boost/config.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/detail/numeric_traits.hpp>
#include <boost/graph/bandwidth.hpp>
#include <boost/config/no_tr1/cmath.hpp>
#include <vector>
#include <algorithm> // for std::min and std::max

namespace boost
{

template < typename Graph, typename VertexIndexMap >
typename graph_traits< Graph >::vertices_size_type ith_wavefront(
    typename graph_traits< Graph >::vertex_descriptor i, const Graph& g,
    VertexIndexMap index)
{
    typename graph_traits< Graph >::vertex_descriptor v, w;
    typename graph_traits< Graph >::vertices_size_type b = 1;
    typename graph_traits< Graph >::out_edge_iterator edge_it2, edge_it2_end;
    typename graph_traits< Graph >::vertices_size_type index_i = index[i];
    std::vector< bool > rows_active(num_vertices(g), false);

    rows_active[index_i] = true;

    typename graph_traits< Graph >::vertex_iterator ui, ui_end;
    for (boost::tie(ui, ui_end) = vertices(g); ui != ui_end; ++ui)
    {
        v = *ui;
        if (index[v] <= index_i)
        {
            for (boost::tie(edge_it2, edge_it2_end) = out_edges(v, g);
                 edge_it2 != edge_it2_end; ++edge_it2)
            {
                w = target(*edge_it2, g);
                if ((index[w] >= index_i) && (!rows_active[index[w]]))
                {
                    b++;
                    rows_active[index[w]] = true;
                }
            }
        }
    }

    return b;
}

template < typename Graph >
typename graph_traits< Graph >::vertices_size_type ith_wavefront(
    typename graph_traits< Graph >::vertex_descriptor i, const Graph& g)
{
    return ith_wavefront(i, g, get(vertex_index, g));
}

template < typename Graph, typename VertexIndexMap >
typename graph_traits< Graph >::vertices_size_type max_wavefront(
    const Graph& g, VertexIndexMap index)
{
    BOOST_USING_STD_MAX();
    typename graph_traits< Graph >::vertices_size_type b = 0;
    typename graph_traits< Graph >::vertex_iterator i, end;
    for (boost::tie(i, end) = vertices(g); i != end; ++i)
        b = max BOOST_PREVENT_MACRO_SUBSTITUTION(
            b, ith_wavefront(*i, g, index));
    return b;
}

template < typename Graph >
typename graph_traits< Graph >::vertices_size_type max_wavefront(const Graph& g)
{
    return max_wavefront(g, get(vertex_index, g));
}

template < typename Graph, typename VertexIndexMap >
double aver_wavefront(const Graph& g, VertexIndexMap index)
{
    double b = 0;
    typename graph_traits< Graph >::vertex_iterator i, end;
    for (boost::tie(i, end) = vertices(g); i != end; ++i)
        b += ith_wavefront(*i, g, index);

    b /= num_vertices(g);
    return b;
}

template < typename Graph > double aver_wavefront(const Graph& g)
{
    return aver_wavefront(g, get(vertex_index, g));
}

template < typename Graph, typename VertexIndexMap >
double rms_wavefront(const Graph& g, VertexIndexMap index)
{
    double b = 0;
    typename graph_traits< Graph >::vertex_iterator i, end;
    for (boost::tie(i, end) = vertices(g); i != end; ++i)
        b += std::pow(double(ith_wavefront(*i, g, index)), 2.0);

    b /= num_vertices(g);

    return std::sqrt(b);
}

template < typename Graph > double rms_wavefront(const Graph& g)
{
    return rms_wavefront(g, get(vertex_index, g));
}

} // namespace boost

#endif // BOOST_GRAPH_WAVEFRONT_HPP
