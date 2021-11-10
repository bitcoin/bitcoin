//  (C) Copyright Nick Thompson 2018.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_TOOLS_SIGNAL_STATISTICS_HPP
#define BOOST_MATH_TOOLS_SIGNAL_STATISTICS_HPP

#include <algorithm>
#include <iterator>
#include <boost/math/tools/assert.hpp>
#include <boost/math/tools/complex.hpp>
#include <boost/math/tools/roots.hpp>
#include <boost/math/statistics/univariate_statistics.hpp>


namespace boost::math::statistics {

template<class ForwardIterator>
auto absolute_gini_coefficient(ForwardIterator first, ForwardIterator last)
{
    using std::abs;
    using RealOrComplex = typename std::iterator_traits<ForwardIterator>::value_type;
    BOOST_MATH_ASSERT_MSG(first != last && std::next(first) != last, "Computation of the Gini coefficient requires at least two samples.");

    std::sort(first, last,  [](RealOrComplex a, RealOrComplex b) { return abs(b) > abs(a); });


    decltype(abs(*first)) i = 1;
    decltype(abs(*first)) num = 0;
    decltype(abs(*first)) denom = 0;
    for (auto it = first; it != last; ++it)
    {
        decltype(abs(*first)) tmp = abs(*it);
        num += tmp*i;
        denom += tmp;
        ++i;
    }

    // If the l1 norm is zero, all elements are zero, so every element is the same.
    if (denom == 0)
    {
        decltype(abs(*first)) zero = 0;
        return zero;
    }
    return ((2*num)/denom - i)/(i-1);
}

template<class RandomAccessContainer>
inline auto absolute_gini_coefficient(RandomAccessContainer & v)
{
    return boost::math::statistics::absolute_gini_coefficient(v.begin(), v.end());
}

template<class ForwardIterator>
auto sample_absolute_gini_coefficient(ForwardIterator first, ForwardIterator last)
{
    size_t n = std::distance(first, last);
    return n*boost::math::statistics::absolute_gini_coefficient(first, last)/(n-1);
}

template<class RandomAccessContainer>
inline auto sample_absolute_gini_coefficient(RandomAccessContainer & v)
{
    return boost::math::statistics::sample_absolute_gini_coefficient(v.begin(), v.end());
}


// The Hoyer sparsity measure is defined in:
// https://arxiv.org/pdf/0811.4706.pdf
template<class ForwardIterator>
auto hoyer_sparsity(const ForwardIterator first, const ForwardIterator last)
{
    using T = typename std::iterator_traits<ForwardIterator>::value_type;
    using std::abs;
    using std::sqrt;
    BOOST_MATH_ASSERT_MSG(first != last && std::next(first) != last, "Computation of the Hoyer sparsity requires at least two samples.");

    if constexpr (std::is_unsigned<T>::value)
    {
        T l1 = 0;
        T l2 = 0;
        size_t n = 0;
        for (auto it = first; it != last; ++it)
        {
            l1 += *it;
            l2 += (*it)*(*it);
            n += 1;
        }

        double rootn = sqrt(n);
        return (rootn - l1/sqrt(l2) )/ (rootn - 1);
    }
    else {
        decltype(abs(*first)) l1 = 0;
        decltype(abs(*first)) l2 = 0;
        // We wouldn't need to count the elements if it was a random access iterator,
        // but our only constraint is that it's a forward iterator.
        size_t n = 0;
        for (auto it = first; it != last; ++it)
        {
            decltype(abs(*first)) tmp = abs(*it);
            l1 += tmp;
            l2 += tmp*tmp;
            n += 1;
        }
        if constexpr (std::is_integral<T>::value)
        {
            double rootn = sqrt(n);
            return (rootn - l1/sqrt(l2) )/ (rootn - 1);
        }
        else
        {
            decltype(abs(*first)) rootn = sqrt(static_cast<decltype(abs(*first))>(n));
            return (rootn - l1/sqrt(l2) )/ (rootn - 1);
        }
    }
}

template<class Container>
inline auto hoyer_sparsity(Container const & v)
{
    return boost::math::statistics::hoyer_sparsity(v.cbegin(), v.cend());
}


template<class Container>
auto oracle_snr(Container const & signal, Container const & noisy_signal)
{
    using Real = typename Container::value_type;
    BOOST_MATH_ASSERT_MSG(signal.size() == noisy_signal.size(),
                     "Signal and noisy_signal must be have the same number of elements.");
    if constexpr (std::is_integral<Real>::value)
    {
        double numerator = 0;
        double denominator = 0;
        for (size_t i = 0; i < signal.size(); ++i)
        {
            numerator += signal[i]*signal[i];
            denominator += (noisy_signal[i] - signal[i])*(noisy_signal[i] - signal[i]);
        }
        if (numerator == 0 && denominator == 0)
        {
            return std::numeric_limits<double>::quiet_NaN();
        }
        if (denominator == 0)
        {
            return std::numeric_limits<double>::infinity();
        }
        return numerator/denominator;
    }
    else if constexpr (boost::math::tools::is_complex_type<Real>::value)

    {
        using std::norm;
        typename Real::value_type numerator = 0;
        typename Real::value_type denominator = 0;
        for (size_t i = 0; i < signal.size(); ++i)
        {
            numerator += norm(signal[i]);
            denominator += norm(noisy_signal[i] - signal[i]);
        }
        if (numerator == 0 && denominator == 0)
        {
            return std::numeric_limits<typename Real::value_type>::quiet_NaN();
        }
        if (denominator == 0)
        {
            return std::numeric_limits<typename Real::value_type>::infinity();
        }

        return numerator/denominator;
    }
    else
    {
        Real numerator = 0;
        Real denominator = 0;
        for (size_t i = 0; i < signal.size(); ++i)
        {
            numerator += signal[i]*signal[i];
            denominator += (signal[i] - noisy_signal[i])*(signal[i] - noisy_signal[i]);
        }
        if (numerator == 0 && denominator == 0)
        {
            return std::numeric_limits<Real>::quiet_NaN();
        }
        if (denominator == 0)
        {
            return std::numeric_limits<Real>::infinity();
        }

        return numerator/denominator;
    }
}

template<class Container>
auto mean_invariant_oracle_snr(Container const & signal, Container const & noisy_signal)
{
    using Real = typename Container::value_type;
    BOOST_MATH_ASSERT_MSG(signal.size() == noisy_signal.size(), "Signal and noisy signal must be have the same number of elements.");

    Real mu = boost::math::statistics::mean(signal);
    Real numerator = 0;
    Real denominator = 0;
    for (size_t i = 0; i < signal.size(); ++i)
    {
        Real tmp = signal[i] - mu;
        numerator += tmp*tmp;
        denominator += (signal[i] - noisy_signal[i])*(signal[i] - noisy_signal[i]);
    }
    if (numerator == 0 && denominator == 0)
    {
        return std::numeric_limits<Real>::quiet_NaN();
    }
    if (denominator == 0)
    {
        return std::numeric_limits<Real>::infinity();
    }

    return numerator/denominator;

}

template<class Container>
auto mean_invariant_oracle_snr_db(Container const & signal, Container const & noisy_signal)
{
    using std::log10;
    return 10*log10(boost::math::statistics::mean_invariant_oracle_snr(signal, noisy_signal));
}


// Follows the definition of SNR given in Mallat, A Wavelet Tour of Signal Processing, equation 11.16.
template<class Container>
auto oracle_snr_db(Container const & signal, Container const & noisy_signal)
{
    using std::log10;
    return 10*log10(boost::math::statistics::oracle_snr(signal, noisy_signal));
}

// A good reference on the M2M4 estimator:
// D. R. Pauluzzi and N. C. Beaulieu, "A comparison of SNR estimation techniques for the AWGN channel," IEEE Trans. Communications, Vol. 48, No. 10, pp. 1681-1691, 2000.
// A nice python implementation:
// https://github.com/gnuradio/gnuradio/blob/master/gr-digital/examples/snr_estimators.py
template<class ForwardIterator>
auto m2m4_snr_estimator(ForwardIterator first, ForwardIterator last, decltype(*first) estimated_signal_kurtosis=1, decltype(*first) estimated_noise_kurtosis=3)
{
    BOOST_MATH_ASSERT_MSG(estimated_signal_kurtosis > 0, "The estimated signal kurtosis must be positive");
    BOOST_MATH_ASSERT_MSG(estimated_noise_kurtosis > 0, "The estimated noise kurtosis must be positive.");
    using Real = typename std::iterator_traits<ForwardIterator>::value_type;
    using std::sqrt;
    if constexpr (std::is_floating_point<Real>::value || std::numeric_limits<Real>::max_exponent)
    {
        // If we first eliminate N, we obtain the quadratic equation:
        // (ka+kw-6)S^2 + 2M2(3-kw)S + kw*M2^2 - M4 = 0 =: a*S^2 + bs*N + cs = 0
        // If we first eliminate S, we obtain the quadratic equation:
        // (ka+kw-6)N^2 + 2M2(3-ka)N + ka*M2^2 - M4 = 0 =: a*N^2 + bn*N + cn = 0
        // I believe these equations are totally independent quadratics;
        // if one has a complex solution it is not necessarily the case that the other must also.
        // However, I can't prove that, so there is a chance that this does unnecessary work.
        // Future improvements: There are algorithms which can solve quadratics much more effectively than the naive implementation found here.
        // See: https://stackoverflow.com/questions/48979861/numerically-stable-method-for-solving-quadratic-equations/50065711#50065711
        auto [M1, M2, M3, M4] = boost::math::statistics::first_four_moments(first, last);
        if (M4 == 0)
        {
            // The signal is constant. There is no noise:
            return std::numeric_limits<Real>::infinity();
        }
        // Change to notation in Pauluzzi, equation 41:
        auto kw = estimated_noise_kurtosis;
        auto ka = estimated_signal_kurtosis;
        // A common case, since it's the default:
        Real a = (ka+kw-6);
        Real bs = 2*M2*(3-kw);
        Real cs = kw*M2*M2 - M4;
        Real bn = 2*M2*(3-ka);
        Real cn = ka*M2*M2 - M4;
        auto [S0, S1] = boost::math::tools::quadratic_roots(a, bs, cs);
        if (S1 > 0)
        {
            auto N = M2 - S1;
            if (N > 0)
            {
                return S1/N;
            }
            if (S0 > 0)
            {
                N = M2 - S0;
                if (N > 0)
                {
                    return S0/N;
                }
            }
        }
        auto [N0, N1] = boost::math::tools::quadratic_roots(a, bn, cn);
        if (N1 > 0)
        {
            auto S = M2 - N1;
            if (S > 0)
            {
                return S/N1;
            }
            if (N0 > 0)
            {
                S = M2 - N0;
                if (S > 0)
                {
                    return S/N0;
                }
            }
        }
        // This happens distressingly often. It's a limitation of the method.
        return std::numeric_limits<Real>::quiet_NaN();
    }
    else
    {
        BOOST_MATH_ASSERT_MSG(false, "The M2M4 estimator has not been implemented for this type.");
        return std::numeric_limits<Real>::quiet_NaN();
    }
}

template<class Container>
inline auto m2m4_snr_estimator(Container const & noisy_signal,  typename Container::value_type estimated_signal_kurtosis=1, typename Container::value_type estimated_noise_kurtosis=3)
{
    return m2m4_snr_estimator(noisy_signal.cbegin(), noisy_signal.cend(), estimated_signal_kurtosis, estimated_noise_kurtosis);
}

template<class ForwardIterator>
inline auto m2m4_snr_estimator_db(ForwardIterator first, ForwardIterator last, decltype(*first) estimated_signal_kurtosis=1, decltype(*first) estimated_noise_kurtosis=3)
{
    using std::log10;
    return 10*log10(m2m4_snr_estimator(first, last, estimated_signal_kurtosis, estimated_noise_kurtosis));
}


template<class Container>
inline auto m2m4_snr_estimator_db(Container const & noisy_signal,  typename Container::value_type estimated_signal_kurtosis=1, typename Container::value_type estimated_noise_kurtosis=3)
{
    using std::log10;
    return 10*log10(m2m4_snr_estimator(noisy_signal, estimated_signal_kurtosis, estimated_noise_kurtosis));
}

}
#endif
