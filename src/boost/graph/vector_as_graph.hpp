//=======================================================================
// Copyright 1997, 1998, 1999, 2000 University of Notre Dame.
// Copyright 2006 The Trustees of Indiana University.
// Copyright (C) 2001 Vladimir Prus <ghost@cs.msu.su>
// Authors: Andrew Lumsdaine, Lie-Quan Lee, Jeremy G. Siek, Douglas Gregor
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

// The mutating functions (add_edge, etc.) were added by Vladimir Prus.

#ifndef BOOST_VECTOR_AS_GRAPH_HPP
#define BOOST_VECTOR_AS_GRAPH_HPP

#include <cassert>
#include <utility>
#include <vector>
#include <cstddef>
#include <iterator>
#include <boost/iterator/counting_iterator.hpp>
#include <boost/range/irange.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/graph/properties.hpp>
#include <algorithm>

/*
  This module implements the VertexListGraph concept using a
  std::vector as the "back-bone" of the graph (the vector *is* the
  graph object). The edge-lists type of the graph is templated, so the
  user can choose any STL container, so long as the value_type of the
  container is convertible to the size_type of the vector. For now any
  graph properties must be stored seperately.

  This module requires the C++ compiler to support partial
  specialization for the graph_traits class, so this is not portable
  to VC++.

*/

namespace boost
{
namespace detail
{
    template < class EdgeList > struct val_out_edge_ret;
    template < class EdgeList > struct val_out_edge_iter;
    template < class EdgeList > struct val_edge;
}
}

namespace boost
{

struct vector_as_graph_traversal_tag : public vertex_list_graph_tag,
                                       public adjacency_graph_tag,
                                       public incidence_graph_tag
{
};

template < class EdgeList > struct graph_traits< std::vector< EdgeList > >
{
    typedef typename EdgeList::value_type V;
    typedef V vertex_descriptor;
    typedef typename detail::val_edge< EdgeList >::type edge_descriptor;
    typedef typename EdgeList::const_iterator adjacency_iterator;
    typedef
        typename detail::val_out_edge_iter< EdgeList >::type out_edge_iterator;
    typedef void in_edge_iterator;
    typedef void edge_iterator;
    typedef counting_iterator< V > vertex_iterator;
    typedef directed_tag directed_category;
    typedef allow_parallel_edge_tag edge_parallel_category;
    typedef vector_as_graph_traversal_tag traversal_category;
    typedef typename std::vector< EdgeList >::size_type vertices_size_type;
    typedef void edges_size_type;
    typedef typename EdgeList::size_type degree_size_type;
    static V null_vertex() { return V(-1); }
};
template < class EdgeList > struct edge_property_type< std::vector< EdgeList > >
{
    typedef void type;
};
template < class EdgeList >
struct vertex_property_type< std::vector< EdgeList > >
{
    typedef void type;
};
template < class EdgeList >
struct graph_property_type< std::vector< EdgeList > >
{
    typedef void type;
};
}

namespace boost
{

namespace detail
{

    // "val" is short for Vector Adjacency List

    template < class EdgeList > struct val_edge
    {
        typedef typename EdgeList::value_type V;
        typedef std::pair< V, V > type;
    };

    // need rewrite this using boost::iterator_adaptor
    template < class V, class Iter > class val_out_edge_iterator
    {
        typedef val_out_edge_iterator self;
        typedef std::pair< V, V > Edge;

    public:
        typedef std::input_iterator_tag iterator_category;
        typedef std::pair< V, V > value_type;
        typedef std::ptrdiff_t difference_type;
        typedef std::pair< V, V >* pointer;
        typedef const std::pair< V, V > reference;
        val_out_edge_iterator() {}
        val_out_edge_iterator(V s, Iter i) : _source(s), _iter(i) {}
        Edge operator*() const { return Edge(_source, *_iter); }
        self& operator++()
        {
            ++_iter;
            return *this;
        }
        self operator++(int)
        {
            self t = *this;
            ++_iter;
            return t;
        }
        bool operator==(const self& x) const { return _iter == x._iter; }
        bool operator!=(const self& x) const { return _iter != x._iter; }

    protected:
        V _source;
        Iter _iter;
    };

    template < class EdgeList > struct val_out_edge_iter
    {
        typedef typename EdgeList::value_type V;
        typedef typename EdgeList::const_iterator Iter;
        typedef val_out_edge_iterator< V, Iter > type;
    };

