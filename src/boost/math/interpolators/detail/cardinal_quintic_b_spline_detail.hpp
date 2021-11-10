// Copyright Nick Thompson, 2019
// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_INTERPOLATORS_CARDINAL_QUINTIC_B_SPLINE_DETAIL_HPP
#define BOOST_MATH_INTERPOLATORS_CARDINAL_QUINTIC_B_SPLINE_DETAIL_HPP
#include <cmath>
#include <vector>
#include <utility>
#include <boost/math/special_functions/cardinal_b_spline.hpp>

namespace boost{ namespace math{ namespace interpolators{ namespace detail{


template <class Real>
class cardinal_quintic_b_spline_detail
{
public:
    // If you don't know the value of the derivative at the endpoints, leave them as nans and the routine will estimate them.
    // y[0] = y(a), y[n -1] = y(b), step_size = (b - a)/(n -1).

    cardinal_quintic_b_spline_detail(const Real* const y,
                                     size_t n,
                                     Real t0 /* initial time, left endpoint */,
                                     Real h  /*spacing, stepsize*/,
                                     std::pair<Real, Real> left_endpoint_derivatives,
                                     std::pair<Real, Real> right_endpoint_derivatives)
    {
        static_assert(!std::is_integral<Real>::value, "The quintic B-spline interpolator only works with floating point types.");
        if (h <= 0) {
            throw std::logic_error("Spacing must be > 0.");
        }
        m_inv_h = 1/h;
        m_t0 = t0;

        if (n < 8) {
            throw std::logic_error("The quintic B-spline interpolator requires at least 8 points.");
        }

        using std::isnan;
        // This interpolator has error of order h^6, so the derivatives should be estimated with the same error.
        // See: https://en.wikipedia.org/wiki/Finite_difference_coefficient
        if (isnan(left_endpoint_derivatives.first)) {
            Real tmp = -49*y[0]/20 + 6*y[1] - 15*y[2]/2 + 20*y[3]/3 - 15*y[4]/4 + 6*y[5]/5 - y[6]/6;
            left_endpoint_derivatives.first = tmp/h;
        }
        if (isnan(right_endpoint_derivatives.first)) {
            Real tmp = 49*y[n-1]/20 - 6*y[n-2] + 15*y[n-3]/2 - 20*y[n-4]/3 + 15*y[n-5]/4 - 6*y[n-6]/5 + y[n-7]/6;
            right_endpoint_derivatives.first = tmp/h;
        }
        if(isnan(left_endpoint_derivatives.second)) {
            Real tmp = 469*y[0]/90 - 223*y[1]/10 + 879*y[2]/20 - 949*y[3]/18 + 41*y[4] - 201*y[5]/10 + 1019*y[6]/180 - 7*y[7]/10;
            left_endpoint_derivatives.second = tmp/(h*h);
        }
        if (isnan(right_endpoint_derivatives.second)) {
            Real tmp = 469*y[n-1]/90 - 223*y[n-2]/10 + 879*y[n-3]/20 - 949*y[n-4]/18 + 41*y[n-5] - 201*y[n-6]/10 + 1019*y[n-7]/180 - 7*y[n-8]/10;
            right_endpoint_derivatives.second = tmp/(h*h);
        }

        // This is really challenging my mental limits on by-hand row reduction.
        // I debated bringing in a dependency on a sparse linear solver, but given that that would cause much agony for users I decided against it.

        std::vector<Real> rhs(n+4);
        rhs[0] = 20*y[0] - 12*h*left_endpoint_derivatives.first +  2*h*h*left_endpoint_derivatives.second;
        rhs[1] = 60*y[0] - 12*h*left_endpoint_derivatives.first;
        for (size_t i = 2; i < n + 2; ++i) {
            rhs[i] = 120*y[i-2];
        }
        rhs[n+2] = 60*y[n-1] + 12*h*right_endpoint_derivatives.first;
        rhs[n+3] = 20*y[n-1] + 12*h*right_endpoint_derivatives.first +  2*h*h*right_endpoint_derivatives.second;

        std::vector<Real> diagonal(n+4, 66);
        diagonal[0] = 1;
        diagonal[1] = 18;
        diagonal[n+2] = 18;
        diagonal[n+3] = 1;

        std::vector<Real> first_superdiagonal(n+4, 26);
        first_superdiagonal[0] = 10;
        first_superdiagonal[1] = 33;
        first_superdiagonal[n+2] = 1;
        // There is one less superdiagonal than diagonal; make sure that if we read it, it shows up as a bug:
        first_superdiagonal[n+3] = std::numeric_limits<Real>::quiet_NaN();

        std::vector<Real> second_superdiagonal(n+4, 1);
        second_superdiagonal[0] = 9;
        second_superdiagonal[1] = 8;
        second_superdiagonal[n+2] = std::numeric_limits<Real>::quiet_NaN();
        second_superdiagonal[n+3] = std::numeric_limits<Real>::quiet_NaN();

        std::vector<Real> first_subdiagonal(n+4, 26);
        first_subdiagonal[0] = std::numeric_limits<Real>::quiet_NaN();
        first_subdiagonal[1] = 1;
        first_subdiagonal[n+2] = 33;
        first_subdiagonal[n+3] = 10;

        std::vector<Real> second_subdiagonal(n+4, 1);
        second_subdiagonal[0] = std::numeric_limits<Real>::quiet_NaN();
        second_subdiagonal[1] = std::numeric_limits<Real>::quiet_NaN();
        second_subdiagonal[n+2] = 8;
        second_subdiagonal[n+3] = 9;


        for (size_t i = 0; i < n+2; ++i) {
            Real di = diagonal[i];
            diagonal[i] = 1;
            first_superdiagonal[i] /= di;
            second_superdiagonal[i] /= di;
            rhs[i] /= di;

            // Eliminate first subdiagonal:
            Real nfsub = -first_subdiagonal[i+1];
            // Superfluous:
            first_subdiagonal[i+1] /= nfsub;
            // Not superfluous:
            diagonal[i+1] /= nfsub;
            first_superdiagonal[i+1] /= nfsub;
            second_superdiagonal[i+1] /= nfsub;
            rhs[i+1] /= nfsub;

            diagonal[i+1] += first_superdiagonal[i];
            first_superdiagonal[i+1] += second_superdiagonal[i];
            rhs[i+1] += rhs[i];
            // Superfluous, but clarifying:
            first_subdiagonal[i+1] = 0;

            // Eliminate second subdiagonal:
            Real nssub = -second_subdiagonal[i+2];
            first_subdiagonal[i+2] /= nssub;
            diagonal[i+2] /= nssub;
            first_superdiagonal[i+2] /= nssub;
            second_superdiagonal[i+2] /= nssub;
            rhs[i+2] /= nssub;

            first_subdiagonal[i+2] += first_superdiagonal[i];
            diagonal[i+2] += second_superdiagonal[i];
            rhs[i+2] += rhs[i];
            // Superfluous, but clarifying:
            second_subdiagonal[i+2] = 0;
        }

        // Eliminate last subdiagonal:
        Real dnp2 = diagonal[n+2];
        diagonal[n+2] = 1;
        first_superdiagonal[n+2] /= dnp2;
        rhs[n+2] /= dnp2;
        Real nfsubnp3 = -first_subdiagonal[n+3];
        diagonal[n+3] /= nfsubnp3;
        rhs[n+3] /= nfsubnp3;

        diagonal[n+3] += first_superdiagonal[n+2];
        rhs[n+3] += rhs[n+2];

        m_alpha.resize(n + 4, std::numeric_limits<Real>::quiet_NaN());

        m_alpha[n+3] = rhs[n+3]/diagonal[n+3];
        m_alpha[n+2] = rhs[n+2] - first_superdiagonal[n+2]*m_alpha[n+3];
        for (int64_t i = int64_t(n+1); i >= 0; --i) {
            m_alpha[i] = rhs[i] - first_superdiagonal[i]*m_alpha[i+1] - second_superdiagonal[i]*m_alpha[i+2];
        }

    }

