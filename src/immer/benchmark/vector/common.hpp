//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <utility>
#include <cstddef>
#include <limits>

#include "benchmark/config.hpp"

#if IMMER_BENCHMARK_LIBRRB
extern "C" {
#define restrict __restrict__
#include <rrb.h>
#undef restrict
}
#include <immer/heap/gc_heap.hpp>
#endif

namespace immer {
template <typename T, typename MP> class array;
} // namespace immer

namespace {

auto make_generator(std::size_t runs)
{
    assert(runs > 0);
    auto engine = std::default_random_engine{42};
    auto dist = std::uniform_int_distribution<std::size_t>{0, runs-1};
    auto r = std::vector<std::size_t>(runs);
    std::generate_n(r.begin(), runs, std::bind(dist, engine));
    return r;
}

struct push_back_fn
{
    template <typename T, typename U>
    auto operator() (T&& v, U&& x)
    { return std::forward<T>(v).push_back(std::forward<U>(x)); }
};

struct push_front_fn
{
    template <typename T, typename U>
    auto operator() (T&& v, U&& x)
    { return std::forward<T>(v).push_front(std::forward<U>(x)); }
};

struct set_fn
{
    template <typename T, typename I, typename U>
    decltype(auto) operator() (T&& v, I i, U&& x)
    { return std::forward<T>(v).set(i, std::forward<U>(x)); }
};

struct store_fn
{
    template <typename T, typename I, typename U>
    decltype(auto) operator() (T&& v, I i, U&& x)
    { return std::forward<T>(v).store(i, std::forward<U>(x)); }
};

template <typename T>
struct get_limit : std::integral_constant<
    std::size_t, std::numeric_limits<std::size_t>::max()> {};

template <typename T, typename MP>
struct get_limit<immer::array<T, MP>> : std::integral_constant<
    std::size_t, 10000> {};

auto make_librrb_vector(std::size_t n)
{
    auto v = rrb_create();
    for (auto i = 0u; i < n; ++i) {
        v = rrb_push(v, reinterpret_cast<void*>(i));
    }
    return v;
}

auto make_librrb_vector_f(std::size_t n)
{
    auto v = rrb_create();
    for (auto i = 0u; i < n; ++i) {
        auto f = rrb_push(rrb_create(),
                          reinterpret_cast<void*>(i));
        v = rrb_concat(f, v);
    }
    return v;
}


// copied from:
// https://github.com/ivmai/bdwgc/blob/master/include/gc_allocator.h

template <class GC_tp>
struct GC_type_traits
{
  std::false_type GC_is_ptr_free;
};

# define GC_DECLARE_PTRFREE(T)                  \
    template<> struct GC_type_traits<T> {       \
        std::true_type GC_is_ptr_free;          \
    }

GC_DECLARE_PTRFREE(char);
GC_DECLARE_PTRFREE(signed char);
GC_DECLARE_PTRFREE(unsigned char);
GC_DECLARE_PTRFREE(signed short);
GC_DECLARE_PTRFREE(unsigned short);
GC_DECLARE_PTRFREE(signed int);
GC_DECLARE_PTRFREE(unsigned int);
GC_DECLARE_PTRFREE(signed long);
GC_DECLARE_PTRFREE(unsigned long);
GC_DECLARE_PTRFREE(float);
GC_DECLARE_PTRFREE(double);
GC_DECLARE_PTRFREE(long double);

template <class IsPtrFree>
inline void* GC_selective_alloc(size_t n, IsPtrFree, bool ignore_off_page)
{
    return ignore_off_page
        ? GC_MALLOC_IGNORE_OFF_PAGE(n)
        : GC_MALLOC(n);
}

template <>
inline void* GC_selective_alloc<std::true_type>(size_t n,
                                                std::true_type,
                                                bool ignore_off_page)
{
    return ignore_off_page
        ? GC_MALLOC_ATOMIC_IGNORE_OFF_PAGE(n)
        : GC_MALLOC_ATOMIC(n);
}

template <class T>
class gc_allocator
{
public:
    typedef size_t       size_type;
    typedef ptrdiff_t    difference_type;
    typedef T*       pointer;
    typedef const T* const_pointer;
    typedef T&       reference;
    typedef const T& const_reference;
    typedef T        value_type;

    template <class T1> struct rebind {
        typedef gc_allocator<T1> other;
    };

    gc_allocator()  {}
    gc_allocator(const gc_allocator&) throw() {}
    template <class T1>
    explicit gc_allocator(const gc_allocator<T1>&) throw() {}
    ~gc_allocator() throw() {}

    pointer address(reference GC_x) const { return &GC_x; }
    const_pointer address(const_reference GC_x) const { return &GC_x; }

    // GC_n is permitted to be 0.  The C++ standard says nothing about what
    // the return value is when GC_n == 0.
    T* allocate(size_type GC_n, const void* = 0)
    {
        GC_type_traits<T> traits;
        return static_cast<T *>
            (GC_selective_alloc(GC_n * sizeof(T),
                                traits.GC_is_ptr_free, false));
    }

    // p is not permitted to be a null pointer.
    void deallocate(pointer p, size_type /* GC_n */)
    { GC_FREE(p); }

    size_type max_size() const throw()
    { return size_t(-1) / sizeof(T); }

    void construct(pointer p, const T& __val) { new(p) T(__val); }
    void destroy(pointer p) { p->~T(); }
};

template<>
class gc_allocator<void>
{
    typedef size_t      size_type;
    typedef ptrdiff_t   difference_type;
    typedef void*       pointer;
    typedef const void* const_pointer;
    typedef void        value_type;

    template <class T1> struct rebind {
        typedef gc_allocator<T1> other;
    };
};

template <class T1, class T2>
inline bool operator==(const gc_allocator<T1>&, const gc_allocator<T2>&)
{ return true; }

template <class T1, class T2>
inline bool operator!=(const gc_allocator<T1>&, const gc_allocator<T2>&)
{ return false; }

} // anonymous namespace
