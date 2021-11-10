//  (C) Copyright Nick Thompson 2018.
//  (C) Copyright Matt Borland 2021.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_STATISTICS_BIVARIATE_STATISTICS_HPP
#define BOOST_MATH_STATISTICS_BIVARIATE_STATISTICS_HPP

#include <iterator>
#include <tuple>
#include <type_traits>
#include <stdexcept>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <boost/math/tools/assert.hpp>
#include <boost/math/tools/config.hpp>

// Support compilers with P0024R2 implemented without linking TBB
// https://en.cppreference.com/w/cpp/compiler_support
#if !defined(BOOST_NO_CXX17_HDR_EXECUTION) && defined(BOOST_HAS_THREADS)
#include <execution>
#include <future>
#include <thread>
#define EXEC_COMPATIBLE
#endif

namespace boost{ namespace math{ namespace statistics { namespace detail {

// See Equation III.9 of "Numerically Stable, Single-Pass, Parallel Statistics Algorithms", Bennet et al.
template<typename ReturnType, typename ForwardIterator>
ReturnType means_and_covariance_seq_impl(ForwardIterator u_begin, ForwardIterator u_end, ForwardIterator v_begin, ForwardIterator v_end)
{
    using Real = typename std::tuple_element<0, ReturnType>::type;

    Real cov = 0;
    ForwardIterator u_it = u_begin;
    ForwardIterator v_it = v_begin;
    Real mu_u = *u_it++;
    Real mu_v = *v_it++;
    std::size_t i = 1;

    while(u_it != u_end && v_it != v_end)
    {
        Real u_temp = (*u_it++ - mu_u)/(i+1);
        Real v_temp = *v_it++ - mu_v;
        cov += i*u_temp*v_temp;
        mu_u = mu_u + u_temp;
        mu_v = mu_v + v_temp/(i+1);
        i = i + 1;
    }

    if(u_it != u_end || v_it != v_end)
    {
        throw std::domain_error("The size of each sample set must be the same to compute covariance");
    }

    return std::make_tuple(mu_u, mu_v, cov/i, i);
}

#ifdef EXEC_COMPATIBLE

// Numerically stable parallel computation of (co-)variance
// https://dl.acm.org/doi/10.1145/3221269.3223036
template<typename ReturnType, typename ForwardIterator>
ReturnType means_and_covariance_parallel_impl(ForwardIterator u_begin, ForwardIterator u_end, ForwardIterator v_begin, ForwardIterator v_end)
{
    using Real = typename std::tuple_element<0, ReturnType>::type;

    const auto u_elements = std::distance(u_begin, u_end);
    const auto v_elements = std::distance(v_begin, v_end);

    if(u_elements != v_elements)
    {
        throw std::domain_error("The size of each sample set must be the same to compute covariance");
    }

    const unsigned max_concurrency = std::thread::hardware_concurrency() == 0 ? 2u : std::thread::hardware_concurrency();
    unsigned num_threads = 2u;
    
    // 5.16 comes from benchmarking. See boost/math/reporting/performance/bivariate_statistics_performance.cpp
    // Threading is faster for: 10 + 5.16e-3 N/j <= 5.16e-3N => N >= 10^4j/5.16(j-1).
    const auto parallel_lower_bound = 10e4*max_concurrency/(5.16*(max_concurrency-1));
    const auto parallel_upper_bound = 10e4*2/5.16; // j = 2

    // https://lemire.me/blog/2020/01/30/cost-of-a-thread-in-c-under-linux/
    if(u_elements < parallel_lower_bound)
    {
        return means_and_covariance_seq_impl<ReturnType>(u_begin, u_end, v_begin, v_end);
    }
    else if(u_elements >= parallel_upper_bound)
    {
        num_threads = max_concurrency;
    }
    else
    {
        for(unsigned i = 3; i < max_concurrency; ++i)
        {
            if(parallel_lower_bound < 10e4*i/(5.16*(i-1)))
            {
                num_threads = i;
                break;
            }
        }
    }

    std::vector<std::future<ReturnType>> future_manager;
    const auto elements_per_thread = std::ceil(static_cast<double>(u_elements)/num_threads);

    ForwardIterator u_it = u_begin;
    ForwardIterator v_it = v_begin;

    for(std::size_t i = 0; i < num_threads - 1; ++i)
    {
        future_manager.emplace_back(std::async(std::launch::async | std::launch::deferred, [u_it, v_it, elements_per_thread]() -> ReturnType
        {
            return means_and_covariance_seq_impl<ReturnType>(u_it, std::next(u_it, elements_per_thread), v_it, std::next(v_it, elements_per_thread));
        }));
        u_it = std::next(u_it, elements_per_thread);
        v_it = std::next(v_it, elements_per_thread);
    }

    future_manager.emplace_back(std::async(std::launch::async | std::launch::deferred, [u_it, u_end, v_it, v_end]() -> ReturnType
    {
        return means_and_covariance_seq_impl<ReturnType>(u_it, u_end, v_it, v_end);
    }));

    ReturnType temp = future_manager[0].get();
    Real mu_u_a = std::get<0>(temp);
    Real mu_v_a = std::get<1>(temp);
    Real cov_a = std::get<2>(temp);
    Real n_a = std::get<3>(temp);

    for(std::size_t i = 1; i < future_manager.size(); ++i)
    {
        temp = future_manager[i].get();
        Real mu_u_b = std::get<0>(temp);
        Real mu_v_b = std::get<1>(temp);
        Real cov_b = std::get<2>(temp);
        Real n_b = std::get<3>(temp);

        const Real n_ab = n_a + n_b;
        const Real delta_u = mu_u_b - mu_u_a;
        const Real delta_v = mu_v_b - mu_v_a;

        cov_a = cov_a + cov_b + (-delta_u)*(-delta_v)*((n_a*n_b)/n_ab);
        mu_u_a = mu_u_a + delta_u*(n_b/n_ab);
        mu_v_a = mu_v_a + delta_v*(n_b/n_ab);
        n_a = n_ab;
    }

    return std::make_tuple(mu_u_a, mu_v_a, cov_a, n_a);
}

#endif // EXEC_COMPATIBLE

template<typename ReturnType, typename ForwardIterator>
ReturnType correlation_coefficient_seq_impl(ForwardIterator u_begin, ForwardIterator u_end, ForwardIterator v_begin, ForwardIterator v_end)
{
    using Real = typename std::tuple_element<0, ReturnType>::type;
    using std::sqrt;

    Real cov = 0;
    ForwardIterator u_it = u_begin;
    ForwardIterator v_it = v_begin;
    Real mu_u = *u_it++;
    Real mu_v = *v_it++;
    Real Qu = 0;
    Real Qv = 0;
    std::size_t i = 1;

    while(u_it != u_end && v_it != v_end)
    {
        Real u_tmp = *u_it++ - mu_u;
        Real v_tmp = *v_it++ - mu_v;
        Qu = Qu + (i*u_tmp*u_tmp)/(i+1);
        Qv = Qv + (i*v_tmp*v_tmp)/(i+1);
        cov += i*u_tmp*v_tmp/(i+1);
        mu_u = mu_u + u_tmp/(i+1);
        mu_v = mu_v + v_tmp/(i+1);
        ++i;
    }

    // If both datasets are constant, then they are perfectly correlated.
    if (Qu == 0 && Qv == 0)
    {
        return std::make_tuple(mu_u, Qu, mu_v, Qv, cov, Real(1), i);
    }
    // If one dataset is constant and the other isn't, then they have no correlation:
    if (Qu == 0 || Qv == 0)
    {
        return std::make_tuple(mu_u, Qu, mu_v, Qv, cov, Real(0), i);
    }

    // Make sure rho in [-1, 1], even in the presence of numerical noise.
    Real rho = cov/sqrt(Qu*Qv);
    if (rho > 1) {
        rho = 1;
    }
    if (rho < -1) {
        rho = -1;
    }

    return std::make_tuple(mu_u, Qu, mu_v, Qv, cov, rho, i);
}

#ifdef EXEC_COMPATIBLE

// Numerically stable parallel computation of (co-)variance:
// https://dl.acm.org/doi/10.1145/3221269.3223036
//
// Parallel computation of variance:
// http://i.stanford.edu/pub/cstr/reports/cs/tr/79/773/CS-TR-79-773.pdf
template<typename ReturnType, typename ForwardIterator>
ReturnType correlation_coefficient_parallel_impl(ForwardIterator u_begin, ForwardIterator u_end, ForwardIterator v_begin, ForwardIterator v_end)
{
    using Real = typename std::tuple_element<0, ReturnType>::type;

    const auto u_elements = std::distance(u_begin, u_end);
    const auto v_elements = std::distance(v_begin, v_end);

    if(u_elements != v_elements)
    {
        throw std::domain_error("The size of each sample set must be the same to compute covariance");
    }

    const unsigned max_concurrency = std::thread::hardware_concurrency() == 0 ? 2u : std::thread::hardware_concurrency();
    unsigned num_threads = 2u;
    
    // 3.25 comes from benchmarking. See boost/math/reporting/performance/bivariate_statistics_performance.cpp
    // Threading is faster for: 10 + 3.25e-3 N/j <= 3.25e-3N => N >= 10^4j/3.25(j-1).
    const auto parallel_lower_bound = 10e4*max_concurrency/(3.25*(max_concurrency-1));
    const auto parallel_upper_bound = 10e4*2/3.25; // j = 2

    // https://lemire.me/blog/2020/01/30/cost-of-a-thread-in-c-under-linux/
    if(u_elements < parallel_lower_bound)
    {
        return correlation_coefficient_seq_impl<ReturnType>(u_begin, u_end, v_begin, v_end);
    }
    else if(u_elements >= parallel_upper_bound)
    {
        num_threads = max_concurrency;
    }
    else
    {
        for(unsigned i = 3; i < max_concurrency; ++i)
        {
            if(parallel_lower_bound < 10e4*i/(3.25*(i-1)))
            {
                num_threads = i;
                break;
            }
        }
    }

    std::vector<std::future<ReturnType>> future_manager;
    const auto elements_per_thread = std::ceil(static_cast<double>(u_elements)/num_threads);

    ForwardIterator u_it = u_begin;
    ForwardIterator v_it = v_begin;

    for(std::size_t i = 0; i < num_threads - 1; ++i)
    {
        future_manager.emplace_back(std::async(std::launch::async | std::launch::deferred, [u_it, v_it, elements_per_thread]() -> ReturnType
        {
            return correlation_coefficient_seq_impl<ReturnType>(u_it, std::next(u_it, elements_per_thread), v_it, std::next(v_it, elements_per_thread));
        }));
        u_it = std::next(u_it, elements_per_thread);
        v_it = std::next(v_it, elements_per_thread);
    }

    future_manager.emplace_back(std::async(std::launch::async | std::launch::deferred, [u_it, u_end, v_it, v_end]() -> ReturnType
    {
        return correlation_coefficient_seq_impl<ReturnType>(u_it, u_end, v_it, v_end);
    }));

    ReturnType temp = future_manager[0].get();
    Real mu_u_a = std::get<0>(temp);
    Real Qu_a = std::get<1>(temp);
    Real mu_v_a = std::get<2>(temp);
    Real Qv_a = std::get<3>(temp);
    Real cov_a = std::get<4>(temp);
    Real n_a = std::get<6>(temp);

    for(std::size_t i = 1; i < future_manager.size(); ++i)
    {
        temp = future_manager[i].get();
        Real mu_u_b = std::get<0>(temp);
        Real Qu_b = std::get<1>(temp);
        Real mu_v_b = std::get<2>(temp);
        Real Qv_b = std::get<3>(temp);
        Real cov_b = std::get<4>(temp);
        Real n_b = std::get<6>(temp);

        const Real n_ab = n_a + n_b;
        const Real delta_u = mu_u_b - mu_u_a;
        const Real delta_v = mu_v_b - mu_v_a;

        cov_a = cov_a + cov_b + (-delta_u)*(-delta_v)*((n_a*n_b)/n_ab);
        mu_u_a = mu_u_a + delta_u*(n_b/n_ab);
        mu_v_a = mu_v_a + delta_v*(n_b/n_ab);
        Qu_a = Qu_a + Qu_b + delta_u*delta_u*((n_a*n_b)/n_ab);
        Qv_b = Qv_a + Qv_b + delta_v*delta_v*((n_a*n_b)/n_ab);
        n_a = n_ab;
    }

    // If both datasets are constant, then they are perfectly correlated.
    if (Qu_a == 0 && Qv_a == 0)
    {
        return std::make_tuple(mu_u_a, Qu_a, mu_v_a, Qv_a, cov_a, Real(1), n_a);
    }
    // If one dataset is constant and the other isn't, then they have no correlation:
    if (Qu_a == 0 || Qv_a == 0)
    {
        return std::make_tuple(mu_u_a, Qu_a, mu_v_a, Qv_a, cov_a, Real(0), n_a);
    }

    // Make sure rho in [-1, 1], even in the presence of numerical noise.
    Real rho = cov_a/sqrt(Qu_a*Qv_a);
    if (rho > 1) {
        rho = 1;
    }
    if (rho < -1) {
        rho = -1;
    }

    return std::make_tuple(mu_u_a, Qu_a, mu_v_a, Qv_a, cov_a, rho, n_a);
}

#endif // EXEC_COMPATIBLE

} // namespace detail

#ifdef EXEC_COMPATIBLE

template<typename ExecutionPolicy, typename Container, typename Real = typename Container::value_type>
inline auto means_and_covariance(ExecutionPolicy&& exec, Container const & u, Container const & v)
{
    if constexpr (std::is_same_v<std::remove_reference_t<decltype(exec)>, decltype(std::execution::seq)>)
    {
        if constexpr (std::is_integral_v<Real>)
        {
            using ReturnType = std::tuple<double, double, double, double>;
            ReturnType temp = detail::means_and_covariance_seq_impl<ReturnType>(std::begin(u), std::end(u), std::begin(v), std::end(v));
            return std::make_tuple(std::get<0>(temp), std::get<1>(temp), std::get<2>(temp));
        }
        else
        {
            using ReturnType = std::tuple<Real, Real, Real, Real>;
            ReturnType temp = detail::means_and_covariance_seq_impl<ReturnType>(std::begin(u), std::end(u), std::begin(v), std::end(v));
            return std::make_tuple(std::get<0>(temp), std::get<1>(temp), std::get<2>(temp));
        }
    }
    else
    {
        if constexpr (std::is_integral_v<Real>)
        {
            using ReturnType = std::tuple<double, double, double, double>;
            ReturnType temp = detail::means_and_covariance_parallel_impl<ReturnType>(std::begin(u), std::end(u), std::begin(v), std::end(v));
            return std::make_tuple(std::get<0>(temp), std::get<1>(temp), std::get<2>(temp));
        }
        else
        {
            using ReturnType = std::tuple<Real, Real, Real, Real>;
            ReturnType temp = detail::means_and_covariance_parallel_impl<ReturnType>(std::begin(u), std::end(u), std::begin(v), std::end(v));
            return std::make_tuple(std::get<0>(temp), std::get<1>(temp), std::get<2>(temp));
        }
    }
}

template<typename Container>
inline auto means_and_covariance(Container const & u, Container const & v)
{
    return means_and_covariance(std::execution::seq, u, v);
}

template<typename ExecutionPolicy, typename Container>
inline auto covariance(ExecutionPolicy&& exec, Container const & u, Container const & v)
{
    return std::get<2>(means_and_covariance(exec, u, v));
}

template<typename Container>
inline auto covariance(Container const & u, Container const & v)
{
    return covariance(std::execution::seq, u, v);
}

template<typename ExecutionPolicy, typename Container, typename Real = typename Container::value_type>
inline auto correlation_coefficient(ExecutionPolicy&& exec, Container const & u, Container const & v)
{
    if constexpr (std::is_same_v<std::remove_reference_t<decltype(exec)>, decltype(std::execution::seq)>)
    {
        if constexpr (std::is_integral_v<Real>)
        {
            using ReturnType = std::tuple<double, double, double, double, double, double, double>;
            return std::get<5>(detail::correlation_coefficient_seq_impl<ReturnType>(std::begin(u), std::end(u), std::begin(v), std::end(v)));
        }
        else
        {
            using ReturnType = std::tuple<Real, Real, Real, Real, Real, Real, Real>;
            return std::get<5>(detail::correlation_coefficient_seq_impl<ReturnType>(std::begin(u), std::end(u), std::begin(v), std::end(v)));
        }
    }
    else
    {
        if constexpr (std::is_integral_v<Real>)
        {
            using ReturnType = std::tuple<double, double, double, double, double, double, double>;
            return std::get<5>(detail::correlation_coefficient_parallel_impl<ReturnType>(std::begin(u), std::end(u), std::begin(v), std::end(v)));
        }
        else
        {
            using ReturnType = std::tuple<Real, Real, Real, Real, Real, Real, Real>;
            return std::get<5>(detail::correlation_coefficient_parallel_impl<ReturnType>(std::begin(u), std::end(u), std::begin(v), std::end(v)));
        }
    }
}

template<typename Container, typename Real = typename Container::value_type>
inline auto correlation_coefficient(Container const & u, Container const & v)
{
    return correlation_coefficient(std::execution::seq, u, v);
}

#else // C++11 and single threaded bindings

template<typename Container, typename Real = typename Container::value_type, typename std::enable_if<std::is_integral<Real>::value, bool>::type = true>
inline auto means_and_covariance(Container const & u, Container const & v) -> std::tuple<double, double, double>
{
    using ReturnType = std::tuple<double, double, double, double>;
    ReturnType temp = detail::means_and_covariance_seq_impl<ReturnType>(std::begin(u), std::end(u), std::begin(v), std::end(v));
    return std::make_tuple(std::get<0>(temp), std::get<1>(temp), std::get<2>(temp));
}

template<typename Container, typename Real = typename Container::value_type, typename std::enable_if<!std::is_integral<Real>::value, bool>::type = true>
inline auto means_and_covariance(Container const & u, Container const & v) -> std::tuple<Real, Real, Real>
{
    using ReturnType = std::tuple<Real, Real, Real, Real>;
    ReturnType temp = detail::means_and_covariance_seq_impl<ReturnType>(std::begin(u), std::end(u), std::begin(v), std::end(v));
    return std::make_tuple(std::get<0>(temp), std::get<1>(temp), std::get<2>(temp));
}

template<typename Container, typename Real = typename Container::value_type, typename std::enable_if<std::is_integral<Real>::value, bool>::type = true>
inline double covariance(Container const & u, Container const & v)
{
    using ReturnType = std::tuple<double, double, double, double>;
    return std::get<2>(detail::means_and_covariance_seq_impl<ReturnType>(std::begin(u), std::end(u), std::begin(v), std::end(v)));
}

template<typename Container, typename Real = typename Container::value_type, typename std::enable_if<!std::is_integral<Real>::value, bool>::type = true>
inline Real covariance(Container const & u, Container const & v)
{
    using ReturnType = std::tuple<Real, Real, Real, Real>;
    return std::get<2>(detail::means_and_covariance_seq_impl<ReturnType>(std::begin(u), std::end(u), std::begin(v), std::end(v)));
}

template<typename Container, typename Real = typename Container::value_type, typename std::enable_if<std::is_integral<Real>::value, bool>::type = true>
inline double correlation_coefficient(Container const & u, Container const & v)
{
    using ReturnType = std::tuple<double, double, double, double, double, double, double>;
    return std::get<5>(detail::correlation_coefficient_seq_impl<ReturnType>(std::begin(u), std::end(u), std::begin(v), std::end(v)));
}

template<typename Container, typename Real = typename Container::value_type, typename std::enable_if<!std::is_integral<Real>::value, bool>::type = true>
inline Real correlation_coefficient(Container const & u, Container const & v)
{
    using ReturnType = std::tuple<Real, Real, Real, Real, Real, Real, Real>;
    return std::get<5>(detail::correlation_coefficient_seq_impl<ReturnType>(std::begin(u), std::end(u), std::begin(v), std::end(v)));
}

#endif

}}} // namespace boost::math::statistics

#endif
