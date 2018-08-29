//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <immer/config.hpp>
#include <immer/detail/rbts/node.hpp>
#include <immer/detail/rbts/position.hpp>
#include <immer/detail/rbts/operations.hpp>

#include <cassert>
#include <memory>
#include <numeric>

namespace immer {
namespace detail {
namespace rbts {

template <typename T,
          typename MemoryPolicy,
          bits_t   B,
          bits_t   BL>
struct rrbtree_iterator;

template <typename T,
          typename MemoryPolicy,
          bits_t   B,
          bits_t   BL>
struct rrbtree
{
    using node_t  = node<T, MemoryPolicy, B, BL>;
    using edit_t  = typename node_t::edit_t;
    using owner_t = typename MemoryPolicy::transience_t::owner;

    size_t  size;
    shift_t shift;
    node_t* root;
    node_t* tail;

    static const rrbtree empty;

    template <typename U>
    static auto from_initializer_list(std::initializer_list<U> values)
    {
        auto e = owner_t{};
        auto result = rrbtree{empty};
        for (auto&& v : values)
            result.push_back_mut(e, v);
        return result;
    }

    template <typename Iter>
    static auto from_range(Iter first, Iter last)
    {
        auto e = owner_t{};
        auto result = rrbtree{empty};
        for (; first != last; ++first)
            result.push_back_mut(e, *first);
        return result;
    }

    static auto from_fill(size_t n, T v)
    {
        auto e = owner_t{};
        auto result = rrbtree{empty};
        while (n --> 0)
            result.push_back_mut(e, v);
        return result;
    }

    rrbtree(size_t sz, shift_t sh, node_t* r, node_t* t)
        : size{sz}, shift{sh}, root{r}, tail{t}
    {
        assert(check_tree());
    }

    rrbtree(const rrbtree& other)
        : rrbtree{other.size, other.shift, other.root, other.tail}
    {
        inc();
    }

    rrbtree(rrbtree&& other)
        : rrbtree{empty}
    {
        swap(*this, other);
    }

    rrbtree& operator=(const rrbtree& other)
    {
        auto next{other};
        swap(*this, next);
        return *this;
    }

    rrbtree& operator=(rrbtree&& other)
    {
        swap(*this, other);
        return *this;
    }

    friend void swap(rrbtree& x, rrbtree& y)
    {
        using std::swap;
        swap(x.size,  y.size);
        swap(x.shift, y.shift);
        swap(x.root,  y.root);
        swap(x.tail,  y.tail);
    }

    ~rrbtree()
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
        return size - tail_offset();
    }

    auto tail_offset() const
    {
        auto r = root->relaxed();
        assert(r == nullptr || r->d.count);
        return
            r               ? r->d.sizes[r->d.count - 1] :
            size            ? (size - 1) & ~mask<BL>
            /* otherwise */ : 0;
    }

    template <typename Visitor, typename... Args>
    void traverse(Visitor v, Args&&... args) const
    {
        auto tail_off  = tail_offset();
        auto tail_size = size - tail_off;

        if (tail_off) visit_maybe_relaxed_sub(root, shift, tail_off, v, args...);
        else make_empty_regular_pos(root).visit(v, args...);

        if (tail_size) make_leaf_sub_pos(tail, tail_size).visit(v, args...);
        else make_empty_leaf_pos(tail).visit(v, args...);
    }

    template <typename Visitor, typename... Args>
    void traverse(Visitor v, size_t first, size_t last, Args&&... args) const
    {
        auto tail_off  = tail_offset();
        auto tail_size = size - tail_off;

        if (first < tail_off)
            visit_maybe_relaxed_sub(root, shift, tail_off, v,
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
                ? visit_maybe_relaxed_sub(root, shift, tail_off, v, args...)
                : make_empty_regular_pos(root).visit(v, args...))
            && (tail_size
                ? make_leaf_sub_pos(tail, tail_size).visit(v, args...)
                : make_empty_leaf_pos(tail).visit(v, args...));
    }

