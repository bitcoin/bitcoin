// Boost.Geometry

// Copyright (c) 2017-2020, Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_SRS_PROJECTIONS_DPAR_HPP
#define BOOST_GEOMETRY_SRS_PROJECTIONS_DPAR_HPP


#include <string>
#include <type_traits>
#include <vector>

#include <boost/geometry/core/radius.hpp>
#include <boost/geometry/core/tag.hpp>
#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/srs/projections/exception.hpp>
#include <boost/geometry/srs/projections/par_data.hpp>
#include <boost/geometry/srs/sphere.hpp>
#include <boost/geometry/srs/spheroid.hpp>

#include <boost/geometry/util/range.hpp>

#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/size.hpp>
#include <boost/range/value_type.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/variant/variant.hpp>


namespace boost { namespace geometry { namespace srs
{

namespace detail
{

template <typename T, int I, typename ...>
struct find_type_index_impl
    : std::integral_constant<int, I>
{};

template
<
    typename T,
    int I,
    typename Type,
    typename ...Types
>
struct find_type_index_impl<T, I, Type, Types...>
    : std::conditional_t
        <
            std::is_same<T, Type>::value,
            std::integral_constant<int, I>,
            typename find_type_index_impl<T, I + 1, Types...>::type
        >
{};

template <typename Variant, typename T>
struct find_type_index
{};

template <BOOST_VARIANT_ENUM_PARAMS(typename T), typename T>
struct find_type_index<boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)>, T>
    : find_type_index_impl<T, 0, BOOST_VARIANT_ENUM_PARAMS(T)>
{};


template
<
    typename Range,
    typename ToValue,
    bool IsRange = range::detail::is_range<Range>::value
>
struct is_convertible_range
    : std::is_convertible
        <
            typename boost::range_value<Range>::type,
            ToValue
        >
{};

template
<
    typename Range,
    typename ToValue
>
struct is_convertible_range<Range, ToValue, false>
    : std::false_type
{};

} // namespace detail


namespace dpar
{

enum value_datum
{
    datum_wgs84 = 0,
    datum_ggrs87,
    datum_nad83,
    datum_nad27,
    datum_potsdam,
    datum_carthage,
    datum_hermannskogel,
    datum_ire65,
    datum_nzgd49,
    datum_osgb36,
};

enum value_ellps
{
    ellps_merit = 0,
    ellps_sgs85,
    ellps_grs80,
    ellps_iau76,
    ellps_airy,
    ellps_apl4_9,
    ellps_nwl9d,
    ellps_mod_airy,
    ellps_andrae,
    ellps_aust_sa,
    ellps_grs67,
    ellps_bessel,
    ellps_bess_nam,
    ellps_clrk66,
    ellps_clrk80,
    ellps_clrk80ign,
    ellps_cpm,
    ellps_delmbr,
    ellps_engelis,
    ellps_evrst30,
    ellps_evrst48,
    ellps_evrst56,
    ellps_evrst69,
    ellps_evrstss,
    ellps_fschr60,
    ellps_fschr60m,
    ellps_fschr68,
    ellps_helmert,
    ellps_hough,
    ellps_intl,
    ellps_krass,
    ellps_kaula,
    ellps_lerch,
    ellps_mprts,
    ellps_new_intl,
    ellps_plessis,
    ellps_seasia,
    ellps_walbeck,
    ellps_wgs60,
    ellps_wgs66,
    ellps_wgs72,
    ellps_wgs84,
    ellps_sphere
};

enum value_mode
{
    mode_plane = 0,
    mode_di,
    mode_dd,
    mode_hex
};

enum value_orient
{
    orient_isea = 0,
    orient_pole,
};

enum value_pm
{
    pm_greenwich = 0,
    pm_lisbon,
    pm_paris,
    pm_bogota,
    pm_madrid,
    pm_rome,
    pm_bern,
    pm_jakarta,
    pm_ferro,
    pm_brussels,
    pm_stockholm,
    pm_athens,
    pm_oslo
};

enum value_proj
{
    proj_unknown = 0,
    proj_aea, proj_leac,
    proj_aeqd,
    proj_airy,
    proj_aitoff, proj_wintri,
    proj_august,
    proj_apian, proj_ortel, proj_bacon,
    proj_bipc,
    proj_boggs,
    proj_bonne,
    proj_cass,
    proj_cc,
    proj_cea,
    proj_chamb,
    proj_collg,
    proj_crast,
    proj_denoy,
    proj_eck1,
    proj_eck2,
    proj_eck3, proj_putp1, proj_wag6, proj_kav7,
    proj_eck4,
    proj_eck5,
    proj_eqc,
    proj_eqdc,
    proj_etmerc, proj_utm,
    proj_fahey,
    proj_fouc_s,
    proj_gall,
    proj_geocent,
    proj_geos,
    proj_gins8,
    proj_gn_sinu, proj_sinu, proj_eck6, proj_mbtfps,
    proj_gnom,
    proj_goode,
    proj_gstmerc,
    proj_hammer,
    proj_hatano,
    proj_healpix,
    proj_rhealpix,
    proj_igh,
    proj_imw_p,
    proj_isea,
    proj_krovak,
    proj_labrd,
    proj_laea,
    proj_lagrng,
    proj_larr,
    proj_lask,
    proj_lonlat, proj_latlon, proj_latlong, proj_longlat,
    proj_lcc,
    proj_lcca,
    proj_loxim,
    proj_lsat,
    proj_mbt_fps,
    proj_mbtfpp,
    proj_mbtfpq,
    proj_merc,
    proj_mill,
    proj_mil_os, proj_lee_os, proj_gs48, proj_alsk, proj_gs50,
    proj_moll, proj_wag4, proj_wag5,
    proj_natearth,
    proj_nell,
    proj_nell_h,
    proj_nicol,
    proj_nsper, proj_tpers,
    proj_nzmg,
    proj_ob_tran,
    proj_ocea,
    proj_oea,
    proj_omerc,
    proj_ortho,
    proj_poly,
    proj_putp2,
    proj_putp3, proj_putp3p,
    proj_putp4p, proj_weren,
    proj_putp5, proj_putp5p,
    proj_putp6, proj_putp6p,
    proj_qsc,
    proj_robin,
    proj_rouss,
    proj_rpoly,
    proj_euler, proj_murd1, proj_murd2, proj_murd3, proj_pconic, proj_tissot, proj_vitk1,
    proj_somerc,
    proj_stere, proj_ups,
    proj_sterea,
    proj_kav5, proj_qua_aut, proj_fouc, proj_mbt_s,
    proj_tcc,
    proj_tcea,
    proj_tmerc,
    proj_tpeqd,
    proj_urm5,
    proj_urmfps, proj_wag1,
    proj_vandg,
    proj_vandg2, proj_vandg3,
    proj_vandg4,
    proj_wag2,
    proj_wag3,
    proj_wag7,
    proj_wink1,
    proj_wink2
};

enum value_sweep
{
    sweep_x = 0, sweep_y
};

enum value_units
{
    units_km = 0,
    units_m,
    units_dm,
    units_cm,
    units_mm,
    units_kmi,
    units_in,
    units_ft,
    units_yd,
    units_mi,
    units_fath,
    units_ch,
    units_link,
    units_us_in,
    units_us_ft,
    units_us_yd,
    units_us_ch,
    units_us_mi,
    units_ind_yd,
    units_ind_ft,
    units_ind_ch
};

enum name_f
{
    a = 0,
    b,
    e,
    es,
    f,
    h,
    //h_0, // currently not used
    k = 7,
    k_0,
    m, // also used for M
    n,
    //phdg_0, // currently not used
    //plat_0, // currently not used
    //plon_0, // currently not used
    q = 14,
    r, // originally R
    rf,
    to_meter,
    vto_meter,
    w, // originally W
    x_0,
    y_0
};

enum name_r
{
    alpha = 22,
    azi,
    gamma,
    lat_0,
    lat_1,
    lat_2,
    lat_3,
    lat_b,
    lat_ts, // 30
    lon_0,
    lon_1,
    lon_2,
    lon_3,
    lon_wrap,
    lonc,
    o_alpha,
    o_lat_1,
    o_lat_2,
    o_lat_c, // 40
    o_lat_p,
    o_lon_1,
    o_lon_2,
    o_lon_c,
    o_lon_p,
    r_lat_a, // originally R_lat_a
    r_lat_g, // originally R_lat_g
    theta,
    tilt
};

enum name_i
{
    aperture = 50,
    lsat,
    north_square,
    path,
    resolution,
    south_square,
    zone
};

enum name_be
{
    czech = 57,
    geoc,
    guam,
    no_cut, // 60
    no_defs,
    no_rot,
    ns,
    over,
    r_au, // originally R_A
    r_a, // originally R_a
    r_g, // originally R_g
    r_h, // originally R_h
    r_v, // originally R_V
    rescale, // 70
    south
};

/*enum name_catalog
{
    catalog = 72 // currently not used
};

enum name_date
{
    date = 73 // currently not used
};*/

enum name_datum
{
    datum = 74
};

enum name_ellps
{
    ellps = 75 // id, sphere or spheroid
};

/*enum name_geoidgrids
{
    geoidgrids = 76 // currently not used
};*/

enum name_mode
{
    mode = 77
};

enum name_nadgrids
{
    nadgrids = 78 // arbitrary-length list of strings
};

enum name_orient
{
    orient = 79
};

enum name_pm
{
    pm = 80 // id or angle
};

enum name_proj
{
    o_proj = 81,
    proj
};

enum name_sweep
{
    sweep = 83
};

enum name_towgs84
{
    towgs84 = 84 // 3 or 7 element list of numbers
};

enum name_units
{
    units = 85,
    vunits
};

template <typename T>
struct parameter
{
    parameter()
        : m_id(-1), m_value(false)
    {}