    Real operator()(Real t) const {
        using std::ceil;
        using std::floor;
        using boost::math::cardinal_b_spline;
        // tf = t0 + (n-1)*h
        // alpha.size() = n+4
        if (t < m_t0 || t > m_t0 + (m_alpha.size()-5)/m_inv_h) {
            const char* err_msg = "Tried to evaluate the cardinal quintic b-spline outside the domain of of interpolation; extrapolation does not work.";
            throw std::domain_error(err_msg);
        }
        Real x = (t-m_t0)*m_inv_h;
        // Support of B_5 is [-3, 3]. So -3 < x - j + 2 < 3, so x-1 < j < x+5.
        // TODO: Zero pad m_alpha so that only the domain check is necessary.
        int64_t j_min = (std::max)(int64_t(0), int64_t(ceil(x-1)));
        int64_t j_max = (std::min)(int64_t(m_alpha.size() - 1), int64_t(floor(x+5)) );
        Real s = 0;
        for (int64_t j = j_min; j <= j_max; ++j) {
            // TODO: Use Cox 1972 to generate all integer translates of B5 simultaneously.
            s += m_alpha[j]*cardinal_b_spline<5, Real>(x - j + 2);
        }
        return s;
    }

    Real prime(Real t) const {
        using std::ceil;
        using std::floor;
        using boost::math::cardinal_b_spline_prime;
        if (t < m_t0 || t > m_t0 + (m_alpha.size()-5)/m_inv_h) {
            const char* err_msg = "Tried to evaluate the cardinal quintic b-spline outside the domain of of interpolation; extrapolation does not work.";
            throw std::domain_error(err_msg);
        }
        Real x = (t-m_t0)*m_inv_h;
        // Support of B_5 is [-3, 3]. So -3 < x - j + 2 < 3, so x-1 < j < x+5
        int64_t j_min = (std::max)(int64_t(0), int64_t(ceil(x-1)));
        int64_t j_max = (std::min)(int64_t(m_alpha.size() - 1), int64_t(floor(x+5)) );
        Real s = 0;
        for (int64_t j = j_min; j <= j_max; ++j) {
            s += m_alpha[j]*cardinal_b_spline_prime<5, Real>(x - j + 2);
        }
        return s*m_inv_h;

    }

