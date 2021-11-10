// Copyright (C) 2006 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Douglas Gregor
//           Andrew Lumsdaine
#ifndef BOOST_GRAPH_DISTRIBUTED_ST_CONNECTED_HPP
#define BOOST_GRAPH_DISTRIBUTED_ST_CONNECTED_HPP

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/two_bit_color_map.hpp>
#include <boost/graph/iteration_macros.hpp>
#include <boost/pending/queue.hpp>

namespace boost
{
namespace graph
{

    template < typename Graph, typename ColorMap >
    bool st_connected(const Graph& g,
        typename graph_traits< Graph >::vertex_descriptor s,
        typename graph_traits< Graph >::vertex_descriptor t, ColorMap color)
    {
        typedef typename property_traits< ColorMap >::value_type Color;
        typedef color_traits< Color > ColorTraits;
        typedef typename graph_traits< Graph >::vertex_descriptor Vertex;

        // Set all vertices to white (unvisited)
        BGL_FORALL_VERTICES_T(v, g, Graph)
        put(color, v, ColorTraits::white());

        // Vertices found from the source are grey
        put(color, s, ColorTraits::gray());

        // Vertices found from the target are greeen
        put(color, t, ColorTraits::green());
        queue< Vertex > Q;
        Q.push(s);
        Q.push(t);

        while (!Q.empty())
        {
            Vertex u = Q.top();
            Q.pop();
            Color u_color = get(color, u);

            BGL_FORALL_OUTEDGES_T(u, e, g, Graph)
            {
                Vertex v = target(e, g);
                Color v_color = get(color, v);
                if (v_color == ColorTraits::white())
                {
                    // We have not seen "v" before; mark it with the same color
                    // as u
                    Color u_color = get(color, u);
                    put(color, v, u_color);

                    // Push it on the queue
                    Q.push(v);
                }
                else if (v_color != ColorTraits::black() && u_color != v_color)
                {
                    // Colors have collided. We're done!
                    return true;
                }
            }
            // u is done, so mark it black
            put(color, u, ColorTraits::black());
        }

        return false;
    }

    template < typename Graph >
    inline bool st_connected(const Graph& g,
        typename graph_traits< Graph >::vertex_descriptor s,
        typename graph_traits< Graph >::vertex_descriptor t)
    {
        return st_connected(g, s, t,
            make_two_bit_color_map(num_vertices(g), get(vertex_index, g)));
    }

}
} // end namespace boost::graph

#endif // BOOST_GRAPH_DISTRIBUTED_ST_CONNECTED_HPP
