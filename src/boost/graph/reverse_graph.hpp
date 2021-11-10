//  (C) Copyright David Abrahams 2000.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef REVERSE_GRAPH_DWA092300_H_
#define REVERSE_GRAPH_DWA092300_H_

#include <boost/graph/adjacency_iterator.hpp>
#include <boost/graph/properties.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/type_traits.hpp>
#include <boost/mpl/if.hpp>

namespace boost
{

struct reverse_graph_tag
{
};

namespace detail
{

    template < typename EdgeDesc > class reverse_graph_edge_descriptor
    {
    public:
        EdgeDesc
            underlying_descx; // Odd name is because this needs to be public but
                              // shouldn't be exposed to users anymore

    private:
        typedef EdgeDesc base_descriptor_type;

    public:
        explicit reverse_graph_edge_descriptor(
            const EdgeDesc& underlying_descx = EdgeDesc())
        : underlying_descx(underlying_descx)
        {
        }

        friend bool operator==(const reverse_graph_edge_descriptor& a,
            const reverse_graph_edge_descriptor& b)
        {
            return a.underlying_descx == b.underlying_descx;
        }
        friend bool operator!=(const reverse_graph_edge_descriptor& a,
            const reverse_graph_edge_descriptor& b)
        {
            return a.underlying_descx != b.underlying_descx;
        }
        friend bool operator<(const reverse_graph_edge_descriptor& a,
            const reverse_graph_edge_descriptor& b)
        {
            return a.underlying_descx < b.underlying_descx;
        }
        friend bool operator>(const reverse_graph_edge_descriptor& a,
            const reverse_graph_edge_descriptor& b)
        {
            return a.underlying_descx > b.underlying_descx;
        }
        friend bool operator<=(const reverse_graph_edge_descriptor& a,
            const reverse_graph_edge_descriptor& b)
        {
            return a.underlying_descx <= b.underlying_descx;
        }
        friend bool operator>=(const reverse_graph_edge_descriptor& a,
            const reverse_graph_edge_descriptor& b)
        {
            return a.underlying_descx >= b.underlying_descx;
        }
    };

    template < typename EdgeDesc > struct reverse_graph_edge_descriptor_maker
    {
        typedef reverse_graph_edge_descriptor< EdgeDesc > result_type;

        reverse_graph_edge_descriptor< EdgeDesc > operator()(
            const EdgeDesc& ed) const
        {
            return reverse_graph_edge_descriptor< EdgeDesc >(ed);
        }
    };

    template < typename EdgeDesc, typename Iter >
    std::pair< transform_iterator<
                   reverse_graph_edge_descriptor_maker< EdgeDesc >, Iter >,
        transform_iterator< reverse_graph_edge_descriptor_maker< EdgeDesc >,
            Iter > >
    reverse_edge_iter_pair(const std::pair< Iter, Iter >& ip)
    {
        return std::make_pair(
            make_transform_iterator(
                ip.first, reverse_graph_edge_descriptor_maker< EdgeDesc >()),
            make_transform_iterator(
                ip.second, reverse_graph_edge_descriptor_maker< EdgeDesc >()));
    }

    // Get the underlying descriptor from a vertex or edge descriptor
    template < typename Desc >
    struct get_underlying_descriptor_from_reverse_descriptor
    {
        typedef Desc type;
        static Desc convert(const Desc& d) { return d; }
    };
    template < typename Desc >
    struct get_underlying_descriptor_from_reverse_descriptor<
        reverse_graph_edge_descriptor< Desc > >
    {
        typedef Desc type;
        static Desc convert(const reverse_graph_edge_descriptor< Desc >& d)
        {
            return d.underlying_descx;
        }
    };

    template < bool isEdgeList > struct choose_rev_edge_iter
    {
    };
    template <> struct choose_rev_edge_iter< true >
    {
        template < class G > struct bind_
        {
            typedef transform_iterator<
                reverse_graph_edge_descriptor_maker<
                    typename graph_traits< G >::edge_descriptor >,
                typename graph_traits< G >::edge_iterator >
                type;
        };
    };
    template <> struct choose_rev_edge_iter< false >
    {
        template < class G > struct bind_
        {
            typedef void type;
        };
    };

} // namespace detail

template < class BidirectionalGraph,
    class GraphRef = const BidirectionalGraph& >
class reverse_graph
{
    typedef reverse_graph< BidirectionalGraph, GraphRef > Self;
    typedef graph_traits< BidirectionalGraph > Traits;

public:
    typedef BidirectionalGraph base_type;
    typedef GraphRef base_ref_type;

