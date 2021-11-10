// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// This file was modified by Oracle on 2017-2020.
// Modifications copyright (c) 2017-2020, Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_PROJECTIONS_IMPL_BASE_DYNAMIC_HPP
#define BOOST_GEOMETRY_PROJECTIONS_IMPL_BASE_DYNAMIC_HPP

#include <string>

#include <boost/geometry/srs/projections/exception.hpp>
#include <boost/geometry/srs/projections/impl/projects.hpp>

namespace boost { namespace geometry { namespace projections
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail
{

/*!
    \brief projection virtual base class
    \details class containing virtual methods
    \ingroup projection
    \tparam CT calculation type
    \tparam P parameters type
*/
template <typename CT, typename P>
class dynamic_wrapper_b
{
public :
    dynamic_wrapper_b(P const& par)
        : m_par(par)
    {}

    virtual ~dynamic_wrapper_b() {}

    /// Forward projection using lon / lat and x / y separately
    virtual void fwd(P const& par, CT const& lp_lon, CT const& lp_lat, CT& xy_x, CT& xy_y) const = 0;

    /// Inverse projection using x / y and lon / lat
    virtual void inv(P const& par, CT const& xy_x, CT const& xy_y, CT& lp_lon, CT& lp_lat) const = 0;

    /// Forward projection, from Latitude-Longitude to Cartesian
    template <typename LL, typename XY>
    inline bool forward(LL const& lp, XY& xy) const
    {
        try
        {
            pj_fwd(*this, m_par, lp, xy);
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    /// Inverse projection, from Cartesian to Latitude-Longitude
    template <typename LL, typename XY>
    inline bool inverse(XY const& xy, LL& lp) const
    {
        try
        {
            pj_inv(*this, m_par, xy, lp);
            return true;
        }
        catch (projection_not_invertible_exception &)
        {
            BOOST_RETHROW
        }
        catch (...)
        {
            return false;
        }
    }

    /// Returns name of projection
    std::string name() const { return m_par.id.name; }

    /// Returns parameters of projection
    P const& params() const { return m_par; }

    /// Returns mutable parameters of projection
    P& mutable_params() { return m_par; }

protected:
    P m_par;
};

// Forward
template <typename Prj, typename CT, typename P>
class dynamic_wrapper_f
    : public dynamic_wrapper_b<CT, P>
    , protected Prj
{
    typedef dynamic_wrapper_b<CT, P> base_t;

public:
    template <typename Params>
    dynamic_wrapper_f(Params const& params, P const& par)
        : base_t(par)
        , Prj(params, this->m_par) // prj can modify parameters
    {}

    template <typename Params, typename P3>
    dynamic_wrapper_f(Params const& params, P const& par, P3 const& p3)
        : base_t(par)
        , Prj(params, this->m_par, p3) // prj can modify parameters
    {}

    virtual void fwd(P const& par, CT const& lp_lon, CT const& lp_lat, CT& xy_x, CT& xy_y) const
    {
        prj().fwd(par, lp_lon, lp_lat, xy_x, xy_y);
    }

    virtual void inv(P const& , CT const& , CT const& , CT& , CT& ) const
    {
        BOOST_THROW_EXCEPTION(projection_not_invertible_exception(this->name()));
    }

protected:
    Prj const& prj() const { return *this; }
};

// Forward/inverse
template <typename Prj, typename CT, typename P>
class dynamic_wrapper_fi : public dynamic_wrapper_f<Prj, CT, P>
{
    typedef dynamic_wrapper_f<Prj, CT, P> base_t;

public:
    template <typename Params>
    dynamic_wrapper_fi(Params const& params, P const& par)
        : base_t(params, par)
    {}

    template <typename Params, typename P3>
    dynamic_wrapper_fi(Params const& params, P const& par, P3 const& p3)
        : base_t(params, par, p3)
    {}
    
    virtual void inv(P const& par, CT const& xy_x, CT const& xy_y, CT& lp_lon, CT& lp_lat) const
    {
        this->prj().inv(par, xy_x, xy_y, lp_lon, lp_lat);
    }
};

} // namespace detail
#endif // DOXYGEN_NO_DETAIL

}}} // namespace boost::geometry::projections

#endif // BOOST_GEOMETRY_PROJECTIONS_IMPL_BASE_DYNAMIC_HPP
