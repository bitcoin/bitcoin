/*
 *  Copyright Nick Thompson, 2017
 *  Use, modification and distribution are subject to the
 *  Boost Software License, Version 1.0. (See accompanying file
 *  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_MATH_INTERPOLATORS_BARYCENTRIC_RATIONAL_DETAIL_HPP
#define BOOST_MATH_INTERPOLATORS_BARYCENTRIC_RATIONAL_DETAIL_HPP

#include <vector>
#include <utility> // for std::move
#include <algorithm> // for std::is_sorted
#include <string>
#include <boost/math/special_functions/fpclassify.hpp>
#include <boost/math/tools/assert.hpp>

namespace boost{ namespace math{ namespace interpolators { namespace detail{

template<class Real>
class barycentric_rational_imp
{
public:
    template <class InputIterator1, class InputIterator2>
    barycentric_rational_imp(InputIterator1 start_x, InputIterator1 end_x, InputIterator2 start_y, size_t approximation_order = 3);

    barycentric_rational_imp(std::vector<Real>&& x, std::vector<Real>&& y, size_t approximation_order = 3);

    Real operator()(Real x) const;

    Real prime(Real x) const;

    // The barycentric weights are not really that interesting; except to the unit tests!
    Real weight(size_t i) const { return m_w[i]; }

    std::vector<Real>&& return_x()
    {
        return std::move(m_x);
    }

    std::vector<Real>&& return_y()
    {
        return std::move(m_y);
    }

private:

    void calculate_weights(size_t approximation_order);

    std::vector<Real> m_x;
    std::vector<Real> m_y;
    std::vector<Real> m_w;
};

template <class Real>
template <class InputIterator1, class InputIterator2>
barycentric_rational_imp<Real>::barycentric_rational_imp(InputIterator1 start_x, InputIterator1 end_x, InputIterator2 start_y, size_t approximation_order)
{
    std::ptrdiff_t n = std::distance(start_x, end_x);

    if (approximation_order >= (std::size_t)n)
    {
        throw std::domain_error("Approximation order must be < data length.");
    }

    // Big sad memcpy.
    m_x.resize(n);
    m_y.resize(n);
    for(unsigned i = 0; start_x != end_x; ++start_x, ++start_y, ++i)
    {
        // But if we're going to do a memcpy, we can do some error checking which is inexpensive relative to the copy:
        if(boost::math::isnan(*start_x))
        {
            std::string msg = std::string("x[") + std::to_string(i) + "] is a NAN";
            throw std::domain_error(msg);
        }

        if(boost::math::isnan(*start_y))
        {
           std::string msg = std::string("y[") + std::to_string(i) + "] is a NAN";
           throw std::domain_error(msg);
        }

        m_x[i] = *start_x;
        m_y[i] = *start_y;
    }
    calculate_weights(approximation_order);
}

template <class Real>
barycentric_rational_imp<Real>::barycentric_rational_imp(std::vector<Real>&& x, std::vector<Real>&& y,size_t approximation_order) : m_x(std::move(x)), m_y(std::move(y))
{
    BOOST_MATH_ASSERT_MSG(m_x.size() == m_y.size(), "There must be the same number of abscissas and ordinates.");
    BOOST_MATH_ASSERT_MSG(approximation_order < m_x.size(), "Approximation order must be < data length.");
    BOOST_MATH_ASSERT_MSG(std::is_sorted(m_x.begin(), m_x.end()), "The abscissas must be listed in increasing order x[0] < x[1] < ... < x[n-1].");
    calculate_weights(approximation_order);
}

template<class Real>
void barycentric_rational_imp<Real>::calculate_weights(size_t approximation_order)
{
    using std::abs;
    int64_t n = m_x.size();
    m_w.resize(n, 0);
    for(int64_t k = 0; k < n; ++k)
    {
        int64_t i_min = (std::max)(k - (int64_t) approximation_order, (int64_t) 0);
        int64_t i_max = k;
        if (k >= n - (std::ptrdiff_t)approximation_order)
        {
            i_max = n - approximation_order - 1;
        }

        for(int64_t i = i_min; i <= i_max; ++i)
        {
            Real inv_product = 1;
            int64_t j_max = (std::min)(static_cast<int64_t>(i + approximation_order), static_cast<int64_t>(n - 1));
            for(int64_t j = i; j <= j_max; ++j)
            {
                if (j == k)
                {
                    continue;
                }

                Real diff = m_x[k] - m_x[j];
                using std::numeric_limits;
                if (abs(diff) < (numeric_limits<Real>::min)())
                {
                   std::string msg = std::string("Spacing between  x[")
                      + std::to_string(k) + std::string("] and x[")
                      + std::to_string(i) + std::string("] is ")
                      + std::string("smaller than the epsilon of ")
                      + std::string(typeid(Real).name());
                    throw std::logic_error(msg);
                }
                inv_product *= diff;
            }
            if (i % 2 == 0)
            {
                m_w[k] += 1/inv_product;
            }
            else
            {
                m_w[k] -= 1/inv_product;
            }
        }
    }
}


template<class Real>
Real barycentric_rational_imp<Real>::operator()(Real x) const
{
    Real numerator = 0;
    Real denominator = 0;
    for(size_t i = 0; i < m_x.size(); ++i)
    {
        // Presumably we should see if the accuracy is improved by using ULP distance of say, 5 here, instead of testing for floating point equality.
        // However, it has been shown that if x approx x_i, but x != x_i, then inaccuracy in the numerator cancels the inaccuracy in the denominator,
        // and the result is fairly accurate. See: http://epubs.siam.org/doi/pdf/10.1137/S0036144502417715
        if (x == m_x[i])
        {
            return m_y[i];
        }
        Real t = m_w[i]/(x - m_x[i]);
        numerator += t*m_y[i];
        denominator += t;
    }
    return numerator/denominator;
}

/*
 * A formula for computing the derivative of the barycentric representation is given in
 * "Some New Aspects of Rational Interpolation", by Claus Schneider and Wilhelm Werner,
 * Mathematics of Computation, v47, number 175, 1986.
 * http://www.ams.org/journals/mcom/1986-47-175/S0025-5718-1986-0842136-8/S0025-5718-1986-0842136-8.pdf
 * and reviewed in
 * Recent developments in barycentric rational interpolation
 * Jean-Paul Berrut, Richard Baltensperger and Hans D. Mittelmann
 *
 * Is it possible to complete this in one pass through the data?
 */

template<class Real>
Real barycentric_rational_imp<Real>::prime(Real x) const
{
    Real rx = this->operator()(x);
    Real numerator = 0;
    Real denominator = 0;
    for(size_t i = 0; i < m_x.size(); ++i)
    {
        if (x == m_x[i])
        {
            Real sum = 0;
            for (size_t j = 0; j < m_x.size(); ++j)
            {
                if (j == i)
                {
                    continue;
                }
                sum += m_w[j]*(m_y[i] - m_y[j])/(m_x[i] - m_x[j]);
            }
            return -sum/m_w[i];
        }
        Real t = m_w[i]/(x - m_x[i]);
        Real diff = (rx - m_y[i])/(x-m_x[i]);
        numerator += t*diff;
        denominator += t;
    }

    return numerator/denominator;
}
}}}}
#endif
