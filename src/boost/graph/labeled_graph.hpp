// Copyright (C) 2009 Andrew Sutton

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GRAPH_LABELED_GRAPH_HPP
#define BOOST_GRAPH_LABELED_GRAPH_HPP

#include <boost/config.hpp>
#include <vector>
#include <map>

#include <boost/static_assert.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/unordered_map.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/is_unsigned.hpp>
#include <boost/pending/container_traits.hpp>
#include <boost/graph/graph_traits.hpp>

// This file implements a utility for creating mappings from arbitrary
// identifiers to the vertices of a graph.

namespace boost
{

// A type selector that denotes the use of some default value.
struct defaultS
{
};

/** @internal */
namespace graph_detail
{
    /** Returns true if the selector is the default selector. */
    template < typename Selector >
    struct is_default : mpl::bool_< is_same< Selector, defaultS >::value >
    {
    };

    /**
     * Choose the default map instance. If Label is an unsigned integral type
     * the we can use a vector to store the information.
     */
    template < typename Label, typename Vertex > struct choose_default_map
    {
        typedef typename mpl::if_< is_unsigned< Label >, std::vector< Vertex >,
            std::map< Label, Vertex > // TODO: Should use unordered_map?
            >::type type;
    };

    /**
     * @name Generate Label Map
     * These type generators are responsible for instantiating an associative
     * container for the the labeled graph. Note that the Selector must be
     * select a pair associative container or a vecS, which is only valid if
     * Label is an integral type.
     */
    //@{
    template < typename Selector, typename Label, typename Vertex >
    struct generate_label_map
    {
    };

    template < typename Label, typename Vertex >
    struct generate_label_map< vecS, Label, Vertex >
    {
        typedef std::vector< Vertex > type;
    };

    template < typename Label, typename Vertex >
    struct generate_label_map< mapS, Label, Vertex >
    {
        typedef std::map< Label, Vertex > type;
    };

    template < typename Label, typename Vertex >
    struct generate_label_map< multimapS, Label, Vertex >
    {
        typedef std::multimap< Label, Vertex > type;
    };

    template < typename Label, typename Vertex >
    struct generate_label_map< hash_mapS, Label, Vertex >
    {
        typedef boost::unordered_map< Label, Vertex > type;
    };

    template < typename Label, typename Vertex >
    struct generate_label_map< hash_multimapS, Label, Vertex >
    {
        typedef boost::unordered_multimap< Label, Vertex > type;
    };

    template < typename Selector, typename Label, typename Vertex >
    struct choose_custom_map
    {
        typedef
            typename generate_label_map< Selector, Label, Vertex >::type type;
    };
    //@}

    /**
     * Choose and instantiate an "associative" container. Note that this can
     * also choose vector.
     */
    template < typename Selector, typename Label, typename Vertex >
    struct choose_map
    {
        typedef typename mpl::eval_if< is_default< Selector >,
            choose_default_map< Label, Vertex >,
            choose_custom_map< Selector, Label, Vertex > >::type type;
    };

    /** @name Insert Labeled Vertex */
    //@{
    // Tag dispatch on random access containers (i.e., vectors). This function
    // basically requires a) that Container is vector<Label> and that Label
    // is an unsigned integral value. Note that this will resize the vector
    // to accommodate indices.
    template < typename Container, typename Graph, typename Label,
        typename Prop >
    std::pair< typename graph_traits< Graph >::vertex_descriptor, bool >
    insert_labeled_vertex(Container& c, Graph& g, Label const& l, Prop const& p,
        random_access_container_tag)
    {
        typedef typename graph_traits< Graph >::vertex_descriptor Vertex;

        // If the label is out of bounds, resize the vector to accommodate.
        // Resize by 2x the index so we don't cause quadratic insertions over
        // time.
        if (l >= c.size())
        {
            c.resize((l + 1) * 2);
        }
        Vertex v = add_vertex(p, g);
        c[l] = v;
        return std::make_pair(c[l], true);
    }

