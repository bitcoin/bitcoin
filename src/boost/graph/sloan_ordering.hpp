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

#ifndef BOOST_GRAPH_SLOAN_HPP
#define BOOST_GRAPH_SLOAN_HPP

#define WEIGHT1 1 // default weight for the distance in the Sloan algorithm
#define WEIGHT2 2 // default weight for the degree in the Sloan algorithm

#include <boost/config.hpp>
#include <vector>
#include <queue>
#include <algorithm>
#include <limits>
#include <boost/pending/queue.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/breadth_first_search.hpp>
#include <boost/graph/properties.hpp>
#include <boost/pending/indirect_cmp.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/graph/visitors.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/cuthill_mckee_ordering.hpp>

////////////////////////////////////////////////////////////
//
// Sloan-Algorithm for graph reordering
//(optimzes profile and wavefront, not primiraly bandwidth
//
////////////////////////////////////////////////////////////

namespace boost
{

/////////////////////////////////////////////////////////////////////////
// Function that returns the maximum depth of
// a rooted level strucutre (RLS)
//
/////////////////////////////////////////////////////////////////////////
template < class Distance > typename Distance::value_type RLS_depth(Distance& d)
{
    typename Distance::value_type h_s = 0;
    typename Distance::iterator iter;

    for (iter = d.begin(); iter != d.end(); ++iter)
    {
        if (*iter > h_s)
        {
            h_s = *iter;
        }
    }

    return h_s;
}

/////////////////////////////////////////////////////////////////////////
// Function that returns the width of the largest level of
// a rooted level strucutre (RLS)
//
/////////////////////////////////////////////////////////////////////////
template < class Distance, class my_int >
typename Distance::value_type RLS_max_width(Distance& d, my_int depth)
{

    typedef typename Distance::value_type Degree;

    // Searching for the maximum width of a level
    std::vector< Degree > dummy_width(depth + 1, 0);
    typename std::vector< Degree >::iterator my_it;
    typename Distance::iterator iter;
    Degree w_max = 0;

    for (iter = d.begin(); iter != d.end(); ++iter)
    {
        dummy_width[*iter]++;
    }

    for (my_it = dummy_width.begin(); my_it != dummy_width.end(); ++my_it)
    {
        if (*my_it > w_max)
            w_max = *my_it;
    }

    return w_max;
}

/////////////////////////////////////////////////////////////////////////
// Function for finding a good starting node for Sloan algorithm
//
// This is to find a good starting node. "good" is in the sense
// of the ordering generated.
/////////////////////////////////////////////////////////////////////////
template < class Graph, class ColorMap, class DegreeMap >
typename graph_traits< Graph >::vertex_descriptor sloan_start_end_vertices(
    Graph& G, typename graph_traits< Graph >::vertex_descriptor& s,
    ColorMap color, DegreeMap degree)
{
    typedef typename property_traits< DegreeMap >::value_type Degree;
    typedef typename graph_traits< Graph >::vertex_descriptor Vertex;
    typedef typename std::vector<
        typename graph_traits< Graph >::vertices_size_type >::iterator vec_iter;
    typedef typename graph_traits< Graph >::vertices_size_type size_type;

    typedef typename property_map< Graph, vertex_index_t >::const_type VertexID;

    s = *(vertices(G).first);
    Vertex e = s;
    Vertex i;
    Degree my_degree = get(degree, s);
    Degree dummy, h_i, h_s, w_i, w_e;
    bool new_start = true;
    Degree maximum_degree = 0;

    // Creating a std-vector for storing the distance from the start vertex in
    // dist
    std::vector< typename graph_traits< Graph >::vertices_size_type > dist(
        num_vertices(G), 0);

    // Wrap a property_map_iterator around the std::iterator
    boost::iterator_property_map< vec_iter, VertexID, size_type, size_type& >
        dist_pmap(dist.begin(), get(vertex_index, G));

    // Creating a property_map for the indices of a vertex
    typename property_map< Graph, vertex_index_t >::type index_map
        = get(vertex_index, G);

    // Creating a priority queue
    typedef indirect_cmp< DegreeMap, std::greater< Degree > > Compare;
    Compare comp(degree);
    std::priority_queue< Vertex, std::vector< Vertex >, Compare > degree_queue(
        comp);

    // step 1
    // Scan for the vertex with the smallest degree and the maximum degree
    typename graph_traits< Graph >::vertex_iterator ui, ui_end;
    for (boost::tie(ui, ui_end) = vertices(G); ui != ui_end; ++ui)
    {
        dummy = get(degree, *ui);

        if (dummy < my_degree)
        {
            my_degree = dummy;
            s = *ui;
        }

        if (dummy > maximum_degree)
        {
            maximum_degree = dummy;
        }
    }
    // end 1

    do
    {
        new_start = false; // Setting the loop repetition status to false

        // step 2
        // initialize the the disance std-vector with 0
        for (typename std::vector< typename graph_traits<
                 Graph >::vertices_size_type >::iterator iter
             = dist.begin();
             iter != dist.end(); ++iter)
            *iter = 0;

        // generating the RLS (rooted level structure)
        breadth_first_search(G, s,
            visitor(
                make_bfs_visitor(record_distances(dist_pmap, on_tree_edge()))));

        // end 2

        // step 3
        // calculating the depth of the RLS
        h_s = RLS_depth(dist);

        // step 4
        // pushing one node of each degree in an ascending manner into
        // degree_queue
        std::vector< bool > shrink_trace(maximum_degree, false);
        for (boost::tie(ui, ui_end) = vertices(G); ui != ui_end; ++ui)
        {
            dummy = get(degree, *ui);

            if ((dist[index_map[*ui]] == h_s) && (!shrink_trace[dummy]))
            {
                degree_queue.push(*ui);
                shrink_trace[dummy] = true;
            }
        }

        // end 3 & 4

        // step 5
        // Initializing w
        w_e = (std::numeric_limits< Degree >::max)();
        // end 5

        // step 6
        // Testing for termination
        while (!degree_queue.empty())
        {
            i = degree_queue.top(); // getting the node with the lowest degree
                                    // from the degree queue
            degree_queue.pop(); // ereasing the node with the lowest degree from
                                // the degree queue

            // generating a RLS
            for (typename std::vector< typename graph_traits<
                     Graph >::vertices_size_type >::iterator iter
                 = dist.begin();
                 iter != dist.end(); ++iter)
                *iter = 0;

            breadth_first_search(G, i,
                boost::visitor(make_bfs_visitor(
                    record_distances(dist_pmap, on_tree_edge()))));

            // Calculating depth and width of the rooted level
            h_i = RLS_depth(dist);
            w_i = RLS_max_width(dist, h_i);

            // Testing for termination
            if ((h_i > h_s) && (w_i < w_e))
            {
                h_s = h_i;
                s = i;
                while (!degree_queue.empty())
                    degree_queue.pop();
                new_start = true;
            }
            else if (w_i < w_e)
            {
                w_e = w_i;
                e = i;
            }
        }
        // end 6

    } while (new_start);

    return e;
}

//////////////////////////////////////////////////////////////////////////
// Sloan algorithm with a given starting Vertex.
//
// This algorithm requires user to provide a starting vertex to
// compute Sloan ordering.
//////////////////////////////////////////////////////////////////////////
template < class Graph, class OutputIterator, class ColorMap, class DegreeMap,
    class PriorityMap, class Weight >
OutputIterator sloan_ordering(Graph& g,
    typename graph_traits< Graph >::vertex_descriptor s,
    typename graph_traits< Graph >::vertex_descriptor e,
    OutputIterator permutation, ColorMap color, DegreeMap degree,
    PriorityMap priority, Weight W1, Weight W2)
{
    // typedef typename property_traits<DegreeMap>::value_type Degree;
    typedef typename property_traits< PriorityMap >::value_type Degree;
    typedef typename property_traits< ColorMap >::value_type ColorValue;
    typedef color_traits< ColorValue > Color;
    typedef typename graph_traits< Graph >::vertex_descriptor Vertex;
    typedef typename std::vector<
        typename graph_traits< Graph >::vertices_size_type >::iterator vec_iter;
    typedef typename graph_traits< Graph >::vertices_size_type size_type;

    typedef typename property_map< Graph, vertex_index_t >::const_type VertexID;

    // Creating a std-vector for storing the distance from the end vertex in it
    typename std::vector< typename graph_traits< Graph >::vertices_size_type >
        dist(num_vertices(g), 0);

    // Wrap a property_map_iterator around the std::iterator
    boost::iterator_property_map< vec_iter, VertexID, size_type, size_type& >
        dist_pmap(dist.begin(), get(vertex_index, g));

    breadth_first_search(g, e,
        visitor(make_bfs_visitor(record_distances(dist_pmap, on_tree_edge()))));

    // Creating a property_map for the indices of a vertex
    typename property_map< Graph, vertex_index_t >::type index_map
        = get(vertex_index, g);

    // Sets the color and priority to their initial status
    Degree cdeg;
    typename graph_traits< Graph >::vertex_iterator ui, ui_end;
    for (boost::tie(ui, ui_end) = vertices(g); ui != ui_end; ++ui)
    {
        put(color, *ui, Color::white());
        cdeg = get(degree, *ui) + 1;
        put(priority, *ui, W1 * dist[index_map[*ui]] - W2 * cdeg);
    }

    // Priority list
    typedef indirect_cmp< PriorityMap, std::greater< Degree > > Compare;
    Compare comp(priority);
    std::list< Vertex > priority_list;

    // Some more declarations
    typename graph_traits< Graph >::out_edge_iterator ei, ei_end, ei2, ei2_end;
    Vertex u, v, w;

    put(color, s,
        Color::green()); // Sets the color of the starting vertex to gray
    priority_list.push_front(s); // Puts s into the priority_list

    while (!priority_list.empty())
    {
        priority_list.sort(comp); // Orders the elements in the priority list in
                                  // an ascending manner

        u = priority_list
                .front(); // Accesses the last element in the priority list
        priority_list
            .pop_front(); // Removes the last element in the priority list

        if (get(color, u) == Color::green())
        {
            // for-loop over all out-edges of vertex u
            for (boost::tie(ei, ei_end) = out_edges(u, g); ei != ei_end; ++ei)
            {
                v = target(*ei, g);

                put(priority, v, get(priority, v) + W2); // updates the priority

                if (get(color, v)
                    == Color::white()) // test if the vertex is inactive
                {
                    put(color, v,
                        Color::green()); // giving the vertex a preactive status
                    priority_list.push_front(
                        v); // writing the vertex in the priority_queue
                }
            }
        }

        // Here starts step 8
        *permutation++
            = u; // Puts u to the first position in the permutation-vector
        put(color, u, Color::black()); // Gives u an inactive status

        // for loop over all the adjacent vertices of u
        for (boost::tie(ei, ei_end) = out_edges(u, g); ei != ei_end; ++ei)
        {

            v = target(*ei, g);

            if (get(color, v) == Color::green())
            { // tests if the vertex is inactive

                put(color, v,
                    Color::red()); // giving the vertex an active status
                put(priority, v, get(priority, v) + W2); // updates the priority

                // for loop over alll adjacent vertices of v
                for (boost::tie(ei2, ei2_end) = out_edges(v, g); ei2 != ei2_end;
                     ++ei2)
                {
                    w = target(*ei2, g);

                    if (get(color, w) != Color::black())
                    { // tests if vertex is postactive

                        put(priority, w,
                            get(priority, w) + W2); // updates the priority

                        if (get(color, w) == Color::white())
                        {

                            put(color, w, Color::green()); // gives the vertex a
                                                           // preactive status
                            priority_list.push_front(
                                w); // puts the vertex into the priority queue

                        } // end if

                    } // end if

                } // end for

            } // end if

        } // end for

    } // end while

    return permutation;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Same algorithm as before, but without the weights given (taking default
// weights
template < class Graph, class OutputIterator, class ColorMap, class DegreeMap,
    class PriorityMap >
OutputIterator sloan_ordering(Graph& g,
    typename graph_traits< Graph >::vertex_descriptor s,
    typename graph_traits< Graph >::vertex_descriptor e,
    OutputIterator permutation, ColorMap color, DegreeMap degree,
    PriorityMap priority)
{
    return sloan_ordering(
        g, s, e, permutation, color, degree, priority, WEIGHT1, WEIGHT2);
}

//////////////////////////////////////////////////////////////////////////
// Sloan algorithm without a given starting Vertex.
//
// This algorithm finds a good starting vertex itself to
// compute Sloan-ordering.
//////////////////////////////////////////////////////////////////////////

template < class Graph, class OutputIterator, class Color, class Degree,
    class Priority, class Weight >
inline OutputIterator sloan_ordering(Graph& G, OutputIterator permutation,
    Color color, Degree degree, Priority priority, Weight W1, Weight W2)
{
    typedef typename boost::graph_traits< Graph >::vertex_descriptor Vertex;

    Vertex s, e;
    e = sloan_start_end_vertices(G, s, color, degree);

    return sloan_ordering(
        G, s, e, permutation, color, degree, priority, W1, W2);
}

/////////////////////////////////////////////////////////////////////////////////////////
// Same as before, but without given weights (default weights are taken instead)
template < class Graph, class OutputIterator, class Color, class Degree,
    class Priority >
inline OutputIterator sloan_ordering(Graph& G, OutputIterator permutation,
    Color color, Degree degree, Priority priority)
{
    return sloan_ordering(
        G, permutation, color, degree, priority, WEIGHT1, WEIGHT2);
}

} // namespace boost

#endif // BOOST_GRAPH_SLOAN_HPP
