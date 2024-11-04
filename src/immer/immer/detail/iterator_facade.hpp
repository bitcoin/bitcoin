//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <cstddef>
#include <iterator>
#include <type_traits>

namespace immer {
namespace detail {

struct iterator_core_access
{
    template <typename T>
    static decltype(auto) dereference(T&& x)
    {
        return x.dereference();
    }

    template <typename T>
    static decltype(auto) increment(T&& x)
    {
        return x.increment();
    }

    template <typename T>
    static decltype(auto) decrement(T&& x)
    {
        return x.decrement();
    }

    template <typename T1, typename T2>
    static decltype(auto) equal(T1&& x1, T2&& x2)
    {
        return x1.equal(x2);
    }

    template <typename T, typename D>
    static decltype(auto) advance(T&& x, D d)
    {
        return x.advance(d);
    }

    template <typename T1, typename T2>
    static decltype(auto) distance_to(T1&& x1, T2&& x2)
    {
        return x1.distance_to(x2);
    }
};

/*!
 * Minimalistic reimplementation of boost::iterator_facade
 */
template <typename DerivedT,
          typename IteratorCategoryT,
          typename T,
          typename ReferenceT      = T&,
          typename DifferenceTypeT = std::ptrdiff_t,
          typename PointerT        = T*>
class iterator_facade
{
public:
    using iterator_category = IteratorCategoryT;
    using value_type        = T;
    using difference_type   = DifferenceTypeT;
    using pointer           = PointerT;
    using reference         = ReferenceT;

protected:
    using access_t = iterator_core_access;

    constexpr static auto is_random_access =
        std::is_base_of<std::random_access_iterator_tag,
                        IteratorCategoryT>::value;
    constexpr static auto is_bidirectional =
        std::is_base_of<std::bidirectional_iterator_tag,
                        IteratorCategoryT>::value;

    const DerivedT& derived() const
    {
        static_assert(std::is_base_of<iterator_facade, DerivedT>::value,
                      "must pass a derived thing");
        return *static_cast<const DerivedT*>(this);
    }
    DerivedT& derived()
    {
        static_assert(std::is_base_of<iterator_facade, DerivedT>::value,
                      "must pass a derived thing");
        return *static_cast<DerivedT*>(this);
    }

public:
    ReferenceT operator*() const { return access_t::dereference(derived()); }
    PointerT operator->() const { return &access_t::dereference(derived()); }
    ReferenceT operator[](DifferenceTypeT n) const
    {
        static_assert(is_random_access, "");
        return *(derived() + n);
    }

    friend bool operator==(const DerivedT& a, const DerivedT& b)
    {
        return access_t::equal(a, b);
    }
    friend bool operator!=(const DerivedT& a, const DerivedT& b)
    {
        return !access_t::equal(a, b);
    }

    DerivedT& operator++()
    {
        access_t::increment(derived());
        return derived();
    }
    DerivedT operator++(int)
    {
        auto tmp = derived();
        access_t::increment(derived());
        return tmp;
    }

    DerivedT& operator--()
    {
        static_assert(is_bidirectional || is_random_access, "");
        access_t::decrement(derived());
        return derived();
    }
    DerivedT operator--(int)
    {
        static_assert(is_bidirectional || is_random_access, "");
        auto tmp = derived();
        access_t::decrement(derived());
        return tmp;
    }

    DerivedT& operator+=(DifferenceTypeT n)
    {
        access_t::advance(derived(), n);
        return derived();
    }
    DerivedT& operator-=(DifferenceTypeT n)
    {
        access_t::advance(derived(), -n);
        return derived();
    }

    DerivedT operator+(DifferenceTypeT n) const
    {
        static_assert(is_random_access, "");
        auto tmp = derived();
        return tmp += n;
    }
    friend DerivedT operator+(DifferenceTypeT n, const DerivedT& i)
    {
        static_assert(is_random_access, "");
        return i + n;
    }
    DerivedT operator-(DifferenceTypeT n) const
    {
        static_assert(is_random_access, "");
        auto tmp = derived();
        return tmp -= n;
    }
    friend DifferenceTypeT operator-(const DerivedT& a, const DerivedT& b)
    {
        static_assert(is_random_access, "");
        return access_t::distance_to(b, a);
    }

    friend bool operator<(const DerivedT& a, const DerivedT& b)
    {
        static_assert(is_random_access, "");
        return access_t::distance_to(a, b) > 0;
    }
    friend bool operator<=(const DerivedT& a, const DerivedT& b)
    {
        static_assert(is_random_access, "");
        return access_t::distance_to(a, b) >= 0;
    }
    friend bool operator>(const DerivedT& a, const DerivedT& b)
    {
        static_assert(is_random_access, "");
        return access_t::distance_to(a, b) < 0;
    }
    friend bool operator>=(const DerivedT& a, const DerivedT& b)
    {
        static_assert(is_random_access, "");
        return access_t::distance_to(a, b) <= 0;
    }
};

} // namespace detail
} // namespace immer
