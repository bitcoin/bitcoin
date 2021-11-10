//
//=======================================================================
// Copyright 1997, 1998, 1999, 2000 University of Notre Dame.
// Authors: Andrew Lumsdaine, Lie-Quan Lee, Jeremy G. Siek
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================
//
#ifndef BOOST_GRAPH_TOPOLOGICAL_SORT_HPP
#define BOOST_GRAPH_TOPOLOGICAL_SORT_HPP

#include <boost/config.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/visitors.hpp>
#include <boost/graph/exception.hpp>
#include <boost/throw_exception.hpp>

namespace boost
{

// Topological sort visitor
//
// This visitor merely writes the linear ordering into an
// OutputIterator. The OutputIterator could be something like an
// ostream_iterator, or it could be a back/front_insert_iterator.
// Note that if it is a back_insert_iterator, the recorded order is
// the reverse topological order. On the other hand, if it is a
// front_insert_iterator, the recorded order is the topological
// order.
//
template < typename OutputIterator >
struct topo_sort_visitor : public dfs_visitor<>
{
    topo_sort_visitor(OutputIterator _iter) : m_iter(_iter) {}

    template < typename Edge, typename Graph >
    void back_edge(const Edge&, Graph&)
    {
        BOOST_THROW_EXCEPTION(not_a_dag());
    }

    template < typename Vertex, typename Graph >
    void finish_vertex(const Vertex& u, Graph&)
    {
        *m_iter++ = u;
    }

    OutputIterator m_iter;
};

// Topological Sort
//
// The topological sort algorithm creates a linear ordering
// of the vertices such that if edge (u,v) appears in the graph,
// then u comes before v in the ordering. The graph must
// be a directed acyclic graph (DAG). The implementation
// consists mainly of a call to depth-first search.
//

template < typename VertexListGraph, typename OutputIterator, typename P,
    typename T, typename R >
void topological_sort(VertexListGraph& g, OutputIterator result,
    const bgl_named_params< P, T, R >& params)
{
    typedef topo_sort_visitor< OutputIterator > TopoVisitor;
    depth_first_search(g, params.visitor(TopoVisitor(result)));
}

template < typename VertexListGraph, typename OutputIterator >
void topological_sort(VertexListGraph& g, OutputIterator result)
{
    topological_sort(
        g, result, bgl_named_params< int, buffer_param_t >(0)); // bogus
}

} // namespace boost

#endif /*BOOST_GRAPH_TOPOLOGICAL_SORT_H*/
