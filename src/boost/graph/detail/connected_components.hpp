//=======================================================================
// Copyright 1997, 1998, 1999, 2000 University of Notre Dame.
// Authors: Andrew Lumsdaine, Lie-Quan Lee, Jeremy G. Siek
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================
#ifndef BOOST_GRAPH_DETAIL_CONNECTED_COMPONENTS_HPP
#define BOOST_GRAPH_DETAIL_CONNECTED_COMPONENTS_HPP

#if defined(__sgi) && !defined(__GNUC__)
#pragma set woff 1234
#endif

#include <boost/operators.hpp>

namespace boost
{

namespace detail
{

    //=========================================================================
    // Implementation details of connected_components

    // This is used both in the connected_components algorithm and in
    // the kosaraju strong components algorithm during the second DFS
    // traversal.
    template < class ComponentsPA, class DFSVisitor >
    class components_recorder : public DFSVisitor
    {
        typedef typename property_traits< ComponentsPA >::value_type comp_type;

    public:
        components_recorder(ComponentsPA c, comp_type& c_count, DFSVisitor v)
        : DFSVisitor(v), m_component(c), m_count(c_count)
        {
        }

        template < class Vertex, class Graph >
        void start_vertex(Vertex u, Graph& g)
        {
            ++m_count;
            DFSVisitor::start_vertex(u, g);
        }
        template < class Vertex, class Graph >
        void discover_vertex(Vertex u, Graph& g)
        {
            put(m_component, u, m_count);
            DFSVisitor::discover_vertex(u, g);
        }

    protected:
        ComponentsPA m_component;
        comp_type& m_count;
    };

    template < class DiscoverTimeMap, class FinishTimeMap, class TimeT,
        class DFSVisitor >
    class time_recorder : public DFSVisitor
    {
    public:
        time_recorder(
            DiscoverTimeMap d, FinishTimeMap f, TimeT& t, DFSVisitor v)
        : DFSVisitor(v), m_discover_time(d), m_finish_time(f), m_t(t)
        {
        }

        template < class Vertex, class Graph >
        void discover_vertex(Vertex u, Graph& g)
        {
            put(m_discover_time, u, ++m_t);
            DFSVisitor::discover_vertex(u, g);
        }
        template < class Vertex, class Graph >
        void finish_vertex(Vertex u, Graph& g)
        {
            put(m_finish_time, u, ++m_t);
            DFSVisitor::discover_vertex(u, g);
        }

    protected:
        DiscoverTimeMap m_discover_time;
        FinishTimeMap m_finish_time;
        TimeT m_t;
    };
    template < class DiscoverTimeMap, class FinishTimeMap, class TimeT,
        class DFSVisitor >
    time_recorder< DiscoverTimeMap, FinishTimeMap, TimeT, DFSVisitor >
    record_times(DiscoverTimeMap d, FinishTimeMap f, TimeT& t, DFSVisitor vis)
    {
        return time_recorder< DiscoverTimeMap, FinishTimeMap, TimeT,
            DFSVisitor >(d, f, t, vis);
    }

    //=========================================================================
    // Implementation detail of dynamic_components

    //-------------------------------------------------------------------------
    // Helper functions for the component_index class

    // Record the representative vertices in the header array.
    // Representative vertices now point to the component number.

    template < class Parent, class OutputIterator, class Integer >
    inline void build_components_header(
        Parent p, OutputIterator header, Integer num_nodes)
    {
        Parent component = p;
        Integer component_num = 0;
        for (Integer v = 0; v != num_nodes; ++v)
            if (p[v] == v)
            {
                *header++ = v;
                component[v] = component_num++;
            }
    }

    // Pushes x onto the front of the list. The list is represented in
    // an array.
    template < class Next, class T, class V >
    inline void push_front(Next next, T& head, V x)
    {
        T tmp = head;
        head = x;
        next[x] = tmp;
    }

    // Create a linked list of the vertices in each component
    // by reusing the representative array.
    template < class Parent1, class Parent2, class Integer >
    void link_components(Parent1 component, Parent2 header, Integer num_nodes,
        Integer num_components)
    {
        // Make the non-representative vertices point to their component
        Parent1 representative = component;
        for (Integer v = 0; v != num_nodes; ++v)
            if (component[v] >= num_components || header[component[v]] != v)
                component[v] = component[representative[v]];

        // initialize the "head" of the lists to "NULL"
        std::fill_n(header, num_components, num_nodes);

        // Add each vertex to the linked list for its component
        Parent1 next = component;
        for (Integer k = 0; k != num_nodes; ++k)
            push_front(next, header[component[k]], k);
    }

    template < class IndexContainer, class HeaderContainer >
    void construct_component_index(
        IndexContainer& index, HeaderContainer& header)
    {
        build_components_header(index.begin(), std::back_inserter(header),
            index.end() - index.begin());

        link_components(index.begin(), header.begin(),
            index.end() - index.begin(), header.end() - header.begin());
    }

    template < class IndexIterator, class Integer, class Distance >
    class component_iterator
    : boost::forward_iterator_helper<
          component_iterator< IndexIterator, Integer, Distance >, Integer,
          Distance, Integer*, Integer& >
    {
    public:
        typedef component_iterator self;

        IndexIterator next;
        Integer node;

        typedef std::forward_iterator_tag iterator_category;
        typedef Integer value_type;
        typedef Integer& reference;
        typedef Integer* pointer;
        typedef Distance difference_type;

        component_iterator() {}
        component_iterator(IndexIterator x, Integer i) : next(x), node(i) {}
        Integer operator*() const { return node; }
        self& operator++()
        {
            node = next[node];
            return *this;
        }
    };

    template < class IndexIterator, class Integer, class Distance >
    inline bool operator==(
        const component_iterator< IndexIterator, Integer, Distance >& x,
        const component_iterator< IndexIterator, Integer, Distance >& y)
    {
        return x.node == y.node;
    }

} // namespace detail

} // namespace detail

#if defined(__sgi) && !defined(__GNUC__)
#pragma reset woff 1234
#endif

#endif
