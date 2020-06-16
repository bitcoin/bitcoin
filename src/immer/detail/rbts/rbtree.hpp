//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <immer/detail/rbts/node.hpp>
#include <immer/detail/rbts/position.hpp>
#include <immer/detail/rbts/operations.hpp>

#include <immer/detail/type_traits.hpp>

#include <cassert>
#include <memory>
#include <numeric>

namespace immer {
namespace detail {
namespace rbts {

template <typename T,
          typename MemoryPolicy,
          bits_t B,
          bits_t BL>
struct rbtree
{
    using node_t  = node<T, MemoryPolicy, B, BL>;
    using edit_t  = typename node_t::edit_t;
    using owner_t = typename MemoryPolicy::transience_t::owner;

    size_t   size;
    shift_t  shift;
    node_t*  root;
    node_t*  tail;

    static const rbtree& empty()
    {
        static const rbtree empty_ {
            0,
            BL,
            node_t::make_inner_n(0u),
            node_t::make_leaf_n(0u)
        };
        return empty_;
    }

    template <typename U>
    static auto from_initializer_list(std::initializer_list<U> values)
    {
        auto e = owner_t{};
        auto result = rbtree{empty()};
        for (auto&& v : values)
            result.push_back_mut(e, v);
        return result;
    }

    template <typename Iter, typename Sent,
              std::enable_if_t
              <compatible_sentinel_v<Iter, Sent>, bool> = true>
    static auto from_range(Iter first, Sent last)
    {
        auto e = owner_t{};
        auto result = rbtree{empty()};
        for (; first != last; ++first)
            result.push_back_mut(e, *first);
        return result;
    }

    static auto from_fill(size_t n, T v)
    {
        auto e = owner_t{};
        auto result = rbtree{empty()};
        while (n --> 0)
            result.push_back_mut(e, v);
        return result;
    }

    rbtree(size_t sz, shift_t sh, node_t* r, node_t* t)
        : size{sz}, shift{sh}, root{r}, tail{t}
    {
        assert(check_tree());
    }

    rbtree(const rbtree& other)
        : rbtree{other.size, other.shift, other.root, other.tail}
    {
        inc();
    }

    rbtree(rbtree&& other)
        : rbtree{empty()}
    {
        swap(*this, other);
    }

    rbtree& operator=(const rbtree& other)
    {
        auto next = other;
        swap(*this, next);
        return *this;
    }

    rbtree& operator=(rbtree&& other)
    {
        swap(*this, other);
        return *this;
    }

    friend void swap(rbtree& x, rbtree& y)
    {
        using std::swap;
        swap(x.size,  y.size);
        swap(x.shift, y.shift);
        swap(x.root,  y.root);
        swap(x.tail,  y.tail);
    }

    ~rbtree()
    {
        dec();
    }

    void inc() const
    {
        root->inc();
        tail->inc();
    }

    void dec() const
    {
        traverse(dec_visitor());
    }

    auto tail_size() const
    {
        return size ? ((size - 1) & mask<BL>) + 1 : 0;
    }

    auto tail_offset() const
    {
        return size ? (size - 1) & ~mask<BL> : 0;
    }

    template <typename Visitor, typename... Args>
    void traverse(Visitor v, Args&&... args) const
    {
        auto tail_off  = tail_offset();
        auto tail_size = size - tail_off;

        if (tail_off) make_regular_sub_pos(root, shift, tail_off).visit(v, args...);
        else make_empty_regular_pos(root).visit(v, args...);

        make_leaf_sub_pos(tail, tail_size).visit(v, args...);
    }

    template <typename Visitor, typename... Args>
    void traverse(Visitor v, size_t first, size_t last, Args&&... args) const
    {
        auto tail_off  = tail_offset();
        auto tail_size = size - tail_off;

        if (first < tail_off)
            make_regular_sub_pos(root, shift, tail_off).visit(
                v,
                first,
                last < tail_off ? last : tail_off,
                args...);
        if (last > tail_off)
            make_leaf_sub_pos(tail, tail_size).visit(
                v,
                first > tail_off ? first - tail_off : 0,
                last - tail_off,
                args...);
    }

    template <typename Visitor, typename... Args>
    bool traverse_p(Visitor v, Args&&... args) const
    {
        auto tail_off  = tail_offset();
        auto tail_size = size - tail_off;
        return (tail_off
                ? make_regular_sub_pos(root, shift, tail_off).visit(v, args...)
                : make_empty_regular_pos(root).visit(v, args...))
            && make_leaf_sub_pos(tail, tail_size).visit(v, args...);
    }

    template <typename Visitor, typename... Args>
    bool traverse_p(Visitor v, size_t first, size_t last, Args&&... args) const
    {
        auto tail_off  = tail_offset();
        auto tail_size = size - tail_off;

        return
            (first < tail_off
             ? make_regular_sub_pos(root, shift, tail_off).visit(
                 v,
                 first,
                 last < tail_off ? last : tail_off,
                 args...)
             : true)
         && (last > tail_off
             ? make_leaf_sub_pos(tail, tail_size).visit(
                 v,
                 first > tail_off ? first - tail_off : 0,
                 last - tail_off,
                 args...)
             : true);
    }

