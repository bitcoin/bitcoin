// Boost.Geometry

// Copyright (c) 2018-2019 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_GEOMETRIES_INFINITE_LINE_HPP
#define BOOST_GEOMETRY_GEOMETRIES_INFINITE_LINE_HPP


namespace boost { namespace geometry
{

namespace model
{

//--------------------------------------------------------------------------
// Structure containing an infinite line.
// It is written using "General Form", a*x + b*y + c == 0
// Might be conceptized later. Therefore operations are implemented outside
// the structure itself.
template <typename Type = double>
struct infinite_line
{
    using type = Type;

    infinite_line()
        : a(0)
        , b(0)
        , c(0)
        , normalized(false)
    {}

    // Horizontal: a == 0, for example y-3=0, y==3
    // Vertical: b == 0, for example x-2=0, x==2
    // Through origin: c == 0
    Type a;
    Type b;
    Type c;
    bool normalized;
};


} // namespace model


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_GEOMETRIES_INFINITE_LINE_HPP