    // Constructor
    reverse_graph(GraphRef g) : m_g(g) {}
    // Conversion from reverse_graph on non-const reference to one on const
    // reference
    reverse_graph(
        const reverse_graph< BidirectionalGraph, BidirectionalGraph& >& o)
    : m_g(o.m_g)
    {
    }

    // Graph requirements
    typedef typename Traits::vertex_descriptor vertex_descriptor;
    typedef detail::reverse_graph_edge_descriptor<
        typename Traits::edge_descriptor >
        edge_descriptor;
    typedef typename Traits::directed_category directed_category;
    typedef typename Traits::edge_parallel_category edge_parallel_category;
    typedef typename Traits::traversal_category traversal_category;

    // IncidenceGraph requirements
    typedef transform_iterator< detail::reverse_graph_edge_descriptor_maker<
                                    typename Traits::edge_descriptor >,
        typename Traits::in_edge_iterator >
        out_edge_iterator;
    typedef typename Traits::degree_size_type degree_size_type;

    // BidirectionalGraph requirements
    typedef transform_iterator< detail::reverse_graph_edge_descriptor_maker<
                                    typename Traits::edge_descriptor >,
        typename Traits::out_edge_iterator >
        in_edge_iterator;

    // AdjacencyGraph requirements
    typedef typename adjacency_iterator_generator< Self, vertex_descriptor,
        out_edge_iterator >::type adjacency_iterator;

    // VertexListGraph requirements
    typedef typename Traits::vertex_iterator vertex_iterator;

    // EdgeListGraph requirements
    enum
    {
        is_edge_list
        = is_convertible< traversal_category, edge_list_graph_tag >::value
    };
    typedef detail::choose_rev_edge_iter< is_edge_list > ChooseEdgeIter;
    typedef typename ChooseEdgeIter::template bind_< BidirectionalGraph >::type
        edge_iterator;
    typedef typename Traits::vertices_size_type vertices_size_type;
    typedef typename Traits::edges_size_type edges_size_type;

    typedef reverse_graph_tag graph_tag;

#ifndef BOOST_GRAPH_NO_BUNDLED_PROPERTIES
    // Bundled properties support
    template < typename Descriptor >
    typename graph::detail::bundled_result< BidirectionalGraph,
        typename detail::get_underlying_descriptor_from_reverse_descriptor<
            Descriptor >::type >::type&
    operator[](Descriptor x)
    {
        return m_g[detail::get_underlying_descriptor_from_reverse_descriptor<
            Descriptor >::convert(x)];
    }

    template < typename Descriptor >
    typename graph::detail::bundled_result< BidirectionalGraph,
        typename detail::get_underlying_descriptor_from_reverse_descriptor<
            Descriptor >::type >::type const&
    operator[](Descriptor x) const
    {
        return m_g[detail::get_underlying_descriptor_from_reverse_descriptor<
            Descriptor >::convert(x)];
    }
#endif // BOOST_GRAPH_NO_BUNDLED_PROPERTIES

    static vertex_descriptor null_vertex() { return Traits::null_vertex(); }

