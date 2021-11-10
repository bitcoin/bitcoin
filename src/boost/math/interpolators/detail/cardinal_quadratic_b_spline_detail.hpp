// Copyright Nick Thompson, 2019
// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_INTERPOLATORS_CARDINAL_QUADRATIC_B_SPLINE_DETAIL_HPP
#define BOOST_MATH_INTERPOLATORS_CARDINAL_QUADRATIC_B_SPLINE_DETAIL_HPP
#include <vector>
#include <cmath>
#include <stdexcept>

namespace boost{ namespace math{ namespace interpolators{ namespace detail{

template <class Real>
Real b2_spline(Real x) {
    using std::abs;
    Real absx = abs(x);
    if (absx < 1/Real(2))
    {
        Real y = absx - 1/Real(2);
        Real z = absx + 1/Real(2);
        return (2-y*y-z*z)/2;
    }
    if (absx < Real(3)/Real(2))
    {
        Real y = absx - Real(3)/Real(2);
        return y*y/2;
    }
    return (Real) 0;
}

template <class Real>
Real b2_spline_prime(Real x) {
    if (x < 0) {
        return -b2_spline_prime(-x);
    }

    if (x < 1/Real(2))
    {
        return -2*x;
    }
    if (x < Real(3)/Real(2))
    {
        return x - Real(3)/Real(2);
    }
    return (Real) 0;
}


template <class Real>
class cardinal_quadratic_b_spline_detail
{
public:
    // If you don't know the value of the derivative at the endpoints, leave them as nans and the routine will estimate them.
    // y[0] = y(a), y[n -1] = y(b), step_size = (b - a)/(n -1).

    cardinal_quadratic_b_spline_detail(const Real* const y,
                                size_t n,
                                Real t0 /* initial time, left endpoint */,
                                Real h  /*spacing, stepsize*/,
                                Real left_endpoint_derivative = std::numeric_limits<Real>::quiet_NaN(),
                                Real right_endpoint_derivative = std::numeric_limits<Real>::quiet_NaN())
    {
        if (h <= 0) {
            throw std::logic_error("Spacing must be > 0.");
        }
        m_inv_h = 1/h;
        m_t0 = t0;

        if (n < 3) {
            throw std::logic_error("The interpolator requires at least 3 points.");
        }

        using std::isnan;
        Real a;
        if (isnan(left_endpoint_derivative)) {
            // http://web.media.mit.edu/~crtaylor/calculator.html
            a = -3*y[0] + 4*y[1] - y[2];
        }
        else {
            a = 2*h*left_endpoint_derivative;
        }

        Real b;
        if (isnan(right_endpoint_derivative)) {
            b = 3*y[n-1] - 4*y[n-2] + y[n-3];
        }
        else {
            b = 2*h*right_endpoint_derivative;
        }

        m_alpha.resize(n + 2);

        // Begin row reduction:
        std::vector<Real> rhs(n + 2, std::numeric_limits<Real>::quiet_NaN());
        std::vector<Real> super_diagonal(n + 2, std::numeric_limits<Real>::quiet_NaN());

        rhs[0] = -a;
        rhs[rhs.size() - 1] = b;

        super_diagonal[0] = 0;

        for(size_t i = 1; i < rhs.size() - 1; ++i) {
            rhs[i] = 8*y[i - 1];
            super_diagonal[i] = 1;
        }

        // Patch up 5-diagonal problem:
        rhs[1] = (rhs[1] - rhs[0])/6;
        super_diagonal[1] = Real(1)/Real(3);
        // First two rows are now:
        // 1 0 -1 | -2hy0'
        // 0 1 1/3| (8y0+2hy0')/6


        // Start traditional tridiagonal row reduction:
        for (size_t i = 2; i < rhs.size() - 1; ++i) {
            Real diagonal = 6 - super_diagonal[i - 1];
            rhs[i] = (rhs[i] - rhs[i - 1])/diagonal;
            super_diagonal[i] /= diagonal;
        }

        //  1 sd[n-1] 0     | rhs[n-1]
        //  0 1       sd[n] | rhs[n]
        // -1 0       1     | rhs[n+1]

        rhs[n+1] = rhs[n+1] + rhs[n-1];
        Real bottom_subdiagonal = super_diagonal[n-1];

        // We're here:
        //  1 sd[n-1] 0     | rhs[n-1]
        //  0 1       sd[n] | rhs[n]
        //  0 bs      1     | rhs[n+1]

        rhs[n+1] = (rhs[n+1]-bottom_subdiagonal*rhs[n])/(1-bottom_subdiagonal*super_diagonal[n]);

        m_alpha[n+1] = rhs[n+1];
        for (size_t i = n; i > 0; --i) {
            m_alpha[i] = rhs[i] - m_alpha[i+1]*super_diagonal[i];
        }
        m_alpha[0] = m_alpha[2] + rhs[0];
    }

    Real operator()(Real t) const {
        if (t < m_t0 || t > m_t0 + (m_alpha.size()-2)/m_inv_h) {
            const char* err_msg = "Tried to evaluate the cardinal quadratic b-spline outside the domain of of interpolation; extrapolation does not work.";
            throw std::domain_error(err_msg);
        }
        // Let k, gamma be defined via t = t0 + kh + gamma * h.
        // Now find all j: |k-j+1+gamma|< 3/2, or, in other words
        // j_min = ceil((t-t0)/h - 1/2)
        // j_max = floor(t-t0)/h + 5/2)
        using std::floor;
        using std::ceil;
        Real x = (t-m_t0)*m_inv_h;
        size_t j_min = ceil(x - Real(1)/Real(2));
        size_t j_max = ceil(x + Real(5)/Real(2));
        if (j_max >= m_alpha.size()) {
            j_max = m_alpha.size() - 1;
        }

        Real y = 0;
        x += 1;
        for (size_t j = j_min; j <= j_max; ++j) {
            y += m_alpha[j]*detail::b2_spline(x - j);
        }
        return y;
    }

    Real prime(Real t) const {
        if (t < m_t0 || t > m_t0 + (m_alpha.size()-2)/m_inv_h) {
            const char* err_msg = "Tried to evaluate the cardinal quadratic b-spline outside the domain of of interpolation; extrapolation does not work.";
            throw std::domain_error(err_msg);
        }
        // Let k, gamma be defined via t = t0 + kh + gamma * h.
        // Now find all j: |k-j+1+gamma|< 3/2, or, in other words
        // j_min = ceil((t-t0)/h - 1/2)
        // j_max = floor(t-t0)/h + 5/2)
        using std::floor;
        using std::ceil;
        Real x = (t-m_t0)*m_inv_h;
        size_t j_min = ceil(x - Real(1)/Real(2));
        size_t j_max = ceil(x + Real(5)/Real(2));
        if (j_max >= m_alpha.size()) {
            j_max = m_alpha.size() - 1;
        }

        Real y = 0;
        x += 1;
        for (size_t j = j_min; j <= j_max; ++j) {
            y += m_alpha[j]*detail::b2_spline_prime(x - j);
        }
        return y*m_inv_h;
    }

    Real t_max() const {
        return m_t0 + (m_alpha.size()-3)/m_inv_h;
    }

private:
    std::vector<Real> m_alpha;
    Real m_inv_h;
    Real m_t0;
};

}}}}
#endif
