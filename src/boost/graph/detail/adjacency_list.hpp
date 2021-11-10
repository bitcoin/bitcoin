// -*- c++ -*-
//=======================================================================
// Copyright 1997, 1998, 1999, 2000 University of Notre Dame.
// Copyright 2010 Thomas Claveirole
// Authors: Andrew Lumsdaine, Lie-Quan Lee, Jeremy G. Siek, Thomas Claveirole
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#ifndef BOOST_GRAPH_DETAIL_ADJACENCY_LIST_HPP
#define BOOST_GRAPH_DETAIL_ADJACENCY_LIST_HPP

#include <map> // for vertex_map in copy_impl
#include <boost/config.hpp>
#include <boost/detail/workaround.hpp>
#include <boost/operators.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/pending/container_traits.hpp>
#include <boost/range/irange.hpp>
#include <boost/graph/graph_traits.hpp>
#include <memory>
#include <iterator>
#include <algorithm>
#include <boost/limits.hpp>

#include <boost/iterator/iterator_adaptor.hpp>

#include <boost/mpl/if.hpp>
#include <boost/mpl/not.hpp>
#include <boost/mpl/and.hpp>
#include <boost/graph/graph_concepts.hpp>
#include <boost/pending/container_traits.hpp>
#include <boost/graph/detail/adj_list_edge_iterator.hpp>
#include <boost/graph/properties.hpp>
#include <boost/pending/property.hpp>
#include <boost/graph/adjacency_iterator.hpp>
#include <boost/static_assert.hpp>
#include <boost/assert.hpp>

#ifdef BOOST_NO_CXX11_RVALUE_REFERENCES
#define BOOST_GRAPH_MOVE_IF_POSSIBLE(x) (x)
#else
#include <utility>
#define BOOST_GRAPH_MOVE_IF_POSSIBLE(x) (std::move((x)))
#endif

/*
  Outline for this file:

  out_edge_iterator and in_edge_iterator implementation
  edge_iterator for undirected graph
  stored edge types (these object live in the out-edge/in-edge lists)
  directed edges helper class
  directed graph helper class
  undirected graph helper class
  bidirectional graph helper class
  bidirectional graph helper class (without edge properties)
  bidirectional graph helper class (with edge properties)
  adjacency_list helper class
  adj_list_impl class
  vec_adj_list_impl class
  adj_list_gen class
  vertex property map
  edge property map


  Note: it would be nice to merge some of the undirected and
  bidirectional code... it is awful similar.
 */

namespace boost
{

namespace detail
{

    template < typename DirectedS > struct directed_category_traits
    {
        typedef directed_tag directed_category;
    };

    template <> struct directed_category_traits< directedS >
    {
        typedef directed_tag directed_category;
    };
    template <> struct directed_category_traits< undirectedS >
    {
        typedef undirected_tag directed_category;
    };
    template <> struct directed_category_traits< bidirectionalS >
    {
        typedef bidirectional_tag directed_category;
    };

    template < class Vertex > struct target_is
    {
        target_is(const Vertex& v) : m_target(v) {}
        template < class StoredEdge > bool operator()(const StoredEdge& e) const
        {
            return e.get_target() == m_target;
        }
        Vertex m_target;
    };

    // O(E/V)
    template < class EdgeList, class vertex_descriptor >
    void erase_from_incidence_list(
        EdgeList& el, vertex_descriptor v, allow_parallel_edge_tag)
    {
        boost::graph_detail::erase_if(
            el, detail::target_is< vertex_descriptor >(v));
    }
    // O(log(E/V))
    template < class EdgeList, class vertex_descriptor >
    void erase_from_incidence_list(
        EdgeList& el, vertex_descriptor v, disallow_parallel_edge_tag)
    {
        typedef typename EdgeList::value_type StoredEdge;
        el.erase(StoredEdge(v));
    }

    //=========================================================================
    // Out-Edge and In-Edge Iterator Implementation

    template < class BaseIter, class VertexDescriptor, class EdgeDescriptor,
        class Difference >
    struct out_edge_iter
    : iterator_adaptor< out_edge_iter< BaseIter, VertexDescriptor,
                            EdgeDescriptor, Difference >,
          BaseIter, EdgeDescriptor, use_default, EdgeDescriptor, Difference >
    {
        typedef iterator_adaptor< out_edge_iter< BaseIter, VertexDescriptor,
                                      EdgeDescriptor, Difference >,
            BaseIter, EdgeDescriptor, use_default, EdgeDescriptor, Difference >
            super_t;

        inline out_edge_iter() {}
        inline out_edge_iter(const BaseIter& i, const VertexDescriptor& src)
        : super_t(i), m_src(src)
        {
        }

        inline EdgeDescriptor dereference() const
        {
            return EdgeDescriptor(m_src, (*this->base()).get_target(),
                &(*this->base()).get_property());
        }
        VertexDescriptor m_src;
    };

    template < class BaseIter, class VertexDescriptor, class EdgeDescriptor,
        class Difference >
    struct in_edge_iter
    : iterator_adaptor< in_edge_iter< BaseIter, VertexDescriptor,
                            EdgeDescriptor, Difference >,
          BaseIter, EdgeDescriptor, use_default, EdgeDescriptor, Difference >
    {
        typedef iterator_adaptor< in_edge_iter< BaseIter, VertexDescriptor,
                                      EdgeDescriptor, Difference >,
            BaseIter, EdgeDescriptor, use_default, EdgeDescriptor, Difference >
            super_t;

        inline in_edge_iter() {}
        inline in_edge_iter(const BaseIter& i, const VertexDescriptor& src)
        : super_t(i), m_src(src)
        {
        }

        inline EdgeDescriptor dereference() const
        {
            return EdgeDescriptor((*this->base()).get_target(), m_src,
                &this->base()->get_property());
        }
        VertexDescriptor m_src;
    };

    //=========================================================================
    // Undirected Edge Iterator Implementation

    template < class EdgeIter, class EdgeDescriptor, class Difference >
    struct undirected_edge_iter
    : iterator_adaptor<
          undirected_edge_iter< EdgeIter, EdgeDescriptor, Difference >,
          EdgeIter, EdgeDescriptor, use_default, EdgeDescriptor, Difference >
    {
        typedef iterator_adaptor<
            undirected_edge_iter< EdgeIter, EdgeDescriptor, Difference >,
            EdgeIter, EdgeDescriptor, use_default, EdgeDescriptor, Difference >
            super_t;

        undirected_edge_iter() {}

        explicit undirected_edge_iter(EdgeIter i) : super_t(i) {}

        inline EdgeDescriptor dereference() const
        {
            return EdgeDescriptor((*this->base()).m_source,
                (*this->base()).m_target, &this->base()->get_property());
        }
    };

    //=========================================================================
    // Edge Storage Types (stored in the out-edge/in-edge lists)

    template < class Vertex >
    class stored_edge
    : public boost::equality_comparable1< stored_edge< Vertex >,
          boost::less_than_comparable1< stored_edge< Vertex > > >
    {
    public:
        typedef no_property property_type;
        inline stored_edge() {}
        inline stored_edge(Vertex target, const no_property& = no_property())
        : m_target(target)
        {
        }
        inline Vertex& get_target() const { return m_target; }
        inline const no_property& get_property() const { return s_prop; }
        inline bool operator==(const stored_edge& x) const
        {
            return m_target == x.get_target();
        }
        inline bool operator<(const stored_edge& x) const
        {
            return m_target < x.get_target();
        }
        // protected: need to add hash<> as a friend
        static no_property s_prop;
        // Sometimes target not used as key in the set, and in that case
        // it is ok to change the target.
        mutable Vertex m_target;
    };
    template < class Vertex > no_property stored_edge< Vertex >::s_prop;

#if defined(BOOST_NO_CXX11_RVALUE_REFERENCES) \
    || defined(BOOST_NO_CXX11_SMART_PTR)
    template < class Vertex, class Property >
    class stored_edge_property : public stored_edge< Vertex >
    {
        typedef stored_edge_property self;
        typedef stored_edge< Vertex > Base;

    public:
        typedef Property property_type;
        inline stored_edge_property() {}
        inline stored_edge_property(
            Vertex target, const Property& p = Property())
        : stored_edge< Vertex >(target), m_property(new Property(p))
        {
        }
        stored_edge_property(const self& x)
        : Base(static_cast< Base const& >(x))
        , m_property(const_cast< self& >(x).m_property)
        {
        }
        self& operator=(const self& x)
        {
            // NOTE: avoid 'Base::operator=(x);' broken on SGI MIPSpro (bug
            // 55771 of Mozilla).
            static_cast< Base& >(*this) = static_cast< Base const& >(x);
            m_property = const_cast< self& >(x).m_property;
            return *this;
        }
#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        // NOTE Don't rely on default operators, their behavior is broken on
        // several compilers (GCC 4.6).
        stored_edge_property(self&& x)
        : Base(static_cast< Base&& >(x)), m_property(std::move(x.m_property))
        {
        }
        self& operator=(self&& x)
        {
            // NOTE: avoid 'Base::operator=(x);' broken on SGI MIPSpro (bug
            // 55771 of Mozilla).
            static_cast< Base& >(*this) = static_cast< Base&& >(x);
            m_property = std::move(x.m_property);
            return *this;
        }
#endif
        inline Property& get_property() { return *m_property; }
        inline const Property& get_property() const { return *m_property; }

    protected:
        // Holding the property by-value causes edge-descriptor
        // invalidation for add_edge() with EdgeList=vecS. Instead we
        // hold a pointer to the property. std::auto_ptr is not
        // a perfect fit for the job, but it is darn close.
#ifdef BOOST_NO_AUTO_PTR
        std::unique_ptr< Property > m_property;
#else
        std::auto_ptr< Property > m_property;
#endif
    };
#else
    template < class Vertex, class Property >
    class stored_edge_property : public stored_edge< Vertex >
    {
        typedef stored_edge_property self;
        typedef stored_edge< Vertex > Base;

    public:
        typedef Property property_type;
        inline stored_edge_property() {}
        inline stored_edge_property(
            Vertex target, const Property& p = Property())
        : stored_edge< Vertex >(target), m_property(new Property(p))
        {
        }
        stored_edge_property(self&& x)
        : Base(static_cast< Base&& >(x)), m_property(std::move(x.m_property))
        {
        }
        stored_edge_property(self const& x)
        : Base(static_cast< Base const& >(x))
        , m_property(std::move(const_cast< self& >(x).m_property))
        {
        }
        self& operator=(self&& x)
        {
            // NOTE: avoid 'Base::operator=(x);' broken on SGI MIPSpro (bug
            // 55771 of Mozilla).
            static_cast< Base& >(*this) = static_cast< Base&& >(x);
            m_property = std::move(x.m_property);
            return *this;
        }
        self& operator=(self const& x)
        {
            // NOTE: avoid 'Base::operator=(x);' broken on SGI MIPSpro (bug
            // 55771 of Mozilla).
            static_cast< Base& >(*this) = static_cast< Base const& >(x);
            m_property = std::move(const_cast< self& >(x).m_property);
            return *this;
        }
        inline Property& get_property() { return *m_property; }
        inline const Property& get_property() const { return *m_property; }

    protected:
        std::unique_ptr< Property > m_property;
    };
#endif

    template < class Vertex, class Iter, class Property >
    class stored_edge_iter : public stored_edge< Vertex >
    {
    public:
        typedef Property property_type;
        inline stored_edge_iter() {}
        inline stored_edge_iter(Vertex v) : stored_edge< Vertex >(v) {}
        inline stored_edge_iter(Vertex v, Iter i, void* = 0)
        : stored_edge< Vertex >(v), m_iter(i)
        {
        }
        inline Property& get_property() { return m_iter->get_property(); }
        inline const Property& get_property() const
        {
            return m_iter->get_property();
        }
        inline Iter get_iter() const { return m_iter; }

    protected:
        Iter m_iter;
    };

    // For when the EdgeList is a std::vector.
    // Want to make the iterator stable, so use an offset
    // instead of an iterator into a std::vector
    template < class Vertex, class EdgeVec, class Property >
    class stored_ra_edge_iter : public stored_edge< Vertex >
    {
        typedef typename EdgeVec::iterator Iter;

    public:
        typedef Property property_type;
        inline stored_ra_edge_iter() {}
        inline explicit stored_ra_edge_iter(
            Vertex v) // Only used for comparisons
        : stored_edge< Vertex >(v), m_i(0), m_vec(0)
        {
        }
        inline stored_ra_edge_iter(Vertex v, Iter i, EdgeVec* edge_vec)
        : stored_edge< Vertex >(v), m_i(i - edge_vec->begin()), m_vec(edge_vec)
        {
        }
        inline Property& get_property()
        {
            BOOST_ASSERT((m_vec != 0));
            return (*m_vec)[m_i].get_property();
        }
        inline const Property& get_property() const
        {
            BOOST_ASSERT((m_vec != 0));
            return (*m_vec)[m_i].get_property();
        }
        inline Iter get_iter() const
        {
            BOOST_ASSERT((m_vec != 0));
            return m_vec->begin() + m_i;
        }

