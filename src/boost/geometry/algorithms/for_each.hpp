// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2014 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2014 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2014 Mateusz Loskot, London, UK.
// Copyright (c) 2014 Adam Wulkiewicz, Lodz, Poland.

// This file was modified by Oracle on 2014-2020.
// Modifications copyright (c) 2014-2020, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_FOR_EACH_HPP
#define BOOST_GEOMETRY_ALGORITHMS_FOR_EACH_HPP


#include <algorithm>

#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/reference.hpp>
#include <boost/range/value_type.hpp>

#include <boost/geometry/algorithms/detail/interior_iterator.hpp>
#include <boost/geometry/algorithms/not_implemented.hpp>
#include <boost/geometry/core/closure.hpp>
#include <boost/geometry/core/exterior_ring.hpp>
#include <boost/geometry/core/interior_rings.hpp>
#include <boost/geometry/core/point_type.hpp>
#include <boost/geometry/core/tag_cast.hpp>
#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/geometries/concepts/check.hpp>

#include <boost/geometry/geometries/segment.hpp>

#include <boost/geometry/util/range.hpp>
#include <boost/geometry/util/type_traits.hpp>

#include <boost/geometry/views/detail/indexed_point_view.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace for_each
{


struct fe_point_point
{
    template <typename Point, typename Functor>
    static inline bool apply(Point& point, Functor&& f)
    {
        return f(point);
    }
};


struct fe_segment_point
{
    template <typename Point, typename Functor>
    static inline bool apply(Point& , Functor&& )
    {
        // TODO: if non-const, we should extract the points from the segment
        // and call the functor on those two points

        //model::referring_segment<Point> s(point, point);
        //return f(s);

        return true;
    }
};


struct fe_point_segment
{
    template <typename Segment, typename Functor>
    static inline bool apply(Segment& s, Functor&& f)
    {
        // Or should we guarantee that the type of points is
        // point_type<Segment>::type ?
        geometry::detail::indexed_point_view<Segment, 0> p0(s);
        geometry::detail::indexed_point_view<Segment, 1> p1(s);
        return f(p0) && f(p1);
    }
};

struct fe_segment_segment
{
    template <typename Segment, typename Functor>
    static inline bool apply(Segment& s, Functor&& f)
    {
        // Or should we guarantee that the type of segment is
        // referring_segment<...> ?
        return f(s);
    }
};


template <typename Range>
struct fe_range_value
{
    typedef util::transcribe_const_t
        <
            Range,
            typename boost::range_value<Range>::type
        > type;
};

template <typename Range>
struct fe_point_type
{
    typedef util::transcribe_const_t
        <
            Range,
            typename point_type<Range>::type
        > type;
};


template <typename Range>
struct fe_point_type_is_referencable
{
    static const bool value =
        std::is_const<Range>::value
     || std::is_same
            <
                typename boost::range_reference<Range>::type,
                typename fe_point_type<Range>::type&
            >::value;
};


template
<
    typename Range,
    bool UseReferences = fe_point_type_is_referencable<Range>::value
>
struct fe_point_call_f
{
    template <typename Iterator, typename Functor>
    static inline bool apply(Iterator it, Functor&& f)
    {
        // Implementation for real references (both const and mutable)
        // and const proxy references.
        typedef typename fe_point_type<Range>::type point_type;
        point_type& p = *it;
        return f(p);
    }
};

template <typename Range>
struct fe_point_call_f<Range, false>
{
    template <typename Iterator, typename Functor>
    static inline bool apply(Iterator it, Functor&& f)
    {
        // Implementation for proxy mutable references.
        // Temporary point has to be created and assigned afterwards.
        typedef typename fe_point_type<Range>::type point_type;
        point_type p = *it;
        bool result = f(p);
        *it = p;
        return result;
    }
};


struct fe_point_range
{
    template <typename Range, typename Functor>
    static inline bool apply(Range& range, Functor&& f)
    {
        auto const end = boost::end(range);
        for (auto it = boost::begin(range); it != end; ++it)
        {
            if (! fe_point_call_f<Range>::apply(it, f))
            {
                return false;
            }
        }

        return true;
    }
};


template
<
    typename Range,
    bool UseReferences = fe_point_type_is_referencable<Range>::value
>
struct fe_segment_call_f
{
    template <typename Iterator, typename Functor>
    static inline bool apply(Iterator it0, Iterator it1, Functor&& f)
    {
        // Implementation for real references (both const and mutable)
        // and const proxy references.
        // If const proxy references are returned by iterators
        // then const real references here prevents temporary
        // objects from being destroyed.
        typedef typename fe_point_type<Range>::type point_type;
        point_type& p0 = *it0;
        point_type& p1 = *it1;
        model::referring_segment<point_type> s(p0, p1);
        return f(s);
    }
};

template <typename Range>
struct fe_segment_call_f<Range, false>
{
    template <typename Iterator, typename Functor>
    static inline bool apply(Iterator it0, Iterator it1, Functor&& f)
    {
        // Mutable proxy references returned by iterators.
        // Temporary points have to be created and assigned afterwards.
        typedef typename fe_point_type<Range>::type point_type;
        point_type p0 = *it0;
        point_type p1 = *it1;
        model::referring_segment<point_type> s(p0, p1);
        bool result = f(s);
        *it0 = p0;
        *it1 = p1;
        return result;
    }
};


template <closure_selector Closure>
struct fe_segment_range_with_closure
{
    template <typename Range, typename Functor>
    static inline bool apply(Range& range, Functor&& f)
    {
        auto it = boost::begin(range);
        auto const end = boost::end(range);
        if (it == end)
        {
            return true;
        }

        auto previous = it++;
        if (it == end)
        {
            return fe_segment_call_f<Range>::apply(previous, previous, f);
        }

        while (it != end)
        {
            if (! fe_segment_call_f<Range>::apply(previous, it, f))
            {
                return false;
            }
            previous = it++;
        }

        return true;
    }
};


template <>
struct fe_segment_range_with_closure<open>
{
    template <typename Range, typename Functor>
    static inline bool apply(Range& range, Functor&& f)
    {
        fe_segment_range_with_closure<closed>::apply(range, f);

        auto const begin = boost::begin(range);
        auto end = boost::end(range);
        if (begin == end)
        {
            return true;
        }
        
        --end;
        
        if (begin == end)
        {
            // single point ranges already handled in closed case above
            return true;
        }

        return fe_segment_call_f<Range>::apply(end, begin, f);
    }
};


struct fe_segment_range
{
    template <typename Range, typename Functor>
    static inline bool apply(Range& range, Functor&& f)
    {
        return fe_segment_range_with_closure
            <
                closure<Range>::value
            >::apply(range, f);
    }
};


template <typename RangePolicy>
struct for_each_polygon
{
    template <typename Polygon, typename Functor>
    static inline bool apply(Polygon& poly, Functor&& f)
    {
        if (! RangePolicy::apply(exterior_ring(poly), f))
        {
            return false;
        }

        typename interior_return_type<Polygon>::type
            rings = interior_rings(poly);

        auto const end = boost::end(rings);
        for (auto it = boost::begin(rings); it != end; ++it)
        {
            // NOTE: Currently lvalue iterator required
            if (! RangePolicy::apply(*it, f))
            {
                return false;
            }
        }

        return true;
    }

};

// Implementation of multi, for both point and segment,
// just calling the single version.
template <typename SinglePolicy>
struct for_each_multi
{
    template <typename MultiGeometry, typename Functor>
    static inline bool apply(MultiGeometry& multi, Functor&& f)
    {
        auto const end = boost::end(multi);
        for (auto it = boost::begin(multi); it != end; ++it)
        {
            // NOTE: Currently lvalue iterator required
            if (! SinglePolicy::apply(*it, f))
            {
                return false;
            }
        }

        return true;
    }
};

}} // namespace detail::for_each
#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

