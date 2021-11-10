// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2017-2020, Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_SRS_TRANSFORMATION_HPP
#define BOOST_GEOMETRY_SRS_TRANSFORMATION_HPP


#include <string>
#include <type_traits>

#include <boost/throw_exception.hpp>

#include <boost/geometry/algorithms/convert.hpp>

#include <boost/geometry/core/coordinate_dimension.hpp>
#include <boost/geometry/core/static_assert.hpp>

#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/multi_point.hpp>
#include <boost/geometry/geometries/linestring.hpp>
#include <boost/geometry/geometries/ring.hpp>
#include <boost/geometry/geometries/segment.hpp>

#include <boost/geometry/srs/projection.hpp>
#include <boost/geometry/srs/projections/grids.hpp>
#include <boost/geometry/srs/projections/impl/pj_transform.hpp>

#include <boost/geometry/views/detail/indexed_point_view.hpp>


namespace boost { namespace geometry
{

namespace projections { namespace detail
{

template <typename T1, typename T2>
inline bool same_object(T1 const& , T2 const& )
{
    return false;
}

template <typename T>
inline bool same_object(T const& o1, T const& o2)
{
    return boost::addressof(o1) == boost::addressof(o2);
}

template
<
    typename PtIn,
    typename PtOut,
    bool SameUnits = std::is_same
                        <
                            typename geometry::detail::cs_angular_units<PtIn>::type,
                            typename geometry::detail::cs_angular_units<PtOut>::type
                        >::value
>
struct transform_geometry_point_coordinates
{
    static inline void apply(PtIn const& in, PtOut & out, bool /*enable_angles*/)
    {
        geometry::set<0>(out, geometry::get<0>(in));
        geometry::set<1>(out, geometry::get<1>(in));
    }
};

template <typename PtIn, typename PtOut>
struct transform_geometry_point_coordinates<PtIn, PtOut, false>
{
    static inline void apply(PtIn const& in, PtOut & out, bool enable_angles)
    {
        if (enable_angles)
        {
            geometry::set_from_radian<0>(out, geometry::get_as_radian<0>(in));
            geometry::set_from_radian<1>(out, geometry::get_as_radian<1>(in));
        }
        else
        {
            geometry::set<0>(out, geometry::get<0>(in));
            geometry::set<1>(out, geometry::get<1>(in));
        }
    }
};

template <typename Geometry, typename CT>
struct transform_geometry_point
{
    typedef typename geometry::point_type<Geometry>::type point_type;

    typedef geometry::model::point
        <   
            typename select_most_precise
                <
                    typename geometry::coordinate_type<point_type>::type,
                    CT
                >::type,
            geometry::dimension<point_type>::type::value,
            typename geometry::coordinate_system<point_type>::type
        > type;

    template <typename PtIn, typename PtOut>
    static inline void apply(PtIn const& in, PtOut & out, bool enable_angles)
    {
        transform_geometry_point_coordinates<PtIn, PtOut>::apply(in, out, enable_angles);
        projections::detail::copy_higher_dimensions<2>(in, out);
    }
};

template <typename Geometry, typename CT>
struct transform_geometry_range_base
{
    struct convert_strategy
    {
        convert_strategy(bool enable_angles)
            : m_enable_angles(enable_angles)
        {}

        template <typename PtIn, typename PtOut>
        void apply(PtIn const& in, PtOut & out)
        {
            transform_geometry_point<Geometry, CT>::apply(in, out, m_enable_angles);
        }

        bool m_enable_angles;
    };

    template <typename In, typename Out>
    static inline void apply(In const& in, Out & out, bool enable_angles)
    {
        // Change the order and/or closure if needed
        // In - arbitrary geometry
        // Out - either Geometry or std::vector
        // So the order and closure of In and Geometry shoudl be compared
        // std::vector's order is assumed to be the same as of Geometry
        geometry::detail::conversion::range_to_range
            <
                In,
                Out,
                geometry::point_order<In>::value != geometry::point_order<Out>::value
            >::apply(in, out, convert_strategy(enable_angles));
    }
};

template
<
    typename Geometry,
    typename CT,
    typename Tag = typename geometry::tag<Geometry>::type
>
struct transform_geometry
{};

template <typename Point, typename CT>
struct transform_geometry<Point, CT, point_tag>
    : transform_geometry_point<Point, CT>
{};

template <typename Segment, typename CT>
struct transform_geometry<Segment, CT, segment_tag>
{
    typedef geometry::model::segment
        <
            typename transform_geometry_point<Segment, CT>::type
        > type;