    protected:
        std::size_t m_i;
        EdgeVec* m_vec;
    };

} // namespace detail

template < class Tag, class Vertex, class Property >
const typename property_value< Property, Tag >::type& get(
    Tag property_tag, const detail::stored_edge_property< Vertex, Property >& e)
{
    return get_property_value(e.get_property(), property_tag);
}

template < class Tag, class Vertex, class Iter, class Property >
const typename property_value< Property, Tag >::type& get(Tag property_tag,
    const detail::stored_edge_iter< Vertex, Iter, Property >& e)
{
    return get_property_value(e.get_property(), property_tag);
}

template < class Tag, class Vertex, class EdgeVec, class Property >
const typename property_value< Property, Tag >::type& get(Tag property_tag,
    const detail::stored_ra_edge_iter< Vertex, EdgeVec, Property >& e)
{
    return get_property_value(e.get_property(), property_tag);
}

//=========================================================================
// Directed Edges Helper Class

namespace detail
{

    // O(E/V)
    template < class edge_descriptor, class EdgeList, class StoredProperty >
    inline void remove_directed_edge_dispatch(
        edge_descriptor, EdgeList& el, StoredProperty& p)
    {
        for (typename EdgeList::iterator i = el.begin(); i != el.end(); ++i)
            if (&(*i).get_property() == &p)
            {
                el.erase(i);
                return;
            }
    }

    template < class incidence_iterator, class EdgeList, class Predicate >
    inline void remove_directed_edge_if_dispatch(incidence_iterator first,
        incidence_iterator last, EdgeList& el, Predicate pred,
        boost::allow_parallel_edge_tag)
    {
        // remove_if
        while (first != last && !pred(*first))
            ++first;
        incidence_iterator i = first;
        if (first != last)
            for (++i; i != last; ++i)
                if (!pred(*i))
                {
                    *first.base() = BOOST_GRAPH_MOVE_IF_POSSIBLE(*i.base());
                    ++first;
                }
        el.erase(first.base(), el.end());
    }
    template < class incidence_iterator, class EdgeList, class Predicate >
    inline void remove_directed_edge_if_dispatch(incidence_iterator first,
        incidence_iterator last, EdgeList& el, Predicate pred,
        boost::disallow_parallel_edge_tag)
    {
        for (incidence_iterator next = first; first != last; first = next)
        {
            ++next;
            if (pred(*first))
                el.erase(first.base());
        }
    }

    template < class PropT, class Graph, class incidence_iterator,
        class EdgeList, class Predicate >
    inline void undirected_remove_out_edge_if_dispatch(Graph& g,
        incidence_iterator first, incidence_iterator last, EdgeList& el,
        Predicate pred, boost::allow_parallel_edge_tag)
    {
        typedef typename Graph::global_edgelist_selector EdgeListS;
        BOOST_STATIC_ASSERT((!is_same< EdgeListS, vecS >::value));

        // remove_if
        while (first != last && !pred(*first))
            ++first;
        incidence_iterator i = first;
        bool self_loop_removed = false;
        if (first != last)
            for (; i != last; ++i)
            {
                if (self_loop_removed)
                {
                    /* With self loops, the descriptor will show up
                     * twice. The first time it will be removed, and now it
                     * will be skipped.
                     */
                    self_loop_removed = false;
                }
                else if (!pred(*i))
                {
                    *first.base() = BOOST_GRAPH_MOVE_IF_POSSIBLE(*i.base());
                    ++first;
                }
                else
                {
                    if (source(*i, g) == target(*i, g))
                        self_loop_removed = true;
                    else
                    {
                        // Remove the edge from the target
                        detail::remove_directed_edge_dispatch(*i,
                            g.out_edge_list(target(*i, g)),
                            *(PropT*)(*i).get_property());
                    }

                    // Erase the edge property
                    g.m_edges.erase((*i.base()).get_iter());
                }
            }
        el.erase(first.base(), el.end());
    }
    template < class PropT, class Graph, class incidence_iterator,
        class EdgeList, class Predicate >
    inline void undirected_remove_out_edge_if_dispatch(Graph& g,
        incidence_iterator first, incidence_iterator last, EdgeList& el,
        Predicate pred, boost::disallow_parallel_edge_tag)
    {
        typedef typename Graph::global_edgelist_selector EdgeListS;
        BOOST_STATIC_ASSERT((!is_same< EdgeListS, vecS >::value));

        for (incidence_iterator next = first; first != last; first = next)
        {
            ++next;
            if (pred(*first))
            {
                if (source(*first, g) != target(*first, g))
                {
                    // Remove the edge from the target
                    detail::remove_directed_edge_dispatch(*first,
                        g.out_edge_list(target(*first, g)),
                        *(PropT*)(*first).get_property());
                }

                // Erase the edge property
                g.m_edges.erase((*first.base()).get_iter());

                // Erase the edge in the source
                el.erase(first.base());
            }
        }
    }

    // O(E/V)
    template < class edge_descriptor, class EdgeList, class StoredProperty >
    inline void remove_directed_edge_dispatch(
        edge_descriptor e, EdgeList& el, no_property&)
    {
        for (typename EdgeList::iterator i = el.begin(); i != el.end(); ++i)
            if ((*i).get_target() == e.m_target)
            {
                el.erase(i);
                return;
            }
    }

} // namespace detail

template < class Config > struct directed_edges_helper
{

    // Placement of these overloaded remove_edge() functions
    // inside the class avoids a VC++ bug.

    // O(E/V)
    inline void remove_edge(typename Config::edge_descriptor e)
    {
        typedef typename Config::graph_type graph_type;
        graph_type& g = static_cast< graph_type& >(*this);
        typename Config::OutEdgeList& el = g.out_edge_list(source(e, g));
        typedef typename Config::OutEdgeList::value_type::property_type PType;
        detail::remove_directed_edge_dispatch(e, el, *(PType*)e.get_property());
    }

    // O(1)
    inline void remove_edge(typename Config::out_edge_iterator iter)
    {
        typedef typename Config::graph_type graph_type;
        graph_type& g = static_cast< graph_type& >(*this);
        typename Config::edge_descriptor e = *iter;
        typename Config::OutEdgeList& el = g.out_edge_list(source(e, g));
        el.erase(iter.base());
    }
};

// O(1)
template < class Config >
inline std::pair< typename Config::edge_iterator,
    typename Config::edge_iterator >
edges(const directed_edges_helper< Config >& g_)
{
    typedef typename Config::graph_type graph_type;
    typedef typename Config::edge_iterator edge_iterator;
    const graph_type& cg = static_cast< const graph_type& >(g_);
    graph_type& g = const_cast< graph_type& >(cg);
    return std::make_pair(edge_iterator(g.vertex_set().begin(),
                              g.vertex_set().begin(), g.vertex_set().end(), g),
        edge_iterator(g.vertex_set().begin(), g.vertex_set().end(),
            g.vertex_set().end(), g));
}

//=========================================================================
// Directed Graph Helper Class

struct adj_list_dir_traversal_tag : public virtual vertex_list_graph_tag,
                                    public virtual incidence_graph_tag,
                                    public virtual adjacency_graph_tag,
                                    public virtual edge_list_graph_tag
{
};

template < class Config >
struct directed_graph_helper : public directed_edges_helper< Config >
{
    typedef typename Config::edge_descriptor edge_descriptor;
    typedef adj_list_dir_traversal_tag traversal_category;
};

// O(E/V)
template < class Config >
inline void remove_edge(typename Config::vertex_descriptor u,
    typename Config::vertex_descriptor v, directed_graph_helper< Config >& g_)
{
    typedef typename Config::graph_type graph_type;
    typedef typename Config::edge_parallel_category Cat;
    graph_type& g = static_cast< graph_type& >(g_);
    detail::erase_from_incidence_list(g.out_edge_list(u), v, Cat());
}

template < class Config, class Predicate >
inline void remove_out_edge_if(typename Config::vertex_descriptor u,
    Predicate pred, directed_graph_helper< Config >& g_)
{
    typedef typename Config::graph_type graph_type;
    graph_type& g = static_cast< graph_type& >(g_);
    typename Config::out_edge_iterator first, last;
    boost::tie(first, last) = out_edges(u, g);
    typedef typename Config::edge_parallel_category edge_parallel_category;
    detail::remove_directed_edge_if_dispatch(
        first, last, g.out_edge_list(u), pred, edge_parallel_category());
}

template < class Config, class Predicate >
inline void remove_edge_if(Predicate pred, directed_graph_helper< Config >& g_)
{
    typedef typename Config::graph_type graph_type;
    graph_type& g = static_cast< graph_type& >(g_);

    typename Config::vertex_iterator vi, vi_end;
    for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi)
        remove_out_edge_if(*vi, pred, g);
}

template < class EdgeOrIter, class Config >
inline void remove_edge(
    EdgeOrIter e_or_iter, directed_graph_helper< Config >& g_)
{
    g_.remove_edge(e_or_iter);
}

// O(V + E) for allow_parallel_edges
// O(V * log(E/V)) for disallow_parallel_edges
template < class Config >
inline void clear_vertex(
    typename Config::vertex_descriptor u, directed_graph_helper< Config >& g_)
{
    typedef typename Config::graph_type graph_type;
    typedef typename Config::edge_parallel_category Cat;
    graph_type& g = static_cast< graph_type& >(g_);
    typename Config::vertex_iterator vi, viend;
    for (boost::tie(vi, viend) = vertices(g); vi != viend; ++vi)
        detail::erase_from_incidence_list(g.out_edge_list(*vi), u, Cat());
    g.out_edge_list(u).clear();
    // clear() should be a req of Sequence and AssociativeContainer,
    // or maybe just Container
}

template < class Config >
inline void clear_out_edges(
    typename Config::vertex_descriptor u, directed_graph_helper< Config >& g_)
{
    typedef typename Config::graph_type graph_type;
    graph_type& g = static_cast< graph_type& >(g_);
    g.out_edge_list(u).clear();
    // clear() should be a req of Sequence and AssociativeContainer,
    // or maybe just Container
}

// O(V), could do better...
template < class Config >
inline typename Config::edges_size_type num_edges(
    const directed_graph_helper< Config >& g_)
{
    typedef typename Config::graph_type graph_type;
    const graph_type& g = static_cast< const graph_type& >(g_);
    typename Config::edges_size_type num_e = 0;
    typename Config::vertex_iterator vi, vi_end;
    for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi)
        num_e += out_degree(*vi, g);
    return num_e;
}
// O(1) for allow_parallel_edge_tag
// O(log(E/V)) for disallow_parallel_edge_tag
template < class Config >
inline std::pair< typename directed_graph_helper< Config >::edge_descriptor,
    bool >
add_edge(typename Config::vertex_descriptor u,
    typename Config::vertex_descriptor v,
    const typename Config::edge_property_type& p,
    directed_graph_helper< Config >& g_)
{
    typedef typename Config::edge_descriptor edge_descriptor;
    typedef typename Config::graph_type graph_type;
    typedef typename Config::StoredEdge StoredEdge;
    graph_type& g = static_cast< graph_type& >(g_);
    typename Config::OutEdgeList::iterator i;
    bool inserted;
    boost::tie(i, inserted)
        = boost::graph_detail::push(g.out_edge_list(u), StoredEdge(v, p));
    return std::make_pair(
        edge_descriptor(u, v, &(*i).get_property()), inserted);
}
// Did not use default argument here because that
// causes Visual C++ to get confused.
template < class Config >
inline std::pair< typename Config::edge_descriptor, bool > add_edge(
    typename Config::vertex_descriptor u, typename Config::vertex_descriptor v,
    directed_graph_helper< Config >& g_)
{
    typename Config::edge_property_type p;
    return add_edge(u, v, p, g_);
}
//=========================================================================
// Undirected Graph Helper Class

template < class Config > struct undirected_graph_helper;

struct undir_adj_list_traversal_tag : public virtual vertex_list_graph_tag,
                                      public virtual incidence_graph_tag,
                                      public virtual adjacency_graph_tag,
                                      public virtual edge_list_graph_tag,
                                      public virtual bidirectional_graph_tag
{
};

namespace detail
{

    // using class with specialization for dispatch is a VC++ workaround.
    template < class StoredProperty > struct remove_undirected_edge_dispatch
    {

        // O(E/V)
        template < class edge_descriptor, class Config >
        static void apply(edge_descriptor e,
            undirected_graph_helper< Config >& g_, StoredProperty& p)
        {
            typedef typename Config::global_edgelist_selector EdgeListS;
            BOOST_STATIC_ASSERT((!is_same< EdgeListS, vecS >::value));

            typedef typename Config::graph_type graph_type;
            graph_type& g = static_cast< graph_type& >(g_);

            typename Config::OutEdgeList& out_el
                = g.out_edge_list(source(e, g));
            typename Config::OutEdgeList::iterator out_i = out_el.begin();
            typename Config::EdgeIter edge_iter_to_erase;
            for (; out_i != out_el.end(); ++out_i)
                if (&(*out_i).get_property() == &p)
                {
                    edge_iter_to_erase = (*out_i).get_iter();
                    out_el.erase(out_i);
                    break;
                }
            typename Config::OutEdgeList& in_el = g.out_edge_list(target(e, g));
            typename Config::OutEdgeList::iterator in_i = in_el.begin();
            for (; in_i != in_el.end(); ++in_i)
                if (&(*in_i).get_property() == &p)
                {
                    in_el.erase(in_i);
                    break;
                }
            g.m_edges.erase(edge_iter_to_erase);
        }
    };

