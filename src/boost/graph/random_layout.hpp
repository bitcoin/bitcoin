// Copyright 2004 The Trustees of Indiana University.

// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Douglas Gregor
//           Andrew Lumsdaine
#ifndef BOOST_GRAPH_RANDOM_LAYOUT_HPP
#define BOOST_GRAPH_RANDOM_LAYOUT_HPP

#include <boost/graph/graph_traits.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/uniform_01.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/type_traits/is_integral.hpp>
#include <boost/mpl/if.hpp>
#include <boost/graph/iteration_macros.hpp>

namespace boost
{

template < typename Topology, typename Graph, typename PositionMap >
void random_graph_layout(
    const Graph& g, PositionMap position_map, const Topology& topology)
{
    BGL_FORALL_VERTICES_T(v, g, Graph)
    {
        put(position_map, v, topology.random_point());
    }
}

} // end namespace boost

#endif // BOOST_GRAPH_RANDOM_LAYOUT_HPP
