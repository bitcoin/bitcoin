// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2008-2012 Barend Gehrels, Amsterdam, the Netherlands.

// This file was modified by Oracle on 2017-2020.
// Modifications copyright (c) 2017-2020, Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_SRS_PROJECTION_HPP
#define BOOST_GEOMETRY_SRS_PROJECTION_HPP


#include <string>
#include <type_traits>

#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/throw_exception.hpp>

#include <boost/geometry/algorithms/convert.hpp>
#include <boost/geometry/algorithms/detail/convert_point_to_point.hpp>

#include <boost/geometry/core/coordinate_dimension.hpp>
#include <boost/geometry/core/static_assert.hpp>

#include <boost/geometry/srs/projections/dpar.hpp>
#include <boost/geometry/srs/projections/exception.hpp>
#include <boost/geometry/srs/projections/factory.hpp>
#include <boost/geometry/srs/projections/impl/base_dynamic.hpp>
#include <boost/geometry/srs/projections/impl/base_static.hpp>
#include <boost/geometry/srs/projections/impl/pj_init.hpp>
#include <boost/geometry/srs/projections/invalid_point.hpp>
#include <boost/geometry/srs/projections/proj4.hpp>
#include <boost/geometry/srs/projections/spar.hpp>

#include <boost/geometry/views/detail/indexed_point_view.hpp>


