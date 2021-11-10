//  (C) Copyright Nick Thompson 2019.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_INTERPOLATORS_CARDINAL_TRIGONOMETRIC_HPP
#define BOOST_MATH_INTERPOLATORS_CARDINAL_TRIGONOMETRIC_HPP
#include <memory>
#include <boost/math/interpolators/detail/cardinal_trigonometric_detail.hpp>

namespace boost { namespace math { namespace interpolators {

template<class RandomAccessContainer>
class cardinal_trigonometric
{
public:
    using Real = typename RandomAccessContainer::value_type;
    cardinal_trigonometric(RandomAccessContainer const & v, Real t0, Real h)
    {
        m_impl = std::make_shared<interpolators::detail::cardinal_trigonometric_detail<Real>>(v.data(), v.size(), t0, h);
    }

    Real operator()(Real t) const
    {
        return m_impl->operator()(t);
    }

    Real prime(Real t) const
    {
        return m_impl->prime(t);
    }

    Real double_prime(Real t) const
    {
        return m_impl->double_prime(t);
    }

    Real period() const
    {
        return m_impl->period();
    }

    Real integrate() const
    {
        return m_impl->integrate();
    }

    Real squared_l2() const
    {
        return m_impl->squared_l2();
    }

private:
    std::shared_ptr<interpolators::detail::cardinal_trigonometric_detail<Real>> m_impl;
};

}}}
#endif
