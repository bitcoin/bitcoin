//=======================================================================
// Copyright 2001 Universite Joseph Fourier, Grenoble.
// Author: Francois Faure
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================
#ifndef BOOST_GRAPH_ADJACENCY_LIST_IO_HPP
#define BOOST_GRAPH_ADJACENCY_LIST_IO_HPP

#include <iostream>
#include <vector>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/iteration_macros.hpp>
#include <cctype>

// Method read to parse an adjacency list from an input stream. Examples:
// cin >> read( G );
// cin >> read( G, NodePropertySubset(), EdgepropertySubset() );
//
// Method write to print an adjacency list to an output stream. Examples:
// cout << write( G );
// cout << write( G, NodePropertySubset(), EdgepropertySubset() );

namespace boost
{

/* outline
        - basic property input
        - get property subset
        - graph parser
        - property printer
        - graph printer
        - user methods
*/

//===========================================================================
// basic property input

template < class Tag, class Value, class Next >
std::istream& operator>>(std::istream& in, property< Tag, Value, Next >& p)
{
    in >> p.m_value >> p.m_base; // houpla !!
    return in;
}

template < class Tag, class Value >
std::istream& operator>>(
    std::istream& in, property< Tag, Value, no_property >& p)
{
    in >> p.m_value;
    return in;
}

inline std::istream& operator>>(std::istream& in, no_property&) { return in; }

// basic property input
//===========================================================================
// get property subsets

// get a single property tagged Stag
template < class Tag, class Value, class Next, class V, class Stag >
void get(property< Tag, Value, Next >& p, const V& v, Stag s)
{
    get(p.m_base, v, s);
}

template < class Value, class Next, class V, class Stag >
void get(property< Stag, Value, Next >& p, const V& v, Stag)
{
    p.m_value = v;
}

// get a subset of properties tagged Stag
template < class Tag, class Value, class Next, class Stag, class Svalue,
    class Snext >
void getSubset(
    property< Tag, Value, Next >& p, const property< Stag, Svalue, Snext >& s)
{
    get(p, s.m_value, Stag());
    getSubset(p, s.m_base);
}

template < class Tag, class Value, class Next, class Stag, class Svalue >
void getSubset(property< Tag, Value, Next >& p,
    const property< Stag, Svalue, no_property >& s)
{
    get(p, s.m_value, Stag());
}

inline void getSubset(no_property&, const no_property&) {}

#if !defined(BOOST_GRAPH_NO_BUNDLED_PROPERTIES)
template < typename T, typename U > void getSubset(T& p, const U& s) { p = s; }

template < typename T > void getSubset(T&, const no_property&) {}

#endif

//  get property subset
//===========================================================================
// graph parser
typedef enum
{
    PARSE_NUM_NODES,
    PARSE_VERTEX,
    PARSE_EDGE
} GraphParserState;

template < class Graph_t, class VertexProperty, class EdgeProperty,
    class VertexPropertySubset, class EdgePropertySubset >
struct GraphParser
{

    typedef Graph_t Graph;

    GraphParser(Graph* g) : graph(g) {}

    GraphParser& operator()(std::istream& in)
    {
        typedef typename graph_traits< Graph >::vertex_descriptor Vertex;
        std::vector< Vertex > nodes;

        GraphParserState state = PARSE_VERTEX;

        unsigned int numLine = 1;
        char c;
        while (in.get(c))
        {
            if (c == '#')
                skip(in);
            else if (c == 'n')
                state = PARSE_NUM_NODES;
            else if (c == 'v')
                state = PARSE_VERTEX;
            else if (c == 'e')
                state = PARSE_EDGE;
            else if (c == '\n')
                numLine++;
            else if (!std::isspace(c))
            {
                in.putback(c);
                if (state == PARSE_VERTEX)
                {
                    VertexPropertySubset readProp;
                    if (in >> readProp)
                    {
                        VertexProperty vp;
                        getSubset(vp, readProp);
                        nodes.push_back(add_vertex(vp, *graph));
                    }
                    else
                        std::cerr << "read vertex, parse error at line"
                                  << numLine << std::endl;
                }
                else if (state == PARSE_EDGE)
                {
                    int source, target;
                    EdgePropertySubset readProp;
                    in >> source >> target;
                    if (in >> readProp)
                    {
                        EdgeProperty ep;
                        getSubset(ep, readProp);
                        add_edge(nodes[source], nodes[target], ep, *graph);
                    }
                    else
                        std::cerr << "read edge, parse error at line" << numLine
                                  << std::endl;
                }
                else
                { // state == PARSE_NUM_NODES
                    int n;
                    if (in >> n)
                    {
                        for (int i = 0; i < n; ++i)
                            nodes.push_back(add_vertex(*graph));
                    }
                    else
                        std::cerr << "read num_nodes, parse error at line "
                                  << numLine << std::endl;
                }
            }
        }
        return (*this);
    }

protected:
    Graph* graph;

    void skip(std::istream& in)
    {
        char c = 0;
        while (c != '\n' && !in.eof())
            in.get(c);
        in.putback(c);
    }
};

// parser
//=======================================================================
// property printer

#if defined(BOOST_GRAPH_NO_BUNDLED_PROPERTIES)
template < class Graph, class Property > struct PropertyPrinter
{
    typedef typename Property::value_type Value;
    typedef typename Property::tag_type Tag;
    typedef typename Property::next_type Next;

    PropertyPrinter(const Graph& g) : graph(&g) {}

    template < class Val >
    PropertyPrinter& operator()(std::ostream& out, const Val& v)
    {
        typename property_map< Graph, Tag >::const_type ps = get(Tag(), *graph);
        out << ps[v] << " ";
        PropertyPrinter< Graph, Next > print(*graph);
        print(out, v);
        return (*this);
    }

private:
    const Graph* graph;
};
#else
template < class Graph, typename Property > struct PropertyPrinter
{
    PropertyPrinter(const Graph& g) : graph(&g) {}

    template < class Val >
    PropertyPrinter& operator()(std::ostream& out, const Val& v)
    {
        out << (*graph)[v] << " ";
        return (*this);
    }

private:
    const Graph* graph;
};

template < class Graph, typename Tag, typename Value, typename Next >
struct PropertyPrinter< Graph, property< Tag, Value, Next > >
{
    PropertyPrinter(const Graph& g) : graph(&g) {}

