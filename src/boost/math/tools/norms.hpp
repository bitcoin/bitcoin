//  (C) Copyright Nick Thompson 2018.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_TOOLS_NORMS_HPP
#define BOOST_MATH_TOOLS_NORMS_HPP
#include <algorithm>
#include <iterator>
#include <complex>
#include <cmath>
#include <boost/math/tools/assert.hpp>
#include <boost/math/tools/complex.hpp>


namespace boost::math::tools {

// Mallat, "A Wavelet Tour of Signal Processing", equation 2.60:
template<class ForwardIterator>
auto total_variation(ForwardIterator first, ForwardIterator last)
{
    using T = typename std::iterator_traits<ForwardIterator>::value_type;
    using std::abs;
    BOOST_MATH_ASSERT_MSG(first != last && std::next(first) != last, "At least two samples are required to compute the total variation.");
    auto it = first;
    if constexpr (std::is_unsigned<T>::value)
    {
        T tmp = *it;
        double tv = 0;
        while (++it != last)
        {
            if (*it > tmp)
            {
                tv += *it - tmp;
            }
            else
            {
                tv += tmp - *it;
            }
            tmp = *it;
        }
        return tv;
    }
    else if constexpr (std::is_integral<T>::value)
    {
        double tv = 0;
        double tmp = *it;
        while(++it != last)
        {
            double tmp2 = *it;
            tv += abs(tmp2 - tmp);
            tmp = *it;
        }
        return tv;
    }
    else
    {
        T tmp = *it;
        T tv = 0;
        while (++it != last)
        {
            tv += abs(*it - tmp);
            tmp = *it;
        }
        return tv;
    }
}

template<class Container>
inline auto total_variation(Container const & v)
{
    return total_variation(v.cbegin(), v.cend());
}


template<class ForwardIterator>
auto sup_norm(ForwardIterator first, ForwardIterator last)
{
    BOOST_MATH_ASSERT_MSG(first != last, "At least one value is required to compute the sup norm.");
    using T = typename std::iterator_traits<ForwardIterator>::value_type;
    using std::abs;
    if constexpr (boost::math::tools::is_complex_type<T>::value)
    {
        auto it = std::max_element(first, last, [](T a, T b) { return abs(b) > abs(a); });
        return abs(*it);
    }
    else if constexpr (std::is_unsigned<T>::value)
    {
        return *std::max_element(first, last);
    }
    else
    {
        auto pair = std::minmax_element(first, last);
        if (abs(*pair.first) > abs(*pair.second))
        {
            return abs(*pair.first);
        }
        else
        {
            return abs(*pair.second);
        }
    }
}

template<class Container>
inline auto sup_norm(Container const & v)
{
    return sup_norm(v.cbegin(), v.cend());
}

template<class ForwardIterator>
auto l1_norm(ForwardIterator first, ForwardIterator last)
{
    using T = typename std::iterator_traits<ForwardIterator>::value_type;
    using std::abs;
    if constexpr (std::is_unsigned<T>::value)
    {
        double l1 = 0;
        for (auto it = first; it != last; ++it)
        {
            l1 += *it;
        }
        return l1;
    }
    else if constexpr (std::is_integral<T>::value)
    {
        double l1 = 0;
        for (auto it = first; it != last; ++it)
        {
            double tmp = *it;
            l1 += abs(tmp);
        }
        return l1;
    }
    else
    {
        decltype(abs(*first)) l1 = 0;
        for (auto it = first; it != last; ++it)
        {
            l1 += abs(*it);
        }
        return l1;
    }

}

template<class Container>
inline auto l1_norm(Container const & v)
{
    return l1_norm(v.cbegin(), v.cend());
}


template<class ForwardIterator>
auto l2_norm(ForwardIterator first, ForwardIterator last)
{
    using T = typename std::iterator_traits<ForwardIterator>::value_type;
    using std::abs;
    using std::norm;
    using std::sqrt;
    using std::is_floating_point;
    using std::isfinite;
    if constexpr (boost::math::tools::is_complex_type<T>::value)
    {
        typedef typename T::value_type Real;
        Real l2 = 0;
        for (auto it = first; it != last; ++it)
        {
            l2 += norm(*it);
        }
        Real result = sqrt(l2);
        if (!isfinite(result))
        {
            Real a = sup_norm(first, last);
            l2 = 0;
            for (auto it = first; it != last; ++it)
            {
                l2 += norm(*it/a);
            }
            return a*sqrt(l2);
        }
        return result;
    }
    else if constexpr (is_floating_point<T>::value ||
                       std::numeric_limits<T>::max_exponent)
    {
        T l2 = 0;
        for (auto it = first; it != last; ++it)
        {
            l2 += (*it)*(*it);
        }
        T result = sqrt(l2);
        // Higham, Accuracy and Stability of Numerical Algorithms,
        // Problem 27.5 presents a different algorithm to deal with overflow.
        // The algorithm used here takes 3 passes *if* there is overflow.
        // Higham's algorithm is 1 pass, but more requires operations than the no overflow case.
        // I'm operating under the assumption that overflow is rare since the dynamic range of floating point numbers is huge.
        if (!isfinite(result))
        {
            T a = sup_norm(first, last);
            l2 = 0;
            for (auto it = first; it != last; ++it)
            {
                T tmp = *it/a;
                l2 += tmp*tmp;
            }
            return a*sqrt(l2);
        }
        return result;
    }
    else
    {
        double l2 = 0;
        for (auto it = first; it != last; ++it)
        {
            double tmp = *it;
            l2 += tmp*tmp;
        }
        return sqrt(l2);
    }
}

template<class Container>
inline auto l2_norm(Container const & v)
{
    return l2_norm(v.cbegin(), v.cend());
}

template<class ForwardIterator>
size_t l0_pseudo_norm(ForwardIterator first, ForwardIterator last)
{
    using RealOrComplex = typename std::iterator_traits<ForwardIterator>::value_type;
    size_t count = 0;
    for (auto it = first; it != last; ++it)
    {
        if (*it != RealOrComplex(0))
        {
            ++count;
        }
    }
    return count;
}

template<class Container>
inline size_t l0_pseudo_norm(Container const & v)
{
    return l0_pseudo_norm(v.cbegin(), v.cend());
}

template<class ForwardIterator>
size_t hamming_distance(ForwardIterator first1, ForwardIterator last1, ForwardIterator first2)
{
    size_t count = 0;
    auto it1 = first1;
    auto it2 = first2;
    while (it1 != last1)
    {
        if (*it1++ != *it2++)
        {
            ++count;
        }
    }
    return count;
}

template<class Container>
inline size_t hamming_distance(Container const & v, Container const & w)
{
    return hamming_distance(v.cbegin(), v.cend(), w.cbegin());
}

template<class ForwardIterator>
auto lp_norm(ForwardIterator first, ForwardIterator last, unsigned p)
{
    using std::abs;
    using std::pow;
    using std::is_floating_point;
    using std::isfinite;
    using RealOrComplex = typename std::iterator_traits<ForwardIterator>::value_type;
    if constexpr (boost::math::tools::is_complex_type<RealOrComplex>::value)
    {
        using std::norm;
        using Real = typename RealOrComplex::value_type;
        Real lp = 0;
        for (auto it = first; it != last; ++it)
        {
            lp += pow(abs(*it), p);
        }

        auto result = pow(lp, Real(1)/Real(p));
        if (!isfinite(result))
        {
            auto a = boost::math::tools::sup_norm(first, last);
            Real lp = 0;
            for (auto it = first; it != last; ++it)
            {
                lp += pow(abs(*it)/a, p);
            }
            result = a*pow(lp, Real(1)/Real(p));
        }
        return result;
    }
    else if constexpr (is_floating_point<RealOrComplex>::value || std::numeric_limits<RealOrComplex>::max_exponent)
    {
        BOOST_MATH_ASSERT_MSG(p >= 0, "For p < 0, the lp norm is not a norm");
        RealOrComplex lp = 0;

        for (auto it = first; it != last; ++it)
        {
            lp += pow(abs(*it), p);
        }

        RealOrComplex result = pow(lp, RealOrComplex(1)/RealOrComplex(p));
        if (!isfinite(result))
        {
            RealOrComplex a = boost::math::tools::sup_norm(first, last);
            lp = 0;
            for (auto it = first; it != last; ++it)
            {
                lp += pow(abs(*it)/a, p);
            }
            result = a*pow(lp, RealOrComplex(1)/RealOrComplex(p));
        }
        return result;
    }
    else
    {
        double lp = 0;

        for (auto it = first; it != last; ++it)
        {
            double tmp = *it;
            lp += pow(abs(tmp), p);
        }
        double result = pow(lp, 1.0/double(p));
        if (!isfinite(result))
        {
            double a = boost::math::tools::sup_norm(first, last);
            lp = 0;
            for (auto it = first; it != last; ++it)
            {
                double tmp = *it;
                lp += pow(abs(tmp)/a, p);
            }
            result = a*pow(lp, double(1)/double(p));
        }
        return result;
    }
}

template<class Container>
inline auto lp_norm(Container const & v, unsigned p)
{
    return lp_norm(v.cbegin(), v.cend(), p);
}


template<class ForwardIterator>
auto lp_distance(ForwardIterator first1, ForwardIterator last1, ForwardIterator first2, unsigned p)
{
    using std::pow;
    using std::abs;
    using std::is_floating_point;
    using std::isfinite;
    using RealOrComplex = typename std::iterator_traits<ForwardIterator>::value_type;
    auto it1 = first1;
    auto it2 = first2;

    if constexpr (boost::math::tools::is_complex_type<RealOrComplex>::value)
    {
        using Real = typename RealOrComplex::value_type;
        using std::norm;
        Real dist = 0;
        while(it1 != last1)
        {
            auto tmp = *it1++ - *it2++;
            dist += pow(abs(tmp), p);
        }
        return pow(dist, Real(1)/Real(p));
    }
    else if constexpr (is_floating_point<RealOrComplex>::value || std::numeric_limits<RealOrComplex>::max_exponent)
    {
        RealOrComplex dist = 0;
        while(it1 != last1)
        {
            auto tmp = *it1++ - *it2++;
            dist += pow(abs(tmp), p);
        }
        return pow(dist, RealOrComplex(1)/RealOrComplex(p));
    }
    else
    {
        double dist = 0;
        while(it1 != last1)
        {
            double tmp1 = *it1++;
            double tmp2 = *it2++;
            // Naively you'd expect the integer subtraction to be faster,
            // but this can overflow or wraparound:
            //double tmp = *it1++ - *it2++;
            dist += pow(abs(tmp1 - tmp2), p);
        }
        return pow(dist, 1.0/double(p));
    }
}

template<class Container>
inline auto lp_distance(Container const & v, Container const & w, unsigned p)
{
    return lp_distance(v.cbegin(), v.cend(), w.cbegin(), p);
}


template<class ForwardIterator>
auto l1_distance(ForwardIterator first1, ForwardIterator last1, ForwardIterator first2)
{
    using std::abs;
    using std::is_floating_point;
    using std::isfinite;
    using T = typename std::iterator_traits<ForwardIterator>::value_type;
    auto it1 = first1;
    auto it2 = first2;
    if constexpr (boost::math::tools::is_complex_type<T>::value)
    {
        using Real = typename T::value_type;
        Real sum = 0;
        while (it1 != last1) {
            sum += abs(*it1++ - *it2++);
        }
        return sum;
    }
    else if constexpr (is_floating_point<T>::value || std::numeric_limits<T>::max_exponent)
    {
        T sum = 0;
        while (it1 != last1)
        {
            sum += abs(*it1++ - *it2++);
        }
        return sum;
    }
    else if constexpr (std::is_unsigned<T>::value)
    {
        double sum = 0;
        while(it1 != last1)
        {
            T x1 = *it1++;
            T x2 = *it2++;
            if (x1 > x2)
            {
                sum += (x1 - x2);
            }
            else
            {
                sum += (x2 - x1);
            }
        }
        return sum;
    }
    else if constexpr (std::is_integral<T>::value)
    {
        double sum = 0;
        while(it1 != last1)
        {
            double x1 = *it1++;
            double x2 = *it2++;
            sum += abs(x1-x2);
        }
        return sum;
    }
    else
    {
        BOOST_MATH_ASSERT_MSG(false, "Could not recognize type.");
    }

}

template<class Container>
auto l1_distance(Container const & v, Container const & w)
{
    using std::size;
    BOOST_MATH_ASSERT_MSG(size(v) == size(w),
                     "L1 distance requires both containers to have the same number of elements");
    return l1_distance(v.cbegin(), v.cend(), w.begin());
}

template<class ForwardIterator>
auto l2_distance(ForwardIterator first1, ForwardIterator last1, ForwardIterator first2)
{
    using std::abs;
    using std::norm;
    using std::sqrt;
    using std::is_floating_point;
    using std::isfinite;
    using T = typename std::iterator_traits<ForwardIterator>::value_type;
    auto it1 = first1;
    auto it2 = first2;
    if constexpr (boost::math::tools::is_complex_type<T>::value)
    {
        using Real = typename T::value_type;
        Real sum = 0;
        while (it1 != last1) {
            sum += norm(*it1++ - *it2++);
        }
        return sqrt(sum);
    }
    else if constexpr (is_floating_point<T>::value || std::numeric_limits<T>::max_exponent)
    {
        T sum = 0;
        while (it1 != last1)
        {
            T tmp = *it1++ - *it2++;
            sum += tmp*tmp;
        }
        return sqrt(sum);
    }
    else if constexpr (std::is_unsigned<T>::value)
    {
        double sum = 0;
        while(it1 != last1)
        {
            T x1 = *it1++;
            T x2 = *it2++;
            if (x1 > x2)
            {
                double tmp = x1-x2;
                sum += tmp*tmp;
            }
            else
            {
                double tmp = x2 - x1;
                sum += tmp*tmp;
            }
        }
        return sqrt(sum);
    }
    else
    {
        double sum = 0;
        while(it1 != last1)
        {
            double x1 = *it1++;
            double x2 = *it2++;
            double tmp = x1-x2;
            sum += tmp*tmp;
        }
        return sqrt(sum);
    }
}

template<class Container>
auto l2_distance(Container const & v, Container const & w)
{
    using std::size;
    BOOST_MATH_ASSERT_MSG(size(v) == size(w),
                     "L2 distance requires both containers to have the same number of elements");
    return l2_distance(v.cbegin(), v.cend(), w.begin());
}

template<class ForwardIterator>
auto sup_distance(ForwardIterator first1, ForwardIterator last1, ForwardIterator first2)
{
    using std::abs;
    using std::norm;
    using std::sqrt;
    using std::is_floating_point;
    using std::isfinite;
    using T = typename std::iterator_traits<ForwardIterator>::value_type;
    auto it1 = first1;
    auto it2 = first2;
    if constexpr (boost::math::tools::is_complex_type<T>::value)
    {
        using Real = typename T::value_type;
        Real sup_sq = 0;
        while (it1 != last1) {
            Real tmp = norm(*it1++ - *it2++);
            if (tmp > sup_sq) {
                sup_sq = tmp;
            }
        }
        return sqrt(sup_sq);
    }
    else if constexpr (is_floating_point<T>::value || std::numeric_limits<T>::max_exponent)
    {
        T sup = 0;
        while (it1 != last1)
        {
            T tmp = *it1++ - *it2++;
            if (sup < abs(tmp))
            {
                sup = abs(tmp);
            }
        }
        return sup;
    }
    else // integral values:
    {
        double sup = 0;
        while(it1 != last1)
        {
            T x1 = *it1++;
            T x2 = *it2++;
            double tmp;
            if (x1 > x2)
            {
                tmp = x1-x2;
            }
            else
            {
                tmp = x2 - x1;
            }
            if (sup < tmp) {
                sup = tmp;
            }
        }
        return sup;
    }
}

template<class Container>
auto sup_distance(Container const & v, Container const & w)
{
    using std::size;
    BOOST_MATH_ASSERT_MSG(size(v) == size(w),
                     "sup distance requires both containers to have the same number of elements");
    return sup_distance(v.cbegin(), v.cend(), w.begin());
}


}
#endif