    parameter(name_f id, T const& v)
        : m_id(id), m_value(v)
    {}

    // TODO various angle units
    parameter(name_r id, T const& v)
        : m_id(id), m_value(v)
    {}

    parameter(name_i id, int v)
        : m_id(id), m_value(v)
    {}

    parameter(name_be id)
        : m_id(id), m_value(true)
    {}

    parameter(name_be id, bool v)
        : m_id(id), m_value(v)
    {}

    parameter(name_datum id, value_datum v)
        : m_id(id), m_value(int(v))
    {}

    parameter(value_datum v)
        : m_id(datum), m_value(int(v))
    {}

    // TODO: store model at this point?
    parameter(name_ellps id, value_ellps v)
        : m_id(id), m_value(int(v))
    {}
    // TODO: store model at this point?
    parameter(value_ellps v)
        : m_id(ellps), m_value(int(v))
    {}

    template
    <
        typename Sphere,
        std::enable_if_t
            <
                std::is_same<typename geometry::tag<Sphere>::type, srs_sphere_tag>::value,
                int
            > = 0
    >
    parameter(name_ellps id, Sphere const& v)
        : m_id(id)
        , m_value(T(get_radius<0>(v)))
    {}

    template
    <
        typename Spheroid,
        std::enable_if_t
            <
                std::is_same<typename geometry::tag<Spheroid>::type, srs_spheroid_tag>::value,
                int
            > = 0
    >
    parameter(name_ellps id, Spheroid const& v)
        : m_id(id)
        , m_value(srs::spheroid<T>(get_radius<0>(v), get_radius<2>(v)))
    {}