    template < class Val >
    PropertyPrinter& operator()(std::ostream& out, const Val& v)
    {
        typename property_map< Graph, Tag >::const_type ps = get(Tag(), *graph);
        out << ps[v] << " ";
        PropertyPrinter< Graph, Next > print(*graph);
        print(out, v);
        return (*this);
    }

private:
    const Graph* graph;
};
#endif

template < class Graph > struct PropertyPrinter< Graph, no_property >
{
    PropertyPrinter(const Graph&) {}

    template < class Val >
    PropertyPrinter& operator()(std::ostream&, const Val&)
    {
        return *this;
    }
};

// property printer
//=========================================================================
// graph printer

template < class Graph_t, class EdgeProperty > struct EdgePrinter
{

    typedef Graph_t Graph;
    typedef typename graph_traits< Graph >::vertex_descriptor Vertex;

    EdgePrinter(const Graph& g) : graph(g) {}

    const EdgePrinter& operator()(std::ostream& out) const
    {
        // assign indices to vertices
        std::map< Vertex, int > indices;
        int num = 0;
        BGL_FORALL_VERTICES_T(v, graph, Graph) { indices[v] = num++; }

        // write edges
        PropertyPrinter< Graph, EdgeProperty > print_Edge(graph);
        out << "e" << std::endl;
        BGL_FORALL_EDGES_T(e, graph, Graph)
        {
            out << indices[source(e, graph)] << " " << indices[target(e, graph)]
                << "  ";
            print_Edge(out, e);
            out << std::endl;
        }
        out << std::endl;
        return (*this);
    }

protected:
    const Graph& graph;
};

template < class Graph, class V, class E >
struct GraphPrinter : public EdgePrinter< Graph, E >
{
    GraphPrinter(const Graph& g) : EdgePrinter< Graph, E >(g) {}

    const GraphPrinter& operator()(std::ostream& out) const
    {
        PropertyPrinter< Graph, V > printNode(this->graph);
        out << "v" << std::endl;
        BGL_FORALL_VERTICES_T(v, this->graph, Graph)
        {
            printNode(out, v);
            out << std::endl;
        }

        EdgePrinter< Graph, E >::operator()(out);
        return (*this);
    }
};

template < class Graph, class E >
struct GraphPrinter< Graph, no_property, E > : public EdgePrinter< Graph, E >
{
    GraphPrinter(const Graph& g) : EdgePrinter< Graph, E >(g) {}

    const GraphPrinter& operator()(std::ostream& out) const
    {
        out << "n " << num_vertices(this->graph) << std::endl;
        EdgePrinter< Graph, E >::operator()(out);
        return (*this);
    }
};

// graph printer
//=========================================================================
// user methods

/// input stream for reading a graph
template < class Graph, class VP, class EP, class VPS, class EPS >
std::istream& operator>>(
    std::istream& in, GraphParser< Graph, VP, EP, VPS, EPS > gp)
{
    gp(in);
    return in;
}

/// graph parser for given subsets of internal vertex and edge properties
template < class EL, class VL, class D, class VP, class EP, class GP, class VPS,
    class EPS >
GraphParser< adjacency_list< EL, VL, D, VP, EP, GP >, VP, EP, VPS, EPS > read(
    adjacency_list< EL, VL, D, VP, EP, GP >& g, VPS vps, EPS eps)
{
    return GraphParser< adjacency_list< EL, VL, D, VP, EP, GP >, VP, EP, VPS,
        EPS >(&g);
}

/// graph parser for all internal vertex and edge properties
template < class EL, class VL, class D, class VP, class EP, class GP >
GraphParser< adjacency_list< EL, VL, D, VP, EP, GP >, VP, EP, VP, EP > read(
    adjacency_list< EL, VL, D, VP, EP, GP >& g)
{
    return GraphParser< adjacency_list< EL, VL, D, VP, EP, GP >, VP, EP, VP,
        EP >(&g);
}

/// output stream for writing a graph
template < class Graph, class VP, class EP >
std::ostream& operator<<(
    std::ostream& out, const GraphPrinter< Graph, VP, EP >& gp)
{
    gp(out);
    return out;
}

/// write the graph with given property subsets
template < class EL, class VL, class D, class VP, class EP, class GP, class VPS,
    class EPS >
GraphPrinter< adjacency_list< EL, VL, D, VP, EP, GP >, VPS, EPS > write(
    const adjacency_list< EL, VL, D, VP, EP, GP >& g, VPS, EPS)
{
    return GraphPrinter< adjacency_list< EL, VL, D, VP, EP, GP >, VPS, EPS >(g);
}

/// write the graph with all internal vertex and edge properties
template < class EL, class VL, class D, class VP, class EP, class GP >
GraphPrinter< adjacency_list< EL, VL, D, VP, EP, GP >, VP, EP > write(
    const adjacency_list< EL, VL, D, VP, EP, GP >& g)
{
    return GraphPrinter< adjacency_list< EL, VL, D, VP, EP, GP >, VP, EP >(g);
}

// user methods
//=========================================================================
} // boost
#endif
