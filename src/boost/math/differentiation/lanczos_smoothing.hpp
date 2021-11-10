//  (C) Copyright Nick Thompson 2019.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_DIFFERENTIATION_LANCZOS_SMOOTHING_HPP
#define BOOST_MATH_DIFFERENTIATION_LANCZOS_SMOOTHING_HPP
#include <cmath> // for std::abs
#include <cstddef>
#include <limits> // to nan initialize
#include <vector>
#include <string>
#include <stdexcept>
#include <type_traits>
#include <boost/math/tools/assert.hpp>

namespace boost::math::differentiation {

namespace detail {
template <typename Real>
class discrete_legendre {
  public:
    explicit discrete_legendre(std::size_t n, Real x) : m_n{n}, m_r{2}, m_x{x},
                                                        m_qrm2{1}, m_qrm1{x},
                                                        m_qrm2p{0}, m_qrm1p{1},
                                                        m_qrm2pp{0}, m_qrm1pp{0}
    {
        using std::abs;
        BOOST_MATH_ASSERT_MSG(abs(m_x) <= 1, "Three term recurrence is stable only for |x| <=1.");
        // The integer n indexes a family of discrete Legendre polynomials indexed by k <= 2*n
    }

    Real norm_sq(int r) const
    {
        Real prod = Real(2) / Real(2 * r + 1);
        for (int k = -r; k <= r; ++k) {
            prod *= Real(2 * m_n + 1 + k) / Real(2 * m_n);
        }
        return prod;
    }

    Real next()
    {
        Real N = 2 * m_n + 1;
        Real num = (m_r - 1) * (N * N - (m_r - 1) * (m_r - 1)) * m_qrm2;
        Real tmp = (2 * m_r - 1) * m_x * m_qrm1 - num / Real(4 * m_n * m_n);
        m_qrm2 = m_qrm1;
        m_qrm1 = tmp / m_r;
        ++m_r;
        return m_qrm1;
    }

    Real next_prime()
    {
        Real N = 2 * m_n + 1;
        Real s = (m_r - 1) * (N * N - (m_r - 1) * (m_r - 1)) / Real(4 * m_n * m_n);
        Real tmp1 = ((2 * m_r - 1) * m_x * m_qrm1 - s * m_qrm2) / m_r;
        Real tmp2 = ((2 * m_r - 1) * (m_qrm1 + m_x * m_qrm1p) - s * m_qrm2p) / m_r;
        m_qrm2 = m_qrm1;
        m_qrm1 = tmp1;
        m_qrm2p = m_qrm1p;
        m_qrm1p = tmp2;
        ++m_r;
        return m_qrm1p;
    }

    Real next_dbl_prime()
    {
        Real N = 2*m_n + 1;
        Real trm1 = 2*m_r - 1;
        Real s = (m_r - 1) * (N * N - (m_r - 1) * (m_r - 1)) / Real(4 * m_n * m_n);
        Real rqrpp = 2*trm1*m_qrm1p + trm1*m_x*m_qrm1pp - s*m_qrm2pp;
        Real tmp1 = ((2 * m_r - 1) * m_x * m_qrm1 - s * m_qrm2) / m_r;
        Real tmp2 = ((2 * m_r - 1) * (m_qrm1 + m_x * m_qrm1p) - s * m_qrm2p) / m_r;
        m_qrm2 = m_qrm1;
        m_qrm1 = tmp1;
        m_qrm2p = m_qrm1p;
        m_qrm1p = tmp2;
        m_qrm2pp = m_qrm1pp;
        m_qrm1pp = rqrpp/m_r;
        ++m_r;
        return m_qrm1pp;
    }

    Real operator()(Real x, std::size_t k)
    {
        BOOST_MATH_ASSERT_MSG(k <= 2 * m_n, "r <= 2n is required.");
        if (k == 0)
        {
            return 1;
        }
        if (k == 1)
        {
            return x;
        }
        Real qrm2 = 1;
        Real qrm1 = x;
        Real N = 2 * m_n + 1;
        for (std::size_t r = 2; r <= k; ++r) {
            Real num = (r - 1) * (N * N - (r - 1) * (r - 1)) * qrm2;
            Real tmp = (2 * r - 1) * x * qrm1 - num / Real(4 * m_n * m_n);
            qrm2 = qrm1;
            qrm1 = tmp / r;
        }
        return qrm1;
    }

