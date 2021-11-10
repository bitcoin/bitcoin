// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// This file was modified by Oracle on 2014, 2016, 2018.
// Modifications copyright (c) 2014-2018 Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_SRS_SPHERE_HPP
#define BOOST_GEOMETRY_SRS_SPHERE_HPP


#include <cstddef>

#include <boost/static_assert.hpp>

#include <boost/geometry/core/radius.hpp>
#include <boost/geometry/core/tag.hpp>
#include <boost/geometry/core/tags.hpp>


namespace boost { namespace geometry
{

namespace srs
{

/*!
    \brief Defines sphere radius value for use in spherical CS calculations
    \ingroup srs
    \tparam RadiusType tparam_radius
*/
template <typename RadiusType>
class sphere
{
public:
    explicit sphere(RadiusType const& r)
        : m_r(r)
    {}

    sphere()
        : m_r(RadiusType((2.0 * 6378137.0 + 6356752.3142451793) / 3.0))
    {}

    template <std::size_t I>
    RadiusType get_radius() const
    {
        BOOST_STATIC_ASSERT(I < 3);

        return m_r;
    }

    template <std::size_t I>
    void set_radius(RadiusType const& radius)
    {
        BOOST_STATIC_ASSERT(I < 3);

        m_r = radius;
    }

private:
    RadiusType m_r; // radius
};

} // namespace srs

// Traits specializations for sphere
#ifndef DOXYGEN_NO_TRAITS_SPECIALIZATIONS
namespace traits
{

template <typename RadiusType>
struct tag< srs::sphere<RadiusType> >
{
    typedef srs_sphere_tag type;
};

template <typename RadiusType>
struct radius_type< srs::sphere<RadiusType> >
{
    typedef RadiusType type;
};

template <typename RadiusType, std::size_t Dimension>
struct radius_access<srs::sphere<RadiusType>, Dimension>
{
    typedef srs::sphere<RadiusType> sphere_type;

    static inline RadiusType get(sphere_type const& s)
    {
        return s.template get_radius<Dimension>();
    }

    static inline void set(sphere_type& s, RadiusType const& value)
    {
        s.template set_radius<Dimension>(value);
    }
};

} // namespace traits
#endif // DOXYGEN_NO_TRAITS_SPECIALIZATIONS


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_SRS_SPHERE_HPP
