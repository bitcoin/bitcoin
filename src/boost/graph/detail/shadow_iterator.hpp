// (C) Copyright Jeremy Siek 2001.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_SHADOW_ITERATOR_HPP
#define BOOST_SHADOW_ITERATOR_HPP

#include <boost/iterator_adaptors.hpp>
#include <boost/operators.hpp>

namespace boost
{

namespace detail
{

    template < class A, class B, class D >
    class shadow_proxy : boost::operators< shadow_proxy< A, B, D > >
    {
        typedef shadow_proxy self;

    public:
        inline shadow_proxy(A aa, B bb) : a(aa), b(bb) {}
        inline shadow_proxy(const self& x) : a(x.a), b(x.b) {}
        template < class Self > inline shadow_proxy(Self x) : a(x.a), b(x.b) {}
        inline self& operator=(const self& x)
        {
            a = x.a;
            b = x.b;
            return *this;
        }
        inline self& operator++()
        {
            ++a;
            return *this;
        }
        inline self& operator--()
        {
            --a;
            return *this;
        }
        inline self& operator+=(const self& x)
        {
            a += x.a;
            return *this;
        }
        inline self& operator-=(const self& x)
        {
            a -= x.a;
            return *this;
        }
        inline self& operator*=(const self& x)
        {
            a *= x.a;
            return *this;
        }
        inline self& operator/=(const self& x)
        {
            a /= x.a;
            return *this;
        }
        inline self& operator%=(const self& x) { return *this; } // JGS
        inline self& operator&=(const self& x) { return *this; } // JGS
        inline self& operator|=(const self& x) { return *this; } // JGS
        inline self& operator^=(const self& x) { return *this; } // JGS
        inline friend D operator-(const self& x, const self& y)
        {
            return x.a - y.a;
        }
        inline bool operator==(const self& x) const { return a == x.a; }
        inline bool operator<(const self& x) const { return a < x.a; }
        //  protected:
        A a;
        B b;
    };

    struct shadow_iterator_policies
    {
        template < typename iter_pair > void initialize(const iter_pair&) {}

        template < typename Iter >
        typename Iter::reference dereference(const Iter& i) const
        {
            typedef typename Iter::reference R;
            return R(*i.base().first, *i.base().second);
        }
        template < typename Iter >
        bool equal(const Iter& p1, const Iter& p2) const
        {
            return p1.base().first == p2.base().first;
        }
        template < typename Iter > void increment(Iter& i)
        {
            ++i.base().first;
            ++i.base().second;
        }

        template < typename Iter > void decrement(Iter& i)
        {
            --i.base().first;
            --i.base().second;
        }

        template < typename Iter > bool less(const Iter& x, const Iter& y) const
        {
            return x.base().first < y.base().first;
        }
        template < typename Iter >
        typename Iter::difference_type distance(
            const Iter& x, const Iter& y) const
        {
            return y.base().first - x.base().first;
        }
        template < typename D, typename Iter > void advance(Iter& p, D n)
        {
            p.base().first += n;
            p.base().second += n;
        }
    };

} // namespace detail

template < typename IterA, typename IterB > struct shadow_iterator_generator
{

    // To use the iterator_adaptor we can't derive from
    // random_access_iterator because we don't have a real reference.
    // However, we want the STL algorithms to treat the shadow
    // iterator like a random access iterator.
    struct shadow_iterator_tag : public std::input_iterator_tag
    {
        operator std::random_access_iterator_tag()
        {
            return std::random_access_iterator_tag();
        };
    };
    typedef typename std::iterator_traits< IterA >::value_type Aval;
    typedef typename std::iterator_traits< IterB >::value_type Bval;
    typedef typename std::iterator_traits< IterA >::reference Aref;
    typedef typename std::iterator_traits< IterB >::reference Bref;
    typedef typename std::iterator_traits< IterA >::difference_type D;
    typedef detail::shadow_proxy< Aval, Bval, Aval > V;
    typedef detail::shadow_proxy< Aref, Bref, Aval > R;
    typedef iterator_adaptor< std::pair< IterA, IterB >,
        detail::shadow_iterator_policies, V, R, V*, shadow_iterator_tag, D >
        type;
};

// short cut for creating a shadow iterator
template < class IterA, class IterB >
inline typename shadow_iterator_generator< IterA, IterB >::type
make_shadow_iter(IterA a, IterB b)
{
    typedef typename shadow_iterator_generator< IterA, IterB >::type Iter;
    return Iter(std::make_pair(a, b));
}

template < class Cmp > struct shadow_cmp
{
    inline shadow_cmp(const Cmp& c) : cmp(c) {}
    template < class ShadowProxy1, class ShadowProxy2 >
    inline bool operator()(const ShadowProxy1& x, const ShadowProxy2& y) const
    {
        return cmp(x.a, y.a);
    }
    Cmp cmp;
};

} // namespace boost

namespace std
{
template < class A1, class B1, class D1, class A2, class B2, class D2 >
void swap(boost::detail::shadow_proxy< A1&, B1&, D1 > x,
    boost::detail::shadow_proxy< A2&, B2&, D2 > y)
{
    std::swap(x.a, y.a);
    std::swap(x.b, y.b);
}
}

#endif // BOOST_SHADOW_ITERATOR_HPP
