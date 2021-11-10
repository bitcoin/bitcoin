//  (C) Copyright Nick Thompson 2018.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_TOOLS_UNIVARIATE_STATISTICS_HPP
#define BOOST_MATH_TOOLS_UNIVARIATE_STATISTICS_HPP

#include <algorithm>
#include <iterator>
#include <tuple>
#include <boost/math/tools/assert.hpp>
#include <boost/math/tools/header_deprecated.hpp>

BOOST_MATH_HEADER_DEPRECATED("<boost/math/statistics/univariate_statistics.hpp>");

namespace boost::math::tools {

template<class ForwardIterator>
auto mean(ForwardIterator first, ForwardIterator last)
{
    using Real = typename std::iterator_traits<ForwardIterator>::value_type;
    BOOST_MATH_ASSERT_MSG(first != last, "At least one sample is required to compute the mean.");
    if constexpr (std::is_integral<Real>::value)
    {
        double mu = 0;
        double i = 1;
        for(auto it = first; it != last; ++it) {
            mu = mu + (*it - mu)/i;
            i += 1;
        }
        return mu;
    }
    else if constexpr (std::is_same_v<typename std::iterator_traits<ForwardIterator>::iterator_category, std::random_access_iterator_tag>)
    {
        size_t elements = std::distance(first, last);
        Real mu0 = 0;
        Real mu1 = 0;
        Real mu2 = 0;
        Real mu3 = 0;
        Real i = 1;
        auto end = last - (elements % 4);
        for(auto it = first; it != end;  it += 4) {
            Real inv = Real(1)/i;
            Real tmp0 = (*it - mu0);
            Real tmp1 = (*(it+1) - mu1);
            Real tmp2 = (*(it+2) - mu2);
            Real tmp3 = (*(it+3) - mu3);
            // please generate a vectorized fma here
            mu0 += tmp0*inv;
            mu1 += tmp1*inv;
            mu2 += tmp2*inv;
            mu3 += tmp3*inv;
            i += 1;
        }
        Real num1 = Real(elements  - (elements %4))/Real(4);
        Real num2 = num1 + Real(elements % 4);

        for (auto it = end; it != last; ++it)
        {
            mu3 += (*it-mu3)/i;
            i += 1;
        }

        return (num1*(mu0+mu1+mu2) + num2*mu3)/Real(elements);
    }
    else
    {
        auto it = first;
        Real mu = *it;
        Real i = 2;
        while(++it != last)
        {
            mu += (*it - mu)/i;
            i += 1;
        }
        return mu;
    }
}

template<class Container>
inline auto mean(Container const & v)
{
    return mean(v.cbegin(), v.cend());
}

template<class ForwardIterator>
auto variance(ForwardIterator first, ForwardIterator last)
{
    using Real = typename std::iterator_traits<ForwardIterator>::value_type;
    BOOST_MATH_ASSERT_MSG(first != last, "At least one sample is required to compute mean and variance.");
    // Higham, Accuracy and Stability, equation 1.6a and 1.6b:
    if constexpr (std::is_integral<Real>::value)
    {
        double M = *first;
        double Q = 0;
        double k = 2;
        for (auto it = std::next(first); it != last; ++it)
        {
            double tmp = *it - M;
            Q = Q + ((k-1)*tmp*tmp)/k;
            M = M + tmp/k;
            k += 1;
        }
        return Q/(k-1);
    }
    else
    {
        Real M = *first;
        Real Q = 0;
        Real k = 2;
        for (auto it = std::next(first); it != last; ++it)
        {
            Real tmp = (*it - M)/k;
            Q += k*(k-1)*tmp*tmp;
            M += tmp;
            k += 1;
        }
        return Q/(k-1);
    }
}

template<class Container>
inline auto variance(Container const & v)
{
    return variance(v.cbegin(), v.cend());
}

template<class ForwardIterator>
auto sample_variance(ForwardIterator first, ForwardIterator last)
{
    size_t n = std::distance(first, last);
    BOOST_MATH_ASSERT_MSG(n > 1, "At least two samples are required to compute the sample variance.");
    return n*variance(first, last)/(n-1);
}

template<class Container>
inline auto sample_variance(Container const & v)
{
    return sample_variance(v.cbegin(), v.cend());
}


// Follows equation 1.5 of:
// https://prod.sandia.gov/techlib-noauth/access-control.cgi/2008/086212.pdf
template<class ForwardIterator>
auto skewness(ForwardIterator first, ForwardIterator last)
{
    using Real = typename std::iterator_traits<ForwardIterator>::value_type;
    BOOST_MATH_ASSERT_MSG(first != last, "At least one sample is required to compute skewness.");
    if constexpr (std::is_integral<Real>::value)
    {
        double M1 = *first;
        double M2 = 0;
        double M3 = 0;
        double n = 2;
        for (auto it = std::next(first); it != last; ++it)
        {
            double delta21 = *it - M1;
            double tmp = delta21/n;
            M3 = M3 + tmp*((n-1)*(n-2)*delta21*tmp - 3*M2);
            M2 = M2 + tmp*(n-1)*delta21;
            M1 = M1 + tmp;
            n += 1;
        }

        double var = M2/(n-1);
        if (var == 0)
        {
            // The limit is technically undefined, but the interpretation here is clear:
            // A constant dataset has no skewness.
            return double(0);
        }
        double skew = M3/(M2*sqrt(var));
        return skew;
    }
    else
    {
        Real M1 = *first;
        Real M2 = 0;
        Real M3 = 0;
        Real n = 2;
        for (auto it = std::next(first); it != last; ++it)
        {
            Real delta21 = *it - M1;
            Real tmp = delta21/n;
            M3 += tmp*((n-1)*(n-2)*delta21*tmp - 3*M2);
            M2 += tmp*(n-1)*delta21;
            M1 += tmp;
            n += 1;
        }

        Real var = M2/(n-1);
        if (var == 0)
        {
            // The limit is technically undefined, but the interpretation here is clear:
            // A constant dataset has no skewness.
            return Real(0);
        }
        Real skew = M3/(M2*sqrt(var));
        return skew;
    }
}

template<class Container>
inline auto skewness(Container const & v)
{
    return skewness(v.cbegin(), v.cend());
}

// Follows equation 1.5/1.6 of:
// https://prod.sandia.gov/techlib-noauth/access-control.cgi/2008/086212.pdf
template<class ForwardIterator>
auto first_four_moments(ForwardIterator first, ForwardIterator last)
{
    using Real = typename std::iterator_traits<ForwardIterator>::value_type;
    BOOST_MATH_ASSERT_MSG(first != last, "At least one sample is required to compute the first four moments.");
    if constexpr (std::is_integral<Real>::value)
    {
        double M1 = *first;
        double M2 = 0;
        double M3 = 0;
        double M4 = 0;
        double n = 2;
        for (auto it = std::next(first); it != last; ++it)
        {
            double delta21 = *it - M1;
            double tmp = delta21/n;
            M4 = M4 + tmp*(tmp*tmp*delta21*((n-1)*(n*n-3*n+3)) + 6*tmp*M2 - 4*M3);
            M3 = M3 + tmp*((n-1)*(n-2)*delta21*tmp - 3*M2);
            M2 = M2 + tmp*(n-1)*delta21;
            M1 = M1 + tmp;
            n += 1;
        }

        return std::make_tuple(M1, M2/(n-1), M3/(n-1), M4/(n-1));
    }
    else
    {
        Real M1 = *first;
        Real M2 = 0;
        Real M3 = 0;
        Real M4 = 0;
        Real n = 2;
        for (auto it = std::next(first); it != last; ++it)
        {
            Real delta21 = *it - M1;
            Real tmp = delta21/n;
            M4 = M4 + tmp*(tmp*tmp*delta21*((n-1)*(n*n-3*n+3)) + 6*tmp*M2 - 4*M3);
            M3 = M3 + tmp*((n-1)*(n-2)*delta21*tmp - 3*M2);
            M2 = M2 + tmp*(n-1)*delta21;
            M1 = M1 + tmp;
            n += 1;
        }

        return std::make_tuple(M1, M2/(n-1), M3/(n-1), M4/(n-1));
    }
}

template<class Container>
inline auto first_four_moments(Container const & v)
{
    return first_four_moments(v.cbegin(), v.cend());
}


// Follows equation 1.6 of:
// https://prod.sandia.gov/techlib-noauth/access-control.cgi/2008/086212.pdf
template<class ForwardIterator>
auto kurtosis(ForwardIterator first, ForwardIterator last)
{
    auto [M1, M2, M3, M4] = first_four_moments(first, last);
    if (M2 == 0)
    {
        return M2;
    }
    return M4/(M2*M2);
}

template<class Container>
inline auto kurtosis(Container const & v)
{
    return kurtosis(v.cbegin(), v.cend());
}

template<class ForwardIterator>
auto excess_kurtosis(ForwardIterator first, ForwardIterator last)
{
    return kurtosis(first, last) - 3;
}

template<class Container>
inline auto excess_kurtosis(Container const & v)
{
    return excess_kurtosis(v.cbegin(), v.cend());
}


template<class RandomAccessIterator>
auto median(RandomAccessIterator first, RandomAccessIterator last)
{
    size_t num_elems = std::distance(first, last);
    BOOST_MATH_ASSERT_MSG(num_elems > 0, "The median of a zero length vector is undefined.");
    if (num_elems & 1)
    {
        auto middle = first + (num_elems - 1)/2;
        std::nth_element(first, middle, last);
        return *middle;
    }
    else
    {
        auto middle = first + num_elems/2 - 1;
        std::nth_element(first, middle, last);
        std::nth_element(middle, middle+1, last);
        return (*middle + *(middle+1))/2;
    }
}


template<class RandomAccessContainer>
inline auto median(RandomAccessContainer & v)
{
    return median(v.begin(), v.end());
}

template<class RandomAccessIterator>
auto gini_coefficient(RandomAccessIterator first, RandomAccessIterator last)
{
    using Real = typename std::iterator_traits<RandomAccessIterator>::value_type;
    BOOST_MATH_ASSERT_MSG(first != last && std::next(first) != last, "Computation of the Gini coefficient requires at least two samples.");

    std::sort(first, last);
    if constexpr (std::is_integral<Real>::value)
    {
        double i = 1;
        double num = 0;
        double denom = 0;
        for (auto it = first; it != last; ++it)
        {
            num += *it*i;
            denom += *it;
            ++i;
        }

        // If the l1 norm is zero, all elements are zero, so every element is the same.
        if (denom == 0)
        {
            return double(0);
        }

        return ((2*num)/denom - i)/(i-1);
    }
    else
    {
        Real i = 1;
        Real num = 0;
        Real denom = 0;
        for (auto it = first; it != last; ++it)
        {
            num += *it*i;
            denom += *it;
            ++i;
        }

        // If the l1 norm is zero, all elements are zero, so every element is the same.
        if (denom == 0)
        {
            return Real(0);
        }

        return ((2*num)/denom - i)/(i-1);
    }
}

template<class RandomAccessContainer>
inline auto gini_coefficient(RandomAccessContainer & v)
{
    return gini_coefficient(v.begin(), v.end());
}

template<class RandomAccessIterator>
inline auto sample_gini_coefficient(RandomAccessIterator first, RandomAccessIterator last)
{
    size_t n = std::distance(first, last);
    return n*gini_coefficient(first, last)/(n-1);
}

template<class RandomAccessContainer>
inline auto sample_gini_coefficient(RandomAccessContainer & v)
{
    return sample_gini_coefficient(v.begin(), v.end());
}

template<class RandomAccessIterator>
auto median_absolute_deviation(RandomAccessIterator first, RandomAccessIterator last, typename std::iterator_traits<RandomAccessIterator>::value_type center=std::numeric_limits<typename std::iterator_traits<RandomAccessIterator>::value_type>::quiet_NaN())
{
    using std::abs;
    using Real = typename std::iterator_traits<RandomAccessIterator>::value_type;
    using std::isnan;
    if (isnan(center))
    {
        center = boost::math::tools::median(first, last);
    }
    size_t num_elems = std::distance(first, last);
    BOOST_MATH_ASSERT_MSG(num_elems > 0, "The median of a zero-length vector is undefined.");
    auto comparator = [&center](Real a, Real b) { return abs(a-center) < abs(b-center);};
    if (num_elems & 1)
    {
        auto middle = first + (num_elems - 1)/2;
        std::nth_element(first, middle, last, comparator);
        return abs(*middle);
    }
    else
    {
        auto middle = first + num_elems/2 - 1;
        std::nth_element(first, middle, last, comparator);
        std::nth_element(middle, middle+1, last, comparator);
        return (abs(*middle) + abs(*(middle+1)))/abs(static_cast<Real>(2));
    }
}

template<class RandomAccessContainer>
inline auto median_absolute_deviation(RandomAccessContainer & v, typename RandomAccessContainer::value_type center=std::numeric_limits<typename RandomAccessContainer::value_type>::quiet_NaN())
{
    return median_absolute_deviation(v.begin(), v.end(), center);
}

}
#endif