    template < class EdgeList > struct val_out_edge_ret
    {
        typedef typename val_out_edge_iter< EdgeList >::type IncIter;
        typedef std::pair< IncIter, IncIter > type;
    };

} // namesapce detail

template < class EdgeList, class Alloc >
typename detail::val_out_edge_ret< EdgeList >::type out_edges(
    typename EdgeList::value_type v, const std::vector< EdgeList, Alloc >& g)
{
    typedef typename detail::val_out_edge_iter< EdgeList >::type Iter;
    typedef typename detail::val_out_edge_ret< EdgeList >::type return_type;
    return return_type(Iter(v, g[v].begin()), Iter(v, g[v].end()));
}

template < class EdgeList, class Alloc >
typename EdgeList::size_type out_degree(
    typename EdgeList::value_type v, const std::vector< EdgeList, Alloc >& g)
{
    return g[v].size();
}

template < class EdgeList, class Alloc >
std::pair< typename EdgeList::const_iterator,
    typename EdgeList::const_iterator >
adjacent_vertices(
    typename EdgeList::value_type v, const std::vector< EdgeList, Alloc >& g)
{
    return std::make_pair(g[v].begin(), g[v].end());
}

// source() and target() already provided for pairs in graph_traits.hpp

template < class EdgeList, class Alloc >
std::pair< boost::counting_iterator< typename EdgeList::value_type >,
    boost::counting_iterator< typename EdgeList::value_type > >
vertices(const std::vector< EdgeList, Alloc >& v)
{
    typedef boost::counting_iterator< typename EdgeList::value_type > Iter;
    return std::make_pair(Iter(0), Iter(v.size()));
}

template < class EdgeList, class Alloc >
typename std::vector< EdgeList, Alloc >::size_type num_vertices(
    const std::vector< EdgeList, Alloc >& v)
{
    return v.size();
}

template < class EdgeList, class Allocator >
typename std::pair< typename detail::val_edge< EdgeList >::type, bool >
add_edge(typename EdgeList::value_type u, typename EdgeList::value_type v,
    std::vector< EdgeList, Allocator >& g)
{
    typedef typename detail::val_edge< EdgeList >::type edge_type;
    g[u].insert(g[u].end(), v);
    return std::make_pair(edge_type(u, v), true);
}

template < class EdgeList, class Allocator >
typename std::pair< typename detail::val_edge< EdgeList >::type, bool > edge(
    typename EdgeList::value_type u, typename EdgeList::value_type v,
    std::vector< EdgeList, Allocator >& g)
{
    typedef typename detail::val_edge< EdgeList >::type edge_type;
    typename EdgeList::iterator i = g[u].begin(), end = g[u].end();
    for (; i != end; ++i)
        if (*i == v)
            return std::make_pair(edge_type(u, v), true);
    return std::make_pair(edge_type(), false);
}

template < class EdgeList, class Allocator >
void remove_edge(typename EdgeList::value_type u,
    typename EdgeList::value_type v, std::vector< EdgeList, Allocator >& g)
{
    typename EdgeList::iterator i = std::remove(g[u].begin(), g[u].end(), v);
    if (i != g[u].end())
        g[u].erase(i, g[u].end());
}

template < class EdgeList, class Allocator >
void remove_edge(typename detail::val_edge< EdgeList >::type e,
    std::vector< EdgeList, Allocator >& g)
{
    typename EdgeList::value_type u, v;
    u = e.first;
    v = e.second;
    // FIXME: edge type does not fully specify the edge to be deleted
    typename EdgeList::iterator i = std::remove(g[u].begin(), g[u].end(), v);
    if (i != g[u].end())
        g[u].erase(i, g[u].end());
}

template < class EdgeList, class Allocator, class Predicate >
void remove_edge_if(Predicate p, std::vector< EdgeList, Allocator >& g)
{
    for (std::size_t u = 0; u < g.size(); ++u)
    {
        // Oops! gcc gets internal compiler error on compose_.......

        typedef typename EdgeList::iterator iterator;
        iterator b = g[u].begin(), e = g[u].end();

        if (!g[u].empty())
        {

            for (; b != e;)
            {
                if (p(std::make_pair(u, *b)))
                {
                    --e;
                    if (b == e)
                        break;
                    else
                        iter_swap(b, e);
                }
                else
                {
                    ++b;
                }
            }
        }

        if (e != g[u].end())
            g[u].erase(e, g[u].end());
    }
}

template < class EdgeList, class Allocator >
typename EdgeList::value_type add_vertex(std::vector< EdgeList, Allocator >& g)
{
    g.resize(g.size() + 1);
    return g.size() - 1;
}

template < class EdgeList, class Allocator >
void clear_vertex(
    typename EdgeList::value_type u, std::vector< EdgeList, Allocator >& g)
{
    g[u].clear();
    for (std::size_t i = 0; i < g.size(); ++i)
        remove_edge(i, u, g);
}

template < class EdgeList, class Allocator >
void remove_vertex(
    typename EdgeList::value_type u, std::vector< EdgeList, Allocator >& g)
{
    typedef typename EdgeList::iterator iterator;
    clear_vertex(u, g);
    g.erase(g.begin() + u);
    for (std::size_t i = 0; i < g.size(); ++i)
        for (iterator it = g[i].begin(); it != g[i].end(); ++it)
            // after clear_vertex *it is never equal to u
            if (*it > u)
                --*it;
}

template < typename EdgeList, typename Allocator >
struct property_map< std::vector< EdgeList, Allocator >, vertex_index_t >
{
    typedef identity_property_map type;
    typedef type const_type;
};

template < typename EdgeList, typename Allocator >
identity_property_map get(
    vertex_index_t, const std::vector< EdgeList, Allocator >&)
{
    return identity_property_map();
}

template < typename EdgeList, typename Allocator >
identity_property_map get(vertex_index_t, std::vector< EdgeList, Allocator >&)
{
    return identity_property_map();
}
} // namespace boost

#endif // BOOST_VECTOR_AS_GRAPH_HPP
