//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <immer/heap/heap_policy.hpp>
#include <immer/refcount/enable_intrusive_ptr.hpp>
#include <immer/refcount/refcount_policy.hpp>

#include <boost/intrusive_ptr.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/smart_ptr/intrusive_ref_counter.hpp>

#include <cassert>
#include <cstddef>
#include <limits>

namespace immer {
namespace detail {
namespace dvektor {

constexpr auto fast_log2(std::size_t x)
{
    return x == 0 ? 0 : sizeof(std::size_t) * 8 - 1 - __builtin_clzl(x);
}

template <int B, typename T = std::size_t>
constexpr T branches = T{1} << B;

template <int B, typename T = std::size_t>
constexpr T mask = branches<B, T> - 1;

template <int B, typename T = std::size_t>
constexpr auto
    max_depth = fast_log2(std::numeric_limits<std::size_t>::max()) / B;

template <typename T, int B, typename MP>
struct node;

template <typename T, int B, typename MP>
using node_ptr = boost::intrusive_ptr<node<T, B, MP>>;

template <typename T, int B>
using leaf_node = std::array<T, 1 << B>;

template <typename T, int B, typename MP>
using inner_node = std::array<node_ptr<T, B, MP>, 1 << B>;

template <typename T, int B, typename MP>
struct node
    : enable_intrusive_ptr<node<T, B, MP>, typename MP::refcount>
    , enable_optimized_heap_policy<node<T, B, MP>, typename MP::heap>
{
    using leaf_node_t  = leaf_node<T, B>;
    using inner_node_t = inner_node<T, B, MP>;

    enum
    {
        leaf_kind,
        inner_kind
    } kind;

    union data_t
    {
        leaf_node_t leaf;
        inner_node_t inner;
        data_t(leaf_node_t n)
            : leaf(std::move(n))
        {}
        data_t(inner_node_t n)
            : inner(std::move(n))
        {}
        ~data_t() {}
    } data;

    ~node()
    {
        switch (kind) {
        case leaf_kind:
            data.leaf.~leaf_node_t();
            break;
        case inner_kind:
            data.inner.~inner_node_t();
            break;
        }
    }

    node(leaf_node<T, B> n)
        : kind{leaf_kind}
        , data{std::move(n)}
    {}

    node(inner_node<T, B, MP> n)
        : kind{inner_kind}
        , data{std::move(n)}
    {}

    inner_node_t& inner() &
    {
        assert(kind == inner_kind);
        return data.inner;
    }
    const inner_node_t& inner() const&
    {
        assert(kind == inner_kind);
        return data.inner;
    }
    inner_node_t&& inner() &&
    {
        assert(kind == inner_kind);
        return std::move(data.inner);
    }

    leaf_node_t& leaf() &
    {
        assert(kind == leaf_kind);
        return data.leaf;
    }
    const leaf_node_t& leaf() const&
    {
        assert(kind == leaf_kind);
        return data.leaf;
    }
    leaf_node_t&& leaf() &&
    {
        assert(kind == leaf_kind);
        return std::move(data.leaf);
    }
};

template <typename T, int B, typename MP, typename... Ts>
auto make_node(Ts&&... xs) -> boost::intrusive_ptr<node<T, B, MP>>
{
    return new node<T, B, MP>(std::forward<Ts>(xs)...);
}

template <typename T, int B, typename MP>
struct ref
{
    using inner_t    = inner_node<T, B, MP>;
    using leaf_t     = leaf_node<T, B>;
    using node_t     = node<T, B, MP>;
    using node_ptr_t = node_ptr<T, B, MP>;

    unsigned depth;
    std::array<node_ptr_t, max_depth<B>> display;

    template <typename... Ts>
    static auto make_node(Ts&&... xs)
    {
        return dvektor::make_node<T, B, MP>(std::forward<Ts>(xs)...);
    }

    const T& get_elem(std::size_t index, std::size_t xr) const
    {
        auto display_idx = fast_log2(xr) / B;
        auto node        = display[display_idx].get();
        auto shift       = display_idx * B;
        while (display_idx--) {
            node = node->inner()[(index >> shift) & mask<B>].get();
            shift -= B;
        }
        return node->leaf()[index & mask<B>];
    }

    node_ptr_t null_slot_and_copy_inner(node_ptr_t& node, std::size_t idx)
    {
        auto& n = node->inner();
        auto x  = node_ptr_t{};
        x.swap(n[idx]);
        return copy_of_inner(x);
    }

    node_ptr_t null_slot_and_copy_leaf(node_ptr_t& node, std::size_t idx)
    {
        auto& n = node->inner();
        auto x  = node_ptr_t{};
        x.swap(n[idx]);
        return copy_of_leaf(x);
    }

