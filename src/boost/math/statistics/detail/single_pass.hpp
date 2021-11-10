//  (C) Copyright Nick Thompson 2018
//  (C) Copyright Matt Borland 2020
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_STATISTICS_UNIVARIATE_STATISTICS_DETAIL_SINGLE_PASS_HPP
#define BOOST_MATH_STATISTICS_UNIVARIATE_STATISTICS_DETAIL_SINGLE_PASS_HPP

#include <boost/math/tools/config.hpp>
#include <boost/math/tools/assert.hpp>
#include <tuple>
#include <iterator>
#include <type_traits>
#include <cmath>
#include <algorithm>
#include <valarray>
#include <stdexcept>
#include <functional>
#include <vector>

#ifdef BOOST_HAS_THREADS
#include <future>
#include <thread>
#endif

namespace boost { namespace math { namespace statistics { namespace detail {

template<typename ReturnType, typename ForwardIterator>
ReturnType mean_sequential_impl(ForwardIterator first, ForwardIterator last)
{
    const std::size_t elements {static_cast<std::size_t>(std::distance(first, last))};
    std::valarray<ReturnType> mu {0, 0, 0, 0};
    std::valarray<ReturnType> temp {0, 0, 0, 0};
    ReturnType i {1};
    const ForwardIterator end {std::next(first, elements - (elements % 4))};
    ForwardIterator it {first};

    while(it != end)
    {
        const ReturnType inv {ReturnType(1) / i};
        temp = {static_cast<ReturnType>(*it++), static_cast<ReturnType>(*it++), static_cast<ReturnType>(*it++), static_cast<ReturnType>(*it++)};
        temp -= mu;
        mu += (temp *= inv);
        i += 1;
    }

    const ReturnType num1 {ReturnType(elements - (elements % 4))/ReturnType(4)};
    const ReturnType num2 {num1 + ReturnType(elements % 4)};

    while(it != last)
    {
        mu[3] += (*it-mu[3])/i;
        i += 1;
        ++it;
    }

    return (num1 * std::valarray<ReturnType>(mu[std::slice(0,3,1)]).sum() + num2 * mu[3]) / ReturnType(elements);
}

// Higham, Accuracy and Stability, equation 1.6a and 1.6b:
// Calculates Mean, M2, and variance
template<typename ReturnType, typename ForwardIterator>
ReturnType variance_sequential_impl(ForwardIterator first, ForwardIterator last)
{
    using Real = typename std::tuple_element<0, ReturnType>::type;

    Real M = *first;
    Real Q = 0;
    Real k = 2;
    Real M2 = 0;
    std::size_t n = 1;

    for(auto it = std::next(first); it != last; ++it)
    {
        Real tmp = (*it - M) / k;
        Real delta_1 = *it - M;
        Q += k*(k-1)*tmp*tmp;
        M += tmp;
        k += 1;
        Real delta_2 = *it - M;
        M2 += delta_1 * delta_2;
        ++n;
    }

    return std::make_tuple(M, M2, Q/(k-1), Real(n));
}

// https://en.wikipedia.org/wiki/Algorithms_for_calculating_variance#Higher-order_statistics
template<typename ReturnType, typename ForwardIterator>
ReturnType first_four_moments_sequential_impl(ForwardIterator first, ForwardIterator last)
{
    using Real = typename std::tuple_element<0, ReturnType>::type;
    using Size = typename std::tuple_element<4, ReturnType>::type;

    Real M1 = *first;
    Real M2 = 0;
    Real M3 = 0;
    Real M4 = 0;
    Size n = 2;
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

    return std::make_tuple(M1, M2, M3, M4, n-1);
}

#ifdef BOOST_HAS_THREADS

// https://en.wikipedia.org/wiki/Algorithms_for_calculating_variance#Higher-order_statistics
// EQN 3.1: https://www.osti.gov/servlets/purl/1426900
template<typename ReturnType, typename ForwardIterator>
ReturnType first_four_moments_parallel_impl(ForwardIterator first, ForwardIterator last)
{
    using Real = typename std::tuple_element<0, ReturnType>::type;

    const auto elements = std::distance(first, last);
    const unsigned max_concurrency = std::thread::hardware_concurrency() == 0 ? 2u : std::thread::hardware_concurrency();
    unsigned num_threads = 2u;
    
    // Threading is faster for: 10 + 5.13e-3 N/j <= 5.13e-3N => N >= 10^4j/5.13(j-1).
    const auto parallel_lower_bound = 10e4*max_concurrency/(5.13*(max_concurrency-1));
    const auto parallel_upper_bound = 10e4*2/5.13; // j = 2

    // https://lemire.me/blog/2020/01/30/cost-of-a-thread-in-c-under-linux/
    if(elements < parallel_lower_bound)
    {
        return detail::first_four_moments_sequential_impl<ReturnType>(first, last);
    }
    else if(elements >= parallel_upper_bound)
    {
        num_threads = max_concurrency;
    }
    else
    {
        for(unsigned i = 3; i < max_concurrency; ++i)
        {
            if(parallel_lower_bound < 10e4*i/(5.13*(i-1)))
            {
                num_threads = i;
                break;
            }
        }
    }

    std::vector<std::future<ReturnType>> future_manager;
    const auto elements_per_thread = std::ceil(static_cast<double>(elements) / num_threads);

    auto it = first;
    for(std::size_t i {}; i < num_threads - 1; ++i)
    {
        future_manager.emplace_back(std::async(std::launch::async | std::launch::deferred, [it, elements_per_thread]() -> ReturnType
        {
            return first_four_moments_sequential_impl<ReturnType>(it, std::next(it, elements_per_thread));
        }));
        it = std::next(it, elements_per_thread);
    }

    future_manager.emplace_back(std::async(std::launch::async | std::launch::deferred, [it, last]() -> ReturnType
    {
        return first_four_moments_sequential_impl<ReturnType>(it, last);
    }));

    auto temp = future_manager[0].get();
    Real M1_a = std::get<0>(temp);
    Real M2_a = std::get<1>(temp);
    Real M3_a = std::get<2>(temp);
    Real M4_a = std::get<3>(temp);
    Real range_a = std::get<4>(temp);

    for(std::size_t i = 1; i < future_manager.size(); ++i)
    {
        temp = future_manager[i].get();
        Real M1_b = std::get<0>(temp);
        Real M2_b = std::get<1>(temp);
        Real M3_b = std::get<2>(temp);
        Real M4_b = std::get<3>(temp);
        Real range_b = std::get<4>(temp);

        const Real n_ab = range_a + range_b;
        const Real delta = M1_b - M1_a;
        
        M1_a = (range_a * M1_a + range_b * M1_b) / n_ab;
        M2_a = M2_a + M2_b + delta * delta * (range_a * range_b / n_ab);
        M3_a = M3_a + M3_b + (delta * delta * delta) * range_a * range_b * (range_a - range_b) / (n_ab * n_ab)    
               + Real(3) * delta * (range_a * M2_b - range_b * M2_a) / n_ab;
        M4_a = M4_a + M4_b + (delta * delta * delta * delta) * range_a * range_b * (range_a * range_a - range_a * range_b + range_b * range_b) / (n_ab * n_ab * n_ab)
               + Real(6) * delta * delta * (range_a * range_a * M2_b + range_b * range_b * M2_a) / (n_ab * n_ab) 
               + Real(4) * delta * (range_a * M3_b - range_b * M3_a) / n_ab;
        range_a = n_ab;
    }

    return std::make_tuple(M1_a, M2_a, M3_a, M4_a, elements);
}

#endif // BOOST_HAS_THREADS

// Follows equation 1.5 of:
// https://prod.sandia.gov/techlib-noauth/access-control.cgi/2008/086212.pdf
template<typename ReturnType, typename ForwardIterator>
ReturnType skewness_sequential_impl(ForwardIterator first, ForwardIterator last)
{
    using std::sqrt;
    BOOST_MATH_ASSERT_MSG(first != last, "At least one sample is required to compute skewness.");
    
    ReturnType M1 = *first;
    ReturnType M2 = 0;
    ReturnType M3 = 0;
    ReturnType n = 2;
        
    for (auto it = std::next(first); it != last; ++it)    
    {
        ReturnType delta21 = *it - M1;
        ReturnType tmp = delta21/n;
        M3 += tmp*((n-1)*(n-2)*delta21*tmp - 3*M2);
        M2 += tmp*(n-1)*delta21;
        M1 += tmp;
        n += 1;
    }
   
    ReturnType var = M2/(n-1);
    
    if (var == 0)
    {
        // The limit is technically undefined, but the interpretation here is clear:
        // A constant dataset has no skewness.
        return ReturnType(0);
    }
    
    ReturnType skew = M3/(M2*sqrt(var));
    return skew;
}

template<typename ReturnType, typename ForwardIterator>
ReturnType gini_coefficient_sequential_impl(ForwardIterator first, ForwardIterator last)
{
    ReturnType i = 1;
    ReturnType num = 0;
    ReturnType denom = 0;

    for(auto it = first; it != last; ++it)
    {
        num += *it*i;
        denom += *it;
        ++i;
    }

    // If the l1 norm is zero, all elements are zero, so every element is the same.
    if(denom == 0)
    {
        return ReturnType(0);
    }
    else
    {
        return ((2*num)/denom - i)/(i-1);
    }
}

template<typename ReturnType, typename ForwardIterator>
ReturnType gini_range_fraction(ForwardIterator first, ForwardIterator last, std::size_t starting_index)
{
    using Real = typename std::tuple_element<0, ReturnType>::type;

    std::size_t i = starting_index + 1;
    Real num = 0;
    Real denom = 0;

    for(auto it = first; it != last; ++it)
    {
        num += *it*i;
        denom += *it;
        ++i;
    }

    return std::make_tuple(num, denom, i);
}

#ifdef BOOST_HAS_THREADS

template<typename ReturnType, typename ExecutionPolicy, typename ForwardIterator>
ReturnType gini_coefficient_parallel_impl(ExecutionPolicy&&, ForwardIterator first, ForwardIterator last)
{
    using range_tuple = std::tuple<ReturnType, ReturnType, std::size_t>;
    
    const auto elements = std::distance(first, last);
    const unsigned max_concurrency = std::thread::hardware_concurrency() == 0 ? 2u : std::thread::hardware_concurrency();
    unsigned num_threads = 2u;
    
    // Threading is faster for: 10 + 10.12e-3 N/j <= 10.12e-3N => N >= 10^4j/10.12(j-1).
    const auto parallel_lower_bound = 10e4*max_concurrency/(10.12*(max_concurrency-1));
    const auto parallel_upper_bound = 10e4*2/10.12; // j = 2

    // https://lemire.me/blog/2020/01/30/cost-of-a-thread-in-c-under-linux/
    if(elements < parallel_lower_bound)
    {
        return gini_coefficient_sequential_impl<ReturnType>(first, last);
    }
    else if(elements >= parallel_upper_bound)
    {
        num_threads = max_concurrency;
    }
    else
    {
        for(unsigned i = 3; i < max_concurrency; ++i)
        {
            if(parallel_lower_bound < 10e4*i/(10.12*(i-1)))
            {
                num_threads = i;
                break;
            }
        }
    }

    std::vector<std::future<range_tuple>> future_manager;
    const auto elements_per_thread = std::ceil(static_cast<double>(elements) / num_threads);

    auto it = first;
    for(std::size_t i {}; i < num_threads - 1; ++i)
    {
        future_manager.emplace_back(std::async(std::launch::async | std::launch::deferred, [it, elements_per_thread, i]() -> range_tuple
        {
            return gini_range_fraction<range_tuple>(it, std::next(it, elements_per_thread), i*elements_per_thread);
        }));
        it = std::next(it, elements_per_thread);
    }

    future_manager.emplace_back(std::async(std::launch::async | std::launch::deferred, [it, last, num_threads, elements_per_thread]() -> range_tuple
    {
        return gini_range_fraction<range_tuple>(it, last, (num_threads - 1)*elements_per_thread);
    }));

    ReturnType num = 0;
    ReturnType denom = 0;

    for(std::size_t i = 0; i < future_manager.size(); ++i)
    {
        auto temp = future_manager[i].get();
        num += std::get<0>(temp);
        denom += std::get<1>(temp);
    }

    // If the l1 norm is zero, all elements are zero, so every element is the same.
    if(denom == 0)
    {
        return ReturnType(0);
    }
    else
    {
        return ((2*num)/denom - elements)/(elements-1);
    }
}

#endif // BOOST_HAS_THREADS

template<typename ForwardIterator, typename OutputIterator>
OutputIterator mode_impl(ForwardIterator first, ForwardIterator last, OutputIterator output)
{
    using Z = typename std::iterator_traits<ForwardIterator>::value_type;
    using Size = typename std::iterator_traits<ForwardIterator>::difference_type;

    std::vector<Z> modes {};
    modes.reserve(16);
    Size max_counter {0};

    while(first != last)
    {
        Size current_count {0};
        ForwardIterator end_it {first};
        while(end_it != last && *end_it == *first)
        {
            ++current_count;
            ++end_it;
        }

        if(current_count > max_counter)
        {
            modes.resize(1);
            modes[0] = *first;
            max_counter = current_count;
        }

        else if(current_count == max_counter)
        {
            modes.emplace_back(*first);
        }

        first = end_it;
    }

    return std::move(modes.begin(), modes.end(), output);
}
}}}}

#endif // BOOST_MATH_STATISTICS_UNIVARIATE_STATISTICS_DETAIL_SINGLE_PASS_HPP
