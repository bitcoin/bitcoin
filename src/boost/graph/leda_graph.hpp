//=======================================================================
// Copyright 1997, 1998, 1999, 2000 University of Notre Dame.
// Copyright 2004 The Trustees of Indiana University.
// Copyright 2007 University of Karlsruhe
// Authors: Andrew Lumsdaine, Lie-Quan Lee, Jeremy G. Siek, Douglas Gregor,
//          Jens Mueller
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================
#ifndef BOOST_GRAPH_LEDA_HPP
#define BOOST_GRAPH_LEDA_HPP

#include <boost/config.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/properties.hpp>

#include <LEDA/graph/graph.h>
#include <LEDA/graph/node_array.h>
#include <LEDA/graph/node_map.h>

// The functions and classes in this file allows the user to
// treat a LEDA GRAPH object as a boost graph "as is". No
// wrapper is needed for the GRAPH object.

// Warning: this implementation relies on partial specialization
// for the graph_traits class (so it won't compile with Visual C++)

// Warning: this implementation is in alpha and has not been tested

namespace boost
{

struct leda_graph_traversal_category : public virtual bidirectional_graph_tag,
                                       public virtual adjacency_graph_tag,
                                       public virtual vertex_list_graph_tag
{
};

template < class vtype, class etype >
struct graph_traits< leda::GRAPH< vtype, etype > >
{
    typedef leda::node vertex_descriptor;
    typedef leda::edge edge_descriptor;

    class adjacency_iterator
    : public iterator_facade< adjacency_iterator, leda::node,
          bidirectional_traversal_tag, leda::node, const leda::node* >
    {
    public:
        adjacency_iterator(
            leda::node node = 0, const leda::GRAPH< vtype, etype >* g = 0)
        : base(node), g(g)
        {
        }

    private:
        leda::node dereference() const { return leda::target(base); }

        bool equal(const adjacency_iterator& other) const
        {
            return base == other.base;
        }

        void increment() { base = g->adj_succ(base); }
        void decrement() { base = g->adj_pred(base); }

        leda::edge base;
        const leda::GRAPH< vtype, etype >* g;

        friend class iterator_core_access;
    };

    class out_edge_iterator
    : public iterator_facade< out_edge_iterator, leda::edge,
          bidirectional_traversal_tag, const leda::edge&, const leda::edge* >
    {
    public:
        out_edge_iterator(
            leda::node node = 0, const leda::GRAPH< vtype, etype >* g = 0)
        : base(node), g(g)
        {
        }

    private:
        const leda::edge& dereference() const { return base; }

        bool equal(const out_edge_iterator& other) const
        {
            return base == other.base;
        }

        void increment() { base = g->adj_succ(base); }
        void decrement() { base = g->adj_pred(base); }

        leda::edge base;
        const leda::GRAPH< vtype, etype >* g;

        friend class iterator_core_access;
    };

    class in_edge_iterator
    : public iterator_facade< in_edge_iterator, leda::edge,
          bidirectional_traversal_tag, const leda::edge&, const leda::edge* >
    {
    public:
        in_edge_iterator(
            leda::node node = 0, const leda::GRAPH< vtype, etype >* g = 0)
        : base(node), g(g)
        {
        }

    private:
        const leda::edge& dereference() const { return base; }

        bool equal(const in_edge_iterator& other) const
        {
            return base == other.base;
        }

        void increment() { base = g->in_succ(base); }
        void decrement() { base = g->in_pred(base); }

        leda::edge base;
        const leda::GRAPH< vtype, etype >* g;

        friend class iterator_core_access;
    };

    class vertex_iterator
    : public iterator_facade< vertex_iterator, leda::node,
          bidirectional_traversal_tag, const leda::node&, const leda::node* >
    {
    public:
        vertex_iterator(
            leda::node node = 0, const leda::GRAPH< vtype, etype >* g = 0)
        : base(node), g(g)
        {
        }

    private:
        const leda::node& dereference() const { return base; }

        bool equal(const vertex_iterator& other) const
        {
            return base == other.base;
        }

        void increment() { base = g->succ_node(base); }
        void decrement() { base = g->pred_node(base); }

        leda::node base;
        const leda::GRAPH< vtype, etype >* g;

        friend class iterator_core_access;
    };

