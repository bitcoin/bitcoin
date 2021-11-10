//
//=======================================================================
// Copyright 1997, 1998, 1999, 2000 University of Notre Dame.
// Authors: Andrew Lumsdaine, Lie-Quan Lee, Jeremy G. Siek
//
// Copyright 2009, Andrew Sutton
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================
//
#ifndef BOOST_GRAPH_CONCEPTS_HPP
#define BOOST_GRAPH_CONCEPTS_HPP

#include <boost/config.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/properties.hpp>
#include <boost/graph/numeric_values.hpp>
#include <boost/graph/buffer_concepts.hpp>
#include <boost/concept_check.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/mpl/not.hpp>
#include <boost/static_assert.hpp>
#include <boost/detail/workaround.hpp>
#include <boost/concept/assert.hpp>

#include <boost/concept/detail/concept_def.hpp>
namespace boost
{
// dwa 2003/7/11 -- This clearly shouldn't be necessary, but if
// you want to use vector_as_graph, it is!  I'm sure the graph
// library leaves these out all over the place.  Probably a
// redesign involving specializing a template with a static
// member function is in order :(
//
// It is needed in order to allow us to write using boost::vertices as
// needed for ADL when using vector_as_graph below.
#if !defined(BOOST_NO_ARGUMENT_DEPENDENT_LOOKUP) \
    && !BOOST_WORKAROUND(BOOST_BORLANDC, BOOST_TESTED_AT(0x564))
#define BOOST_VECTOR_AS_GRAPH_GRAPH_ADL_HACK
#endif

#ifdef BOOST_VECTOR_AS_GRAPH_GRAPH_ADL_HACK
template < class T >
typename T::ThereReallyIsNoMemberByThisNameInT vertices(T const&);
#endif

namespace concepts
{
    BOOST_concept(MultiPassInputIterator, (T)) { BOOST_CONCEPT_USAGE(
        MultiPassInputIterator) { BOOST_CONCEPT_ASSERT((InputIterator< T >));
}
};

BOOST_concept(Graph, (G))
{
    typedef typename graph_traits< G >::vertex_descriptor vertex_descriptor;
    typedef typename graph_traits< G >::edge_descriptor edge_descriptor;
    typedef typename graph_traits< G >::directed_category directed_category;
    typedef typename graph_traits< G >::edge_parallel_category
        edge_parallel_category;
    typedef typename graph_traits< G >::traversal_category traversal_category;

    BOOST_CONCEPT_USAGE(Graph)
    {
        BOOST_CONCEPT_ASSERT((DefaultConstructible< vertex_descriptor >));
        BOOST_CONCEPT_ASSERT((EqualityComparable< vertex_descriptor >));
        BOOST_CONCEPT_ASSERT((Assignable< vertex_descriptor >));
    }
    G g;
};

BOOST_concept(IncidenceGraph, (G)) : Graph< G >
{
    typedef typename graph_traits< G >::edge_descriptor edge_descriptor;
    typedef typename graph_traits< G >::out_edge_iterator out_edge_iterator;
    typedef typename graph_traits< G >::degree_size_type degree_size_type;
    typedef typename graph_traits< G >::traversal_category traversal_category;

    BOOST_STATIC_ASSERT(
        (boost::mpl::not_< boost::is_same< out_edge_iterator, void > >::value));
    BOOST_STATIC_ASSERT(
        (boost::mpl::not_< boost::is_same< degree_size_type, void > >::value));

    BOOST_CONCEPT_USAGE(IncidenceGraph)
    {
        BOOST_CONCEPT_ASSERT((MultiPassInputIterator< out_edge_iterator >));
        BOOST_CONCEPT_ASSERT((DefaultConstructible< edge_descriptor >));
        BOOST_CONCEPT_ASSERT((EqualityComparable< edge_descriptor >));
        BOOST_CONCEPT_ASSERT((Assignable< edge_descriptor >));
        BOOST_CONCEPT_ASSERT(
            (Convertible< traversal_category, incidence_graph_tag >));

        p = out_edges(u, g);
        n = out_degree(u, g);
        e = *p.first;
        u = source(e, g);
        v = target(e, g);
        const_constraints(g);
    }
    void const_constraints(const G& cg)
    {
        p = out_edges(u, cg);
        n = out_degree(u, cg);
        e = *p.first;
        u = source(e, cg);
        v = target(e, cg);
    }
    std::pair< out_edge_iterator, out_edge_iterator > p;
    typename graph_traits< G >::vertex_descriptor u, v;
    typename graph_traits< G >::edge_descriptor e;
    typename graph_traits< G >::degree_size_type n;
    G g;
};

BOOST_concept(BidirectionalGraph, (G)) : IncidenceGraph< G >
{
    typedef typename graph_traits< G >::in_edge_iterator in_edge_iterator;
    typedef typename graph_traits< G >::traversal_category traversal_category;

    BOOST_CONCEPT_USAGE(BidirectionalGraph)
    {
        BOOST_CONCEPT_ASSERT((MultiPassInputIterator< in_edge_iterator >));
        BOOST_CONCEPT_ASSERT(
            (Convertible< traversal_category, bidirectional_graph_tag >));

        BOOST_STATIC_ASSERT((boost::mpl::not_<
            boost::is_same< in_edge_iterator, void > >::value));

        p = in_edges(v, g);
        n = in_degree(v, g);
        n = degree(v, g);
        e = *p.first;
        const_constraints(g);
    }
    void const_constraints(const G& cg)
    {
        p = in_edges(v, cg);
        n = in_degree(v, cg);
        n = degree(v, cg);
        e = *p.first;
    }
    std::pair< in_edge_iterator, in_edge_iterator > p;
    typename graph_traits< G >::vertex_descriptor v;
    typename graph_traits< G >::edge_descriptor e;
    typename graph_traits< G >::degree_size_type n;
    G g;
};

BOOST_concept(AdjacencyGraph, (G)) : Graph< G >
{
    typedef typename graph_traits< G >::adjacency_iterator adjacency_iterator;
    typedef typename graph_traits< G >::traversal_category traversal_category;

    BOOST_CONCEPT_USAGE(AdjacencyGraph)
    {
        BOOST_CONCEPT_ASSERT((MultiPassInputIterator< adjacency_iterator >));
        BOOST_CONCEPT_ASSERT(
            (Convertible< traversal_category, adjacency_graph_tag >));

        BOOST_STATIC_ASSERT((boost::mpl::not_<
            boost::is_same< adjacency_iterator, void > >::value));

        p = adjacent_vertices(v, g);
        v = *p.first;
        const_constraints(g);
    }
    void const_constraints(const G& cg) { p = adjacent_vertices(v, cg); }
    std::pair< adjacency_iterator, adjacency_iterator > p;
    typename graph_traits< G >::vertex_descriptor v;
    G g;
};

BOOST_concept(VertexListGraph, (G)) : Graph< G >
{
    typedef typename graph_traits< G >::vertex_iterator vertex_iterator;
    typedef typename graph_traits< G >::vertices_size_type vertices_size_type;
    typedef typename graph_traits< G >::traversal_category traversal_category;

    BOOST_CONCEPT_USAGE(VertexListGraph)
    {
        BOOST_CONCEPT_ASSERT((MultiPassInputIterator< vertex_iterator >));
        BOOST_CONCEPT_ASSERT(
            (Convertible< traversal_category, vertex_list_graph_tag >));

        BOOST_STATIC_ASSERT((boost::mpl::not_<
            boost::is_same< vertex_iterator, void > >::value));
        BOOST_STATIC_ASSERT((boost::mpl::not_<
            boost::is_same< vertices_size_type, void > >::value));

#ifdef BOOST_VECTOR_AS_GRAPH_GRAPH_ADL_HACK
        // dwa 2003/7/11 -- This clearly shouldn't be necessary, but if
        // you want to use vector_as_graph, it is!  I'm sure the graph
        // library leaves these out all over the place.  Probably a
        // redesign involving specializing a template with a static
        // member function is in order :(
        using boost::vertices;
#endif
        p = vertices(g);
        v = *p.first;
        const_constraints(g);
    }
    void const_constraints(const G& cg)
    {
#ifdef BOOST_VECTOR_AS_GRAPH_GRAPH_ADL_HACK
        // dwa 2003/7/11 -- This clearly shouldn't be necessary, but if
        // you want to use vector_as_graph, it is!  I'm sure the graph
        // library leaves these out all over the place.  Probably a
        // redesign involving specializing a template with a static
        // member function is in order :(
        using boost::vertices;
#endif

        p = vertices(cg);
        v = *p.first;
        V = num_vertices(cg);
    }
    std::pair< vertex_iterator, vertex_iterator > p;
    typename graph_traits< G >::vertex_descriptor v;
    G g;
    vertices_size_type V;
};

BOOST_concept(EdgeListGraph, (G)) : Graph< G >
{
    typedef typename graph_traits< G >::edge_descriptor edge_descriptor;
    typedef typename graph_traits< G >::edge_iterator edge_iterator;
    typedef typename graph_traits< G >::edges_size_type edges_size_type;
    typedef typename graph_traits< G >::traversal_category traversal_category;

    BOOST_CONCEPT_USAGE(EdgeListGraph)
    {
        BOOST_CONCEPT_ASSERT((MultiPassInputIterator< edge_iterator >));
        BOOST_CONCEPT_ASSERT((DefaultConstructible< edge_descriptor >));
        BOOST_CONCEPT_ASSERT((EqualityComparable< edge_descriptor >));
        BOOST_CONCEPT_ASSERT((Assignable< edge_descriptor >));
        BOOST_CONCEPT_ASSERT(
            (Convertible< traversal_category, edge_list_graph_tag >));

        BOOST_STATIC_ASSERT(
            (boost::mpl::not_< boost::is_same< edge_iterator, void > >::value));
        BOOST_STATIC_ASSERT((boost::mpl::not_<
            boost::is_same< edges_size_type, void > >::value));

        p = edges(g);
        e = *p.first;
        u = source(e, g);
        v = target(e, g);
        const_constraints(g);
    }
    void const_constraints(const G& cg)
    {
        p = edges(cg);
        E = num_edges(cg);
        e = *p.first;
        u = source(e, cg);
        v = target(e, cg);
    }
    std::pair< edge_iterator, edge_iterator > p;
    typename graph_traits< G >::vertex_descriptor u, v;
    typename graph_traits< G >::edge_descriptor e;
    edges_size_type E;
    G g;
};

BOOST_concept(VertexAndEdgeListGraph, (G))
: VertexListGraph< G >, EdgeListGraph< G > {};

// Where to put the requirement for this constructor?
//      G g(n_vertices);
// Not in mutable graph, then LEDA graph's can't be models of
// MutableGraph.
BOOST_concept(EdgeMutableGraph, (G))
{
    typedef typename graph_traits< G >::edge_descriptor edge_descriptor;

    BOOST_CONCEPT_USAGE(EdgeMutableGraph)
    {
        p = add_edge(u, v, g);
        remove_edge(u, v, g);
        remove_edge(e, g);
        clear_vertex(v, g);
    }
    G g;
    edge_descriptor e;
    std::pair< edge_descriptor, bool > p;
    typename graph_traits< G >::vertex_descriptor u, v;
};

BOOST_concept(VertexMutableGraph, (G))
{

    BOOST_CONCEPT_USAGE(VertexMutableGraph)
    {
        v = add_vertex(g);
        remove_vertex(v, g);
    }
    G g;
    typename graph_traits< G >::vertex_descriptor u, v;
};

BOOST_concept(MutableGraph, (G))
: EdgeMutableGraph< G >, VertexMutableGraph< G > {};

template < class edge_descriptor > struct dummy_edge_predicate
{
    bool operator()(const edge_descriptor&) const { return false; }
};

BOOST_concept(MutableIncidenceGraph, (G)) : MutableGraph< G >
{
    BOOST_CONCEPT_USAGE(MutableIncidenceGraph)
    {
        remove_edge(iter, g);
        remove_out_edge_if(u, p, g);
    }
    G g;
    typedef typename graph_traits< G >::edge_descriptor edge_descriptor;
    dummy_edge_predicate< edge_descriptor > p;
    typename boost::graph_traits< G >::vertex_descriptor u;
    typename boost::graph_traits< G >::out_edge_iterator iter;
};

BOOST_concept(MutableBidirectionalGraph, (G)) : MutableIncidenceGraph< G >
{
    BOOST_CONCEPT_USAGE(MutableBidirectionalGraph)
    {
        remove_in_edge_if(u, p, g);
    }
    G g;
    typedef typename graph_traits< G >::edge_descriptor edge_descriptor;
    dummy_edge_predicate< edge_descriptor > p;
    typename boost::graph_traits< G >::vertex_descriptor u;
};

BOOST_concept(MutableEdgeListGraph, (G)) : EdgeMutableGraph< G >
{
    BOOST_CONCEPT_USAGE(MutableEdgeListGraph) { remove_edge_if(p, g); }
    G g;
    typedef typename graph_traits< G >::edge_descriptor edge_descriptor;
    dummy_edge_predicate< edge_descriptor > p;
};

BOOST_concept(VertexMutablePropertyGraph, (G)) : VertexMutableGraph< G >
{
    BOOST_CONCEPT_USAGE(VertexMutablePropertyGraph) { v = add_vertex(vp, g); }
    G g;
    typename graph_traits< G >::vertex_descriptor v;
    typename vertex_property_type< G >::type vp;
};

BOOST_concept(EdgeMutablePropertyGraph, (G)) : EdgeMutableGraph< G >
{
    typedef typename graph_traits< G >::edge_descriptor edge_descriptor;

    BOOST_CONCEPT_USAGE(EdgeMutablePropertyGraph) { p = add_edge(u, v, ep, g); }
    G g;
    std::pair< edge_descriptor, bool > p;
    typename graph_traits< G >::vertex_descriptor u, v;
    typename edge_property_type< G >::type ep;
};

BOOST_concept(AdjacencyMatrix, (G)) : Graph< G >
{
    typedef typename graph_traits< G >::edge_descriptor edge_descriptor;

    BOOST_CONCEPT_USAGE(AdjacencyMatrix)
    {
        p = edge(u, v, g);
        const_constraints(g);
    }
    void const_constraints(const G& cg) { p = edge(u, v, cg); }
    typename graph_traits< G >::vertex_descriptor u, v;
    std::pair< edge_descriptor, bool > p;
    G g;
};

BOOST_concept(ReadablePropertyGraph, (G)(X)(Property)) : Graph< G >
{
    typedef typename property_map< G, Property >::const_type const_Map;

    BOOST_CONCEPT_USAGE(ReadablePropertyGraph)
    {
        BOOST_CONCEPT_ASSERT((ReadablePropertyMapConcept< const_Map, X >));

        const_constraints(g);
    }
    void const_constraints(const G& cg)
    {
        const_Map pmap = get(Property(), cg);
        pval = get(Property(), cg, x);
        ignore_unused_variable_warning(pmap);
    }
    G g;
    X x;
    typename property_traits< const_Map >::value_type pval;
};

BOOST_concept(PropertyGraph, (G)(X)(Property))
: ReadablePropertyGraph< G, X, Property >
{
    typedef typename property_map< G, Property >::type Map;
    BOOST_CONCEPT_USAGE(PropertyGraph)
    {
        BOOST_CONCEPT_ASSERT((ReadWritePropertyMapConcept< Map, X >));

        Map pmap = get(Property(), g);
        pval = get(Property(), g, x);
        put(Property(), g, x, pval);
        ignore_unused_variable_warning(pmap);
    }
    G g;
    X x;
    typename property_traits< Map >::value_type pval;
};

BOOST_concept(LvaluePropertyGraph, (G)(X)(Property))
: ReadablePropertyGraph< G, X, Property >
{
    typedef typename property_map< G, Property >::type Map;
    typedef typename property_map< G, Property >::const_type const_Map;

    BOOST_CONCEPT_USAGE(LvaluePropertyGraph)
    {
        BOOST_CONCEPT_ASSERT((LvaluePropertyMapConcept< const_Map, X >));

        pval = get(Property(), g, x);
        put(Property(), g, x, pval);
    }
    G g;
    X x;
    typename property_traits< Map >::value_type pval;
};

// The *IndexGraph concepts are "semantic" graph concpepts. These can be
// applied to describe any graph that has an index map that can be accessed
// using the get(*_index, g) method. For example, adjacency lists with
// VertexSet == vecS are implicitly models of this concept.
//
// NOTE: We could require an associated type vertex_index_type, but that
// would mean propagating that type name into graph_traits and all of the
// other graph implementations. Much easier to simply call it unsigned.

BOOST_concept(VertexIndexGraph, (Graph))
{
    BOOST_CONCEPT_USAGE(VertexIndexGraph)
    {
        typedef typename graph_traits< Graph >::vertex_descriptor Vertex;
        typedef typename property_map< Graph, vertex_index_t >::type Map;
        typedef unsigned Index; // This could be Graph::vertex_index_type
        Map m = get(vertex_index, g);
        Index x = get(vertex_index, g, Vertex());
        ignore_unused_variable_warning(m);
        ignore_unused_variable_warning(x);

        // This is relaxed
        renumber_vertex_indices(g);

        const_constraints(g);
    }
    void const_constraints(const Graph& g_)
    {
        typedef typename property_map< Graph, vertex_index_t >::const_type Map;
        Map m = get(vertex_index, g_);
        ignore_unused_variable_warning(m);
    }

private:
    Graph g;
};

BOOST_concept(EdgeIndexGraph, (Graph))
{
    BOOST_CONCEPT_USAGE(EdgeIndexGraph)
    {
        typedef typename graph_traits< Graph >::edge_descriptor Edge;
        typedef typename property_map< Graph, edge_index_t >::type Map;
        typedef unsigned Index; // This could be Graph::vertex_index_type
        Map m = get(edge_index, g);
        Index x = get(edge_index, g, Edge());
        ignore_unused_variable_warning(m);
        ignore_unused_variable_warning(x);

        // This is relaxed
        renumber_edge_indices(g);

        const_constraints(g);
    }
    void const_constraints(const Graph& g_)
    {
        typedef typename property_map< Graph, edge_index_t >::const_type Map;
        Map m = get(edge_index, g_);
        ignore_unused_variable_warning(m);
    }

private:
    Graph g;
};

BOOST_concept(ColorValue, (C))
: EqualityComparable< C >, DefaultConstructible< C >
{
    BOOST_CONCEPT_USAGE(ColorValue)
    {
        c = color_traits< C >::white();
        c = color_traits< C >::gray();
        c = color_traits< C >::black();
    }
    C c;
};

BOOST_concept(BasicMatrix, (M)(I)(V))
{
    BOOST_CONCEPT_USAGE(BasicMatrix)
    {
        V& elt = A[i][j];
        const_constraints(A);
        ignore_unused_variable_warning(elt);
    }
    void const_constraints(const M& cA)
    {
        const V& elt = cA[i][j];
        ignore_unused_variable_warning(elt);
    }
    M A;
    I i, j;
};

// The following concepts describe aspects of numberic values and measure
// functions. We're extending the notion of numeric values to include
// emulation for zero and infinity.

BOOST_concept(NumericValue, (Numeric)) { BOOST_CONCEPT_USAGE(NumericValue) {
    BOOST_CONCEPT_ASSERT((DefaultConstructible< Numeric >));
BOOST_CONCEPT_ASSERT((CopyConstructible< Numeric >));
numeric_values< Numeric >::zero();
numeric_values< Numeric >::infinity();
}
}
;

BOOST_concept(DegreeMeasure, (Measure)(Graph))
{
    BOOST_CONCEPT_USAGE(DegreeMeasure)
    {
        typedef typename Measure::degree_type Degree;
        typedef typename Measure::vertex_type Vertex;

        Degree d = m(Vertex(), g);
        ignore_unused_variable_warning(d);
    }

private:
    Measure m;
    Graph g;
};

BOOST_concept(DistanceMeasure, (Measure)(Graph))
{
    BOOST_CONCEPT_USAGE(DistanceMeasure)
    {
        typedef typename Measure::distance_type Distance;
        typedef typename Measure::result_type Result;
        Result r = m(Distance(), g);
        ignore_unused_variable_warning(r);
    }

private:
    Measure m;
    Graph g;
};

} /* namespace concepts */

using boost::concepts::MultiPassInputIteratorConcept;

// Graph concepts
using boost::concepts::AdjacencyGraphConcept;
using boost::concepts::AdjacencyMatrixConcept;
using boost::concepts::BidirectionalGraphConcept;
using boost::concepts::EdgeIndexGraphConcept;
using boost::concepts::EdgeListGraphConcept;
using boost::concepts::EdgeMutableGraphConcept;
using boost::concepts::EdgeMutablePropertyGraphConcept;
using boost::concepts::GraphConcept;
using boost::concepts::IncidenceGraphConcept;
using boost::concepts::LvaluePropertyGraphConcept;
using boost::concepts::MutableBidirectionalGraphConcept;
using boost::concepts::MutableEdgeListGraphConcept;
using boost::concepts::MutableGraphConcept;
using boost::concepts::MutableIncidenceGraphConcept;
using boost::concepts::PropertyGraphConcept;
using boost::concepts::ReadablePropertyGraphConcept;
using boost::concepts::VertexAndEdgeListGraphConcept;
using boost::concepts::VertexIndexGraphConcept;
using boost::concepts::VertexListGraphConcept;
using boost::concepts::VertexMutableGraphConcept;
using boost::concepts::VertexMutablePropertyGraphConcept;

// Utility concepts
using boost::concepts::BasicMatrixConcept;
using boost::concepts::ColorValueConcept;
using boost::concepts::DegreeMeasureConcept;
using boost::concepts::DistanceMeasureConcept;
using boost::concepts::NumericValueConcept;

} /* namespace boost */
#include <boost/concept/detail/concept_undef.hpp>

#endif /* BOOST_GRAPH_CONCEPTS_H */
