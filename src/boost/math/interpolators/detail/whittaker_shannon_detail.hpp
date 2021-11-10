// Copyright Nick Thompson, 2019
// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_MATH_INTERPOLATORS_WHITAKKER_SHANNON_DETAIL_HPP
#define BOOST_MATH_INTERPOLATORS_WHITAKKER_SHANNON_DETAIL_HPP
#include <cmath>
#include <boost/math/tools/assert.hpp>
#include <boost/math/constants/constants.hpp>
#include <boost/math/special_functions/sin_pi.hpp>
#include <boost/math/special_functions/cos_pi.hpp>

namespace boost { namespace math { namespace interpolators { namespace detail {

template<class RandomAccessContainer>
class whittaker_shannon_detail {
public:

    using Real = typename RandomAccessContainer::value_type;
    whittaker_shannon_detail(RandomAccessContainer&& y, Real const & t0, Real const & h) : m_y{std::move(y)}, m_t0{t0}, m_h{h}
    {
        for (size_t i = 1; i < m_y.size(); i += 2)
        {
            m_y[i] = -m_y[i];
        }
    }

    inline Real operator()(Real t) const {
        using boost::math::constants::pi;
        using std::isfinite;
        using std::floor;
        Real y = 0;
        Real x = (t - m_t0)/m_h;
        Real z = x;
        auto it = m_y.begin();

        // For some reason, neither clang nor g++ will cache the address of m_y.end() in a register.
        // Hence make a copy of it:
        auto end = m_y.end();
        while(it != end)
        {

            y += *it++/z;
            z -= 1;
        }

        if (!isfinite(y))
        {
            BOOST_MATH_ASSERT_MSG(floor(x) == ceil(x), "Floor and ceiling should be equal.\n");
            size_t i = static_cast<size_t>(floor(x));
            if (i & 1)
            {
                return -m_y[i];
            }
            return m_y[i];
        }
        return y*boost::math::sin_pi(x)/pi<Real>();
    }

    Real prime(Real t) const {
        using boost::math::constants::pi;
        using std::isfinite;
        using std::floor;

        Real x = (t - m_t0)/m_h;
        if (ceil(x) == x) {
            Real s = 0;
            long j = static_cast<long>(x);
            long n = m_y.size();
            for (long i = 0; i < n; ++i)
            {
                if (j - i != 0)
                {
                    s += m_y[i]/(j-i);
                }
                // else derivative of sinc at zero is zero.
            }
            if (j & 1) {
                s /= -m_h;
            } else {
                s /= m_h;
            }
            return s;
        }
        Real z = x;
        auto it = m_y.begin();
        Real cospix = boost::math::cos_pi(x);
        Real sinpix_div_pi = boost::math::sin_pi(x)/pi<Real>();

        Real s = 0;
        auto end = m_y.end();
        while(it != end)
        {
            s += (*it++)*(z*cospix - sinpix_div_pi)/(z*z);
            z -= 1;
        }

        return s/m_h;
    }



    Real operator[](size_t i) const {
        if (i & 1)
        {
            return -m_y[i];
        }
        return m_y[i];
    }

    RandomAccessContainer&& return_data() {
        for (size_t i = 1; i < m_y.size(); i += 2)
        {
            m_y[i] = -m_y[i];
        }
        return std::move(m_y);
    }


private:
    RandomAccessContainer m_y;
    Real m_t0;
    Real m_h;
};
}}}}
#endif
