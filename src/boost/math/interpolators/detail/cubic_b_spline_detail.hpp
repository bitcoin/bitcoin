// Copyright Nick Thompson, 2017
// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef CUBIC_B_SPLINE_DETAIL_HPP
#define CUBIC_B_SPLINE_DETAIL_HPP

#include <limits>
#include <cmath>
#include <vector>
#include <memory>
#include <boost/math/constants/constants.hpp>
#include <boost/math/special_functions/fpclassify.hpp>

namespace boost{ namespace math{ namespace detail{


template <class Real>
class cubic_b_spline_imp
{
public:
    // If you don't know the value of the derivative at the endpoints, leave them as nans and the routine will estimate them.
    // f[0] = f(a), f[length -1] = b, step_size = (b - a)/(length -1).
    template <class BidiIterator>
    cubic_b_spline_imp(BidiIterator f, BidiIterator end_p, Real left_endpoint, Real step_size,
                       Real left_endpoint_derivative = std::numeric_limits<Real>::quiet_NaN(),
                       Real right_endpoint_derivative = std::numeric_limits<Real>::quiet_NaN());

    Real operator()(Real x) const;

    Real prime(Real x) const;

    Real double_prime(Real x) const;

private:
    std::vector<Real> m_beta;
    Real m_h_inv;
    Real m_a;
    Real m_avg;
};



template <class Real>
Real b3_spline(Real x)
{
    using std::abs;
    Real absx = abs(x);
    if (absx < 1)
    {
        Real y = 2 - absx;
        Real z = 1 - absx;
        return boost::math::constants::sixth<Real>()*(y*y*y - 4*z*z*z);
    }
    if (absx < 2)
    {
        Real y = 2 - absx;
        return boost::math::constants::sixth<Real>()*y*y*y;
    }
    return (Real) 0;
}

template<class Real>
Real b3_spline_prime(Real x)
{
    if (x < 0)
    {
        return -b3_spline_prime(-x);
    }

    if (x < 1)
    {
        return x*(3*boost::math::constants::half<Real>()*x - 2);
    }
    if (x < 2)
    {
        return -boost::math::constants::half<Real>()*(2 - x)*(2 - x);
    }
    return (Real) 0;
}

template<class Real>
Real b3_spline_double_prime(Real x)
{
    if (x < 0)
    {
        return b3_spline_double_prime(-x);
    }

    if (x < 1)
    {
        return 3*x - 2;
    }
    if (x < 2)
    {
        return (2 - x);
    }
    return (Real) 0;
}


template <class Real>
template <class BidiIterator>
cubic_b_spline_imp<Real>::cubic_b_spline_imp(BidiIterator f, BidiIterator end_p, Real left_endpoint, Real step_size,
                                             Real left_endpoint_derivative, Real right_endpoint_derivative) : m_a(left_endpoint), m_avg(0)
{
    using boost::math::constants::third;

    std::size_t length = end_p - f;

    if (length < 5)
    {
        if (boost::math::isnan(left_endpoint_derivative) || boost::math::isnan(right_endpoint_derivative))
        {
            throw std::logic_error("Interpolation using a cubic b spline with derivatives estimated at the endpoints requires at least 5 points.\n");
        }
        if (length < 3)
        {
            throw std::logic_error("Interpolation using a cubic b spline requires at least 3 points.\n");
        }
    }

    if (boost::math::isnan(left_endpoint))
    {
        throw std::logic_error("Left endpoint is NAN; this is disallowed.\n");
    }
    if (left_endpoint + length*step_size >= (std::numeric_limits<Real>::max)())
    {
        throw std::logic_error("Right endpoint overflows the maximum representable number of the specified precision.\n");
    }
    if (step_size <= 0)
    {
        throw std::logic_error("The step size must be strictly > 0.\n");
    }
    // Storing the inverse of the stepsize does provide a measurable speedup.
    // It's not huge, but nonetheless worthwhile.
    m_h_inv = 1/step_size;

    // Following Kress's notation, s'(a) = a1, s'(b) = b1
    Real a1 = left_endpoint_derivative;
    // See the finite-difference table on Wikipedia for reference on how
    // to construct high-order estimates for one-sided derivatives:
    // https://en.wikipedia.org/wiki/Finite_difference_coefficient#Forward_and_backward_finite_difference
    // Here, we estimate then to O(h^4), as that is the maximum accuracy we could obtain from this method.
    if (boost::math::isnan(a1))
    {
        // For simple functions (linear, quadratic, so on)
        // almost all the error comes from derivative estimation.
        // This does pairwise summation which gives us another digit of accuracy over naive summation.
        Real t0 = 4*(f[1] + third<Real>()*f[3]);
        Real t1 = -(25*third<Real>()*f[0] + f[4])/4  - 3*f[2];
        a1 = m_h_inv*(t0 + t1);
    }

    Real b1 = right_endpoint_derivative;
    if (boost::math::isnan(b1))
    {
        size_t n = length - 1;
        Real t0 = 4*(f[n-3] + third<Real>()*f[n - 1]);
        Real t1 = -(25*third<Real>()*f[n - 4] + f[n])/4  - 3*f[n - 2];

        b1 = m_h_inv*(t0 + t1);
    }

    // s(x) = \sum \alpha_i B_{3}( (x- x_i - a)/h )
    // Of course we must reindex from Kress's notation, since he uses negative indices which make C++ unhappy.
    m_beta.resize(length + 2, std::numeric_limits<Real>::quiet_NaN());

    // Since the splines have compact support, they decay to zero very fast outside the endpoints.
    // This is often very annoying; we'd like to evaluate the interpolant a little bit outside the
    // boundary [a,b] without massive error.
    // A simple way to deal with this is just to subtract the DC component off the signal, so we need the average.
    // This algorithm for computing the average is recommended in
    // http://www.heikohoffmann.de/htmlthesis/node134.html
    Real t = 1;
    for (size_t i = 0; i < length; ++i)
    {
        if (boost::math::isnan(f[i]))
        {
            std::string err = "This function you are trying to interpolate is a nan at index " + std::to_string(i) + "\n";
            throw std::logic_error(err);
        }
        m_avg += (f[i] - m_avg) / t;
        t += 1;
    }


    // Now we must solve an almost-tridiagonal system, which requires O(N) operations.
    // There are, in fact 5 diagonals, but they only differ from zero on the first and last row,
    // so we can patch up the tridiagonal row reduction algorithm to deal with two special rows.
    // See Kress, equations 8.41
    // The the "tridiagonal" matrix is:
    // 1  0 -1
    // 1  4  1
    //    1  4  1
    //       1  4  1
    //          ....
    //          1  4  1
    //          1  0 -1
    // Numerical estimate indicate that as N->Infinity, cond(A) -> 6.9, so this matrix is good.
    std::vector<Real> rhs(length + 2, std::numeric_limits<Real>::quiet_NaN());
    std::vector<Real> super_diagonal(length + 2, std::numeric_limits<Real>::quiet_NaN());

    rhs[0] = -2*step_size*a1;
    rhs[rhs.size() - 1] = -2*step_size*b1;

    super_diagonal[0] = 0;

    for(size_t i = 1; i < rhs.size() - 1; ++i)
    {
        rhs[i] = 6*(f[i - 1] - m_avg);
        super_diagonal[i] = 1;
    }


    // One step of row reduction on the first row to patch up the 5-diagonal problem:
    // 1 0 -1 | r0
    // 1 4 1  | r1
    // mapsto:
    // 1 0 -1 | r0
    // 0 4 2  | r1 - r0
    // mapsto
    // 1 0 -1 | r0
    // 0 1 1/2| (r1 - r0)/4
    super_diagonal[1] = 0.5;
    rhs[1] = (rhs[1] - rhs[0])/4;

    // Now do a tridiagonal row reduction the standard way, until just before the last row:
    for (size_t i = 2; i < rhs.size() - 1; ++i)
    {
        Real diagonal = 4 - super_diagonal[i - 1];
        rhs[i] = (rhs[i] - rhs[i - 1])/diagonal;
        super_diagonal[i] /= diagonal;
    }

    // Now the last row, which is in the form
    // 1 sd[n-3] 0      | rhs[n-3]
    // 0  1     sd[n-2] | rhs[n-2]
    // 1  0     -1      | rhs[n-1]
    Real final_subdiag = -super_diagonal[rhs.size() - 3];
    rhs[rhs.size() - 1] = (rhs[rhs.size() - 1] - rhs[rhs.size() - 3])/final_subdiag;
    Real final_diag = -1/final_subdiag;
    // Now we're here:
    // 1 sd[n-3] 0         | rhs[n-3]
    // 0  1     sd[n-2]    | rhs[n-2]
    // 0  1     final_diag | (rhs[n-1] - rhs[n-3])/diag

    final_diag = final_diag - super_diagonal[rhs.size() - 2];
    rhs[rhs.size() - 1] = rhs[rhs.size() - 1] - rhs[rhs.size() - 2];


    // Back substitutions:
    m_beta[rhs.size() - 1] = rhs[rhs.size() - 1]/final_diag;
    for(size_t i = rhs.size() - 2; i > 0; --i)
    {
        m_beta[i] = rhs[i] - super_diagonal[i]*m_beta[i + 1];
    }
    m_beta[0] = m_beta[2] + rhs[0];
}

template<class Real>
Real cubic_b_spline_imp<Real>::operator()(Real x) const
{
    // See Kress, 8.40: Since B3 has compact support, we don't have to sum over all terms,
    // just the (at most 5) whose support overlaps the argument.
    Real z = m_avg;
    Real t = m_h_inv*(x - m_a) + 1;

    using std::max;
    using std::min;
    using std::ceil;
    using std::floor;

    size_t k_min = (size_t) (max)(static_cast<long>(0), boost::math::ltrunc(ceil(t - 2)));
    size_t k_max = (size_t) (max)((min)(static_cast<long>(m_beta.size() - 1), boost::math::ltrunc(floor(t + 2))), (long) 0);
    for (size_t k = k_min; k <= k_max; ++k)
    {
        z += m_beta[k]*b3_spline(t - k);
    }

    return z;
}

template<class Real>
Real cubic_b_spline_imp<Real>::prime(Real x) const
{
    Real z = 0;
    Real t = m_h_inv*(x - m_a) + 1;

    using std::max;
    using std::min;
    using std::ceil;
    using std::floor;

    size_t k_min = (size_t) (max)(static_cast<long>(0), boost::math::ltrunc(ceil(t - 2)));
    size_t k_max = (size_t) (min)(static_cast<long>(m_beta.size() - 1), boost::math::ltrunc(floor(t + 2)));

    for (size_t k = k_min; k <= k_max; ++k)
    {
        z += m_beta[k]*b3_spline_prime(t - k);
    }
    return z*m_h_inv;
}

template<class Real>
Real cubic_b_spline_imp<Real>::double_prime(Real x) const
{
    Real z = 0;
    Real t = m_h_inv*(x - m_a) + 1;

    using std::max;
    using std::min;
    using std::ceil;
    using std::floor;

    size_t k_min = (size_t) (max)(static_cast<long>(0), boost::math::ltrunc(ceil(t - 2)));
    size_t k_max = (size_t) (min)(static_cast<long>(m_beta.size() - 1), boost::math::ltrunc(floor(t + 2)));

    for (size_t k = k_min; k <= k_max; ++k)
    {
        z += m_beta[k]*b3_spline_double_prime(t - k);
    }
    return z*m_h_inv*m_h_inv;
}


}}}
#endif