    template <> struct remove_undirected_edge_dispatch< no_property >
    {
        // O(E/V)
        template < class edge_descriptor, class Config >
        static void apply(edge_descriptor e,
            undirected_graph_helper< Config >& g_, no_property&)
        {
            typedef typename Config::global_edgelist_selector EdgeListS;
            BOOST_STATIC_ASSERT((!is_same< EdgeListS, vecS >::value));

            typedef typename Config::graph_type graph_type;
            graph_type& g = static_cast< graph_type& >(g_);
            no_property* p = (no_property*)e.get_property();
            typename Config::OutEdgeList& out_el
                = g.out_edge_list(source(e, g));
            typename Config::OutEdgeList::iterator out_i = out_el.begin();
            typename Config::EdgeIter edge_iter_to_erase;
            for (; out_i != out_el.end(); ++out_i)
                if (&(*out_i).get_property() == p)
                {
                    edge_iter_to_erase = (*out_i).get_iter();
                    out_el.erase(out_i);
                    break;
                }
            typename Config::OutEdgeList& in_el = g.out_edge_list(target(e, g));
            typename Config::OutEdgeList::iterator in_i = in_el.begin();
            for (; in_i != in_el.end(); ++in_i)
                if (&(*in_i).get_property() == p)
                {
                    in_el.erase(in_i);
                    break;
                }
            g.m_edges.erase(edge_iter_to_erase);
        }
    };

    // O(E/V)
    template < class Graph, class EdgeList, class Vertex >
    inline void remove_edge_and_property(
        Graph& g, EdgeList& el, Vertex v, boost::allow_parallel_edge_tag cat)
    {
        typedef typename Graph::global_edgelist_selector EdgeListS;
        BOOST_STATIC_ASSERT((!is_same< EdgeListS, vecS >::value));

        typename EdgeList::iterator i = el.begin(), end = el.end();
        for (; i != end; ++i)
        {
            if ((*i).get_target() == v)
            {
                // NOTE: Wihtout this skip, this loop will double-delete
                // properties of loop edges. This solution is based on the
                // observation that the incidence edges of a vertex with a loop
                // are adjacent in the out edge list. This *may* actually hold
                // for multisets also.
                bool skip = (boost::next(i) != end
                    && i->get_iter() == boost::next(i)->get_iter());
                g.m_edges.erase((*i).get_iter());
                if (skip)
                    ++i;
            }
        }
        detail::erase_from_incidence_list(el, v, cat);
    }
    // O(log(E/V))
    template < class Graph, class EdgeList, class Vertex >
    inline void remove_edge_and_property(
        Graph& g, EdgeList& el, Vertex v, boost::disallow_parallel_edge_tag)
    {
        typedef typename Graph::global_edgelist_selector EdgeListS;
        BOOST_STATIC_ASSERT((!is_same< EdgeListS, vecS >::value));

        typedef typename EdgeList::value_type StoredEdge;
        typename EdgeList::iterator i = el.find(StoredEdge(v)), end = el.end();
        if (i != end)
        {
            g.m_edges.erase((*i).get_iter());
            el.erase(i);
        }
    }

} // namespace detail

template < class Vertex, class EdgeProperty >
struct list_edge // short name due to VC++ truncation and linker problems
: public boost::detail::edge_base< boost::undirected_tag, Vertex >
{
    typedef EdgeProperty property_type;
    typedef boost::detail::edge_base< boost::undirected_tag, Vertex > Base;
    list_edge(Vertex u, Vertex v, const EdgeProperty& p = EdgeProperty())
    : Base(u, v), m_property(p)
    {
    }
    EdgeProperty& get_property() { return m_property; }
    const EdgeProperty& get_property() const { return m_property; }
    // the following methods should never be used, but are needed
    // to make SGI MIPSpro C++ happy
    list_edge() {}
    bool operator==(const list_edge&) const { return false; }
    bool operator<(const list_edge&) const { return false; }
    EdgeProperty m_property;
};

template < class Config > struct undirected_graph_helper
{

    typedef undir_adj_list_traversal_tag traversal_category;

    // Placement of these overloaded remove_edge() functions
    // inside the class avoids a VC++ bug.

    // O(E/V)
    inline void remove_edge(typename Config::edge_descriptor e)
    {
        typedef typename Config::global_edgelist_selector EdgeListS;
        BOOST_STATIC_ASSERT((!is_same< EdgeListS, vecS >::value));

        typedef typename Config::OutEdgeList::value_type::property_type PType;
        detail::remove_undirected_edge_dispatch< PType >::apply(
            e, *this, *(PType*)e.get_property());
    }
    // O(E/V)
    inline void remove_edge(typename Config::out_edge_iterator iter)
    {
        this->remove_edge(*iter);
    }
};

// Had to make these non-members to avoid accidental instantiation
// on SGI MIPSpro C++
template < class C >
inline typename C::InEdgeList& in_edge_list(
    undirected_graph_helper< C >&, typename C::vertex_descriptor v)
{
    typename C::stored_vertex* sv = (typename C::stored_vertex*)v;
    return sv->m_out_edges;
}
template < class C >
inline const typename C::InEdgeList& in_edge_list(
    const undirected_graph_helper< C >&, typename C::vertex_descriptor v)
{
    typename C::stored_vertex* sv = (typename C::stored_vertex*)v;
    return sv->m_out_edges;
}

// O(E/V)
template < class EdgeOrIter, class Config >
inline void remove_edge(EdgeOrIter e, undirected_graph_helper< Config >& g_)
{
    typedef typename Config::global_edgelist_selector EdgeListS;
    BOOST_STATIC_ASSERT((!is_same< EdgeListS, vecS >::value));

    g_.remove_edge(e);
}

// O(E/V) or O(log(E/V))
template < class Config >
void remove_edge(typename Config::vertex_descriptor u,
    typename Config::vertex_descriptor v, undirected_graph_helper< Config >& g_)
{
    typedef typename Config::global_edgelist_selector EdgeListS;
    BOOST_STATIC_ASSERT((!is_same< EdgeListS, vecS >::value));

    typedef typename Config::graph_type graph_type;
    graph_type& g = static_cast< graph_type& >(g_);
    typedef typename Config::edge_parallel_category Cat;
    detail::remove_edge_and_property(g, g.out_edge_list(u), v, Cat());
    detail::erase_from_incidence_list(g.out_edge_list(v), u, Cat());
}

template < class Config, class Predicate >
void remove_out_edge_if(typename Config::vertex_descriptor u, Predicate pred,
    undirected_graph_helper< Config >& g_)
{
    typedef typename Config::global_edgelist_selector EdgeListS;
    BOOST_STATIC_ASSERT((!is_same< EdgeListS, vecS >::value));

    typedef typename Config::graph_type graph_type;
    typedef typename Config::OutEdgeList::value_type::property_type PropT;
    graph_type& g = static_cast< graph_type& >(g_);
    typename Config::out_edge_iterator first, last;
    boost::tie(first, last) = out_edges(u, g);
    typedef typename Config::edge_parallel_category Cat;
    detail::undirected_remove_out_edge_if_dispatch< PropT >(
        g, first, last, g.out_edge_list(u), pred, Cat());
}
template < class Config, class Predicate >
void remove_in_edge_if(typename Config::vertex_descriptor u, Predicate pred,
    undirected_graph_helper< Config >& g_)
{
    typedef typename Config::global_edgelist_selector EdgeListS;
    BOOST_STATIC_ASSERT((!is_same< EdgeListS, vecS >::value));

    remove_out_edge_if(u, pred, g_);
}

// O(E/V * E) or O(log(E/V) * E)
template < class Predicate, class Config >
void remove_edge_if(Predicate pred, undirected_graph_helper< Config >& g_)
{
    typedef typename Config::global_edgelist_selector EdgeListS;
    BOOST_STATIC_ASSERT((!is_same< EdgeListS, vecS >::value));

    typedef typename Config::graph_type graph_type;
    graph_type& g = static_cast< graph_type& >(g_);
    typename Config::edge_iterator ei, ei_end, next;
    boost::tie(ei, ei_end) = edges(g);
    for (next = ei; ei != ei_end; ei = next)
    {
        ++next;
        if (pred(*ei))
            remove_edge(*ei, g);
    }
}

// O(1)
template < class Config >
inline std::pair< typename Config::edge_iterator,
    typename Config::edge_iterator >
edges(const undirected_graph_helper< Config >& g_)
{
    typedef typename Config::graph_type graph_type;
    typedef typename Config::edge_iterator edge_iterator;
    const graph_type& cg = static_cast< const graph_type& >(g_);
    graph_type& g = const_cast< graph_type& >(cg);
    return std::make_pair(
        edge_iterator(g.m_edges.begin()), edge_iterator(g.m_edges.end()));
}
// O(1)
template < class Config >
inline typename Config::edges_size_type num_edges(
    const undirected_graph_helper< Config >& g_)
{
    typedef typename Config::graph_type graph_type;
    const graph_type& g = static_cast< const graph_type& >(g_);
    return g.m_edges.size();
}
// O(E/V * E/V)
template < class Config >
inline void clear_vertex(
    typename Config::vertex_descriptor u, undirected_graph_helper< Config >& g_)
{
    typedef typename Config::global_edgelist_selector EdgeListS;
    BOOST_STATIC_ASSERT((!is_same< EdgeListS, vecS >::value));

    typedef typename Config::graph_type graph_type;
    graph_type& g = static_cast< graph_type& >(g_);
    while (true)
    {
        typename Config::out_edge_iterator ei, ei_end;
        boost::tie(ei, ei_end) = out_edges(u, g);
        if (ei == ei_end)
            break;
        remove_edge(*ei, g);
    }
}
// O(1) for allow_parallel_edge_tag
// O(log(E/V)) for disallow_parallel_edge_tag
template < class Config >
inline std::pair< typename Config::edge_descriptor, bool > add_edge(
    typename Config::vertex_descriptor u, typename Config::vertex_descriptor v,
    const typename Config::edge_property_type& p,
    undirected_graph_helper< Config >& g_)
{
    typedef typename Config::StoredEdge StoredEdge;
    typedef typename Config::edge_descriptor edge_descriptor;
    typedef typename Config::graph_type graph_type;
    graph_type& g = static_cast< graph_type& >(g_);

    bool inserted;
    typename Config::EdgeContainer::value_type e(u, v, p);
    typename Config::EdgeContainer::iterator p_iter
        = graph_detail::push(g.m_edges, e).first;

    typename Config::OutEdgeList::iterator i;
    boost::tie(i, inserted) = boost::graph_detail::push(
        g.out_edge_list(u), StoredEdge(v, p_iter, &g.m_edges));
    if (inserted)
    {
        boost::graph_detail::push(
            g.out_edge_list(v), StoredEdge(u, p_iter, &g.m_edges));
        return std::make_pair(
            edge_descriptor(u, v, &p_iter->get_property()), true);
    }
    else
    {
        g.m_edges.erase(p_iter);
        return std::make_pair(
            edge_descriptor(u, v, &i->get_iter()->get_property()), false);
    }
}
template < class Config >
inline std::pair< typename Config::edge_descriptor, bool > add_edge(
    typename Config::vertex_descriptor u, typename Config::vertex_descriptor v,
    undirected_graph_helper< Config >& g_)
{
    typename Config::edge_property_type p;
    return add_edge(u, v, p, g_);
}

// O(1)
template < class Config >
inline typename Config::degree_size_type degree(
    typename Config::vertex_descriptor u,
    const undirected_graph_helper< Config >& g_)
{
    typedef typename Config::graph_type Graph;
    const Graph& g = static_cast< const Graph& >(g_);
    return out_degree(u, g);
}

template < class Config >
inline std::pair< typename Config::in_edge_iterator,
    typename Config::in_edge_iterator >
in_edges(typename Config::vertex_descriptor u,
    const undirected_graph_helper< Config >& g_)
{
    typedef typename Config::graph_type Graph;
    const Graph& cg = static_cast< const Graph& >(g_);
    Graph& g = const_cast< Graph& >(cg);
    typedef typename Config::in_edge_iterator in_edge_iterator;
    return std::make_pair(in_edge_iterator(g.out_edge_list(u).begin(), u),
        in_edge_iterator(g.out_edge_list(u).end(), u));
}