    // would be private, but template friends aren't portable enough.
    // private:
    GraphRef m_g;
};

// These are separate so they are not instantiated unless used (see bug 1021)
template < class BidirectionalGraph, class GraphRef >
struct vertex_property_type< reverse_graph< BidirectionalGraph, GraphRef > >
{
    typedef
        typename boost::vertex_property_type< BidirectionalGraph >::type type;
};

template < class BidirectionalGraph, class GraphRef >
struct edge_property_type< reverse_graph< BidirectionalGraph, GraphRef > >
{
    typedef typename boost::edge_property_type< BidirectionalGraph >::type type;
};

template < class BidirectionalGraph, class GraphRef >
struct graph_property_type< reverse_graph< BidirectionalGraph, GraphRef > >
{
    typedef
        typename boost::graph_property_type< BidirectionalGraph >::type type;
};

#ifndef BOOST_GRAPH_NO_BUNDLED_PROPERTIES
template < typename Graph, typename GraphRef >
struct vertex_bundle_type< reverse_graph< Graph, GraphRef > >
: vertex_bundle_type< Graph >
{
};

template < typename Graph, typename GraphRef >
struct edge_bundle_type< reverse_graph< Graph, GraphRef > >
: edge_bundle_type< Graph >
{
};

template < typename Graph, typename GraphRef >
struct graph_bundle_type< reverse_graph< Graph, GraphRef > >
: graph_bundle_type< Graph >
{
};
#endif // BOOST_GRAPH_NO_BUNDLED_PROPERTIES

template < class BidirectionalGraph >
inline reverse_graph< BidirectionalGraph > make_reverse_graph(
    const BidirectionalGraph& g)
{
    return reverse_graph< BidirectionalGraph >(g);
}

template < class BidirectionalGraph >
inline reverse_graph< BidirectionalGraph, BidirectionalGraph& >
make_reverse_graph(BidirectionalGraph& g)
{
    return reverse_graph< BidirectionalGraph, BidirectionalGraph& >(g);
}

template < class BidirectionalGraph, class GRef >
std::pair< typename reverse_graph< BidirectionalGraph >::vertex_iterator,
    typename reverse_graph< BidirectionalGraph >::vertex_iterator >
vertices(const reverse_graph< BidirectionalGraph, GRef >& g)
{
    return vertices(g.m_g);
}

template < class BidirectionalGraph, class GRef >
std::pair< typename reverse_graph< BidirectionalGraph >::edge_iterator,
    typename reverse_graph< BidirectionalGraph >::edge_iterator >
edges(const reverse_graph< BidirectionalGraph, GRef >& g)
{
    return detail::reverse_edge_iter_pair<
        typename graph_traits< BidirectionalGraph >::edge_descriptor >(
        edges(g.m_g));
}

template < class BidirectionalGraph, class GRef >
inline std::pair<
    typename reverse_graph< BidirectionalGraph >::out_edge_iterator,
    typename reverse_graph< BidirectionalGraph >::out_edge_iterator >
out_edges(
    const typename graph_traits< BidirectionalGraph >::vertex_descriptor u,
    const reverse_graph< BidirectionalGraph, GRef >& g)
{
    return detail::reverse_edge_iter_pair<
        typename graph_traits< BidirectionalGraph >::edge_descriptor >(
        in_edges(u, g.m_g));
}

template < class BidirectionalGraph, class GRef >
inline typename graph_traits< BidirectionalGraph >::vertices_size_type
num_vertices(const reverse_graph< BidirectionalGraph, GRef >& g)
{
    return num_vertices(g.m_g);
}

template < class BidirectionalGraph, class GRef >
inline typename reverse_graph< BidirectionalGraph >::edges_size_type num_edges(
    const reverse_graph< BidirectionalGraph, GRef >& g)
{
    return num_edges(g.m_g);
}

template < class BidirectionalGraph, class GRef >
inline typename graph_traits< BidirectionalGraph >::degree_size_type out_degree(
    const typename graph_traits< BidirectionalGraph >::vertex_descriptor u,
    const reverse_graph< BidirectionalGraph, GRef >& g)
{
    return in_degree(u, g.m_g);
}

template < class BidirectionalGraph, class GRef >
inline typename graph_traits< BidirectionalGraph >::vertex_descriptor vertex(
    const typename graph_traits< BidirectionalGraph >::vertices_size_type v,
    const reverse_graph< BidirectionalGraph, GRef >& g)
{
    return vertex(v, g.m_g);
}

template < class BidirectionalGraph, class GRef >
inline std::pair< typename graph_traits< reverse_graph< BidirectionalGraph,
                      GRef > >::edge_descriptor,
    bool >
edge(const typename graph_traits< BidirectionalGraph >::vertex_descriptor u,
    const typename graph_traits< BidirectionalGraph >::vertex_descriptor v,
    const reverse_graph< BidirectionalGraph, GRef >& g)
{
    typedef typename graph_traits< BidirectionalGraph >::edge_descriptor
        underlying_edge_descriptor;
    std::pair< underlying_edge_descriptor, bool > e = edge(v, u, g.m_g);
    return std::make_pair(
        detail::reverse_graph_edge_descriptor< underlying_edge_descriptor >(
            e.first),
        e.second);
}

template < class BidirectionalGraph, class GRef >
inline std::pair<
    typename reverse_graph< BidirectionalGraph >::in_edge_iterator,
    typename reverse_graph< BidirectionalGraph >::in_edge_iterator >
in_edges(const typename graph_traits< BidirectionalGraph >::vertex_descriptor u,
    const reverse_graph< BidirectionalGraph, GRef >& g)
{
    return detail::reverse_edge_iter_pair<
        typename graph_traits< BidirectionalGraph >::edge_descriptor >(
        out_edges(u, g.m_g));
}

template < class BidirectionalGraph, class GRef >
inline std::pair<
    typename reverse_graph< BidirectionalGraph, GRef >::adjacency_iterator,
    typename reverse_graph< BidirectionalGraph, GRef >::adjacency_iterator >
adjacent_vertices(
    typename graph_traits< BidirectionalGraph >::vertex_descriptor u,
    const reverse_graph< BidirectionalGraph, GRef >& g)
{
    typedef reverse_graph< BidirectionalGraph, GRef > Graph;
    typename graph_traits< Graph >::out_edge_iterator first, last;
    boost::tie(first, last) = out_edges(u, g);
    typedef
        typename graph_traits< Graph >::adjacency_iterator adjacency_iterator;
    return std::make_pair(adjacency_iterator(first, const_cast< Graph* >(&g)),
        adjacency_iterator(last, const_cast< Graph* >(&g)));
}

template < class BidirectionalGraph, class GRef >
inline typename graph_traits< BidirectionalGraph >::degree_size_type in_degree(
    const typename graph_traits< BidirectionalGraph >::vertex_descriptor u,
    const reverse_graph< BidirectionalGraph, GRef >& g)
{
    return out_degree(u, g.m_g);
}

template < class Edge, class BidirectionalGraph, class GRef >
inline typename graph_traits< BidirectionalGraph >::vertex_descriptor source(
    const detail::reverse_graph_edge_descriptor< Edge >& e,
    const reverse_graph< BidirectionalGraph, GRef >& g)
{
    return target(e.underlying_descx, g.m_g);
}

template < class Edge, class BidirectionalGraph, class GRef >
inline typename graph_traits< BidirectionalGraph >::vertex_descriptor target(
    const detail::reverse_graph_edge_descriptor< Edge >& e,
    const reverse_graph< BidirectionalGraph, GRef >& g)
{
    return source(e.underlying_descx, g.m_g);
}

template < class BidirectionalGraph, class GRef >
inline typename graph_traits< BidirectionalGraph >::degree_size_type degree(
    const typename graph_traits< BidirectionalGraph >::vertex_descriptor u,
    const reverse_graph< BidirectionalGraph, GRef >& g)
{
    return degree(u, g.m_g);
}

namespace detail
{

