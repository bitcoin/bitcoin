// Boost.Geometry
// This file is manually converted from PROJ4

// This file was modified by Oracle on 2017, 2018.
// Modifications copyright (c) 2017-2018, Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// This file is converted from PROJ4, http://trac.osgeo.org/proj
// PROJ4 is originally written by Gerald Evenden (then of the USGS)
// PROJ4 is maintained by Frank Warmerdam
// This file was converted to Geometry Library by Adam Wulkiewicz

// Original copyright notice:

// Copyright (c) 2000, Frank Warmerdam

// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#ifndef BOOST_GEOMETRY_SRS_PROJECTIONS_IMPL_PJ_TRANSFORM_HPP
#define BOOST_GEOMETRY_SRS_PROJECTIONS_IMPL_PJ_TRANSFORM_HPP


#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/coordinate_dimension.hpp>
#include <boost/geometry/core/radian_access.hpp>

#include <boost/geometry/srs/projections/impl/geocent.hpp>
#include <boost/geometry/srs/projections/impl/pj_apply_gridshift.hpp>
#include <boost/geometry/srs/projections/impl/projects.hpp>
#include <boost/geometry/srs/projections/invalid_point.hpp>

#include <boost/geometry/util/range.hpp>

#include <cstring>
#include <cmath>


namespace boost { namespace geometry { namespace projections
{

namespace detail
{

// -----------------------------------------------------------
// Boost.Geometry helpers begin
// -----------------------------------------------------------

template
<
    typename Point,
    bool HasCoord2 = (dimension<Point>::value > 2)
>
struct z_access
{
    typedef typename coordinate_type<Point>::type type;
    static inline type get(Point const& point)
    {
        return geometry::get<2>(point);
    }
    static inline void set(Point & point, type const& h)
    {
        return geometry::set<2>(point, h);
    }
};

template <typename Point>
struct z_access<Point, false>
{
    typedef typename coordinate_type<Point>::type type;
    static inline type get(Point const& )
    {
        return type(0);
    }
    static inline void set(Point & , type const& )
    {}
};

template <typename XYorXYZ>
inline typename z_access<XYorXYZ>::type
get_z(XYorXYZ const& xy_or_xyz)
{
    return z_access<XYorXYZ>::get(xy_or_xyz);
}

template <typename XYorXYZ>
inline void set_z(XYorXYZ & xy_or_xyz,
                  typename z_access<XYorXYZ>::type const& z)
{
    return z_access<XYorXYZ>::set(xy_or_xyz, z);
}

template
<
    typename Range,
    bool AddZ = (dimension<typename boost::range_value<Range>::type>::value < 3)
>
struct range_wrapper
{
    typedef Range range_type;
    typedef typename boost::range_value<Range>::type point_type;
    typedef typename coordinate_type<point_type>::type coord_t;

    range_wrapper(Range & range)
        : m_range(range)
    {}

    range_type & get_range() { return m_range; }

    coord_t get_z(std::size_t i) { return detail::get_z(range::at(m_range, i)); }
    void set_z(std::size_t i, coord_t const& v) { return detail::set_z(range::at(m_range, i), v); }

private:
    Range & m_range;
};

template <typename Range>
struct range_wrapper<Range, true>
{
    typedef Range range_type;
    typedef typename boost::range_value<Range>::type point_type;
    typedef typename coordinate_type<point_type>::type coord_t;

    range_wrapper(Range & range)
        : m_range(range)
        , m_zs(boost::size(range), coord_t(0))
    {}

    range_type & get_range() { return m_range; }