    // Tag dispatch on multi associative containers (i.e. multimaps).
    template < typename Container, typename Graph, typename Label,
        typename Prop >
    std::pair< typename graph_traits< Graph >::vertex_descriptor, bool >
    insert_labeled_vertex(Container& c, Graph& g, Label const& l, Prop const& p,
        multiple_associative_container_tag const&)
    {
        // Note that insertion always succeeds so we can add the vertex first
        // and then the mapping to the label.
        typedef typename graph_traits< Graph >::vertex_descriptor Vertex;
        Vertex v = add_vertex(p, g);
        c.insert(std::make_pair(l, v));
        return std::make_pair(v, true);
    }

    // Tag dispatch on unique associative containers (i.e. maps).
    template < typename Container, typename Graph, typename Label,
        typename Prop >
    std::pair< typename graph_traits< Graph >::vertex_descriptor, bool >
    insert_labeled_vertex(Container& c, Graph& g, Label const& l, Prop const& p,
        unique_associative_container_tag)
    {
        // Here, we actually have to try the insertion first, and only add
        // the vertex if we get a new element.
        typedef typename graph_traits< Graph >::vertex_descriptor Vertex;
        typedef typename Container::iterator Iterator;
        std::pair< Iterator, bool > x = c.insert(std::make_pair(l, Vertex()));
        if (x.second)
        {
            x.first->second = add_vertex(g);
            put(boost::vertex_all, g, x.first->second, p);
        }
        return std::make_pair(x.first->second, x.second);
    }

    // Dispatcher
    template < typename Container, typename Graph, typename Label,
        typename Prop >
    std::pair< typename graph_traits< Graph >::vertex_descriptor, bool >
    insert_labeled_vertex(Container& c, Graph& g, Label const& l, Prop const& p)
    {
        return insert_labeled_vertex(c, g, l, p, container_category(c));
    }
    //@}

    /** @name Find Labeled Vertex */
    //@{
    // Tag dispatch for sequential maps (i.e., vectors).
    template < typename Container, typename Graph, typename Label >
    typename graph_traits< Graph >::vertex_descriptor find_labeled_vertex(
        Container const& c, Graph const&, Label const& l,
        random_access_container_tag)
    {
        return l < c.size() ? c[l] : graph_traits< Graph >::null_vertex();
    }

    // Tag dispatch for pair associative maps (more or less).
    template < typename Container, typename Graph, typename Label >
    typename graph_traits< Graph >::vertex_descriptor find_labeled_vertex(
        Container const& c, Graph const&, Label const& l,
        associative_container_tag)
    {
        typename Container::const_iterator i = c.find(l);
        return i != c.end() ? i->second : graph_traits< Graph >::null_vertex();
    }

    // Dispatcher
    template < typename Container, typename Graph, typename Label >
    typename graph_traits< Graph >::vertex_descriptor find_labeled_vertex(
        Container const& c, Graph const& g, Label const& l)
    {
        return find_labeled_vertex(c, g, l, container_category(c));
    }
    //@}

    /** @name Put Vertex Label */
    //@{
    // Tag dispatch on vectors.
    template < typename Container, typename Label, typename Graph,
        typename Vertex >
    bool put_vertex_label(Container& c, Graph const&, Label const& l, Vertex v,
        random_access_container_tag)
    {
        // If the element is already occupied, then we probably don't want to
        // overwrite it.
        if (c[l] == graph_traits< Graph >::null_vertex())
            return false;
        c[l] = v;
        return true;
    }

    // Attempt the insertion and return its result.
    template < typename Container, typename Label, typename Graph,
        typename Vertex >
    bool put_vertex_label(Container& c, Graph const&, Label const& l, Vertex v,
        unique_associative_container_tag)
    {
        return c.insert(std::make_pair(l, v)).second;
    }

    // Insert the pair and return true.
    template < typename Container, typename Label, typename Graph,
        typename Vertex >
    bool put_vertex_label(Container& c, Graph const&, Label const& l, Vertex v,
        multiple_associative_container_tag)
    {
        c.insert(std::make_pair(l, v));
        return true;
    }

