// Boost.Geometry
// This file is manually converted from PROJ4

// This file was modified by Oracle on 2018, 2019.
// Modifications copyright (c) 2018-2019, Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// This file is converted from PROJ4, http://trac.osgeo.org/proj
// PROJ4 is originally written by Gerald Evenden (then of the USGS)
// PROJ4 is maintained by Frank Warmerdam
// This file was converted to Geometry Library by Adam Wulkiewicz

// Original copyright notice:
// Author:   Frank Warmerdam, warmerdam@pobox.com

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

#ifndef BOOST_GEOMETRY_SRS_PROJECTIONS_IMPL_PJ_APPLY_GRIDSHIFT_HPP
#define BOOST_GEOMETRY_SRS_PROJECTIONS_IMPL_PJ_APPLY_GRIDSHIFT_HPP


#include <boost/geometry/core/assert.hpp>
#include <boost/geometry/core/radian_access.hpp>

#include <boost/geometry/srs/projections/impl/adjlon.hpp>
#include <boost/geometry/srs/projections/impl/function_overloads.hpp>
#include <boost/geometry/srs/projections/impl/pj_gridlist.hpp>

#include <boost/geometry/util/range.hpp>


namespace boost { namespace geometry { namespace projections
{

namespace detail
{

// Originally implemented in nad_intr.c
template <typename CalcT>
inline void nad_intr(CalcT in_lon, CalcT in_lat,
                     CalcT & out_lon, CalcT & out_lat,
                     pj_ctable const& ct)
{
	pj_ctable::lp_t frct;
	pj_ctable::ilp_t indx;
	boost::int32_t in;

	indx.lam = int_floor(in_lon /= ct.del.lam);
	indx.phi = int_floor(in_lat /= ct.del.phi);
	frct.lam = in_lon - indx.lam;
	frct.phi = in_lat - indx.phi;
    // TODO: implement differently
	out_lon = out_lat = HUGE_VAL;
	if (indx.lam < 0) {
		if (indx.lam == -1 && frct.lam > 0.99999999999) {
			++indx.lam;
			frct.lam = 0.;
		} else
			return;
	} else if ((in = indx.lam + 1) >= ct.lim.lam) {
		if (in == ct.lim.lam && frct.lam < 1e-11) {
			--indx.lam;
			frct.lam = 1.;
		} else
			return;
	}
	if (indx.phi < 0) {
		if (indx.phi == -1 && frct.phi > 0.99999999999) {
			++indx.phi;
			frct.phi = 0.;
		} else
			return;
	} else if ((in = indx.phi + 1) >= ct.lim.phi) {
		if (in == ct.lim.phi && frct.phi < 1e-11) {
			--indx.phi;
			frct.phi = 1.;
		} else
			return;
	}
	boost::int32_t index = indx.phi * ct.lim.lam + indx.lam;
	pj_ctable::flp_t const& f00 = ct.cvs[index++];
	pj_ctable::flp_t const& f10 = ct.cvs[index];
	index += ct.lim.lam;
	pj_ctable::flp_t const& f11 = ct.cvs[index--];
	pj_ctable::flp_t const& f01 = ct.cvs[index];
    CalcT m00, m10, m01, m11;
	m11 = m10 = frct.lam;
	m00 = m01 = 1. - frct.lam;
	m11 *= frct.phi;
	m01 *= frct.phi;
	frct.phi = 1. - frct.phi;
	m00 *= frct.phi;
	m10 *= frct.phi;
	out_lon = m00 * f00.lam + m10 * f10.lam +
			  m01 * f01.lam + m11 * f11.lam;
	out_lat = m00 * f00.phi + m10 * f10.phi +
			  m01 * f01.phi + m11 * f11.phi;
}

// Originally implemented in nad_cvt.c
template <bool Inverse, typename CalcT>
inline void nad_cvt(CalcT const& in_lon, CalcT const& in_lat,
                    CalcT & out_lon, CalcT & out_lat,
                    pj_gi const& gi)
{
    static const int max_iterations = 10;
    static const CalcT tol = 1e-12;
    static const CalcT toltol = tol * tol;
    static const CalcT pi = math::pi<CalcT>();

    // horizontal grid expected
    BOOST_GEOMETRY_ASSERT_MSG(gi.format != pj_gi::gtx,
        "Vertical grid cannot be used in horizontal shift.");

    pj_ctable const& ct = gi.ct;

    // TODO: implement differently
    if (in_lon == HUGE_VAL)
    {
        out_lon = HUGE_VAL;
        out_lat = HUGE_VAL;
        return;
    }

    // normalize input to ll origin
    pj_ctable::lp_t tb;
    tb.lam = in_lon - ct.ll.lam;
    tb.phi = in_lat - ct.ll.phi;
    tb.lam = adjlon (tb.lam - pi) + pi;

    pj_ctable::lp_t t;
    nad_intr(tb.lam, tb.phi, t.lam, t.phi, ct);
    if (t.lam == HUGE_VAL)
    {
        out_lon = HUGE_VAL;
        out_lat = HUGE_VAL;
        return;
    }

    if (! Inverse)
    {
        out_lon = in_lon - t.lam;
        out_lat = in_lat - t.phi;
        return;
    }

    t.lam = tb.lam + t.lam;
    t.phi = tb.phi - t.phi;

    int i = max_iterations;
    pj_ctable::lp_t del, dif;
    do
    {
        nad_intr(t.lam, t.phi, del.lam, del.phi, ct);

        // This case used to return failure, but I have
        // changed it to return the first order approximation
        // of the inverse shift.  This avoids cases where the
        // grid shift *into* this grid came from another grid.
        // While we aren't returning optimally correct results
        // I feel a close result in this case is better than
        // no result.  NFW
        // To demonstrate use -112.5839956 49.4914451 against
        // the NTv2 grid shift file from Canada.
        if (del.lam == HUGE_VAL)
        {
            // Inverse grid shift iteration failed, presumably at grid edge. Using first approximation.
            break;
        }

        dif.lam = t.lam - del.lam - tb.lam;
        dif.phi = t.phi + del.phi - tb.phi;
        t.lam -= dif.lam;
        t.phi -= dif.phi;

    }
    while (--i && (dif.lam*dif.lam + dif.phi*dif.phi > toltol)); // prob. slightly faster than hypot()

    if (i==0)
    {
        // Inverse grid shift iterator failed to converge.
        out_lon = HUGE_VAL;
        out_lat = HUGE_VAL;
        return;
    }

    out_lon = adjlon (t.lam + ct.ll.lam);
    out_lat = t.phi + ct.ll.phi;
}


/************************************************************************/
/*                             find_grid()                              */
/*                                                                      */
/*    Determine which grid is the correct given an input coordinate.    */
/************************************************************************/

// Originally find_ctable()
// here divided into grid_disjoint(), find_grid() and load_grid()

template <typename T>
inline bool grid_disjoint(T const& lam, T const& phi,
                          pj_ctable const& ct)
{
    double epsilon = (fabs(ct.del.phi)+fabs(ct.del.lam))/10000.0;
    return ct.ll.phi - epsilon > phi
        || ct.ll.lam - epsilon > lam
        || (ct.ll.phi + (ct.lim.phi-1) * ct.del.phi + epsilon < phi)
        || (ct.ll.lam + (ct.lim.lam-1) * ct.del.lam + epsilon < lam);
}

template <typename T>
inline pj_gi * find_grid(T const& lam,
                         T const& phi,
                         std::vector<pj_gi>::iterator first,
                         std::vector<pj_gi>::iterator last)
{
    pj_gi * gip = NULL;

    for( ; first != last ; ++first )
    {
        // skip tables that don't match our point at all.
        if (! grid_disjoint(lam, phi, first->ct))
        {
            // skip vertical grids
            if (first->format != pj_gi::gtx)
            {
                gip = boost::addressof(*first);
                break;
            }
        }
    }

    // If we didn't find a child then nothing more to do
    if( gip == NULL )
        return gip;

    // Otherwise use the child, first checking it's children
    pj_gi * child = find_grid(lam, phi, first->children.begin(), first->children.end());
    if (child != NULL)
        gip = child;

    return gip;
}

template <typename T>
inline pj_gi * find_grid(T const& lam,
                         T const& phi,
                         pj_gridinfo & grids,
                         std::vector<std::size_t> const& gridindexes)
{
    pj_gi * gip = NULL;

    // keep trying till we find a table that works
    for (std::size_t i = 0 ; i < gridindexes.size() ; ++i)
    {
        pj_gi & gi = grids[gridindexes[i]];

        // skip tables that don't match our point at all.
        if (! grid_disjoint(lam, phi, gi.ct))
        {
            // skip vertical grids
            if (gi.format != pj_gi::gtx)
            {
                gip = boost::addressof(gi);
                break;
            }
        }
    }

    if (gip == NULL)
        return gip;

    // If we have child nodes, check to see if any of them apply.
    pj_gi * child = find_grid(lam, phi, gip->children.begin(), gip->children.end());
    if (child != NULL)
        gip = child;

    // if we get this far we have found a suitable grid
    return gip;
}


template <typename StreamPolicy>
inline bool load_grid(StreamPolicy const& stream_policy, pj_gi_load & gi)
{
    // load the grid shift info if we don't have it.
    if (gi.ct.cvs.empty())
    {
        typename StreamPolicy::stream_type is;
        stream_policy.open(is, gi.gridname);

        if (! pj_gridinfo_load(is, gi))
        {
            //pj_ctx_set_errno( ctx, PJD_ERR_FAILED_TO_LOAD_GRID );
            return false;
        }
    }

    return true;
}


/************************************************************************/
/*                        pj_apply_gridshift_3()                        */
/*                                                                      */
/*      This is the real workhorse, given a gridlist.                   */
/************************************************************************/

// Generic stream policy and standard grids
template <bool Inverse, typename CalcT, typename StreamPolicy, typename Range, typename Grids>
inline bool pj_apply_gridshift_3(StreamPolicy const& stream_policy,
                                 Range & range,
                                 Grids & grids,
                                 std::vector<std::size_t> const& gridindexes,
                                 grids_tag)
{
    typedef typename boost::range_size<Range>::type size_type;
    
    // If the grids are empty the indexes are as well
    if (gridindexes.empty())
    {
        //pj_ctx_set_errno(ctx, PJD_ERR_FAILED_TO_LOAD_GRID);
        //return PJD_ERR_FAILED_TO_LOAD_GRID;
        return false;
    }

    size_type point_count = boost::size(range);

    for (size_type i = 0 ; i < point_count ; ++i)
    {
        typename boost::range_reference<Range>::type
            point = range::at(range, i);

        CalcT in_lon = geometry::get_as_radian<0>(point);
        CalcT in_lat = geometry::get_as_radian<1>(point);
        
        pj_gi * gip = find_grid(in_lon, in_lat, grids.gridinfo, gridindexes);

        if ( gip != NULL )
        {
            // load the grid shift info if we don't have it.
            if (! gip->ct.cvs.empty() || load_grid(stream_policy, *gip))
            {
                // TODO: use set_invalid_point() or similar mechanism
                CalcT out_lon = HUGE_VAL;
                CalcT out_lat = HUGE_VAL;

                nad_cvt<Inverse>(in_lon, in_lat, out_lon, out_lat, *gip);
            
                // TODO: check differently
                if ( out_lon != HUGE_VAL )
                {
                    geometry::set_from_radian<0>(point, out_lon);
                    geometry::set_from_radian<1>(point, out_lat);
                }
            }
        }
    }

    return true;
}

// Generic stream policy and shared grids
template <bool Inverse, typename CalcT, typename StreamPolicy, typename Range, typename SharedGrids>
inline bool pj_apply_gridshift_3(StreamPolicy const& stream_policy,
                                 Range & range,
                                 SharedGrids & grids,
                                 std::vector<std::size_t> const& gridindexes,
                                 shared_grids_tag)
{
    typedef typename boost::range_size<Range>::type size_type;
    
    // If the grids are empty the indexes are as well
    if (gridindexes.empty())
    {
        //pj_ctx_set_errno(ctx, PJD_ERR_FAILED_TO_LOAD_GRID);
        //return PJD_ERR_FAILED_TO_LOAD_GRID;
        return false;
    }

    size_type point_count = boost::size(range);

    // local storage
    pj_gi_load local_gi;

    for (size_type i = 0 ; i < point_count ; )
    {
        bool load_needed = false;

        CalcT in_lon = 0;
        CalcT in_lat = 0;

        {
            typename SharedGrids::read_locked lck_grids(grids);

            for ( ; i < point_count ; ++i )
            {
                typename boost::range_reference<Range>::type
                    point = range::at(range, i);

                in_lon = geometry::get_as_radian<0>(point);
                in_lat = geometry::get_as_radian<1>(point);

                pj_gi * gip = find_grid(in_lon, in_lat, lck_grids.gridinfo, gridindexes);

                if (gip == NULL)
                {
                    // do nothing
                }
                else if (! gip->ct.cvs.empty())
                {
                    // TODO: use set_invalid_point() or similar mechanism
                    CalcT out_lon = HUGE_VAL;
                    CalcT out_lat = HUGE_VAL;

                    nad_cvt<Inverse>(in_lon, in_lat, out_lon, out_lat, *gip);

                    // TODO: check differently
                    if (out_lon != HUGE_VAL)
                    {
                        geometry::set_from_radian<0>(point, out_lon);
                        geometry::set_from_radian<1>(point, out_lat);
                    }
                }
                else
                {
                    // loading is needed
                    local_gi = *gip;
                    load_needed = true;
                    break;
                }
            }
        }

        if (load_needed)
        {
            if (load_grid(stream_policy, local_gi))
            {
                typename SharedGrids::write_locked lck_grids(grids);

                // check again in case other thread already loaded the grid.
                pj_gi * gip = find_grid(in_lon, in_lat, lck_grids.gridinfo, gridindexes);

                if (gip != NULL && gip->ct.cvs.empty())
                {
                    // swap loaded local storage with empty grid
                    local_gi.swap(*gip);
                }
            }
            else
            {
                ++i;
            }
        }
    }

    return true;
}


/************************************************************************/
/*                        pj_apply_gridshift_2()                        */
/*                                                                      */
/*      This implementation uses the gridlist from a coordinate         */
/*      system definition.  If the gridlist has not yet been            */
/*      populated in the coordinate system definition we set it up      */
/*      now.                                                            */
/************************************************************************/

template <bool Inverse, typename Par, typename Range, typename ProjGrids>
inline bool pj_apply_gridshift_2(Par const& /*defn*/, Range & range, ProjGrids const& proj_grids)
{
    /*if( defn->catalog_name != NULL )
        return pj_gc_apply_gridshift( defn, inverse, point_count, point_offset,
                                      x, y, z );*/

    /*std::vector<std::size_t> gridindexes;
    pj_gridlist_from_nadgrids(pj_get_param_s(defn.params, "nadgrids"),
                              grids.storage_ptr->stream_policy,
                              grids.storage_ptr->grids,
                              gridindexes);*/

    // At this point the grids should be initialized
    if (proj_grids.hindexes.empty())
        return false;

    return pj_apply_gridshift_3
            <
                Inverse, typename Par::type
            >(proj_grids.grids_storage().stream_policy,
              range,
              proj_grids.grids_storage().hgrids,
              proj_grids.hindexes,
              typename ProjGrids::grids_storage_type::grids_type::tag());
}

template <bool Inverse, typename Par, typename Range>
inline bool pj_apply_gridshift_2(Par const& , Range & , srs::detail::empty_projection_grids const& )
{
    return false;
}


} // namespace detail

}}} // namespace boost::geometry::projections

#endif // BOOST_GEOMETRY_SRS_PROJECTIONS_IMPL_PJ_APPLY_GRIDSHIFT_HPP