    template < typename PM > struct reverse_graph_edge_property_map
    {
    private:
        PM underlying_pm;

    public:
        typedef reverse_graph_edge_descriptor<
            typename property_traits< PM >::key_type >
            key_type;
        typedef typename property_traits< PM >::value_type value_type;
        typedef typename property_traits< PM >::reference reference;
        typedef typename property_traits< PM >::category category;

        explicit reverse_graph_edge_property_map(const PM& pm)
        : underlying_pm(pm)
        {
        }

        friend reference get(
            const reverse_graph_edge_property_map& m, const key_type& e)
        {
            return get(m.underlying_pm, e.underlying_descx);
        }

        friend void put(const reverse_graph_edge_property_map& m,
            const key_type& e, const value_type& v)
        {
            put(m.underlying_pm, e.underlying_descx, v);
        }

        reference operator[](const key_type& k) const
        {
            return (this->underlying_pm)[k.underlying_descx];
        }
    };

} // namespace detail

template < class BidirGraph, class GRef, class Property >
struct property_map< reverse_graph< BidirGraph, GRef >, Property >
{
    typedef boost::is_same<
        typename detail::property_kind_from_graph< BidirGraph, Property >::type,
        edge_property_tag >
        is_edge_prop;
    typedef boost::is_const< typename boost::remove_reference< GRef >::type >
        is_ref_const;
    typedef typename boost::mpl::if_< is_ref_const,
        typename property_map< BidirGraph, Property >::const_type,
        typename property_map< BidirGraph, Property >::type >::type orig_type;
    typedef typename property_map< BidirGraph, Property >::const_type
        orig_const_type;
    typedef typename boost::mpl::if_< is_edge_prop,
        detail::reverse_graph_edge_property_map< orig_type >, orig_type >::type
        type;
    typedef typename boost::mpl::if_< is_edge_prop,
        detail::reverse_graph_edge_property_map< orig_const_type >,
        orig_const_type >::type const_type;
};

template < class BidirGraph, class GRef, class Property >
struct property_map< const reverse_graph< BidirGraph, GRef >, Property >
{
    typedef boost::is_same<
        typename detail::property_kind_from_graph< BidirGraph, Property >::type,
        edge_property_tag >
        is_edge_prop;
    typedef typename property_map< BidirGraph, Property >::const_type
        orig_const_type;
    typedef typename boost::mpl::if_< is_edge_prop,
        detail::reverse_graph_edge_property_map< orig_const_type >,
        orig_const_type >::type const_type;
    typedef const_type type;
};

template < class BidirGraph, class GRef, class Property >
typename disable_if< is_same< Property, edge_underlying_t >,
    typename property_map< reverse_graph< BidirGraph, GRef >,
        Property >::type >::type
get(Property p, reverse_graph< BidirGraph, GRef >& g)
{
    return typename property_map< reverse_graph< BidirGraph, GRef >,
        Property >::type(get(p, g.m_g));
}

template < class BidirGraph, class GRef, class Property >
typename disable_if< is_same< Property, edge_underlying_t >,
    typename property_map< reverse_graph< BidirGraph, GRef >,
        Property >::const_type >::type
get(Property p, const reverse_graph< BidirGraph, GRef >& g)
{
    const BidirGraph& gref = g.m_g; // in case GRef is non-const
    return typename property_map< reverse_graph< BidirGraph, GRef >,
        Property >::const_type(get(p, gref));
}

template < class BidirectionalGraph, class GRef, class Property, class Key >
typename disable_if< is_same< Property, edge_underlying_t >,
    typename property_traits<
        typename property_map< reverse_graph< BidirectionalGraph, GRef >,
            Property >::const_type >::value_type >::type
get(Property p, const reverse_graph< BidirectionalGraph, GRef >& g,
    const Key& k)
{
    return get(get(p, g), k);
}

template < class BidirectionalGraph, class GRef, class Property, class Key,
    class Value >
void put(Property p, reverse_graph< BidirectionalGraph, GRef >& g, const Key& k,
    const Value& val)
{
    put(get(p, g), k, val);
}

// Get the underlying descriptor from a reverse_graph's wrapped edge descriptor

namespace detail
{
    template < class E > struct underlying_edge_desc_map_type
    {
        E operator[](const reverse_graph_edge_descriptor< E >& k) const
        {
            return k.underlying_descx;
        }
    };

