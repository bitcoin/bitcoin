// Copyright Nick Thompson 2017.
// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_SPECIAL_LEGENDRE_STIELTJES_HPP
#define BOOST_MATH_SPECIAL_LEGENDRE_STIELTJES_HPP

/*
 * Constructs the Legendre-Stieltjes polynomial of degree m.
 * The Legendre-Stieltjes polynomials are used to create extensions for Gaussian quadratures,
 * commonly called "Gauss-Konrod" quadratures.
 *
 * References:
 * Patterson, TNL. "The optimum addition of points to quadrature formulae." Mathematics of Computation 22.104 (1968): 847-856.
 */

#include <iostream>
#include <vector>
#include <boost/math/tools/roots.hpp>
#include <boost/math/special_functions/legendre.hpp>

namespace boost{
namespace math{

template<class Real>
class legendre_stieltjes
{
public:
    legendre_stieltjes(size_t m)
    {
        if (m == 0)
        {
           throw std::domain_error("The Legendre-Stieltjes polynomial is defined for order m > 0.\n");
        }
        m_m = static_cast<int>(m);
        std::ptrdiff_t n = m - 1;
        std::ptrdiff_t q;
        std::ptrdiff_t r;
        bool odd = n & 1;
        if (odd)
        {
           q = 1;
           r = (n-1)/2 + 2;
        }
        else
        {
           q = 0;
           r = n/2 + 1;
        }
        m_a.resize(r + 1);
        // We'll keep the ones-based indexing at the cost of storing a superfluous element
        // so that we can follow Patterson's notation exactly.
        m_a[r] = static_cast<Real>(1);
        // Make sure using the zero index is a bug:
        m_a[0] = std::numeric_limits<Real>::quiet_NaN();

        for (std::ptrdiff_t k = 1; k < r; ++k)
        {
            Real ratio = 1;
            m_a[r - k] = 0;
            for (std::ptrdiff_t i = r + 1 - k; i <= r; ++i)
            {
                // See Patterson, equation 12
                std::ptrdiff_t num = (n - q + 2*(i + k - 1))*(n + q + 2*(k - i + 1))*(n-1-q+2*(i-k))*(2*(k+i-1) -1 -q -n);
                std::ptrdiff_t den = (n - q + 2*(i - k))*(2*(k + i - 1) - q - n)*(n + 1 + q + 2*(k - i))*(n - 1 - q + 2*(i + k));
                ratio *= static_cast<Real>(num)/static_cast<Real>(den);
                m_a[r - k] -= ratio*m_a[i];
            }
        }
    }


    Real norm_sq() const
    {
        Real t = 0;
        bool odd = m_m & 1;
        for (size_t i = 1; i < m_a.size(); ++i)
        {
            if(odd)
            {
                t += 2*m_a[i]*m_a[i]/static_cast<Real>(4*i-1);
            }
            else
            {
                t += 2*m_a[i]*m_a[i]/static_cast<Real>(4*i-3);
            }
        }
        return t;
    }


    Real operator()(Real x) const
    {
        // Trivial implementation:
        // Em += m_a[i]*legendre_p(2*i - 1, x);  m odd
        // Em += m_a[i]*legendre_p(2*i - 2, x);  m even
        size_t r = m_a.size() - 1;
        Real p0 = 1;
        Real p1 = x;

        Real Em;
        bool odd = m_m & 1;
        if (odd)
        {
            Em = m_a[1]*p1;
        }
        else
        {
            Em = m_a[1]*p0;
        }

        unsigned n = 1;
        for (size_t i = 2; i <= r; ++i)
        {
            std::swap(p0, p1);
            p1 = boost::math::legendre_next(n, x, p0, p1);
            ++n;
            if (!odd)
            {
               Em += m_a[i]*p1;
            }
            std::swap(p0, p1);
            p1 = boost::math::legendre_next(n, x, p0, p1);
            ++n;
            if(odd)
            {
                Em += m_a[i]*p1;
            }
        }
        return Em;
    }


    Real prime(Real x) const
    {
        Real Em_prime = 0;

        for (size_t i = 1; i < m_a.size(); ++i)
        {
            if(m_m & 1)
            {
                Em_prime += m_a[i]*detail::legendre_p_prime_imp(static_cast<unsigned>(2*i - 1), x, policies::policy<>());
            }
            else
            {
                Em_prime += m_a[i]*detail::legendre_p_prime_imp(static_cast<unsigned>(2*i - 2), x, policies::policy<>());
            }
        }
        return Em_prime;
    }

    std::vector<Real> zeros() const
    {
        using boost::math::constants::half;

        std::vector<Real> stieltjes_zeros;
        std::vector<Real> legendre_zeros = legendre_p_zeros<Real>(m_m - 1);
        int k;
        if (m_m & 1)
        {
            stieltjes_zeros.resize(legendre_zeros.size() + 1, std::numeric_limits<Real>::quiet_NaN());
            stieltjes_zeros[0] = 0;
            k = 1;
        }
        else
        {
            stieltjes_zeros.resize(legendre_zeros.size(), std::numeric_limits<Real>::quiet_NaN());
            k = 0;
        }

        while (k < (int)stieltjes_zeros.size())
        {
            Real lower_bound;
            Real upper_bound;
            if (m_m & 1)
            {
                lower_bound = legendre_zeros[k - 1];
                if (k == (int)legendre_zeros.size())
                {
                    upper_bound = 1;
                }
                else
                {
                    upper_bound = legendre_zeros[k];
                }
            }
            else
            {
                lower_bound = legendre_zeros[k];
                if (k == (int)legendre_zeros.size() - 1)
                {
                    upper_bound = 1;
                }
                else
                {
                    upper_bound = legendre_zeros[k+1];
                }
            }

            // The root bracketing is not very tight; to keep weird stuff from happening
            // in the Newton's method, let's tighten up the tolerance using a few bisections.
            boost::math::tools::eps_tolerance<Real> tol(6);
            auto g = [&](Real t) { return this->operator()(t); };
            auto p = boost::math::tools::bisect(g, lower_bound, upper_bound, tol);

            Real x_nk_guess = p.first + (p.second - p.first)*half<Real>();
            std::uintmax_t number_of_iterations = 500;

            auto f = [&] (Real x) { Real Pn = this->operator()(x);
                                    Real Pn_prime = this->prime(x);
                                    return std::pair<Real, Real>(Pn, Pn_prime); };

            const Real x_nk = boost::math::tools::newton_raphson_iterate(f, x_nk_guess,
                                                  p.first, p.second,
                                                  tools::digits<Real>(),
                                                  number_of_iterations);

            BOOST_MATH_ASSERT(p.first < x_nk);
            BOOST_MATH_ASSERT(x_nk < p.second);
            stieltjes_zeros[k] = x_nk;
            ++k;
        }
        return stieltjes_zeros;
    }

private:
    // Coefficients of Legendre expansion
    std::vector<Real> m_a;
    int m_m;
};

}}
#endif