template < class Config >
inline typename Config::degree_size_type in_degree(
    typename Config::vertex_descriptor u,
    const undirected_graph_helper< Config >& g_)
{
    return degree(u, g_);
}

//=========================================================================
// Bidirectional Graph Helper Class

struct bidir_adj_list_traversal_tag : public virtual vertex_list_graph_tag,
                                      public virtual incidence_graph_tag,
                                      public virtual adjacency_graph_tag,
                                      public virtual edge_list_graph_tag,
                                      public virtual bidirectional_graph_tag
{
};

template < class Config >
struct bidirectional_graph_helper : public directed_edges_helper< Config >
{
    typedef bidir_adj_list_traversal_tag traversal_category;
};

// Had to make these non-members to avoid accidental instantiation
// on SGI MIPSpro C++
template < class C >
inline typename C::InEdgeList& in_edge_list(
    bidirectional_graph_helper< C >&, typename C::vertex_descriptor v)
{
    typename C::stored_vertex* sv = (typename C::stored_vertex*)v;
    return sv->m_in_edges;
}
template < class C >
inline const typename C::InEdgeList& in_edge_list(
    const bidirectional_graph_helper< C >&, typename C::vertex_descriptor v)
{
    typename C::stored_vertex* sv = (typename C::stored_vertex*)v;
    return sv->m_in_edges;
}

template < class Predicate, class Config >
inline void remove_edge_if(
    Predicate pred, bidirectional_graph_helper< Config >& g_)
{
    typedef typename Config::global_edgelist_selector EdgeListS;
    BOOST_STATIC_ASSERT((!is_same< EdgeListS, vecS >::value));

    typedef typename Config::graph_type graph_type;
    graph_type& g = static_cast< graph_type& >(g_);
    typename Config::edge_iterator ei, ei_end, next;
    boost::tie(ei, ei_end) = edges(g);
    for (next = ei; ei != ei_end; ei = next)
    {
        ++next;
        if (pred(*ei))
            remove_edge(*ei, g);
    }
}

template < class Config >
inline std::pair< typename Config::in_edge_iterator,
    typename Config::in_edge_iterator >
in_edges(typename Config::vertex_descriptor u,
    const bidirectional_graph_helper< Config >& g_)
{
    typedef typename Config::graph_type graph_type;
    const graph_type& cg = static_cast< const graph_type& >(g_);
    graph_type& g = const_cast< graph_type& >(cg);
    typedef typename Config::in_edge_iterator in_edge_iterator;
    return std::make_pair(in_edge_iterator(in_edge_list(g, u).begin(), u),
        in_edge_iterator(in_edge_list(g, u).end(), u));
}

// O(1)
template < class Config >
inline std::pair< typename Config::edge_iterator,
    typename Config::edge_iterator >
edges(const bidirectional_graph_helper< Config >& g_)
{
    typedef typename Config::graph_type graph_type;
    typedef typename Config::edge_iterator edge_iterator;
    const graph_type& cg = static_cast< const graph_type& >(g_);
    graph_type& g = const_cast< graph_type& >(cg);
    return std::make_pair(
        edge_iterator(g.m_edges.begin()), edge_iterator(g.m_edges.end()));
}

//=========================================================================
// Bidirectional Graph Helper Class (with edge properties)

template < class Config >
struct bidirectional_graph_helper_with_property
: public bidirectional_graph_helper< Config >
{
    typedef typename Config::graph_type graph_type;
    typedef typename Config::out_edge_iterator out_edge_iterator;

    std::pair< out_edge_iterator, out_edge_iterator > get_parallel_edge_sublist(
        typename Config::edge_descriptor e, const graph_type& g, void*)
    {
        return out_edges(source(e, g), g);
    }

    std::pair< out_edge_iterator, out_edge_iterator > get_parallel_edge_sublist(
        typename Config::edge_descriptor e, const graph_type& g, setS*)
    {
        return edge_range(source(e, g), target(e, g), g);
    }

    std::pair< out_edge_iterator, out_edge_iterator > get_parallel_edge_sublist(
        typename Config::edge_descriptor e, const graph_type& g, multisetS*)
    {
        return edge_range(source(e, g), target(e, g), g);
    }

    std::pair< out_edge_iterator, out_edge_iterator > get_parallel_edge_sublist(
        typename Config::edge_descriptor e, const graph_type& g, hash_setS*)
    {
        return edge_range(source(e, g), target(e, g), g);
    }

    // Placement of these overloaded remove_edge() functions
    // inside the class avoids a VC++ bug.

    // O(E/V) or O(log(E/V))
    void remove_edge(typename Config::edge_descriptor e)
    {
        typedef typename Config::global_edgelist_selector EdgeListS;
        BOOST_STATIC_ASSERT((!is_same< EdgeListS, vecS >::value));

        graph_type& g = static_cast< graph_type& >(*this);

        typedef typename Config::edgelist_selector OutEdgeListS;

        std::pair< out_edge_iterator, out_edge_iterator > rng
            = get_parallel_edge_sublist(e, g, (OutEdgeListS*)(0));
        rng.first = std::find(rng.first, rng.second, e);
        BOOST_ASSERT(rng.first != rng.second);
        remove_edge(rng.first);
    }

    inline void remove_edge(typename Config::out_edge_iterator iter)
    {
        typedef typename Config::global_edgelist_selector EdgeListS;
        BOOST_STATIC_ASSERT((!is_same< EdgeListS, vecS >::value));

        typedef typename Config::graph_type graph_type;
        graph_type& g = static_cast< graph_type& >(*this);
        typename Config::edge_descriptor e = *iter;
        typename Config::OutEdgeList& oel = g.out_edge_list(source(e, g));
        typename Config::InEdgeList& iel = in_edge_list(g, target(e, g));
        typedef typename Config::OutEdgeList::value_type::property_type PType;
        PType& p = *(PType*)e.get_property();
        detail::remove_directed_edge_dispatch(*iter, iel, p);
        g.m_edges.erase(iter.base()->get_iter());
        oel.erase(iter.base());
    }
};

// O(E/V) for allow_parallel_edge_tag
// O(log(E/V)) for disallow_parallel_edge_tag
template < class Config >
inline void remove_edge(typename Config::vertex_descriptor u,
    typename Config::vertex_descriptor v,
    bidirectional_graph_helper_with_property< Config >& g_)
{
    typedef typename Config::global_edgelist_selector EdgeListS;
    BOOST_STATIC_ASSERT((!is_same< EdgeListS, vecS >::value));

    typedef typename Config::graph_type graph_type;
    graph_type& g = static_cast< graph_type& >(g_);
    typedef typename Config::edge_parallel_category Cat;
    detail::remove_edge_and_property(g, g.out_edge_list(u), v, Cat());
    detail::erase_from_incidence_list(in_edge_list(g, v), u, Cat());
}

// O(E/V) or O(log(E/V))
template < class EdgeOrIter, class Config >
inline void remove_edge(
    EdgeOrIter e, bidirectional_graph_helper_with_property< Config >& g_)
{
    typedef typename Config::global_edgelist_selector EdgeListS;
    BOOST_STATIC_ASSERT((!is_same< EdgeListS, vecS >::value));

    g_.remove_edge(e);
}

template < class Config, class Predicate >
inline void remove_out_edge_if(typename Config::vertex_descriptor u,
    Predicate pred, bidirectional_graph_helper_with_property< Config >& g_)
{
    typedef typename Config::global_edgelist_selector EdgeListS;
    BOOST_STATIC_ASSERT((!is_same< EdgeListS, vecS >::value));

    typedef typename Config::graph_type graph_type;
    typedef typename Config::OutEdgeList::value_type::property_type PropT;
    graph_type& g = static_cast< graph_type& >(g_);

    typedef typename Config::EdgeIter EdgeIter;
    typedef std::vector< EdgeIter > Garbage;
    Garbage garbage;

    // First remove the edges from the targets' in-edge lists and
    // from the graph's edge set list.
    typename Config::out_edge_iterator out_i, out_end;
    for (boost::tie(out_i, out_end) = out_edges(u, g); out_i != out_end;
         ++out_i)
        if (pred(*out_i))
        {
            detail::remove_directed_edge_dispatch(*out_i,
                in_edge_list(g, target(*out_i, g)),
                *(PropT*)(*out_i).get_property());
            // Put in garbage to delete later. Will need the properties
            // for the remove_if of the out-edges.
            garbage.push_back((*out_i.base()).get_iter());
        }

    // Now remove the edges from this out-edge list.
    typename Config::out_edge_iterator first, last;
    boost::tie(first, last) = out_edges(u, g);
    typedef typename Config::edge_parallel_category Cat;
    detail::remove_directed_edge_if_dispatch(
        first, last, g.out_edge_list(u), pred, Cat());

    // Now delete the edge properties from the g.m_edges list
    for (typename Garbage::iterator i = garbage.begin(); i != garbage.end();
         ++i)
        g.m_edges.erase(*i);
}
template < class Config, class Predicate >
inline void remove_in_edge_if(typename Config::vertex_descriptor v,
    Predicate pred, bidirectional_graph_helper_with_property< Config >& g_)
{
    typedef typename Config::global_edgelist_selector EdgeListS;
    BOOST_STATIC_ASSERT((!is_same< EdgeListS, vecS >::value));

    typedef typename Config::graph_type graph_type;
    typedef typename Config::OutEdgeList::value_type::property_type PropT;
    graph_type& g = static_cast< graph_type& >(g_);

    typedef typename Config::EdgeIter EdgeIter;
    typedef std::vector< EdgeIter > Garbage;
    Garbage garbage;

    // First remove the edges from the sources' out-edge lists and
    // from the graph's edge set list.
    typename Config::in_edge_iterator in_i, in_end;
    for (boost::tie(in_i, in_end) = in_edges(v, g); in_i != in_end; ++in_i)
        if (pred(*in_i))
        {
            typename Config::vertex_descriptor u = source(*in_i, g);
            detail::remove_directed_edge_dispatch(
                *in_i, g.out_edge_list(u), *(PropT*)(*in_i).get_property());
            // Put in garbage to delete later. Will need the properties
            // for the remove_if of the out-edges.
            garbage.push_back((*in_i.base()).get_iter());
        }
    // Now remove the edges from this in-edge list.
    typename Config::in_edge_iterator first, last;
    boost::tie(first, last) = in_edges(v, g);
    typedef typename Config::edge_parallel_category Cat;
    detail::remove_directed_edge_if_dispatch(
        first, last, in_edge_list(g, v), pred, Cat());

    // Now delete the edge properties from the g.m_edges list
    for (typename Garbage::iterator i = garbage.begin(); i != garbage.end();
         ++i)
        g.m_edges.erase(*i);
}

// O(1)
template < class Config >
inline typename Config::edges_size_type num_edges(
    const bidirectional_graph_helper_with_property< Config >& g_)
{
    typedef typename Config::graph_type graph_type;
    const graph_type& g = static_cast< const graph_type& >(g_);
    return g.m_edges.size();
}
// O(E/V * E/V) for allow_parallel_edge_tag
// O(E/V * log(E/V)) for disallow_parallel_edge_tag
template < class Config >
inline void clear_vertex(typename Config::vertex_descriptor u,
    bidirectional_graph_helper_with_property< Config >& g_)
{
    typedef typename Config::global_edgelist_selector EdgeListS;
    BOOST_STATIC_ASSERT((!is_same< EdgeListS, vecS >::value));

    typedef typename Config::graph_type graph_type;
    typedef typename Config::edge_parallel_category Cat;
    graph_type& g = static_cast< graph_type& >(g_);
    typename Config::OutEdgeList& el = g.out_edge_list(u);
    typename Config::OutEdgeList::iterator ei = el.begin(), ei_end = el.end();
    for (; ei != ei_end; ++ei)
    {
        detail::erase_from_incidence_list(
            in_edge_list(g, (*ei).get_target()), u, Cat());
        g.m_edges.erase((*ei).get_iter());
    }
    typename Config::InEdgeList& in_el = in_edge_list(g, u);
    typename Config::InEdgeList::iterator in_ei = in_el.begin(),
                                          in_ei_end = in_el.end();
    for (; in_ei != in_ei_end; ++in_ei)
    {
        detail::erase_from_incidence_list(
            g.out_edge_list((*in_ei).get_target()), u, Cat());
        g.m_edges.erase((*in_ei).get_iter());
    }
    g.out_edge_list(u).clear();
    in_edge_list(g, u).clear();
}

template < class Config >
inline void clear_out_edges(typename Config::vertex_descriptor u,
    bidirectional_graph_helper_with_property< Config >& g_)
{
    typedef typename Config::global_edgelist_selector EdgeListS;
    BOOST_STATIC_ASSERT((!is_same< EdgeListS, vecS >::value));

    typedef typename Config::graph_type graph_type;
    typedef typename Config::edge_parallel_category Cat;
    graph_type& g = static_cast< graph_type& >(g_);
    typename Config::OutEdgeList& el = g.out_edge_list(u);
    typename Config::OutEdgeList::iterator ei = el.begin(), ei_end = el.end();
    for (; ei != ei_end; ++ei)
    {
        detail::erase_from_incidence_list(
            in_edge_list(g, (*ei).get_target()), u, Cat());
        g.m_edges.erase((*ei).get_iter());
    }
    g.out_edge_list(u).clear();
}