    Real prime(Real x, std::size_t k) {
        BOOST_MATH_ASSERT_MSG(k <= 2 * m_n, "r <= 2n is required.");
        if (k == 0) {
            return 0;
        }
        if (k == 1) {
            return 1;
        }
        Real qrm2 = 1;
        Real qrm1 = x;
        Real qrm2p = 0;
        Real qrm1p = 1;
        Real N = 2 * m_n + 1;
        for (std::size_t r = 2; r <= k; ++r) {
            Real s =
                (r - 1) * (N * N - (r - 1) * (r - 1)) / Real(4 * m_n * m_n);
            Real tmp1 = ((2 * r - 1) * x * qrm1 - s * qrm2) / r;
            Real tmp2 = ((2 * r - 1) * (qrm1 + x * qrm1p) - s * qrm2p) / r;
            qrm2 = qrm1;
            qrm1 = tmp1;
            qrm2p = qrm1p;
            qrm1p = tmp2;
        }
        return qrm1p;
    }

  private:
    std::size_t m_n;
    std::size_t m_r;
    Real m_x;
    Real m_qrm2;
    Real m_qrm1;
    Real m_qrm2p;
    Real m_qrm1p;
    Real m_qrm2pp;
    Real m_qrm1pp;
};

template <class Real>
std::vector<Real> interior_velocity_filter(std::size_t n, std::size_t p) {
    auto dlp = discrete_legendre<Real>(n, 0);
    std::vector<Real> coeffs(p+1);
    coeffs[1] = 1/dlp.norm_sq(1);
    for (std::size_t l = 3; l < p + 1; l += 2)
    {
        dlp.next_prime();
        coeffs[l] = dlp.next_prime()/ dlp.norm_sq(l);
    }

    // We could make the filter length n, as f[0] = 0,
    // but that'd make the indexing awkward when applying the filter.
    std::vector<Real> f(n + 1);
    // This value should never be read, but this is the correct value *if it is read*.
    // Hmm, should it be a nan then? I'm not gonna agonize.
    f[0] = 0;
    for (std::size_t j = 1; j < f.size(); ++j)
    {
        Real arg = Real(j) / Real(n);
        dlp = discrete_legendre<Real>(n, arg);
        f[j] = coeffs[1]*arg;
        for (std::size_t l = 3; l <= p; l += 2)
        {
            dlp.next();
            f[j] += coeffs[l]*dlp.next();
        }
        f[j] /= (n * n);
    }
    return f;
}

template <class Real>
std::vector<Real> boundary_velocity_filter(std::size_t n, std::size_t p, int64_t s)
{
    std::vector<Real> coeffs(p+1, std::numeric_limits<Real>::quiet_NaN());
    Real sn = Real(s) / Real(n);
    auto dlp = discrete_legendre<Real>(n, sn);
    coeffs[0] = 0;
    coeffs[1] = 1/dlp.norm_sq(1);
    for (std::size_t l = 2; l < p + 1; ++l)
    {
        // Calculation of the norms is common to all filters,
        // so it seems like an obvious optimization target.
        // I tried this: The spent in computing the norms time is not negligible,
        // but still a small fraction of the total compute time.
        // Hence I'm not refactoring out these norm calculations.
        coeffs[l] = dlp.next_prime()/ dlp.norm_sq(l);
    }

    std::vector<Real> f(2*n + 1);
    for (std::size_t k = 0; k < f.size(); ++k)
    {
        Real j = Real(k) - Real(n);
        Real arg = j/Real(n);
        dlp = discrete_legendre<Real>(n, arg);
        f[k] = coeffs[1]*arg;
        for (std::size_t l = 2; l <= p; ++l)
        {
            f[k] += coeffs[l]*dlp.next();
        }
        f[k] /= (n * n);
    }
    return f;
}

template <class Real>
std::vector<Real> acceleration_filter(std::size_t n, std::size_t p, int64_t s)
{
    BOOST_MATH_ASSERT_MSG(p <= 2*n, "Approximation order must be <= 2*n");
    BOOST_MATH_ASSERT_MSG(p > 2, "Approximation order must be > 2");

    std::vector<Real> coeffs(p+1, std::numeric_limits<Real>::quiet_NaN());
    Real sn = Real(s) / Real(n);
    auto dlp = discrete_legendre<Real>(n, sn);
    coeffs[0] = 0;
    coeffs[1] = 0;
    for (std::size_t l = 2; l < p + 1; ++l)
    {
        coeffs[l] = dlp.next_dbl_prime()/ dlp.norm_sq(l);
    }

    std::vector<Real> f(2*n + 1, 0);
    for (std::size_t k = 0; k < f.size(); ++k)
    {
        Real j = Real(k) - Real(n);
        Real arg = j/Real(n);
        dlp = discrete_legendre<Real>(n, arg);
        for (std::size_t l = 2; l <= p; ++l)
        {
            f[k] += coeffs[l]*dlp.next();
        }
        f[k] /= (n * n * n);
    }
    return f;
}


} // namespace detail

template <typename Real, std::size_t order = 1>
class discrete_lanczos_derivative {
public:
    discrete_lanczos_derivative(Real const & spacing,
                                std::size_t n = 18,
                                std::size_t approximation_order = 3)
        : m_dt{spacing}
    {
        static_assert(!std::is_integral_v<Real>,
                      "Spacing must be a floating point type.");
        BOOST_MATH_ASSERT_MSG(spacing > 0,
                         "Spacing between samples must be > 0.");

        if constexpr (order == 1)
        {
            BOOST_MATH_ASSERT_MSG(approximation_order <= 2 * n,
                             "The approximation order must be <= 2n");
            BOOST_MATH_ASSERT_MSG(approximation_order >= 2,
                             "The approximation order must be >= 2");

            if constexpr (std::is_same_v<Real, float> || std::is_same_v<Real, double>)
            {
                auto interior = detail::interior_velocity_filter<long double>(n, approximation_order);
                m_f.resize(interior.size());
                for (std::size_t j = 0; j < interior.size(); ++j)
                {
                    m_f[j] = static_cast<Real>(interior[j])/m_dt;
                }
            }
            else
            {
                m_f = detail::interior_velocity_filter<Real>(n, approximation_order);
                for (auto & x : m_f)
                {
                    x /= m_dt;
                }
            }

            m_boundary_filters.resize(n);
            // This for loop is a natural candidate for parallelization.
            // But does it matter? Probably not.
            for (std::size_t i = 0; i < n; ++i)
            {
                if constexpr (std::is_same_v<Real, float> || std::is_same_v<Real, double>)
                {
                    int64_t s = static_cast<int64_t>(i) - static_cast<int64_t>(n);
                    auto bf = detail::boundary_velocity_filter<long double>(n, approximation_order, s);
                    m_boundary_filters[i].resize(bf.size());
                    for (std::size_t j = 0; j < bf.size(); ++j)
                    {
                        m_boundary_filters[i][j] = static_cast<Real>(bf[j])/m_dt;
                    }
                }
                else
                {
                    int64_t s = static_cast<int64_t>(i) - static_cast<int64_t>(n);
                    m_boundary_filters[i] = detail::boundary_velocity_filter<Real>(n, approximation_order, s);
                    for (auto & bf : m_boundary_filters[i])
                    {
                        bf /= m_dt;
                    }
                }
            }
        }
        else if constexpr (order == 2)
        {
            // High precision isn't warranted for small p; only for large p.
            // (The computation appears stable for large n.)
            // But given that the filters are reusable for many vectors,
            // it's better to do a high precision computation and then cast back,
            // since the resulting cost is a factor of 2, and the cost of the filters not working is hours of debugging.
            if constexpr (std::is_same_v<Real, double> || std::is_same_v<Real, float>)
            {
                auto f = detail::acceleration_filter<long double>(n, approximation_order, 0);
                m_f.resize(n+1);
                for (std::size_t i = 0; i < m_f.size(); ++i)
                {
                    m_f[i] = static_cast<Real>(f[i+n])/(m_dt*m_dt);
                }
                m_boundary_filters.resize(n);
                for (std::size_t i = 0; i < n; ++i)
                {
                    int64_t s = static_cast<int64_t>(i) - static_cast<int64_t>(n);
                    auto bf = detail::acceleration_filter<long double>(n, approximation_order, s);
                    m_boundary_filters[i].resize(bf.size());
                    for (std::size_t j = 0; j < bf.size(); ++j)
                    {
                        m_boundary_filters[i][j] = static_cast<Real>(bf[j])/(m_dt*m_dt);
                    }
                }
            }
            else
            {
                // Given that the purpose is denoising, for higher precision calculations,
                // the default precision should be fine.
                auto f = detail::acceleration_filter<Real>(n, approximation_order, 0);
                m_f.resize(n+1);
                for (std::size_t i = 0; i < m_f.size(); ++i)
                {
                    m_f[i] = f[i+n]/(m_dt*m_dt);
                }
                m_boundary_filters.resize(n);
                for (std::size_t i = 0; i < n; ++i)
                {
                    int64_t s = static_cast<int64_t>(i) - static_cast<int64_t>(n);
                    m_boundary_filters[i] = detail::acceleration_filter<Real>(n, approximation_order, s);
                    for (auto & bf : m_boundary_filters[i])
                    {
                        bf /= (m_dt*m_dt);
                    }
                }
            }
        }
        else
        {
            BOOST_MATH_ASSERT_MSG(false, "Derivatives of order 3 and higher are not implemented.");
        }
    }