    // Dispatcher
    template < typename Container, typename Label, typename Graph,
        typename Vertex >
    bool put_vertex_label(
        Container& c, Graph const& g, Label const& l, Vertex v)
    {
        return put_vertex_label(c, g, l, v, container_category(c));
    }
    //@}

} // namespace detail

struct labeled_graph_class_tag
{
};

/** @internal
 * This class is responsible for the deduction and declaration of type names
 * for the labeled_graph class template.
 */
template < typename Graph, typename Label, typename Selector >
struct labeled_graph_types
{
    typedef Graph graph_type;

    // Label and maps
    typedef Label label_type;
    typedef typename graph_detail::choose_map< Selector, Label,
        typename graph_traits< Graph >::vertex_descriptor >::type map_type;
};

/**
 * The labeled_graph class is a graph adaptor that maintains a mapping between
 * vertex labels and vertex descriptors.
 *
 * @todo This class is somewhat redundant for adjacency_list<*, vecS>  if
 * the intended label is an unsigned int (and perhaps some other cases), but
 * it does avoid some weird ambiguities (i.e. adding a vertex with a label that
 * does not match its target index).
 *
 * @todo This needs to be reconciled with the named_graph, but since there is
 * no documentation or examples, its not going to happen.
 */
template < typename Graph, typename Label, typename Selector = defaultS >
class labeled_graph : protected labeled_graph_types< Graph, Label, Selector >
{
    typedef labeled_graph_types< Graph, Label, Selector > Base;

public:
    typedef labeled_graph_class_tag graph_tag;

    typedef typename Base::graph_type graph_type;
    typedef typename graph_traits< graph_type >::vertex_descriptor
        vertex_descriptor;
    typedef
        typename graph_traits< graph_type >::edge_descriptor edge_descriptor;
    typedef typename graph_traits< graph_type >::directed_category
        directed_category;
    typedef typename graph_traits< graph_type >::edge_parallel_category
        edge_parallel_category;
    typedef typename graph_traits< graph_type >::traversal_category
        traversal_category;

    typedef typename graph_traits< graph_type >::out_edge_iterator
        out_edge_iterator;
    typedef
        typename graph_traits< graph_type >::in_edge_iterator in_edge_iterator;
    typedef typename graph_traits< graph_type >::adjacency_iterator
        adjacency_iterator;
    typedef
        typename graph_traits< graph_type >::degree_size_type degree_size_type;

    typedef
        typename graph_traits< graph_type >::vertex_iterator vertex_iterator;
    typedef typename graph_traits< graph_type >::vertices_size_type
        vertices_size_type;
    typedef typename graph_traits< graph_type >::edge_iterator edge_iterator;
    typedef
        typename graph_traits< graph_type >::edges_size_type edges_size_type;

    typedef typename graph_type::graph_property_type graph_property_type;
    typedef typename graph_type::graph_bundled graph_bundled;

    typedef typename graph_type::vertex_property_type vertex_property_type;
    typedef typename graph_type::vertex_bundled vertex_bundled;

    typedef typename graph_type::edge_property_type edge_property_type;
    typedef typename graph_type::edge_bundled edge_bundled;

    typedef typename Base::label_type label_type;
    typedef typename Base::map_type map_type;

public:
    labeled_graph(graph_property_type const& gp = graph_property_type())
    : _graph(gp), _map()
    {
    }

    labeled_graph(labeled_graph const& x) : _graph(x._graph), _map(x._map) {}

    // This constructor can only be used if map_type supports positional
    // range insertion (i.e. its a vector). This is the only case where we can
    // try to guess the intended labels for graph.
    labeled_graph(vertices_size_type n,
        graph_property_type const& gp = graph_property_type())
    : _graph(n, gp), _map()
    {
        std::pair< vertex_iterator, vertex_iterator > rng = vertices(_graph);
        _map.insert(_map.end(), rng.first, rng.second);
    }