    template <typename Visitor, typename... Args>
    bool traverse_p(Visitor v, size_t first, size_t last, Args&&... args) const
    {
        auto tail_off  = tail_offset();
        auto tail_size = size - tail_off;
        return
            (first < tail_off
             ? visit_maybe_relaxed_sub(root, shift, tail_off, v,
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
            ? make_leaf_descent_pos(tail).visit(v, idx - tail_off)
            : visit_maybe_relaxed_descent(root, shift, v, idx);
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

    bool equals(const rrbtree& other) const
    {
        using iter_t = rrbtree_iterator<T, MemoryPolicy, B, BL>;
        if (size != other.size) return false;
        if (size == 0) return true;
        auto tail_off = tail_offset();
        auto tail_off_other = other.tail_offset();
        // compare trees
        if (tail_off > 0 && tail_off_other > 0) {
            // other.shift != shift is a theoretical possibility for
            // relaxed trees that sadly we haven't managed to exercise
            // in tests yet...
            if (other.shift >= shift) {
                if (!visit_maybe_relaxed_sub(
                        other.root, other.shift, tail_off_other,
                        equals_visitor::rrb{}, iter_t{other},
                        root, shift, tail_off))
                    return false;
            } else {
                if (!visit_maybe_relaxed_sub(
                        root, shift, tail_off,
                        equals_visitor::rrb{}, iter_t{*this},
                        other.root, other.shift, tail_off_other))
                    return false;
            }
        }
        return
            tail_off == tail_off_other ? make_leaf_sub_pos(
                tail, tail_size()).visit(
                    equals_visitor{}, other.tail) :
            tail_off > tail_off_other  ? std::equal(
                tail->leaf(), tail->leaf() + (size - tail_off),
                other.tail->leaf() + (tail_off - tail_off_other))
            /* otherwise */            : std::equal(
                tail->leaf(), tail->leaf() + (size - tail_off),
                iter_t{other} + tail_off);
    }

    std::tuple<shift_t, node_t*>
    push_tail(node_t* root, shift_t shift, size_t size,
              node_t* tail, count_t tail_size) const
    {
        if (auto r = root->relaxed()) {
            auto new_root = make_relaxed_pos(root, shift, r)
                .visit(push_tail_visitor<node_t>{}, tail, tail_size);
            if (new_root)
                return { shift, new_root };
            else {
                auto new_root = node_t::make_inner_r_n(2);
                try {
                    auto new_path = node_t::make_path(shift, tail);
                    new_root->inner() [0] = root->inc();
                    new_root->inner() [1] = new_path;
                    new_root->relaxed()->d.sizes [0] = size;
                    new_root->relaxed()->d.sizes [1] = size + tail_size;
                    new_root->relaxed()->d.count = 2u;
                } catch (...) {
                    node_t::delete_inner_r(new_root, 2);
                    throw;
                }
                return { shift + B, new_root };
            }
        } else if (size == size_t{branches<B>} << shift) {
            auto new_root = node_t::make_inner_n(2);
            try {
                auto new_path = node_t::make_path(shift, tail);
                new_root->inner() [0] = root->inc();
                new_root->inner() [1] = new_path;
            } catch (...) {
                node_t::delete_inner(new_root, 2);
                throw;
            }
            return { shift + B, new_root };
        } else if (size) {
            auto new_root = make_regular_sub_pos(root, shift, size)
                .visit(push_tail_visitor<node_t>{}, tail);
            return { shift, new_root };
        } else {
            return { shift, node_t::make_path(shift, tail) };
        }
    }

    void push_tail_mut(edit_t e, size_t tail_off,
                       node_t* tail, count_t tail_size)
    {
        if (auto r = root->relaxed()) {
            auto new_root = make_relaxed_pos(root, shift, r)
                .visit(push_tail_mut_visitor<node_t>{}, e, tail, tail_size);
            if (new_root) {
                root = new_root;
            } else {
                auto new_root = node_t::make_inner_r_e(e);
                try {
                    auto new_path = node_t::make_path_e(e, shift, tail);
                    new_root->inner() [0] = root;
                    new_root->inner() [1] = new_path;
                    new_root->relaxed()->d.sizes [0] = tail_off;
                    new_root->relaxed()->d.sizes [1] = tail_off + tail_size;
                    new_root->relaxed()->d.count = 2u;
                    root = new_root;
                    shift += B;
                } catch (...) {
                    node_t::delete_inner_r_e(new_root);
                    throw;
                }
            }
        } else if (tail_off == size_t{branches<B>} << shift) {
            auto new_root = node_t::make_inner_e(e);
            try {
                auto new_path = node_t::make_path_e(e, shift, tail);
                new_root->inner() [0] = root;
                new_root->inner() [1] = new_path;
                root = new_root;
                shift += B;
            } catch (...) {
                node_t::delete_inner_e(new_root);
                throw;
            }
        } else if (tail_off) {
            auto new_root = make_regular_sub_pos(root, shift, tail_off)
                .visit(push_tail_mut_visitor<node_t>{}, e, tail);
            root = new_root;
        } else {
            auto new_root = node_t::make_path_e(e, shift, tail);
            dec_empty_regular(root);
            root = new_root;
        }
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
        auto ts = tail_size();
        if (ts < branches<BL>) {
            ensure_mutable_tail(e, ts);
            new (&tail->leaf()[ts]) T{std::move(value)};
        } else {
            using std::get;
            auto new_tail = node_t::make_leaf_e(e, std::move(value));
            auto tail_off = tail_offset();
            try {
                push_tail_mut(e, tail_off, tail, ts);
                tail = new_tail;
            } catch (...) {
                node_t::delete_leaf(new_tail, 1u);
                throw;
            }
        }
        ++size;
    }

    rrbtree push_back(T value) const
    {
        auto ts = tail_size();
        if (ts < branches<BL>) {
            auto new_tail = node_t::copy_leaf_emplace(tail, ts,
                                                      std::move(value));
            return { size + 1, shift, root->inc(), new_tail };
        } else {
            using std::get;
            auto new_tail = node_t::make_leaf_n(1u, std::move(value));
            auto tail_off = tail_offset();
            try {
                auto new_root = push_tail(root, shift, tail_off,
                                          tail, size - tail_off);
                tail->inc();
                return { size + 1, get<0>(new_root), get<1>(new_root), new_tail };
            } catch (...) {
                node_t::delete_leaf(new_tail, 1u);
                throw;
            }
        }
    }

    std::tuple<const T*, size_t, size_t>
    region_for(size_t idx) const
    {
        using std::get;
        auto tail_off = tail_offset();
        if (idx >= tail_off) {
            return { tail->leaf(), tail_off, size };
        } else {
            auto subs = visit_maybe_relaxed_sub(
                root, shift, tail_off,
                region_for_visitor<T>(), idx);
            auto first = idx - get<1>(subs);
            auto end   = first + get<2>(subs);
            return { get<0>(subs), first, end };
        }
    }

    T& get_mut(edit_t e, size_t idx)
    {
        auto tail_off = tail_offset();
        if (idx >= tail_off) {
            ensure_mutable_tail(e, size - tail_off);
            return tail->leaf() [(idx - tail_off) & mask<BL>];
        } else {
            return visit_maybe_relaxed_sub(
                root, shift, tail_off,
                get_mut_visitor<node_t>{}, idx, e, &root);
        }
    }

    const T& get(size_t index) const
    {
        return descend(get_visitor<T>(), index);
    }

    const T& get_check(size_t index) const
    {
        if (index >= size)
            throw std::out_of_range{"out of range"};
        return descend(get_visitor<T>(), index);
    }

    const T& front() const
    {
        return get(0);
    }

    const T& back() const
    {
        return get(size - 1);
    }

    template <typename FnT>
    void update_mut(edit_t e, size_t idx, FnT&& fn)
    {
        auto& elem = get_mut(e, idx);
        elem = std::forward<FnT>(fn) (std::move(elem));
    }

    template <typename FnT>
    rrbtree update(size_t idx, FnT&& fn) const
    {
        auto tail_off  = tail_offset();
        if (idx >= tail_off) {
            auto tail_size = size - tail_off;
            auto new_tail  = make_leaf_sub_pos(tail, tail_size)
                .visit(update_visitor<node_t>{}, idx - tail_off, fn);
            return { size, shift, root->inc(), new_tail };
        } else {
            auto new_root  = visit_maybe_relaxed_sub(
                root, shift, tail_off,
                update_visitor<node_t>{}, idx, fn);
            return { size, shift, new_root, tail->inc() };
        }
    }

    void assoc_mut(edit_t e, size_t idx, T value)
    {
        update_mut(e, idx, [&] (auto&&) {
                return std::move(value);
            });
    }

    rrbtree assoc(size_t idx, T value) const
    {
        return update(idx, [&] (auto&&) {
                return std::move(value);
            });
    }

    void take_mut(edit_t e, size_t new_size)
    {
        auto tail_off = tail_offset();
        if (new_size == 0) {
            *this = empty;
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
            auto r = visit_maybe_relaxed_sub(root, shift, tail_off, v, l, e);
            auto new_shift = get<0>(r);
            auto new_root  = get<1>(r);
            auto new_tail  = get<3>(r);
            if (new_root) {
                root  = new_root;
                shift = new_shift;
            } else {
                root  = empty.root->inc();
                shift = BL;
            }
            dec_leaf(tail, size - tail_off);
            size  = new_size;
            tail  = new_tail;
            return;
        }
    }

    rrbtree take(size_t new_size) const
    {
        auto tail_off = tail_offset();
        if (new_size == 0) {
            return empty;
        } else if (new_size >= size) {
            return *this;
        } else if (new_size > tail_off) {
            auto new_tail = node_t::copy_leaf(tail, new_size - tail_off);
            return { new_size, shift, root->inc(), new_tail };
        } else {
            using std::get;
            auto l = new_size - 1;
            auto v = slice_right_visitor<node_t>();
            auto r = visit_maybe_relaxed_sub(root, shift, tail_off, v, l);
            auto new_shift = get<0>(r);
            auto new_root  = get<1>(r);
            auto new_tail  = get<3>(r);
            if (new_root) {
                assert(new_root->compute_shift() == get<0>(r));
                assert(new_root->check(new_shift, new_size - get<2>(r)));
                return { new_size, new_shift, new_root, new_tail };
            } else {
                return { new_size, BL, empty.root->inc(), new_tail };
            }
        }
    }

    void drop_mut(edit_t e, size_t elems)
    {
        using std::get;
        auto tail_off = tail_offset();
        if (elems == 0) {
            return;
        } else if (elems >= size) {
            *this = empty;
        } else if (elems == tail_off) {
            dec_inner(root, shift, tail_off);
            shift = BL;
            root  = empty.root->inc();
            size -= elems;
            return;
        } else if (elems > tail_off) {
            auto v = slice_left_mut_visitor<node_t>();
            tail = get<1>(make_leaf_sub_pos(tail, size - tail_off).visit(
                              v, elems - tail_off, e));
            if (root != empty.root) {
                dec_inner(root, shift, tail_off);
                shift = BL;
                root  = empty.root->inc();
            }
            size -= elems;
            return;
        } else {
            auto v = slice_left_mut_visitor<node_t>();
            auto r = visit_maybe_relaxed_sub(root, shift, tail_off, v, elems, e);
            shift = get<0>(r);
            root  = get<1>(r);
            size -= elems;
            return;
        }
    }

    rrbtree drop(size_t elems) const
    {
        if (elems == 0) {
            return *this;
        } else if (elems >= size) {
            return empty;
        } else if (elems == tail_offset()) {
            return { size - elems, BL, empty.root->inc(), tail->inc() };
        } else if (elems > tail_offset()) {
            auto tail_off = tail_offset();
            auto new_tail = node_t::copy_leaf(tail, elems - tail_off,
                                              size - tail_off);
            return { size - elems, BL, empty.root->inc(), new_tail };
        } else {
            using std::get;
            auto v = slice_left_visitor<node_t>();
            auto r = visit_maybe_relaxed_sub(root, shift, tail_offset(), v, elems);
            auto new_root  = get<1>(r);
            auto new_shift = get<0>(r);
            return { size - elems, new_shift, new_root, tail->inc() };
        }
        return *this;
    }

    rrbtree concat(const rrbtree& r) const
    {
        assert(r.size < (std::numeric_limits<size_t>::max() - size));
        using std::get;
        if (size == 0)
            return r;
        else if (r.size == 0)
            return *this;
        else if (r.tail_offset() == 0) {
            // just concat the tail, similar to push_back
            auto tail_offst = tail_offset();
            auto tail_size  = size - tail_offst;
            if (tail_size == branches<BL>) {
                auto new_root = push_tail(root, shift, tail_offst,
                                          tail, tail_size);
                tail->inc();
                return { size + r.size, get<0>(new_root), get<1>(new_root),
                         r.tail->inc() };
            } else if (tail_size + r.size <= branches<BL>) {
                auto new_tail = node_t::copy_leaf(tail, tail_size,
                                                  r.tail, r.size);
                return { size + r.size, shift, root->inc(), new_tail };
            } else {
                auto remaining = branches<BL> - tail_size;
                auto add_tail  = node_t::copy_leaf(tail, tail_size,
                                                   r.tail, remaining);
                try {
                    auto new_tail = node_t::copy_leaf(r.tail, remaining, r.size);
                    try {
                        auto new_root  = push_tail(root, shift, tail_offst,
                                                   add_tail, branches<BL>);
                        return { size + r.size,
                                 get<0>(new_root), get<1>(new_root),
                                 new_tail };
                    } catch (...) {
                        node_t::delete_leaf(new_tail, r.size - remaining);
                        throw;
                    }
                } catch (...) {
                    node_t::delete_leaf(add_tail, branches<BL>);
                    throw;
                }
            }
        } else if (tail_offset() == 0) {
            auto tail_offst = tail_offset();
            auto tail_size  = size - tail_offst;
            auto concated   = concat_trees(tail, tail_size,
                                           r.root, r.shift, r.tail_offset());
            auto new_shift  = concated.shift();
            auto new_root   = concated.node();
            assert(new_shift == new_root->compute_shift());
            assert(new_root->check(new_shift, size + r.tail_offset()));
            return { size + r.size, new_shift, new_root, r.tail->inc() };
        } else {
            auto tail_offst = tail_offset();
            auto tail_size  = size - tail_offst;
            auto concated   = concat_trees(root, shift, tail_offst,
                                           tail, tail_size,
                                           r.root, r.shift, r.tail_offset());
            auto new_shift  = concated.shift();
            auto new_root   = concated.node();
            assert(new_shift == new_root->compute_shift());
            assert(new_root->check(new_shift, size + r.tail_offset()));
            return { size + r.size, new_shift, new_root, r.tail->inc() };
        }
    }

    constexpr static bool supports_transient_concat =
        !std::is_empty<edit_t>::value;

    friend void concat_mut_l(rrbtree& l, edit_t el, const rrbtree& r)
    {
        assert(&l != &r);
        assert(r.size < (std::numeric_limits<size_t>::max() - l.size));
        using std::get;
        if (l.size == 0)
            l = r;
        else if (r.size == 0)
            return;
        else if (r.tail_offset() == 0) {
            // just concat the tail, similar to push_back
            auto tail_offst = l.tail_offset();
            auto tail_size  = l.size - tail_offst;
            if (tail_size == branches<BL>) {
                l.push_tail_mut(el, tail_offst, l.tail, tail_size);
                l.tail = r.tail->inc();
                l.size += r.size;
                return;
            } else if (tail_size + r.size <= branches<BL>) {
                l.ensure_mutable_tail(el, tail_size);
                std::uninitialized_copy(r.tail->leaf(),
                                        r.tail->leaf() + r.size,
                                        l.tail->leaf() + tail_size);
                l.size += r.size;
                return;
            } else {
                auto remaining = branches<BL> - tail_size;
                l.ensure_mutable_tail(el, tail_size);
                std::uninitialized_copy(r.tail->leaf(),
                                        r.tail->leaf() + remaining,
                                        l.tail->leaf() + tail_size);
                try {
                    auto new_tail = node_t::copy_leaf_e(el, r.tail, remaining, r.size);
                    try {
                        l.push_tail_mut(el, tail_offst, l.tail, branches<BL>);
                        l.tail = new_tail;
                        l.size += r.size;
                        return;
                    } catch (...) {
                        node_t::delete_leaf(new_tail, r.size - remaining);
                        throw;
                    }
                } catch (...) {
                    destroy_n(r.tail->leaf() + tail_size, remaining);
                    throw;
                }
            }
        } else if (l.tail_offset() == 0) {
            if (supports_transient_concat) {
                auto tail_offst = l.tail_offset();
                auto tail_size  = l.size - tail_offst;
                auto concated   = concat_trees_mut(
                    el,
                    el, l.tail, tail_size,
                    MemoryPolicy::transience_t::noone,
                    r.root, r.shift, r.tail_offset());
                assert(concated.shift() == concated.node()->compute_shift());
                assert(concated.node()->check(concated.shift(), l.size + r.tail_offset()));
                l.size += r.size;
                l.shift = concated.shift();
                l.root  = concated.node();
                l.tail  = r.tail;
            } else {
                auto tail_offst = l.tail_offset();
                auto tail_size  = l.size - tail_offst;
                auto concated   = concat_trees(l.tail, tail_size,
                                               r.root, r.shift, r.tail_offset());
                l = { l.size + r.size, concated.shift(),
                      concated.node(), r.tail->inc() };
                return;
            }
        } else {
            if (supports_transient_concat) {
                auto tail_offst = l.tail_offset();
                auto tail_size  = l.size - tail_offst;
                auto concated   = concat_trees_mut(
                    el,
                    el, l.root, l.shift, tail_offst, l.tail, tail_size,
                    MemoryPolicy::transience_t::noone,
                    r.root, r.shift, r.tail_offset());
                assert(concated.shift() == concated.node()->compute_shift());
                assert(concated.node()->check(concated.shift(), l.size + r.tail_offset()));
                l.size += r.size;
                l.shift = concated.shift();
                l.root  = concated.node();
                l.tail  = r.tail;
            } else {
                auto tail_offst = l.tail_offset();
                auto tail_size  = l.size - tail_offst;
                auto concated   = concat_trees(l.root, l.shift, tail_offst,
                                               l.tail, tail_size,
                                               r.root, r.shift, r.tail_offset());
                l = { l.size + r.size, concated.shift(),
                      concated.node(), r.tail->inc() };
            }
        }
    }

    friend void concat_mut_r(const rrbtree& l, rrbtree& r, edit_t er)
    {
        assert(&l != &r);
        assert(r.size < (std::numeric_limits<size_t>::max() - l.size));
        using std::get;
        if (r.size == 0)
            r = std::move(l);
        else if (l.size == 0)
            return;
        else if (r.tail_offset() == 0) {
            // just concat the tail, similar to push_back
            auto tail_offst = l.tail_offset();
            auto tail_size  = l.size - tail_offst;
            if (tail_size == branches<BL>) {
                // this could be improved by making sure that the
                // newly created nodes as part of the `push_tail()`
                // are tagged with `er`
                auto res = l.push_tail(l.root, l.shift, tail_offst,
                                       l.tail, tail_size);
                l.tail->inc(); // note: leak if mutably concatenated
                               // with itself, but this is forbidden
                               // by the interface
                r = { l.size + r.size, get<0>(res), get<1>(res),
                      r.tail->inc() };
                return;
            } else if (tail_size + r.size <= branches<BL>) {
                // doing this in a exception way mutating way is very
                // tricky while potential performance gains are
                // minimal (we need to move every element of the right
                // tail anyways to make space for the left tail)
                //
                // we could however improve this by at least moving the
                // elements of the right tail...
                auto new_tail = node_t::copy_leaf(l.tail, tail_size,
                                                  r.tail, r.size);
                r = { l.size + r.size, l.shift, l.root->inc(), new_tail };
                return;
            } else {
                // like the immutable version
                auto remaining = branches<BL> - tail_size;
                auto add_tail  = node_t::copy_leaf_e(er,
                                                     l.tail, tail_size,
                                                     r.tail, remaining);
                try {
                    auto new_tail = node_t::copy_leaf_e(er, r.tail, remaining, r.size);
                    try {
                        // this could be improved by making sure that the
                        // newly created nodes as part of the `push_tail()`
                        // are tagged with `er`
                        auto new_root = l.push_tail(l.root, l.shift, tail_offst,
                                                    add_tail, branches<BL>);
                        r = { l.size + r.size,
                              get<0>(new_root), get<1>(new_root),
                              new_tail };
                        return;
                    } catch (...) {
                        node_t::delete_leaf(new_tail, r.size - remaining);
                        throw;
                    }
                } catch (...) {
                    node_t::delete_leaf(add_tail, branches<BL>);
                    throw;
                }
                return;
            }
        } else if (l.tail_offset() == 0) {
            if (supports_transient_concat) {
                auto tail_offst = l.tail_offset();
                auto tail_size  = l.size - tail_offst;
                auto concated   = concat_trees_mut(
                    er,
                    MemoryPolicy::transience_t::noone, l.tail, tail_size,
                    er,r.root, r.shift, r.tail_offset());
                assert(concated.shift() == concated.node()->compute_shift());
                assert(concated.node()->check(concated.shift(), l.size + r.tail_offset()));
                r.size += l.size;
                r.shift = concated.shift();
                r.root  = concated.node();
            } else {
                auto tail_offst = l.tail_offset();
                auto tail_size  = l.size - tail_offst;
                auto concated   = concat_trees(l.tail, tail_size,
                                               r.root, r.shift, r.tail_offset());
                r = { l.size + r.size, concated.shift(),
                      concated.node(), r.tail->inc() };
                return;
            }
        } else {
            if (supports_transient_concat) {
                auto tail_offst = l.tail_offset();
                auto tail_size  = l.size - tail_offst;
                auto concated   = concat_trees_mut(
                    er,
                    MemoryPolicy::transience_t::noone,
                    l.root, l.shift, tail_offst, l.tail, tail_size,
                    er, r.root, r.shift, r.tail_offset());
                assert(concated.shift() == concated.node()->compute_shift());
                assert(concated.node()->check(concated.shift(), l.size + r.tail_offset()));
                r.size += l.size;
                r.shift = concated.shift();
                r.root  = concated.node();
                return;
            } else {
                auto tail_offst = l.tail_offset();
                auto tail_size  = l.size - tail_offst;
                auto concated   = concat_trees(l.root, l.shift, tail_offst,
                                               l.tail, tail_size,
                                               r.root, r.shift, r.tail_offset());
                r = { l.size + r.size, concated.shift(),
                      concated.node(), r.tail->inc() };
                return;
            }
        }
    }

    friend void concat_mut_lr_l(rrbtree& l, edit_t el, rrbtree& r, edit_t er)
    {
        assert(&l != &r);
        assert(r.size < (std::numeric_limits<size_t>::max() - l.size));
        using std::get;
        if (l.size == 0)
            l = r;
        else if (r.size == 0)
            return;
        else if (r.tail_offset() == 0) {
            // just concat the tail, similar to push_back
            auto tail_offst = l.tail_offset();
            auto tail_size  = l.size - tail_offst;
            if (tail_size == branches<BL>) {
                l.push_tail_mut(el, tail_offst, l.tail, tail_size);
                l.tail = r.tail->inc();
                l.size += r.size;
                return;
            } else if (tail_size + r.size <= branches<BL>) {
                l.ensure_mutable_tail(el, tail_size);
                if (r.tail->can_mutate(er))
                    uninitialized_move(r.tail->leaf(),
                                       r.tail->leaf() + r.size,
                                       l.tail->leaf() + tail_size);
                else
                    std::uninitialized_copy(r.tail->leaf(),
                                            r.tail->leaf() + r.size,
                                            l.tail->leaf() + tail_size);
                l.size += r.size;
                return;
            } else {
                auto remaining = branches<BL> - tail_size;
                l.ensure_mutable_tail(el, tail_size);
                if (r.tail->can_mutate(er))
                    uninitialized_move(r.tail->leaf(),
                                       r.tail->leaf() + remaining,
                                       l.tail->leaf() + tail_size);
                else
                    std::uninitialized_copy(r.tail->leaf(),
                                            r.tail->leaf() + remaining,
                                            l.tail->leaf() + tail_size);
                try {
                    auto new_tail = node_t::copy_leaf_e(el, r.tail, remaining, r.size);
                    try {
                        l.push_tail_mut(el, tail_offst, l.tail, branches<BL>);
                        l.tail = new_tail;
                        l.size += r.size;
                        return;
                    } catch (...) {
                        node_t::delete_leaf(new_tail, r.size - remaining);
                        throw;
                    }
                } catch (...) {
                    destroy_n(r.tail->leaf() + tail_size, remaining);
                    throw;
                }
            }
        } else if (l.tail_offset() == 0) {
            if (supports_transient_concat) {
                auto tail_offst = l.tail_offset();
                auto tail_size  = l.size - tail_offst;
                auto concated   = concat_trees_mut(
                    el,
                    el, l.tail, tail_size,
                    er, r.root, r.shift, r.tail_offset());
                assert(concated.shift() == concated.node()->compute_shift());
                assert(concated.node()->check(concated.shift(), l.size + r.tail_offset()));
                l.size += r.size;
                l.shift = concated.shift();
                l.root  = concated.node();
                l.tail  = r.tail;
                r.hard_reset();
            } else {
                auto tail_offst = l.tail_offset();
                auto tail_size  = l.size - tail_offst;
                auto concated   = concat_trees(l.tail, tail_size,
                                               r.root, r.shift, r.tail_offset());
                l = { l.size + r.size, concated.shift(),
                      concated.node(), r.tail->inc() };
                return;
            }
        } else {
            if (supports_transient_concat) {
                auto tail_offst = l.tail_offset();
                auto tail_size  = l.size - tail_offst;
                auto concated   = concat_trees_mut(
                    el,
                    el, l.root, l.shift, tail_offst, l.tail, tail_size,
                    er, r.root, r.shift, r.tail_offset());
                assert(concated.shift() == concated.node()->compute_shift());
                assert(concated.node()->check(concated.shift(), l.size + r.tail_offset()));
                l.size += r.size;
                l.shift = concated.shift();
                l.root  = concated.node();
                l.tail  = r.tail;
                r.hard_reset();
            } else {
                auto tail_offst = l.tail_offset();
                auto tail_size  = l.size - tail_offst;
                auto concated   = concat_trees(l.root, l.shift, tail_offst,
                                               l.tail, tail_size,
                                               r.root, r.shift, r.tail_offset());
                l = { l.size + r.size, concated.shift(),
                      concated.node(), r.tail->inc() };
            }
        }
    }

    friend void concat_mut_lr_r(rrbtree& l, edit_t el, rrbtree& r, edit_t er)
    {
        assert(&l != &r);
        assert(r.size < (std::numeric_limits<size_t>::max() - l.size));
        using std::get;
        if (r.size == 0)
            r = l;
        else if (l.size == 0)
            return;
        else if (r.tail_offset() == 0) {
            // just concat the tail, similar to push_back
            auto tail_offst = l.tail_offset();
            auto tail_size  = l.size - tail_offst;
            if (tail_size == branches<BL>) {
                // this could be improved by making sure that the
                // newly created nodes as part of the `push_tail()`
                // are tagged with `er`
                auto res = l.push_tail(l.root, l.shift, tail_offst,
                                       l.tail, tail_size);
                r = { l.size + r.size, get<0>(res), get<1>(res),
                      r.tail->inc() };
                return;
            } else if (tail_size + r.size <= branches<BL>) {
                // doing this in a exception way mutating way is very
                // tricky while potential performance gains are
                // minimal (we need to move every element of the right
                // tail anyways to make space for the left tail)
                //
                // we could however improve this by at least moving the
                // elements of the mutable tails...
                auto new_tail = node_t::copy_leaf(l.tail, tail_size,
                                                  r.tail, r.size);
                r = { l.size + r.size, l.shift, l.root->inc(), new_tail };
                return;
            } else {
                // like the immutable version.
                // we could improve this also by moving elements
                // instead of just copying them
                auto remaining = branches<BL> - tail_size;
                auto add_tail  = node_t::copy_leaf_e(er,
                                                     l.tail, tail_size,
                                                     r.tail, remaining);
                try {
                    auto new_tail = node_t::copy_leaf_e(er, r.tail, remaining, r.size);
                    try {
                        // this could be improved by making sure that the
                        // newly created nodes as part of the `push_tail()`
                        // are tagged with `er`
                        auto new_root = l.push_tail(l.root, l.shift, tail_offst,
                                                    add_tail, branches<BL>);
                        r = { l.size + r.size,
                              get<0>(new_root), get<1>(new_root),
                              new_tail };
                        return;
                    } catch (...) {
                        node_t::delete_leaf(new_tail, r.size - remaining);
                        throw;
                    }
                } catch (...) {
                    node_t::delete_leaf(add_tail, branches<BL>);
                    throw;
                }
                return;
            }
        } else if (l.tail_offset() == 0) {
            if (supports_transient_concat) {
                auto tail_offst = l.tail_offset();
                auto tail_size  = l.size - tail_offst;
                auto concated   = concat_trees_mut(
                    er,
                    el, l.tail, tail_size,
                    er,r.root, r.shift, r.tail_offset());
                assert(concated.shift() == concated.node()->compute_shift());
                assert(concated.node()->check(concated.shift(), l.size + r.tail_offset()));
                r.size += l.size;
                r.shift = concated.shift();
                r.root  = concated.node();
                l.hard_reset();
            } else {
                auto tail_offst = l.tail_offset();
                auto tail_size  = l.size - tail_offst;
                auto concated   = concat_trees(l.tail, tail_size,
                                               r.root, r.shift, r.tail_offset());
                r = { l.size + r.size, concated.shift(),
                      concated.node(), r.tail->inc() };
                return;
            }
        } else {
            if (supports_transient_concat) {
                auto tail_offst = l.tail_offset();
                auto tail_size  = l.size - tail_offst;
                auto concated   = concat_trees_mut(
                    er,
                    el, l.root, l.shift, tail_offst, l.tail, tail_size,
                    er, r.root, r.shift, r.tail_offset());
                assert(concated.shift() == concated.node()->compute_shift());
                assert(concated.node()->check(concated.shift(), l.size + r.tail_offset()));
                r.size += l.size;
                r.shift = concated.shift();
                r.root  = concated.node();
                l.hard_reset();
            } else {
                auto tail_offst = l.tail_offset();
                auto tail_size  = l.size - tail_offst;
                auto concated   = concat_trees(l.root, l.shift, tail_offst,
                                               l.tail, tail_size,
                                               r.root, r.shift, r.tail_offset());
                r = { l.size + r.size, concated.shift(),
                      concated.node(), r.tail->inc() };
            }
        }
    }

    void hard_reset()
    {
        assert(supports_transient_concat);
        size = empty.size;
        shift = empty.shift;
        root = empty.root;
        tail = empty.tail;
    }

    bool check_tree() const
    {
        assert(shift <= sizeof(size_t) * 8 - BL);
        assert(shift >= BL);
        assert(tail_offset() <= size);
        assert(tail_size() <= branches<BL>);
#if IMMER_DEBUG_DEEP_CHECK
        assert(check_root());
        assert(check_tail());
#endif
        return true;
    }

    bool check_tail() const
    {
#if IMMER_DEBUG_DEEP_CHECK
        if (tail_size() > 0)
            assert(tail->check(endshift<B, BL>, tail_size()));
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

#if IMMER_DEBUG_PRINT
    void debug_print() const
    {
        std::cerr
            << "--" << std::endl
            << "{" << std::endl
            << "  size  = " << size << std::endl
            << "  shift = " << shift << std::endl
            << "  root  = " << std::endl;
        debug_print_node(root, shift, tail_offset());
        std::cerr << "  tail  = " << std::endl;
        debug_print_node(tail, endshift<B, BL>, tail_size());
        std::cerr << "}" << std::endl;
    }

    void debug_print_indent(unsigned indent) const
    {
        while (indent --> 0)
            std::cerr << ' ';
    }

    void debug_print_node(node_t* node,
                          shift_t shift,
                          size_t size,
                          unsigned indent = 8) const
    {
        const auto indent_step = 4;

        if (shift == endshift<B, BL>) {
            debug_print_indent(indent);
            std::cerr << "- {" << size << "} "
                      << pretty_print_array(node->leaf(), size)
                      << std::endl;
        } else if (auto r = node->relaxed()) {
            auto count = r->d.count;
            debug_print_indent(indent);
            std::cerr << "# {" << size << "} "
                      << pretty_print_array(r->d.sizes, r->d.count)
                      << std::endl;
            auto last_size = size_t{};
            for (auto i = 0; i < count; ++i) {
                debug_print_node(node->inner()[i],
                                 shift - B,
                                 r->d.sizes[i] - last_size,
                                 indent + indent_step);
                last_size = r->d.sizes[i];
            }
        } else {
            debug_print_indent(indent);
            std::cerr << "+ {" << size << "}" << std::endl;
            auto count = (size >> shift)
                + (size - ((size >> shift) << shift) > 0);
            if (count) {
                for (auto i = 0; i < count - 1; ++i)
                    debug_print_node(node->inner()[i],
                                     shift - B,
                                     1 << shift,
                                     indent + indent_step);
                debug_print_node(node->inner()[count - 1],
                                 shift - B,
                                 size - ((count - 1) << shift),
                                 indent + indent_step);
            }
        }
    }
#endif // IMMER_DEBUG_PRINT
};

template <typename T, typename MP, bits_t B, bits_t BL>
const rrbtree<T, MP, B, BL> rrbtree<T, MP, B, BL>::empty = {
    0,
    BL,
    node_t::make_inner_n(0u),
    node_t::make_leaf_n(0u)
};

} // namespace rbts
} // namespace detail
} // namespace immer