    class edge_iterator
    : public iterator_facade< edge_iterator, leda::edge,
          bidirectional_traversal_tag, const leda::edge&, const leda::edge* >
    {
    public:
        edge_iterator(
            leda::edge edge = 0, const leda::GRAPH< vtype, etype >* g = 0)
        : base(edge), g(g)
        {
        }

    private:
        const leda::edge& dereference() const { return base; }

        bool equal(const edge_iterator& other) const
        {
            return base == other.base;
        }

        void increment() { base = g->succ_edge(base); }
        void decrement() { base = g->pred_edge(base); }

        leda::node base;
        const leda::GRAPH< vtype, etype >* g;

        friend class iterator_core_access;
    };

    typedef directed_tag directed_category;
    typedef allow_parallel_edge_tag edge_parallel_category; // not sure here
    typedef leda_graph_traversal_category traversal_category;
    typedef int vertices_size_type;
    typedef int edges_size_type;
    typedef int degree_size_type;
};

template <> struct graph_traits< leda::graph >
{
    typedef leda::node vertex_descriptor;
    typedef leda::edge edge_descriptor;

    class adjacency_iterator
    : public iterator_facade< adjacency_iterator, leda::node,
          bidirectional_traversal_tag, leda::node, const leda::node* >
    {
    public:
        adjacency_iterator(leda::edge edge = 0, const leda::graph* g = 0)
        : base(edge), g(g)
        {
        }

    private:
        leda::node dereference() const { return leda::target(base); }

        bool equal(const adjacency_iterator& other) const
        {
            return base == other.base;
        }

        void increment() { base = g->adj_succ(base); }
        void decrement() { base = g->adj_pred(base); }

        leda::edge base;
        const leda::graph* g;

        friend class iterator_core_access;
    };

    class out_edge_iterator
    : public iterator_facade< out_edge_iterator, leda::edge,
          bidirectional_traversal_tag, const leda::edge&, const leda::edge* >
    {
    public:
        out_edge_iterator(leda::edge edge = 0, const leda::graph* g = 0)
        : base(edge), g(g)
        {
        }

    private:
        const leda::edge& dereference() const { return base; }

        bool equal(const out_edge_iterator& other) const
        {
            return base == other.base;
        }

        void increment() { base = g->adj_succ(base); }
        void decrement() { base = g->adj_pred(base); }

        leda::edge base;
        const leda::graph* g;

        friend class iterator_core_access;
    };

    class in_edge_iterator
    : public iterator_facade< in_edge_iterator, leda::edge,
          bidirectional_traversal_tag, const leda::edge&, const leda::edge* >
    {
    public:
        in_edge_iterator(leda::edge edge = 0, const leda::graph* g = 0)
        : base(edge), g(g)
        {
        }

    private:
        const leda::edge& dereference() const { return base; }

        bool equal(const in_edge_iterator& other) const
        {
            return base == other.base;
        }

        void increment() { base = g->in_succ(base); }
        void decrement() { base = g->in_pred(base); }

        leda::edge base;
        const leda::graph* g;

        friend class iterator_core_access;
    };

    class vertex_iterator
    : public iterator_facade< vertex_iterator, leda::node,
          bidirectional_traversal_tag, const leda::node&, const leda::node* >
    {
    public:
        vertex_iterator(leda::node node = 0, const leda::graph* g = 0)
        : base(node), g(g)
        {
        }

    private:
        const leda::node& dereference() const { return base; }

        bool equal(const vertex_iterator& other) const
        {
            return base == other.base;
        }

        void increment() { base = g->succ_node(base); }
        void decrement() { base = g->pred_node(base); }

        leda::node base;
        const leda::graph* g;

        friend class iterator_core_access;
    };

    class edge_iterator
    : public iterator_facade< edge_iterator, leda::edge,
          bidirectional_traversal_tag, const leda::edge&, const leda::edge* >
    {
    public:
        edge_iterator(leda::edge edge = 0, const leda::graph* g = 0)
        : base(edge), g(g)
        {
        }

    private:
        const leda::edge& dereference() const { return base; }

        bool equal(const edge_iterator& other) const
        {
            return base == other.base;
        }

        void increment() { base = g->succ_edge(base); }
        void decrement() { base = g->pred_edge(base); }

        leda::edge base;
        const leda::graph* g;

        friend class iterator_core_access;
    };

    typedef directed_tag directed_category;
    typedef allow_parallel_edge_tag edge_parallel_category; // not sure here
    typedef leda_graph_traversal_category traversal_category;
    typedef int vertices_size_type;
    typedef int edges_size_type;
    typedef int degree_size_type;
};

} // namespace boost

