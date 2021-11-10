// (C) Copyright 2009 Eric Bose-Wolf
//
// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0 (See accompanying file
// LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GRAPH_TRANSITIVE_REDUCTION_HPP
#define BOOST_GRAPH_TRANSITIVE_REDUCTION_HPP

#include <vector>
#include <algorithm> //std::find
#include <boost/concept/requires.hpp>
#include <boost/concept_check.hpp>

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/topological_sort.hpp>

// also I didn't got all of the concepts thin. Am I suppose to check
// for all concepts, which are needed for functions I call? (As if I
// wouldn't do that, the users would see the functions called by
// complaining about missings concepts, which would be clearly an error
// message revealing internal implementation and should therefore be avoided?)

// the pseudocode which I followed implementing this algorithmn was taken
// from the german book Algorithmische Graphentheorie by Volker Turau
// it is proposed to be of O(n + nm_red ) where n is the number
// of vertices and m_red is the number of edges in the transitive
// reduction, but I think my implementation spoiled this up at some point
// indicated below.

namespace boost
{

template < typename Graph, typename GraphTR, typename G_to_TR_VertexMap,
    typename VertexIndexMap >
BOOST_CONCEPT_REQUIRES(
    ((VertexListGraphConcept< Graph >))((IncidenceGraphConcept< Graph >))(
        (MutableGraphConcept< GraphTR >))(
        (ReadablePropertyMapConcept< VertexIndexMap,
            typename graph_traits< Graph >::vertex_descriptor >))(
        (Integer< typename property_traits< VertexIndexMap >::value_type >))(
        (LvaluePropertyMapConcept< G_to_TR_VertexMap,
            typename graph_traits< Graph >::vertex_descriptor >)),
    (void))
transitive_reduction(const Graph& g, GraphTR& tr, G_to_TR_VertexMap g_to_tr_map,
    VertexIndexMap g_index_map)
{
    typedef typename graph_traits< Graph >::vertex_descriptor Vertex;
    typedef typename graph_traits< Graph >::vertex_iterator VertexIterator;
    typedef typename std::vector< Vertex >::size_type size_type;

    std::vector< Vertex > topo_order;
    topological_sort(g, std::back_inserter(topo_order));

    std::vector< size_type > topo_number_storage(num_vertices(g));

    iterator_property_map< size_type*, VertexIndexMap, size_type, size_type& >
        topo_number(&topo_number_storage[0], g_index_map);

    {
        typename std::vector< Vertex >::reverse_iterator it
            = topo_order.rbegin();
        size_type n = 0;
        for (; it != topo_order.rend(); ++it, ++n)
        {
            topo_number[*it] = n;
        }
    }

    std::vector< std::vector< bool > > edge_in_closure(
        num_vertices(g), std::vector< bool >(num_vertices(g), false));
    {
        typename std::vector< Vertex >::reverse_iterator it
            = topo_order.rbegin();
        for (; it != topo_order.rend(); ++it)
        {
            g_to_tr_map[*it] = add_vertex(tr);
        }
    }

    typename std::vector< Vertex >::iterator it = topo_order.begin(),
                                             end = topo_order.end();
    for (; it != end; ++it)
    {
        size_type i = topo_number[*it];
        edge_in_closure[i][i] = true;
        std::vector< Vertex > neighbors;

        // I have to collect the successors of *it and traverse them in
        // ascending topological order. I didn't know a better way, how to
        // do that. So what I'm doint is, collection the successors of *it here
        {
            typename Graph::out_edge_iterator oi, oi_end;
            for (boost::tie(oi, oi_end) = out_edges(*it, g); oi != oi_end; ++oi)
            {
                neighbors.push_back(target(*oi, g));
            }
        }

        {
            // and run through all vertices in topological order
            typename std::vector< Vertex >::reverse_iterator rit
                = topo_order.rbegin(),
                rend = topo_order.rend();
            for (; rit != rend; ++rit)
            {
                // looking if they are successors of *it
                if (std::find(neighbors.begin(), neighbors.end(), *rit)
                    != neighbors.end())
                {
                    size_type j = topo_number[*rit];
                    if (not edge_in_closure[i][j])
                    {
                        for (size_type k = j; k < num_vertices(g); ++k)
                        {
                            if (not edge_in_closure[i][k])
                            {
                                // here we need edge_in_closure to be in
                                // topological order,
                                edge_in_closure[i][k] = edge_in_closure[j][k];
                            }
                        }
                        // therefore we only access edge_in_closure only through
                        // topo_number property_map
                        add_edge(g_to_tr_map[*it], g_to_tr_map[*rit], tr);
                    } // if ( not edge_in_
                } // if (find (
            } // for( typename vector<Vertex>::reverse_iterator
        } // {

    } // for( typename vector<Vertex>::iterator

} // void transitive_reduction

} // namespace boost

#endif