    Real get_spacing() const
    {
        return m_dt;
    }

    template<class RandomAccessContainer>
    Real operator()(RandomAccessContainer const & v, std::size_t i) const
    {
        static_assert(std::is_same_v<typename RandomAccessContainer::value_type, Real>,
                      "The type of the values in the vector provided does not match the type in the filters.");

        BOOST_MATH_ASSERT_MSG(std::size(v) >= m_boundary_filters[0].size(),
            "Vector must be at least as long as the filter length");

        if constexpr (order==1)
        {
            if (i >= m_f.size() - 1 && i <= std::size(v) - m_f.size())
            {
                // The filter has length >= 1:
                Real dvdt = m_f[1] * (v[i + 1] - v[i - 1]);
                for (std::size_t j = 2; j < m_f.size(); ++j)
                {
                    dvdt += m_f[j] * (v[i + j] - v[i - j]);
                }
                return dvdt;
            }

            // m_f.size() = N+1
            if (i < m_f.size() - 1)
            {
                auto &bf = m_boundary_filters[i];
                Real dvdt = bf[0]*v[0];
                for (std::size_t j = 1; j < bf.size(); ++j)
                {
                    dvdt += bf[j] * v[j];
                }
                return dvdt;
            }

            if (i > std::size(v) - m_f.size() && i < std::size(v))
            {
                int k = std::size(v) - 1 - i;
                auto &bf = m_boundary_filters[k];
                Real dvdt = bf[0]*v[std::size(v)-1];
                for (std::size_t j = 1; j < bf.size(); ++j)
                {
                    dvdt += bf[j] * v[std::size(v) - 1 - j];
                }
                return -dvdt;
            }
        }
        else if constexpr (order==2)
        {
            if (i >= m_f.size() - 1 && i <= std::size(v) - m_f.size())
            {
                Real d2vdt2 = m_f[0]*v[i];
                for (std::size_t j = 1; j < m_f.size(); ++j)
                {
                    d2vdt2 += m_f[j] * (v[i + j] + v[i - j]);
                }
                return d2vdt2;
            }

            // m_f.size() = N+1
            if (i < m_f.size() - 1)
            {
                auto &bf = m_boundary_filters[i];
                Real d2vdt2 = bf[0]*v[0];
                for (std::size_t j = 1; j < bf.size(); ++j)
                {
                    d2vdt2 += bf[j] * v[j];
                }
                return d2vdt2;
            }

            if (i > std::size(v) - m_f.size() && i < std::size(v))
            {
                int k = std::size(v) - 1 - i;
                auto &bf = m_boundary_filters[k];
                Real d2vdt2 = bf[0] * v[std::size(v) - 1];
                for (std::size_t j = 1; j < bf.size(); ++j)
                {
                    d2vdt2 += bf[j] * v[std::size(v) - 1 - j];
                }
                return d2vdt2;
            }
        }

        // OOB access:
        std::string msg = "Out of bounds access in Lanczos derivative.";
        msg += "Input vector has length " + std::to_string(std::size(v)) + ", but user requested access at index " + std::to_string(i) + ".";
        throw std::out_of_range(msg);
        return std::numeric_limits<Real>::quiet_NaN();
    }

