// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2014-2015, Oracle and/or its affiliates.

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_INCONSISTENT_TURNS_EXCEPTION_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_INCONSISTENT_TURNS_EXCEPTION_HPP

#if ! defined(BOOST_GEOMETRY_OVERLAY_NO_THROW)
#include <boost/geometry/core/exception.hpp>

namespace boost { namespace geometry
{

class inconsistent_turns_exception : public geometry::exception
{
public:

    inline inconsistent_turns_exception() {}

    virtual ~inconsistent_turns_exception() throw()
    {}

    virtual char const* what() const throw()
    {
        return "Boost.Geometry Inconsistent Turns exception";
    }
};

}} // boost::geometry

#endif // BOOST_GEOMETRY_OVERLAY_NO_THROW

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_INCONSISTENT_TURNS_EXCEPTION_HPP