    parameter(name_mode id, value_mode v)
        : m_id(id), m_value(int(v))
    {}

    parameter(value_mode v)
        : m_id(mode), m_value(int(v))
    {}

    template
    <
        typename Range,
        std::enable_if_t
            <
                detail::is_convertible_range<Range const, std::string>::value,
                int
            > = 0
    >
    parameter(name_nadgrids id, Range const& v)
        : m_id(id)
        , m_value(srs::detail::nadgrids(boost::begin(v), boost::end(v)))
    {}

#ifndef BOOST_NO_CXX11_HDR_INITIALIZER_LIST
    parameter(name_nadgrids id, std::initializer_list<std::string> v)
        : m_id(id)
        , m_value(srs::detail::nadgrids(v))
    {}
#endif

    parameter(name_orient id, value_orient v)
        : m_id(id), m_value(int(v))
    {}

    parameter(value_orient v)
        : m_id(orient), m_value(int(v))
    {}

    // TODO: store to_meters at this point?
    parameter(name_pm id, value_pm v)
        : m_id(id), m_value(int(v))
    {}
    // TODO: store to_meters at this point?
    parameter(value_pm v)
        : m_id(pm), m_value(int(v))
    {}

    // TODO angle units
    parameter(name_pm id, T const& v)
        : m_id(id), m_value(v)
    {}

