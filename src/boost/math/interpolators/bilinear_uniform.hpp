// Copyright Nick Thompson, 2021
// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// This implements bilinear interpolation on a uniform grid.
// If dx and dy are both positive, then the (x,y) = (x0, y0) is associated with data index 0 (herein referred to as f[0])
// The point (x0 + dx, y0) is associated with f[1], and (x0 + i*dx, y0) is associated with f[i],
// i.e., we are assuming traditional C row major order.
// The y coordinate increases *downward*, as is traditional in 2D computer graphics.
// This is *not* how people generally think in numerical analysis (although it *is* how they lay out matrices).
// Providing the capability of a grid rotation is too expensive and not ergonomic; you'll need to perform any rotations at the call level.

// For clarity, the value f(x0 + i*dx, y0 + j*dy) must be stored in the f[j*cols + i] position.

#ifndef BOOST_MATH_INTERPOLATORS_BILINEAR_UNIFORM_HPP
#define BOOST_MATH_INTERPOLATORS_BILINEAR_UNIFORM_HPP

#include <utility>
#include <memory>
#include <boost/math/interpolators/detail/bilinear_uniform_detail.hpp>

namespace boost::math::interpolators {

template <class RandomAccessContainer>
class bilinear_uniform
{
public:
    using Real = typename RandomAccessContainer::value_type;
    using Z = typename RandomAccessContainer::size_type;

    bilinear_uniform(RandomAccessContainer && fieldData, Z rows, Z cols, Real dx = 1, Real dy = 1, Real x0 = 0, Real y0 = 0)
    : m_imp(std::make_shared<detail::bilinear_uniform_imp<RandomAccessContainer>>(std::move(fieldData), rows, cols, dx, dy, x0, y0))
    {
    }

    Real operator()(Real x, Real y) const
    {
        return m_imp->operator()(x,y);
    }


    friend std::ostream& operator<<(std::ostream& out, bilinear_uniform<RandomAccessContainer> const & bu) {
        out << *bu.m_imp;
        return out;
    }

private:
    std::shared_ptr<detail::bilinear_uniform_imp<RandomAccessContainer>> m_imp;
};

}
#endif