    template <typename In, typename Out>
    static inline void apply(In const& in, Out & out, bool enable_angles)
    {
        apply<0>(in, out, enable_angles);
        apply<1>(in, out, enable_angles);
    }

private:
    template <std::size_t Index, typename In, typename Out>
    static inline void apply(In const& in, Out & out, bool enable_angles)
    {
        geometry::detail::indexed_point_view<In const, Index> in_pt(in);
        geometry::detail::indexed_point_view<Out, Index> out_pt(out);
        transform_geometry_point<Segment, CT>::apply(in_pt, out_pt, enable_angles);
    }
};

template <typename MultiPoint, typename CT>
struct transform_geometry<MultiPoint, CT, multi_point_tag>
    : transform_geometry_range_base<MultiPoint, CT>
{
    typedef model::multi_point
        <
            typename transform_geometry_point<MultiPoint, CT>::type
        > type;
};

template <typename LineString, typename CT>
struct transform_geometry<LineString, CT, linestring_tag>
    : transform_geometry_range_base<LineString, CT>
{
    typedef model::linestring
        <
            typename transform_geometry_point<LineString, CT>::type
        > type;
};

template <typename Ring, typename CT>
struct transform_geometry<Ring, CT, ring_tag>
    : transform_geometry_range_base<Ring, CT>
{
    typedef model::ring
        <
            typename transform_geometry_point<Ring, CT>::type,
            geometry::point_order<Ring>::value == clockwise,
            geometry::closure<Ring>::value == closed
        > type;
};


template
<
    typename OutGeometry,
    typename CT,
    bool EnableTemporary = ! std::is_same
                                <
                                    typename select_most_precise
                                        <
                                            typename geometry::coordinate_type<OutGeometry>::type,
                                            CT
                                        >::type,
                                    typename geometry::coordinate_type<OutGeometry>::type
                                >::value
>
struct transform_geometry_wrapper
{
    typedef transform_geometry<OutGeometry, CT> transform;
    typedef typename transform::type type;

    template <typename InGeometry>
    transform_geometry_wrapper(InGeometry const& in, OutGeometry & out, bool input_angles)
        : m_out(out)
    {
        transform::apply(in, m_temp, input_angles);
    }

    type & get() { return m_temp; }
    void finish() { geometry::convert(m_temp, m_out); } // this is always copy 1:1 without changing the order or closure

private:
    type m_temp;
    OutGeometry & m_out;
};

template
<
    typename OutGeometry,
    typename CT
>
struct transform_geometry_wrapper<OutGeometry, CT, false>
{
    typedef transform_geometry<OutGeometry, CT> transform;
    typedef OutGeometry type;

    transform_geometry_wrapper(OutGeometry const& in, OutGeometry & out, bool input_angles)
        : m_out(out)
    {
        if (! same_object(in, out))
            transform::apply(in, out, input_angles);
    }

    template <typename InGeometry>
    transform_geometry_wrapper(InGeometry const& in, OutGeometry & out, bool input_angles)
        : m_out(out)
    {
        transform::apply(in, out, input_angles);
    }