namespace boost
{

//===========================================================================
// functions for GRAPH<vtype,etype>

template < class vtype, class etype >
typename graph_traits< leda::GRAPH< vtype, etype > >::vertex_descriptor source(
    typename graph_traits< leda::GRAPH< vtype, etype > >::edge_descriptor e,
    const leda::GRAPH< vtype, etype >& g)
{
    return source(e);
}

template < class vtype, class etype >
typename graph_traits< leda::GRAPH< vtype, etype > >::vertex_descriptor target(
    typename graph_traits< leda::GRAPH< vtype, etype > >::edge_descriptor e,
    const leda::GRAPH< vtype, etype >& g)
{
    return target(e);
}

template < class vtype, class etype >
inline std::pair<
    typename graph_traits< leda::GRAPH< vtype, etype > >::vertex_iterator,
    typename graph_traits< leda::GRAPH< vtype, etype > >::vertex_iterator >
vertices(const leda::GRAPH< vtype, etype >& g)
{
    typedef
        typename graph_traits< leda::GRAPH< vtype, etype > >::vertex_iterator
            Iter;
    return std::make_pair(Iter(g.first_node(), &g), Iter(0, &g));
}

template < class vtype, class etype >
inline std::pair<
    typename graph_traits< leda::GRAPH< vtype, etype > >::edge_iterator,
    typename graph_traits< leda::GRAPH< vtype, etype > >::edge_iterator >
edges(const leda::GRAPH< vtype, etype >& g)
{
    typedef typename graph_traits< leda::GRAPH< vtype, etype > >::edge_iterator
        Iter;
    return std::make_pair(Iter(g.first_edge(), &g), Iter(0, &g));
}

template < class vtype, class etype >
inline std::pair<
    typename graph_traits< leda::GRAPH< vtype, etype > >::out_edge_iterator,
    typename graph_traits< leda::GRAPH< vtype, etype > >::out_edge_iterator >
out_edges(
    typename graph_traits< leda::GRAPH< vtype, etype > >::vertex_descriptor u,
    const leda::GRAPH< vtype, etype >& g)
{
    typedef
        typename graph_traits< leda::GRAPH< vtype, etype > >::out_edge_iterator
            Iter;
    return std::make_pair(Iter(g.first_adj_edge(u, 0), &g), Iter(0, &g));
}

template < class vtype, class etype >
inline std::pair<
    typename graph_traits< leda::GRAPH< vtype, etype > >::in_edge_iterator,
    typename graph_traits< leda::GRAPH< vtype, etype > >::in_edge_iterator >
in_edges(
    typename graph_traits< leda::GRAPH< vtype, etype > >::vertex_descriptor u,
    const leda::GRAPH< vtype, etype >& g)
{
    typedef
        typename graph_traits< leda::GRAPH< vtype, etype > >::in_edge_iterator
            Iter;
    return std::make_pair(Iter(g.first_adj_edge(u, 1), &g), Iter(0, &g));
}

template < class vtype, class etype >
inline std::pair<
    typename graph_traits< leda::GRAPH< vtype, etype > >::adjacency_iterator,
    typename graph_traits< leda::GRAPH< vtype, etype > >::adjacency_iterator >
adjacent_vertices(
    typename graph_traits< leda::GRAPH< vtype, etype > >::vertex_descriptor u,
    const leda::GRAPH< vtype, etype >& g)
{
    typedef
        typename graph_traits< leda::GRAPH< vtype, etype > >::adjacency_iterator
            Iter;
    return std::make_pair(Iter(g.first_adj_edge(u, 0), &g), Iter(0, &g));
}

template < class vtype, class etype >
typename graph_traits< leda::GRAPH< vtype, etype > >::vertices_size_type
num_vertices(const leda::GRAPH< vtype, etype >& g)
{
    return g.number_of_nodes();
}

template < class vtype, class etype >
typename graph_traits< leda::GRAPH< vtype, etype > >::edges_size_type num_edges(
    const leda::GRAPH< vtype, etype >& g)
{
    return g.number_of_edges();
}

template < class vtype, class etype >
typename graph_traits< leda::GRAPH< vtype, etype > >::degree_size_type
out_degree(
    typename graph_traits< leda::GRAPH< vtype, etype > >::vertex_descriptor u,
    const leda::GRAPH< vtype, etype >& g)
{
    return g.outdeg(u);
}

template < class vtype, class etype >
typename graph_traits< leda::GRAPH< vtype, etype > >::degree_size_type
in_degree(
    typename graph_traits< leda::GRAPH< vtype, etype > >::vertex_descriptor u,
    const leda::GRAPH< vtype, etype >& g)
{
    return g.indeg(u);
}

template < class vtype, class etype >
typename graph_traits< leda::GRAPH< vtype, etype > >::degree_size_type degree(
    typename graph_traits< leda::GRAPH< vtype, etype > >::vertex_descriptor u,
    const leda::GRAPH< vtype, etype >& g)
{
    return g.outdeg(u) + g.indeg(u);
}

template < class vtype, class etype >
typename graph_traits< leda::GRAPH< vtype, etype > >::vertex_descriptor
add_vertex(leda::GRAPH< vtype, etype >& g)
{
    return g.new_node();
}

template < class vtype, class etype >
typename graph_traits< leda::GRAPH< vtype, etype > >::vertex_descriptor
add_vertex(const vtype& vp, leda::GRAPH< vtype, etype >& g)
{
    return g.new_node(vp);
}

template < class vtype, class etype >
void clear_vertex(
    typename graph_traits< leda::GRAPH< vtype, etype > >::vertex_descriptor u,
    leda::GRAPH< vtype, etype >& g)
{
    typename graph_traits< leda::GRAPH< vtype, etype > >::out_edge_iterator ei,
        ei_end;
    for (boost::tie(ei, ei_end) = out_edges(u, g); ei != ei_end; ei++)
        remove_edge(*ei);

    typename graph_traits< leda::GRAPH< vtype, etype > >::in_edge_iterator iei,
        iei_end;
    for (boost::tie(iei, iei_end) = in_edges(u, g); iei != iei_end; iei++)
        remove_edge(*iei);
}

template < class vtype, class etype >
void remove_vertex(
    typename graph_traits< leda::GRAPH< vtype, etype > >::vertex_descriptor u,
    leda::GRAPH< vtype, etype >& g)
{
    g.del_node(u);
}

template < class vtype, class etype >
std::pair<
    typename graph_traits< leda::GRAPH< vtype, etype > >::edge_descriptor,
    bool >
add_edge(
    typename graph_traits< leda::GRAPH< vtype, etype > >::vertex_descriptor u,
    typename graph_traits< leda::GRAPH< vtype, etype > >::vertex_descriptor v,
    leda::GRAPH< vtype, etype >& g)
{
    return std::make_pair(g.new_edge(u, v), true);
}

template < class vtype, class etype >
std::pair<
    typename graph_traits< leda::GRAPH< vtype, etype > >::edge_descriptor,
    bool >
add_edge(
    typename graph_traits< leda::GRAPH< vtype, etype > >::vertex_descriptor u,
    typename graph_traits< leda::GRAPH< vtype, etype > >::vertex_descriptor v,
    const etype& et, leda::GRAPH< vtype, etype >& g)
{
    return std::make_pair(g.new_edge(u, v, et), true);
}

template < class vtype, class etype >
void remove_edge(
    typename graph_traits< leda::GRAPH< vtype, etype > >::vertex_descriptor u,
    typename graph_traits< leda::GRAPH< vtype, etype > >::vertex_descriptor v,
    leda::GRAPH< vtype, etype >& g)
{
    typename graph_traits< leda::GRAPH< vtype, etype > >::out_edge_iterator i,
        iend;
    for (boost::tie(i, iend) = out_edges(u, g); i != iend; ++i)
        if (target(*i, g) == v)
            g.del_edge(*i);
}

template < class vtype, class etype >
void remove_edge(
    typename graph_traits< leda::GRAPH< vtype, etype > >::edge_descriptor e,
    leda::GRAPH< vtype, etype >& g)
{
    g.del_edge(e);
}

//===========================================================================
// functions for graph (non-templated version)

graph_traits< leda::graph >::vertex_descriptor source(
    graph_traits< leda::graph >::edge_descriptor e, const leda::graph& g)
{
    return source(e);
}

graph_traits< leda::graph >::vertex_descriptor target(
    graph_traits< leda::graph >::edge_descriptor e, const leda::graph& g)
{
    return target(e);
}

inline std::pair< graph_traits< leda::graph >::vertex_iterator,
    graph_traits< leda::graph >::vertex_iterator >
vertices(const leda::graph& g)
{
    typedef graph_traits< leda::graph >::vertex_iterator Iter;
    return std::make_pair(Iter(g.first_node(), &g), Iter(0, &g));
}

inline std::pair< graph_traits< leda::graph >::edge_iterator,
    graph_traits< leda::graph >::edge_iterator >
edges(const leda::graph& g)
{
    typedef graph_traits< leda::graph >::edge_iterator Iter;
    return std::make_pair(Iter(g.first_edge(), &g), Iter(0, &g));
}

inline std::pair< graph_traits< leda::graph >::out_edge_iterator,
    graph_traits< leda::graph >::out_edge_iterator >
out_edges(
    graph_traits< leda::graph >::vertex_descriptor u, const leda::graph& g)
{
    typedef graph_traits< leda::graph >::out_edge_iterator Iter;
    return std::make_pair(Iter(g.first_adj_edge(u), &g), Iter(0, &g));
}

inline std::pair< graph_traits< leda::graph >::in_edge_iterator,
    graph_traits< leda::graph >::in_edge_iterator >
in_edges(graph_traits< leda::graph >::vertex_descriptor u, const leda::graph& g)
{
    typedef graph_traits< leda::graph >::in_edge_iterator Iter;
    return std::make_pair(Iter(g.first_in_edge(u), &g), Iter(0, &g));
}

inline std::pair< graph_traits< leda::graph >::adjacency_iterator,
    graph_traits< leda::graph >::adjacency_iterator >
adjacent_vertices(
    graph_traits< leda::graph >::vertex_descriptor u, const leda::graph& g)
{
    typedef graph_traits< leda::graph >::adjacency_iterator Iter;
    return std::make_pair(Iter(g.first_adj_edge(u), &g), Iter(0, &g));
}

graph_traits< leda::graph >::vertices_size_type num_vertices(
    const leda::graph& g)
{
    return g.number_of_nodes();
}

graph_traits< leda::graph >::edges_size_type num_edges(const leda::graph& g)
{
    return g.number_of_edges();
}

graph_traits< leda::graph >::degree_size_type out_degree(
    graph_traits< leda::graph >::vertex_descriptor u, const leda::graph& g)
{
    return g.outdeg(u);
}

graph_traits< leda::graph >::degree_size_type in_degree(
    graph_traits< leda::graph >::vertex_descriptor u, const leda::graph& g)
{
    return g.indeg(u);
}

graph_traits< leda::graph >::degree_size_type degree(
    graph_traits< leda::graph >::vertex_descriptor u, const leda::graph& g)
{
    return g.outdeg(u) + g.indeg(u);
}

graph_traits< leda::graph >::vertex_descriptor add_vertex(leda::graph& g)
{
    return g.new_node();
}

void remove_edge(graph_traits< leda::graph >::vertex_descriptor u,
    graph_traits< leda::graph >::vertex_descriptor v, leda::graph& g)
{
    graph_traits< leda::graph >::out_edge_iterator i, iend;
    for (boost::tie(i, iend) = out_edges(u, g); i != iend; ++i)
        if (target(*i, g) == v)
            g.del_edge(*i);
}

void remove_edge(graph_traits< leda::graph >::edge_descriptor e, leda::graph& g)
{
    g.del_edge(e);
}

void clear_vertex(
    graph_traits< leda::graph >::vertex_descriptor u, leda::graph& g)
{
    graph_traits< leda::graph >::out_edge_iterator ei, ei_end;
    for (boost::tie(ei, ei_end) = out_edges(u, g); ei != ei_end; ei++)
        remove_edge(*ei, g);

    graph_traits< leda::graph >::in_edge_iterator iei, iei_end;
    for (boost::tie(iei, iei_end) = in_edges(u, g); iei != iei_end; iei++)
        remove_edge(*iei, g);
}

void remove_vertex(
    graph_traits< leda::graph >::vertex_descriptor u, leda::graph& g)
{
    g.del_node(u);
}

std::pair< graph_traits< leda::graph >::edge_descriptor, bool > add_edge(
    graph_traits< leda::graph >::vertex_descriptor u,
    graph_traits< leda::graph >::vertex_descriptor v, leda::graph& g)
{
    return std::make_pair(g.new_edge(u, v), true);
}

//===========================================================================
// property maps for GRAPH<vtype,etype>

class leda_graph_id_map : public put_get_helper< int, leda_graph_id_map >
{
public:
    typedef readable_property_map_tag category;
    typedef int value_type;
    typedef int reference;
    typedef leda::node key_type;
    leda_graph_id_map() {}
    template < class T > long operator[](T x) const { return x->id(); }
};
template < class vtype, class etype >
inline leda_graph_id_map get(
    vertex_index_t, const leda::GRAPH< vtype, etype >& g)
{
    return leda_graph_id_map();
}
template < class vtype, class etype >
inline leda_graph_id_map get(edge_index_t, const leda::GRAPH< vtype, etype >& g)
{
    return leda_graph_id_map();
}

template < class Tag > struct leda_property_map
{
};

template <> struct leda_property_map< vertex_index_t >
{
    template < class vtype, class etype > struct bind_
    {
        typedef leda_graph_id_map type;
        typedef leda_graph_id_map const_type;
    };
};
template <> struct leda_property_map< edge_index_t >
{
    template < class vtype, class etype > struct bind_
    {
        typedef leda_graph_id_map type;
        typedef leda_graph_id_map const_type;
    };
};

template < class Data, class DataRef, class GraphPtr >
class leda_graph_data_map : public put_get_helper< DataRef,
                                leda_graph_data_map< Data, DataRef, GraphPtr > >
{
public:
    typedef Data value_type;
    typedef DataRef reference;
    typedef void key_type;
    typedef lvalue_property_map_tag category;
    leda_graph_data_map(GraphPtr g) : m_g(g) {}
    template < class NodeOrEdge > DataRef operator[](NodeOrEdge x) const
    {
        return (*m_g)[x];
    }

protected:
    GraphPtr m_g;
};

template <> struct leda_property_map< vertex_all_t >
{
    template < class vtype, class etype > struct bind_
    {
        typedef leda_graph_data_map< vtype, vtype&,
            leda::GRAPH< vtype, etype >* >
            type;
        typedef leda_graph_data_map< vtype, const vtype&,
            const leda::GRAPH< vtype, etype >* >
            const_type;
    };
};
template < class vtype, class etype >
inline typename property_map< leda::GRAPH< vtype, etype >, vertex_all_t >::type
get(vertex_all_t, leda::GRAPH< vtype, etype >& g)
{
    typedef
        typename property_map< leda::GRAPH< vtype, etype >, vertex_all_t >::type
            pmap_type;
    return pmap_type(&g);
}
template < class vtype, class etype >
inline typename property_map< leda::GRAPH< vtype, etype >,
    vertex_all_t >::const_type
get(vertex_all_t, const leda::GRAPH< vtype, etype >& g)
{
    typedef typename property_map< leda::GRAPH< vtype, etype >,
        vertex_all_t >::const_type pmap_type;
    return pmap_type(&g);
}

template <> struct leda_property_map< edge_all_t >
{
    template < class vtype, class etype > struct bind_
    {
        typedef leda_graph_data_map< etype, etype&,
            leda::GRAPH< vtype, etype >* >
            type;
        typedef leda_graph_data_map< etype, const etype&,
            const leda::GRAPH< vtype, etype >* >
            const_type;
    };
};
template < class vtype, class etype >
inline typename property_map< leda::GRAPH< vtype, etype >, edge_all_t >::type
get(edge_all_t, leda::GRAPH< vtype, etype >& g)
{
    typedef
        typename property_map< leda::GRAPH< vtype, etype >, edge_all_t >::type
            pmap_type;
    return pmap_type(&g);
}
template < class vtype, class etype >
inline
    typename property_map< leda::GRAPH< vtype, etype >, edge_all_t >::const_type
    get(edge_all_t, const leda::GRAPH< vtype, etype >& g)
{
    typedef typename property_map< leda::GRAPH< vtype, etype >,
        edge_all_t >::const_type pmap_type;
    return pmap_type(&g);
}

// property map interface to the LEDA node_array class

template < class E, class ERef, class NodeMapPtr >
class leda_node_property_map
: public put_get_helper< ERef, leda_node_property_map< E, ERef, NodeMapPtr > >
{
public:
    typedef E value_type;
    typedef ERef reference;
    typedef leda::node key_type;
    typedef lvalue_property_map_tag category;
    leda_node_property_map(NodeMapPtr a) : m_array(a) {}
    ERef operator[](leda::node n) const { return (*m_array)[n]; }

protected:
    NodeMapPtr m_array;
};
template < class E >
leda_node_property_map< E, const E&, const leda::node_array< E >* >
make_leda_node_property_map(const leda::node_array< E >& a)
{
    typedef leda_node_property_map< E, const E&, const leda::node_array< E >* >
        pmap_type;
    return pmap_type(&a);
}
template < class E >
leda_node_property_map< E, E&, leda::node_array< E >* >
make_leda_node_property_map(leda::node_array< E >& a)
{
    typedef leda_node_property_map< E, E&, leda::node_array< E >* > pmap_type;
    return pmap_type(&a);
}

template < class E >
leda_node_property_map< E, const E&, const leda::node_map< E >* >
make_leda_node_property_map(const leda::node_map< E >& a)
{
    typedef leda_node_property_map< E, const E&, const leda::node_map< E >* >
        pmap_type;
    return pmap_type(&a);
}
template < class E >
leda_node_property_map< E, E&, leda::node_map< E >* >
make_leda_node_property_map(leda::node_map< E >& a)
{
    typedef leda_node_property_map< E, E&, leda::node_map< E >* > pmap_type;
    return pmap_type(&a);
}

// g++ 'enumeral_type' in template unification not implemented workaround
template < class vtype, class etype, class Tag >
struct property_map< leda::GRAPH< vtype, etype >, Tag >
{
    typedef typename leda_property_map< Tag >::template bind_< vtype, etype >
        map_gen;
    typedef typename map_gen::type type;
    typedef typename map_gen::const_type const_type;
};

template < class vtype, class etype, class PropertyTag, class Key >
inline typename boost::property_traits< typename boost::property_map<
    leda::GRAPH< vtype, etype >, PropertyTag >::const_type >::value_type
get(PropertyTag p, const leda::GRAPH< vtype, etype >& g, const Key& key)
{
    return get(get(p, g), key);
}

template < class vtype, class etype, class PropertyTag, class Key, class Value >
inline void put(PropertyTag p, leda::GRAPH< vtype, etype >& g, const Key& key,
    const Value& value)
{
    typedef
        typename property_map< leda::GRAPH< vtype, etype >, PropertyTag >::type
            Map;
    Map pmap = get(p, g);
    put(pmap, key, value);
}

// property map interface to the LEDA edge_array class

template < class E, class ERef, class EdgeMapPtr >
class leda_edge_property_map
: public put_get_helper< ERef, leda_edge_property_map< E, ERef, EdgeMapPtr > >
{
public:
    typedef E value_type;
    typedef ERef reference;
    typedef leda::edge key_type;
    typedef lvalue_property_map_tag category;
    leda_edge_property_map(EdgeMapPtr a) : m_array(a) {}
    ERef operator[](leda::edge n) const { return (*m_array)[n]; }

protected:
    EdgeMapPtr m_array;
};
template < class E >
leda_edge_property_map< E, const E&, const leda::edge_array< E >* >
make_leda_node_property_map(const leda::node_array< E >& a)
{
    typedef leda_edge_property_map< E, const E&, const leda::node_array< E >* >
        pmap_type;
    return pmap_type(&a);
}
template < class E >
leda_edge_property_map< E, E&, leda::edge_array< E >* >
make_leda_edge_property_map(leda::edge_array< E >& a)
{
    typedef leda_edge_property_map< E, E&, leda::edge_array< E >* > pmap_type;
    return pmap_type(&a);
}

template < class E >
leda_edge_property_map< E, const E&, const leda::edge_map< E >* >
make_leda_edge_property_map(const leda::edge_map< E >& a)
{
    typedef leda_edge_property_map< E, const E&, const leda::edge_map< E >* >
        pmap_type;
    return pmap_type(&a);
}
template < class E >
leda_edge_property_map< E, E&, leda::edge_map< E >* >
make_leda_edge_property_map(leda::edge_map< E >& a)
{
    typedef leda_edge_property_map< E, E&, leda::edge_map< E >* > pmap_type;
    return pmap_type(&a);
}

} // namespace boost

#endif // BOOST_GRAPH_LEDA_HPP