namespace boost { namespace geometry
{
    
namespace projections
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail
{

template <typename G1, typename G2>
struct same_tags
    : std::is_same
        <
            typename geometry::tag<G1>::type,
            typename geometry::tag<G2>::type
        >
{};

template <typename CT>
struct promote_to_double
{
    typedef std::conditional_t
        <
            std::is_integral<CT>::value || std::is_same<CT, float>::value,
            double, CT
        > type;
};

// Copy coordinates of dimensions >= MinDim
template <std::size_t MinDim, typename Point1, typename Point2>
inline void copy_higher_dimensions(Point1 const& point1, Point2 & point2)
{
    static const std::size_t dim1 = geometry::dimension<Point1>::value;
    static const std::size_t dim2 = geometry::dimension<Point2>::value;
    static const std::size_t lesser_dim = dim1 < dim2 ? dim1 : dim2;
    BOOST_GEOMETRY_STATIC_ASSERT((lesser_dim >= MinDim),
        "The dimension of Point1 or Point2 is too small.",
        Point1, Point2);

    geometry::detail::conversion::point_to_point
        <
            Point1, Point2, MinDim, lesser_dim
        > ::apply(point1, point2);

    // TODO: fill point2 with zeros if dim1 < dim2 ?
    // currently no need because equal dimensions are checked
}


struct forward_point_projection_policy
{
    template <typename LL, typename XY, typename Proj>
    static inline bool apply(LL const& ll, XY & xy, Proj const& proj)
    {
        return proj.forward(ll, xy);
    }
};

struct inverse_point_projection_policy
{
    template <typename XY, typename LL, typename Proj>
    static inline bool apply(XY const& xy, LL & ll, Proj const& proj)
    {
        return proj.inverse(xy, ll);
    }
};

template <typename PointPolicy>
struct project_point
{
    template <typename P1, typename P2, typename Proj>
    static inline bool apply(P1 const& p1, P2 & p2, Proj const& proj)
    {
        // (Geographic -> Cartesian) will be projected, rest will be copied.
        // So first copy third or higher dimensions
        projections::detail::copy_higher_dimensions<2>(p1, p2);

        if (! PointPolicy::apply(p1, p2, proj))
        {
            // For consistency with transformation
            set_invalid_point(p2);
            return false;
        }

        return true;
    }
};

template <typename PointPolicy>
struct project_range
{
    template <typename Proj>
    struct convert_policy
    {
        explicit convert_policy(Proj const& proj)
            : m_proj(proj)
            , m_result(true)
        {}

        template <typename Point1, typename Point2>
        inline void apply(Point1 const& point1, Point2 & point2)
        {
            if (! project_point<PointPolicy>::apply(point1, point2, m_proj) )
                m_result = false;
        }

        bool result() const
        {
            return m_result;
        }

    private:
        Proj const& m_proj;
        bool m_result;
    };

    template <typename R1, typename R2, typename Proj>
    static inline bool apply(R1 const& r1, R2 & r2, Proj const& proj)
    {
        return geometry::detail::conversion::range_to_range
            <
                R1, R2,
                geometry::point_order<R1>::value != geometry::point_order<R2>::value
            >::apply(r1, r2, convert_policy<Proj>(proj)).result();
    }
};

template <typename Policy>
struct project_multi
{
    template <typename G1, typename G2, typename Proj>
    static inline bool apply(G1 const& g1, G2 & g2, Proj const& proj)
    {
        range::resize(g2, boost::size(g1));
        return apply(boost::begin(g1), boost::end(g1),
                     boost::begin(g2),
                     proj);
    }

private:
    template <typename It1, typename It2, typename Proj>
    static inline bool apply(It1 g1_first, It1 g1_last, It2 g2_first, Proj const& proj)
    {
        bool result = true;
        for ( ; g1_first != g1_last ; ++g1_first, ++g2_first )
        {
            if (! Policy::apply(*g1_first, *g2_first, proj))
            {
                result = false;
            }
        }
        return result;
    }
};

template
<
    typename Geometry,
    typename PointPolicy,
    typename Tag = typename geometry::tag<Geometry>::type
>
struct project_geometry
{};

template <typename Geometry, typename PointPolicy>
struct project_geometry<Geometry, PointPolicy, point_tag>
    : project_point<PointPolicy>
{};

template <typename Geometry, typename PointPolicy>
struct project_geometry<Geometry, PointPolicy, multi_point_tag>
    : project_range<PointPolicy>
{};

template <typename Geometry, typename PointPolicy>
struct project_geometry<Geometry, PointPolicy, segment_tag>
{
    template <typename G1, typename G2, typename Proj>
    static inline bool apply(G1 const& g1, G2 & g2, Proj const& proj)
    {
        bool r1 = apply<0>(g1, g2, proj);
        bool r2 = apply<1>(g1, g2, proj);
        return r1 && r2;
    }

private:
    template <std::size_t Index, typename G1, typename G2, typename Proj>
    static inline bool apply(G1 const& g1, G2 & g2, Proj const& proj)
    {
        geometry::detail::indexed_point_view<G1 const, Index> pt1(g1);
        geometry::detail::indexed_point_view<G2, Index> pt2(g2);
        return project_point<PointPolicy>::apply(pt1, pt2, proj);
    }
};

template <typename Geometry, typename PointPolicy>
struct project_geometry<Geometry, PointPolicy, linestring_tag>
    : project_range<PointPolicy>
{};

template <typename Geometry, typename PointPolicy>
struct project_geometry<Geometry, PointPolicy, multi_linestring_tag>
    : project_multi< project_range<PointPolicy> >
{};

template <typename Geometry, typename PointPolicy>
struct project_geometry<Geometry, PointPolicy, ring_tag>
    : project_range<PointPolicy>
{};

template <typename Geometry, typename PointPolicy>
struct project_geometry<Geometry, PointPolicy, polygon_tag>
{
    template <typename G1, typename G2, typename Proj>
    static inline bool apply(G1 const& g1, G2 & g2, Proj const& proj)
    {
        bool r1 = project_range
                    <
                        PointPolicy
                    >::apply(geometry::exterior_ring(g1),
                             geometry::exterior_ring(g2),
                             proj);
        bool r2 = project_multi
                    <
                        project_range<PointPolicy>
                    >::apply(geometry::interior_rings(g1),
                             geometry::interior_rings(g2),
                             proj);
        return r1 && r2;
    }
};

template <typename MultiPolygon, typename PointPolicy>
struct project_geometry<MultiPolygon, PointPolicy, multi_polygon_tag>
    : project_multi
        <
            project_geometry
            <
                typename boost::range_value<MultiPolygon>::type,
                PointPolicy,
                polygon_tag
            >
        >
{};


} // namespace detail
#endif // DOXYGEN_NO_DETAIL


template <typename Params>
struct dynamic_parameters
{
    static const bool is_specialized = false;
};

template <>
struct dynamic_parameters<srs::proj4>
{
    static const bool is_specialized = true;
    static inline srs::detail::proj4_parameters apply(srs::proj4 const& params)
    {
        return srs::detail::proj4_parameters(params.str());
    }
};

template <typename T>
struct dynamic_parameters<srs::dpar::parameters<T> >
{
    static const bool is_specialized = true;
    static inline srs::dpar::parameters<T> const& apply(srs::dpar::parameters<T> const& params)
    {
        return params;
    }
};


// proj_wrapper class and its specializations wrapps the internal projection
// representation and implements transparent creation of projection object
template <typename Proj, typename CT>
class proj_wrapper
{
    BOOST_GEOMETRY_STATIC_ASSERT_FALSE(
        "Unknown projection definition.",
        Proj);
};

template <typename CT>
class proj_wrapper<srs::dynamic, CT>
{
    // Some projections do not work with float -> wrong results
    // select <double> from int/float/double and else selects T
    typedef typename projections::detail::promote_to_double<CT>::type calc_t;

    typedef projections::parameters<calc_t> parameters_type;
    typedef projections::detail::dynamic_wrapper_b<calc_t, parameters_type> vprj_t;

public:
    template
    <
        typename Params,
        std::enable_if_t
            <
                dynamic_parameters<Params>::is_specialized,
                int
            > = 0
    >
    proj_wrapper(Params const& params)
        : m_ptr(create(dynamic_parameters<Params>::apply(params)))
    {}