template < class Config >
inline void clear_in_edges(typename Config::vertex_descriptor u,
    bidirectional_graph_helper_with_property< Config >& g_)
{
    typedef typename Config::global_edgelist_selector EdgeListS;
    BOOST_STATIC_ASSERT((!is_same< EdgeListS, vecS >::value));

    typedef typename Config::graph_type graph_type;
    typedef typename Config::edge_parallel_category Cat;
    graph_type& g = static_cast< graph_type& >(g_);
    typename Config::InEdgeList& in_el = in_edge_list(g, u);
    typename Config::InEdgeList::iterator in_ei = in_el.begin(),
                                          in_ei_end = in_el.end();
    for (; in_ei != in_ei_end; ++in_ei)
    {
        detail::erase_from_incidence_list(
            g.out_edge_list((*in_ei).get_target()), u, Cat());
        g.m_edges.erase((*in_ei).get_iter());
    }
    in_edge_list(g, u).clear();
}

// O(1) for allow_parallel_edge_tag
// O(log(E/V)) for disallow_parallel_edge_tag
template < class Config >
inline std::pair< typename Config::edge_descriptor, bool > add_edge(
    typename Config::vertex_descriptor u, typename Config::vertex_descriptor v,
    const typename Config::edge_property_type& p,
    bidirectional_graph_helper_with_property< Config >& g_)
{
    typedef typename Config::graph_type graph_type;
    graph_type& g = static_cast< graph_type& >(g_);
    typedef typename Config::edge_descriptor edge_descriptor;
    typedef typename Config::StoredEdge StoredEdge;
    bool inserted;
    typename Config::EdgeContainer::value_type e(u, v, p);
    typename Config::EdgeContainer::iterator p_iter
        = graph_detail::push(g.m_edges, e).first;
    typename Config::OutEdgeList::iterator i;
    boost::tie(i, inserted) = boost::graph_detail::push(
        g.out_edge_list(u), StoredEdge(v, p_iter, &g.m_edges));
    if (inserted)
    {
        boost::graph_detail::push(
            in_edge_list(g, v), StoredEdge(u, p_iter, &g.m_edges));
        return std::make_pair(edge_descriptor(u, v, &p_iter->m_property), true);
    }
    else
    {
        g.m_edges.erase(p_iter);
        return std::make_pair(
            edge_descriptor(u, v, &i->get_iter()->get_property()), false);
    }
}

template < class Config >
inline std::pair< typename Config::edge_descriptor, bool > add_edge(
    typename Config::vertex_descriptor u, typename Config::vertex_descriptor v,
    bidirectional_graph_helper_with_property< Config >& g_)
{
    typename Config::edge_property_type p;
    return add_edge(u, v, p, g_);
}
// O(1)
template < class Config >
inline typename Config::degree_size_type degree(
    typename Config::vertex_descriptor u,
    const bidirectional_graph_helper_with_property< Config >& g_)
{
    typedef typename Config::graph_type graph_type;
    const graph_type& g = static_cast< const graph_type& >(g_);
    return in_degree(u, g) + out_degree(u, g);
}

//=========================================================================
// Adjacency List Helper Class

template < class Config, class Base > struct adj_list_helper : public Base
{
    typedef typename Config::graph_type AdjList;
    typedef typename Config::vertex_descriptor vertex_descriptor;
    typedef typename Config::edge_descriptor edge_descriptor;
    typedef typename Config::out_edge_iterator out_edge_iterator;
    typedef typename Config::in_edge_iterator in_edge_iterator;
    typedef typename Config::adjacency_iterator adjacency_iterator;
    typedef typename Config::inv_adjacency_iterator inv_adjacency_iterator;
    typedef typename Config::vertex_iterator vertex_iterator;
    typedef typename Config::edge_iterator edge_iterator;
    typedef typename Config::directed_category directed_category;
    typedef typename Config::edge_parallel_category edge_parallel_category;
    typedef typename Config::vertices_size_type vertices_size_type;
    typedef typename Config::edges_size_type edges_size_type;
    typedef typename Config::degree_size_type degree_size_type;
    typedef typename Config::StoredEdge StoredEdge;
    typedef typename Config::vertex_property_type vertex_property_type;
    typedef typename Config::edge_property_type edge_property_type;
    typedef typename Config::graph_property_type graph_property_type;

    typedef typename Config::global_edgelist_selector global_edgelist_selector;

    typedef typename lookup_one_property< vertex_property_type,
        vertex_bundle_t >::type vertex_bundled;
    typedef
        typename lookup_one_property< edge_property_type, edge_bundle_t >::type
            edge_bundled;
    typedef typename lookup_one_property< graph_property_type,
        graph_bundle_t >::type graph_bundled;
};

template < class Config, class Base >
inline std::pair< typename Config::adjacency_iterator,
    typename Config::adjacency_iterator >
adjacent_vertices(typename Config::vertex_descriptor u,
    const adj_list_helper< Config, Base >& g_)
{
    typedef typename Config::graph_type AdjList;
    const AdjList& cg = static_cast< const AdjList& >(g_);
    AdjList& g = const_cast< AdjList& >(cg);
    typedef typename Config::adjacency_iterator adjacency_iterator;
    typename Config::out_edge_iterator first, last;
    boost::tie(first, last) = out_edges(u, g);
    return std::make_pair(
        adjacency_iterator(first, &g), adjacency_iterator(last, &g));
}
template < class Config, class Base >
inline std::pair< typename Config::inv_adjacency_iterator,
    typename Config::inv_adjacency_iterator >
inv_adjacent_vertices(typename Config::vertex_descriptor u,
    const adj_list_helper< Config, Base >& g_)
{
    typedef typename Config::graph_type AdjList;
    const AdjList& cg = static_cast< const AdjList& >(g_);
    AdjList& g = const_cast< AdjList& >(cg);
    typedef typename Config::inv_adjacency_iterator inv_adjacency_iterator;
    typename Config::in_edge_iterator first, last;
    boost::tie(first, last) = in_edges(u, g);
    return std::make_pair(
        inv_adjacency_iterator(first, &g), inv_adjacency_iterator(last, &g));
}
template < class Config, class Base >
inline std::pair< typename Config::out_edge_iterator,
    typename Config::out_edge_iterator >
out_edges(typename Config::vertex_descriptor u,
    const adj_list_helper< Config, Base >& g_)
{
    typedef typename Config::graph_type AdjList;
    typedef typename Config::out_edge_iterator out_edge_iterator;
    const AdjList& cg = static_cast< const AdjList& >(g_);
    AdjList& g = const_cast< AdjList& >(cg);
    return std::make_pair(out_edge_iterator(g.out_edge_list(u).begin(), u),
        out_edge_iterator(g.out_edge_list(u).end(), u));
}
template < class Config, class Base >
inline std::pair< typename Config::vertex_iterator,
    typename Config::vertex_iterator >
vertices(const adj_list_helper< Config, Base >& g_)
{
    typedef typename Config::graph_type AdjList;
    const AdjList& cg = static_cast< const AdjList& >(g_);
    AdjList& g = const_cast< AdjList& >(cg);
    return std::make_pair(g.vertex_set().begin(), g.vertex_set().end());
}
template < class Config, class Base >
inline typename Config::vertices_size_type num_vertices(
    const adj_list_helper< Config, Base >& g_)
{
    typedef typename Config::graph_type AdjList;
    const AdjList& g = static_cast< const AdjList& >(g_);
    return g.vertex_set().size();
}
template < class Config, class Base >
inline typename Config::degree_size_type out_degree(
    typename Config::vertex_descriptor u,
    const adj_list_helper< Config, Base >& g_)
{
    typedef typename Config::graph_type AdjList;
    const AdjList& g = static_cast< const AdjList& >(g_);
    return g.out_edge_list(u).size();
}
template < class Config, class Base >
inline std::pair< typename Config::edge_descriptor, bool > edge(
    typename Config::vertex_descriptor u, typename Config::vertex_descriptor v,
    const adj_list_helper< Config, Base >& g_)
{
    typedef typename Config::graph_type Graph;
    typedef typename Config::StoredEdge StoredEdge;
    const Graph& cg = static_cast< const Graph& >(g_);
    const typename Config::OutEdgeList& el = cg.out_edge_list(u);
    typename Config::OutEdgeList::const_iterator it
        = graph_detail::find(el, StoredEdge(v));
    return std::make_pair(typename Config::edge_descriptor(u, v,
                              (it == el.end() ? 0 : &(*it).get_property())),
        (it != el.end()));
}

template < class Config, class Base >
inline std::pair< typename Config::out_edge_iterator,
    typename Config::out_edge_iterator >
edge_range(typename Config::vertex_descriptor u,
    typename Config::vertex_descriptor v,
    const adj_list_helper< Config, Base >& g_)
{
    typedef typename Config::graph_type Graph;
    typedef typename Config::StoredEdge StoredEdge;
    const Graph& cg = static_cast< const Graph& >(g_);
    Graph& g = const_cast< Graph& >(cg);
    typedef typename Config::out_edge_iterator out_edge_iterator;
    typename Config::OutEdgeList& el = g.out_edge_list(u);
    typename Config::OutEdgeList::iterator first, last;
    typename Config::EdgeContainer fake_edge_container;
    boost::tie(first, last) = graph_detail::equal_range(el, StoredEdge(v));
    return std::make_pair(
        out_edge_iterator(first, u), out_edge_iterator(last, u));
}

template < class Config >
inline typename Config::degree_size_type in_degree(
    typename Config::vertex_descriptor u,
    const directed_edges_helper< Config >& g_)
{
    typedef typename Config::graph_type Graph;
    const Graph& cg = static_cast< const Graph& >(g_);
    Graph& g = const_cast< Graph& >(cg);
    return in_edge_list(g, u).size();
}

namespace detail
{
    template < class Config, class Base, class Property >
    inline typename boost::property_map< typename Config::graph_type,
        Property >::type
    get_dispatch(
        adj_list_helper< Config, Base >&, Property p, boost::edge_property_tag)
    {
        typedef typename Config::graph_type Graph;
        typedef typename boost::property_map< Graph, Property >::type PA;
        return PA(p);
    }
    template < class Config, class Base, class Property >
    inline typename boost::property_map< typename Config::graph_type,
        Property >::const_type
    get_dispatch(const adj_list_helper< Config, Base >&, Property p,
        boost::edge_property_tag)
    {
        typedef typename Config::graph_type Graph;
        typedef typename boost::property_map< Graph, Property >::const_type PA;
        return PA(p);
    }

    template < class Config, class Base, class Property >
    inline typename boost::property_map< typename Config::graph_type,
        Property >::type
    get_dispatch(adj_list_helper< Config, Base >& g, Property p,
        boost::vertex_property_tag)
    {
        typedef typename Config::graph_type Graph;
        typedef typename boost::property_map< Graph, Property >::type PA;
        return PA(&static_cast< Graph& >(g), p);
    }
    template < class Config, class Base, class Property >
    inline typename boost::property_map< typename Config::graph_type,
        Property >::const_type
    get_dispatch(const adj_list_helper< Config, Base >& g, Property p,
        boost::vertex_property_tag)
    {
        typedef typename Config::graph_type Graph;
        typedef typename boost::property_map< Graph, Property >::const_type PA;
        const Graph& cg = static_cast< const Graph& >(g);
        return PA(&cg, p);
    }

} // namespace detail

// Implementation of the PropertyGraph interface
template < class Config, class Base, class Property >
inline
    typename boost::property_map< typename Config::graph_type, Property >::type
    get(Property p, adj_list_helper< Config, Base >& g)
{
    typedef typename detail::property_kind_from_graph<
        adj_list_helper< Config, Base >, Property >::type Kind;
    return detail::get_dispatch(g, p, Kind());
}
template < class Config, class Base, class Property >
inline typename boost::property_map< typename Config::graph_type,
    Property >::const_type
get(Property p, const adj_list_helper< Config, Base >& g)
{
    typedef typename detail::property_kind_from_graph<
        adj_list_helper< Config, Base >, Property >::type Kind;
    return detail::get_dispatch(g, p, Kind());
}

template < class Config, class Base, class Property, class Key >
inline typename boost::property_traits< typename boost::property_map<
    typename Config::graph_type, Property >::type >::reference
get(Property p, adj_list_helper< Config, Base >& g, const Key& key)
{
    return get(get(p, g), key);
}

template < class Config, class Base, class Property, class Key >
inline typename boost::property_traits< typename boost::property_map<
    typename Config::graph_type, Property >::const_type >::reference
get(Property p, const adj_list_helper< Config, Base >& g, const Key& key)
{
    return get(get(p, g), key);
}