    template <typename Visitor>
    decltype(auto) descend(Visitor v, size_t idx) const
    {
        auto tail_off  = tail_offset();
        return idx >= tail_off
            ? make_leaf_descent_pos(tail).visit(v, idx)
            : visit_regular_descent(root, shift, v, idx);
    }

    template <typename Fn>
    void for_each_chunk(Fn&& fn) const
    {
        traverse(for_each_chunk_visitor{}, std::forward<Fn>(fn));
    }

    template <typename Fn>
    void for_each_chunk(size_t first, size_t last, Fn&& fn) const
    {
        traverse(for_each_chunk_i_visitor{}, first, last, std::forward<Fn>(fn));
    }

    template <typename Fn>
    bool for_each_chunk_p(Fn&& fn) const
    {
        return traverse_p(for_each_chunk_p_visitor{}, std::forward<Fn>(fn));
    }

    template <typename Fn>
    bool for_each_chunk_p(size_t first, size_t last, Fn&& fn) const
    {
        return traverse_p(for_each_chunk_p_i_visitor{}, first, last, std::forward<Fn>(fn));
    }

    bool equals(const rbtree& other) const
    {
        if (size != other.size) return false;
        if (size == 0) return true;
        return (size <= branches<BL>
                || make_regular_sub_pos(root, shift, tail_offset()).visit(
                    equals_visitor{}, other.root))
            && make_leaf_sub_pos(tail, tail_size()).visit(
                equals_visitor{}, other.tail);
    }

    void ensure_mutable_tail(edit_t e, count_t n)
    {
        if (!tail->can_mutate(e)) {
            auto new_tail = node_t::copy_leaf_e(e, tail, n);
            dec_leaf(tail, n);
            tail = new_tail;
        }
    }

    void push_back_mut(edit_t e, T value)
    {
        auto tail_off = tail_offset();
        auto ts = size - tail_off;
        if (ts < branches<BL>) {
            ensure_mutable_tail(e, ts);
            new (&tail->leaf()[ts]) T{std::move(value)};
        } else {
            auto new_tail = node_t::make_leaf_e(e, std::move(value));
            try {
                if (tail_off == size_t{branches<B>} << shift) {
                    auto new_root = node_t::make_inner_e(e);
                    try {
                        auto path = node_t::make_path_e(e, shift, tail);
                        new_root->inner() [0] = root;
                        new_root->inner() [1] = path;
                        root = new_root;
                        tail = new_tail;
                        shift += B;
                    } catch (...) {
                        node_t::delete_inner_e(new_root);
                        throw;
                    }
                } else if (tail_off) {
                    auto new_root = make_regular_sub_pos(root, shift, tail_off)
                        .visit(push_tail_mut_visitor<node_t>{}, e, tail);
                    root = new_root;
                    tail = new_tail;
                } else {
                    auto new_root = node_t::make_path_e(e, shift, tail);
                    assert(tail_off == 0);
                    dec_empty_regular(root);
                    root = new_root;
                    tail = new_tail;
                }
            } catch (...) {
                node_t::delete_leaf(new_tail, 1);
                throw;
            }
        }
        ++size;
    }

    rbtree push_back(T value) const
    {
        auto tail_off = tail_offset();
        auto ts = size - tail_off;
        if (ts < branches<BL>) {
            auto new_tail = node_t::copy_leaf_emplace(tail, ts,
                                                      std::move(value));
            return { size + 1, shift, root->inc(), new_tail };
        } else {
            auto new_tail = node_t::make_leaf_n(1, std::move(value));
            try {
                if (tail_off == size_t{branches<B>} << shift) {
                    auto new_root = node_t::make_inner_n(2);
                    try {
                        auto path = node_t::make_path(shift, tail);
                        new_root->inner() [0] = root;
                        new_root->inner() [1] = path;
                        root->inc();
                        tail->inc();
                        return { size + 1, shift + B, new_root, new_tail };
                    } catch (...) {
                        node_t::delete_inner(new_root, 2);
                        throw;
                    }
                } else if (tail_off) {
                    auto new_root = make_regular_sub_pos(root, shift, tail_off)
                        .visit(push_tail_visitor<node_t>{}, tail);
                    tail->inc();
                    return { size + 1, shift, new_root, new_tail };
                } else {
                    auto new_root = node_t::make_path(shift, tail);
                    tail->inc();
                    return { size + 1, shift, new_root, new_tail };
                }
            } catch (...) {
                node_t::delete_leaf(new_tail, 1);
                throw;
            }
        }
    }

    const T* array_for(size_t index) const
    {
        return descend(array_for_visitor<T>(), index);
    }

