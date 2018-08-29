//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <immer/detail/arrays/node.hpp>

namespace immer {
namespace detail {
namespace arrays {

template <typename T, typename MemoryPolicy>
struct no_capacity
{
    using node_t      = node<T, MemoryPolicy>;
    using edit_t      = typename MemoryPolicy::transience_t::edit;
    using size_t      = std::size_t;

    node_t* ptr;
    size_t  size;

    static const no_capacity empty;

    no_capacity(node_t* p, size_t s)
        : ptr{p}, size{s}
    {}

    no_capacity(const no_capacity& other)
        : no_capacity{other.ptr, other.size}
    {
        inc();
    }

    no_capacity(no_capacity&& other)
        : no_capacity{empty}
    {
        swap(*this, other);
    }

    no_capacity& operator=(const no_capacity& other)
    {
        auto next = other;
        swap(*this, next);
        return *this;
    }

    no_capacity& operator=(no_capacity&& other)
    {
        swap(*this, other);
        return *this;
    }

    friend void swap(no_capacity& x, no_capacity& y)
    {
        using std::swap;
        swap(x.ptr, y.ptr);
        swap(x.size, y.size);
    }

    ~no_capacity()
    {
        dec();
    }

    void inc()
    {
        using immer::detail::get;
        ptr->refs().inc();
    }

    void dec()
    {
        using immer::detail::get;
        if (ptr->refs().dec())
            node_t::delete_n(ptr, size, size);
    }

    T* data() { return ptr->data(); }
    const T* data() const { return ptr->data(); }

    template <typename Iter>
    static no_capacity from_range(Iter first, Iter last)
    {
        auto count = static_cast<size_t>(std::distance(first, last));
        return {
            node_t::copy_n(count, first, last),
            count,
        };
    }

    static no_capacity from_fill(size_t n, T v)
    {
        return { node_t::fill_n(n, v), n };
    }

    template <typename U>
    static no_capacity from_initializer_list(std::initializer_list<U> values)
    {
        using namespace std;
        return from_range(begin(values), end(values));
    }

    template <typename Fn>
    void for_each_chunk(Fn&& fn) const
    {
        std::forward<Fn>(fn)(data(), data() + size);
    }

    template <typename Fn>
    bool for_each_chunk_p(Fn&& fn) const
    {
        return std::forward<Fn>(fn)(data(), data() + size);
    }

    const T& get(std::size_t index) const
    {
        return data()[index];
    }

    const T& get_check(std::size_t index) const
    {
        if (index >= size)
            throw std::out_of_range{"out of range"};
        return data()[index];
    }

    bool equals(const no_capacity& other) const
    {
        return ptr == other.ptr ||
            (size == other.size &&
             std::equal(data(), data() + size, other.data()));
    }

    no_capacity push_back(T value) const
    {
        auto p = node_t::copy_n(size + 1, ptr, size);
        try {
            new (p->data() + size) T{std::move(value)};
            return { p, size + 1 };
        } catch (...) {
            node_t::delete_n(p, size, size + 1);
            throw;
        }
    }

    no_capacity assoc(std::size_t idx, T value) const
    {
        auto p = node_t::copy_n(size, ptr, size);
        try {
            p->data()[idx] = std::move(value);
            return { p, size };
        } catch (...) {
            node_t::delete_n(p, size, size);
            throw;
        }
    }

    template <typename Fn>
    no_capacity update(std::size_t idx, Fn&& op) const
    {
        auto p = node_t::copy_n(size, ptr, size);
        try {
            auto& elem = p->data()[idx];
            elem = std::forward<Fn>(op)(std::move(elem));
            return { p, size };
        } catch (...) {
            node_t::delete_n(p, size, size);
            throw;
        }
    }

    no_capacity take(std::size_t sz) const
    {
        auto p = node_t::copy_n(sz, ptr, sz);
        return { p, sz };
    }
};

template <typename T, typename MP>
const no_capacity<T, MP> no_capacity<T, MP>::empty = {
    node_t::make_n(0),
    0,
};

} // namespace arrays
} // namespace detail
} // namespace immer