    parameter(name_proj id, value_proj v)
        : m_id(id), m_value(int(v))
    {}

    parameter(value_proj v)
        : m_id(proj), m_value(int(v))
    {}

    parameter(name_sweep id, value_sweep v)
        : m_id(id), m_value(int(v))
    {}

    parameter(value_sweep v)
        : m_id(sweep), m_value(int(v))
    {}

    template
    <
        typename Range,
        std::enable_if_t
            <
                detail::is_convertible_range<Range const, T>::value,
                int
            > = 0
    >
    parameter(name_towgs84 id, Range const& v)
        : m_id(id)
        , m_value(srs::detail::towgs84<T>(boost::begin(v), boost::end(v)))
    {
        std::size_t n = boost::size(v);
        if (n != 3 && n != 7)
        {
            BOOST_THROW_EXCEPTION( projection_exception("Invalid number of towgs84 elements. Should be 3 or 7.") );
        }
    }

#ifndef BOOST_NO_CXX11_HDR_INITIALIZER_LIST
    parameter(name_towgs84 id, std::initializer_list<T> v)
        : m_id(id)
        , m_value(srs::detail::towgs84<T>(v))
    {
        std::size_t n = v.size();
        if (n != 3 && n != 7)
        {
            BOOST_THROW_EXCEPTION( projection_exception("Invalid number of towgs84 elements. Should be 3 or 7.") );
        }
    }
#endif

    parameter(name_units id, value_units v)
        : m_id(id), m_value(int(v))
    {}

    parameter(value_units v)
        : m_id(units), m_value(int(v))
    {}
    
private:
    typedef boost::variant
        <
            bool,
            int,
            T,
            srs::spheroid<T>,
            srs::detail::nadgrids,
            srs::detail::towgs84<T>
        > variant_type;

public:
    bool is_id_equal(name_f const& id) const { return m_id == int(id); }
    bool is_id_equal(name_r const& id) const { return m_id == int(id); }
    bool is_id_equal(name_i const& id) const { return m_id == int(id); }
    bool is_id_equal(name_be const& id) const { return m_id == int(id); }
    bool is_id_equal(name_datum const& id) const { return m_id == int(id); }
    bool is_id_equal(name_ellps const& id) const { return m_id == int(id); }
    bool is_id_equal(name_mode const& id) const { return m_id == int(id); }
    bool is_id_equal(name_nadgrids const& id) const { return m_id == int(id); }
    bool is_id_equal(name_orient const& id) const { return m_id == int(id); }
    bool is_id_equal(name_pm const& id) const { return m_id == int(id); }
    bool is_id_equal(name_proj const& id) const { return m_id == int(id); }
    bool is_id_equal(name_sweep const& id) const { return m_id == int(id); }
    bool is_id_equal(name_towgs84 const& id) const { return m_id == int(id); }
    bool is_id_equal(name_units const& id) const { return m_id == int(id); }

    template <typename V>
    V const& get_value() const
    {
        return boost::get<V>(m_value);
    }

