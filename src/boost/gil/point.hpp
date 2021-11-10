//
// Copyright 2005-2007 Adobe Systems Incorporated
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_POINT_HPP
#define BOOST_GIL_POINT_HPP

#include <boost/gil/utilities.hpp>
#include <boost/gil/detail/std_common_type.hpp>

#include <boost/config.hpp>

#include <cstddef>
#include <type_traits>

namespace boost { namespace gil {

/// \addtogroup PointModel
///
/// Example:
/// \code
/// point<std::ptrdiff_t> p(3,2);
/// assert((p[0] == p.x) && (p[1] == p.y));
/// assert(axis_value<0>(p) == 3);
/// assert(axis_value<1>(p) == 2);
/// \endcode

/// \brief 2D point both axes of which have the same dimension type
/// \ingroup PointModel
/// Models: Point2DConcept
template <typename T>
class point
{
public:
    using value_type = T;

    template<std::size_t D>
    struct axis
    {
        using coord_t = value_type;
    };

    static constexpr std::size_t num_dimensions = 2;

    point() = default;
    point(T px, T py) : x(px), y(py) {}

    point operator<<(std::ptrdiff_t shift) const
    {
        return point(x << shift, y << shift);
    }

    point operator>>(std::ptrdiff_t shift) const
    {
        return point(x >> shift, y >> shift);
    }

    point& operator+=(point const& p)
    {
        x += p.x;
        y += p.y;
        return *this;
    }

    point& operator-=(point const& p)
    {
        x -= p.x;
        y -= p.y;
        return *this;
    }

    point& operator/=(double d)
    {
        if (d < 0 || 0 < d)
        {
            x = static_cast<T>(x / d);
            y = static_cast<T>(y / d);
        }
        return *this;
    }

    point& operator*=(double d)
    {
        x = static_cast<T>(x * d);
        y = static_cast<T>(y * d);
        return *this;
    }

    T const& operator[](std::size_t i) const
    {
        return this->*mem_array[i];
    }

    T& operator[](std::size_t i)
    {
        return this->*mem_array[i];
    }

    T x{0};
    T y{0};

private:
    // this static array of pointers to member variables makes operator[] safe
    // and doesn't seem to exhibit any performance penalty.
    static T point<T>::* const mem_array[num_dimensions];
};

/// Alias template for backward compatibility with Boost <=1.68.
template <typename T>
using point2 = point<T>;

/// Common type to represent 2D dimensions or in-memory size of image or view.
/// @todo TODO: rename to dims_t or dimensions_t for purpose clarity?
using point_t = point<std::ptrdiff_t>;

template <typename T>
T point<T>::* const point<T>::mem_array[point<T>::num_dimensions] =
{
    &point<T>::x,
    &point<T>::y
};

/// \ingroup PointModel
template <typename T>
BOOST_FORCEINLINE
bool operator==(const point<T>& p1, const point<T>& p2)
{
    return p1.x == p2.x && p1.y == p2.y;
}

/// \ingroup PointModel
template <typename T>
BOOST_FORCEINLINE
bool operator!=(const point<T>& p1, const point<T>& p2)
{
    return p1.x != p2.x || p1.y != p2.y;
}

/// \ingroup PointModel
template <typename T>
BOOST_FORCEINLINE
point<T> operator+(const point<T>& p1, const point<T>& p2)
{
    return { p1.x + p2.x, p1.y + p2.y };
}

/// \ingroup PointModel
template <typename T>
BOOST_FORCEINLINE
point<T> operator-(const point<T>& p)
{
    return { -p.x, -p.y };
}

/// \ingroup PointModel
template <typename T>
BOOST_FORCEINLINE
point<T> operator-(const point<T>& p1, const point<T>& p2)
{
    return { p1.x - p2.x, p1.y - p2.y };
}

/// \ingroup PointModel
template <typename T, typename D>
BOOST_FORCEINLINE
auto operator/(point<T> const& p, D d)
    -> typename std::enable_if
    <
        std::is_arithmetic<D>::value,
        point<typename detail::std_common_type<T, D>::type>
    >::type
{
    static_assert(std::is_arithmetic<D>::value, "denominator is not arithmetic type");
    using result_type = typename detail::std_common_type<T, D>::type;
    if (d < 0 || 0 < d)
    {
        double const x = static_cast<double>(p.x) / static_cast<double>(d);
        double const y = static_cast<double>(p.y) / static_cast<double>(d);
        return point<result_type>{
            static_cast<result_type>(iround(x)),
            static_cast<result_type>(iround(y))};
    }
    else
    {
        return point<result_type>{0, 0};
    }
}

/// \ingroup PointModel
template <typename T, typename M>
BOOST_FORCEINLINE
auto operator*(point<T> const& p, M m)
    -> typename std::enable_if
    <
        std::is_arithmetic<M>::value,
        point<typename detail::std_common_type<T, M>::type>
    >::type
{
    static_assert(std::is_arithmetic<M>::value, "multiplier is not arithmetic type");
    using result_type = typename detail::std_common_type<T, M>::type;
    return point<result_type>{p.x * m, p.y * m};
}

/// \ingroup PointModel
template <typename T, typename M>
BOOST_FORCEINLINE
auto operator*(M m, point<T> const& p)
    -> typename std::enable_if
    <
        std::is_arithmetic<M>::value,
        point<typename detail::std_common_type<T, M>::type>
    >::type
{
    static_assert(std::is_arithmetic<M>::value, "multiplier is not arithmetic type");
    using result_type = typename detail::std_common_type<T, M>::type;
    return point<result_type>{p.x * m, p.y * m};
}

/// \ingroup PointModel
template <std::size_t K, typename T>
BOOST_FORCEINLINE
T const& axis_value(point<T> const& p)
{
    static_assert(K < point<T>::num_dimensions, "axis index out of range");
    return p[K];
}

/// \ingroup PointModel
template <std::size_t K, typename T>
BOOST_FORCEINLINE
T& axis_value(point<T>& p)
{
    static_assert(K < point<T>::num_dimensions, "axis index out of range");
    return p[K];
}

/// \addtogroup PointAlgorithm
///
/// Example:
/// \code
/// assert(iround(point<double>(3.1, 3.9)) == point<std::ptrdiff_t>(3,4));
/// \endcode

/// \ingroup PointAlgorithm
template <typename T>
inline point<std::ptrdiff_t> iround(point<T> const& p)
{
    static_assert(std::is_integral<T>::value, "T is not integer");
    return { static_cast<std::ptrdiff_t>(p.x), static_cast<std::ptrdiff_t>(p.y) };
}

/// \ingroup PointAlgorithm
inline point<std::ptrdiff_t> iround(point<float> const& p)
{
    return { iround(p.x), iround(p.y) };
}

/// \ingroup PointAlgorithm
inline point<std::ptrdiff_t> iround(point<double> const& p)
{
    return { iround(p.x), iround(p.y) };
}

/// \ingroup PointAlgorithm
inline point<std::ptrdiff_t> ifloor(point<float> const& p)
{
    return { ifloor(p.x), ifloor(p.y) };
}

/// \ingroup PointAlgorithm
inline point<std::ptrdiff_t> ifloor(point<double> const& p)
{
    return { ifloor(p.x), ifloor(p.y) };
}

/// \ingroup PointAlgorithm
inline point<std::ptrdiff_t> iceil(point<float> const& p)
{
    return { iceil(p.x), iceil(p.y) };
}

/// \ingroup PointAlgorithm
inline point<std::ptrdiff_t> iceil(point<double> const& p)
{
    return { iceil(p.x), iceil(p.y) };
}

}} // namespace boost::gil

#endif
