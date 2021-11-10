// (C) Copyright 2007-2009 Andrew Sutton
//
// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0 (See accompanying file
// LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GRAPH_MATRIX_PROPERTY_MAP_HPP
#define BOOST_GRAPH_MATRIX_PROPERTY_MAP_HPP

#include <boost/graph/property_maps/container_property_map.hpp>

namespace boost
{
// This property map is built specifically for property maps over
// matrices. Like the basic property map over a container, this builds
// the property abstraction over a matrix (usually a vector of vectors)
// and returns property maps over the nested containers.
template < typename Graph, typename Key, typename Matrix >
struct matrix_property_map
: boost::put_get_helper<
      container_property_map< Graph, Key, typename Matrix::value_type >,
      matrix_property_map< Graph, Key, Matrix > >
{
    // abstract the indexing keys
    typedef typename detail::choose_indexer< Graph, Key >::indexer_type
        indexer_type;

    // aliases for the nested container and its corresponding map
    typedef typename Matrix::value_type container_type;
    typedef container_property_map< Graph, Key, container_type > map_type;

    typedef Key key_type;

    // This property map doesn't really provide access to nested containers,
    // but returns property maps over them. Since property maps are all
    // copy-constructible (or should be anyways), we never return references.
    // As such, this property is only readable, but not writable. Curiously,
    // the inner property map is actually an lvalue pmap.
    typedef map_type value_type;
    typedef map_type reference;
    typedef readable_property_map_tag category;

    matrix_property_map() : m_matrix(0), m_graph(0) {}

    matrix_property_map(Matrix& m, const Graph& g)
    : m_matrix(&m), m_graph(const_cast< Graph* >(&g))
    {
    }

    matrix_property_map(const matrix_property_map& x)
    : m_matrix(x.m_matrix), m_graph(x.m_graph)
    {
    }

    inline reference operator[](key_type k) const
    {
        typedef typename indexer_type::value_type Index;
        Index x = indexer_type::index(k, *m_graph);
        return map_type((*m_matrix)[x], *m_graph);
    }

private:
    mutable Matrix* m_matrix;
    mutable Graph* m_graph;
};
}

#endif