    template < class E >
    E get(underlying_edge_desc_map_type< E > m,
        const reverse_graph_edge_descriptor< E >& k)
    {
        return m[k];
    }
}

template < class E >
struct property_traits< detail::underlying_edge_desc_map_type< E > >
{
    typedef detail::reverse_graph_edge_descriptor< E > key_type;
    typedef E value_type;
    typedef const E& reference;
    typedef readable_property_map_tag category;
};

template < class Graph, class GRef >
struct property_map< reverse_graph< Graph, GRef >, edge_underlying_t >
{
private:
    typedef typename graph_traits< Graph >::edge_descriptor ed;

public:
    typedef detail::underlying_edge_desc_map_type< ed > type;
    typedef detail::underlying_edge_desc_map_type< ed > const_type;
};

template < typename T > struct is_reverse_graph : boost::mpl::false_
{
};
template < typename G, typename R >
struct is_reverse_graph< reverse_graph< G, R > > : boost::mpl::true_
{
};

template < class G >
typename enable_if< is_reverse_graph< G >,
    detail::underlying_edge_desc_map_type< typename graph_traits<
        typename G::base_type >::edge_descriptor > >::type
get(edge_underlying_t, G&)
{
    return detail::underlying_edge_desc_map_type<
        typename graph_traits< typename G::base_type >::edge_descriptor >();
}

template < class G >
typename enable_if< is_reverse_graph< G >,
    typename graph_traits< typename G::base_type >::edge_descriptor >::type
get(edge_underlying_t, G&, const typename graph_traits< G >::edge_descriptor& k)
{
    return k.underlying_descx;
}

template < class G >
typename enable_if< is_reverse_graph< G >,
    detail::underlying_edge_desc_map_type< typename graph_traits<
        typename G::base_type >::edge_descriptor > >::type
get(edge_underlying_t, const G&)
{
    return detail::underlying_edge_desc_map_type<
        typename graph_traits< typename G::base_type >::edge_descriptor >();
}

template < class G >
typename enable_if< is_reverse_graph< G >,
    typename graph_traits< typename G::base_type >::edge_descriptor >::type
get(edge_underlying_t, const G&,
    const typename graph_traits< G >::edge_descriptor& k)
{
    return k.underlying_descx;
}

// Access to wrapped graph's graph properties

template < typename BidirectionalGraph, typename GRef, typename Tag,
    typename Value >
inline void set_property(const reverse_graph< BidirectionalGraph, GRef >& g,
    Tag tag, const Value& value)
{
    set_property(g.m_g, tag, value);
}

template < typename BidirectionalGraph, typename GRef, typename Tag >
inline typename boost::mpl::if_<
    boost::is_const< typename boost::remove_reference< GRef >::type >,
    const typename graph_property< BidirectionalGraph, Tag >::type&,
    typename graph_property< BidirectionalGraph, Tag >::type& >::type
get_property(const reverse_graph< BidirectionalGraph, GRef >& g, Tag tag)
{
    return get_property(g.m_g, tag);
}

} // namespace boost

#endif
