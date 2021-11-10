// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2013 Barend Gehrels, Amsterdam, the Netherlands.

// This file was modified by Oracle on 2016-2020.
// Modifications copyright (c) 2016-2020 Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_POLICIES_ROBUSTNESS_SEGMENT_RATIO_HPP
#define BOOST_GEOMETRY_POLICIES_ROBUSTNESS_SEGMENT_RATIO_HPP

#include <type_traits>

#include <boost/config.hpp>
#include <boost/rational.hpp>

#include <boost/geometry/core/assert.hpp>
#include <boost/geometry/util/math.hpp>
#include <boost/geometry/util/promote_floating_point.hpp>

namespace boost { namespace geometry
{


namespace detail { namespace segment_ratio
{

template
<
    typename Type,
    bool IsIntegral = std::is_integral<Type>::type::value
>
struct less {};

template <typename Type>
struct less<Type, true>
{
    template <typename Ratio>
    static inline bool apply(Ratio const& lhs, Ratio const& rhs)
    {
        return boost::rational<Type>(lhs.numerator(), lhs.denominator())
             < boost::rational<Type>(rhs.numerator(), rhs.denominator());
    }
};

template <typename Type>
struct less<Type, false>
{
    template <typename Ratio>
    static inline bool apply(Ratio const& lhs, Ratio const& rhs)
    {
        BOOST_GEOMETRY_ASSERT(lhs.denominator() != 0);
        BOOST_GEOMETRY_ASSERT(rhs.denominator() != 0);
        Type const a = lhs.numerator() / lhs.denominator();
        Type const b = rhs.numerator() / rhs.denominator();
        return ! geometry::math::equals(a, b)
            && a < b;
    }
};

template
<
    typename Type,
    bool IsIntegral = std::is_integral<Type>::type::value
>
struct equal {};

template <typename Type>
struct equal<Type, true>
{
    template <typename Ratio>
    static inline bool apply(Ratio const& lhs, Ratio const& rhs)
    {
        return boost::rational<Type>(lhs.numerator(), lhs.denominator())
            == boost::rational<Type>(rhs.numerator(), rhs.denominator());
    }
};

template <typename Type>
struct equal<Type, false>
{
    template <typename Ratio>
    static inline bool apply(Ratio const& lhs, Ratio const& rhs)
    {
        BOOST_GEOMETRY_ASSERT(lhs.denominator() != 0);
        BOOST_GEOMETRY_ASSERT(rhs.denominator() != 0);
        Type const a = lhs.numerator() / lhs.denominator();
        Type const b = rhs.numerator() / rhs.denominator();
        return geometry::math::equals(a, b);
    }
};

template
<
    typename Type,
    bool IsFloatingPoint = std::is_floating_point<Type>::type::value
>
struct possibly_collinear {};

template <typename Type>
struct possibly_collinear<Type, true>
{
    template <typename Ratio, typename Threshold>
    static inline bool apply(Ratio const& ratio, Threshold threshold)
    {
        return std::abs(ratio.denominator()) < threshold;
    }
};

// Any ratio based on non-floating point (or user defined floating point)
// is collinear if the denominator is exactly zero
template <typename Type>
struct possibly_collinear<Type, false>
{
    template <typename Ratio, typename Threshold>
    static inline bool apply(Ratio const& ratio, Threshold)
    {
        return ratio.denominator() == 0;
    }
};

}}

//! Small class to keep a ratio (e.g. 1/4)
//! Main purpose is intersections and checking on 0, 1, and smaller/larger
//! The prototype used Boost.Rational. However, we also want to store FP ratios,
//! (so numerator/denominator both in float)
//! and Boost.Rational starts with GCD which we prefer to avoid if not necessary
//! On a segment means: this ratio is between 0 and 1 (both inclusive)
//!
template <typename Type>
class segment_ratio
{
public :
    typedef Type numeric_type;

    // Type-alias for the type itself
    typedef segment_ratio<Type> thistype;

    inline segment_ratio()
        : m_numerator(0)
        , m_denominator(1)
        , m_approximation(0)
    {}

    inline segment_ratio(const Type& numerator, const Type& denominator)
        : m_numerator(numerator)
        , m_denominator(denominator)
    {
        initialize();
    }

    segment_ratio(segment_ratio const&) = default;
    segment_ratio& operator=(segment_ratio const&) = default;
    segment_ratio(segment_ratio&&) = default;
    segment_ratio& operator=(segment_ratio&&) = default;
    
