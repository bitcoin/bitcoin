//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <type_traits>

#if __GNUC__ == 7 || __GNUC_MINOR__ == 1
#define IMMER_BROKEN_STANDARD_LAYOUT_DETECTION 1
#define immer_offsetof(st, m) ((std::size_t) &(((st*)0)->m))
#else
#define IMMER_BROKEN_STANDARD_LAYOUT_DETECTION 0
#define immer_offsetof offsetof
#endif

namespace immer {
namespace detail {

//
// Metafunction that returns a standard layout struct that combines
// all the standard layout types in `Ts...`, while making sure that
// empty base optimizations are used.
//
// To query a part of the type do `get<my_part>(x)`;
//
// This is useful when putting together a type that merges various
// types coming from different policies.  Some of them might be empty,
// so we shall enable empty base optimizations.  But if we just
// inherit from all of them, we would break the "standard layout"
// rules, preventing us from using `offseof(...)`.  So metafunction
// will generate the type by sometimes inheriting, sometimes adding as
// member.
//
// Note that the types are added to the combined type from right to
// left!
//
template <typename... Ts>
struct combine_standard_layout;

template <typename... Ts>
using combine_standard_layout_t = typename combine_standard_layout<Ts...>::type;

namespace csl {

template <typename T>
struct type_t {};

template <typename U, typename T>
U& get(T& x);

template <typename U, typename T>
const U& get(const T& x);

template <typename T, typename Next=void>
struct inherit
{
    struct type : T, Next
    {
        using Next::get_;

        template <typename U>
        friend decltype(auto) get(type& x) { return x.get_(type_t<U>{}); }
        template <typename U>
        friend decltype(auto) get(const type& x) { return x.get_(type_t<U>{}); }

        T& get_(type_t<T>) { return *this; }
        const T& get_(type_t<T>) const { return *this; }
    };
};

template <typename T>
struct inherit<T, void>
{
    struct type : T
    {
        template <typename U>
        friend decltype(auto) get(type& x) { return x.get_(type_t<U>{}); }
        template <typename U>
        friend decltype(auto) get(const type& x) { return x.get_(type_t<U>{}); }

        T& get_(type_t<T>) { return *this; }
        const T& get_(type_t<T>) const { return *this; }
    };
};

template <typename T, typename Next=void>
struct member
{
    struct type : Next
    {
        T d;

        using Next::get_;

        template <typename U>
        friend decltype(auto) get(type& x) { return x.get_(type_t<U>{}); }
        template <typename U>
        friend decltype(auto) get(const type& x) { return x.get_(type_t<U>{}); }

        T& get_(type_t<T>) { return d; }
        const T& get_(type_t<T>) const { return d; }
    };
};

template <typename T>
struct member<T, void>
{
    struct type
    {
        T d;

        template <typename U>
        friend decltype(auto) get(type& x) { return x.get_(type_t<U>{}); }
        template <typename U>
        friend decltype(auto) get(const type& x) { return x.get_(type_t<U>{}); }

        T& get_(type_t<T>) { return d; }
        const T& get_(type_t<T>) const { return d; }
    };
};

template <typename T, typename Next>
struct member_two
{
    struct type
    {
        Next n;
        T d;

        template <typename U>
        friend decltype(auto) get(type& x) { return x.get_(type_t<U>{}); }
        template <typename U>
        friend decltype(auto) get(const type& x) { return x.get_(type_t<U>{}); }

        T& get_(type_t<T>) { return d; }
        const T& get_(type_t<T>) const { return d; }

        template <typename U>
        auto get_(type_t<U> t) -> decltype(auto) { return n.get_(t); }
        template <typename U>
        auto get_(type_t<U> t) const -> decltype(auto) { return n.get_(t); }
    };
};

template <typename... Ts>
struct combine_standard_layout_aux;

template <typename T>
struct combine_standard_layout_aux<T>
{
    static_assert(std::is_standard_layout<T>::value, "");

    using type = typename std::conditional_t<
        std::is_empty<T>::value,
        csl::inherit<T>,
        csl::member<T>>::type;
};

template <typename T, typename... Ts>
struct combine_standard_layout_aux<T, Ts...>
{
    static_assert(std::is_standard_layout<T>::value, "");

    using this_t = T;
    using next_t = typename combine_standard_layout_aux<Ts...>::type;

    static constexpr auto empty_this = std::is_empty<this_t>::value;
    static constexpr auto empty_next = std::is_empty<next_t>::value;

    using type = typename std::conditional_t<
        empty_this, inherit<this_t, next_t>,
        std::conditional_t<
            empty_next, member<this_t, next_t>,
            member_two<this_t, next_t>>>::type;
};

} // namespace csl

using csl::get;

template <typename... Ts>
struct combine_standard_layout
{
    using type = typename csl::combine_standard_layout_aux<Ts...>::type;
#if !IMMER_BROKEN_STANDARD_LAYOUT_DETECTION
    static_assert(std::is_standard_layout<type>::value, "");
#endif
};

} // namespace detail
} // namespace immer