    coord_t get_z(std::size_t i) { return m_zs[i]; }
    void set_z(std::size_t i, coord_t const& v) { m_zs[i] = v; }

private:
    Range & m_range;
    std::vector<coord_t> m_zs;
};

// -----------------------------------------------------------
// Boost.Geometry helpers end
// -----------------------------------------------------------

template <typename Par>
inline typename Par::type Dx_BF(Par const& defn) { return defn.datum_params[0]; }
template <typename Par>
inline typename Par::type Dy_BF(Par const& defn) { return defn.datum_params[1]; }
template <typename Par>
inline typename Par::type Dz_BF(Par const& defn) { return defn.datum_params[2]; }
template <typename Par>
inline typename Par::type Rx_BF(Par const& defn) { return defn.datum_params[3]; }
template <typename Par>
inline typename Par::type Ry_BF(Par const& defn) { return defn.datum_params[4]; }
template <typename Par>
inline typename Par::type Rz_BF(Par const& defn) { return defn.datum_params[5]; }
template <typename Par>
inline typename Par::type M_BF(Par const& defn) { return defn.datum_params[6]; }

/*
** This table is intended to indicate for any given error code in
** the range 0 to -56, whether that error will occur for all locations (ie.
** it is a problem with the coordinate system as a whole) in which case the
** value would be 0, or if the problem is with the point being transformed
** in which case the value is 1.
**
** At some point we might want to move this array in with the error message
** list or something, but while experimenting with it this should be fine.
**
**
** NOTE (2017-10-01): Non-transient errors really should have resulted in a
** PJ==0 during initialization, and hence should be handled at the level
** before calling pj_transform. The only obvious example of the contrary
** appears to be the PJD_ERR_GRID_AREA case, which may also be taken to
** mean "no grids available"
**
**
*/

static const int transient_error[60] = {
    /*             0  1  2  3  4  5  6  7  8  9   */
    /* 0 to 9 */   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 10 to 19 */ 0, 0, 0, 0, 1, 1, 0, 1, 1, 1,
    /* 20 to 29 */ 1, 0, 0, 0, 0, 0, 0, 1, 0, 0,
    /* 30 to 39 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 40 to 49 */ 0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
    /* 50 to 59 */ 1, 0, 1, 0, 1, 1, 1, 1, 0, 0 };


template <typename T, typename Range>
inline int pj_geocentric_to_geodetic( T const& a, T const& es,
                                      Range & range );
template <typename T, typename Range>
inline int pj_geodetic_to_geocentric( T const& a, T const& es,
                                      Range & range );

/************************************************************************/
/*                            pj_transform()                            */
/*                                                                      */
/*      Currently this function doesn't recognise if two projections    */
/*      are identical (to short circuit reprojection) because it is     */
/*      difficult to compare PJ structures (since there are some        */
/*      projection specific components).                                */
/************************************************************************/

template <
    typename SrcPrj,
    typename DstPrj2,
    typename Par,
    typename Range,
    typename Grids
>
inline bool pj_transform(SrcPrj const& srcprj, Par const& srcdefn,
                         DstPrj2 const& dstprj, Par const& dstdefn,
                         Range & range,
                         Grids const& srcgrids,
                         Grids const& dstgrids)

{
    typedef typename boost::range_value<Range>::type point_type;
    typedef typename coordinate_type<point_type>::type coord_t;
    static const std::size_t dimension = geometry::dimension<point_type>::value;
    std::size_t point_count = boost::size(range);
    bool result = true;

/* -------------------------------------------------------------------- */
/*      Transform unusual input coordinate axis orientation to          */
/*      standard form if needed.                                        */
/* -------------------------------------------------------------------- */
    // Ignored

/* -------------------------------------------------------------------- */
/*      Transform Z to meters if it isn't already.                      */
/* -------------------------------------------------------------------- */
    if( srcdefn.vto_meter != 1.0 && dimension > 2 )
    {
        for( std::size_t i = 0; i < point_count; i++ )
        {
            point_type & point = geometry::range::at(range, i);
            set_z(point, get_z(point) * srcdefn.vto_meter);
        }
    }

/* -------------------------------------------------------------------- */
/*      Transform geocentric source coordinates to lat/long.            */
/* -------------------------------------------------------------------- */
    if( srcdefn.is_geocent )
    {
        // Point should be cartesian 3D (ECEF)
        if (dimension < 3)
            BOOST_THROW_EXCEPTION( projection_exception(error_geocentric) );
            //return PJD_ERR_GEOCENTRIC;

        if( srcdefn.to_meter != 1.0 )
        {
            for(std::size_t i = 0; i < point_count; i++ )
            {
                point_type & point = range::at(range, i);
                if( ! is_invalid_point(point) )
                {
                    set<0>(point, get<0>(point) * srcdefn.to_meter);
                    set<1>(point, get<1>(point) * srcdefn.to_meter);
                }
            }
        }

        range_wrapper<Range, false> rng(range);
        int err = pj_geocentric_to_geodetic( srcdefn.a_orig, srcdefn.es_orig,
                                             rng );
        if( err != 0 )
            BOOST_THROW_EXCEPTION( projection_exception(err) );
            //return err;

        // NOTE: here 3D cartesian ECEF is converted into 3D geodetic LLH
    }

/* -------------------------------------------------------------------- */
/*      Transform source points to lat/long, if they aren't             */
/*      already.                                                        */
/* -------------------------------------------------------------------- */
    else if( !srcdefn.is_latlong )
    {
        // Point should be cartesian 2D or 3D (map projection)

        /* Check first if projection is invertible. */
        /*if( (srcdefn->inv3d == NULL) && (srcdefn->inv == NULL))
        {
            pj_ctx_set_errno( pj_get_ctx(srcdefn), -17 );
            pj_log( pj_get_ctx(srcdefn), PJ_LOG_ERROR,
                    "pj_transform(): source projection not invertible" );
            return -17;
        }*/

        /* If invertible - First try inv3d if defined */
        //if (srcdefn->inv3d != NULL)
        //{
        //    /* Three dimensions must be defined */
        //    if ( z == NULL)
        //    {
        //        pj_ctx_set_errno( pj_get_ctx(srcdefn), PJD_ERR_GEOCENTRIC);
        //        return PJD_ERR_GEOCENTRIC;
        //    }

        //    for (i=0; i < point_count; i++)
        //    {
        //        XYZ projected_loc;
        //        XYZ geodetic_loc;

        //        projected_loc.u = x[point_offset*i];
        //        projected_loc.v = y[point_offset*i];
        //        projected_loc.w = z[point_offset*i];

        //        if (projected_loc.u == HUGE_VAL)
        //            continue;

        //        geodetic_loc = pj_inv3d(projected_loc, srcdefn);
        //        if( srcdefn->ctx->last_errno != 0 )
        //        {
        //            if( (srcdefn->ctx->last_errno != 33 /*EDOM*/
        //                 && srcdefn->ctx->last_errno != 34 /*ERANGE*/ )
        //                && (srcdefn->ctx->last_errno > 0
        //                    || srcdefn->ctx->last_errno < -44 || point_count == 1
        //                    || transient_error[-srcdefn->ctx->last_errno] == 0 ) )
        //                return srcdefn->ctx->last_errno;
        //            else
        //            {
        //                geodetic_loc.u = HUGE_VAL;
        //                geodetic_loc.v = HUGE_VAL;
        //                geodetic_loc.w = HUGE_VAL;
        //            }
        //        }

        //        x[point_offset*i] = geodetic_loc.u;
        //        y[point_offset*i] = geodetic_loc.v;
        //        z[point_offset*i] = geodetic_loc.w;

        //    }

        //}
        //else
        {
            /* Fallback to the original PROJ.4 API 2d inversion - inv */
            for( std::size_t i = 0; i < point_count; i++ )
            {
                point_type & point = range::at(range, i);

                if( is_invalid_point(point) )
                    continue;

                try
                {
                    pj_inv(srcprj, srcdefn, point, point);
                }
                catch(projection_exception const& e)
                {
                    if( (e.code() != 33 /*EDOM*/
                        && e.code() != 34 /*ERANGE*/ )
                        && (e.code() > 0
                            || e.code() < -44 /*|| point_count == 1*/
                            || transient_error[-e.code()] == 0) ) {
                        BOOST_RETHROW
                    } else {
                        set_invalid_point(point);
                        result = false;
                        if (point_count == 1)
                            return result;
                    }
                }
            }
        }
    }

/* -------------------------------------------------------------------- */
/*      But if they are already lat long, adjust for the prime          */
/*      meridian if there is one in effect.                             */
/* -------------------------------------------------------------------- */
    if( srcdefn.from_greenwich != 0.0 )
    {
        for( std::size_t i = 0; i < point_count; i++ )
        {
            point_type & point = range::at(range, i);

            if( ! is_invalid_point(point) )
                set<0>(point, get<0>(point) + srcdefn.from_greenwich);
        }
    }

/* -------------------------------------------------------------------- */
/*      Do we need to translate from geoid to ellipsoidal vertical      */
/*      datum?                                                          */
/* -------------------------------------------------------------------- */
    /*if( srcdefn->has_geoid_vgrids && z != NULL )
    {
        if( pj_apply_vgridshift( srcdefn, "sgeoidgrids",
                                 &(srcdefn->vgridlist_geoid),
                                 &(srcdefn->vgridlist_geoid_count),
                                 0, point_count, point_offset, x, y, z ) != 0 )
            return pj_ctx_get_errno(srcdefn->ctx);
    }*/

/* -------------------------------------------------------------------- */
/*      Convert datums if needed, and possible.                         */
/* -------------------------------------------------------------------- */
    if ( ! pj_datum_transform( srcdefn, dstdefn, range, srcgrids, dstgrids ) )
    {
        result = false;
    }

/* -------------------------------------------------------------------- */
/*      Do we need to translate from ellipsoidal to geoid vertical      */
/*      datum?                                                          */
/* -------------------------------------------------------------------- */
    /*if( dstdefn->has_geoid_vgrids && z != NULL )
    {
        if( pj_apply_vgridshift( dstdefn, "sgeoidgrids",
                                 &(dstdefn->vgridlist_geoid),
                                 &(dstdefn->vgridlist_geoid_count),
                                 1, point_count, point_offset, x, y, z ) != 0 )
            return dstdefn->ctx->last_errno;
    }*/

/* -------------------------------------------------------------------- */
/*      But if they are staying lat long, adjust for the prime          */
/*      meridian if there is one in effect.                             */
/* -------------------------------------------------------------------- */
    if( dstdefn.from_greenwich != 0.0 )
    {
        for( std::size_t i = 0; i < point_count; i++ )
        {
            point_type & point = range::at(range, i);

            if( ! is_invalid_point(point) )
                set<0>(point, get<0>(point) - dstdefn.from_greenwich);
        }
    }

/* -------------------------------------------------------------------- */
/*      Transform destination latlong to geocentric if required.        */
/* -------------------------------------------------------------------- */
    if( dstdefn.is_geocent )
    {
        // Point should be cartesian 3D (ECEF)
        if (dimension < 3)
            BOOST_THROW_EXCEPTION( projection_exception(error_geocentric) );
            //return PJD_ERR_GEOCENTRIC;

        // NOTE: In the original code the return value of the following
        // function is not checked
        range_wrapper<Range, false> rng(range);
        int err = pj_geodetic_to_geocentric( dstdefn.a_orig, dstdefn.es_orig,
                                             rng );
        if( err == -14 )
            result = false;
        else
            BOOST_THROW_EXCEPTION( projection_exception(err) );
            
        if( dstdefn.fr_meter != 1.0 )
        {
            for( std::size_t i = 0; i < point_count; i++ )
            {
                point_type & point = range::at(range, i);
                if( ! is_invalid_point(point) )
                {
                    set<0>(point, get<0>(point) * dstdefn.fr_meter);
                    set<1>(point, get<1>(point) * dstdefn.fr_meter);
                }
            }
        }
    }

/* -------------------------------------------------------------------- */
/*      Transform destination points to projection coordinates, if      */
/*      desired.                                                        */
/* -------------------------------------------------------------------- */
    else if( !dstdefn.is_latlong )
    {

        //if( dstdefn->fwd3d != NULL)
        //{
        //    for( i = 0; i < point_count; i++ )
        //    {
        //        XYZ projected_loc;
        //        LPZ geodetic_loc;

        //        geodetic_loc.u = x[point_offset*i];
        //        geodetic_loc.v = y[point_offset*i];
        //        geodetic_loc.w = z[point_offset*i];

        //        if (geodetic_loc.u == HUGE_VAL)
        //            continue;

        //        projected_loc = pj_fwd3d( geodetic_loc, dstdefn);
        //        if( dstdefn->ctx->last_errno != 0 )
        //        {
        //            if( (dstdefn->ctx->last_errno != 33 /*EDOM*/
        //                 && dstdefn->ctx->last_errno != 34 /*ERANGE*/ )
        //                && (dstdefn->ctx->last_errno > 0
        //                    || dstdefn->ctx->last_errno < -44 || point_count == 1
        //                    || transient_error[-dstdefn->ctx->last_errno] == 0 ) )
        //                return dstdefn->ctx->last_errno;
        //            else
        //            {
        //                projected_loc.u = HUGE_VAL;
        //                projected_loc.v = HUGE_VAL;
        //                projected_loc.w = HUGE_VAL;
        //            }
        //        }

        //        x[point_offset*i] = projected_loc.u;
        //        y[point_offset*i] = projected_loc.v;
        //        z[point_offset*i] = projected_loc.w;
        //    }

        //}
        //else
        {
            for(std::size_t i = 0; i < point_count; i++ )
            {
                point_type & point = range::at(range, i);

                if( is_invalid_point(point) )
                    continue;

                try {
                    pj_fwd(dstprj, dstdefn, point, point);
                } catch (projection_exception const& e) {

                    if( (e.code() != 33 /*EDOM*/
                         && e.code() != 34 /*ERANGE*/ )
                        && (e.code() > 0
                            || e.code() < -44 /*|| point_count == 1*/
                            || transient_error[-e.code()] == 0) ) {
                        BOOST_RETHROW
                    } else {
                        set_invalid_point(point);
                        result = false;
                        if (point_count == 1)
                            return result;
                    }
                }
            }
        }
    }

/* -------------------------------------------------------------------- */
/*      If a wrapping center other than 0 is provided, rewrap around    */
/*      the suggested center (for latlong coordinate systems only).     */
/* -------------------------------------------------------------------- */
    else if( dstdefn.is_latlong && dstdefn.is_long_wrap_set )
    {
        for( std::size_t i = 0; i < point_count; i++ )
        {
            point_type & point = range::at(range, i);
            coord_t x = get_as_radian<0>(point);
            
            if( is_invalid_point(point) )
                continue;

            // TODO - units-dependant constants could be used instead
            while( x < dstdefn.long_wrap_center - math::pi<coord_t>() )
                x += math::two_pi<coord_t>();
            while( x > dstdefn.long_wrap_center + math::pi<coord_t>() )
                x -= math::two_pi<coord_t>();

            set_from_radian<0>(point, x);
        }
    }

/* -------------------------------------------------------------------- */
/*      Transform Z from meters if needed.                              */
/* -------------------------------------------------------------------- */
    if( dstdefn.vto_meter != 1.0 && dimension > 2 )
    {
        for( std::size_t i = 0; i < point_count; i++ )
        {
            point_type & point = geometry::range::at(range, i);
            set_z(point, get_z(point) * dstdefn.vfr_meter);
        }
    }

/* -------------------------------------------------------------------- */
/*      Transform normalized axes into unusual output coordinate axis   */
/*      orientation if needed.                                          */
/* -------------------------------------------------------------------- */
    // Ignored

    return result;
}

/************************************************************************/
/*                     pj_geodetic_to_geocentric()                      */
/************************************************************************/

template <typename T, typename Range, bool AddZ>
inline int pj_geodetic_to_geocentric( T const& a, T const& es,
                                      range_wrapper<Range, AddZ> & range_wrapper )

{
    //typedef typename boost::range_iterator<Range>::type iterator;
    typedef typename boost::range_value<Range>::type point_type;
    //typedef typename coordinate_type<point_type>::type coord_t;

    Range & rng = range_wrapper.get_range();
    std::size_t point_count = boost::size(rng);

    int ret_errno = 0;

    T const b = (es == 0.0) ? a : a * sqrt(1-es);

    GeocentricInfo<T> gi;
    if( pj_Set_Geocentric_Parameters( gi, a, b ) != 0 )
    {
        return error_geocentric;
    }

    for( std::size_t i = 0 ; i < point_count ; ++i )
    {
        point_type & point = range::at(rng, i);

        if( is_invalid_point(point) )
            continue;

        T X = 0, Y = 0, Z = 0;
        if( pj_Convert_Geodetic_To_Geocentric( gi,
                                               get_as_radian<0>(point),
                                               get_as_radian<1>(point),
                                               range_wrapper.get_z(i), // Height
                                               X, Y, Z ) != 0 )
        {
            ret_errno = error_lat_or_lon_exceed_limit;
            set_invalid_point(point);
            /* but keep processing points! */
        }
        else
        {
            set<0>(point, X);
            set<1>(point, Y);
            range_wrapper.set_z(i, Z);
        }
    }

    return ret_errno;
}

/************************************************************************/
/*                     pj_geodetic_to_geocentric()                      */
/************************************************************************/

template <typename T, typename Range, bool AddZ>
inline int pj_geocentric_to_geodetic( T const& a, T const& es,
                                      range_wrapper<Range, AddZ> & range_wrapper )

{
    //typedef typename boost::range_iterator<Range>::type iterator;
    typedef typename boost::range_value<Range>::type point_type;
    //typedef typename coordinate_type<point_type>::type coord_t;

    Range & rng = range_wrapper.get_range();
    std::size_t point_count = boost::size(rng);

    T const b = (es == 0.0) ? a : a * sqrt(1-es);

    GeocentricInfo<T> gi;
    if( pj_Set_Geocentric_Parameters( gi, a, b ) != 0 )
    {
        return error_geocentric;
    }

    for( std::size_t i = 0 ; i < point_count ; ++i )
    {
        point_type & point = range::at(rng, i);

        if( is_invalid_point(point) )
            continue;

        T Longitude = 0, Latitude = 0, Height = 0;
        pj_Convert_Geocentric_To_Geodetic( gi,
                                           get<0>(point),
                                           get<1>(point),
                                           range_wrapper.get_z(i), // z
                                           Longitude, Latitude, Height );

        set_from_radian<0>(point, Longitude);
        set_from_radian<1>(point, Latitude);
        range_wrapper.set_z(i, Height); // Height
    }

    return 0;
}

/************************************************************************/
/*                         pj_compare_datums()                          */
/*                                                                      */
/*      Returns TRUE if the two datums are identical, otherwise         */
/*      FALSE.                                                          */
/************************************************************************/

template <typename Par>
inline bool pj_compare_datums( Par & srcdefn, Par & dstdefn )
{
    if( srcdefn.datum_type != dstdefn.datum_type )
    {
        return false;
    }
    else if( srcdefn.a_orig != dstdefn.a_orig
             || math::abs(srcdefn.es_orig - dstdefn.es_orig) > 0.000000000050 )
    {
        /* the tolerance for es is to ensure that GRS80 and WGS84 are
           considered identical */
        return false;
    }
    else if( srcdefn.datum_type == datum_3param )
    {
        return (srcdefn.datum_params[0] == dstdefn.datum_params[0]
                && srcdefn.datum_params[1] == dstdefn.datum_params[1]
                && srcdefn.datum_params[2] == dstdefn.datum_params[2]);
    }
    else if( srcdefn.datum_type == datum_7param )
    {
        return (srcdefn.datum_params[0] == dstdefn.datum_params[0]
                && srcdefn.datum_params[1] == dstdefn.datum_params[1]
                && srcdefn.datum_params[2] == dstdefn.datum_params[2]
                && srcdefn.datum_params[3] == dstdefn.datum_params[3]
                && srcdefn.datum_params[4] == dstdefn.datum_params[4]
                && srcdefn.datum_params[5] == dstdefn.datum_params[5]
                && srcdefn.datum_params[6] == dstdefn.datum_params[6]);
    }
    else if( srcdefn.datum_type == datum_gridshift )
    {
        return srcdefn.nadgrids == dstdefn.nadgrids;
    }
    else
        return true;
}

/************************************************************************/
/*                       pj_geocentic_to_wgs84()                        */
/************************************************************************/

template <typename Par, typename Range, bool AddZ>
inline int pj_geocentric_to_wgs84( Par const& defn,
                                   range_wrapper<Range, AddZ> & range_wrapper )

{
    typedef typename boost::range_value<Range>::type point_type;
    typedef typename coordinate_type<point_type>::type coord_t;

    Range & rng = range_wrapper.get_range();
    std::size_t point_count = boost::size(rng);

    if( defn.datum_type == datum_3param )
    {
        for(std::size_t i = 0; i < point_count; i++ )
        {
            point_type & point = range::at(rng, i);
            
            if( is_invalid_point(point) )
                continue;

            set<0>(point,                   get<0>(point) + Dx_BF(defn));
            set<1>(point,                   get<1>(point) + Dy_BF(defn));
            range_wrapper.set_z(i, range_wrapper.get_z(i) + Dz_BF(defn));
        }
    }
    else if( defn.datum_type == datum_7param )
    {
        for(std::size_t i = 0; i < point_count; i++ )
        {
            point_type & point = range::at(rng, i);

            if( is_invalid_point(point) )
                continue;

            coord_t x = get<0>(point);
            coord_t y = get<1>(point);
            coord_t z = range_wrapper.get_z(i);

            coord_t x_out, y_out, z_out;

            x_out = M_BF(defn)*(             x - Rz_BF(defn)*y + Ry_BF(defn)*z) + Dx_BF(defn);
            y_out = M_BF(defn)*( Rz_BF(defn)*x +             y - Rx_BF(defn)*z) + Dy_BF(defn);
            z_out = M_BF(defn)*(-Ry_BF(defn)*x + Rx_BF(defn)*y +             z) + Dz_BF(defn);

            set<0>(point, x_out);
            set<1>(point, y_out);
            range_wrapper.set_z(i, z_out);
        }
    }

    return 0;
}

/************************************************************************/
/*                      pj_geocentic_from_wgs84()                       */
/************************************************************************/

template <typename Par, typename Range, bool AddZ>
inline int pj_geocentric_from_wgs84( Par const& defn,
                                     range_wrapper<Range, AddZ> & range_wrapper )

{
    typedef typename boost::range_value<Range>::type point_type;
    typedef typename coordinate_type<point_type>::type coord_t;

    Range & rng = range_wrapper.get_range();
    std::size_t point_count = boost::size(rng);

    if( defn.datum_type == datum_3param )
    {
        for(std::size_t i = 0; i < point_count; i++ )
        {
            point_type & point = range::at(rng, i);

            if( is_invalid_point(point) )
                continue;

            set<0>(point,                   get<0>(point) - Dx_BF(defn));
            set<1>(point,                   get<1>(point) - Dy_BF(defn));
            range_wrapper.set_z(i, range_wrapper.get_z(i) - Dz_BF(defn));
        }
    }
    else if( defn.datum_type == datum_7param )
    {
        for(std::size_t i = 0; i < point_count; i++ )
        {
            point_type & point = range::at(rng, i);

            if( is_invalid_point(point) )
                continue;

            coord_t x = get<0>(point);
            coord_t y = get<1>(point);
            coord_t z = range_wrapper.get_z(i);

            coord_t x_tmp = (x - Dx_BF(defn)) / M_BF(defn);
            coord_t y_tmp = (y - Dy_BF(defn)) / M_BF(defn);
            coord_t z_tmp = (z - Dz_BF(defn)) / M_BF(defn);

            x =              x_tmp + Rz_BF(defn)*y_tmp - Ry_BF(defn)*z_tmp;
            y = -Rz_BF(defn)*x_tmp +             y_tmp + Rx_BF(defn)*z_tmp;
            z =  Ry_BF(defn)*x_tmp - Rx_BF(defn)*y_tmp +             z_tmp;

            set<0>(point, x);
            set<1>(point, y);
            range_wrapper.set_z(i, z);
        }
    }

    return 0;
}


inline bool pj_datum_check_error(int err)
{
    return err != 0 && (err > 0 || transient_error[-err] == 0);
}

/************************************************************************/
/*                         pj_datum_transform()                         */
/*                                                                      */
/*      The input should be long/lat/z coordinates in radians in the    */
/*      source datum, and the output should be long/lat/z               */
/*      coordinates in radians in the destination datum.                */
/************************************************************************/

template <typename Par, typename Range, typename Grids>
inline bool pj_datum_transform(Par const& srcdefn,
                               Par const& dstdefn,
                               Range & range,
                               Grids const& srcgrids,
                               Grids const& dstgrids)

{
    typedef typename Par::type calc_t;

    // This has to be consistent with default spheroid and pj_ellps
    // TODO: Define in one place
    static const calc_t wgs84_a = 6378137.0;
    static const calc_t wgs84_b = 6356752.3142451793;
    static const calc_t wgs84_es = 1. - (wgs84_b * wgs84_b) / (wgs84_a * wgs84_a);

    bool result = true;

    calc_t      src_a, src_es, dst_a, dst_es;

/* -------------------------------------------------------------------- */
/*      We cannot do any meaningful datum transformation if either      */
/*      the source or destination are of an unknown datum type          */
/*      (ie. only a +ellps declaration, no +datum).  This is new        */
/*      behavior for PROJ 4.6.0.                                        */
/* -------------------------------------------------------------------- */
    if( srcdefn.datum_type == datum_unknown
        || dstdefn.datum_type == datum_unknown )
        return result;

/* -------------------------------------------------------------------- */
/*      Short cut if the datums are identical.                          */
/* -------------------------------------------------------------------- */
    if( pj_compare_datums( srcdefn, dstdefn ) )
        return result;

    src_a = srcdefn.a_orig;
    src_es = srcdefn.es_orig;

    dst_a = dstdefn.a_orig;
    dst_es = dstdefn.es_orig;

/* -------------------------------------------------------------------- */
/*      Create a temporary Z array if one is not provided.              */
/* -------------------------------------------------------------------- */
    
    range_wrapper<Range> z_range(range);

/* -------------------------------------------------------------------- */
/*      If this datum requires grid shifts, then apply it to geodetic   */
/*      coordinates.                                                    */
/* -------------------------------------------------------------------- */
    if( srcdefn.datum_type == datum_gridshift )
    {
        try {
            pj_apply_gridshift_2<false>( srcdefn, range, srcgrids );
        } catch (projection_exception const& e) {
            if (pj_datum_check_error(e.code())) {
                BOOST_RETHROW
            }
        }

        src_a = wgs84_a;
        src_es = wgs84_es;
    }

    if( dstdefn.datum_type == datum_gridshift )
    {
        dst_a = wgs84_a;
        dst_es = wgs84_es;
    }

/* ==================================================================== */
/*      Do we need to go through geocentric coordinates?                */
/* ==================================================================== */
    if( src_es != dst_es || src_a != dst_a
        || srcdefn.datum_type == datum_3param
        || srcdefn.datum_type == datum_7param
        || dstdefn.datum_type == datum_3param
        || dstdefn.datum_type == datum_7param)
    {
/* -------------------------------------------------------------------- */
/*      Convert to geocentric coordinates.                              */
/* -------------------------------------------------------------------- */
        int err = pj_geodetic_to_geocentric( src_a, src_es, z_range );
        if (pj_datum_check_error(err))
            BOOST_THROW_EXCEPTION( projection_exception(err) );
        else if (err != 0)
            result = false;

/* -------------------------------------------------------------------- */
/*      Convert between datums.                                         */
/* -------------------------------------------------------------------- */
        if( srcdefn.datum_type == datum_3param
            || srcdefn.datum_type == datum_7param )
        {
            try {
                pj_geocentric_to_wgs84( srcdefn, z_range );
            } catch (projection_exception const& e) {
                if (pj_datum_check_error(e.code())) {
                    BOOST_RETHROW
                }
            }
        }

        if( dstdefn.datum_type == datum_3param
            || dstdefn.datum_type == datum_7param )
        {
            try {
                pj_geocentric_from_wgs84( dstdefn, z_range );
            } catch (projection_exception const& e) {
                if (pj_datum_check_error(e.code())) {
                    BOOST_RETHROW
                }
            }
        }

/* -------------------------------------------------------------------- */
/*      Convert back to geodetic coordinates.                           */
/* -------------------------------------------------------------------- */
        err = pj_geocentric_to_geodetic( dst_a, dst_es, z_range );
        if (pj_datum_check_error(err))
            BOOST_THROW_EXCEPTION( projection_exception(err) );
        else if (err != 0)
            result = false;
    }

/* -------------------------------------------------------------------- */
/*      Apply grid shift to destination if required.                    */
/* -------------------------------------------------------------------- */
    if( dstdefn.datum_type == datum_gridshift )
    {
        try {
            pj_apply_gridshift_2<true>( dstdefn, range, dstgrids );
        } catch (projection_exception const& e) {
            if (pj_datum_check_error(e.code()))
                BOOST_RETHROW
        }
    }

    return result;
}

} // namespace detail

}}} // namespace boost::geometry::projections

#endif // BOOST_GEOMETRY_SRS_PROJECTIONS_IMPL_PJ_TRANSFORM_HPP
