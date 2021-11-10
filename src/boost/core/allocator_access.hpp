/*
Copyright 2020-2021 Glen Joseph Fernandes
(glenjofe@gmail.com)

Distributed under the Boost Software License, Version 1.0.
(http://www.boost.org/LICENSE_1_0.txt)
*/
#ifndef BOOST_CORE_ALLOCATOR_ACCESS_HPP
#define BOOST_CORE_ALLOCATOR_ACCESS_HPP

#include <boost/config.hpp>
#if !defined(BOOST_NO_CXX11_ALLOCATOR)
#include <boost/core/pointer_traits.hpp>
#include <limits>
#include <type_traits>
#endif
#include <new>
#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
#include <utility>
#endif

#if defined(_LIBCPP_SUPPRESS_DEPRECATED_PUSH)
_LIBCPP_SUPPRESS_DEPRECATED_PUSH
#endif
#if defined(_STL_DISABLE_DEPRECATED_WARNING)
_STL_DISABLE_DEPRECATED_WARNING
#endif
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable:4996)
#endif

namespace boost {

template<class A>
struct allocator_value_type {
    typedef typename A::value_type type;
};

#if defined(BOOST_NO_CXX11_ALLOCATOR)
template<class A>
struct allocator_pointer {
    typedef typename A::pointer type;
};
#else
template<class A, class = void>
struct allocator_pointer {
    typedef typename A::value_type* type;
};

namespace detail {

template<class>
struct alloc_void {
    typedef void type;
};

} /* detail */

template<class A>
struct allocator_pointer<A,
    typename detail::alloc_void<typename A::pointer>::type> {
    typedef typename A::pointer type;
};
#endif

#if defined(BOOST_NO_CXX11_ALLOCATOR)
template<class A>
struct allocator_const_pointer {
    typedef typename A::const_pointer type;
};
#else
template<class A, class = void>
struct allocator_const_pointer {
    typedef typename pointer_traits<typename
        allocator_pointer<A>::type>::template
            rebind_to<const typename A::value_type>::type type;
};

template<class A>
struct allocator_const_pointer<A,
    typename detail::alloc_void<typename A::const_pointer>::type> {
    typedef typename A::const_pointer type;
};
#endif

#if defined(BOOST_NO_CXX11_ALLOCATOR)
template<class A>
struct allocator_void_pointer {
    typedef typename A::template rebind<void>::other::pointer type;
};
#else
template<class A, class = void>
struct allocator_void_pointer {
     typedef typename pointer_traits<typename
        allocator_pointer<A>::type>::template
            rebind_to<void>::type type;
};

template<class A>
struct allocator_void_pointer<A,
    typename detail::alloc_void<typename A::void_pointer>::type> {
    typedef typename A::void_pointer type;
};
#endif

#if defined(BOOST_NO_CXX11_ALLOCATOR)
template<class A>
struct allocator_const_void_pointer {
    typedef typename A::template rebind<void>::other::const_pointer type;
};
#else
template<class A, class = void>
struct allocator_const_void_pointer {
     typedef typename pointer_traits<typename
        allocator_pointer<A>::type>::template
            rebind_to<const void>::type type;
};

template<class A>
struct allocator_const_void_pointer<A,
    typename detail::alloc_void<typename A::const_void_pointer>::type> {
    typedef typename A::const_void_pointer type;
};
#endif

#if defined(BOOST_NO_CXX11_ALLOCATOR)
template<class A>
struct allocator_difference_type {
    typedef typename A::difference_type type;
};
#else
template<class A, class = void>
struct allocator_difference_type {
    typedef typename pointer_traits<typename
        allocator_pointer<A>::type>::difference_type type;
};

template<class A>
struct allocator_difference_type<A,
    typename detail::alloc_void<typename A::difference_type>::type> {
    typedef typename A::difference_type type;
};
#endif

#if defined(BOOST_NO_CXX11_ALLOCATOR)
template<class A>
struct allocator_size_type {
    typedef typename A::size_type type;
};
#else
template<class A, class = void>
struct allocator_size_type {
    typedef typename std::make_unsigned<typename
        allocator_difference_type<A>::type>::type type;
};

template<class A>
struct allocator_size_type<A,
    typename detail::alloc_void<typename A::size_type>::type> {
    typedef typename A::size_type type;
};
#endif

#if defined(BOOST_NO_CXX11_ALLOCATOR)
namespace detail {

struct alloc_false {
    BOOST_STATIC_CONSTEXPR bool value = false;
};

} /* detail */

template<class A>
struct allocator_propagate_on_container_copy_assignment {
    typedef detail::alloc_false type;
};
#else
template<class A, class = void>
struct allocator_propagate_on_container_copy_assignment {
    typedef std::false_type type;
};

template<class A>
struct allocator_propagate_on_container_copy_assignment<A,
    typename detail::alloc_void<typename
        A::propagate_on_container_copy_assignment>::type> {
    typedef typename A::propagate_on_container_copy_assignment type;
};
#endif

#if defined(BOOST_NO_CXX11_ALLOCATOR)
template<class A>
struct allocator_propagate_on_container_move_assignment {
    typedef detail::alloc_false type;
};
#else
template<class A, class = void>
struct allocator_propagate_on_container_move_assignment {
    typedef std::false_type type;
};

template<class A>
struct allocator_propagate_on_container_move_assignment<A,
    typename detail::alloc_void<typename
        A::propagate_on_container_move_assignment>::type> {
    typedef typename A::propagate_on_container_move_assignment type;
};
#endif

#if defined(BOOST_NO_CXX11_ALLOCATOR)
template<class A>
struct allocator_propagate_on_container_swap {
    typedef detail::alloc_false type;
};
#else
template<class A, class = void>
struct allocator_propagate_on_container_swap {
    typedef std::false_type type;
};

template<class A>
struct allocator_propagate_on_container_swap<A,
    typename detail::alloc_void<typename
        A::propagate_on_container_swap>::type> {
    typedef typename A::propagate_on_container_swap type;
};
#endif

#if defined(BOOST_NO_CXX11_ALLOCATOR)
template<class A>
struct allocator_is_always_equal {
    typedef detail::alloc_false type;
};
#else
template<class A, class = void>
struct allocator_is_always_equal {
    typedef typename std::is_empty<A>::type type;
};

template<class A>
struct allocator_is_always_equal<A,
    typename detail::alloc_void<typename A::is_always_equal>::type> {
    typedef typename A::is_always_equal type;
};
#endif

#if defined(BOOST_NO_CXX11_ALLOCATOR)
template<class A, class T>
struct allocator_rebind {
    typedef typename A::template rebind<T>::other type;
};
#else
namespace detail {

template<class, class>
struct alloc_to { };

template<template<class, class...> class A, class T, class U, class... V>
struct alloc_to<A<U, V...>, T> {
    typedef A<T, V...> type;
};

} /* detail */

template<class A, class T, class = void>
struct allocator_rebind {
    typedef typename detail::alloc_to<A, T>::type type;
};

template<class A, class T>
struct allocator_rebind<A, T,
    typename detail::alloc_void<typename A::template rebind<T>::other>::type> {
    typedef typename A::template rebind<T>::other type;
};
#endif

template<class A>
inline typename allocator_pointer<A>::type
allocator_allocate(A& a, typename allocator_size_type<A>::type n)
{
    return a.allocate(n);
}

template<class A>
inline void
allocator_deallocate(A& a, typename allocator_pointer<A>::type p,
    typename allocator_size_type<A>::type n)
{
    a.deallocate(p, n);
}

#if defined(BOOST_NO_CXX11_ALLOCATOR)
template<class A>
inline typename allocator_pointer<A>::type
allocator_allocate(A& a, typename allocator_size_type<A>::type n,
    typename allocator_const_void_pointer<A>::type h)
{
    return a.allocate(n, h);
}
#else
namespace detail {

struct alloc_none { };

template<class A>
class alloc_has_allocate {
    template<class O>
    static auto check(int) -> decltype(std::declval<O&>().allocate(
        std::declval<typename allocator_size_type<A>::type>(),
        std::declval<typename allocator_const_void_pointer<A>::type>()));

