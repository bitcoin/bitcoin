// (C) Copyright Jeremy Siek 2001.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PERMUTATION_HPP
#define BOOST_PERMUTATION_HPP

#include <vector>
#include <memory>
#include <functional>
#include <algorithm>
#include <boost/graph/detail/shadow_iterator.hpp>

namespace boost
{

template < class Iter1, class Iter2 >
void permute_serial(Iter1 permuter, Iter1 last, Iter2 result)
{
#ifdef BOOST_NO_STD_ITERATOR_TRAITS
    typedef std::ptrdiff_t D :
#else
    typedef typename std::iterator_traits< Iter1 >::difference_type D;
#endif

        D n
        = 0;
    while (permuter != last)
    {
        std::swap(result[n], result[*permuter]);
        ++n;
        ++permuter;
    }
}

template < class InIter, class RandIterP, class RandIterR >
void permute_copy(InIter first, InIter last, RandIterP p, RandIterR result)
{
#ifdef BOOST_NO_STD_ITERATOR_TRAITS
    typedef std::ptrdiff_t i = 0;
#else
    typename std::iterator_traits< RandIterP >::difference_type i = 0;
#endif
    for (; first != last; ++first, ++i)
        result[p[i]] = *first;
}

namespace detail
{

    template < class RandIter, class RandIterPerm, class D, class T >
    void permute_helper(RandIter first, RandIter last, RandIterPerm p, D, T)
    {
        D i = 0, pi, n = last - first, cycle_start;
        T tmp;
        std::vector< int > visited(n, false);

        while (i != n)
        { // continue until all elements have been processed
            cycle_start = i;
            tmp = first[i];
            do
            { // walk around a cycle
                pi = p[i];
                visited[pi] = true;
                std::swap(tmp, first[pi]);
                i = pi;
            } while (i != cycle_start);

            // find the next cycle
            for (i = 0; i < n; ++i)
                if (visited[i] == false)
                    break;
        }
    }

} // namespace detail

template < class RandIter, class RandIterPerm >
void permute(RandIter first, RandIter last, RandIterPerm p)
{
    detail::permute_helper(first, last, p, last - first, *first);
}

// Knuth 1.3.3, Vol. 1 p 176
// modified for zero-based arrays
// time complexity?
//
// WARNING: T must be a signed integer!
template < class PermIter > void invert_permutation(PermIter X, PermIter Xend)
{
#ifdef BOOST_NO_STD_ITERATOR_TRAITS
    typedef std::ptrdiff_t T :
#else
    typedef typename std::iterator_traits< PermIter >::value_type T;
#endif
        T n
        = Xend - X;
    T m = n;
    T j = -1;

    while (m > 0)
    {
        T i = X[m - 1] + 1;
        if (i > 0)
        {
            do
            {
                X[m - 1] = j - 1;
                j = -m;
                m = i;
                i = X[m - 1] + 1;
            } while (i > 0);
            i = j;
        }
        X[m - 1] = -i - 1;
        --m;
    }
}

// Takes a "normal" permutation array (and its inverse), and turns it
// into a BLAS-style permutation array (which can be thought of as a
// serialized permutation).
template < class Iter1, class Iter2, class Iter3 >
inline void serialize_permutation(Iter1 q, Iter1 q_end, Iter2 q_inv, Iter3 p)
{
#ifdef BOOST_NO_STD_ITERATOR_TRAITS
    typedef std::ptrdiff_t P1;
    typedef std::ptrdiff_t P2;
    typedef std::ptrdiff_t D;
#else
    typedef typename std::iterator_traits< Iter1 >::value_type P1;
    typedef typename std::iterator_traits< Iter2 >::value_type P2;
    typedef typename std::iterator_traits< Iter1 >::difference_type D;
#endif
    D n = q_end - q;
    for (D i = 0; i < n; ++i)
    {
        P1 qi = q[i];
        P2 qii = q_inv[i];
        *p++ = qii;
        std::swap(q[i], q[qii]);
        std::swap(q_inv[i], q_inv[qi]);
    }
}

// Not used anymore, leaving it here for future reference.
template < typename Iter, typename Compare >
void merge_sort(Iter first, Iter last, Compare cmp)
{
    if (first + 1 < last)
    {
        Iter mid = first + (last - first) / 2;
        merge_sort(first, mid, cmp);
        merge_sort(mid, last, cmp);
        std::inplace_merge(first, mid, last, cmp);
    }
}

// time: N log N + 3N + ?
// space: 2N
template < class Iter, class IterP, class Cmp, class Alloc >
inline void sortp(Iter first, Iter last, IterP p, Cmp cmp, Alloc alloc)
{
    typedef typename std::iterator_traits< IterP >::value_type P;
    typedef typename std::iterator_traits< IterP >::difference_type D;
    D n = last - first;
    std::vector< P, Alloc > q(n);
    for (D i = 0; i < n; ++i)
        q[i] = i;
    std::sort(make_shadow_iter(first, q.begin()),
        make_shadow_iter(last, q.end()), shadow_cmp< Cmp >(cmp));
    invert_permutation(q.begin(), q.end());
    std::copy(q.begin(), q.end(), p);
}

template < class Iter, class IterP, class Cmp >
inline void sortp(Iter first, Iter last, IterP p, Cmp cmp)
{
    typedef typename std::iterator_traits< IterP >::value_type P;
    sortp(first, last, p, cmp, std::allocator< P >());
}

template < class Iter, class IterP >
inline void sortp(Iter first, Iter last, IterP p)
{
    typedef typename std::iterator_traits< Iter >::value_type T;
    typedef typename std::iterator_traits< IterP >::value_type P;
    sortp(first, last, p, std::less< T >(), std::allocator< P >());
}

template < class Iter, class IterP, class Cmp, class Alloc >
inline void sortv(Iter first, Iter last, IterP p, Cmp cmp, Alloc alloc)
{
    typedef typename std::iterator_traits< IterP >::value_type P;
    typedef typename std::iterator_traits< IterP >::difference_type D;
    D n = last - first;
    std::vector< P, Alloc > q(n), q_inv(n);
    for (D i = 0; i < n; ++i)
        q_inv[i] = i;
    std::sort(make_shadow_iter(first, q_inv.begin()),
        make_shadow_iter(last, q_inv.end()), shadow_cmp< Cmp >(cmp));
    std::copy(q_inv, q_inv.end(), q.begin());
    invert_permutation(q.begin(), q.end());
    serialize_permutation(q.begin(), q.end(), q_inv.end(), p);
}

} // namespace boost

#endif // BOOST_PERMUTATION_HPP
