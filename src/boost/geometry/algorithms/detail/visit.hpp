// Boost.Geometry

// Copyright (c) 2021, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_VISIT_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_VISIT_HPP

#include <deque>
#include <iterator>
#include <utility>

#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>

#include <boost/geometry/core/static_assert.hpp>
#include <boost/geometry/core/tag.hpp>
#include <boost/geometry/core/tags.hpp>
#include <boost/geometry/core/visit.hpp>
#include <boost/geometry/util/type_traits.hpp>

namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

template <typename Geometry, typename Tag = typename tag<Geometry>::type>
struct visit_one
{
    template <typename F, typename G>
    static void apply(F && f, G && g)
    {
        f(std::forward<G>(g));
    }
};

template <typename Geometry>
struct visit_one<Geometry, void>
{
    BOOST_GEOMETRY_STATIC_ASSERT_FALSE(
        "Not implemented for this Geometry type.",
        Geometry);
};

template <typename Geometry>
struct visit_one<Geometry, dynamic_geometry_tag>
{
    template <typename F, typename Geom>
    static void apply(F && function, Geom && geom)
    {
        traits::visit
            <
                util::remove_cref_t<Geom>
            >::apply(std::forward<F>(function), std::forward<Geom>(geom));
    }
};


template
<
    typename Geometry1, typename Geometry2,
    typename Tag1 = typename tag<Geometry1>::type,
    typename Tag2 = typename tag<Geometry2>::type
>
struct visit_two
{
    template <typename F, typename G1, typename G2>
    static void apply(F && f, G1 && geom1, G2 && geom2)
    {
        f(std::forward<G1>(geom1), std::forward<G2>(geom2));
    }
};

template <typename Geometry1, typename Geometry2, typename Tag2>
struct visit_two<Geometry1, Geometry2, void, Tag2>
{
    BOOST_GEOMETRY_STATIC_ASSERT_FALSE(
        "Not implemented for this Geometry1 type.",
        Geometry1);
};

template <typename Geometry1, typename Geometry2, typename Tag1>
struct visit_two<Geometry1, Geometry2, Tag1, void>
{
    BOOST_GEOMETRY_STATIC_ASSERT_FALSE(
        "Not implemented for this Geometry2 type.",
        Geometry2);
};

template <typename Geometry1, typename Geometry2>
struct visit_two<Geometry1, Geometry2, void, void>
{
    BOOST_GEOMETRY_STATIC_ASSERT_FALSE(
        "Not implemented for these types.",
        Geometry1, Geometry2);
};

template <typename Geometry1, typename Geometry2, typename Tag2>
struct visit_two<Geometry1, Geometry2, dynamic_geometry_tag, Tag2>
{
    template <typename F, typename G1, typename G2>
    static void apply(F && f, G1 && geom1, G2 && geom2)
    {
        traits::visit<util::remove_cref_t<G1>>::apply([&](auto && g1)
        {
            f(std::forward<decltype(g1)>(g1), std::forward<G2>(geom2));
        }, std::forward<G1>(geom1));
    }
};

template <typename Geometry1, typename Geometry2, typename Tag1>
struct visit_two<Geometry1, Geometry2, Tag1, dynamic_geometry_tag>
{
    template <typename F, typename G1, typename G2>
    static void apply(F && f, G1 && geom1, G2 && geom2)
    {
        traits::visit<util::remove_cref_t<G2>>::apply([&](auto && g2)
        {
            f(std::forward<G1>(geom1), std::forward<decltype(g2)>(g2));
        }, std::forward<G2>(geom2));
    }
};

template <typename Geometry1, typename Geometry2>
struct visit_two<Geometry1, Geometry2, dynamic_geometry_tag, dynamic_geometry_tag>
{
    template <typename F, typename G1, typename G2>
    static void apply(F && f, G1 && geom1, G2 && geom2)
    {
        traits::visit
            <
                util::remove_cref_t<G1>, util::remove_cref_t<G2>
            >::apply([&](auto && g1, auto && g2)
            {
                f(std::forward<decltype(g1)>(g1), std::forward<decltype(g2)>(g2));
            }, std::forward<G1>(geom1), std::forward<G2>(geom2));
    }
};


template <typename Geometry, typename Tag = typename tag<Geometry>::type>
struct visit_breadth_first
{
    template <typename F, typename G>
    static bool apply(F f, G && g)
    {
        return f(std::forward<G>(g));
    }
};

template <typename Geometry>
struct visit_breadth_first<Geometry, void>
{
    BOOST_GEOMETRY_STATIC_ASSERT_FALSE(
        "Not implemented for this Geometry type.",
        Geometry);
};