    OutGeometry & get() { return m_out; }
    void finish() {}

private:
    OutGeometry & m_out;
};

template <typename CT>
struct transform_range
{
    template
    <
        typename Proj1, typename Par1,
        typename Proj2, typename Par2,
        typename RangeIn, typename RangeOut,
        typename Grids
    >
    static inline bool apply(Proj1 const& proj1, Par1 const& par1,
                             Proj2 const& proj2, Par2 const& par2,
                             RangeIn const& in, RangeOut & out,
                             Grids const& grids1, Grids const& grids2)
    {
        // NOTE: this has to be consistent with pj_transform()
        bool const input_angles = !par1.is_geocent && par1.is_latlong;

        transform_geometry_wrapper<RangeOut, CT> wrapper(in, out, input_angles);

        bool res = true;
        try
        {
            res = pj_transform(proj1, par1, proj2, par2, wrapper.get(), grids1, grids2);
        }
        catch (projection_exception const&)
        {
            res = false;
        }
        catch(...)
        {
            BOOST_RETHROW
        }

        wrapper.finish();

        return res;
    }
};

template <typename Policy>
struct transform_multi
{
    template
    <
        typename Proj1, typename Par1,
        typename Proj2, typename Par2,
        typename MultiIn, typename MultiOut,
        typename Grids
    >
    static inline bool apply(Proj1 const& proj1, Par1 const& par1,
                             Proj2 const& proj2, Par2 const& par2,
                             MultiIn const& in, MultiOut & out,
                             Grids const& grids1, Grids const& grids2)
    {
        if (! same_object(in, out))
            range::resize(out, boost::size(in));

        return apply(proj1, par1, proj2, par2,
                     boost::begin(in), boost::end(in),
                     boost::begin(out),
                     grids1, grids2);
    }

private:
    template
    <
        typename Proj1, typename Par1,
        typename Proj2, typename Par2,
        typename InIt, typename OutIt,
        typename Grids
    >
    static inline bool apply(Proj1 const& proj1, Par1 const& par1,
                             Proj2 const& proj2, Par2 const& par2,
                             InIt in_first, InIt in_last, OutIt out_first,
                             Grids const& grids1, Grids const& grids2)
    {
        bool res = true;
        for ( ; in_first != in_last ; ++in_first, ++out_first )
        {
            if ( ! Policy::apply(proj1, par1, proj2, par2, *in_first, *out_first, grids1, grids2) )
            {
                res = false;
            }
        }
        return res;
    }
};

template
<
    typename Geometry,
    typename CT,
    typename Tag = typename geometry::tag<Geometry>::type
>
struct transform
    : not_implemented<Tag>
{};

template <typename Point, typename CT>
struct transform<Point, CT, point_tag>
{
    template
    <
        typename Proj1, typename Par1,
        typename Proj2, typename Par2,
        typename PointIn, typename PointOut,
        typename Grids
    >
    static inline bool apply(Proj1 const& proj1, Par1 const& par1,
                             Proj2 const& proj2, Par2 const& par2,
                             PointIn const& in, PointOut & out,
                             Grids const& grids1, Grids const& grids2)
    {
        // NOTE: this has to be consistent with pj_transform()
        bool const input_angles = !par1.is_geocent && par1.is_latlong;

        transform_geometry_wrapper<PointOut, CT> wrapper(in, out, input_angles);

        typedef typename transform_geometry_wrapper<PointOut, CT>::type point_type;
        point_type * ptr = boost::addressof(wrapper.get());

        std::pair<point_type *, point_type *> range = std::make_pair(ptr, ptr + 1);

        bool res = true;
        try
        {
            res = pj_transform(proj1, par1, proj2, par2, range, grids1, grids2);
        }
        catch (projection_exception const&)
        {
            res = false;
        }
        catch(...)
        {
            BOOST_RETHROW
        }

        wrapper.finish();

        return res;
    }
};

template <typename MultiPoint, typename CT>
struct transform<MultiPoint, CT, multi_point_tag>
    : transform_range<CT>
{};

template <typename Segment, typename CT>
struct transform<Segment, CT, segment_tag>
{
    template
    <
        typename Proj1, typename Par1,
        typename Proj2, typename Par2,
        typename SegmentIn, typename SegmentOut,
        typename Grids
    >
    static inline bool apply(Proj1 const& proj1, Par1 const& par1,
                             Proj2 const& proj2, Par2 const& par2,
                             SegmentIn const& in, SegmentOut & out,
                             Grids const& grids1, Grids const& grids2)
    {
        // NOTE: this has to be consistent with pj_transform()
        bool const input_angles = !par1.is_geocent && par1.is_latlong;

        transform_geometry_wrapper<SegmentOut, CT> wrapper(in, out, input_angles);

        typedef typename geometry::point_type
            <
                typename transform_geometry_wrapper<SegmentOut, CT>::type
            >::type point_type;

        point_type points[2];

        geometry::detail::assign_point_from_index<0>(wrapper.get(), points[0]);
        geometry::detail::assign_point_from_index<1>(wrapper.get(), points[1]);

        std::pair<point_type*, point_type*> range = std::make_pair(points, points + 2);

        bool res = true;
        try
        {
            res = pj_transform(proj1, par1, proj2, par2, range, grids1, grids2);
        }
        catch (projection_exception const&)
        {
            res = false;
        }
        catch(...)
        {
            BOOST_RETHROW
        }

        geometry::detail::assign_point_to_index<0>(points[0], wrapper.get());
        geometry::detail::assign_point_to_index<1>(points[1], wrapper.get());

        wrapper.finish();

        return res;
    }
};

template <typename Linestring, typename CT>
struct transform<Linestring, CT, linestring_tag>
    : transform_range<CT>
{};

template <typename MultiLinestring, typename CT>
struct transform<MultiLinestring, CT, multi_linestring_tag>
    : transform_multi<transform_range<CT> >
{};

template <typename Ring, typename CT>
struct transform<Ring, CT, ring_tag>
    : transform_range<CT>
{};

template <typename Polygon, typename CT>
struct transform<Polygon, CT, polygon_tag>
{
    template
    <
        typename Proj1, typename Par1,
        typename Proj2, typename Par2,
        typename PolygonIn, typename PolygonOut,
        typename Grids
    >
    static inline bool apply(Proj1 const& proj1, Par1 const& par1,
                             Proj2 const& proj2, Par2 const& par2,
                             PolygonIn const& in, PolygonOut & out,
                             Grids const& grids1, Grids const& grids2)
    {
        bool r1 = transform_range
                    <
                        CT
                    >::apply(proj1, par1, proj2, par2,
                             geometry::exterior_ring(in),
                             geometry::exterior_ring(out),
                             grids1, grids2);
        bool r2 = transform_multi
                    <
                        transform_range<CT>
                     >::apply(proj1, par1, proj2, par2,
                              geometry::interior_rings(in),
                              geometry::interior_rings(out),
                              grids1, grids2);
        return r1 && r2;
    }
};

template <typename MultiPolygon, typename CT>
struct transform<MultiPolygon, CT, multi_polygon_tag>
    : transform_multi
        <
            transform
                <
                    typename boost::range_value<MultiPolygon>::type,
                    CT,
                    polygon_tag
                >
        >
{};


}} // namespace projections::detail
    
namespace srs
{


/*!
    \brief Representation of projection
    \details Either dynamic or static projection representation
    \ingroup projection
    \tparam Proj1 default_dynamic or static projection parameters
    \tparam Proj2 default_dynamic or static projection parameters
    \tparam CT calculation type used internally
*/
template
<
    typename Proj1 = srs::dynamic,
    typename Proj2 = srs::dynamic,
    typename CT = double
>
class transformation
{
    typedef typename projections::detail::promote_to_double<CT>::type calc_t;

public:
    // Both static and default constructed
    transformation()
    {}