    node_ptr_t copy_of_inner(const node_ptr_t& n)
    {
        return make_node(n->inner());
    }

    node_ptr_t copy_of_leaf(const node_ptr_t& n)
    {
        return make_node(n->leaf());
    }

    void stabilize(std::size_t index)
    {
        auto shift = B;
        for (auto i = 1u; i < depth; ++i) {
            display[i] = copy_of_inner(display[i]);
            display[i]->inner()[(index >> shift) & mask<B>] = display[i - 1];
            shift += B;
        }
    }

    void goto_pos_writable_from_clean(std::size_t old_index,
                                      std::size_t index,
                                      std::size_t xr)
    {
        assert(depth);
        auto d = depth - 1;
        if (d == 0) {
            display[0] = copy_of_leaf(display[0]);
        } else {
            IMMER_UNREACHABLE;
            display[d] = copy_of_inner(display[d]);
            auto shift = B * d;
            while (--d) {
                display[d] = null_slot_and_copy_inner(
                    display[d + 1], (index >> shift) & mask<B>);
                shift -= B;
            }
            display[0] =
                null_slot_and_copy_leaf(display[1], (index >> B) & mask<B>);
        }
    }

    void goto_pos_writable_from_dirty(std::size_t old_index,
                                      std::size_t new_index,
                                      std::size_t xr)
    {
        assert(depth);
        if (xr < (1 << B)) {
            display[0] = copy_of_leaf(display[0]);
        } else {
            auto display_idx = fast_log2(xr) / B;
            auto shift       = B;
            for (auto i = 1u; i <= display_idx; ++i) {
                display[i] = copy_of_inner(display[i]);
                display[i]->inner()[(old_index >> shift) & mask<B>] =
                    display[i - 1];
                shift += B;
            }
            for (auto i = display_idx - 1; i > 0; --i) {
                shift -= B;
                display[i] = null_slot_and_copy_inner(
                    display[i + 1], (new_index >> shift) & mask<B>);
            }
            display[0] =
                null_slot_and_copy_leaf(display[1], (new_index >> B) & mask<B>);
        }
    }

    void goto_fresh_pos_writable_from_clean(std::size_t old_index,
                                            std::size_t new_index,
                                            std::size_t xr)
    {
        auto display_idx = fast_log2(xr) / B;
        if (display_idx > 0) {
            auto shift = display_idx * B;
            if (display_idx == depth) {
                display[display_idx] = make_node(inner_t{});
                display[display_idx]->inner()[(old_index >> shift) & mask<B>] =
                    display[display_idx - 1];
                ++depth;
            }
            while (--display_idx) {
                auto node = display[display_idx + 1]
                                ->inner()[(new_index >> shift) & mask<B>];
                display[display_idx] =
                    node ? std::move(node) : make_node(inner_t{});
            }
            display[0] = make_node(leaf_t{});
        }
    }

    void goto_fresh_pos_writable_from_dirty(std::size_t old_index,
                                            std::size_t new_index,
                                            std::size_t xr)
    {
        stabilize(old_index);
        goto_fresh_pos_writable_from_clean(old_index, new_index, xr);
    }

    void goto_next_block_start(std::size_t index, std::size_t xr)
    {
        auto display_idx = fast_log2(xr) / B;
        auto shift       = display_idx * B;
        if (display_idx > 0) {
            display[display_idx - 1] =
                display[display_idx]->inner()[(index >> shift) & mask<B>];
            while (--display_idx)
                display[display_idx - 1] = display[display_idx]->inner()[0];
        }
    }

    void goto_pos(std::size_t index, std::size_t xr)
    {
        auto display_idx = fast_log2(xr) / B;
        auto shift       = display_idx * B;
        if (display_idx) {
            do {
                display[display_idx - 1] =
                    display[display_idx]->inner()[(index >> shift) & mask<B>];
                shift -= B;
            } while (--display_idx);
        }
    }
};

template <typename T, int B, typename MP>
struct impl
{
    using inner_t    = inner_node<T, B, MP>;
    using leaf_t     = leaf_node<T, B>;
    using node_t     = node<T, B, MP>;
    using node_ptr_t = node_ptr<T, B, MP>;
    using ref_t      = ref<T, B, MP>;

    std::size_t size;
    std::size_t focus;
    bool dirty;
    ref_t p;

    template <typename... Ts>
    static auto make_node(Ts&&... xs)
    {
        return dvektor::make_node<T, B, MP>(std::forward<Ts>(xs)...);
    }

    void goto_pos_writable(std::size_t old_index,
                           std::size_t new_index,
                           std::size_t xr)
    {
        if (dirty) {
            p.goto_pos_writable_from_dirty(old_index, new_index, xr);
        } else {
            p.goto_pos_writable_from_clean(old_index, new_index, xr);
            dirty = true;
        }
    }