    T& get_mut(edit_t e, size_t idx)
    {
        auto tail_off = tail_offset();
        if (idx >= tail_off) {
            ensure_mutable_tail(e, size - tail_off);
            return tail->leaf() [idx & mask<BL>];
        } else {
            return make_regular_sub_pos(root, shift, tail_off)
                .visit(get_mut_visitor<node_t>{}, idx, e, &root);
        }
    }

    const T& get(size_t index) const
    {
        return descend(get_visitor<T>(), index);
    }

    const T& get_check(size_t index) const
    {
        if (index >= size)
            throw std::out_of_range{"index out of range"};
        return descend(get_visitor<T>(), index);
    }

    const T& front() const
    {
        return get(0);
    }

    const T& back() const
    {
        return tail->leaf()[(size - 1) & mask<BL>];
    }

    template <typename FnT>
    void update_mut(edit_t e, size_t idx, FnT&& fn)
    {
        auto& elem = get_mut(e, idx);
        elem = std::forward<FnT>(fn) (std::move(elem));
    }

    template <typename FnT>
    rbtree update(size_t idx, FnT&& fn) const
    {
        auto tail_off  = tail_offset();
        if (idx >= tail_off) {
            auto tail_size = size - tail_off;
            auto new_tail  = make_leaf_sub_pos(tail, tail_size)
                .visit(update_visitor<node_t>{}, idx - tail_off, fn);
            return { size, shift, root->inc(), new_tail };
        } else {
            auto new_root  = make_regular_sub_pos(root, shift, tail_off)
                .visit(update_visitor<node_t>{}, idx, fn);
            return { size, shift, new_root, tail->inc() };
        }
    }

    void assoc_mut(edit_t e, size_t idx, T value)
    {
        update_mut(e, idx, [&] (auto&&) {
                return std::move(value);
            });
    }

    rbtree assoc(size_t idx, T value) const
    {
        return update(idx, [&] (auto&&) {
                return std::move(value);
            });
    }

    rbtree take(size_t new_size) const
    {
        auto tail_off = tail_offset();
        if (new_size == 0) {
            return empty();
        } else if (new_size >= size) {
            return *this;
        } else if (new_size > tail_off) {
            auto new_tail = node_t::copy_leaf(tail, new_size - tail_off);
            return { new_size, shift, root->inc(), new_tail };
        } else {
            using std::get;
            auto l = new_size - 1;
            auto v = slice_right_visitor<node_t>();
            auto r = make_regular_sub_pos(root, shift, tail_off).visit(v, l);
            auto new_shift = get<0>(r);
            auto new_root  = get<1>(r);
            auto new_tail  = get<3>(r);
            if (new_root) {
                assert(new_root->compute_shift() == get<0>(r));
                assert(new_root->check(new_shift, new_size - get<2>(r)));
                return { new_size, new_shift, new_root, new_tail };
            } else {
                return { new_size, BL, empty().root->inc(), new_tail };
            }
        }
    }

    void take_mut(edit_t e, size_t new_size)
    {
        auto tail_off = tail_offset();
        if (new_size == 0) {
            // todo: more efficient?
            *this = empty();
        } else if (new_size >= size) {
            return;
        } else if (new_size > tail_off) {
            auto ts    = size - tail_off;
            auto newts = new_size - tail_off;
            if (tail->can_mutate(e)) {
                destroy_n(tail->leaf() + newts, ts - newts);
            } else {
                auto new_tail = node_t::copy_leaf_e(e, tail, newts);
                dec_leaf(tail, ts);
                tail = new_tail;
            }
            size = new_size;
            return;
        } else {
            using std::get;
            auto l = new_size - 1;
            auto v = slice_right_mut_visitor<node_t>();
            auto r = make_regular_sub_pos(root, shift, tail_off).visit(v, l, e);
            auto new_shift = get<0>(r);
            auto new_root  = get<1>(r);
            auto new_tail  = get<3>(r);
            if (new_root) {
                root  = new_root;
                shift = new_shift;
            } else {
                root  = empty().root->inc();
                shift = BL;
            }
            dec_leaf(tail, size - tail_off);
            size  = new_size;
            tail  = new_tail;
            return;
        }
    }

    bool check_tree() const
    {
#if IMMER_DEBUG_DEEP_CHECK
        assert(shift >= BL);
        assert(tail_offset() <= size);
        assert(check_root());
        assert(check_tail());
#endif
        return true;
    }

    bool check_tail() const
    {
#if IMMER_DEBUG_DEEP_CHECK
        if (tail_size() > 0)
            assert(tail->check(0, tail_size()));
#endif
        return true;
    }

    bool check_root() const
    {
#if IMMER_DEBUG_DEEP_CHECK
        if (tail_offset() > 0)
            assert(root->check(shift, tail_offset()));
        else {
            assert(root->kind() == node_t::kind_t::inner);
            assert(shift == BL);
        }
#endif
        return true;
    }
};

} // namespace rbts
} // namespace detail
} // namespace immer
