// Copyright Nick Thompson, 2019
// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

/*
 * References:
 * Ooura, Takuya, and Masatake Mori. "A robust double exponential formula for Fourier-type integrals." Journal of computational and applied mathematics 112.1-2 (1999): 229-241.
 * http://www.kurims.kyoto-u.ac.jp/~ooura/intde.html
 */
#ifndef BOOST_MATH_QUADRATURE_OOURA_FOURIER_INTEGRALS_HPP
#define BOOST_MATH_QUADRATURE_OOURA_FOURIER_INTEGRALS_HPP
#include <memory>
#include <boost/math/quadrature/detail/ooura_fourier_integrals_detail.hpp>

namespace boost { namespace math { namespace quadrature {

template<class Real>
class ooura_fourier_sin {
public:
    ooura_fourier_sin(const Real relative_error_tolerance = tools::root_epsilon<Real>(), size_t levels = sizeof(Real)) : impl_(std::make_shared<detail::ooura_fourier_sin_detail<Real>>(relative_error_tolerance, levels))
    {}

    template<class F>
    std::pair<Real, Real> integrate(F const & f, Real omega) {
        return impl_->integrate(f, omega);
    }

    // These are just for debugging/unit tests:
    std::vector<std::vector<Real>> const & big_nodes() const {
        return impl_->big_nodes();
    }

    std::vector<std::vector<Real>> const & weights_for_big_nodes() const {
        return impl_->weights_for_big_nodes();
    }

    std::vector<std::vector<Real>> const & little_nodes() const {
        return impl_->little_nodes();
    }

    std::vector<std::vector<Real>> const & weights_for_little_nodes() const {
        return impl_->weights_for_little_nodes();
    }

private:
    std::shared_ptr<detail::ooura_fourier_sin_detail<Real>> impl_;
};


template<class Real>
class ooura_fourier_cos {
public:
    ooura_fourier_cos(const Real relative_error_tolerance = tools::root_epsilon<Real>(), size_t levels = sizeof(Real)) : impl_(std::make_shared<detail::ooura_fourier_cos_detail<Real>>(relative_error_tolerance, levels))
    {}

    template<class F>
    std::pair<Real, Real> integrate(F const & f, Real omega) {
        return impl_->integrate(f, omega);
    }
private:
    std::shared_ptr<detail::ooura_fourier_cos_detail<Real>> impl_;
};


}}}
#endif
