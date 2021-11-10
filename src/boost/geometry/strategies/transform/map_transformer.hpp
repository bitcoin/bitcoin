// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGIES_TRANSFORM_MAP_TRANSFORMER_HPP
#define BOOST_GEOMETRY_STRATEGIES_TRANSFORM_MAP_TRANSFORMER_HPP


#include <cstddef>

#include <boost/geometry/strategies/transform/matrix_transformers.hpp>

namespace boost { namespace geometry
{

// Silence warning C4127: conditional expression is constant
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4127)
#endif

namespace strategy { namespace transform
{

/*!
\brief Transformation strategy to map from one to another Cartesian coordinate system
\ingroup strategies
\tparam Mirror if true map is mirrored upside-down (in most cases pixels
    are from top to bottom, while map is from bottom to top)
 */
template
<
    typename CalculationType,
    std::size_t Dimension1,
    std::size_t Dimension2,
    bool Mirror = false,
    bool SameScale = true
>
class map_transformer
    : public matrix_transformer<CalculationType, Dimension1, Dimension2>
{
    typedef boost::qvm::mat<CalculationType, Dimension1 + 1, Dimension2 + 1> M;
    typedef boost::qvm::mat<CalculationType, 3, 3> matrix33;

public :
    template <typename B, typename D>
    explicit inline map_transformer(B const& box, D const& width, D const& height)
    {
        set_transformation(
                get<min_corner, 0>(box), get<min_corner, 1>(box),
                get<max_corner, 0>(box), get<max_corner, 1>(box),
                width, height);
    }

    template <typename W, typename D>
    explicit inline map_transformer(W const& wx1, W const& wy1, W const& wx2, W const& wy2,
                        D const& width, D const& height)
    {
        set_transformation(wx1, wy1, wx2, wy2, width, height);
    }


private :
    template <typename W, typename P, typename S>
    inline void set_transformation_point(W const& wx, W const& wy,
        P const& px, P const& py,
        S const& scalex, S const& scaley)
    {

        // Translate to a coordinate system centered on world coordinates (-wx, -wy)
        matrix33 t1;
        qvm::A<0,0>(t1) = 1;   qvm::A<0,1>(t1) = 0;   qvm::A<0,2>(t1) = -wx;
        qvm::A<1,0>(t1) = 0;   qvm::A<1,1>(t1) = 1;   qvm::A<1,2>(t1) = -wy;
        qvm::A<2,0>(t1) = 0;   qvm::A<2,1>(t1) = 0;   qvm::A<2,2>(t1) = 1;

        // Scale the map
        matrix33 s;
        qvm::A<0,0>(s) = scalex;   qvm::A<0,1>(s) = 0;      qvm::A<0,2>(s) = 0;
        qvm::A<1,0>(s) = 0;        qvm::A<1,1>(s) = scaley; qvm::A<1,2>(s) = 0;
        qvm::A<2,0>(s) = 0;        qvm::A<2,1>(s) = 0;      qvm::A<2,2>(s) = 1;

        // Translate to a coordinate system centered on the specified pixels (+px, +py)
        matrix33 t2;
        qvm::A<0,0>(t2) = 1;   qvm::A<0,1>(t2) = 0;   qvm::A<0,2>(t2) = px;
        qvm::A<1,0>(t2) = 0;   qvm::A<1,1>(t2) = 1;   qvm::A<1,2>(t2) = py;
        qvm::A<2,0>(t2) = 0;   qvm::A<2,1>(t2) = 0;   qvm::A<2,2>(t2) = 1;

        // Calculate combination matrix in two steps
        this->m_matrix = s * t1;
        this->m_matrix = t2 * this->m_matrix;
    }


    template <typename W, typename D>
    void set_transformation(W const& wx1, W const& wy1, W const& wx2, W const& wy2,
                    D const& width, D const& height)
    {
        D px1 = 0;
        D py1 = 0;
        D px2 = width;
        D py2 = height;

        // Get the same type, but at least a double
        typedef typename select_most_precise<D, double>::type type;


        // Calculate appropriate scale, take min because whole box must fit
        // Scale is in PIXELS/MAPUNITS (meters)
        W wdx = wx2 - wx1;
        W wdy = wy2 - wy1;
        type sx = (px2 - px1) / boost::numeric_cast<type>(wdx);
        type sy = (py2 - py1) / boost::numeric_cast<type>(wdy);

        if (SameScale)
        {
            type scale = (std::min)(sx, sy);
            sx = scale;
            sy = scale;
        }

        // Calculate centerpoints
        W wtx = wx1 + wx2;
        W wty = wy1 + wy2;
        W two = 2;
        W wmx = wtx / two;
        W wmy = wty / two;
        type pmx = (px1 + px2) / 2.0;
        type pmy = (py1 + py2) / 2.0;

        set_transformation_point(wmx, wmy, pmx, pmy, sx, sy);

        if (Mirror)
        {
            // Mirror in y-direction
            matrix33 m;
            qvm::A<0,0>(m) = 1;   qvm::A<0,1>(m) = 0;   qvm::A<0,2>(m) = 0;
            qvm::A<1,0>(m) = 0;   qvm::A<1,1>(m) = -1;  qvm::A<1,2>(m) = 0;
            qvm::A<2,0>(m) = 0;   qvm::A<2,1>(m) = 0;   qvm::A<2,2>(m) = 1;

            // Translate in y-direction such that it fits again
            matrix33 y;
            qvm::A<0,0>(y) = 1;   qvm::A<0,1>(y) = 0;   qvm::A<0,2>(y) = 0;
            qvm::A<1,0>(y) = 0;   qvm::A<1,1>(y) = 1;   qvm::A<1,2>(y) = height;
            qvm::A<2,0>(y) = 0;   qvm::A<2,1>(y) = 0;   qvm::A<2,2>(y) = 1;

            // Calculate combination matrix in two steps
            this->m_matrix = m * this->m_matrix;
            this->m_matrix = y * this->m_matrix;
        }
    }
};


}} // namespace strategy::transform

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_STRATEGIES_TRANSFORM_MAP_TRANSFORMER_HPP
