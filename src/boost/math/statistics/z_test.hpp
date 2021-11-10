//  (C) Copyright Matt Borland 2021.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_STATISTICS_Z_TEST_HPP
#define BOOST_MATH_STATISTICS_Z_TEST_HPP

#include <boost/math/distributions/normal.hpp>
#include <boost/math/statistics/univariate_statistics.hpp>
#include <iterator>
#include <type_traits>
#include <utility>
#include <cmath>

namespace boost { namespace math { namespace statistics { namespace detail {

template<typename ReturnType, typename T>
ReturnType one_sample_z_test_impl(T sample_mean, T sample_variance, T sample_size, T assumed_mean)
{
    using Real = typename std::tuple_element<0, ReturnType>::type;
    using std::sqrt;
    using no_promote_policy = boost::math::policies::policy<boost::math::policies::promote_float<false>, boost::math::policies::promote_double<false>>;

    Real test_statistic = (sample_mean - assumed_mean) / (sample_variance / sqrt(sample_size));
    auto z = boost::math::normal_distribution<Real, no_promote_policy>(sample_size - 1);
    Real pvalue;
    if(test_statistic > 0)
    {
        pvalue = 2*boost::math::cdf<Real>(z, -test_statistic);
    }
    else
    {
        pvalue = 2*boost::math::cdf<Real>(z, test_statistic);
    }

    return std::make_pair(test_statistic, pvalue);
}

template<typename ReturnType, typename ForwardIterator>
ReturnType one_sample_z_test_impl(ForwardIterator begin, ForwardIterator end, typename std::iterator_traits<ForwardIterator>::value_type assumed_mean) 
{
    using Real = typename std::tuple_element<0, ReturnType>::type;
    std::pair<Real, Real> temp = mean_and_sample_variance(begin, end);
    Real mu = std::get<0>(temp);
    Real s_sq = std::get<1>(temp);
    return one_sample_z_test_impl<ReturnType>(mu, s_sq, Real(std::distance(begin, end)), Real(assumed_mean));
}

template<typename ReturnType, typename T>
ReturnType two_sample_z_test_impl(T mean_1, T variance_1, T size_1, T mean_2, T variance_2, T size_2)
{
    using Real = typename std::tuple_element<0, ReturnType>::type;
    using std::sqrt;
    using no_promote_policy = boost::math::policies::policy<boost::math::policies::promote_float<false>, boost::math::policies::promote_double<false>>;

    Real test_statistic = (mean_1 - mean_2) / sqrt(variance_1/size_1 + variance_2/size_2);
    auto z = boost::math::normal_distribution<Real, no_promote_policy>(size_1 + size_2 - 1);
    Real pvalue;
    if(test_statistic > 0)
    {
        pvalue = 2*boost::math::cdf<Real>(z, -test_statistic);
    }
    else
    {
        pvalue = 2*boost::math::cdf<Real>(z, test_statistic);
    }

    return std::make_pair(test_statistic, pvalue);
}

template<typename ReturnType, typename ForwardIterator>
ReturnType two_sample_z_test_impl(ForwardIterator begin_1, ForwardIterator end_1, ForwardIterator begin_2, ForwardIterator end_2)
{
    using Real = typename std::tuple_element<0, ReturnType>::type;
    using std::sqrt;
    auto n1 = std::distance(begin_1, end_1);
    auto n2 = std::distance(begin_2, end_2);

    ReturnType temp_1 = mean_and_sample_variance(begin_1, end_1);
    Real mean_1 = std::get<0>(temp_1);
    Real variance_1 = std::get<1>(temp_1);

    ReturnType temp_2 = mean_and_sample_variance(begin_2, end_2);
    Real mean_2 = std::get<0>(temp_2);
    Real variance_2 = std::get<1>(temp_2);

    return two_sample_z_test_impl<ReturnType>(mean_1, variance_1, Real(n1), mean_2, variance_2, Real(n2));
}

} // detail

template<typename Real, typename std::enable_if<std::is_integral<Real>::value, bool>::type = true>
inline auto one_sample_z_test(Real sample_mean, Real sample_variance, Real sample_size, Real assumed_mean) -> std::pair<double, double>
{
    return detail::one_sample_z_test_impl<std::pair<double, double>>(sample_mean, sample_variance, sample_size, assumed_mean);
}

template<typename Real, typename std::enable_if<!std::is_integral<Real>::value, bool>::type = true>
inline auto one_sample_z_test(Real sample_mean, Real sample_variance, Real sample_size, Real assumed_mean) -> std::pair<Real, Real>
{
    return detail::one_sample_z_test_impl<std::pair<Real, Real>>(sample_mean, sample_variance, sample_size, assumed_mean);
}

template<typename ForwardIterator, typename Real = typename std::iterator_traits<ForwardIterator>::value_type, 
         typename std::enable_if<std::is_integral<Real>::value, bool>::type = true>
inline auto one_sample_z_test(ForwardIterator begin, ForwardIterator end, Real assumed_mean) -> std::pair<double, double>
{
    return detail::one_sample_z_test_impl<std::pair<double, double>>(begin, end, assumed_mean);
}

template<typename ForwardIterator, typename Real = typename std::iterator_traits<ForwardIterator>::value_type, 
         typename std::enable_if<!std::is_integral<Real>::value, bool>::type = true>
inline auto one_sample_z_test(ForwardIterator begin, ForwardIterator end, Real assumed_mean) -> std::pair<Real, Real>
{
    return detail::one_sample_z_test_impl<std::pair<Real, Real>>(begin, end, assumed_mean);
}

template<typename Container, typename Real = typename Container::value_type,
         typename std::enable_if<std::is_integral<Real>::value, bool>::type = true>
inline auto one_sample_z_test(Container const & v, Real assumed_mean) -> std::pair<double, double>
{
    return detail::one_sample_z_test_impl<std::pair<double, double>>(std::begin(v), std::end(v), assumed_mean);
}

template<typename Container, typename Real = typename Container::value_type,
         typename std::enable_if<!std::is_integral<Real>::value, bool>::type = true>
inline auto one_sample_z_test(Container const & v, Real assumed_mean) -> std::pair<Real, Real>
{
    return detail::one_sample_z_test_impl<std::pair<Real, Real>>(std::begin(v), std::end(v), assumed_mean);
}

template<typename ForwardIterator, typename Real = typename std::iterator_traits<ForwardIterator>::value_type, 
         typename std::enable_if<std::is_integral<Real>::value, bool>::type = true>
inline auto two_sample_z_test(ForwardIterator begin_1, ForwardIterator end_1, ForwardIterator begin_2, ForwardIterator end_2) -> std::pair<double, double>
{
    return detail::two_sample_z_test_impl<std::pair<double, double>>(begin_1, end_1, begin_2, end_2);
}

template<typename ForwardIterator, typename Real = typename std::iterator_traits<ForwardIterator>::value_type, 
         typename std::enable_if<!std::is_integral<Real>::value, bool>::type = true>
inline auto two_sample_z_test(ForwardIterator begin_1, ForwardIterator end_1, ForwardIterator begin_2, ForwardIterator end_2) -> std::pair<Real, Real>
{
    return detail::two_sample_z_test_impl<std::pair<Real, Real>>(begin_1, end_1, begin_2, end_2);
}

template<typename Container, typename Real = typename Container::value_type, typename std::enable_if<std::is_integral<Real>::value, bool>::type = true>
inline auto two_sample_z_test(Container const & u, Container const & v) -> std::pair<double, double>
{
    return detail::two_sample_z_test_impl<std::pair<double, double>>(std::begin(u), std::end(u), std::begin(v), std::end(v));
}

template<typename Container, typename Real = typename Container::value_type, typename std::enable_if<!std::is_integral<Real>::value, bool>::type = true>
inline auto two_sample_z_test(Container const & u, Container const & v) -> std::pair<Real, Real>
{
    return detail::two_sample_z_test_impl<std::pair<Real, Real>>(std::begin(u), std::end(u), std::begin(v), std::end(v));
}

}}} // boost::math::statistics

#endif // BOOST_MATH_STATISTICS_Z_TEST_HPP
