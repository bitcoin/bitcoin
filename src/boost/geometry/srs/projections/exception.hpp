// Boost.Geometry

// Copyright (c) 2008-2012 Barend Gehrels, Amsterdam, the Netherlands.

// This file was modified by Oracle on 2017, 2018.
// Modifications copyright (c) 2017-2018, Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_PROJECTIONS_EXCEPTION_HPP
#define BOOST_GEOMETRY_PROJECTIONS_EXCEPTION_HPP


#include <boost/geometry/core/exception.hpp>
#include <boost/geometry/srs/projections/impl/pj_strerrno.hpp>

#include <boost/throw_exception.hpp>


namespace boost { namespace geometry
{


// TODO: make more for forward/inverse/init/setup
class projection_exception : public geometry::exception
{
public:
    explicit projection_exception(int code = 0)
        : m_code(code)
        , m_msg(projections::detail::pj_strerrno(code))
    {}

    explicit projection_exception(std::string const& msg)
        : m_code(0)
        , m_msg(msg)
    {}

    projection_exception(int code, std::string const& msg)
        : m_code(code)
        , m_msg(msg)
    {}

    virtual char const* what() const throw()
    {
        //return "Boost.Geometry Projection exception";
        return m_msg.what();
    }

    int code() const { return m_code; }
private :
    int m_code;
    std::runtime_error m_msg;
};


struct projection_not_named_exception
    : projection_exception
{
    projection_not_named_exception()
        : projection_exception(projections::detail::error_proj_not_named)
    {}
};

struct projection_unknown_id_exception
    : projection_exception
{
    projection_unknown_id_exception()
        : projection_exception(projections::detail::error_unknown_projection_id,
                               msg())
    {}

    projection_unknown_id_exception(std::string const& proj_name)
        : projection_exception(projections::detail::error_unknown_projection_id,
                               msg(proj_name))
    {}

private:
    static std::string msg()
    {
        using namespace projections::detail;
        return pj_strerrno(error_unknown_projection_id);
    }
    static std::string msg(std::string const& proj_name)
    {
        using namespace projections::detail;
        return pj_strerrno(error_unknown_projection_id) + " (" + proj_name + ")";
    }
};

struct projection_not_invertible_exception
    : projection_exception
{
    // NOTE: There is no error code in proj4 which could be used here
    // Proj4 sets points as invalid (HUGE_VAL) and last errno to EINVAL
    // in pj_inv() if inverse projection is not available.
    projection_not_invertible_exception(std::string const& proj_name)
        : projection_exception(projections::detail::error_non_conv_inv_meri_dist,
                               msg(proj_name))
    {}

private:
    static std::string msg(std::string const& proj_name)
    {
        return std::string("projection (") + proj_name + ") is not invertible";
    }
};


}} // namespace boost::geometry
#endif // BOOST_GEOMETRY_PROJECTIONS_EXCEPTION_HPP