    template<class RandomAccessContainer>
    void operator()(RandomAccessContainer const & v, RandomAccessContainer & w) const
    {
        static_assert(std::is_same_v<typename RandomAccessContainer::value_type, Real>,
                      "The type of the values in the vector provided does not match the type in the filters.");
        if (&w[0] == &v[0])
        {
            throw std::logic_error("This transform cannot be performed in-place.");
        }

        if (std::size(v) < m_boundary_filters[0].size())
        {
            std::string msg = "The input vector must be at least as long as the filter length. ";
            msg += "The input vector has length = " + std::to_string(std::size(v)) + ", the filter has length " + std::to_string(m_boundary_filters[0].size());
            throw std::length_error(msg);
        }

        if (std::size(w) < std::size(v))
        {
            std::string msg = "The output vector (containing the derivative) must be at least as long as the input vector.";
            msg += "The output vector has length = " + std::to_string(std::size(w)) + ", the input vector has length " + std::to_string(std::size(v));
            throw std::length_error(msg);
        }

        if constexpr (order==1)
        {
            for (std::size_t i = 0; i < m_f.size() - 1; ++i)
            {
                auto &bf = m_boundary_filters[i];
                Real dvdt = bf[0] * v[0];
                for (std::size_t j = 1; j < bf.size(); ++j)
                {
                    dvdt += bf[j] * v[j];
                }
                w[i] = dvdt;
            }

            for(std::size_t i = m_f.size() - 1; i <= std::size(v) - m_f.size(); ++i)
            {
                Real dvdt = m_f[1] * (v[i + 1] - v[i - 1]);
                for (std::size_t j = 2; j < m_f.size(); ++j)
                {
                    dvdt += m_f[j] *(v[i + j] - v[i - j]);
                }
                w[i] = dvdt;
            }


            for(std::size_t i = std::size(v) - m_f.size() + 1; i < std::size(v); ++i)
            {
                int k = std::size(v) - 1 - i;
                auto &f = m_boundary_filters[k];
                Real dvdt = f[0] * v[std::size(v) - 1];;
                for (std::size_t j = 1; j < f.size(); ++j)
                {
                    dvdt += f[j] * v[std::size(v) - 1 - j];
                }
                w[i] = -dvdt;
            }
        }
        else if constexpr (order==2)
        {
            // m_f.size() = N+1
            for (std::size_t i = 0; i < m_f.size() - 1; ++i)
            {
                auto &bf = m_boundary_filters[i];
                Real d2vdt2 = 0;
                for (std::size_t j = 0; j < bf.size(); ++j)
                {
                    d2vdt2 += bf[j] * v[j];
                }
                w[i] = d2vdt2;
            }

            for (std::size_t i = m_f.size() - 1; i <= std::size(v) - m_f.size(); ++i)
            {
                Real d2vdt2 = m_f[0]*v[i];
                for (std::size_t j = 1; j < m_f.size(); ++j)
                {
                    d2vdt2 += m_f[j] * (v[i + j] + v[i - j]);
                }
                w[i] = d2vdt2;
            }

            for (std::size_t i = std::size(v) - m_f.size() + 1; i < std::size(v); ++i)
            {
                int k = std::size(v) - 1 - i;
                auto &bf = m_boundary_filters[k];
                Real d2vdt2 = bf[0] * v[std::size(v) - 1];
                for (std::size_t j = 1; j < bf.size(); ++j)
                {
                    d2vdt2 += bf[j] * v[std::size(v) - 1 - j];
                }
                w[i] = d2vdt2;
            }
        }
    }

    template<class RandomAccessContainer>
    RandomAccessContainer operator()(RandomAccessContainer const & v) const
    {
        RandomAccessContainer w(std::size(v));
        this->operator()(v, w);
        return w;
    }


    // Don't copy; too big.
    discrete_lanczos_derivative( const discrete_lanczos_derivative & ) = delete;
    discrete_lanczos_derivative& operator=(const discrete_lanczos_derivative&) = delete;

    // Allow moves:
    discrete_lanczos_derivative(discrete_lanczos_derivative&&) = default;
    discrete_lanczos_derivative& operator=(discrete_lanczos_derivative&&) = default;

private:
    std::vector<Real> m_f;
    std::vector<std::vector<Real>> m_boundary_filters;
    Real m_dt;
};

} // namespaces
#endif