template
<
    typename Geometry,
    typename Tag = typename tag_cast<typename tag<Geometry>::type, multi_tag>::type
>
struct for_each_point: not_implemented<Tag>
{};


template <typename Point>
struct for_each_point<Point, point_tag>
    : detail::for_each::fe_point_point
{};


template <typename Segment>
struct for_each_point<Segment, segment_tag>
    : detail::for_each::fe_point_segment
{};


template <typename Linestring>
struct for_each_point<Linestring, linestring_tag>
    : detail::for_each::fe_point_range
{};


template <typename Ring>
struct for_each_point<Ring, ring_tag>
    : detail::for_each::fe_point_range
{};


template <typename Polygon>
struct for_each_point<Polygon, polygon_tag>
    : detail::for_each::for_each_polygon
        <
            detail::for_each::fe_point_range
        >
{};


template <typename MultiGeometry>
struct for_each_point<MultiGeometry, multi_tag>
    : detail::for_each::for_each_multi
        <
            // Specify the dispatch of the single-version as policy
            for_each_point
                <
                    typename detail::for_each::fe_range_value
                        <
                            MultiGeometry
                        >::type
                >
        >
{};


template
<
    typename Geometry,
    typename Tag = typename tag<Geometry>::type
>
struct for_each_segment: not_implemented<Tag>
{};