    template<class>
    static alloc_none check(long);

public:
    BOOST_STATIC_CONSTEXPR bool value =
        !std::is_same<decltype(check<A>(0)), alloc_none>::value;
};

} /* detail */

template<class A>
inline typename std::enable_if<detail::alloc_has_allocate<A>::value,
    typename allocator_pointer<A>::type>::type
allocator_allocate(A& a, typename allocator_size_type<A>::type n,
    typename allocator_const_void_pointer<A>::type h)
{
    return a.allocate(n, h);
}

template<class A>
inline typename std::enable_if<!detail::alloc_has_allocate<A>::value,
    typename allocator_pointer<A>::type>::type
allocator_allocate(A& a, typename allocator_size_type<A>::type n,
    typename allocator_const_void_pointer<A>::type)
{
    return a.allocate(n);
}
#endif

#if defined(BOOST_NO_CXX11_ALLOCATOR)
template<class A, class T>
inline void
allocator_construct(A&, T* p)
{
    ::new((void*)p) T();
}

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
#if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
template<class A, class T, class V, class... Args>
inline void
allocator_construct(A&, T* p, V&& v, Args&&... args)
{
    ::new((void*)p) T(std::forward<V>(v), std::forward<Args>(args)...);
}
#else
template<class A, class T, class V>
inline void
allocator_construct(A&, T* p, V&& v)
{
    ::new((void*)p) T(std::forward<V>(v));
}
#endif
#else
template<class A, class T, class V>
inline void
allocator_construct(A&, T* p, const V& v)
{
    ::new((void*)p) T(v);
}

template<class A, class T, class V>
inline void
allocator_construct(A&, T* p, V& v)
{
    ::new((void*)p) T(v);
}
#endif
#else
namespace detail {

template<class A, class T, class... Args>
class alloc_has_construct {
    template<class O>
    static auto check(int)
    -> decltype(std::declval<O&>().construct(std::declval<T*>(),
        std::declval<Args&&>()...));