template < class Config, class Base, class Property, class Key, class Value >
inline void put(Property p, adj_list_helper< Config, Base >& g, const Key& key,
    const Value& value)
{
    typedef typename Config::graph_type Graph;
    typedef typename boost::property_map< Graph, Property >::type Map;
    Map pmap = get(p, static_cast< Graph& >(g));
    put(pmap, key, value);
}

//=========================================================================
// Generalize Adjacency List Implementation

struct adj_list_tag
{
};

template < class Derived, class Config, class Base >
class adj_list_impl : public adj_list_helper< Config, Base >
{
    typedef typename Config::OutEdgeList OutEdgeList;
    typedef typename Config::InEdgeList InEdgeList;
    typedef typename Config::StoredVertexList StoredVertexList;

public:
    typedef typename Config::stored_vertex stored_vertex;
    typedef typename Config::EdgeContainer EdgeContainer;
    typedef typename Config::vertex_descriptor vertex_descriptor;
    typedef typename Config::edge_descriptor edge_descriptor;
    typedef typename Config::vertex_iterator vertex_iterator;
    typedef typename Config::edge_iterator edge_iterator;
    typedef typename Config::edge_parallel_category edge_parallel_category;
    typedef typename Config::vertices_size_type vertices_size_type;
    typedef typename Config::edges_size_type edges_size_type;
    typedef typename Config::degree_size_type degree_size_type;
    typedef typename Config::edge_property_type edge_property_type;
    typedef adj_list_tag graph_tag;

    static vertex_descriptor null_vertex() { return 0; }

    inline adj_list_impl() {}

    inline adj_list_impl(const adj_list_impl& x) { copy_impl(x); }
    inline adj_list_impl& operator=(const adj_list_impl& x)
    {
        this->clear();
        copy_impl(x);
        return *this;
    }
    inline void clear()
    {
        for (typename StoredVertexList::iterator i = m_vertices.begin();
             i != m_vertices.end(); ++i)
            delete (stored_vertex*)*i;
        m_vertices.clear();
        m_edges.clear();
    }
    inline adj_list_impl(vertices_size_type num_vertices)
    {
        for (vertices_size_type i = 0; i < num_vertices; ++i)
            add_vertex(static_cast< Derived& >(*this));
    }
    template < class EdgeIterator >
    inline adj_list_impl(
        vertices_size_type num_vertices, EdgeIterator first, EdgeIterator last)
    {
        vertex_descriptor* v = new vertex_descriptor[num_vertices];
        for (vertices_size_type i = 0; i < num_vertices; ++i)
            v[i] = add_vertex(static_cast< Derived& >(*this));

        while (first != last)
        {
            add_edge(v[(*first).first], v[(*first).second], *this);
            ++first;
        }
        delete[] v;
    }
    template < class EdgeIterator, class EdgePropertyIterator >
    inline adj_list_impl(vertices_size_type num_vertices, EdgeIterator first,
        EdgeIterator last, EdgePropertyIterator ep_iter)
    {
        vertex_descriptor* v = new vertex_descriptor[num_vertices];
        for (vertices_size_type i = 0; i < num_vertices; ++i)
            v[i] = add_vertex(static_cast< Derived& >(*this));

        while (first != last)
        {
            add_edge(v[(*first).first], v[(*first).second], *ep_iter, *this);
            ++first;
            ++ep_iter;
        }
        delete[] v;
    }
    ~adj_list_impl()
    {
        for (typename StoredVertexList::iterator i = m_vertices.begin();
             i != m_vertices.end(); ++i)
            delete (stored_vertex*)*i;
    }
    //    protected:
    inline OutEdgeList& out_edge_list(vertex_descriptor v)
    {
        stored_vertex* sv = (stored_vertex*)v;
        return sv->m_out_edges;
    }
    inline const OutEdgeList& out_edge_list(vertex_descriptor v) const
    {
        stored_vertex* sv = (stored_vertex*)v;
        return sv->m_out_edges;
    }
    inline StoredVertexList& vertex_set() { return m_vertices; }
    inline const StoredVertexList& vertex_set() const { return m_vertices; }

    inline void copy_impl(const adj_list_impl& x_)
    {
        const Derived& x = static_cast< const Derived& >(x_);

        // Would be better to have a constant time way to get from
        // vertices in x to the corresponding vertices in *this.
        std::map< stored_vertex*, stored_vertex* > vertex_map;

        // Copy the stored vertex objects by adding each vertex
        // and copying its property object.
        vertex_iterator vi, vi_end;
        for (boost::tie(vi, vi_end) = vertices(x); vi != vi_end; ++vi)
        {
            stored_vertex* v = (stored_vertex*)add_vertex(*this);
            v->m_property = ((stored_vertex*)*vi)->m_property;
            vertex_map[(stored_vertex*)*vi] = v;
        }
        // Copy the edges by adding each edge and copying its
        // property object.
        edge_iterator ei, ei_end;
        for (boost::tie(ei, ei_end) = edges(x); ei != ei_end; ++ei)
        {
            edge_descriptor e;
            bool inserted;
            vertex_descriptor s = source(*ei, x), t = target(*ei, x);
            boost::tie(e, inserted) = add_edge(vertex_map[(stored_vertex*)s],
                vertex_map[(stored_vertex*)t], *this);
            *((edge_property_type*)e.m_eproperty)
                = *((edge_property_type*)(*ei).m_eproperty);
        }
    }

    typename Config::EdgeContainer m_edges;
    StoredVertexList m_vertices;
};

// O(1)
template < class Derived, class Config, class Base >
inline typename Config::vertex_descriptor add_vertex(
    adj_list_impl< Derived, Config, Base >& g_)
{
    Derived& g = static_cast< Derived& >(g_);
    typedef typename Config::stored_vertex stored_vertex;
    stored_vertex* v = new stored_vertex;
    typename Config::StoredVertexList::iterator pos;
    bool inserted;
    boost::tie(pos, inserted) = boost::graph_detail::push(g.m_vertices, v);
    v->m_position = pos;
    g.added_vertex(v);
    return v;
}
// O(1)
template < class Derived, class Config, class Base >
inline typename Config::vertex_descriptor add_vertex(
    const typename Config::vertex_property_type& p,
    adj_list_impl< Derived, Config, Base >& g_)
{
    typedef typename Config::vertex_descriptor vertex_descriptor;
    Derived& g = static_cast< Derived& >(g_);
    if (optional< vertex_descriptor > v
        = g.vertex_by_property(get_property_value(p, vertex_bundle)))
        return *v;

    typedef typename Config::stored_vertex stored_vertex;
    stored_vertex* v = new stored_vertex(p);
    typename Config::StoredVertexList::iterator pos;
    bool inserted;
    boost::tie(pos, inserted) = boost::graph_detail::push(g.m_vertices, v);
    v->m_position = pos;
    g.added_vertex(v);
    return v;
}
// O(1)
template < class Derived, class Config, class Base >
inline void remove_vertex(typename Config::vertex_descriptor u,
    adj_list_impl< Derived, Config, Base >& g_)
{
    typedef typename Config::stored_vertex stored_vertex;
    Derived& g = static_cast< Derived& >(g_);
    g.removing_vertex(
        u, boost::graph_detail::iterator_stability(g_.m_vertices));
    stored_vertex* su = (stored_vertex*)u;
    g.m_vertices.erase(su->m_position);
    delete su;
}
// O(V)
template < class Derived, class Config, class Base >
inline typename Config::vertex_descriptor vertex(
    typename Config::vertices_size_type n,
    const adj_list_impl< Derived, Config, Base >& g_)
{
    const Derived& g = static_cast< const Derived& >(g_);
    typename Config::vertex_iterator i = vertices(g).first;
    while (n--)
        ++i; // std::advance(i, n); (not VC++ portable)
    return *i;
}

//=========================================================================
// Vector-Backbone Adjacency List Implementation

namespace detail
{

    template < class Graph, class vertex_descriptor >
    inline void remove_vertex_dispatch(
        Graph& g, vertex_descriptor u, boost::directed_tag)
    {
        typedef typename Graph::edge_parallel_category edge_parallel_category;
        g.m_vertices.erase(g.m_vertices.begin() + u);
        vertex_descriptor V = num_vertices(g);
        if (u != V)
        {
            for (vertex_descriptor v = 0; v < V; ++v)
                reindex_edge_list(
                    g.out_edge_list(v), u, edge_parallel_category());
        }
    }

    template < class Graph, class vertex_descriptor >
    inline void remove_vertex_dispatch(
        Graph& g, vertex_descriptor u, boost::undirected_tag)
    {
        typedef typename Graph::global_edgelist_selector EdgeListS;
        BOOST_STATIC_ASSERT((!is_same< EdgeListS, vecS >::value));

        typedef typename Graph::edge_parallel_category edge_parallel_category;
        g.m_vertices.erase(g.m_vertices.begin() + u);
        vertex_descriptor V = num_vertices(g);
        for (vertex_descriptor v = 0; v < V; ++v)
            reindex_edge_list(g.out_edge_list(v), u, edge_parallel_category());
        typedef typename Graph::EdgeContainer Container;
        typedef typename Container::iterator Iter;
        Iter ei = g.m_edges.begin(), ei_end = g.m_edges.end();
        for (; ei != ei_end; ++ei)
        {
            if (ei->m_source > u)
                --ei->m_source;
            if (ei->m_target > u)
                --ei->m_target;
        }
    }
    template < class Graph, class vertex_descriptor >
    inline void remove_vertex_dispatch(
        Graph& g, vertex_descriptor u, boost::bidirectional_tag)
    {
        typedef typename Graph::global_edgelist_selector EdgeListS;
        BOOST_STATIC_ASSERT((!is_same< EdgeListS, vecS >::value));

        typedef typename Graph::edge_parallel_category edge_parallel_category;
        g.m_vertices.erase(g.m_vertices.begin() + u);
        vertex_descriptor V = num_vertices(g);
        vertex_descriptor v;
        if (u != V)
        {
            for (v = 0; v < V; ++v)
                reindex_edge_list(
                    g.out_edge_list(v), u, edge_parallel_category());
            for (v = 0; v < V; ++v)
                reindex_edge_list(
                    in_edge_list(g, v), u, edge_parallel_category());

            typedef typename Graph::EdgeContainer Container;
            typedef typename Container::iterator Iter;
            Iter ei = g.m_edges.begin(), ei_end = g.m_edges.end();
            for (; ei != ei_end; ++ei)
            {
                if (ei->m_source > u)
                    --ei->m_source;
                if (ei->m_target > u)
                    --ei->m_target;
            }
        }
    }

    template < class EdgeList, class vertex_descriptor >
    inline void reindex_edge_list(
        EdgeList& el, vertex_descriptor u, boost::allow_parallel_edge_tag)
    {
        typename EdgeList::iterator ei = el.begin(), e_end = el.end();
        for (; ei != e_end; ++ei)
            if ((*ei).get_target() > u)
                --(*ei).get_target();
    }

    template < class EdgeList, class vertex_descriptor >
    inline void reindex_edge_list(
        EdgeList& el, vertex_descriptor u, boost::disallow_parallel_edge_tag)
    {
        for (typename EdgeList::iterator ei = el.begin(); ei != el.end(); ++ei)
        {
            if (ei->get_target() > u)
            {
                typename EdgeList::value_type ce = *ei;
                el.erase(ce);
                --ce.get_target();
                el.insert(ce);
            }
        }
    }
} // namespace detail

struct vec_adj_list_tag
{
};

template < class Graph, class Config, class Base >
class vec_adj_list_impl : public adj_list_helper< Config, Base >
{
    typedef typename Config::OutEdgeList OutEdgeList;
    typedef typename Config::InEdgeList InEdgeList;
    typedef typename Config::StoredVertexList StoredVertexList;

public:
    typedef typename Config::vertex_descriptor vertex_descriptor;
    typedef typename Config::edge_descriptor edge_descriptor;
    typedef typename Config::out_edge_iterator out_edge_iterator;
    typedef typename Config::edge_iterator edge_iterator;
    typedef typename Config::directed_category directed_category;
    typedef typename Config::vertices_size_type vertices_size_type;
    typedef typename Config::edges_size_type edges_size_type;
    typedef typename Config::degree_size_type degree_size_type;
    typedef typename Config::StoredEdge StoredEdge;
    typedef typename Config::stored_vertex stored_vertex;
    typedef typename Config::EdgeContainer EdgeContainer;
    typedef typename Config::edge_property_type edge_property_type;
    typedef vec_adj_list_tag graph_tag;

    static vertex_descriptor null_vertex()
    {
        return (std::numeric_limits< vertex_descriptor >::max)();
    }

    inline vec_adj_list_impl() {}

    inline vec_adj_list_impl(const vec_adj_list_impl& x) { copy_impl(x); }
    inline vec_adj_list_impl& operator=(const vec_adj_list_impl& x)
    {
        this->clear();
        copy_impl(x);
        return *this;
    }
    inline void clear()
    {
        m_vertices.clear();
        m_edges.clear();
    }

    inline vec_adj_list_impl(vertices_size_type _num_vertices)
    : m_vertices(_num_vertices)
    {
    }

