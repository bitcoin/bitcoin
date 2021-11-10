//=======================================================================
// Copyright 2007 Aaron Windsor
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================
#ifndef __ADD_EDGE_VISITORS_HPP__
#define __ADD_EDGE_VISITORS_HPP__

#include <boost/property_map/property_map.hpp>

namespace boost
{

struct default_add_edge_visitor
{

    template < typename Graph, typename Vertex >
    void visit_vertex_pair(Vertex u, Vertex v, Graph& g)
    {
        add_edge(u, v, g);
    }
};

template < typename EdgeIndexMap > struct edge_index_update_visitor
{

    typedef
        typename property_traits< EdgeIndexMap >::value_type edge_index_value_t;

    edge_index_update_visitor(
        EdgeIndexMap em, edge_index_value_t next_index_available)
    : m_em(em), m_next_index(next_index_available)
    {
    }

    template < typename Graph, typename Vertex >
    void visit_vertex_pair(Vertex u, Vertex v, Graph& g)
    {
        typedef typename graph_traits< Graph >::edge_descriptor edge_t;
        std::pair< edge_t, bool > return_value = add_edge(u, v, g);
        if (return_value.second)
            put(m_em, return_value.first, m_next_index++);
    }

private:
    EdgeIndexMap m_em;
    edge_index_value_t m_next_index;
};

} // namespace boost

#endif //__ADD_EDGE_VISITORS_HPP__
