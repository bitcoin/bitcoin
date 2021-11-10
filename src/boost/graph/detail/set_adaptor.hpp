// (C) Copyright Jeremy Siek 2001.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_SET_ADAPTOR_HPP
#define BOOST_SET_ADAPTOR_HPP

#include <set>
#include <boost/unordered_set.hpp>

namespace boost
{

template < class K, class C, class A, class T >
bool set_contains(const std::set< K, C, A >& s, const T& x)
{
    return s.find(x) != s.end();
}

template < class K, class H, class C, class A, class T >
bool set_contains(const boost::unordered_set< K, H, C, A >& s, const T& x)
{
    return s.find(x) != s.end();
}

template < class K, class C, class A >
bool set_equal(const std::set< K, C, A >& x, const std::set< K, C, A >& y)
{
    return x == y;
}

// Not the same as lexicographical_compare_3way applied to std::set.
// this is equivalent semantically to bitset::operator<()
template < class K, class C, class A >
int set_lex_order(const std::set< K, C, A >& x, const std::set< K, C, A >& y)
{
    typename std::set< K, C, A >::iterator xi = x.begin(), yi = y.begin(),
                                           xend = x.end(), yend = y.end();
    for (; xi != xend && yi != yend; ++xi, ++yi)
    {
        if (*xi < *yi)
            return 1;
        else if (*yi < *xi)
            return -1;
    }
    if (xi == xend)
        return (yi == yend) ? 0 : -1;
    else
        return 1;
}

template < class K, class C, class A > void set_clear(std::set< K, C, A >& x)
{
    x.clear();
}

template < class K, class C, class A >
bool set_empty(const std::set< K, C, A >& x)
{
    return x.empty();
}

template < class K, class C, class A, class T >
void set_insert(std::set< K, C, A >& x, const T& a)
{
    x.insert(a);
}

template < class K, class C, class A, class T >
void set_remove(std::set< K, C, A >& x, const T& a)
{
    x.erase(a);
}

template < class K, class C, class A >
void set_intersect(const std::set< K, C, A >& x, const std::set< K, C, A >& y,
    std::set< K, C, A >& z)
{
    z.clear();
    std::set_intersection(
        x.begin(), x.end(), y.begin(), y.end(), std::inserter(z));
}

template < class K, class C, class A >
void set_union(const std::set< K, C, A >& x, const std::set< K, C, A >& y,
    std::set< K, C, A >& z)
{
    z.clear();
    std::set_union(x.begin(), x.end(), y.begin(), y.end(), std::inserter(z));
}

template < class K, class C, class A >
void set_difference(const std::set< K, C, A >& x, const std::set< K, C, A >& y,
    std::set< K, C, A >& z)
{
    z.clear();
    std::set_difference(
        x.begin(), x.end(), y.begin(), y.end(), std::inserter(z, z.begin()));
}

template < class K, class C, class A >
bool set_subset(const std::set< K, C, A >& x, const std::set< K, C, A >& y)
{
    return std::includes(x.begin(), x.end(), y.begin(), y.end());
}

// Shit, can't implement this without knowing the size of the
// universe.
template < class K, class C, class A >
void set_compliment(const std::set< K, C, A >& /*x*/, std::set< K, C, A >& z)
{
    z.clear();
}

} // namespace boost

#endif // BOOST_SET_ADAPTOR_HPP