    // Construct a graph over n vertices, each of which receives a label from
    // the range [l, l + n). Note that the graph is not directly constructed
    // over the n vertices, but added sequentially. This constructor is
    // necessarily slower than the underlying counterpart.
    template < typename LabelIter >
    labeled_graph(vertices_size_type n, LabelIter l,
        graph_property_type const& gp = graph_property_type())
    : _graph(gp)
    {
        while (n-- > 0)
            add_vertex(*l++);
    }

    // Construct the graph over n vertices each of which has a label in the
    // range [l, l + n) and a property in the range [p, p + n).
    template < typename LabelIter, typename PropIter >
    labeled_graph(vertices_size_type n, LabelIter l, PropIter p,
        graph_property_type const& gp = graph_property_type())
    : _graph(gp)
    {
        while (n-- > 0)
            add_vertex(*l++, *p++);
    }

    labeled_graph& operator=(labeled_graph const& x)
    {
        _graph = x._graph;
        _map = x._map;
        return *this;
    }

    /** @name Graph Accessors */
    //@{
    graph_type& graph() { return _graph; }
    graph_type const& graph() const { return _graph; }
    //@}

    /**
     * Create a new label for the given vertex, returning false, if the label
     * cannot be created.
     */
    bool label_vertex(vertex_descriptor v, Label const& l)
    {
        return graph_detail::put_vertex_label(_map, _graph, l, v);
    }

    /** @name Add Vertex
     * Add a vertex to the graph, returning the descriptor. If the vertices
     * are uniquely labeled and the label already exists within the graph,
     * then no vertex is added, and the returned descriptor refers to the
     * existing vertex. A vertex property can be given as a parameter, if
     * needed.
     */
    //@{
    vertex_descriptor add_vertex(Label const& l)
    {
        return graph_detail::insert_labeled_vertex(
            _map, _graph, l, vertex_property_type())
            .first;
    }

    vertex_descriptor add_vertex(Label const& l, vertex_property_type const& p)
    {
        return graph_detail::insert_labeled_vertex(_map, _graph, l, p).first;
    }
    //@}

    /** @name Insert Vertex
     * Insert a vertex into the graph, returning a pair containing the
     * descriptor of a vertex and a boolean value that describes whether or not
     * a new vertex was inserted. If vertices are not uniquely labeled, then
     * insertion will always succeed.
     */
    //@{
    std::pair< vertex_descriptor, bool > insert_vertex(Label const& l)
    {
        return graph_detail::insert_labeled_vertex(
            _map, _graph, l, vertex_property_type());
    }

    std::pair< vertex_descriptor, bool > insert_vertex(
        Label const& l, vertex_property_type const& p)
    {
        return graph_detail::insert_labeled_vertex(_map, _graph, l, p);
    }
    //@}

    /** Remove the vertex with the given label. */
    void remove_vertex(Label const& l)
    {
        return boost::remove_vertex(vertex(l), _graph);
    }

    /** Return a descriptor for the given label. */
    vertex_descriptor vertex(Label const& l) const
    {
        return graph_detail::find_labeled_vertex(_map, _graph, l);
    }

#ifndef BOOST_GRAPH_NO_BUNDLED_PROPERTIES
    /** @name Bundled Properties */
    //@{
    // Lookup the requested vertex and return the bundle.
    vertex_bundled& operator[](Label const& l) { return _graph[vertex(l)]; }

    vertex_bundled const& operator[](Label const& l) const
    {
        return _graph[vertex(l)];
    }

    // Delegate edge lookup to the underlying graph.
    edge_bundled& operator[](edge_descriptor e) { return _graph[e]; }

    edge_bundled const& operator[](edge_descriptor e) const
    {
        return _graph[e];
    }
    //@}
#endif

