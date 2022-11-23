//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <array>
#include <cstddef>
#include <utility>

#include <immer/detail/hamts/bits.hpp>
#include <immer/detail/rbts/bits.hpp>

namespace {

template <typename Iterator>
struct rotator
{
    using value_type = typename std::iterator_traits<Iterator>::value_type;

    Iterator first;
    Iterator last;
    Iterator curr;

    void init(Iterator f, Iterator l)
    {
        first = f;
        last  = l;
        curr  = f;
    }

    value_type next()
    {
        if (curr == last)
            curr = first;
        return *curr++;
    }
};

template <typename Range>
struct range_rotator : rotator<typename Range::iterator>
{
    using base_t = rotator<typename Range::iterator>;

    Range range;

    range_rotator(Range r)
        : range{std::move(r)}
    {
        base_t::init(range.begin(), range.end());
    }
};

template <typename Range>
auto make_rotator(Range r) -> range_rotator<Range>
{
    return {r};
}

inline auto magic_rotator()
{
    return make_rotator(std::array<unsigned, 15>{
        {7, 11, 2, 3, 5, 7, 11, 13, 17, 19, 23, 5, 29, 31, 37}});
}

struct dada_error
{};

struct dadaism;
static dadaism* g_dadaism = nullptr;

struct dadaism
{
    using rotator_t = decltype(magic_rotator());

    rotator_t magic = magic_rotator();

    unsigned step       = magic.next();
    unsigned count      = 0;
    unsigned happenings = 0;
    unsigned last       = 0;
    bool toggle         = false;

    struct scope
    {
        bool moved     = false;
        dadaism* save_ = g_dadaism;
        scope(scope&& s)
        {
            save_   = s.save_;
            s.moved = true;
        }
        scope(dadaism* self) { g_dadaism = self; }
        ~scope()
        {
            if (!moved)
                g_dadaism = save_;
        }
    };

    static scope disable() { return {nullptr}; }

    scope next()
    {
        toggle = last == happenings;
        last   = happenings;
        step   = toggle ? step : magic.next();
        return {this};
    }

    void dada()
    {
        if (toggle && ++count % step == 0) {
            ++happenings;
            throw dada_error{};
        }
    }
};

inline void dada()
{
    if (g_dadaism)
        g_dadaism->dada();
}

inline bool soft_dada()
{
    try {
        dada();
        return false;
    } catch (dada_error) {
        return true;
    }
}

template <typename Heap>
struct dadaist_heap : Heap
{
    template <typename... Tags>
    static auto allocate(std::size_t s, Tags... tags)
    {
        dada();
        return Heap::allocate(s, tags...);
    }
};

template <typename MP>
struct dadaist_memory_policy : MP
{
    struct heap
    {
        using type = dadaist_heap<typename MP::heap::type>;

        template <std::size_t Size>
        struct optimized
        {
            using base = typename MP::heap::template optimized<Size>::type;
            using type = dadaist_heap<base>;
        };
    };
};

struct tristan_tzara
{
    tristan_tzara() { dada(); }
    tristan_tzara(const tristan_tzara&) { dada(); }
    tristan_tzara(tristan_tzara&&) { dada(); }
    tristan_tzara& operator=(const tristan_tzara&)
    {
        dada();
        return *this;
    }
    tristan_tzara& operator=(tristan_tzara&&)
    {
        dada();
        return *this;
    }
};

template <typename T>
struct dadaist : tristan_tzara
{
    T value;

    dadaist()
        : value{}
    {}
    dadaist(T v)
        : value{std::move(v)}
    {}
    operator T() const { return value; }
};

template <typename T>
struct dadaist_wrapper;

using rbits_t = immer::detail::rbts::bits_t;
using hbits_t = immer::detail::rbts::bits_t;

template <template <class, class, rbits_t, rbits_t> class V,
          typename T,
          typename MP,
          rbits_t B,
          rbits_t BL>
struct dadaist_wrapper<V<T, MP, B, BL>>
{
    using type = V<dadaist<T>, dadaist_memory_policy<MP>, B, BL>;
};

template <template <class, class> class V, typename T, typename MP>
struct dadaist_wrapper<V<T, MP>>
{
    using type = V<dadaist<T>, dadaist_memory_policy<MP>>;
};

template <template <class, class, class, class, hbits_t> class V,
          typename T,
          typename E,
          typename H,
          typename MP,
          hbits_t B>
struct dadaist_wrapper<V<T, H, E, MP, B>>
{
    using type = V<dadaist<T>, H, E, dadaist_memory_policy<MP>, B>;
};

template <template <class, class, class, class, class, hbits_t> class V,
          typename K,
          typename T,
          typename E,
          typename H,
          typename MP,
          hbits_t B>
struct dadaist_wrapper<V<K, T, H, E, MP, B>>
{
    using type = V<K, dadaist<T>, H, E, dadaist_memory_policy<MP>, B>;
};

} // anonymous namespace
