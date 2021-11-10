// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2008-2012 Barend Gehrels, Amsterdam, the Netherlands.

// This file was modified by Oracle on 2017-2021.
// Modifications copyright (c) 2017-2021, Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_PROJECTIONS_FACTORY_HPP
#define BOOST_GEOMETRY_PROJECTIONS_FACTORY_HPP

#include <map>
#include <string>

#include <boost/shared_ptr.hpp>

#include <boost/geometry/core/static_assert.hpp>

#include <boost/geometry/srs/projections/dpar.hpp>
#include <boost/geometry/srs/projections/proj4.hpp>
#include <boost/geometry/srs/projections/impl/factory_entry.hpp>
#include <boost/geometry/srs/projections/proj/aea.hpp>
#include <boost/geometry/srs/projections/proj/aeqd.hpp>
#include <boost/geometry/srs/projections/proj/airy.hpp>
#include <boost/geometry/srs/projections/proj/aitoff.hpp>
#include <boost/geometry/srs/projections/proj/august.hpp>
#include <boost/geometry/srs/projections/proj/bacon.hpp>
#include <boost/geometry/srs/projections/proj/bipc.hpp>
#include <boost/geometry/srs/projections/proj/boggs.hpp>
#include <boost/geometry/srs/projections/proj/bonne.hpp>
#include <boost/geometry/srs/projections/proj/cass.hpp>
#include <boost/geometry/srs/projections/proj/cc.hpp>
#include <boost/geometry/srs/projections/proj/cea.hpp>
#include <boost/geometry/srs/projections/proj/chamb.hpp>
#include <boost/geometry/srs/projections/proj/collg.hpp>
#include <boost/geometry/srs/projections/proj/crast.hpp>
#include <boost/geometry/srs/projections/proj/denoy.hpp>
#include <boost/geometry/srs/projections/proj/eck1.hpp>
#include <boost/geometry/srs/projections/proj/eck2.hpp>
#include <boost/geometry/srs/projections/proj/eck3.hpp>
#include <boost/geometry/srs/projections/proj/eck4.hpp>
#include <boost/geometry/srs/projections/proj/eck5.hpp>
#include <boost/geometry/srs/projections/proj/eqc.hpp>
#include <boost/geometry/srs/projections/proj/eqdc.hpp>
#include <boost/geometry/srs/projections/proj/etmerc.hpp>
#include <boost/geometry/srs/projections/proj/fahey.hpp>
#include <boost/geometry/srs/projections/proj/fouc_s.hpp>
#include <boost/geometry/srs/projections/proj/gall.hpp>
#include <boost/geometry/srs/projections/proj/geocent.hpp>
#include <boost/geometry/srs/projections/proj/geos.hpp>
#include <boost/geometry/srs/projections/proj/gins8.hpp>
#include <boost/geometry/srs/projections/proj/gn_sinu.hpp>
#include <boost/geometry/srs/projections/proj/gnom.hpp>
#include <boost/geometry/srs/projections/proj/goode.hpp>
#include <boost/geometry/srs/projections/proj/gstmerc.hpp>
#include <boost/geometry/srs/projections/proj/hammer.hpp>
#include <boost/geometry/srs/projections/proj/hatano.hpp>
#include <boost/geometry/srs/projections/proj/healpix.hpp>
#include <boost/geometry/srs/projections/proj/krovak.hpp>
#include <boost/geometry/srs/projections/proj/igh.hpp>
#include <boost/geometry/srs/projections/proj/imw_p.hpp>
#include <boost/geometry/srs/projections/proj/isea.hpp>
#include <boost/geometry/srs/projections/proj/laea.hpp>
#include <boost/geometry/srs/projections/proj/labrd.hpp>
#include <boost/geometry/srs/projections/proj/lagrng.hpp>
#include <boost/geometry/srs/projections/proj/larr.hpp>
#include <boost/geometry/srs/projections/proj/lask.hpp>
#include <boost/geometry/srs/projections/proj/latlong.hpp>
#include <boost/geometry/srs/projections/proj/lcc.hpp>
#include <boost/geometry/srs/projections/proj/lcca.hpp>
#include <boost/geometry/srs/projections/proj/loxim.hpp>
#include <boost/geometry/srs/projections/proj/lsat.hpp>
#include <boost/geometry/srs/projections/proj/mbtfpp.hpp>
#include <boost/geometry/srs/projections/proj/mbtfpq.hpp>
#include <boost/geometry/srs/projections/proj/mbt_fps.hpp>
#include <boost/geometry/srs/projections/proj/merc.hpp>
#include <boost/geometry/srs/projections/proj/mill.hpp>
#include <boost/geometry/srs/projections/proj/mod_ster.hpp>
#include <boost/geometry/srs/projections/proj/moll.hpp>
#include <boost/geometry/srs/projections/proj/natearth.hpp>
#include <boost/geometry/srs/projections/proj/nell.hpp>
#include <boost/geometry/srs/projections/proj/nell_h.hpp>
#include <boost/geometry/srs/projections/proj/nocol.hpp>
#include <boost/geometry/srs/projections/proj/nsper.hpp>
#include <boost/geometry/srs/projections/proj/nzmg.hpp>
#include <boost/geometry/srs/projections/proj/ob_tran.hpp>
#include <boost/geometry/srs/projections/proj/ocea.hpp>
#include <boost/geometry/srs/projections/proj/oea.hpp>
#include <boost/geometry/srs/projections/proj/omerc.hpp>
#include <boost/geometry/srs/projections/proj/ortho.hpp>
#include <boost/geometry/srs/projections/proj/qsc.hpp>
#include <boost/geometry/srs/projections/proj/poly.hpp>
#include <boost/geometry/srs/projections/proj/putp2.hpp>
#include <boost/geometry/srs/projections/proj/putp3.hpp>
#include <boost/geometry/srs/projections/proj/putp4p.hpp>
#include <boost/geometry/srs/projections/proj/putp5.hpp>
#include <boost/geometry/srs/projections/proj/putp6.hpp>
#include <boost/geometry/srs/projections/proj/robin.hpp>
#include <boost/geometry/srs/projections/proj/rouss.hpp>
#include <boost/geometry/srs/projections/proj/rpoly.hpp>
#include <boost/geometry/srs/projections/proj/sconics.hpp>
#include <boost/geometry/srs/projections/proj/somerc.hpp>
#include <boost/geometry/srs/projections/proj/stere.hpp>
#include <boost/geometry/srs/projections/proj/sterea.hpp>
#include <boost/geometry/srs/projections/proj/sts.hpp>
#include <boost/geometry/srs/projections/proj/tcc.hpp>
#include <boost/geometry/srs/projections/proj/tcea.hpp>
#include <boost/geometry/srs/projections/proj/tmerc.hpp>
#include <boost/geometry/srs/projections/proj/tpeqd.hpp>
#include <boost/geometry/srs/projections/proj/urm5.hpp>
#include <boost/geometry/srs/projections/proj/urmfps.hpp>
#include <boost/geometry/srs/projections/proj/vandg.hpp>
#include <boost/geometry/srs/projections/proj/vandg2.hpp>
#include <boost/geometry/srs/projections/proj/vandg4.hpp>
#include <boost/geometry/srs/projections/proj/wag2.hpp>
#include <boost/geometry/srs/projections/proj/wag3.hpp>
#include <boost/geometry/srs/projections/proj/wag7.hpp>
#include <boost/geometry/srs/projections/proj/wink1.hpp>
#include <boost/geometry/srs/projections/proj/wink2.hpp>