    /** Return a null descriptor */
    static vertex_descriptor null_vertex()
    {
        return graph_traits< graph_type >::null_vertex();
    }

private:
    graph_type _graph;
    map_type _map;
};

/**
 * The partial specialization over graph pointers allows the construction
 * of temporary labeled graph objects. In this case, the labels are destructed
 * when the wrapper goes out of scope.
 */
template < typename Graph, typename Label, typename Selector >
class labeled_graph< Graph*, Label, Selector >
: protected labeled_graph_types< Graph, Label, Selector >
{
    typedef labeled_graph_types< Graph, Label, Selector > Base;

public:
    typedef labeled_graph_class_tag graph_tag;

    typedef typename Base::graph_type graph_type;
    typedef typename graph_traits< graph_type >::vertex_descriptor
        vertex_descriptor;
    typedef
        typename graph_traits< graph_type >::edge_descriptor edge_descriptor;
    typedef typename graph_traits< graph_type >::directed_category
        directed_category;
    typedef typename graph_traits< graph_type >::edge_parallel_category
        edge_parallel_category;
    typedef typename graph_traits< graph_type >::traversal_category
        traversal_category;

    typedef typename graph_traits< graph_type >::out_edge_iterator
        out_edge_iterator;
    typedef
        typename graph_traits< graph_type >::in_edge_iterator in_edge_iterator;
    typedef typename graph_traits< graph_type >::adjacency_iterator
        adjacency_iterator;
    typedef
        typename graph_traits< graph_type >::degree_size_type degree_size_type;

    typedef
        typename graph_traits< graph_type >::vertex_iterator vertex_iterator;
    typedef typename graph_traits< graph_type >::vertices_size_type
        vertices_size_type;
    typedef typename graph_traits< graph_type >::edge_iterator edge_iterator;
    typedef
        typename graph_traits< graph_type >::edges_size_type edges_size_type;

    typedef typename graph_type::vertex_property_type vertex_property_type;
    typedef typename graph_type::edge_property_type edge_property_type;
    typedef typename graph_type::graph_property_type graph_property_type;
    typedef typename graph_type::vertex_bundled vertex_bundled;
    typedef typename graph_type::edge_bundled edge_bundled;

    typedef typename Base::label_type label_type;
    typedef typename Base::map_type map_type;

    labeled_graph(graph_type* g) : _graph(g) {}

    /** @name Graph Access */
    //@{
    graph_type& graph() { return *_graph; }
    graph_type const& graph() const { return *_graph; }
    //@}

    /**
     * Create a new label for the given vertex, returning false, if the label
     * cannot be created.
     */
    bool label_vertex(vertex_descriptor v, Label const& l)
    {
        return graph_detail::put_vertex_label(_map, *_graph, l, v);
    }

    /** @name Add Vertex */
    //@{
    vertex_descriptor add_vertex(Label const& l)
    {
        return graph_detail::insert_labeled_vertex(
            _map, *_graph, l, vertex_property_type())
            .first;
    }

    vertex_descriptor add_vertex(Label const& l, vertex_property_type const& p)
    {
        return graph_detail::insert_labeled_vertex(_map, *_graph, l, p).first;
    }

    std::pair< vertex_descriptor, bool > insert_vertex(Label const& l)
    {
        return graph_detail::insert_labeled_vertex(
            _map, *_graph, l, vertex_property_type());
    }
    //@}

    /** Try to insert a vertex with the given label. */
    std::pair< vertex_descriptor, bool > insert_vertex(
        Label const& l, vertex_property_type const& p)
    {
        return graph_detail::insert_labeled_vertex(_map, *_graph, l, p);
    }

    /** Remove the vertex with the given label. */
    void remove_vertex(Label const& l)
    {
        return boost::remove_vertex(vertex(l), *_graph);
    }

    /** Return a descriptor for the given label. */
    vertex_descriptor vertex(Label const& l) const
    {
        return graph_detail::find_labeled_vertex(_map, *_graph, l);
    }

#ifndef BOOST_GRAPH_NO_BUNDLED_PROPERTIES
    /** @name Bundled Properties */
    //@{
    // Lookup the requested vertex and return the bundle.
    vertex_bundled& operator[](Label const& l) { return (*_graph)[vertex(l)]; }

    vertex_bundled const& operator[](Label const& l) const
    {
        return (*_graph)[vertex(l)];
    }

    // Delegate edge lookup to the underlying graph.
    edge_bundled& operator[](edge_descriptor e) { return (*_graph)[e]; }

    edge_bundled const& operator[](edge_descriptor e) const
    {
        return (*_graph)[e];
    }
    //@}
#endif

    static vertex_descriptor null_vertex()
    {
        return graph_traits< graph_type >::null_vertex();
    }

private:
    graph_type* _graph;
    map_type _map;
};

#define LABELED_GRAPH_PARAMS typename G, typename L, typename S
#define LABELED_GRAPH labeled_graph< G, L, S >

/** @name Labeled Graph */
//@{
template < LABELED_GRAPH_PARAMS >
inline bool label_vertex(typename LABELED_GRAPH::vertex_descriptor v,
    typename LABELED_GRAPH::label_type const l, LABELED_GRAPH& g)
{
    return g.label_vertex(v, l);
}

template < LABELED_GRAPH_PARAMS >
inline typename LABELED_GRAPH::vertex_descriptor vertex_by_label(
    typename LABELED_GRAPH::label_type const l, LABELED_GRAPH& g)
{
    return g.vertex(l);
}
//@}

/** @name Graph */
//@{
template < LABELED_GRAPH_PARAMS >
inline std::pair< typename LABELED_GRAPH::edge_descriptor, bool > edge(
    typename LABELED_GRAPH::vertex_descriptor const& u,
    typename LABELED_GRAPH::vertex_descriptor const& v, LABELED_GRAPH const& g)
{
    return edge(u, v, g.graph());
}

// Labeled Extensions
template < LABELED_GRAPH_PARAMS >
inline std::pair< typename LABELED_GRAPH::edge_descriptor, bool > edge_by_label(
    typename LABELED_GRAPH::label_type const& u,
    typename LABELED_GRAPH::label_type const& v, LABELED_GRAPH const& g)
{
    return edge(g.vertex(u), g.vertex(v), g);
}
//@}

/** @name Incidence Graph */
//@{
template < LABELED_GRAPH_PARAMS >
inline std::pair< typename LABELED_GRAPH::out_edge_iterator,
    typename LABELED_GRAPH::out_edge_iterator >
out_edges(typename LABELED_GRAPH::vertex_descriptor v, LABELED_GRAPH const& g)
{
    return out_edges(v, g.graph());
}

template < LABELED_GRAPH_PARAMS >
inline typename LABELED_GRAPH::degree_size_type out_degree(
    typename LABELED_GRAPH::vertex_descriptor v, LABELED_GRAPH const& g)
{
    return out_degree(v, g.graph());
}

template < LABELED_GRAPH_PARAMS >
inline typename LABELED_GRAPH::vertex_descriptor source(
    typename LABELED_GRAPH::edge_descriptor e, LABELED_GRAPH const& g)
{
    return source(e, g.graph());
}

template < LABELED_GRAPH_PARAMS >
inline typename LABELED_GRAPH::vertex_descriptor target(
    typename LABELED_GRAPH::edge_descriptor e, LABELED_GRAPH const& g)
{
    return target(e, g.graph());
}
//@}

/** @name Bidirectional Graph */
//@{
template < LABELED_GRAPH_PARAMS >
inline std::pair< typename LABELED_GRAPH::in_edge_iterator,
    typename LABELED_GRAPH::in_edge_iterator >
in_edges(typename LABELED_GRAPH::vertex_descriptor v, LABELED_GRAPH const& g)
{
    return in_edges(v, g.graph());
}

template < LABELED_GRAPH_PARAMS >
inline typename LABELED_GRAPH::degree_size_type in_degree(
    typename LABELED_GRAPH::vertex_descriptor v, LABELED_GRAPH const& g)
{
    return in_degree(v, g.graph());
}

template < LABELED_GRAPH_PARAMS >
inline typename LABELED_GRAPH::degree_size_type degree(
    typename LABELED_GRAPH::vertex_descriptor v, LABELED_GRAPH const& g)
{
    return degree(v, g.graph());
}
//@}

/** @name Adjacency Graph */
//@{
template < LABELED_GRAPH_PARAMS >
inline std::pair< typename LABELED_GRAPH::adjacency_iterator,
    typename LABELED_GRAPH::adjacency_iterator >
adjacenct_vertices(
    typename LABELED_GRAPH::vertex_descriptor v, LABELED_GRAPH const& g)
{
    return adjacent_vertices(v, g.graph());
}
//@}

/** @name VertexListGraph */
//@{
template < LABELED_GRAPH_PARAMS >
inline std::pair< typename LABELED_GRAPH::vertex_iterator,
    typename LABELED_GRAPH::vertex_iterator >
vertices(LABELED_GRAPH const& g)
{
    return vertices(g.graph());
}

template < LABELED_GRAPH_PARAMS >
inline typename LABELED_GRAPH::vertices_size_type num_vertices(
    LABELED_GRAPH const& g)
{
    return num_vertices(g.graph());
}
//@}

/** @name EdgeListGraph */
//@{
template < LABELED_GRAPH_PARAMS >
inline std::pair< typename LABELED_GRAPH::edge_iterator,
    typename LABELED_GRAPH::edge_iterator >
edges(LABELED_GRAPH const& g)
{
    return edges(g.graph());
}

template < LABELED_GRAPH_PARAMS >
inline typename LABELED_GRAPH::edges_size_type num_edges(LABELED_GRAPH const& g)
{
    return num_edges(g.graph());
}
//@}

/** @internal @name Property Lookup */
//@{
namespace graph_detail
{
    struct labeled_graph_vertex_property_selector
    {
        template < class LabeledGraph, class Property, class Tag > struct bind_
        {
            typedef typename LabeledGraph::graph_type Graph;
            typedef property_map< Graph, Tag > PropertyMap;
            typedef typename PropertyMap::type type;
            typedef typename PropertyMap::const_type const_type;
        };
    };

