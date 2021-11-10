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

#ifndef BOOST_GRAPH_PROFILE_HPP
#define BOOST_GRAPH_PROFILE_HPP

#include <boost/graph/graph_traits.hpp>
#include <boost/detail/numeric_traits.hpp>
#include <boost/graph/bandwidth.hpp>

namespace boost
{

template < typename Graph, typename VertexIndexMap >
typename graph_traits< Graph >::vertices_size_type profile(
    const Graph& g, VertexIndexMap index)
{
    typename graph_traits< Graph >::vertices_size_type b = 0;
    typename graph_traits< Graph >::vertex_iterator i, end;
    for (boost::tie(i, end) = vertices(g); i != end; ++i)
    {
        b += ith_bandwidth(*i, g, index) + 1;
    }

    return b;
}

template < typename Graph >
typename graph_traits< Graph >::vertices_size_type profile(const Graph& g)
{
    return profile(g, get(vertex_index, g));
}

} // namespace boost

#endif // BOOST_GRAPH_PROFILE_HPP