    template <typename V>
    bool is_value_set() const
    {
        return m_value.which() == srs::detail::find_type_index<variant_type, V>::value;
    }
    
private:
    int m_id;
    variant_type m_value;
};

template <typename T = double>
class parameters
{
    typedef std::vector<parameter<T> > container_type;

public:
    typedef typename container_type::value_type value_type;
    typedef typename container_type::const_iterator const_iterator;
    typedef typename container_type::const_reference const_reference;
    typedef typename container_type::size_type size_type;

    BOOST_DEFAULTED_FUNCTION(parameters(), {})

#if defined(BOOST_NO_CXX11_RVALUE_REFERENCES) || defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
    template <typename Id>
    explicit parameters(Id id)
    {
        add(id);
    }

    template <typename Id>
    parameters & add(Id id)
    {
        m_params.push_back(parameter<T>(id));
        return *this;
    }

    template <typename Id>
    parameters & operator()(Id id)
    {
        return add(id);
    }

    template <typename Id, typename V>
    parameters(Id id, V const& value)
    {
        add(id, value);
    }

    template <typename Id, typename V>
    parameters & add(Id id, V const& value)
    {
        m_params.push_back(parameter<T>(id, value));
        return *this;
    }

    template <typename Id, typename V>
    parameters & operator()(Id id, V const& value)
    {
        return add(id, value);
    }

#ifndef BOOST_NO_CXX11_HDR_INITIALIZER_LIST
    template <typename Id, typename V>
    parameters(Id id, std::initializer_list<V> value)
    {
        add(id, value);
    }

    template <typename Id, typename V>
    parameters & add(Id id, std::initializer_list<V> value)
    {
        m_params.push_back(parameter<T>(id, value));
        return *this;
    }

    template <typename Id, typename V>
    parameters & operator()(Id id, std::initializer_list<V> value)
    {
        return add(id, value);
    }
#endif // BOOST_NO_CXX11_HDR_INITIALIZER_LIST
#else // BOOST_NO_CXX11_RVALUE_REFERENCES || BOOST_NO_CXX11_RVALUE_REFERENCES
    template <typename Id>
    explicit parameters(Id id)
    {
        add(id);
    }

    template <typename Id>
    parameters & add(Id id)
    {
        m_params.emplace_back(id);
        return *this;
    }

    template <typename Id>
    parameters & operator()(Id id)
    {
        return add(id);
    }

    template <typename Id, typename V>
    parameters(Id id, V && value)
    {
        add(id, std::forward<V>(value));
    }

    template <typename Id, typename V>
    parameters & add(Id id, V && value)
    {
        m_params.emplace_back(id, std::forward<V>(value));
        return *this;
    }

    template <typename Id, typename V>
    parameters & operator()(Id id, V && value)
    {
        return add(id, std::forward<V>(value));
    }

#ifndef BOOST_NO_CXX11_HDR_INITIALIZER_LIST
    template <typename Id, typename V>
    parameters(Id id, std::initializer_list<V> value)
    {
        add(id, value);
    }

    template <typename Id, typename V>
    parameters & add(Id id, std::initializer_list<V> value)
    {
        m_params.emplace_back(id, value);
        return *this;
    }

    template <typename Id, typename V>
    parameters & operator()(Id id, std::initializer_list<V> value)
    {
        return add(id, value);
    }
#endif // BOOST_NO_CXX11_HDR_INITIALIZER_LIST
#endif // BOOST_NO_CXX11_RVALUE_REFERENCES || BOOST_NO_CXX11_RVALUE_REFERENCES

    const_iterator begin() const { return m_params.begin(); }
    const_iterator end() const { return m_params.end(); }
    const_reference operator[](size_type i) const { return m_params[i]; }
    size_type size() { return m_params.size(); }
    bool empty() { return m_params.empty(); }

private:
    container_type m_params;
};


} // namespace dpar


}}} // namespace boost::geometry::srs


#endif // BOOST_GEOMETRY_SRS_PROJECTIONS_DPAR_HPP
