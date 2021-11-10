//=======================================================================
// Copyright 1997, 1998, 1999, 2000 University of Notre Dame.
// Authors: Andrew Lumsdaine, Lie-Quan Lee, Jeremy G. Siek
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================
//
// Revision History:
//   01 April 2001: Modified to use new <boost/limits.hpp> header. (JMaddock)
//
#ifndef BOOST_GRAPH_GRAPH_SEARCH_VISITORS_HPP
#define BOOST_GRAPH_GRAPH_SEARCH_VISITORS_HPP

#include <iosfwd>
#include <boost/config.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/limits.hpp>

namespace boost
{

// This is a bit more convenient than std::numeric_limits because
// you don't have to explicitly provide type T.
template < class T > inline T numeric_limits_max(T)
{
    return (std::numeric_limits< T >::max)();
}

//========================================================================
// Event Tags

namespace detail
{
    // For partial specialization workaround
    enum event_visitor_enum
    {
        on_no_event_num,
        on_initialize_vertex_num,
        on_start_vertex_num,
        on_discover_vertex_num,
        on_finish_vertex_num,
        on_examine_vertex_num,
        on_examine_edge_num,
        on_tree_edge_num,
        on_non_tree_edge_num,
        on_gray_target_num,
        on_black_target_num,
        on_forward_or_cross_edge_num,
        on_back_edge_num,
        on_finish_edge_num,
        on_edge_relaxed_num,
        on_edge_not_relaxed_num,
        on_edge_minimized_num,
        on_edge_not_minimized_num
    };

    template < typename Event, typename Visitor >
    struct functor_to_visitor : Visitor
    {
        typedef Event event_filter;
        functor_to_visitor(const Visitor& visitor) : Visitor(visitor) {}
    };

} // namespace detail

struct on_no_event
{
    enum
    {
        num = detail::on_no_event_num
    };
};

struct on_initialize_vertex
{
    enum
    {
        num = detail::on_initialize_vertex_num
    };
};
struct on_start_vertex
{
    enum
    {
        num = detail::on_start_vertex_num
    };
};
struct on_discover_vertex
{
    enum
    {
        num = detail::on_discover_vertex_num
    };
};
struct on_examine_vertex
{
    enum
    {
        num = detail::on_examine_vertex_num
    };
};
struct on_finish_vertex
{
    enum
    {
        num = detail::on_finish_vertex_num
    };
};

struct on_examine_edge
{
    enum
    {
        num = detail::on_examine_edge_num
    };
};
struct on_tree_edge
{
    enum
    {
        num = detail::on_tree_edge_num
    };
};
struct on_non_tree_edge
{
    enum
    {
        num = detail::on_non_tree_edge_num
    };
};
struct on_gray_target
{
    enum
    {
        num = detail::on_gray_target_num
    };
};
struct on_black_target
{
    enum
    {
        num = detail::on_black_target_num
    };
};
struct on_forward_or_cross_edge
{
    enum
    {
        num = detail::on_forward_or_cross_edge_num
    };
};
struct on_back_edge
{
    enum
    {
        num = detail::on_back_edge_num
    };
};
struct on_finish_edge
{
    enum
    {
        num = detail::on_finish_edge_num
    };
};

struct on_edge_relaxed
{
    enum
    {
        num = detail::on_edge_relaxed_num
    };
};
struct on_edge_not_relaxed
{
    enum
    {
        num = detail::on_edge_not_relaxed_num
    };
};
struct on_edge_minimized
{
    enum
    {
        num = detail::on_edge_minimized_num
    };
};
struct on_edge_not_minimized
{
    enum
    {
        num = detail::on_edge_not_minimized_num
    };
};

//========================================================================
// base_visitor and null_visitor

// needed for MSVC workaround
template < class Visitor > struct base_visitor
{
    typedef on_no_event event_filter;
    template < class T, class Graph > void operator()(T, Graph&) {}
};

struct null_visitor : public base_visitor< null_visitor >
{
    typedef on_no_event event_filter;
    template < class T, class Graph > void operator()(T, Graph&) {}
};

//========================================================================
// The invoke_visitors() function

namespace detail
{
    template < class Visitor, class T, class Graph >
    inline void invoke_dispatch(Visitor& v, T x, Graph& g, mpl::true_)
    {
        v(x, g);
    }