    template<class>
    static alloc_none check(long);

public:
    BOOST_STATIC_CONSTEXPR bool value =
        !std::is_same<decltype(check<A>(0)), alloc_none>::value;
};

} /* detail */

template<class A, class T, class... Args>
inline typename std::enable_if<detail::alloc_has_construct<A, T,
    Args...>::value>::type
allocator_construct(A& a, T* p, Args&&... args)
{
    a.construct(p, std::forward<Args>(args)...);
}

template<class A, class T, class... Args>
inline typename std::enable_if<!detail::alloc_has_construct<A, T,
    Args...>::value>::type
allocator_construct(A&, T* p, Args&&... args)
{
    ::new((void*)p) T(std::forward<Args>(args)...);
}
#endif

#if defined(BOOST_NO_CXX11_ALLOCATOR)
template<class A, class T>
inline void
allocator_destroy(A&, T* p)
{
    p->~T();
    (void)p;
}
#else
namespace detail {

template<class A, class T>
class alloc_has_destroy {
    template<class O>
    static auto check(int)
    -> decltype(std::declval<O&>().destroy(std::declval<T*>()));

    template<class>
    static alloc_none check(long);

public:
    BOOST_STATIC_CONSTEXPR bool value =
        !std::is_same<decltype(check<A>(0)), alloc_none>::value;
};

} /* detail */

template<class A, class T>
inline typename std::enable_if<detail::alloc_has_destroy<A, T>::value>::type
allocator_destroy(A& a, T* p)
{
    a.destroy(p);
}

template<class A, class T>
inline typename std::enable_if<!detail::alloc_has_destroy<A, T>::value>::type
allocator_destroy(A&, T* p)
{
    p->~T();
    (void)p;
}
#endif

#if defined(BOOST_NO_CXX11_ALLOCATOR)
template<class A>
inline typename allocator_size_type<A>::type
allocator_max_size(const A& a)
{
    return a.max_size();
}
#else
namespace detail {

template<class A>
class alloc_has_max_size {
    template<class O>
    static auto check(int) -> decltype(std::declval<O&>().max_size());