    // First dynamic, second static and default constructed
    template
    <
        typename Parameters1,
        std::enable_if_t
            <
                std::is_same<Proj1, srs::dynamic>::value
             && projections::dynamic_parameters<Parameters1>::is_specialized,
                int
            > = 0
    >
    explicit transformation(Parameters1 const& parameters1)
        : m_proj1(parameters1)
    {}

    // First static, second static and default constructed
    explicit transformation(Proj1 const& parameters1)
        : m_proj1(parameters1)
    {}

    // Both dynamic
    template
    <
        typename Parameters1, typename Parameters2,
        std::enable_if_t
            <
                std::is_same<Proj1, srs::dynamic>::value
             && std::is_same<Proj2, srs::dynamic>::value
             && projections::dynamic_parameters<Parameters1>::is_specialized
             && projections::dynamic_parameters<Parameters2>::is_specialized,
                int
            > = 0
    >
    transformation(Parameters1 const& parameters1,
                   Parameters2 const& parameters2)
        : m_proj1(parameters1)
        , m_proj2(parameters2)
    {}

    // First dynamic, second static
    template
    <
        typename Parameters1,
        std::enable_if_t
            <
                std::is_same<Proj1, srs::dynamic>::value
             && projections::dynamic_parameters<Parameters1>::is_specialized,
                int
            > = 0
    >
    transformation(Parameters1 const& parameters1,
                   Proj2 const& parameters2)
        : m_proj1(parameters1)
        , m_proj2(parameters2)
    {}

