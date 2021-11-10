/*=============================================================================
    Copyright (c) 2015 Paul Fultz II
    construct.h
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_HOF_GUARD_CONSTRUCT_H
#define BOOST_HOF_GUARD_CONSTRUCT_H

/// construct
/// =========
/// 
/// Description
/// -----------
/// 
/// The `construct` function returns a function object that will construct the
/// object when the called. A template can also be given, which it will deduce
/// the parameters to the template. The `construct_meta` can be used to
/// construct the object from a metafunction.
/// 
/// Synopsis
/// --------
/// 
///     // Construct by decaying each value
///     template<class T>
///     constexpr auto construct();
/// 
///     template<template<class...> class Template>
///     constexpr auto construct();
/// 
///     // Construct by deducing lvalues by reference and rvalue reference by reference
///     template<class T>
///     constexpr auto construct_forward();
/// 
///     template<template<class...> class Template>
///     constexpr auto construct_forward();
/// 
///     // Construct by deducing lvalues by reference and rvalues by value.
///     template<class T>
///     constexpr auto construct_basic();
/// 
///     template<template<class...> class Template>
///     constexpr auto construct_basic();
/// 
///     // Construct by deducing the object from a metafunction
///     template<class MetafunctionClass>
///     constexpr auto construct_meta();
/// 
///     template<template<class...> class MetafunctionTemplate>
///     constexpr auto construct_meta();
/// 
/// Semantics
/// ---------
/// 
///     assert(construct<T>()(xs...) == T(xs...));
///     assert(construct<Template>()(xs...) == Template<decltype(xs)...>(xs...));
///     assert(construct_meta<MetafunctionClass>()(xs...) == MetafunctionClass::apply<decltype(xs)...>(xs...));
///     assert(construct_meta<MetafunctionTemplate>()(xs...) == MetafunctionTemplate<decltype(xs)...>::type(xs...));
/// 
/// Requirements
/// ------------
/// 
/// MetafunctionClass must be a:
/// 
/// * [MetafunctionClass](MetafunctionClass)
/// 
/// MetafunctionTemplate<Ts...> must be a:
/// 
/// * [Metafunction](Metafunction)
/// 
/// T, Template<Ts..>, MetafunctionClass::apply<Ts...>, and
/// MetafunctionTemplate<Ts...>::type must be:
/// 
/// * MoveConstructible
/// 
/// Example
/// -------
/// 
///     #include <boost/hof.hpp>
///     #include <cassert>
///     #include <vector>
/// 
///     int main() {
///         auto v = boost::hof::construct<std::vector<int>>()(5, 5);
///         assert(v.size() == 5);
///     }
/// 

#include <boost/hof/detail/forward.hpp>
#include <boost/hof/detail/move.hpp>
#include <boost/hof/detail/delegate.hpp>
#include <boost/hof/detail/join.hpp>
#include <boost/hof/detail/remove_rvalue_reference.hpp>
#include <boost/hof/decay.hpp>

#include <initializer_list>

namespace boost { namespace hof { 

namespace detail {

template<class T, class=void>
struct construct_f
{
    typedef typename std::aligned_storage<sizeof(T)>::type storage;

    struct storage_holder
    {
        storage * s;
        storage_holder(storage* x) noexcept : s(x)
        {}

        T& data() noexcept
        {
            return *reinterpret_cast<T*>(s);
        }

        ~storage_holder() noexcept(noexcept(std::declval<T>().~T()))
        {
            this->data().~T();
        }
    };

    constexpr construct_f() noexcept
    {}
    template<class... Ts, BOOST_HOF_ENABLE_IF_CONSTRUCTIBLE(T, Ts...)>
    T operator()(Ts&&... xs) const BOOST_HOF_NOEXCEPT_CONSTRUCTIBLE(T, Ts&&...)
    {
        storage buffer{};
        new(&buffer) T(BOOST_HOF_FORWARD(Ts)(xs)...);
        storage_holder h(&buffer);
        return boost::hof::move(h.data());
    }

    template<class X, BOOST_HOF_ENABLE_IF_CONSTRUCTIBLE(T, std::initializer_list<X>&&)>
    T operator()(std::initializer_list<X>&& x) const BOOST_HOF_NOEXCEPT_CONSTRUCTIBLE(T, std::initializer_list<X>&&)
    {
        storage buffer{};
        new(&buffer) T(static_cast<std::initializer_list<X>&&>(x));
        storage_holder h(&buffer);
        return h.data();
    }

    template<class X, BOOST_HOF_ENABLE_IF_CONSTRUCTIBLE(T, std::initializer_list<X>&)>
    T operator()(std::initializer_list<X>& x) const BOOST_HOF_NOEXCEPT_CONSTRUCTIBLE(T, std::initializer_list<X>&)
    {
        storage buffer{};
        new(&buffer) T(x);
        storage_holder h(&buffer);
        return h.data();
    }

    template<class X, BOOST_HOF_ENABLE_IF_CONSTRUCTIBLE(T, const std::initializer_list<X>&)>
    T operator()(const std::initializer_list<X>& x) const BOOST_HOF_NOEXCEPT_CONSTRUCTIBLE(T, const std::initializer_list<X>&)
    {
        storage buffer{};
        new(&buffer) T(x);
        storage_holder h(&buffer);
        return h.data();
    }
};

template<class T>
struct construct_f<T, typename std::enable_if<BOOST_HOF_IS_LITERAL(T)>::type>
{
    constexpr construct_f() noexcept
    {}
    template<class... Ts, BOOST_HOF_ENABLE_IF_CONSTRUCTIBLE(T, Ts...)>
    constexpr T operator()(Ts&&... xs) const noexcept
    {
        return T(BOOST_HOF_FORWARD(Ts)(xs)...);
    }

    template<class X, BOOST_HOF_ENABLE_IF_CONSTRUCTIBLE(T, std::initializer_list<X>&&)>
    constexpr T operator()(std::initializer_list<X>&& x) const noexcept
    {
        return T(static_cast<std::initializer_list<X>&&>(x));
    }

    template<class X, BOOST_HOF_ENABLE_IF_CONSTRUCTIBLE(T, std::initializer_list<X>&)>
    constexpr T operator()(std::initializer_list<X>& x) const noexcept
    {
        return T(x);
    }

    template<class X, BOOST_HOF_ENABLE_IF_CONSTRUCTIBLE(T, const std::initializer_list<X>&)>
    constexpr T operator()(const std::initializer_list<X>& x) const noexcept
    {
        return T(x);
    }
};

template<template<class...> class Template, template<class...> class D>
struct construct_template_f
{
    constexpr construct_template_f() noexcept
    {}
    template<class... Ts, class Result=BOOST_HOF_JOIN(Template, typename D<Ts>::type...), 
        BOOST_HOF_ENABLE_IF_CONSTRUCTIBLE(Result, Ts...)>
    constexpr Result operator()(Ts&&... xs) const BOOST_HOF_NOEXCEPT_CONSTRUCTIBLE(Result, Ts&&...)
    {
        return construct_f<Result>()(BOOST_HOF_FORWARD(Ts)(xs)...);
    }
};

template<class MetafunctionClass>
struct construct_meta_f
{
    constexpr construct_meta_f() noexcept
    {}

    template<class... Ts>
    struct apply
    : MetafunctionClass::template apply<Ts...>
    {};

    template<class... Ts, 
        class Metafunction=BOOST_HOF_JOIN(apply, Ts...), 
        class Result=typename Metafunction::type, 
        BOOST_HOF_ENABLE_IF_CONSTRUCTIBLE(Result, Ts...)>
    constexpr Result operator()(Ts&&... xs) const BOOST_HOF_NOEXCEPT_CONSTRUCTIBLE(Result, Ts&&...)
    {
        return construct_f<Result>()(BOOST_HOF_FORWARD(Ts)(xs)...);
    }
};

template<template<class...> class MetafunctionTemplate>
struct construct_meta_template_f
{
    constexpr construct_meta_template_f() noexcept
    {}
    template<class... Ts, 
        class Metafunction=BOOST_HOF_JOIN(MetafunctionTemplate, Ts...), 
        class Result=typename Metafunction::type, 
        BOOST_HOF_ENABLE_IF_CONSTRUCTIBLE(Result, Ts...)>
    constexpr Result operator()(Ts&&... xs) const BOOST_HOF_NOEXCEPT_CONSTRUCTIBLE(Result, Ts&&...)
    {
        return construct_f<Result>()(BOOST_HOF_FORWARD(Ts)(xs)...);
    }
};


template<class T>
struct construct_id
{
    typedef T type;
};

}

template<class T>
constexpr detail::construct_f<T> construct() noexcept
{
    return {};
}
// These overloads are provide for consistency
template<class T>
constexpr detail::construct_f<T> construct_forward() noexcept
{
    return {};
}

template<class T>
constexpr detail::construct_f<T> construct_basic() noexcept
{
    return {};
}

template<template<class...> class Template>
constexpr detail::construct_template_f<Template, detail::decay_mf> construct() noexcept
{
    return {};
}

template<template<class...> class Template>
constexpr detail::construct_template_f<Template, detail::construct_id> construct_forward() noexcept
{
    return {};
}

template<template<class...> class Template>
constexpr detail::construct_template_f<Template, detail::remove_rvalue_reference> construct_basic() noexcept
{
    return {};
}

template<class T>
constexpr detail::construct_meta_f<T> construct_meta() noexcept
{
    return {};
}

template<template<class...> class Template>
constexpr detail::construct_meta_template_f<Template> construct_meta() noexcept
{
    return {};
}

}} // namespace boost::hof

#endif
