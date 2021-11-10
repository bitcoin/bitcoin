// Copyright Nick Thompson, 2017
// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// This implements the compactly supported cubic b spline algorithm described in
// Kress, Rainer. "Numerical analysis, volume 181 of Graduate Texts in Mathematics." (1998).
// Splines of compact support are faster to evaluate and are better conditioned than classical cubic splines.

// Let f be the function we are trying to interpolate, and s be the interpolating spline.
// The routine constructs the interpolant in O(N) time, and evaluating s at a point takes constant time.
// The order of accuracy depends on the regularity of the f, however, assuming f is
// four-times continuously differentiable, the error is of O(h^4).
// In addition, we can differentiate the spline and obtain a good interpolant for f'.
// The main restriction of this method is that the samples of f must be evenly spaced.
// Look for barycentric rational interpolation for non-evenly sampled data.
// Properties:
// - s(x_j) = f(x_j)
// - All cubic polynomials interpolated exactly

#ifndef BOOST_MATH_INTERPOLATORS_CUBIC_B_SPLINE_HPP
#define BOOST_MATH_INTERPOLATORS_CUBIC_B_SPLINE_HPP

#include <boost/math/interpolators/detail/cubic_b_spline_detail.hpp>
#include <boost/math/tools/header_deprecated.hpp>

BOOST_MATH_HEADER_DEPRECATED("<boost/math/interpolators/cardinal_cubic_b_spline.hpp>");

namespace boost{ namespace math{

template <class Real>
class cubic_b_spline
{
public:
    // If you don't know the value of the derivative at the endpoints, leave them as nans and the routine will estimate them.
    // f[0] = f(a), f[length -1] = b, step_size = (b - a)/(length -1).
    template <class BidiIterator>
    cubic_b_spline(const BidiIterator f, BidiIterator end_p, Real left_endpoint, Real step_size,
                   Real left_endpoint_derivative = std::numeric_limits<Real>::quiet_NaN(),
                   Real right_endpoint_derivative = std::numeric_limits<Real>::quiet_NaN());
    cubic_b_spline(const Real* const f, size_t length, Real left_endpoint, Real step_size,
       Real left_endpoint_derivative = std::numeric_limits<Real>::quiet_NaN(),
       Real right_endpoint_derivative = std::numeric_limits<Real>::quiet_NaN());

    cubic_b_spline() = default;
    Real operator()(Real x) const;

    Real prime(Real x) const;

    Real double_prime(Real x) const;

private:
    std::shared_ptr<detail::cubic_b_spline_imp<Real>> m_imp;
};

template<class Real>
cubic_b_spline<Real>::cubic_b_spline(const Real* const f, size_t length, Real left_endpoint, Real step_size,
                                     Real left_endpoint_derivative, Real right_endpoint_derivative) : m_imp(std::make_shared<detail::cubic_b_spline_imp<Real>>(f, f + length, left_endpoint, step_size, left_endpoint_derivative, right_endpoint_derivative))
{
}

template <class Real>
template <class BidiIterator>
cubic_b_spline<Real>::cubic_b_spline(BidiIterator f, BidiIterator end_p, Real left_endpoint, Real step_size,
   Real left_endpoint_derivative, Real right_endpoint_derivative) : m_imp(std::make_shared<detail::cubic_b_spline_imp<Real>>(f, end_p, left_endpoint, step_size, left_endpoint_derivative, right_endpoint_derivative))
{
}

template<class Real>
Real cubic_b_spline<Real>::operator()(Real x) const
{
    return m_imp->operator()(x);
}

template<class Real>
Real cubic_b_spline<Real>::prime(Real x) const
{
    return m_imp->prime(x);
}

template<class Real>
Real cubic_b_spline<Real>::double_prime(Real x) const
{
    return m_imp->double_prime(x);
}


}}
#endif