template <typename Geometry>
struct visit_breadth_first<Geometry, dynamic_geometry_tag>
{
    template <typename Geom, typename F>
    static bool apply(F function, Geom && geom)
    {
        bool result = true;
        traits::visit<util::remove_cref_t<Geom>>::apply([&](auto && g)
        {
            result = visit_breadth_first
                <
                    std::remove_reference_t<decltype(g)>
                >::apply(function,
                         std::forward<decltype(g)>(g));
        }, std::forward<Geom>(geom));
        return result;
    }
};

// NOTE: This specialization works partially like std::visit and partially like
//   std::ranges::for_each. If the argument is rvalue reference then the elements
//   are passed into the function as rvalue references as well. This is consistent
//   with std::visit but different than std::ranges::for_each. It's done this way
//   because visit_breadth_first is also specialized for static and dynamic geometries
//   which and references for them has to be propagated like that. If this is not
//   desireable then the support for other kinds of geometries should be dropped and
//   this algorithm should work only for geometry collection.
//   This is not a problem right now because only non-rvalue references are passed
//   but in the future there might be some issues. Consider e.g. passing a temporary
//   mutable proxy range as geometry collection. In such case the elements would be
//   passed as rvalue references which would be incorrect.
template <typename Geometry>
struct visit_breadth_first<Geometry, geometry_collection_tag>
{
    template <typename F, typename Geom>
    static bool apply(F function, Geom && geom)
    {
        using iter_t = std::conditional_t
            <
                std::is_rvalue_reference<decltype(geom)>::value,
                std::move_iterator<typename boost::range_iterator<Geom>::type>,
                typename boost::range_iterator<Geom>::type
            >;

        std::deque<iter_t> queue;

        iter_t it = iter_t{ boost::begin(geom) };
        iter_t end = iter_t{ boost::end(geom) };
        for(;;)
        {
            for (; it != end; ++it)
            {
                bool result = true;
                traits::iter_visit<util::remove_cref_t<Geom>>::apply([&](auto && g)
                {
                    result = visit_or_enqueue(function, std::forward<decltype(g)>(g), queue, it);
                }, it);

                if (! result)
                {
                    return false;
                }
            }

            if (queue.empty())
            {
                break;
            }

            // Alternatively store a pointer to GeometryCollection
            // so this call can be avoided.
            traits::iter_visit<util::remove_cref_t<Geom>>::apply([&](auto && g)
            {
                set_iterators(std::forward<decltype(g)>(g), it, end);
            }, queue.front());
            queue.pop_front();
        }

        return true;
    }

private:
    template
    <
        typename F, typename Geom, typename Iterator,
        std::enable_if_t<util::is_geometry_collection<Geom>::value, int> = 0
    >
    static bool visit_or_enqueue(F &, Geom &&, std::deque<Iterator> & queue, Iterator iter)
    {
        queue.push_back(iter);
        return true;
    }
    template
    <
        typename F, typename Geom, typename Iterator,
        std::enable_if_t<! util::is_geometry_collection<Geom>::value, int> = 0
    >
    static bool visit_or_enqueue(F & f, Geom && g, std::deque<Iterator> & , Iterator)
    {
        return f(std::forward<Geom>(g));
    }

    template
    <
        typename Geom, typename Iterator,
        std::enable_if_t<util::is_geometry_collection<Geom>::value, int> = 0
    >
    static void set_iterators(Geom && g, Iterator & first, Iterator & last)
    {
        first = Iterator{ boost::begin(g) };
        last = Iterator{ boost::end(g) };
    }
    template
    <
        typename Geom, typename Iterator,
        std::enable_if_t<! util::is_geometry_collection<Geom>::value, int> = 0
    >
    static void set_iterators(Geom &&, Iterator &, Iterator &)
    {}
};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


#ifndef DOXYGEN_NO_DETAIL
namespace detail
{

template <typename UnaryFunction, typename Geometry>
inline void visit(UnaryFunction && function, Geometry && geometry)
{
    dispatch::visit_one
        <
            std::remove_reference_t<Geometry>
        >::apply(std::forward<UnaryFunction>(function), std::forward<Geometry>(geometry));
}


template <typename UnaryFunction, typename Geometry1, typename Geometry2>
inline void visit(UnaryFunction && function, Geometry1 && geometry1, Geometry2 && geometry2)
{
    dispatch::visit_two
        <
            std::remove_reference_t<Geometry1>,
            std::remove_reference_t<Geometry2>
        >::apply(std::forward<UnaryFunction>(function),
                 std::forward<Geometry1>(geometry1),
                 std::forward<Geometry2>(geometry2));
}


template <typename UnaryFunction, typename Geometry>
inline void visit_breadth_first(UnaryFunction function, Geometry && geometry)
{
    dispatch::visit_breadth_first
        <
            std::remove_reference_t<Geometry>
        >::apply(function, std::forward<Geometry>(geometry));
}

} // namespace detail
#endif // DOXYGEN_NO_DETAIL

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_VISIT_HPP