    // These are needed because in intersection strategies ratios are assigned
    // in fractions and if a user passes CalculationType then ratio Type in
    // turns is taken from geometry coordinate_type and the one used in
    // a strategy uses Type selected using CalculationType.
    // See: detail::overlay::intersection_info_base
    // and  policies::relate::segments_intersection_points
    //      in particular segments_collinear() where ratios are assigned.
    template<typename T> friend class segment_ratio;
    template <typename T>
    segment_ratio(segment_ratio<T> const& r)
        : m_numerator(r.m_numerator)
        , m_denominator(r.m_denominator)
    {
        initialize();
    }
    template <typename T>
    segment_ratio& operator=(segment_ratio<T> const& r)
    {
        m_numerator = r.m_numerator;
        m_denominator = r.m_denominator;
        initialize();
        return *this;
    }
    template <typename T>
    segment_ratio(segment_ratio<T> && r)
        : m_numerator(std::move(r.m_numerator))
        , m_denominator(std::move(r.m_denominator))
    {
        initialize();
    }
    template <typename T>
    segment_ratio& operator=(segment_ratio<T> && r)
    {
        m_numerator = std::move(r.m_numerator);
        m_denominator = std::move(r.m_denominator);
        initialize();
        return *this;
    }

    inline Type const& numerator() const { return m_numerator; }
    inline Type const& denominator() const { return m_denominator; }

    inline void assign(const Type& numerator, const Type& denominator)
    {
        m_numerator = numerator;
        m_denominator = denominator;
        initialize();
    }

    inline void initialize()
    {
        // Minimal normalization
        // 1/-4 => -1/4, -1/-4 => 1/4
        if (m_denominator < 0)
        {
            m_numerator = -m_numerator;
            m_denominator = -m_denominator;
        }

        m_approximation =
            m_denominator == 0 ? 0
            : (
                boost::numeric_cast<fp_type>(m_numerator) * scale()
                / boost::numeric_cast<fp_type>(m_denominator)
            );
    }

    inline bool is_zero() const { return math::equals(m_numerator, 0); }
    inline bool is_one() const { return math::equals(m_numerator, m_denominator); }
    inline bool on_segment() const
    {
        // e.g. 0/4 or 4/4 or 2/4
        return m_numerator >= 0 && m_numerator <= m_denominator;
    }
    inline bool in_segment() const
    {
        // e.g. 1/4
        return m_numerator > 0 && m_numerator < m_denominator;
    }
    inline bool on_end() const
    {
        // e.g. 0/4 or 4/4
        return is_zero() || is_one();
    }
    inline bool left() const
    {
        // e.g. -1/4
        return m_numerator < 0;
    }
    inline bool right() const
    {
        // e.g. 5/4
        return m_numerator > m_denominator;
    }

    inline bool near_end() const
    {
        if (left() || right())
        {
            return false;
        }

        static fp_type const small_part_of_scale = scale() / 100;
        return m_approximation < small_part_of_scale
            || m_approximation > scale() - small_part_of_scale;
    }

    inline bool close_to(thistype const& other) const
    {
        return geometry::math::abs(m_approximation - other.m_approximation) < 50;
    }

    template <typename Threshold>
    inline bool possibly_collinear(Threshold threshold) const
    {
        return detail::segment_ratio::possibly_collinear<Type>::apply(*this, threshold);
    }

    inline bool operator< (thistype const& other) const
    {
        return close_to(other)
            ? detail::segment_ratio::less<Type>::apply(*this, other)
            : m_approximation < other.m_approximation;
    }

    inline bool operator== (thistype const& other) const
    {
        return close_to(other)
            && detail::segment_ratio::equal<Type>::apply(*this, other);
    }

    static inline thistype zero()
    {
        static thistype result(0, 1);
        return result;
    }

    static inline thistype one()
    {
        static thistype result(1, 1);
        return result;
    }

#if defined(BOOST_GEOMETRY_DEFINE_STREAM_OPERATOR_SEGMENT_RATIO)
    friend std::ostream& operator<<(std::ostream &os, segment_ratio const& ratio)
    {
        os << ratio.m_numerator << "/" << ratio.m_denominator
           << " (" << (static_cast<double>(ratio.m_numerator)
                        / static_cast<double>(ratio.m_denominator))
           << ")";
        return os;
    }
#endif



private :
    // NOTE: if this typedef is used then fp_type is non-fundamental type
    // if Type is non-fundamental type
    //typedef typename promote_floating_point<Type>::type fp_type;

    // TODO: What with user-defined numeric types?
    //       Shouldn't here is_integral be checked?
    typedef std::conditional_t
        <
            std::is_floating_point<Type>::value, Type, double
        > fp_type;

    Type m_numerator;
    Type m_denominator;

    // Contains ratio on scale 0..1000000 (for 0..1)
    // This is an approximation for fast and rough comparisons
    // Boost.Rational is used if the approximations are close.
    // Reason: performance, Boost.Rational does a GCD by default and also the
    // comparisons contain while-loops.
    fp_type m_approximation;


    static inline fp_type scale()
    {
        return 1000000.0;
    }
};


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_POLICIES_ROBUSTNESS_SEGMENT_RATIO_HPP