    Real double_prime(Real t) const {
        using std::ceil;
        using std::floor;
        using boost::math::cardinal_b_spline_double_prime;
        if (t < m_t0 || t > m_t0 + (m_alpha.size()-5)/m_inv_h) {
            const char* err_msg = "Tried to evaluate the cardinal quintic b-spline outside the domain of of interpolation; extrapolation does not work.";
            throw std::domain_error(err_msg);
        }
        Real x = (t-m_t0)*m_inv_h;
        // Support of B_5 is [-3, 3]. So -3 < x - j + 2 < 3, so x-1 < j < x+5
        int64_t j_min = (std::max)(int64_t(0), int64_t(ceil(x-1)));
        int64_t j_max = (std::min)(int64_t(m_alpha.size() - 1), int64_t(floor(x+5)) );
        Real s = 0;
        for (int64_t j = j_min; j <= j_max; ++j) {
            s += m_alpha[j]*cardinal_b_spline_double_prime<5, Real>(x - j + 2);
        }
        return s*m_inv_h*m_inv_h;
    }


    Real t_max() const {
        return m_t0 + (m_alpha.size()-5)/m_inv_h;
    }

private:
    std::vector<Real> m_alpha;
    Real m_inv_h;
    Real m_t0;
};

}}}}
#endif