namespace boost { namespace geometry { namespace projections
{

namespace detail
{

template <typename Params>
struct factory_key
{
    BOOST_GEOMETRY_STATIC_ASSERT_FALSE(
        "Invalid parameters type.",
        Params);
};

template <>
struct factory_key<srs::detail::proj4_parameters>
{
    typedef std::string type;
    template <typename ProjParams>
    static type const& get(ProjParams const& par)
    {
        return par.id.name;
    }
    static const char* get(const char* name, srs::dpar::value_proj )
    {
        return name;
    }
};

template <typename T>
struct factory_key<srs::dpar::parameters<T> >
{
    typedef srs::dpar::value_proj type;
    template <typename ProjParams>
    static type const& get(ProjParams const& par)
    {
        return par.id.id;
    }
    static srs::dpar::value_proj get(const char* , srs::dpar::value_proj id)
    {
        return id;
    }
};


template <typename Params, typename CT, typename ProjParams>
class factory
{
private:
    typedef detail::factory_entry
        <
            Params,
            CT,
            ProjParams
        > entry_base;

    typedef factory_key<Params> key;
    typedef typename key::type key_type;
    typedef boost::shared_ptr<entry_base> entry_ptr;

    typedef std::map<key_type, entry_ptr> entries_map;

    entries_map m_entries;

public:

    factory()
    {
        detail::aea_init(*this);
        detail::aeqd_init(*this);
        detail::airy_init(*this);
        detail::aitoff_init(*this);
        detail::august_init(*this);
        detail::bacon_init(*this);
        detail::bipc_init(*this);
        detail::boggs_init(*this);
        detail::bonne_init(*this);
        detail::cass_init(*this);
        detail::cc_init(*this);
        detail::cea_init(*this);
        detail::chamb_init(*this);
        detail::collg_init(*this);
        detail::crast_init(*this);
        detail::denoy_init(*this);
        detail::eck1_init(*this);
        detail::eck2_init(*this);
        detail::eck3_init(*this);
        detail::eck4_init(*this);
        detail::eck5_init(*this);
        detail::eqc_init(*this);
        detail::eqdc_init(*this);
        detail::etmerc_init(*this);
        detail::fahey_init(*this);
        detail::fouc_s_init(*this);
        detail::gall_init(*this);
        detail::geocent_init(*this);
        detail::geos_init(*this);
        detail::gins8_init(*this);
        detail::gn_sinu_init(*this);
        detail::gnom_init(*this);
        detail::goode_init(*this);
        detail::gstmerc_init(*this);
        detail::hammer_init(*this);
        detail::hatano_init(*this);
        detail::healpix_init(*this);
        detail::krovak_init(*this);
        detail::igh_init(*this);
        detail::imw_p_init(*this);
        detail::isea_init(*this);
        detail::labrd_init(*this);
        detail::laea_init(*this);
        detail::lagrng_init(*this);
        detail::larr_init(*this);
        detail::lask_init(*this);
        detail::latlong_init(*this);
        detail::lcc_init(*this);
        detail::lcca_init(*this);
        detail::loxim_init(*this);
        detail::lsat_init(*this);
        detail::mbtfpp_init(*this);
        detail::mbtfpq_init(*this);
        detail::mbt_fps_init(*this);
        detail::merc_init(*this);
        detail::mill_init(*this);
        detail::mod_ster_init(*this);
        detail::moll_init(*this);
        detail::natearth_init(*this);
        detail::nell_init(*this);
        detail::nell_h_init(*this);
        detail::nocol_init(*this);
        detail::nsper_init(*this);
        detail::nzmg_init(*this);
        detail::ob_tran_init(*this);
        detail::ocea_init(*this);
        detail::oea_init(*this);
        detail::omerc_init(*this);
        detail::ortho_init(*this);
        detail::qsc_init(*this);
        detail::poly_init(*this);
        detail::putp2_init(*this);
        detail::putp3_init(*this);
        detail::putp4p_init(*this);
        detail::putp5_init(*this);
        detail::putp6_init(*this);
        detail::robin_init(*this);
        detail::rouss_init(*this);
        detail::rpoly_init(*this);
        detail::sconics_init(*this);
        detail::somerc_init(*this);
        detail::stere_init(*this);
        detail::sterea_init(*this);
        detail::sts_init(*this);
        detail::tcc_init(*this);
        detail::tcea_init(*this);
        detail::tmerc_init(*this);
        detail::tpeqd_init(*this);
        detail::urm5_init(*this);
        detail::urmfps_init(*this);
        detail::vandg_init(*this);
        detail::vandg2_init(*this);
        detail::vandg4_init(*this);
        detail::wag2_init(*this);
        detail::wag3_init(*this);
        detail::wag7_init(*this);
        detail::wink1_init(*this);
        detail::wink2_init(*this);
    }

    void add_to_factory(const char* name, srs::dpar::value_proj id, entry_base* entry)
    {
        // The pointer has to be owned before std::map::operator[] in case it thrown an exception.
        entry_ptr ptr(entry);
        m_entries[key::get(name, id)] = ptr;
    }

    detail::dynamic_wrapper_b<CT, ProjParams>* create_new(Params const& params, ProjParams const& proj_par) const
    {
        typedef typename entries_map::const_iterator const_iterator;
        const_iterator it = m_entries.find(key::get(proj_par));
        if (it != m_entries.end())
        {
            return it->second->create_new(params, proj_par);
        }

        return 0;
    }
};

template <typename T>
inline detail::dynamic_wrapper_b<T, projections::parameters<T> >*
    create_new(srs::detail::proj4_parameters const& params,
               projections::parameters<T> const& parameters)
{
    static factory<srs::detail::proj4_parameters, T, projections::parameters<T> > const fac;
    return fac.create_new(params, parameters);
}

template <typename T>
inline detail::dynamic_wrapper_b<T, projections::parameters<T> >*
    create_new(srs::dpar::parameters<T> const& params,
               projections::parameters<T> const& parameters)
{
    static factory<srs::dpar::parameters<T>, T, projections::parameters<T> > const fac;
    return fac.create_new(params, parameters);
}


} // namespace detail

}}} // namespace boost::geometry::projections

#endif // BOOST_GEOMETRY_PROJECTIONS_FACTORY_HPP
