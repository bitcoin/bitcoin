// Copyright 2004 The Trustees of Indiana University.

// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Douglas Gregor
//           Andrew Lumsdaine
#ifndef BOOST_GRAPH_CIRCLE_LAYOUT_HPP
#define BOOST_GRAPH_CIRCLE_LAYOUT_HPP
#include <boost/config/no_tr1/cmath.hpp>
#include <boost/math/constants/constants.hpp>
#include <utility>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/iteration_macros.hpp>
#include <boost/graph/topology.hpp>
#include <boost/static_assert.hpp>

namespace boost
{
/**
 * \brief Layout the graph with the vertices at the points of a regular
 * n-polygon.
 *
 * The distance from the center of the polygon to each point is
 * determined by the @p radius parameter. The @p position parameter
 * must be an Lvalue Property Map whose value type is a class type
 * containing @c x and @c y members that will be set to the @c x and
 * @c y coordinates.
 */
template < typename VertexListGraph, typename PositionMap, typename Radius >
void circle_graph_layout(
    const VertexListGraph& g, PositionMap position, Radius radius)
{
    BOOST_STATIC_ASSERT(
        property_traits< PositionMap >::value_type::dimensions >= 2);
    const double pi = boost::math::constants::pi< double >();

#ifndef BOOST_NO_STDC_NAMESPACE
    using std::cos;
    using std::sin;
#endif // BOOST_NO_STDC_NAMESPACE

    typedef typename graph_traits< VertexListGraph >::vertices_size_type
        vertices_size_type;

    vertices_size_type n = num_vertices(g);

    vertices_size_type i = 0;
    double two_pi_over_n = 2. * pi / n;
    BGL_FORALL_VERTICES_T(v, g, VertexListGraph)
    {
        position[v][0] = radius * cos(i * two_pi_over_n);
        position[v][1] = radius * sin(i * two_pi_over_n);
        ++i;
    }
}
} // end namespace boost

#endif // BOOST_GRAPH_CIRCLE_LAYOUT_HPP