    void goto_fresh_pos_writable(std::size_t old_index,
                                 std::size_t new_index,
                                 std::size_t xr)
    {
        if (dirty) {
            p.goto_fresh_pos_writable_from_dirty(old_index, new_index, xr);
        } else {
            p.goto_fresh_pos_writable_from_clean(old_index, new_index, xr);
            dirty = true;
        }
    }

    impl push_back(T value) const
    {
        if (size) {
            auto block_index = size & ~mask<B>;
            auto lo          = size & mask<B>;
            if (size != block_index) {
                auto s = impl{size + 1, block_index, dirty, p};
                s.goto_pos_writable(focus, block_index, focus ^ block_index);
                s.p.display[0]->leaf()[lo] = std::move(value);
                return s;
            } else {
                auto s = impl{size + 1, block_index, dirty, p};
                s.goto_fresh_pos_writable(
                    focus, block_index, focus ^ block_index);
                s.p.display[0]->leaf()[lo] = std::move(value);
                return s;
            }
        } else {
            return impl{
                1, 0, false, {1, {{make_node(leaf_t{{std::move(value)}})}}}};
        }
    }

    const T& get(std::size_t index) const
    {
        return p.get_elem(index, index ^ focus);
    }

    template <typename FnT>
    impl update(std::size_t idx, FnT&& fn) const
    {
        auto s = impl{size, idx, dirty, p};
        s.goto_pos_writable(focus, idx, focus ^ idx);
        auto& v = s.p.display[0]->leaf()[idx & mask<B>];
        v       = fn(std::move(v));
        return s;
    }

    impl assoc(std::size_t idx, T value) const
    {
        return update(idx, [&](auto&&) { return std::move(value); });
    }
};

template <typename T, int B, typename MP>
const impl<T, B, MP> empty = {0, 0, false, ref<T, B, MP>{1, {}}};

template <typename T, int B, typename MP>
struct iterator
    : boost::iterator_facade<iterator<T, B, MP>,
                             T,
                             boost::random_access_traversal_tag,
                             const T&>
{
    struct end_t
    {};

    iterator() = default;

    iterator(const impl<T, B, MP>& v)
        : p_{v.p}
        , i_{0}
        , base_{0}
    {
        if (v.dirty)
            p_.stabilize(v.focus);
        p_.goto_pos(0, 0 ^ v.focus);
        curr_ = p_.display[0]->leaf().begin();
    }

    iterator(const impl<T, B, MP>& v, end_t)
        : p_{v.p}
        , i_{v.size}
        , base_{(v.size - 1) & ~mask<B>}
    {
        if (v.dirty)
            p_.stabilize(v.focus);
        p_.goto_pos(base_, base_ ^ v.focus);
        curr_ = p_.display[0]->leaf().begin() + (i_ - base_);
    }

private:
    friend class boost::iterator_core_access;
    using leaf_iterator = typename leaf_node<T, B>::const_iterator;

    ref<T, B, MP> p_;
    std::size_t i_;
    std::size_t base_;
    leaf_iterator curr_;

    void increment()
    {
        ++i_;
        if (i_ - base_ < branches<B>) {
            ++curr_;
        } else {
            auto new_base = base_ + branches<B>;
            p_.goto_next_block_start(new_base, base_ ^ new_base);
            base_ = new_base;
            curr_ = p_.display[0]->leaf().begin();
        }
    }

    void decrement()
    {
        assert(i_ > 0);
        --i_;
        if (i_ >= base_) {
            --curr_;
        } else {
            auto new_base = base_ - branches<B>;
            p_.goto_pos(new_base, base_ ^ new_base);
            base_ = new_base;
            curr_ = std::prev(p_.display[0]->leaf().end());
        }
    }

    void advance(std::ptrdiff_t n)
    {
        i_ += n;
        if (i_ <= base_ && i_ - base_ < branches<B>) {
            curr_ += n;
        } else {
            auto new_base = i_ & ~mask<B>;
            p_.goto_pos(new_base, base_ ^ new_base);
            base_ = new_base;
            curr_ = p_.display[0]->leaf().begin() + (i_ - base_);
        }
    }

    bool equal(const iterator& other) const { return i_ == other.i_; }

    std::ptrdiff_t distance_to(const iterator& other) const
    {
        return other.i_ > i_ ? static_cast<std::ptrdiff_t>(other.i_ - i_)
                             : -static_cast<std::ptrdiff_t>(i_ - other.i_);
    }

    const T& dereference() const { return *curr_; }
};

} /* namespace dvektor */
} /* namespace detail */
} /* namespace immer */
