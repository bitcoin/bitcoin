/*
 *  Copyright Nick Thompson, 2017
 *  Use, modification and distribution are subject to the
 *  Boost Software License, Version 1.0. (See accompanying file
 *  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 *  Given N samples (t_i, y_i) which are irregularly spaced, this routine constructs an
 *  interpolant s which is constructed in O(N) time, occupies O(N) space, and can be evaluated in O(N) time.
 *  The interpolation is stable, unless one point is incredibly close to another, and the next point is incredibly far.
 *  The measure of this stability is the "local mesh ratio", which can be queried from the routine.
 *  Pictorially, the following t_i spacing is bad (has a high local mesh ratio)
 *  ||             |      | |                           |
 *  and this t_i spacing is good (has a low local mesh ratio)
 *  |   |      |    |     |    |        |    |  |    |
 *
 *
 *  If f is C^{d+2}, then the interpolant is O(h^(d+1)) accurate, where d is the interpolation order.
 *  A disadvantage of this interpolant is that it does not reproduce rational functions; for example, 1/(1+x^2) is not interpolated exactly.
 *
 *  References:
 *  Floater, Michael S., and Kai Hormann. "Barycentric rational interpolation with no poles and high rates of approximation."
*      Numerische Mathematik 107.2 (2007): 315-331.
 *  Press, William H., et al. "Numerical recipes third edition: the art of scientific computing." Cambridge University Press 32 (2007): 10013-2473.
 */

#ifndef BOOST_MATH_INTERPOLATORS_BARYCENTRIC_RATIONAL_HPP
#define BOOST_MATH_INTERPOLATORS_BARYCENTRIC_RATIONAL_HPP

#include <memory>
#include <boost/math/interpolators/detail/barycentric_rational_detail.hpp>

namespace boost{ namespace math{ namespace interpolators{

template<class Real>
class barycentric_rational
{
public:
    barycentric_rational(const Real* const x, const Real* const y, size_t n, size_t approximation_order = 3);

    barycentric_rational(std::vector<Real>&& x, std::vector<Real>&& y, size_t approximation_order = 3);

    template <class InputIterator1, class InputIterator2>
    barycentric_rational(InputIterator1 start_x, InputIterator1 end_x, InputIterator2 start_y, size_t approximation_order = 3, typename std::enable_if<!std::is_integral<InputIterator2>::value>::type* = 0);

    Real operator()(Real x) const;

    Real prime(Real x) const;

    std::vector<Real>&& return_x()
    {
        return m_imp->return_x();
    }

    std::vector<Real>&& return_y()
    {
        return m_imp->return_y();
    }

private:
    std::shared_ptr<detail::barycentric_rational_imp<Real>> m_imp;
};

template <class Real>
barycentric_rational<Real>::barycentric_rational(const Real* const x, const Real* const y, size_t n, size_t approximation_order):
 m_imp(std::make_shared<detail::barycentric_rational_imp<Real>>(x, x + n, y, approximation_order))
{
    return;
}

template <class Real>
barycentric_rational<Real>::barycentric_rational(std::vector<Real>&& x, std::vector<Real>&& y, size_t approximation_order):
 m_imp(std::make_shared<detail::barycentric_rational_imp<Real>>(std::move(x), std::move(y), approximation_order))
{
    return;
}


template <class Real>
template <class InputIterator1, class InputIterator2>
barycentric_rational<Real>::barycentric_rational(InputIterator1 start_x, InputIterator1 end_x, InputIterator2 start_y, size_t approximation_order, typename std::enable_if<!std::is_integral<InputIterator2>::value>::type*)
 : m_imp(std::make_shared<detail::barycentric_rational_imp<Real>>(start_x, end_x, start_y, approximation_order))
{
}

template<class Real>
Real barycentric_rational<Real>::operator()(Real x) const
{
    return m_imp->operator()(x);
}

template<class Real>
Real barycentric_rational<Real>::prime(Real x) const
{
    return m_imp->prime(x);
}


}}}
#endif