    struct labeled_graph_edge_property_selector
    {
        template < class LabeledGraph, class Property, class Tag > struct bind_
        {
            typedef typename LabeledGraph::graph_type Graph;
            typedef property_map< Graph, Tag > PropertyMap;
            typedef typename PropertyMap::type type;
            typedef typename PropertyMap::const_type const_type;
        };
    };
}

template <> struct vertex_property_selector< labeled_graph_class_tag >
{
    typedef graph_detail::labeled_graph_vertex_property_selector type;
};

template <> struct edge_property_selector< labeled_graph_class_tag >
{
    typedef graph_detail::labeled_graph_edge_property_selector type;
};
//@}

/** @name Property Graph */
//@{
template < LABELED_GRAPH_PARAMS, typename Prop >
inline typename property_map< LABELED_GRAPH, Prop >::type get(
    Prop p, LABELED_GRAPH& g)
{
    return get(p, g.graph());
}

template < LABELED_GRAPH_PARAMS, typename Prop >
inline typename property_map< LABELED_GRAPH, Prop >::const_type get(
    Prop p, LABELED_GRAPH const& g)
{
    return get(p, g.graph());
}

template < LABELED_GRAPH_PARAMS, typename Prop, typename Key >
inline typename property_traits< typename property_map<
    typename LABELED_GRAPH::graph_type, Prop >::const_type >::value_type
get(Prop p, LABELED_GRAPH const& g, const Key& k)
{
    return get(p, g.graph(), k);
}

template < LABELED_GRAPH_PARAMS, typename Prop, typename Key, typename Value >
inline void put(Prop p, LABELED_GRAPH& g, Key const& k, Value const& v)
{
    put(p, g.graph(), k, v);
}
//@}

/** @name Mutable Graph */
//@{
template < LABELED_GRAPH_PARAMS >
inline std::pair< typename LABELED_GRAPH::edge_descriptor, bool > add_edge(
    typename LABELED_GRAPH::vertex_descriptor const& u,
    typename LABELED_GRAPH::vertex_descriptor const& v, LABELED_GRAPH& g)
{
    return add_edge(u, v, g.graph());
}

template < LABELED_GRAPH_PARAMS >
inline std::pair< typename LABELED_GRAPH::edge_descriptor, bool > add_edge(
    typename LABELED_GRAPH::vertex_descriptor const& u,
    typename LABELED_GRAPH::vertex_descriptor const& v,
    typename LABELED_GRAPH::edge_property_type const& p, LABELED_GRAPH& g)
{
    return add_edge(u, v, p, g.graph());
}

template < LABELED_GRAPH_PARAMS >
inline void clear_vertex(
    typename LABELED_GRAPH::vertex_descriptor v, LABELED_GRAPH& g)
{
    return clear_vertex(v, g.graph());
}

template < LABELED_GRAPH_PARAMS >
inline void remove_edge(
    typename LABELED_GRAPH::edge_descriptor e, LABELED_GRAPH& g)
{
    return remove_edge(e, g.graph());
}

template < LABELED_GRAPH_PARAMS >
inline void remove_edge(typename LABELED_GRAPH::vertex_descriptor u,
    typename LABELED_GRAPH::vertex_descriptor v, LABELED_GRAPH& g)
{
    return remove_edge(u, v, g.graph());
}

// Labeled extensions
template < LABELED_GRAPH_PARAMS >
inline std::pair< typename LABELED_GRAPH::edge_descriptor, bool >
add_edge_by_label(typename LABELED_GRAPH::label_type const& u,
    typename LABELED_GRAPH::label_type const& v, LABELED_GRAPH& g)
{
    return add_edge(g.vertex(u), g.vertex(v), g);
}

template < LABELED_GRAPH_PARAMS >
inline std::pair< typename LABELED_GRAPH::edge_descriptor, bool >
add_edge_by_label(typename LABELED_GRAPH::label_type const& u,
    typename LABELED_GRAPH::label_type const& v,
    typename LABELED_GRAPH::edge_property_type const& p, LABELED_GRAPH& g)
{
    return add_edge(g.vertex(u), g.vertex(v), p, g);
}

template < LABELED_GRAPH_PARAMS >
inline void clear_vertex_by_label(
    typename LABELED_GRAPH::label_type const& l, LABELED_GRAPH& g)
{
    clear_vertex(g.vertex(l), g.graph());
}

template < LABELED_GRAPH_PARAMS >
inline void remove_edge_by_label(typename LABELED_GRAPH::label_type const& u,
    typename LABELED_GRAPH::label_type const& v, LABELED_GRAPH& g)
{
    remove_edge(g.vertex(u), g.vertex(v), g.graph());
}
//@}

/** @name Labeled Mutable Graph
 * The labeled mutable graph hides the add_ and remove_ vertex functions from
 * the mutable graph concept. Note that the remove_vertex is hidden because
 * removing the vertex without its key could leave a dangling reference in
 * the map.
 */
//@{
template < LABELED_GRAPH_PARAMS >
inline typename LABELED_GRAPH::vertex_descriptor add_vertex(
    typename LABELED_GRAPH::label_type const& l, LABELED_GRAPH& g)
{
    return g.add_vertex(l);
}

// MutableLabeledPropertyGraph::add_vertex(l, vp, g)
template < LABELED_GRAPH_PARAMS >
inline typename LABELED_GRAPH::vertex_descriptor add_vertex(
    typename LABELED_GRAPH::label_type const& l,
    typename LABELED_GRAPH::vertex_property_type const& p, LABELED_GRAPH& g)
{
    return g.add_vertex(l, p);
}

template < LABELED_GRAPH_PARAMS >
inline void remove_vertex(
    typename LABELED_GRAPH::label_type const& l, LABELED_GRAPH& g)
{
    g.remove_vertex(l);
}
//@}

#undef LABELED_GRAPH_PARAMS
#undef LABELED_GRAPH

} // namespace boost

// Pull the labeled graph traits
#include <boost/graph/detail/labeled_graph_traits.hpp>

#endif // BOOST_GRAPH_LABELED_GRAPH_HPP