    template < class EdgeIterator >
    inline vec_adj_list_impl(
        vertices_size_type num_vertices, EdgeIterator first, EdgeIterator last)
    : m_vertices(num_vertices)
    {
        while (first != last)
        {
            add_edge(
                (*first).first, (*first).second, static_cast< Graph& >(*this));
            ++first;
        }
    }
    template < class EdgeIterator, class EdgePropertyIterator >
    inline vec_adj_list_impl(vertices_size_type num_vertices,
        EdgeIterator first, EdgeIterator last, EdgePropertyIterator ep_iter)
    : m_vertices(num_vertices)
    {
        while (first != last)
        {
            add_edge((*first).first, (*first).second, *ep_iter,
                static_cast< Graph& >(*this));
            ++first;
            ++ep_iter;
        }
    }

    //    protected:
    inline boost::integer_range< vertex_descriptor > vertex_set() const
    {
        return boost::integer_range< vertex_descriptor >(0, m_vertices.size());
    }
    inline OutEdgeList& out_edge_list(vertex_descriptor v)
    {
        return m_vertices[v].m_out_edges;
    }
    inline const OutEdgeList& out_edge_list(vertex_descriptor v) const
    {
        return m_vertices[v].m_out_edges;
    }
    inline void copy_impl(const vec_adj_list_impl& x_)
    {
        const Graph& x = static_cast< const Graph& >(x_);
        // Copy the stored vertex objects by adding each vertex
        // and copying its property object.
        for (vertices_size_type i = 0; i < num_vertices(x); ++i)
        {
            vertex_descriptor v = add_vertex(*this);
            m_vertices[v].m_property = x.m_vertices[i].m_property;
        }
        // Copy the edges by adding each edge and copying its
        // property object.
        edge_iterator ei, ei_end;
        for (boost::tie(ei, ei_end) = edges(x); ei != ei_end; ++ei)
        {
            edge_descriptor e;
            bool inserted;
            boost::tie(e, inserted)
                = add_edge(source(*ei, x), target(*ei, x), *this);
            *((edge_property_type*)e.m_eproperty)
                = *((edge_property_type*)(*ei).m_eproperty);
        }
    }
    typename Config::EdgeContainer m_edges;
    StoredVertexList m_vertices;
};
// Had to make these non-members to avoid accidental instantiation
// on SGI MIPSpro C++
template < class G, class C, class B >
inline typename C::InEdgeList& in_edge_list(
    vec_adj_list_impl< G, C, B >& g, typename C::vertex_descriptor v)
{
    return g.m_vertices[v].m_in_edges;
}
template < class G, class C, class B >
inline const typename C::InEdgeList& in_edge_list(
    const vec_adj_list_impl< G, C, B >& g, typename C::vertex_descriptor v)
{
    return g.m_vertices[v].m_in_edges;
}

// O(1)
template < class Graph, class Config, class Base >
inline typename Config::vertex_descriptor add_vertex(
    vec_adj_list_impl< Graph, Config, Base >& g_)
{
    Graph& g = static_cast< Graph& >(g_);
    g.m_vertices.resize(g.m_vertices.size() + 1);
    g.added_vertex(g.m_vertices.size() - 1);
    return g.m_vertices.size() - 1;
}

template < class Graph, class Config, class Base >
inline typename Config::vertex_descriptor add_vertex(
    const typename Config::vertex_property_type& p,
    vec_adj_list_impl< Graph, Config, Base >& g_)
{
    typedef typename Config::vertex_descriptor vertex_descriptor;
    Graph& g = static_cast< Graph& >(g_);
    if (optional< vertex_descriptor > v
        = g.vertex_by_property(get_property_value(p, vertex_bundle)))
        return *v;
    typedef typename Config::stored_vertex stored_vertex;
    g.m_vertices.push_back(stored_vertex(p));
    g.added_vertex(g.m_vertices.size() - 1);
    return g.m_vertices.size() - 1;
}

// Here we override the directed_graph_helper add_edge() function
// so that the number of vertices is automatically changed if
// either u or v is greater than the number of vertices.
template < class Graph, class Config, class Base >
inline std::pair< typename Config::edge_descriptor, bool > add_edge(
    typename Config::vertex_descriptor u, typename Config::vertex_descriptor v,
    const typename Config::edge_property_type& p,
    vec_adj_list_impl< Graph, Config, Base >& g_)
{
    BOOST_USING_STD_MAX();
    typename Config::vertex_descriptor x
        = max BOOST_PREVENT_MACRO_SUBSTITUTION(u, v);
    if (x >= num_vertices(g_))
        g_.m_vertices.resize(x + 1);
    adj_list_helper< Config, Base >& g = g_;
    return add_edge(u, v, p, g);
}
template < class Graph, class Config, class Base >
inline std::pair< typename Config::edge_descriptor, bool > add_edge(
    typename Config::vertex_descriptor u, typename Config::vertex_descriptor v,
    vec_adj_list_impl< Graph, Config, Base >& g_)
{
    typename Config::edge_property_type p;
    return add_edge(u, v, p, g_);
}

// O(V + E)
template < class Graph, class Config, class Base >
inline void remove_vertex(typename Config::vertex_descriptor v,
    vec_adj_list_impl< Graph, Config, Base >& g_)
{
    typedef typename Config::directed_category Cat;
    Graph& g = static_cast< Graph& >(g_);
    g.removing_vertex(
        v, boost::graph_detail::iterator_stability(g_.m_vertices));
    detail::remove_vertex_dispatch(g, v, Cat());
}
// O(1)
template < class Graph, class Config, class Base >
inline typename Config::vertex_descriptor vertex(
    typename Config::vertices_size_type n,
    const vec_adj_list_impl< Graph, Config, Base >&)
{
    return n;
}

namespace detail
{

    //=========================================================================
    // Adjacency List Generator

    template < class Graph, class VertexListS, class OutEdgeListS,
        class DirectedS, class VertexProperty, class EdgeProperty,
        class GraphProperty, class EdgeListS >
    struct adj_list_gen
    {
        typedef typename detail::is_random_access< VertexListS >::type
            is_rand_access;
        typedef typename has_property< EdgeProperty >::type has_edge_property;
        typedef typename DirectedS::is_directed_t DirectedT;
        typedef typename DirectedS::is_bidir_t BidirectionalT;

        struct config
        {
            typedef OutEdgeListS edgelist_selector;
            typedef EdgeListS global_edgelist_selector;

            typedef Graph graph_type;
            typedef EdgeProperty edge_property_type;
            typedef VertexProperty vertex_property_type;
            typedef GraphProperty graph_property_type;
            typedef std::size_t vertices_size_type;

            typedef adjacency_list_traits< OutEdgeListS, VertexListS,
                DirectedS >
                Traits;

            typedef typename Traits::directed_category directed_category;
            typedef
                typename Traits::edge_parallel_category edge_parallel_category;
            typedef typename Traits::vertex_descriptor vertex_descriptor;
            typedef typename Traits::edge_descriptor edge_descriptor;

            typedef void* vertex_ptr;

            // need to reorganize this to avoid instantiating stuff
            // that doesn't get used -JGS

            // VertexList and vertex_iterator
            typedef typename container_gen< VertexListS, vertex_ptr >::type
                SeqVertexList;
            typedef boost::integer_range< vertex_descriptor > RandVertexList;
            typedef typename mpl::if_< is_rand_access, RandVertexList,
                SeqVertexList >::type VertexList;

            typedef typename VertexList::iterator vertex_iterator;

            // EdgeContainer and StoredEdge

            typedef typename container_gen< EdgeListS,
                list_edge< vertex_descriptor, EdgeProperty > >::type
                EdgeContainer;

            typedef typename mpl::and_< DirectedT,
                typename mpl::not_< BidirectionalT >::type >::type
                on_edge_storage;

            typedef typename mpl::if_< on_edge_storage, std::size_t,
                typename EdgeContainer::size_type >::type edges_size_type;

            typedef typename EdgeContainer::iterator EdgeIter;

            typedef
                typename detail::is_random_access< EdgeListS >::type is_edge_ra;

            typedef typename mpl::if_< on_edge_storage,
                stored_edge_property< vertex_descriptor, EdgeProperty >,
                typename mpl::if_< is_edge_ra,
                    stored_ra_edge_iter< vertex_descriptor, EdgeContainer,
                        EdgeProperty >,
                    stored_edge_iter< vertex_descriptor, EdgeIter,
                        EdgeProperty > >::type >::type StoredEdge;

            // Adjacency Types

            typedef typename container_gen< OutEdgeListS, StoredEdge >::type
                OutEdgeList;
            typedef typename OutEdgeList::size_type degree_size_type;
            typedef typename OutEdgeList::iterator OutEdgeIter;

            typedef std::iterator_traits< OutEdgeIter >
                OutEdgeIterTraits;
            typedef
                typename OutEdgeIterTraits::iterator_category OutEdgeIterCat;
            typedef typename OutEdgeIterTraits::difference_type OutEdgeIterDiff;

            typedef out_edge_iter< OutEdgeIter, vertex_descriptor,
                edge_descriptor, OutEdgeIterDiff >
                out_edge_iterator;

            typedef typename adjacency_iterator_generator< graph_type,
                vertex_descriptor, out_edge_iterator >::type adjacency_iterator;

            typedef OutEdgeList InEdgeList;
            typedef OutEdgeIter InEdgeIter;
            typedef OutEdgeIterCat InEdgeIterCat;
            typedef OutEdgeIterDiff InEdgeIterDiff;

            typedef in_edge_iter< InEdgeIter, vertex_descriptor,
                edge_descriptor, InEdgeIterDiff >
                in_edge_iterator;

            typedef typename inv_adjacency_iterator_generator< graph_type,
                vertex_descriptor, in_edge_iterator >::type
                inv_adjacency_iterator;

            // Edge Iterator

            typedef std::iterator_traits< EdgeIter > EdgeIterTraits;
            typedef typename EdgeIterTraits::iterator_category EdgeIterCat;
            typedef typename EdgeIterTraits::difference_type EdgeIterDiff;

            typedef undirected_edge_iter< EdgeIter, edge_descriptor,
                EdgeIterDiff >
                UndirectedEdgeIter; // also used for bidirectional

            typedef adj_list_edge_iterator< vertex_iterator, out_edge_iterator,
                graph_type >
                DirectedEdgeIter;

            typedef typename mpl::if_< on_edge_storage, DirectedEdgeIter,
                UndirectedEdgeIter >::type edge_iterator;

            // stored_vertex and StoredVertexList
            typedef typename container_gen< VertexListS, vertex_ptr >::type
                SeqStoredVertexList;
            struct seq_stored_vertex
            {
                seq_stored_vertex() {}
                seq_stored_vertex(const VertexProperty& p) : m_property(p) {}
                OutEdgeList m_out_edges;
                VertexProperty m_property;
                typename SeqStoredVertexList::iterator m_position;
            };
            struct bidir_seq_stored_vertex
            {
                bidir_seq_stored_vertex() {}
                bidir_seq_stored_vertex(const VertexProperty& p) : m_property(p)
                {
                }
                OutEdgeList m_out_edges;
                InEdgeList m_in_edges;
                VertexProperty m_property;
                typename SeqStoredVertexList::iterator m_position;
            };
            struct rand_stored_vertex
            {
                rand_stored_vertex() {}
                rand_stored_vertex(const VertexProperty& p) : m_property(p) {}
                OutEdgeList m_out_edges;
                VertexProperty m_property;
            };
            struct bidir_rand_stored_vertex
            {
                bidir_rand_stored_vertex() {}
                bidir_rand_stored_vertex(const VertexProperty& p)
                : m_property(p)
                {
                }
                OutEdgeList m_out_edges;
                InEdgeList m_in_edges;
                VertexProperty m_property;
            };
            typedef typename mpl::if_< is_rand_access,
                typename mpl::if_< BidirectionalT, bidir_rand_stored_vertex,
                    rand_stored_vertex >::type,
                typename mpl::if_< BidirectionalT, bidir_seq_stored_vertex,
                    seq_stored_vertex >::type >::type StoredVertex;
            struct stored_vertex : public StoredVertex
            {
                stored_vertex() {}
                stored_vertex(const VertexProperty& p) : StoredVertex(p) {}
            };

            typedef typename container_gen< VertexListS, stored_vertex >::type
                RandStoredVertexList;
            typedef typename mpl::if_< is_rand_access, RandStoredVertexList,
                SeqStoredVertexList >::type StoredVertexList;
        }; // end of config

        typedef typename mpl::if_< BidirectionalT,
            bidirectional_graph_helper_with_property< config >,
            typename mpl::if_< DirectedT, directed_graph_helper< config >,
                undirected_graph_helper< config > >::type >::type
            DirectedHelper;

        typedef typename mpl::if_< is_rand_access,
            vec_adj_list_impl< Graph, config, DirectedHelper >,
            adj_list_impl< Graph, config, DirectedHelper > >::type type;
    };

} // namespace detail

