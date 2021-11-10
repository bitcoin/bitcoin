// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// This file was modified by Oracle on 2014, 2016, 2017, 2018, 2020.
// Modifications copyright (c) 2014-2020 Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_SRS_SPHEROID_HPP
#define BOOST_GEOMETRY_SRS_SPHEROID_HPP


#include <cstddef>

#include <boost/static_assert.hpp>

#include <boost/geometry/core/radius.hpp>
#include <boost/geometry/core/tag.hpp>
#include <boost/geometry/core/tags.hpp>

#if defined(BOOST_GEOMETRY_ENABLE_ACCESS_DEBUGGING)
#include <boost/geometry/core/assert.hpp>
#endif


namespace boost { namespace geometry
{
    
namespace srs
{

/*!
    \brief Defines spheroid radius values for use in geographical CS calculations
    \ingroup srs
    \note See http://en.wikipedia.org/wiki/Figure_of_the_Earth
          and http://en.wikipedia.org/wiki/World_Geodetic_System#A_new_World_Geodetic_System:_WGS84
    \tparam RadiusType tparam_radius
*/
template <typename RadiusType>
class spheroid
{
public:
    spheroid(RadiusType const& a, RadiusType const& b)
        : m_a(a)
        , m_b(b)
    {
#if defined(BOOST_GEOMETRY_ENABLE_ACCESS_DEBUGGING)
        m_created = 1;
#endif
    }

    spheroid()
        : m_a(RadiusType(6378137.0))
        , m_b(RadiusType(6356752.3142451793))
    {
#if defined(BOOST_GEOMETRY_ENABLE_ACCESS_DEBUGGING)
        m_created = 1;
#endif
    }

#if defined(BOOST_GEOMETRY_ENABLE_ACCESS_DEBUGGING)
    ~spheroid()
    {
        m_created = 0;
    }
#endif

    template <std::size_t I>
    RadiusType get_radius() const
    {
#if defined(BOOST_GEOMETRY_ENABLE_ACCESS_DEBUGGING)
        if (m_created != 1)
        {
            int a = 10;
        }
        BOOST_GEOMETRY_ASSERT(m_created == 1);
#endif

        BOOST_STATIC_ASSERT(I < 3);

        return I < 2 ? m_a : m_b;
    }

    template <std::size_t I>
    void set_radius(RadiusType const& radius)
    {
#if defined(BOOST_GEOMETRY_ENABLE_ACCESS_DEBUGGING)
        BOOST_GEOMETRY_ASSERT(m_created == 1);
#endif

        BOOST_STATIC_ASSERT(I < 3);

        (I < 2 ? m_a : m_b) = radius;
    }

private:
    RadiusType m_a, m_b; // equatorial radius, polar radius

#if defined(BOOST_GEOMETRY_ENABLE_ACCESS_DEBUGGING)
    int m_created;
#endif
};

} // namespace srs

// Traits specializations for spheroid
#ifndef DOXYGEN_NO_TRAITS_SPECIALIZATIONS
namespace traits
{

template <typename RadiusType>
struct tag< srs::spheroid<RadiusType> >
{
    typedef srs_spheroid_tag type;
};

template <typename RadiusType>
struct radius_type< srs::spheroid<RadiusType> >
{
    typedef RadiusType type;
};

template <typename RadiusType, std::size_t Dimension>
struct radius_access<srs::spheroid<RadiusType>, Dimension>
{
    typedef srs::spheroid<RadiusType> spheroid_type;

    static inline RadiusType get(spheroid_type const& s)
    {
        return s.template get_radius<Dimension>();
    }

    static inline void set(spheroid_type& s, RadiusType const& value)
    {
        s.template set_radius<Dimension>(value);
    }
};

} // namespace traits
#endif // DOXYGEN_NO_TRAITS_SPECIALIZATIONS


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_SRS_SPHEROID_HPP