template <typename Point>
struct for_each_segment<Point, point_tag>
    : detail::for_each::fe_segment_point // empty
{};


template <typename Segment>
struct for_each_segment<Segment, segment_tag>
    : detail::for_each::fe_segment_segment
{};


template <typename Linestring>
struct for_each_segment<Linestring, linestring_tag>
    : detail::for_each::fe_segment_range
{};


template <typename Ring>
struct for_each_segment<Ring, ring_tag>
    : detail::for_each::fe_segment_range
{};


template <typename Polygon>
struct for_each_segment<Polygon, polygon_tag>
    : detail::for_each::for_each_polygon
        <
            detail::for_each::fe_segment_range
        >
{};


template <typename MultiPoint>
struct for_each_segment<MultiPoint, multi_point_tag>
    : detail::for_each::fe_segment_point // empty
{};


template <typename MultiLinestring>
struct for_each_segment<MultiLinestring, multi_linestring_tag>
    : detail::for_each::for_each_multi
        <
            detail::for_each::fe_segment_range
        >
{};

template <typename MultiPolygon>
struct for_each_segment<MultiPolygon, multi_polygon_tag>
    : detail::for_each::for_each_multi
        <
            detail::for_each::for_each_polygon
                <
                    detail::for_each::fe_segment_range
                >
        >
{};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


template<typename Geometry, typename UnaryPredicate>
inline bool all_points_of(Geometry& geometry, UnaryPredicate p)
{
    concepts::check<Geometry>();

    return dispatch::for_each_point<Geometry>::apply(geometry, p);
}


template<typename Geometry, typename UnaryPredicate>
inline bool all_segments_of(Geometry const& geometry, UnaryPredicate p)
{
    concepts::check<Geometry const>();

    return dispatch::for_each_segment<Geometry const>::apply(geometry, p);
}


template<typename Geometry, typename UnaryPredicate>
inline bool any_point_of(Geometry& geometry, UnaryPredicate p)
{
    concepts::check<Geometry>();

    return ! dispatch::for_each_point<Geometry>::apply(geometry, [&](auto&& pt)
    {
        return ! p(pt);
    });
}


template<typename Geometry, typename UnaryPredicate>
inline bool any_segment_of(Geometry const& geometry, UnaryPredicate p)
{
    concepts::check<Geometry const>();

    return ! dispatch::for_each_segment<Geometry const>::apply(geometry, [&](auto&& s)
    {
        return ! p(s);
    });
}

template<typename Geometry, typename UnaryPredicate>
inline bool none_point_of(Geometry& geometry, UnaryPredicate p)
{
    concepts::check<Geometry>();

    return dispatch::for_each_point<Geometry>::apply(geometry, [&](auto&& pt)
    {
        return ! p(pt);
    });
}


template<typename Geometry, typename UnaryPredicate>
inline bool none_segment_of(Geometry const& geometry, UnaryPredicate p)
{
    concepts::check<Geometry const>();

    return dispatch::for_each_segment<Geometry const>::apply(geometry, [&](auto&& s)
    {
        return ! p(s);
    });
}


/*!
\brief \brf_for_each{point}
\details \det_for_each{point}
\ingroup for_each
\param geometry \param_geometry
\param f \par_for_each_f{point}
\tparam Geometry \tparam_geometry
\tparam Functor \tparam_functor

\qbk{[include reference/algorithms/for_each_point.qbk]}
\qbk{[heading Example]}
\qbk{[for_each_point] [for_each_point_output]}
\qbk{[for_each_point_const] [for_each_point_const_output]}
*/
template<typename Geometry, typename Functor>
inline Functor for_each_point(Geometry& geometry, Functor f)
{
    concepts::check<Geometry>();

    dispatch::for_each_point<Geometry>::apply(geometry, [&](auto&& pt)
    {
        f(pt);
        // TODO: Implement separate function?
        return true;
    });
    return f;
}


/*!
\brief \brf_for_each{segment}
\details \det_for_each{segment}
\ingroup for_each
\param geometry \param_geometry
\param f \par_for_each_f{segment}
\tparam Geometry \tparam_geometry
\tparam Functor \tparam_functor

\qbk{[include reference/algorithms/for_each_segment.qbk]}
\qbk{[heading Example]}
\qbk{[for_each_segment_const] [for_each_segment_const_output]}
*/
template<typename Geometry, typename Functor>
inline Functor for_each_segment(Geometry& geometry, Functor f)
{
    concepts::check<Geometry>();

    dispatch::for_each_segment<Geometry>::apply(geometry, [&](auto&& s)
    {
        f(s);
        // TODO: Implement separate function?
        return true;
    });
    return f;
}


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_FOR_EACH_HPP