//=========================================================================
// Vertex Property Maps

template < class Graph, class ValueType, class Reference, class Tag >
struct adj_list_vertex_property_map
: public boost::put_get_helper< Reference,
      adj_list_vertex_property_map< Graph, ValueType, Reference, Tag > >
{
    typedef typename Graph::stored_vertex StoredVertex;
    typedef ValueType value_type;
    typedef Reference reference;
    typedef typename Graph::vertex_descriptor key_type;
    typedef boost::lvalue_property_map_tag category;
    inline adj_list_vertex_property_map(const Graph* = 0, Tag tag = Tag())
    : m_tag(tag)
    {
    }
    inline Reference operator[](key_type v) const
    {
        StoredVertex* sv = (StoredVertex*)v;
        return get_property_value(sv->m_property, m_tag);
    }
    inline Reference operator()(key_type v) const
    {
        return this->operator[](v);
    }
    Tag m_tag;
};

template < class Graph, class Property, class PropRef >
struct adj_list_vertex_all_properties_map
: public boost::put_get_helper< PropRef,
      adj_list_vertex_all_properties_map< Graph, Property, PropRef > >
{
    typedef typename Graph::stored_vertex StoredVertex;
    typedef Property value_type;
    typedef PropRef reference;
    typedef typename Graph::vertex_descriptor key_type;
    typedef boost::lvalue_property_map_tag category;
    inline adj_list_vertex_all_properties_map(
        const Graph* = 0, vertex_all_t = vertex_all_t())
    {
    }
    inline PropRef operator[](key_type v) const
    {
        StoredVertex* sv = (StoredVertex*)v;
        return sv->m_property;
    }
    inline PropRef operator()(key_type v) const { return this->operator[](v); }
};

template < class Graph, class GraphPtr, class ValueType, class Reference,
    class Tag >
struct vec_adj_list_vertex_property_map
: public boost::put_get_helper< Reference,
      vec_adj_list_vertex_property_map< Graph, GraphPtr, ValueType, Reference,
          Tag > >
{
    typedef ValueType value_type;
    typedef Reference reference;
    typedef typename boost::graph_traits< Graph >::vertex_descriptor key_type;
    typedef boost::lvalue_property_map_tag category;
    vec_adj_list_vertex_property_map(GraphPtr g = 0, Tag tag = Tag())
    : m_g(g), m_tag(tag)
    {
    }
    inline Reference operator[](key_type v) const
    {
        return get_property_value(m_g->m_vertices[v].m_property, m_tag);
    }
    inline Reference operator()(key_type v) const
    {
        return this->operator[](v);
    }
    GraphPtr m_g;
    Tag m_tag;
};

template < class Graph, class GraphPtr, class Property, class PropertyRef >
struct vec_adj_list_vertex_all_properties_map
: public boost::put_get_helper< PropertyRef,
      vec_adj_list_vertex_all_properties_map< Graph, GraphPtr, Property,
          PropertyRef > >
{
    typedef Property value_type;
    typedef PropertyRef reference;
    typedef typename boost::graph_traits< Graph >::vertex_descriptor key_type;
    typedef boost::lvalue_property_map_tag category;
    vec_adj_list_vertex_all_properties_map(
        GraphPtr g = 0, vertex_all_t = vertex_all_t())
    : m_g(g)
    {
    }
    inline PropertyRef operator[](key_type v) const
    {
        return m_g->m_vertices[v].m_property;
    }
    inline PropertyRef operator()(key_type v) const
    {
        return this->operator[](v);
    }
    GraphPtr m_g;
};

struct adj_list_any_vertex_pa
{
    template < class Tag, class Graph, class Property > struct bind_
    {
        typedef typename property_value< Property, Tag >::type value_type;
        typedef value_type& reference;
        typedef const value_type& const_reference;

        typedef adj_list_vertex_property_map< Graph, value_type, reference,
            Tag >
            type;
        typedef adj_list_vertex_property_map< Graph, value_type,
            const_reference, Tag >
            const_type;
    };
};
struct adj_list_all_vertex_pa
{
    template < class Tag, class Graph, class Property > struct bind_
    {
        typedef typename Graph::vertex_descriptor Vertex;
        typedef adj_list_vertex_all_properties_map< Graph, Property, Property& >
            type;
        typedef adj_list_vertex_all_properties_map< Graph, Property,
            const Property& >
            const_type;
    };
};

template < class Property, class Vertex >
struct vec_adj_list_vertex_id_map
: public boost::put_get_helper< Vertex,
      vec_adj_list_vertex_id_map< Property, Vertex > >
{
    typedef Vertex value_type;
    typedef Vertex key_type;
    typedef Vertex reference;
    typedef boost::readable_property_map_tag category;
    inline vec_adj_list_vertex_id_map() {}
    template < class Graph >
    inline vec_adj_list_vertex_id_map(const Graph&, vertex_index_t)
    {
    }
    inline value_type operator[](key_type v) const { return v; }
    inline value_type operator()(key_type v) const { return v; }
};

struct vec_adj_list_any_vertex_pa
{
    template < class Tag, class Graph, class Property > struct bind_
    {
        typedef typename property_value< Property, Tag >::type value_type;
        typedef value_type& reference;
        typedef const value_type& const_reference;

        typedef vec_adj_list_vertex_property_map< Graph, Graph*, value_type,
            reference, Tag >
            type;
        typedef vec_adj_list_vertex_property_map< Graph, const Graph*,
            value_type, const_reference, Tag >
            const_type;
    };
};
struct vec_adj_list_id_vertex_pa
{
    template < class Tag, class Graph, class Property > struct bind_
    {
        typedef typename Graph::vertex_descriptor Vertex;
        typedef vec_adj_list_vertex_id_map< Property, Vertex > type;
        typedef vec_adj_list_vertex_id_map< Property, Vertex > const_type;
    };
};
struct vec_adj_list_all_vertex_pa
{
    template < class Tag, class Graph, class Property > struct bind_
    {
        typedef typename Graph::vertex_descriptor Vertex;
        typedef vec_adj_list_vertex_all_properties_map< Graph, Graph*, Property,
            Property& >
            type;
        typedef vec_adj_list_vertex_all_properties_map< Graph, const Graph*,
            Property, const Property& >
            const_type;
    };
};
namespace detail
{
    template < class Tag, class Graph, class Property >
    struct adj_list_choose_vertex_pa
    : boost::mpl::if_< boost::is_same< Tag, vertex_all_t >,
          adj_list_all_vertex_pa, adj_list_any_vertex_pa >::type ::
          template bind_< Tag, Graph, Property >
    {
    };

    template < class Tag > struct vec_adj_list_choose_vertex_pa_helper
    {
        typedef vec_adj_list_any_vertex_pa type;
    };
    template <> struct vec_adj_list_choose_vertex_pa_helper< vertex_index_t >
    {
        typedef vec_adj_list_id_vertex_pa type;
    };
    template <> struct vec_adj_list_choose_vertex_pa_helper< vertex_all_t >
    {
        typedef vec_adj_list_all_vertex_pa type;
    };
    template < class Tag, class Graph, class Property >
    struct vec_adj_list_choose_vertex_pa
    : vec_adj_list_choose_vertex_pa_helper< Tag >::type::template bind_< Tag,
          Graph, Property >
    {
    };
} // namespace detail

//=========================================================================
// Edge Property Map

template < class Directed, class Value, class Ref, class Vertex, class Property,
    class Tag >
struct adj_list_edge_property_map
: public put_get_helper< Ref,
      adj_list_edge_property_map< Directed, Value, Ref, Vertex, Property,
          Tag > >
{
    Tag tag;
    explicit adj_list_edge_property_map(Tag tag = Tag()) : tag(tag) {}

    typedef Value value_type;
    typedef Ref reference;
    typedef detail::edge_desc_impl< Directed, Vertex > key_type;
    typedef boost::lvalue_property_map_tag category;
    inline Ref operator[](key_type e) const
    {
        Property& p = *(Property*)e.get_property();
        return get_property_value(p, tag);
    }
    inline Ref operator()(key_type e) const { return this->operator[](e); }
};

template < class Directed, class Property, class PropRef, class PropPtr,
    class Vertex >
struct adj_list_edge_all_properties_map
: public put_get_helper< PropRef,
      adj_list_edge_all_properties_map< Directed, Property, PropRef, PropPtr,
          Vertex > >
{
    explicit adj_list_edge_all_properties_map(edge_all_t = edge_all_t()) {}
    typedef Property value_type;
    typedef PropRef reference;
    typedef detail::edge_desc_impl< Directed, Vertex > key_type;
    typedef boost::lvalue_property_map_tag category;
    inline PropRef operator[](key_type e) const
    {
        return *(PropPtr)e.get_property();
    }
    inline PropRef operator()(key_type e) const { return this->operator[](e); }
};

// Edge Property Maps

namespace detail
{
    struct adj_list_any_edge_pmap
    {
        template < class Graph, class Property, class Tag > struct bind_
        {
            typedef typename property_value< Property, Tag >::type value_type;
            typedef value_type& reference;
            typedef const value_type& const_reference;

            typedef adj_list_edge_property_map<
                typename Graph::directed_category, value_type, reference,
                typename Graph::vertex_descriptor, Property, Tag >
                type;
            typedef adj_list_edge_property_map<
                typename Graph::directed_category, value_type, const_reference,
                typename Graph::vertex_descriptor, const Property, Tag >
                const_type;
        };
    };
    struct adj_list_all_edge_pmap
    {
        template < class Graph, class Property, class Tag > struct bind_
        {
            typedef adj_list_edge_all_properties_map<
                typename Graph::directed_category, Property, Property&,
                Property*, typename Graph::vertex_descriptor >
                type;
            typedef adj_list_edge_all_properties_map<
                typename Graph::directed_category, Property, const Property&,
                const Property*, typename Graph::vertex_descriptor >
                const_type;
        };
    };

    template < class Tag > struct adj_list_choose_edge_pmap_helper
    {
        typedef adj_list_any_edge_pmap type;
    };
    template <> struct adj_list_choose_edge_pmap_helper< edge_all_t >
    {
        typedef adj_list_all_edge_pmap type;
    };
    template < class Tag, class Graph, class Property >
    struct adj_list_choose_edge_pmap
    : adj_list_choose_edge_pmap_helper< Tag >::type::template bind_< Graph,
          Property, Tag >
    {
    };
    struct adj_list_edge_property_selector
    {
        template < class Graph, class Property, class Tag >
        struct bind_ : adj_list_choose_edge_pmap< Tag, Graph, Property >
        {
        };
    };
} // namespace detail

template <> struct edge_property_selector< adj_list_tag >
{
    typedef detail::adj_list_edge_property_selector type;
};
template <> struct edge_property_selector< vec_adj_list_tag >
{
    typedef detail::adj_list_edge_property_selector type;
};

// Vertex Property Maps

struct adj_list_vertex_property_selector
{
    template < class Graph, class Property, class Tag >
    struct bind_ : detail::adj_list_choose_vertex_pa< Tag, Graph, Property >
    {
    };
};
template <> struct vertex_property_selector< adj_list_tag >
{
    typedef adj_list_vertex_property_selector type;
};

struct vec_adj_list_vertex_property_selector
{
    template < class Graph, class Property, class Tag >
    struct bind_ : detail::vec_adj_list_choose_vertex_pa< Tag, Graph, Property >
    {
    };
};
template <> struct vertex_property_selector< vec_adj_list_tag >
{
    typedef vec_adj_list_vertex_property_selector type;
};

} // namespace boost

namespace boost
{

template < typename V > struct hash< boost::detail::stored_edge< V > >
{
    std::size_t operator()(const boost::detail::stored_edge< V >& e) const
    {
        return hash< V >()(e.m_target);
    }
};

template < typename V, typename P >
struct hash< boost::detail::stored_edge_property< V, P > >
{
    std::size_t operator()(
        const boost::detail::stored_edge_property< V, P >& e) const
    {
        return hash< V >()(e.m_target);
    }
};

template < typename V, typename I, typename P >
struct hash< boost::detail::stored_edge_iter< V, I, P > >
{
    std::size_t operator()(
        const boost::detail::stored_edge_iter< V, I, P >& e) const
    {
        return hash< V >()(e.m_target);
    }
};

}

#endif // BOOST_GRAPH_DETAIL_DETAIL_ADJACENCY_LIST_CCT

/*
  Implementation Notes:

  Many of the public interface functions in this file would have been
  more conveniently implemented as inline friend functions.
  However there are a few compiler bugs that make that approach
  non-portable.

  1. g++ inline friend in namespace bug
  2. g++ using clause doesn't work with inline friends
  3. VC++ doesn't have Koenig lookup

  For these reasons, the functions were all written as non-inline free
  functions, and static cast was used to convert from the helper
  class to the adjacency_list derived class.

  Looking back, it might have been better to write out all functions
  in terms of the adjacency_list, and then use a tag to dispatch
  to the various helpers instead of using inheritance.

 */