    vprj_t const& proj() const { return *m_ptr; }
    vprj_t & mutable_proj() { return *m_ptr; }

private:
    template <typename Params>
    static vprj_t* create(Params const& params)
    {
        parameters_type parameters = projections::detail::pj_init<calc_t>(params);

        vprj_t* result = projections::detail::create_new(params, parameters);

        if (result == NULL)
        {
            if (parameters.id.is_unknown())
            {
                BOOST_THROW_EXCEPTION(projection_not_named_exception());
            }
            else
            {
                // TODO: handle non-string projection id
                BOOST_THROW_EXCEPTION(projection_unknown_id_exception());
            }
        }

        return result;
    }

    boost::shared_ptr<vprj_t> m_ptr;
};

template <typename StaticParameters, typename CT>
class static_proj_wrapper_base
{
    typedef typename projections::detail::promote_to_double<CT>::type calc_t;

    typedef projections::parameters<calc_t> parameters_type;

    typedef typename srs::spar::detail::pick_proj_tag
        <
            StaticParameters
        >::type proj_tag;
    
    typedef typename projections::detail::static_projection_type
        <
            proj_tag,
            typename projections::detail::static_srs_tag<StaticParameters>::type,
            StaticParameters,
            calc_t,
            parameters_type
        >::type projection_type;

public:
    projection_type const& proj() const { return m_proj; }
    projection_type & mutable_proj() { return m_proj; }

protected:
    explicit static_proj_wrapper_base(StaticParameters const& s_params)
        : m_proj(s_params,
                 projections::detail::pj_init<calc_t>(s_params))
    {}

private:
    projection_type m_proj;
};

template <typename ...Ps, typename CT>
class proj_wrapper<srs::spar::parameters<Ps...>, CT>
    : public static_proj_wrapper_base<srs::spar::parameters<Ps...>, CT>
{
    typedef srs::spar::parameters<Ps...>
        static_parameters_type;
    typedef static_proj_wrapper_base
        <
            static_parameters_type,
            CT
        > base_t;

public:
    proj_wrapper()
        : base_t(static_parameters_type())
    {}

    proj_wrapper(static_parameters_type const& s_params)
        : base_t(s_params)
    {}
};


// projection class implements transparent forward/inverse projection interface
template <typename Proj, typename CT>
class projection
    : private proj_wrapper<Proj, CT>
{
    typedef proj_wrapper<Proj, CT> base_t;

public:
    projection()
    {}

    template <typename Params>
    explicit projection(Params const& params)
        : base_t(params)
    {}

    /// Forward projection, from Latitude-Longitude to Cartesian
    template <typename LL, typename XY>
    inline bool forward(LL const& ll, XY& xy) const
    {
        BOOST_GEOMETRY_STATIC_ASSERT(
            (projections::detail::same_tags<LL, XY>::value),
            "Not supported combination of Geometries.",
            LL, XY);

        concepts::check_concepts_and_equal_dimensions<LL const, XY>();

        return projections::detail::project_geometry
                <
                    LL,
                    projections::detail::forward_point_projection_policy
                >::apply(ll, xy, base_t::proj());
    }

    /// Inverse projection, from Cartesian to Latitude-Longitude
    template <typename XY, typename LL>
    inline bool inverse(XY const& xy, LL& ll) const
    {
        BOOST_GEOMETRY_STATIC_ASSERT(
            (projections::detail::same_tags<XY, LL>::value),
            "Not supported combination of Geometries.",
            XY, LL);

        concepts::check_concepts_and_equal_dimensions<XY const, LL>();

        return projections::detail::project_geometry
                <
                    XY,
                    projections::detail::inverse_point_projection_policy
                >::apply(xy, ll, base_t::proj());
    }
};

} // namespace projections


namespace srs
{

    
/*!
    \brief Representation of projection
    \details Either dynamic or static projection representation
    \ingroup projection
    \tparam Parameters default dynamic tag or static projection parameters
    \tparam CT calculation type used internally
*/
template
<
    typename Parameters = srs::dynamic,
    typename CT = double
>
class projection
    : public projections::projection<Parameters, CT>
{
    typedef projections::projection<Parameters, CT> base_t;

public:
    projection()
    {}

    projection(Parameters const& parameters)
        : base_t(parameters)
    {}

    /*!
    \ingroup projection
    \brief Initializes a projection as a string, using the format with + and =
    \details The projection can be initialized with a string (with the same format as the PROJ4 package) for
      convenient initialization from, for example, the command line
    \par Example
        <tt>srs::proj4("+proj=labrd +ellps=intl +lon_0=46d26'13.95E +lat_0=18d54S +azi=18d54 +k_0=.9995 +x_0=400000 +y_0=800000")</tt>
        for the Madagascar projection.
    */
    template
    <
        typename DynamicParameters,
        std::enable_if_t
            <
                projections::dynamic_parameters<DynamicParameters>::is_specialized,
                int
            > = 0
    >
    projection(DynamicParameters const& dynamic_parameters)
        : base_t(dynamic_parameters)
    {}
};


} // namespace srs


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_SRS_PROJECTION_HPP
