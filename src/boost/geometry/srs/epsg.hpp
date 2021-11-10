// Boost.Geometry

// Copyright (c) 2017-2018, Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_SRS_EPSG_HPP
#define BOOST_GEOMETRY_SRS_EPSG_HPP


#include <boost/geometry/srs/projection.hpp>
#include <boost/geometry/srs/projections/epsg.hpp>
#include <boost/geometry/srs/projections/epsg_params.hpp>
#include <boost/geometry/srs/projections/epsg_traits.hpp>


namespace boost { namespace geometry
{
    
namespace projections
{

template <>
struct dynamic_parameters<srs::epsg>
{
    static const bool is_specialized = true;
    static inline srs::dpar::parameters<> apply(srs::epsg const& params)
    {
        return projections::detail::epsg_to_parameters(params.code);
    }
};

template <int Code, typename CT>
class proj_wrapper<srs::static_epsg<Code>, CT>
    : public proj_wrapper
        <
            typename projections::detail::epsg_traits<Code>::parameters_type,
            CT
        >
{
    typedef projections::detail::epsg_traits<Code> epsg_traits;

    typedef proj_wrapper
        <
            typename epsg_traits::parameters_type,
            CT
        > base_t;

public:
    proj_wrapper()
        : base_t(epsg_traits::parameters())
    {}

    explicit proj_wrapper(srs::static_epsg<Code> const&)
        : base_t(epsg_traits::parameters())
    {}
};

} // namespace projections


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_SRS_EPSG_HPP