    template<class>
    static alloc_none check(long);

public:
    BOOST_STATIC_CONSTEXPR bool value =
        !std::is_same<decltype(check<A>(0)), alloc_none>::value;
};

} /* detail */

template<class A>
inline typename std::enable_if<detail::alloc_has_max_size<A>::value,
    typename allocator_size_type<A>::type>::type
allocator_max_size(const A& a)
{
    return a.max_size();
}

template<class A>
inline typename std::enable_if<!detail::alloc_has_max_size<A>::value,
    typename allocator_size_type<A>::type>::type
allocator_max_size(const A&)
{
    return (std::numeric_limits<typename
        allocator_size_type<A>::type>::max)() / sizeof(typename A::value_type);
}
#endif

#if defined(BOOST_NO_CXX11_ALLOCATOR)
template<class A>
inline A
allocator_select_on_container_copy_construction(const A& a)
{
    return a;
}
#else
namespace detail {

template<class A>
class alloc_has_soccc {
    template<class O>
    static auto check(int)
    -> decltype(std::declval<O&>().select_on_container_copy_construction());

    template<class>
    static alloc_none check(long);

public:
    BOOST_STATIC_CONSTEXPR bool value =
        !std::is_same<decltype(check<A>(0)), alloc_none>::value;
};

} /* detail */

template<class A>
inline typename std::enable_if<detail::alloc_has_soccc<A>::value, A>::type
allocator_select_on_container_copy_construction(const A& a)
{
    return a.select_on_container_copy_construction();
}

template<class A>
inline typename std::enable_if<!detail::alloc_has_soccc<A>::value, A>::type
allocator_select_on_container_copy_construction(const A& a)
{
    return a;
}
#endif

#if !defined(BOOST_NO_CXX11_TEMPLATE_ALIASES)
template<class A>
using allocator_value_type_t = typename allocator_value_type<A>::type;

template<class A>
using allocator_pointer_t = typename allocator_pointer<A>::type;

template<class A>
using allocator_const_pointer_t = typename allocator_const_pointer<A>::type;

template<class A>
using allocator_void_pointer_t = typename allocator_void_pointer<A>::type;

template<class A>
using allocator_const_void_pointer_t =
    typename allocator_const_void_pointer<A>::type;

template<class A>
using allocator_difference_type_t =
    typename allocator_difference_type<A>::type;

template<class A>
using allocator_size_type_t = typename allocator_size_type<A>::type;

template<class A>
using allocator_propagate_on_container_copy_assignment_t =
    typename allocator_propagate_on_container_copy_assignment<A>::type;

template<class A>
using allocator_propagate_on_container_move_assignment_t =
    typename allocator_propagate_on_container_move_assignment<A>::type;

template<class A>
using allocator_propagate_on_container_swap_t =
    typename allocator_propagate_on_container_swap<A>::type;

template<class A>
using allocator_is_always_equal_t =
    typename allocator_is_always_equal<A>::type;

template<class A, class T>
using allocator_rebind_t = typename allocator_rebind<A, T>::type;
#endif

} /* boost */

#if defined(_LIBCPP_SUPPRESS_DEPRECATED_POP)
_LIBCPP_SUPPRESS_DEPRECATED_POP
#endif
#if defined(_STL_RESTORE_DEPRECATED_WARNING)
_STL_RESTORE_DEPRECATED_WARNING
#endif
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#endif
