/*
 * Copyright Nick Thompson, 2020
 * Use, modification and distribution are subject to the
 * Boost Software License, Version 1.0. (See accompanying file
 * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#ifndef BOOST_MATH_QUADRATURE_WAVELET_TRANSFORMS_HPP
#define BOOST_MATH_QUADRATURE_WAVELET_TRANSFORMS_HPP
#include <boost/math/special_functions/daubechies_wavelet.hpp>
#include <boost/math/quadrature/trapezoidal.hpp>

namespace boost::math::quadrature {

template<class F, typename Real, int p>
class daubechies_wavelet_transform
{
public:
    daubechies_wavelet_transform(F f, int grid_refinements = -1, Real tol = 100*std::numeric_limits<Real>::epsilon(),
    int max_refinements = 12) : f_{f}, psi_(grid_refinements), tol_{tol}, max_refinements_{max_refinements}
    {}

    daubechies_wavelet_transform(F f, boost::math::daubechies_wavelet<Real, p> wavelet, Real tol = 100*std::numeric_limits<Real>::epsilon(),
    int max_refinements = 12) : f_{f}, psi_{wavelet}, tol_{tol}, max_refinements_{max_refinements}
    {}

    auto operator()(Real s, Real t)->decltype(std::declval<F>()(std::declval<Real>())) const
    {
        using std::sqrt;
        using std::abs;
        using boost::math::quadrature::trapezoidal;
        auto g = [&] (Real u) {
            return f_(s*u+t)*psi_(u);
        };
        auto [a,b] = psi_.support();
        return sqrt(abs(s))*trapezoidal(g, a, b, tol_, max_refinements_);
    }

private:
    F f_;
    boost::math::daubechies_wavelet<Real, p> psi_;
    Real tol_;
    int max_refinements_;
};


}
#endif