    // First static, second dynamic
    template
    <
        typename Parameters2,
        std::enable_if_t
            <
                std::is_same<Proj2, srs::dynamic>::value
             && projections::dynamic_parameters<Parameters2>::is_specialized,
                int
            > = 0
    >
    transformation(Proj1 const& parameters1,
                   Parameters2 const& parameters2)
        : m_proj1(parameters1)
        , m_proj2(parameters2)
    {}

    // Both static
    transformation(Proj1 const& parameters1,
                   Proj2 const& parameters2)
        : m_proj1(parameters1)
        , m_proj2(parameters2)
    {}

    template <typename GeometryIn, typename GeometryOut>
    bool forward(GeometryIn const& in, GeometryOut & out) const
    {
        return forward(in, out, transformation_grids<detail::empty_grids_storage>());
    }

    template <typename GeometryIn, typename GeometryOut>
    bool inverse(GeometryIn const& in, GeometryOut & out) const
    {
        return inverse(in, out, transformation_grids<detail::empty_grids_storage>());
    }

    template <typename GeometryIn, typename GeometryOut, typename GridsStorage>
    bool forward(GeometryIn const& in, GeometryOut & out,
                 transformation_grids<GridsStorage> const& grids) const
    {
        BOOST_GEOMETRY_STATIC_ASSERT(
            (projections::detail::same_tags<GeometryIn, GeometryOut>::value),
            "Not supported combination of Geometries.",
            GeometryIn, GeometryOut);

        return projections::detail::transform
                <
                    GeometryOut,
                    calc_t
                >::apply(m_proj1.proj(), m_proj1.proj().params(),
                         m_proj2.proj(), m_proj2.proj().params(),
                         in, out,
                         grids.src_grids,
                         grids.dst_grids);
    }

    template <typename GeometryIn, typename GeometryOut, typename GridsStorage>
    bool inverse(GeometryIn const& in, GeometryOut & out,
                 transformation_grids<GridsStorage> const& grids) const
    {
        BOOST_GEOMETRY_STATIC_ASSERT(
            (projections::detail::same_tags<GeometryIn, GeometryOut>::value),
            "Not supported combination of Geometries.",
            GeometryIn, GeometryOut);

        return projections::detail::transform
                <
                    GeometryOut,
                    calc_t
                >::apply(m_proj2.proj(), m_proj2.proj().params(),
                         m_proj1.proj(), m_proj1.proj().params(),
                         in, out,
                         grids.dst_grids,
                         grids.src_grids);
    }

    template <typename GridsStorage>
    inline transformation_grids<GridsStorage> initialize_grids(GridsStorage & grids_storage) const
    {
        transformation_grids<GridsStorage> result(grids_storage);

        using namespace projections::detail;
        pj_gridlist_from_nadgrids(m_proj1.proj().params(),
                                  result.src_grids);
        pj_gridlist_from_nadgrids(m_proj2.proj().params(),
                                  result.dst_grids);

        return result;
    }

private:
    projections::proj_wrapper<Proj1, CT> m_proj1;
    projections::proj_wrapper<Proj2, CT> m_proj2;
};


} // namespace srs


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_SRS_TRANSFORMATION_HPP