    template < class Visitor, class T, class Graph >
    inline void invoke_dispatch(Visitor&, T, Graph&, mpl::false_)
    {
    }
} // namespace detail

template < class Visitor, class Rest, class T, class Graph, class Tag >
inline void invoke_visitors(
    std::pair< Visitor, Rest >& vlist, T x, Graph& g, Tag tag)
{
    typedef typename Visitor::event_filter Category;
    typedef typename is_same< Category, Tag >::type IsSameTag;
    detail::invoke_dispatch(vlist.first, x, g, IsSameTag());
    invoke_visitors(vlist.second, x, g, tag);
}
template < class Visitor, class T, class Graph, class Tag >
inline void invoke_visitors(Visitor& v, T x, Graph& g, Tag)
{
    typedef typename Visitor::event_filter Category;
    typedef typename is_same< Category, Tag >::type IsSameTag;
    detail::invoke_dispatch(v, x, g, IsSameTag());
}

//========================================================================
// predecessor_recorder

template < class PredecessorMap, class Tag >
struct predecessor_recorder
: public base_visitor< predecessor_recorder< PredecessorMap, Tag > >
{
    typedef Tag event_filter;
    predecessor_recorder(PredecessorMap pa) : m_predecessor(pa) {}
    template < class Edge, class Graph > void operator()(Edge e, const Graph& g)
    {
        put(m_predecessor, target(e, g), source(e, g));
    }
    PredecessorMap m_predecessor;
};
template < class PredecessorMap, class Tag >
predecessor_recorder< PredecessorMap, Tag > record_predecessors(
    PredecessorMap pa, Tag)
{
    return predecessor_recorder< PredecessorMap, Tag >(pa);
}

//========================================================================
// edge_predecessor_recorder

template < class PredEdgeMap, class Tag >
struct edge_predecessor_recorder
: public base_visitor< edge_predecessor_recorder< PredEdgeMap, Tag > >
{
    typedef Tag event_filter;
    edge_predecessor_recorder(PredEdgeMap pa) : m_predecessor(pa) {}
    template < class Edge, class Graph > void operator()(Edge e, const Graph& g)
    {
        put(m_predecessor, target(e, g), e);
    }
    PredEdgeMap m_predecessor;
};
template < class PredEdgeMap, class Tag >
edge_predecessor_recorder< PredEdgeMap, Tag > record_edge_predecessors(
    PredEdgeMap pa, Tag)
{
    return edge_predecessor_recorder< PredEdgeMap, Tag >(pa);
}

//========================================================================
// distance_recorder

template < class DistanceMap, class Tag >
struct distance_recorder
: public base_visitor< distance_recorder< DistanceMap, Tag > >
{
    typedef Tag event_filter;
    distance_recorder(DistanceMap pa) : m_distance(pa) {}
    template < class Edge, class Graph > void operator()(Edge e, const Graph& g)
    {
        typename graph_traits< Graph >::vertex_descriptor u = source(e, g),
                                                          v = target(e, g);
        put(m_distance, v, get(m_distance, u) + 1);
    }
    DistanceMap m_distance;
};
template < class DistanceMap, class Tag >
distance_recorder< DistanceMap, Tag > record_distances(DistanceMap pa, Tag)
{
    return distance_recorder< DistanceMap, Tag >(pa);
}

//========================================================================
// time_stamper

template < class TimeMap, class TimeT, class Tag >
struct time_stamper : public base_visitor< time_stamper< TimeMap, TimeT, Tag > >
{
    typedef Tag event_filter;
    time_stamper(TimeMap pa, TimeT& t) : m_time_pa(pa), m_time(t) {}
    template < class Vertex, class Graph >
    void operator()(Vertex u, const Graph&)
    {
        put(m_time_pa, u, ++m_time);
    }
    TimeMap m_time_pa;
    TimeT& m_time;
};
template < class TimeMap, class TimeT, class Tag >
time_stamper< TimeMap, TimeT, Tag > stamp_times(
    TimeMap pa, TimeT& time_counter, Tag)
{
    return time_stamper< TimeMap, TimeT, Tag >(pa, time_counter);
}

//========================================================================
// property_writer

template < class PA, class OutputIterator, class Tag >
struct property_writer
: public base_visitor< property_writer< PA, OutputIterator, Tag > >
{
    typedef Tag event_filter;

    property_writer(PA pa, OutputIterator out) : m_pa(pa), m_out(out) {}

    template < class T, class Graph > void operator()(T x, Graph&)
    {
        *m_out++ = get(m_pa, x);
    }
    PA m_pa;
    OutputIterator m_out;
};
template < class PA, class OutputIterator, class Tag >
property_writer< PA, OutputIterator, Tag > write_property(
    PA pa, OutputIterator out, Tag)
{
    return property_writer< PA, OutputIterator, Tag >(pa, out);
}

//========================================================================
// property_put

/**
 * Functor which just sets a given value to a vertex or edge in a property map.
 */

template < typename PropertyMap, typename EventTag > struct property_put
{
    typedef EventTag event_filter;

    property_put(PropertyMap property_map,
        typename property_traits< PropertyMap >::value_type value)
    : property_map_(property_map), value_(value)
    {
    }

    template < typename VertexOrEdge, typename Graph >
    void operator()(VertexOrEdge v, const Graph&)
    {
        put(property_map_, v, value_);
    }

private:
    PropertyMap property_map_;
    typename property_traits< PropertyMap >::value_type value_;
};

/**
 * Creates a property_put functor which just sets a given value to a vertex or
 * edge.
 *
 * @param property_map Given writeable property map
 * @param value Fixed value of the map
 * @param tag Event Filter
 * @return The functor.
 */

template < typename PropertyMap, typename EventTag >
inline property_put< PropertyMap, EventTag > put_property(
    PropertyMap property_map,
    typename property_traits< PropertyMap >::value_type value, EventTag)
{
    return property_put< PropertyMap, EventTag >(property_map, value);
}

#define BOOST_GRAPH_EVENT_STUB(Event, Kind)                                 \
    typedef ::boost::Event Event##_type;                                    \
    template < typename Visitor >                                           \
    Kind##_visitor< std::pair<                                              \
        detail::functor_to_visitor< Event##_type, Visitor >, Visitors > >   \
        do_##Event(Visitor visitor)                                         \
    {                                                                       \
        typedef std::pair<                                                  \
            detail::functor_to_visitor< Event##_type, Visitor >, Visitors > \
            visitor_list;                                                   \
        typedef Kind##_visitor< visitor_list > result_type;                 \
        return result_type(visitor_list(visitor, m_vis));                   \
    }

} /* namespace boost */

#endif
