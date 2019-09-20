//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <immer/config.hpp>
#include <immer/detail/rbts/bits.hpp>

#include <cassert>
#include <utility>
#include <type_traits>

namespace immer {
namespace detail {
namespace rbts {

template <typename Pos>
constexpr auto bits = std::decay_t<Pos>::node_t::bits;

template <typename Pos>
constexpr auto bits_leaf = std::decay_t<Pos>::node_t::bits_leaf;

template <typename Pos>
using node_type = typename std::decay<Pos>::type::node_t;

template <typename Pos>
using edit_type = typename std::decay<Pos>::type::node_t::edit_t;

template <typename NodeT>
struct empty_regular_pos
{
    using node_t = NodeT;
    node_t* node_;

    count_t count() const { return 0; }
    node_t* node()  const { return node_; }
    shift_t shift() const { return 0; }
    size_t  size()  const { return 0; }

    template <typename Visitor, typename... Args>
    void each(Visitor, Args&&...) {}
    template <typename Visitor, typename... Args>
    bool each_pred(Visitor, Args&&...) { return true; }

    template <typename Visitor, typename... Args>
    decltype(auto) visit(Visitor v, Args&& ...args)
    {
        return Visitor::visit_regular(*this, std::forward<Args>(args)...);
    }
};

template <typename NodeT>
empty_regular_pos<NodeT> make_empty_regular_pos(NodeT* node)
{
    return {node};
}

template <typename NodeT>
struct empty_leaf_pos
{
    using node_t = NodeT;
    node_t* node_;

    count_t count() const { return 0; }
    node_t* node()  const { return node_; }
    shift_t shift() const { return 0; }
    size_t  size()  const { return 0; }

    template <typename Visitor, typename ...Args>
    decltype(auto) visit(Visitor v, Args&& ...args)
    {
        return Visitor::visit_leaf(*this, std::forward<Args>(args)...);
    }
};

template <typename NodeT>
empty_leaf_pos<NodeT> make_empty_leaf_pos(NodeT* node)
{
    assert(node);
    return {node};
}

template <typename NodeT>
struct leaf_pos
{
    static constexpr auto B  = NodeT::bits;
    static constexpr auto BL = NodeT::bits_leaf;

    using node_t = NodeT;
    node_t* node_;
    size_t size_;

    count_t count() const { return index(size_ - 1) + 1; }
    node_t* node()  const { return node_; }
    size_t  size()  const { return size_; }
    shift_t shift() const { return 0; }
    count_t index(size_t idx) const { return idx & mask<BL>; }
    count_t subindex(size_t idx) const { return idx; }

    template <typename Visitor, typename ...Args>
    decltype(auto) visit(Visitor v, Args&& ...args)
    {
        return Visitor::visit_leaf(*this, std::forward<Args>(args)...);
    }
};

template <typename NodeT>
leaf_pos<NodeT> make_leaf_pos(NodeT* node, size_t size)
{
    assert(node);
    assert(size > 0);
    return {node, size};
}

template <typename NodeT>
struct leaf_sub_pos
{
    static constexpr auto B  = NodeT::bits;
    static constexpr auto BL = NodeT::bits_leaf;

    using node_t = NodeT;
    node_t* node_;
    count_t count_;

    count_t count() const { return count_; }
    node_t* node()  const { return node_; }
    size_t  size()  const { return count_; }
    shift_t shift() const { return 0; }
    count_t index(size_t idx) const { return idx & mask<BL>; }
    count_t subindex(size_t idx) const { return idx; }

    template <typename Visitor, typename ...Args>
    decltype(auto) visit(Visitor v, Args&& ...args)
    {
        return Visitor::visit_leaf(*this, std::forward<Args>(args)...);
    }
};

template <typename NodeT>
leaf_sub_pos<NodeT> make_leaf_sub_pos(NodeT* node, count_t count)
{
    assert(node);
    assert(count <= branches<NodeT::bits_leaf>);
    return {node, count};
}

template <typename NodeT>
struct leaf_descent_pos
{
    static constexpr auto B  = NodeT::bits;
    static constexpr auto BL = NodeT::bits_leaf;

    using node_t = NodeT;
    node_t* node_;

    node_t* node()  const { return node_; }
    shift_t shift() const { return 0; }
    count_t index(size_t idx) const { return idx & mask<BL>; }

    template <typename... Args>
    decltype(auto) descend(Args&&...) {}

    template <typename Visitor, typename ...Args>
    decltype(auto) visit(Visitor v, Args&& ...args)
    {
        return Visitor::visit_leaf(*this, std::forward<Args>(args)...);
    }
};

template <typename NodeT>
leaf_descent_pos<NodeT> make_leaf_descent_pos(NodeT* node)
{
    assert(node);
    return {node};
}

template <typename NodeT>
struct full_leaf_pos
{
    static constexpr auto B  = NodeT::bits;
    static constexpr auto BL = NodeT::bits_leaf;

    using node_t = NodeT;
    node_t* node_;

    count_t count() const { return branches<BL>; }
    node_t* node()  const { return node_; }
    size_t  size()  const { return branches<BL>; }
    shift_t shift() const { return 0; }
    count_t index(size_t idx) const { return idx & mask<BL>; }
    count_t subindex(size_t idx) const { return idx; }

    template <typename Visitor, typename ...Args>
    decltype(auto) visit(Visitor v, Args&& ...args)
    {
        return Visitor::visit_leaf(*this, std::forward<Args>(args)...);
    }
};

template <typename NodeT>
full_leaf_pos<NodeT> make_full_leaf_pos(NodeT* node)
{
    assert(node);
    return {node};
}

template <typename NodeT>
struct regular_pos
{
    static constexpr auto B  = NodeT::bits;
    static constexpr auto BL = NodeT::bits_leaf;

    using node_t = NodeT;
    node_t* node_;
    shift_t shift_;
    size_t size_;

    count_t count() const { return index(size_ - 1) + 1; }
    node_t* node()  const { return node_; }
    size_t  size()  const { return size_; }
    shift_t shift() const { return shift_; }
    count_t index(size_t idx) const { return (idx >> shift_) & mask<B>; }
    count_t subindex(size_t idx) const { return idx >> shift_; }
    size_t  this_size() const { return ((size_ - 1) & ~(~size_t{} << (shift_ + B))) + 1; }

    template <typename Visitor, typename... Args>
    void each(Visitor v, Args&&... args)
    { return each_regular(*this, v, args...); }

    template <typename Visitor, typename... Args>
    bool each_pred(Visitor v, Args&&... args)
    { return each_pred_regular(*this, v, args...); }

    template <typename Visitor, typename... Args>
    bool each_pred_zip(Visitor v, node_t* other, Args&&... args)
    { return each_pred_zip_regular(*this, v, other, args...); }

    template <typename Visitor, typename... Args>
    bool each_pred_i(Visitor v, count_t i, count_t n, Args&&... args)
    { return each_pred_i_regular(*this, v, i, n, args...); }

    template <typename Visitor, typename... Args>
    bool each_pred_right(Visitor v, count_t start, Args&&... args)
    { return each_pred_right_regular(*this, v, start, args...); }

    template <typename Visitor, typename... Args>
    bool each_pred_left(Visitor v, count_t n, Args&&... args)
    { return each_pred_left_regular(*this, v, n, args...); }

    template <typename Visitor, typename... Args>
    void each_i(Visitor v, count_t i, count_t n, Args&&... args)
    { return each_i_regular(*this, v, i, n, args...); }

    template <typename Visitor, typename... Args>
    void each_right(Visitor v, count_t start, Args&&... args)
    { return each_right_regular(*this, v, start, args...); }

    template <typename Visitor, typename... Args>
    void each_left(Visitor v, count_t n, Args&&... args)
    { return each_left_regular(*this, v, n, args...); }

    template <typename Visitor, typename... Args>
    decltype(auto) towards(Visitor v, size_t idx, Args&&... args)
    { return towards_oh_ch_regular(*this, v, idx, index(idx), count(), args...); }

    template <typename Visitor, typename... Args>
    decltype(auto) towards_oh(Visitor v, size_t idx,
                              count_t offset_hint,
                              Args&&... args)
    { return towards_oh_ch_regular(*this, v, idx, offset_hint, count(), args...); }

    template <typename Visitor, typename... Args>
    decltype(auto) towards_oh_ch(Visitor v, size_t idx,
                                 count_t offset_hint,
                                 count_t count_hint,
                                 Args&&... args)
    { return towards_oh_ch_regular(*this, v, idx, offset_hint, count(), args...); }

    template <typename Visitor, typename... Args>
    decltype(auto) towards_sub_oh(Visitor v, size_t idx,
                                  count_t offset_hint,
                                  Args&&... args)
    { return towards_sub_oh_regular(*this, v, idx, offset_hint, args...); }

    template <typename Visitor, typename... Args>
    decltype(auto) last_oh(Visitor v, count_t offset_hint, Args&&... args)
    { return last_oh_regular(*this, v, offset_hint, args...); }

    template <typename Visitor, typename ...Args>
    decltype(auto) visit(Visitor v, Args&& ...args)
    {
        return Visitor::visit_regular(*this, std::forward<Args>(args)...);
    }
};

template <typename Pos, typename Visitor, typename... Args>
void each_regular(Pos&& p, Visitor v, Args&&... args)
{
    constexpr auto B  = bits<Pos>;
    constexpr auto BL = bits_leaf<Pos>;
    auto n = p.node()->inner();
    auto last = p.count() - 1;
    auto e = n + last;
    if (p.shift() == BL) {
        for (; n != e; ++n) {
            IMMER_PREFETCH(n + 1);
            make_full_leaf_pos(*n).visit(v, args...);
        }
        make_leaf_pos(*n, p.size()).visit(v, args...);
    } else {
        auto ss = p.shift() - B;
        for (; n != e; ++n)
            make_full_pos(*n, ss).visit(v, args...);
        make_regular_pos(*n, ss, p.size()).visit(v, args...);
    }
}

template <typename Pos, typename Visitor, typename... Args>
bool each_pred_regular(Pos&& p, Visitor v, Args&&... args)
{
    constexpr auto B  = bits<Pos>;
    constexpr auto BL = bits_leaf<Pos>;
    auto n = p.node()->inner();
    auto last = p.count() - 1;
    auto e = n + last;
    if (p.shift() == BL) {
        for (; n != e; ++n) {
            IMMER_PREFETCH(n + 1);
            if (!make_full_leaf_pos(*n).visit(v, args...))
                return false;
        }
        return make_leaf_pos(*n, p.size()).visit(v, args...);
    } else {
        auto ss = p.shift() - B;
        for (; n != e; ++n)
            if (!make_full_pos(*n, ss).visit(v, args...))
                return false;
        return make_regular_pos(*n, ss, p.size()).visit(v, args...);
    }
}

template <typename Pos, typename Visitor, typename... Args>
bool each_pred_zip_regular(Pos&& p, Visitor v, node_type<Pos>* other, Args&&... args)
{
    constexpr auto B  = bits<Pos>;
    constexpr auto BL = bits_leaf<Pos>;

    auto n = p.node()->inner();
    auto n2 = other->inner();
    auto last = p.count() - 1;
    auto e = n + last;
    if (p.shift() == BL) {
        for (; n != e; ++n, ++n2) {
            IMMER_PREFETCH(n + 1);
            IMMER_PREFETCH(n2 + 1);
            if (!make_full_leaf_pos(*n).visit(v, *n2, args...))
                return false;
        }
        return make_leaf_pos(*n, p.size()).visit(v, *n2, args...);
    } else {
        auto ss = p.shift() - B;
        for (; n != e; ++n, ++n2)
            if (!make_full_pos(*n, ss).visit(v, *n2, args...))
                return false;
        return make_regular_pos(*n, ss, p.size()).visit(v, *n2, args...);
    }
}

template <typename Pos, typename Visitor, typename... Args>
bool each_pred_i_regular(Pos&& p, Visitor v, count_t f, count_t l, Args&&... args)
{
    constexpr auto B  = bits<Pos>;
    constexpr auto BL = bits_leaf<Pos>;

    if (p.shift() == BL) {
        if (l > f) {
            if (l < p.count()) {
                auto n = p.node()->inner() + f;
                auto e = p.node()->inner() + l;
                for (; n < e; ++n) {
                    IMMER_PREFETCH(n + 1);
                    if (!make_full_leaf_pos(*n).visit(v, args...))
                        return false;
                }
            } else {
                auto n = p.node()->inner() + f;
                auto e = p.node()->inner() + l - 1;
                for (; n < e; ++n) {
                    IMMER_PREFETCH(n + 1);
                    if (!make_full_leaf_pos(*n).visit(v, args...))
                        return false;
                }
                if (!make_leaf_pos(*n, p.size()).visit(v, args...))
                    return false;
            }
        }
    } else {
        if (l > f) {
            auto ss = p.shift() - B;
            if (l < p.count()) {
                auto n = p.node()->inner() + f;
                auto e = p.node()->inner() + l;
                for (; n < e; ++n)
                    if (!make_full_pos(*n, ss).visit(v, args...))
                        return false;
            } else {
                auto n = p.node()->inner() + f;
                auto e = p.node()->inner() + l - 1;
                for (; n < e; ++n)
                    if (!make_full_pos(*n, ss).visit(v, args...))
                        return false;
                if (!make_regular_pos(*n, ss, p.size()).visit(v, args...))
                    return false;
            }
        }
    }
    return true;
}

template <typename Pos, typename Visitor, typename... Args>
bool each_pred_left_regular(Pos&& p, Visitor v, count_t last, Args&&... args)
{
    constexpr auto B  = bits<Pos>;
    constexpr auto BL = bits_leaf<Pos>;
    assert(last < p.count());
    if (p.shift() == BL) {
        auto n = p.node()->inner();
        auto e = n + last;
        for (; n != e; ++n) {
            IMMER_PREFETCH(n + 1);
            if (!make_full_leaf_pos(*n).visit(v, args...))
                return false;
        }
    } else {
        auto n = p.node()->inner();
        auto e = n + last;
        auto ss = p.shift() - B;
        for (; n != e; ++n)
            if (!make_full_pos(*n, ss).visit(v, args...))
                return false;
    }
    return true;
}

template <typename Pos, typename Visitor, typename... Args>
bool each_pred_right_regular(Pos&& p, Visitor v, count_t start, Args&&... args)
{
    constexpr auto B  = bits<Pos>;
    constexpr auto BL = bits_leaf<Pos>;

    if (p.shift() == BL) {
        auto n = p.node()->inner() + start;
        auto last = p.count() - 1;
        auto e = p.node()->inner() + last;
        if (n <= e) {
            for (; n != e; ++n) {
                IMMER_PREFETCH(n + 1);
                if (!make_full_leaf_pos(*n).visit(v, args...))
                    return false;
            }
            if (!make_leaf_pos(*n, p.size()).visit(v, args...))
                return false;
        }
    } else {
        auto n = p.node()->inner() + start;
        auto last = p.count() - 1;
        auto e = p.node()->inner() + last;
        auto ss = p.shift() - B;
        if (n <= e) {
            for (; n != e; ++n)
                if (!make_full_pos(*n, ss).visit(v, args...))
                    return false;
            if (!make_regular_pos(*n, ss, p.size()).visit(v, args...))
                return false;
        }
    }
    return true;
}

template <typename Pos, typename Visitor, typename... Args>
void each_i_regular(Pos&& p, Visitor v, count_t f, count_t l, Args&&... args)
{
    constexpr auto B  = bits<Pos>;
    constexpr auto BL = bits_leaf<Pos>;

    if (p.shift() == BL) {
        if (l > f) {
            if (l < p.count()) {
                auto n = p.node()->inner() + f;
                auto e = p.node()->inner() + l;
                for (; n < e; ++n) {
                    IMMER_PREFETCH(n + 1);
                    make_full_leaf_pos(*n).visit(v, args...);
                }
            } else {
                auto n = p.node()->inner() + f;
                auto e = p.node()->inner() + l - 1;
                for (; n < e; ++n) {
                    IMMER_PREFETCH(n + 1);
                    make_full_leaf_pos(*n).visit(v, args...);
                }
                make_leaf_pos(*n, p.size()).visit(v, args...);
            }
        }
    } else {
        if (l > f) {
            auto ss = p.shift() - B;
            if (l < p.count()) {
                auto n = p.node()->inner() + f;
                auto e = p.node()->inner() + l;
                for (; n < e; ++n)
                    make_full_pos(*n, ss).visit(v, args...);
            } else {
                auto n = p.node()->inner() + f;
                auto e = p.node()->inner() + l - 1;
                for (; n < e; ++n)
                    make_full_pos(*n, ss).visit(v, args...);
                make_regular_pos(*n, ss, p.size()).visit(v, args...);
            }
        }
    }
}

template <typename Pos, typename Visitor, typename... Args>
void each_left_regular(Pos&& p, Visitor v, count_t last, Args&&... args)
{
    constexpr auto B  = bits<Pos>;
    constexpr auto BL = bits_leaf<Pos>;
    assert(last < p.count());
    if (p.shift() == BL) {
        auto n = p.node()->inner();
        auto e = n + last;
        for (; n != e; ++n) {
            IMMER_PREFETCH(n + 1);
            make_full_leaf_pos(*n).visit(v, args...);
        }
    } else {
        auto n = p.node()->inner();
        auto e = n + last;
        auto ss = p.shift() - B;
        for (; n != e; ++n)
            make_full_pos(*n, ss).visit(v, args...);
    }
}

template <typename Pos, typename Visitor, typename... Args>
void each_right_regular(Pos&& p, Visitor v, count_t start, Args&&... args)
{
    constexpr auto B  = bits<Pos>;
    constexpr auto BL = bits_leaf<Pos>;

    if (p.shift() == BL) {
        auto n = p.node()->inner() + start;
        auto last = p.count() - 1;
        auto e = p.node()->inner() + last;
        if (n <= e) {
            for (; n != e; ++n) {
                IMMER_PREFETCH(n + 1);
                make_full_leaf_pos(*n).visit(v, args...);
            }
            make_leaf_pos(*n, p.size()).visit(v, args...);
        }
    } else {
        auto n = p.node()->inner() + start;
        auto last = p.count() - 1;
        auto e = p.node()->inner() + last;
        auto ss = p.shift() - B;
        if (n <= e) {
            for (; n != e; ++n)
                make_full_pos(*n, ss).visit(v, args...);
            make_regular_pos(*n, ss, p.size()).visit(v, args...);
        }
    }
}

template <typename Pos, typename Visitor, typename... Args>
decltype(auto) towards_oh_ch_regular(Pos&& p, Visitor v, size_t idx,
                                     count_t offset_hint,
                                     count_t count_hint,
                                     Args&&... args)
{
    constexpr auto B  = bits<Pos>;
    constexpr auto BL = bits_leaf<Pos>;
    assert(offset_hint == p.index(idx));
    assert(count_hint  == p.count());
    auto is_leaf = p.shift() == BL;
    auto child   = p.node()->inner() [offset_hint];
    auto is_full = offset_hint + 1 != count_hint;
    return is_full
        ? (is_leaf
           ? make_full_leaf_pos(child).visit(v, idx, args...)
           : make_full_pos(child, p.shift() - B).visit(v, idx, args...))
        : (is_leaf
           ? make_leaf_pos(child, p.size()).visit(v, idx, args...)
           : make_regular_pos(child, p.shift() - B, p.size()).visit(v, idx, args...));
}

template <typename Pos, typename Visitor, typename... Args>
decltype(auto) towards_sub_oh_regular(Pos&& p, Visitor v, size_t idx,
                                      count_t offset_hint,
                                      Args&&... args)
{
    constexpr auto B  = bits<Pos>;
    constexpr auto BL = bits_leaf<Pos>;
    assert(offset_hint == p.index(idx));
    auto is_leaf = p.shift() == BL;
    auto child   = p.node()->inner() [offset_hint];
    auto lsize   = offset_hint << p.shift();
    auto size    = p.this_size();
    auto is_full = (size - lsize) >= (size_t{1} << p.shift());
    return is_full
        ? (is_leaf
           ? make_full_leaf_pos(child).visit(
               v, idx - lsize, args...)
           : make_full_pos(child, p.shift() - B).visit(
               v, idx - lsize, args...))
        : (is_leaf
           ? make_leaf_sub_pos(child, size - lsize).visit(
               v, idx - lsize, args...)
           : make_regular_sub_pos(child, p.shift() - B, size - lsize).visit(
               v, idx - lsize, args...));
}

template <typename Pos, typename Visitor, typename... Args>
decltype(auto) last_oh_regular(Pos&& p, Visitor v,
                               count_t offset_hint,
                               Args&&... args)
{
    assert(offset_hint == p.count() - 1);
    constexpr auto B  = bits<Pos>;
    constexpr auto BL = bits_leaf<Pos>;
    auto child     = p.node()->inner() [offset_hint];
    auto is_leaf   = p.shift() == BL;
    return is_leaf
        ? make_leaf_pos(child, p.size()).visit(v, args...)
        : make_regular_pos(child, p.shift() - B, p.size()).visit(v, args...);
}

template <typename NodeT>
regular_pos<NodeT> make_regular_pos(NodeT* node,
                                    shift_t shift,
                                    size_t size)
{
    assert(node);
    assert(shift >= NodeT::bits_leaf);
    assert(size > 0);
    return {node, shift, size};
}

struct null_sub_pos
{
    auto node() const { return nullptr; }

    template <typename Visitor, typename... Args>
    void each_sub(Visitor, Args&&...) {}
    template <typename Visitor, typename... Args>
    void each_right_sub(Visitor, Args&&...) {}
    template <typename Visitor, typename... Args>
    void each_left_sub(Visitor, Args&&...) {}
    template <typename Visitor, typename... Args>
    void visit(Visitor, Args&&...) {}
};

template <typename NodeT>
struct singleton_regular_sub_pos
{
    // this is a fake regular pos made out of a single child... useful
    // to treat a single leaf node as a whole tree

    static constexpr auto B  = NodeT::bits;
    static constexpr auto BL = NodeT::bits_leaf;

    using node_t = NodeT;
    node_t* leaf_;
    count_t count_;

    count_t count() const { return 1; }
    node_t* node()  const { return nullptr; }
    size_t  size()  const { return count_; }
    shift_t shift() const { return BL; }
    count_t index(size_t idx) const { return 0; }
    count_t subindex(size_t idx) const { return 0; }
    size_t  size_before(count_t offset) const { return 0; }
    size_t  this_size() const { return count_; }
    size_t  size(count_t offset) { return count_; }

    template <typename Visitor, typename... Args>
    void each_left_sub(Visitor v, Args&&... args) {}
    template <typename Visitor, typename... Args>
    void each(Visitor v, Args&&... args) {}

    template <typename Visitor, typename... Args>
    decltype(auto) last_sub(Visitor v, Args&&... args)
    {
        return make_leaf_sub_pos(leaf_, count_).visit(v, args...);
    }

    template <typename Visitor, typename ...Args>
    decltype(auto) visit(Visitor v, Args&& ...args)
    {
        return Visitor::visit_regular(*this, std::forward<Args>(args)...);
    }
};

template <typename NodeT>
auto make_singleton_regular_sub_pos(NodeT* leaf, count_t count)
{
    assert(leaf);
    assert(leaf->kind() == NodeT::kind_t::leaf);
    assert(count > 0);
    return singleton_regular_sub_pos<NodeT>{leaf, count};
}

template <typename NodeT>
struct regular_sub_pos
{
    static constexpr auto B  = NodeT::bits;
    static constexpr auto BL = NodeT::bits_leaf;

    using node_t = NodeT;
    node_t* node_;
    shift_t shift_;
    size_t size_;

    count_t count() const { return subindex(size_ - 1) + 1; }
    node_t* node()  const { return node_; }
    size_t  size()  const { return size_; }
    shift_t shift() const { return shift_; }
    count_t index(size_t idx) const { return (idx >> shift_) & mask<B>; }
    count_t subindex(size_t idx) const { return idx >> shift_; }
    size_t  size_before(count_t offset) const { return offset << shift_; }
    size_t  this_size() const { return size_; }

    auto size(count_t offset)
    {
        return offset == subindex(size_ - 1)
            ? size_ - size_before(offset)
            : 1 << shift_;
    }

    auto size_sbh(count_t offset, size_t size_before_hint)
    {
        assert(size_before_hint == size_before(offset));
        return offset == subindex(size_ - 1)
            ? size_ - size_before_hint
            : 1 << shift_;
    }

    void copy_sizes(count_t offset,
                    count_t n,
                    size_t init,
                    size_t* sizes)
    {
        if (n) {
            auto last = offset + n - 1;
            auto e = sizes + n - 1;
            for (; sizes != e; ++sizes)
                init = *sizes = init + (1 << shift_);
            *sizes = init + size(last);
        }
    }

    template <typename Visitor, typename... Args>
    void each(Visitor v, Args&& ...args)
    { return each_regular(*this, v, args...); }

    template <typename Visitor, typename... Args>
    bool each_pred(Visitor v, Args&& ...args)
    { return each_pred_regular(*this, v, args...); }

    template <typename Visitor, typename... Args>
    bool each_pred_zip(Visitor v, node_t* other, Args&&... args)
    { return each_pred_zip_regular(*this, v, other, args...); }

    template <typename Visitor, typename... Args>
    bool each_pred_i(Visitor v, count_t i, count_t n, Args&& ...args)
    { return each_pred_i_regular(*this, v, i, n, args...); }

    template <typename Visitor, typename... Args>
    bool each_pred_right(Visitor v, count_t start, Args&& ...args)
    { return each_pred_right_regular(*this, v, start, args...); }

    template <typename Visitor, typename... Args>
    bool each_pred_left(Visitor v, count_t last, Args&& ...args)
    { return each_pred_left_regular(*this, v, last, args...); }

    template <typename Visitor, typename... Args>
    void each_i(Visitor v, count_t i, count_t n, Args&& ...args)
    { return each_i_regular(*this, v, i, n, args...); }

    template <typename Visitor, typename... Args>
    void each_right(Visitor v, count_t start, Args&& ...args)
    { return each_right_regular(*this, v, start, args...); }

    template <typename Visitor, typename... Args>
    void each_left(Visitor v, count_t last, Args&& ...args)
    { return each_left_regular(*this, v, last, args...); }

    template <typename Visitor, typename... Args>
    void each_right_sub_(Visitor v, count_t i, Args&& ...args)
    {
        auto last  = count() - 1;
        auto lsize = size_ - (last << shift_);
        auto n = node()->inner() + i;
        auto e = node()->inner() + last;
        if (shift() == BL) {
            for (; n != e; ++n) {
                IMMER_PREFETCH(n + 1);
                make_full_leaf_pos(*n).visit(v, args...);
            }
            make_leaf_sub_pos(*n, lsize).visit(v, args...);
        } else {
            auto ss = shift_ - B;
            for (; n != e; ++n)
                make_full_pos(*n, ss).visit(v, args...);
            make_regular_sub_pos(*n, ss, lsize).visit(v, args...);
        }
    }

    template <typename Visitor, typename... Args>
    void each_sub(Visitor v, Args&& ...args)
    { each_right_sub_(v, 0, args...); }

    template <typename Visitor, typename... Args>
    void each_right_sub(Visitor v, Args&& ...args)
    { if (count() > 1) each_right_sub_(v, 1, args...); }

    template <typename Visitor, typename... Args>
    void each_left_sub(Visitor v, Args&& ...args)
    { each_left(v, count() - 1, args...); }

    template <typename Visitor, typename... Args>
    decltype(auto) towards(Visitor v, size_t idx, Args&&... args)
    { return towards_oh_ch_regular(*this, v, idx, index(idx), count(), args...); }

    template <typename Visitor, typename... Args>
    decltype(auto) towards_oh(Visitor v, size_t idx,
                              count_t offset_hint,
                              Args&&... args)
    { return towards_oh_ch_regular(*this, v, idx, offset_hint, count(), args...); }

    template <typename Visitor, typename... Args>
    decltype(auto) towards_oh_ch(Visitor v, size_t idx,
                                 count_t offset_hint,
                                 count_t count_hint,
                                 Args&&... args)
    { return towards_oh_ch_regular(*this, v, idx, offset_hint, count(), args...); }

    template <typename Visitor, typename... Args>
    decltype(auto) towards_sub_oh(Visitor v, size_t idx,
                                  count_t offset_hint,
                                  Args&& ...args)
    { return towards_sub_oh_regular(*this, v, idx, offset_hint, args...); }

    template <typename Visitor, typename... Args>
    decltype(auto) last_oh(Visitor v, count_t offset_hint, Args&&... args)
    { return last_oh_regular(*this, v, offset_hint, args...); }

    template <typename Visitor, typename... Args>
    decltype(auto) last_sub(Visitor v, Args&&... args)
    {
        auto offset    = count() - 1;
        auto child     = node_->inner() [offset];
        auto is_leaf   = shift_ == BL;
        auto lsize     = size_ - (offset << shift_);
        return is_leaf
            ? make_leaf_sub_pos(child, lsize).visit(v, args...)
            : make_regular_sub_pos(child, shift_ - B, lsize).visit(v, args...);
    }

    template <typename Visitor, typename... Args>
    decltype(auto) first_sub(Visitor v, Args&&... args)
    {
        auto is_leaf = shift_ == BL;
        auto child   = node_->inner() [0];
        auto is_full = size_ >= (size_t{1} << shift_);
        return is_full
            ? (is_leaf
               ? make_full_leaf_pos(child).visit(v, args...)
               : make_full_pos(child, shift_ - B).visit(v, args...))
            : (is_leaf
               ? make_leaf_sub_pos(child, size_).visit(v, args...)
               : make_regular_sub_pos(child, shift_ - B, size_).visit(v, args...));
    }

    template <typename Visitor, typename... Args>
    decltype(auto) first_sub_leaf(Visitor v, Args&&... args)
    {
        assert(shift_ == BL);
        auto child   = node_->inner() [0];
        auto is_full = size_ >= branches<BL>;
        return is_full
            ? make_full_leaf_pos(child).visit(v, args...)
            : make_leaf_sub_pos(child, size_).visit(v, args...);
    }

    template <typename Visitor, typename... Args>
    decltype(auto) first_sub_inner(Visitor v, Args&&... args)
    {
        assert(shift_ >= BL);
        auto child   = node_->inner() [0];
        auto is_full = size_ >= branches<BL>;
        return is_full
            ? make_full_pos(child, shift_ - B).visit(v, args...)
            : make_regular_sub_pos(child, shift_ - B, size_).visit(v, args...);
    }

    template <typename Visitor, typename... Args>
    decltype(auto) nth_sub(count_t idx, Visitor v, Args&&... args)
    {
        assert(idx < count());
        auto is_leaf = shift_ == BL;
        auto child   = node_->inner() [idx];
        auto lsize   = size(idx);
        auto is_full = idx + 1 < count();
        return is_full
            ? (is_leaf
               ? make_full_leaf_pos(child).visit(v, args...)
               : make_full_pos(child, shift_ - B).visit(v, args...))
            : (is_leaf
               ? make_leaf_sub_pos(child, lsize).visit(v, args...)
               : make_regular_sub_pos(child, shift_ - B, lsize).visit(v, args...));
    }

    template <typename Visitor, typename... Args>
    decltype(auto) nth_sub_leaf(count_t idx, Visitor v, Args&&... args)
    {
        assert(shift_ == BL);
        auto child   = node_->inner() [idx];
        auto lsize   = size(idx);
        auto is_full = idx + 1 < count();
        return is_full
            ? make_full_leaf_pos(child).visit(v, args...)
            : make_leaf_sub_pos(child, lsize).visit(v, args...);
    }

    template <typename Visitor, typename ...Args>
    decltype(auto) visit(Visitor v, Args&& ...args)
    {
        return Visitor::visit_regular(*this, std::forward<Args>(args)...);
    }
};

template <typename NodeT>
regular_sub_pos<NodeT> make_regular_sub_pos(NodeT* node,
                                            shift_t shift,
                                            size_t size)
{
    assert(node);
    assert(shift >= NodeT::bits_leaf);
    assert(size > 0);
    assert(size <= (branches<NodeT::bits, size_t> << shift));
    return {node, shift, size};
}

template <typename NodeT, shift_t Shift,
          bits_t B  = NodeT::bits,
          bits_t BL = NodeT::bits_leaf>
struct regular_descent_pos
{
    static_assert(Shift > 0, "not leaf...");

    using node_t = NodeT;
    node_t* node_;

    node_t* node()  const { return node_; }
    shift_t shift() const { return Shift; }
    count_t index(size_t idx) const {
#if !defined(_MSC_VER)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshift-count-overflow"
#endif
        return (idx >> Shift) & mask<B>;
#if !defined(_MSC_VER)
#pragma GCC diagnostic pop
#endif
    }

    template <typename Visitor>
    decltype(auto) descend(Visitor v, size_t idx)
    {
        auto offset = index(idx);
        auto child  = node_->inner()[offset];
        return regular_descent_pos<NodeT, Shift - B>{child}.visit(v, idx);
    }

    template <typename Visitor, typename ...Args>
    decltype(auto) visit(Visitor v, Args&& ...args)
    {
        return Visitor::visit_regular(*this, std::forward<Args>(args)...);
    }
};

template <typename NodeT, bits_t B, bits_t BL>
struct regular_descent_pos<NodeT, BL, B, BL>
{
    using node_t = NodeT;
    node_t* node_;

    node_t* node()  const { return node_; }
    shift_t shift() const { return BL; }
    count_t index(size_t idx) const { return (idx >> BL) & mask<B>; }

    template <typename Visitor>
    decltype(auto) descend(Visitor v, size_t idx)
    {
        auto offset = index(idx);
        auto child  = node_->inner()[offset];
        return make_leaf_descent_pos(child).visit(v, idx);
    }

    template <typename Visitor, typename ...Args>
    decltype(auto) visit(Visitor v, Args&& ...args)
    {
        return Visitor::visit_regular(*this, std::forward<Args>(args)...);
    }
};

template <typename NodeT, typename Visitor>
decltype(auto) visit_regular_descent(NodeT* node, shift_t shift, Visitor v,
                                     size_t idx)
{
    constexpr auto B  = NodeT::bits;
    constexpr auto BL = NodeT::bits_leaf;
    assert(node);
    assert(shift >= BL);
    switch (shift) {
    case BL + B * 0: return regular_descent_pos<NodeT, BL + B * 0>{node}.visit(v, idx);
    case BL + B * 1: return regular_descent_pos<NodeT, BL + B * 1>{node}.visit(v, idx);
    case BL + B * 2: return regular_descent_pos<NodeT, BL + B * 2>{node}.visit(v, idx);
    case BL + B * 3: return regular_descent_pos<NodeT, BL + B * 3>{node}.visit(v, idx);
    case BL + B * 4: return regular_descent_pos<NodeT, BL + B * 4>{node}.visit(v, idx);
    case BL + B * 5: return regular_descent_pos<NodeT, BL + B * 5>{node}.visit(v, idx);
#if IMMER_DESCENT_DEEP
    default:
        for (auto level = shift; level != endshift<B, BL>; level -= B)
            node = node->inner() [(idx >> level) & mask<B>];
        return make_leaf_descent_pos(node).visit(v, idx);
#endif // IMMER_DEEP_DESCENT
    }
    IMMER_UNREACHABLE;
}

template <typename NodeT>
struct full_pos
{
    static constexpr auto B  = NodeT::bits;
    static constexpr auto BL = NodeT::bits_leaf;

    using node_t = NodeT;
    node_t* node_;
    shift_t shift_;

    count_t count() const { return branches<B>; }
    node_t* node()  const { return node_; }
    size_t  size()  const { return branches<B> << shift_; }
    shift_t shift() const { return shift_; }
    count_t index(size_t idx) const { return (idx >> shift_) & mask<B>; }
    count_t subindex(size_t idx) const { return idx >> shift_; }
    size_t  size(count_t offset) const { return 1 << shift_; }
    size_t  size_sbh(count_t offset, size_t) const { return 1 << shift_; }
    size_t  size_before(count_t offset) const { return offset << shift_; }

    void copy_sizes(count_t offset,
                    count_t n,
                    size_t init,
                    size_t* sizes)
    {
        auto e = sizes + n;
        for (; sizes != e; ++sizes)
            init = *sizes = init + (1 << shift_);
    }

    template <typename Visitor, typename... Args>
    void each(Visitor v, Args&&... args)
    {
        auto p = node_->inner();
        auto e = p + branches<B>;
        if (shift_ == BL) {
            for (; p != e; ++p) {
                IMMER_PREFETCH(p + 1);
                make_full_leaf_pos(*p).visit(v, args...);
            }
        } else {
            auto ss = shift_ - B;
            for (; p != e; ++p)
                make_full_pos(*p, ss).visit(v, args...);
        }
    }

    template <typename Visitor, typename... Args>
    bool each_pred(Visitor v, Args&&... args)
    {
        auto p = node_->inner();
        auto e = p + branches<B>;
        if (shift_ == BL) {
            for (; p != e; ++p) {
                IMMER_PREFETCH(p + 1);
                if (!make_full_leaf_pos(*p).visit(v, args...))
                    return false;
            }
        } else {
            auto ss = shift_ - B;
            for (; p != e; ++p)
                if (!make_full_pos(*p, ss).visit(v, args...))
                    return false;
        }
        return true;
    }

    template <typename Visitor, typename... Args>
    bool each_pred_zip(Visitor v, node_t* other, Args&&... args)
    {
        auto p = node_->inner();
        auto p2 = other->inner();
        auto e = p + branches<B>;
        if (shift_ == BL) {
            for (; p != e; ++p, ++p2) {
                IMMER_PREFETCH(p + 1);
                if (!make_full_leaf_pos(*p).visit(v, *p2, args...))
                    return false;
            }
        } else {
            auto ss = shift_ - B;
            for (; p != e; ++p, ++p2)
                if (!make_full_pos(*p, ss).visit(v, *p2, args...))
                    return false;
        }
        return true;
    }

    template <typename Visitor, typename... Args>
    bool each_pred_i(Visitor v, count_t i, count_t n, Args&&... args)
    {
        auto p = node_->inner() + i;
        auto e = node_->inner() + n;
        if (shift_ == BL) {
            for (; p != e; ++p) {
                IMMER_PREFETCH(p + 1);
                if (!make_full_leaf_pos(*p).visit(v, args...))
                    return false;
            }
        } else {
            auto ss = shift_ - B;
            for (; p != e; ++p)
                if (!make_full_pos(*p, ss).visit(v, args...))
                    return false;
        }
        return true;
    }

    template <typename Visitor, typename... Args>
    void each_i(Visitor v, count_t i, count_t n, Args&&... args)
    {
        auto p = node_->inner() + i;
        auto e = node_->inner() + n;
        if (shift_ == BL) {
            for (; p != e; ++p) {
                IMMER_PREFETCH(p + 1);
                make_full_leaf_pos(*p).visit(v, args...);
            }
        } else {
            auto ss = shift_ - B;
            for (; p != e; ++p)
                make_full_pos(*p, ss).visit(v, args...);
        }
    }

    template <typename Visitor, typename... Args>
    bool each_pred_right(Visitor v, count_t start, Args&&... args)
    { return each_pred_i(v, start, branches<B>, args...); }

    template <typename Visitor, typename... Args>
    bool each_pred_left(Visitor v, count_t last, Args&&... args)
    { return each_pred_i(v, 0, last, args...); }

    template <typename Visitor, typename... Args>
    void each_sub(Visitor v, Args&&... args)
    { each(v, args...); }

    template <typename Visitor, typename... Args>
    void each_left_sub(Visitor v, Args&&... args)
    { each_i(v, 0, branches<B> - 1, args...); }

    template <typename Visitor, typename... Args>
    void each_right_sub(Visitor v, Args&&... args)
    { each_i(v, 1, branches<B>, args...); }

    template <typename Visitor, typename... Args>
    void each_right(Visitor v, count_t start, Args&&... args)
    { each_i(v, start, branches<B>, args...); }

    template <typename Visitor, typename... Args>
    void each_left(Visitor v, count_t last, Args&&... args)
    { each_i(v, 0, last, args...); }

    template <typename Visitor, typename... Args>
    decltype(auto) towards(Visitor v, size_t idx, Args&&... args)
    { return towards_oh(v, idx, index(idx), args...); }

    template <typename Visitor, typename... Args>
    decltype(auto) towards_oh_ch(Visitor v, size_t idx,
                                 count_t offset_hint, count_t,
                                 Args&&... args)
    { return towards_oh(v, idx, offset_hint, args...); }

    template <typename Visitor, typename... Args>
    decltype(auto) towards_oh(Visitor v, size_t idx,
                              count_t offset_hint,
                              Args&&... args)
    {
        assert(offset_hint == index(idx));
        auto is_leaf = shift_ == BL;
        auto child   = node_->inner() [offset_hint];
        return is_leaf
            ? make_full_leaf_pos(child).visit(v, idx, args...)
            : make_full_pos(child, shift_ - B).visit(v, idx, args...);
    }

    template <typename Visitor, typename... Args>
    decltype(auto) towards_sub_oh(Visitor v, size_t idx,
                                  count_t offset_hint,
                                  Args&&... args)
    {
        assert(offset_hint == index(idx));
        auto is_leaf = shift_ == BL;
        auto child   = node_->inner() [offset_hint];
        auto lsize   = offset_hint << shift_;
        return is_leaf
            ? make_full_leaf_pos(child).visit(v, idx - lsize, args...)
            : make_full_pos(child, shift_ - B).visit(v, idx - lsize, args...);
    }

    template <typename Visitor, typename... Args>
    decltype(auto) first_sub(Visitor v, Args&&... args)
    {
        auto is_leaf = shift_ == BL;
        auto child   = node_->inner() [0];
        return is_leaf
            ? make_full_leaf_pos(child).visit(v, args...)
            : make_full_pos(child, shift_ - B).visit(v, args...);
    }

    template <typename Visitor, typename... Args>
    decltype(auto) first_sub_leaf(Visitor v, Args&&... args)
    {
        assert(shift_ == BL);
        auto child   = node_->inner() [0];
        return make_full_leaf_pos(child).visit(v, args...);
    }

    template <typename Visitor, typename... Args>
    decltype(auto) first_sub_inner(Visitor v, Args&&... args)
    {
        assert(shift_ >= BL);
        auto child   = node_->inner() [0];
        return make_full_pos(child, shift_ - B).visit(v, args...);
    }

    template <typename Visitor, typename... Args>
    decltype(auto) nth_sub(count_t idx, Visitor v, Args&&... args)
    {
        assert(idx < count());
        auto is_leaf = shift_ == BL;
        auto child   = node_->inner() [idx];
        return is_leaf
            ? make_full_leaf_pos(child).visit(v, args...)
            : make_full_pos(child, shift_ - B).visit(v, args...);
    }

    template <typename Visitor, typename... Args>
    decltype(auto) nth_sub_leaf(count_t idx, Visitor v, Args&&... args)
    {
        assert(shift_ == BL);
        assert(idx < count());
        auto child   = node_->inner() [idx];
        return make_full_leaf_pos(child).visit(v, args...);
    }

    template <typename Visitor, typename ...Args>
    decltype(auto) visit(Visitor v, Args&& ...args)
    {
        return Visitor::visit_regular(*this, std::forward<Args>(args)...);
    }
};

template <typename NodeT>
full_pos<NodeT> make_full_pos(NodeT* node, shift_t shift)
{
    assert(node);
    assert(shift >= NodeT::bits_leaf);
    return {node, shift};
}

template <typename NodeT>
struct relaxed_pos
{
    static constexpr auto B  = NodeT::bits;
    static constexpr auto BL = NodeT::bits_leaf;

    using node_t = NodeT;
    using relaxed_t = typename NodeT::relaxed_t;
    node_t* node_;
    shift_t shift_;
    relaxed_t* relaxed_;

    count_t count() const { return relaxed_->d.count; }
    node_t* node()  const { return node_; }
    size_t  size()  const { return relaxed_->d.sizes[relaxed_->d.count - 1]; }
    shift_t shift() const { return shift_; }
    count_t subindex(size_t idx) const { return index(idx); }
    relaxed_t* relaxed() const { return relaxed_; }

    size_t size_before(count_t offset) const
    { return offset ? relaxed_->d.sizes[offset - 1] : 0; }

    size_t size(count_t offset) const
    { return size_sbh(offset, size_before(offset)); }

    size_t size_sbh(count_t offset, size_t size_before_hint) const
    {
        assert(size_before_hint == size_before(offset));
        return relaxed_->d.sizes[offset] - size_before_hint;
    }

    count_t index(size_t idx) const
    {
        auto offset = idx >> shift_;
        while (relaxed_->d.sizes[offset] <= idx) ++offset;
        return offset;
    }

    void copy_sizes(count_t offset,
                    count_t n,
                    size_t init,
                    size_t* sizes)
    {
        auto e = sizes + n;
        auto prev = size_before(offset);
        auto these = relaxed_->d.sizes + offset;
        for (; sizes != e; ++sizes, ++these) {
            auto this_size = *these;
            init = *sizes = init + (this_size - prev);
            prev = this_size;
        }
    }

    template <typename Visitor, typename... Args>
    void each(Visitor v, Args&&... args)
    { each_left(v, relaxed_->d.count, args...); }

    template <typename Visitor, typename... Args>
    bool each_pred(Visitor v, Args&&... args)
    {
        auto p = node_->inner();
        auto s = size_t{};
        auto n = count();
        if (shift_ == BL) {
            for (auto i = count_t{0}; i < n; ++i) {
                IMMER_PREFETCH(p + i + 1);
                if (!make_leaf_sub_pos(p[i], relaxed_->d.sizes[i] - s)
                    .visit(v, args...))
                    return false;
                s = relaxed_->d.sizes[i];
            }
        } else {
            auto ss = shift_ - B;
            for (auto i = count_t{0}; i < n; ++i) {
                if (!visit_maybe_relaxed_sub(p[i], ss, relaxed_->d.sizes[i] - s,
                                             v, args...))
                    return false;
                s = relaxed_->d.sizes[i];
            }
        }
        return true;
    }

    template <typename Visitor, typename... Args>
    bool each_pred_i(Visitor v, count_t i, count_t n, Args&&... args)
    {
        if (shift_ == BL) {
            auto p = node_->inner();
            auto s = i > 0 ? relaxed_->d.sizes[i - 1] : 0;
            for (; i < n; ++i) {
                IMMER_PREFETCH(p + i + 1);
                if (!make_leaf_sub_pos(p[i], relaxed_->d.sizes[i] - s)
                    .visit(v, args...))
                    return false;
                s = relaxed_->d.sizes[i];
            }
        } else {
            auto p = node_->inner();
            auto s = i > 0 ? relaxed_->d.sizes[i - 1] : 0;
            auto ss = shift_ - B;
            for (; i < n; ++i) {
                if (!visit_maybe_relaxed_sub(p[i], ss, relaxed_->d.sizes[i] - s,
                                             v, args...))
                    return false;
                s = relaxed_->d.sizes[i];
            }
        }
        return true;
    }

    template <typename Visitor, typename... Args>
    bool each_pred_left(Visitor v, count_t n, Args&&... args)
    {
        auto p = node_->inner();
        auto s = size_t{};
        if (shift_ == BL) {
            for (auto i = count_t{0}; i < n; ++i) {
                IMMER_PREFETCH(p + i + 1);
                if (!make_leaf_sub_pos(p[i], relaxed_->d.sizes[i] - s)
                    .visit(v, args...))
                    return false;
                s = relaxed_->d.sizes[i];
            }
        } else {
            auto ss = shift_ - B;
            for (auto i = count_t{0}; i < n; ++i) {
                if (!visit_maybe_relaxed_sub(p[i], ss, relaxed_->d.sizes[i] - s,
                                             v, args...))
                    return false;
                s = relaxed_->d.sizes[i];
            }
        }
        return true;
    }

    template <typename Visitor, typename... Args>
    bool each_pred_right(Visitor v, count_t start, Args&&... args)
    {
        assert(start > 0);
        assert(start <= relaxed_->d.count);
        auto s = relaxed_->d.sizes[start - 1];
        auto p = node_->inner();
        if (shift_ == BL) {
            for (auto i = start; i < relaxed_->d.count; ++i) {
                IMMER_PREFETCH(p + i + 1);
                if (!make_leaf_sub_pos(p[i], relaxed_->d.sizes[i] - s)
                    .visit(v, args...))
                    return false;
                s = relaxed_->d.sizes[i];
            }
        } else {
            auto ss = shift_ - B;
            for (auto i = start; i < relaxed_->d.count; ++i) {
                if (!visit_maybe_relaxed_sub(p[i], ss, relaxed_->d.sizes[i] - s,
                                             v, args...))
                    return false;
                s = relaxed_->d.sizes[i];
            }
        }
        return true;
    }

    template <typename Visitor, typename... Args>
    void each_i(Visitor v, count_t i, count_t n, Args&&... args)
    {
        if (shift_ == BL) {
            auto p = node_->inner();
            auto s = i > 0 ? relaxed_->d.sizes[i - 1] : 0;
            for (; i < n; ++i) {
                IMMER_PREFETCH(p + i + 1);
                make_leaf_sub_pos(p[i], relaxed_->d.sizes[i] - s)
                    .visit(v, args...);
                s = relaxed_->d.sizes[i];
            }
        } else {
            auto p = node_->inner();
            auto s = i > 0 ? relaxed_->d.sizes[i - 1] : 0;
            auto ss = shift_ - B;
            for (; i < n; ++i) {
                visit_maybe_relaxed_sub(p[i], ss, relaxed_->d.sizes[i] - s,
                                        v, args...);
                s = relaxed_->d.sizes[i];
            }
        }
    }

    template <typename Visitor, typename... Args>
    void each_sub(Visitor v, Args&&... args)
    { each_left(v, relaxed_->d.count, args...); }

    template <typename Visitor, typename... Args>
    void each_left_sub(Visitor v, Args&&... args)
    { each_left(v, relaxed_->d.count - 1, args...); }

    template <typename Visitor, typename... Args>
    void each_left(Visitor v, count_t n, Args&&... args)
    {
        auto p = node_->inner();
        auto s = size_t{};
        if (shift_ == BL) {
            for (auto i = count_t{0}; i < n; ++i) {
                IMMER_PREFETCH(p + i + 1);
                make_leaf_sub_pos(p[i], relaxed_->d.sizes[i] - s)
                    .visit(v, args...);
                s = relaxed_->d.sizes[i];
            }
        } else {
            auto ss = shift_ - B;
            for (auto i = count_t{0}; i < n; ++i) {
                visit_maybe_relaxed_sub(p[i], ss, relaxed_->d.sizes[i] - s,
                                        v, args...);
                s = relaxed_->d.sizes[i];
            }
        }
    }

    template <typename Visitor, typename... Args>
    void each_right_sub(Visitor v, Args&&... args)
    { each_right(v, 1, std::forward<Args>(args)...); }

    template <typename Visitor, typename... Args>
    void each_right(Visitor v, count_t start, Args&&... args)
    {
        assert(start > 0);
        assert(start <= relaxed_->d.count);
        auto s = relaxed_->d.sizes[start - 1];
        auto p = node_->inner();
        if (shift_ == BL) {
            for (auto i = start; i < relaxed_->d.count; ++i) {
                IMMER_PREFETCH(p + i + 1);
                make_leaf_sub_pos(p[i], relaxed_->d.sizes[i] - s)
                    .visit(v, args...);
                s = relaxed_->d.sizes[i];
            }
        } else {
            auto ss = shift_ - B;
            for (auto i = start; i < relaxed_->d.count; ++i) {
                visit_maybe_relaxed_sub(p[i], ss, relaxed_->d.sizes[i] - s,
                                        v, args...);
                s = relaxed_->d.sizes[i];
            }
        }
    }

    template <typename Visitor, typename... Args>
    decltype(auto) towards(Visitor v, size_t idx, Args&&... args)
    { return towards_oh(v, idx, subindex(idx), args...); }

    template <typename Visitor, typename... Args>
    decltype(auto) towards_oh(Visitor v, size_t idx,
                              count_t offset_hint,
                              Args&&... args)
    {
        assert(offset_hint == index(idx));
        auto left_size = offset_hint ? relaxed_->d.sizes[offset_hint - 1] : 0;
        return towards_oh_sbh(v, idx, offset_hint, left_size, args...);
    }

    template <typename Visitor, typename... Args>
    decltype(auto) towards_oh_sbh(Visitor v, size_t idx,
                                  count_t offset_hint,
                                  size_t left_size_hint,
                                  Args&&... args)
    { return towards_sub_oh_sbh(v, idx, offset_hint, left_size_hint, args...); }

    template <typename Visitor, typename... Args>
    decltype(auto) towards_sub_oh(Visitor v, size_t idx,
                                  count_t offset_hint,
                                  Args&&... args)
    {
        assert(offset_hint == index(idx));
        auto left_size = offset_hint ? relaxed_->d.sizes[offset_hint - 1] : 0;
        return towards_sub_oh_sbh(v, idx, offset_hint, left_size, args...);
    }

    template <typename Visitor, typename... Args>
    decltype(auto) towards_sub_oh_sbh(Visitor v, size_t idx,
                                      count_t offset_hint,
                                      size_t left_size_hint,
                                      Args&&... args)
    {
        assert(offset_hint == index(idx));
        assert(left_size_hint ==
               (offset_hint ? relaxed_->d.sizes[offset_hint - 1] : 0));
        auto child     = node_->inner() [offset_hint];
        auto is_leaf   = shift_ == BL;
        auto next_size = relaxed_->d.sizes[offset_hint] - left_size_hint;
        auto next_idx  = idx - left_size_hint;
        return is_leaf
            ? make_leaf_sub_pos(child, next_size).visit(
                v, next_idx, args...)
            : visit_maybe_relaxed_sub(child, shift_ - B, next_size,
                                      v, next_idx, args...);
    }

    template <typename Visitor, typename... Args>
    decltype(auto) last_oh_csh(Visitor v,
                               count_t offset_hint,
                               size_t child_size_hint,
                               Args&&... args)
    {
        assert(offset_hint == count() - 1);
        assert(child_size_hint == size(offset_hint));
        auto child     = node_->inner() [offset_hint];
        auto is_leaf   = shift_ == BL;
        return is_leaf
            ? make_leaf_sub_pos(child, child_size_hint).visit(v, args...)
            : visit_maybe_relaxed_sub(child, shift_ - B, child_size_hint,
                                      v, args...);
    }

    template <typename Visitor, typename... Args>
    decltype(auto) last_sub(Visitor v, Args&&... args)
    {
        auto offset     = relaxed_->d.count - 1;
        auto child      = node_->inner() [offset];
        auto child_size = size(offset);
        auto is_leaf    = shift_ == BL;
        return is_leaf
            ? make_leaf_sub_pos(child, child_size).visit(v, args...)
            : visit_maybe_relaxed_sub(child, shift_ - B, child_size, v, args...);
    }

    template <typename Visitor, typename... Args>
    decltype(auto) first_sub(Visitor v, Args&&... args)
    {
        auto child      = node_->inner() [0];
        auto child_size = relaxed_->d.sizes[0];
        auto is_leaf    = shift_ == BL;
        return is_leaf
            ? make_leaf_sub_pos(child, child_size).visit(v, args...)
            : visit_maybe_relaxed_sub(child, shift_ - B, child_size, v, args...);
    }

    template <typename Visitor, typename... Args>
    decltype(auto) first_sub_leaf(Visitor v, Args&&... args)
    {
        assert(shift_ == BL);
        auto child      = node_->inner() [0];
        auto child_size = relaxed_->d.sizes[0];
        return make_leaf_sub_pos(child, child_size).visit(v, args...);
    }

    template <typename Visitor, typename... Args>
    decltype(auto) first_sub_inner(Visitor v, Args&&... args)
    {
        assert(shift_ > BL);
        auto child      = node_->inner() [0];
        auto child_size = relaxed_->d.sizes[0];
        return visit_maybe_relaxed_sub(child, shift_ - B, child_size, v, args...);
    }

    template <typename Visitor, typename... Args>
    decltype(auto) nth_sub(count_t offset, Visitor v, Args&&... args)
    {
        auto child      = node_->inner() [offset];
        auto child_size = size(offset);
        auto is_leaf    = shift_ == BL;
        return is_leaf
            ? make_leaf_sub_pos(child, child_size).visit(v, args...)
            : visit_maybe_relaxed_sub(child, shift_ - B, child_size, v, args...);
    }

    template <typename Visitor, typename... Args>
    decltype(auto) nth_sub_leaf(count_t offset, Visitor v, Args&&... args)
    {
        assert(shift_ == BL);
        auto child      = node_->inner() [offset];
        auto child_size = size(offset);
        return make_leaf_sub_pos(child, child_size).visit(v, args...);
    }

    template <typename Visitor, typename ...Args>
    decltype(auto) visit(Visitor v, Args&& ...args)
    {
        return Visitor::visit_relaxed(*this, std::forward<Args>(args)...);
    }
};

template <typename Pos>
using is_relaxed = std::is_same<relaxed_pos<typename std::decay_t<Pos>::node_t>,
                                std::decay_t<Pos>>;

template <typename Pos>
constexpr auto is_relaxed_v = is_relaxed<Pos>::value;

template <typename NodeT>
relaxed_pos<NodeT> make_relaxed_pos(NodeT* node,
                                    shift_t shift,
                                    typename NodeT::relaxed_t* relaxed)
{
    assert(node);
    assert(relaxed);
    assert(shift >= NodeT::bits_leaf);
    return {node, shift, relaxed};
}

template <typename NodeT, typename Visitor, typename... Args>
decltype(auto) visit_maybe_relaxed_sub(NodeT* node, shift_t shift, size_t size,
                                       Visitor v, Args&& ...args)
{
    assert(node);
    auto relaxed = node->relaxed();
    if (relaxed) {
        assert(size == relaxed->d.sizes[relaxed->d.count - 1]);
        return make_relaxed_pos(node, shift, relaxed)
            .visit(v, std::forward<Args>(args)...);
    } else {
        return make_regular_sub_pos(node, shift, size)
            .visit(v, std::forward<Args>(args)...);
    }
}

template <typename NodeT, shift_t Shift,
          bits_t B  = NodeT::bits,
          bits_t BL = NodeT::bits_leaf>
struct relaxed_descent_pos
{
    static_assert(Shift > 0, "not leaf...");

    using node_t = NodeT;
    using relaxed_t = typename NodeT::relaxed_t;
    node_t* node_;
    relaxed_t* relaxed_;

    count_t count() const { return relaxed_->d.count; }
    node_t* node()  const { return node_; }
    shift_t shift() const { return Shift; }
    size_t  size()  const { return relaxed_->d.sizes[relaxed_->d.count - 1]; }

    count_t index(size_t idx) const
    {
        // make gcc happy
#if !defined(_MSC_VER)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshift-count-overflow"
#endif
        auto offset = idx >> Shift;
#if !defined(_MSC_VER)
#pragma GCC diagnostic pop
#endif
        while (relaxed_->d.sizes[offset] <= idx) ++offset;
        return offset;
    }

    template <typename Visitor>
    decltype(auto) descend(Visitor v, size_t idx)
    {
        auto offset    = index(idx);
        auto child     = node_->inner() [offset];
        auto left_size = offset ? relaxed_->d.sizes[offset - 1] : 0;
        auto next_idx  = idx - left_size;
        auto r  = child->relaxed();
        return r
            ? relaxed_descent_pos<NodeT, Shift - B>{child, r}.visit(v, next_idx)
            : regular_descent_pos<NodeT, Shift - B>{child}.visit(v, next_idx);
    }

    template <typename Visitor, typename ...Args>
    decltype(auto) visit(Visitor v, Args&& ...args)
    {
        return Visitor::visit_relaxed(*this, std::forward<Args>(args)...);
    }
};

template <typename NodeT, bits_t B, bits_t BL>
struct relaxed_descent_pos<NodeT, BL, B, BL>
{
    using node_t = NodeT;
    using relaxed_t = typename NodeT::relaxed_t;
    node_t* node_;
    relaxed_t* relaxed_;

    count_t count() const { return relaxed_->d.count; }
    node_t* node()  const { return node_; }
    shift_t shift() const { return BL; }
    size_t  size()  const { return relaxed_->d.sizes[relaxed_->d.count - 1]; }

    count_t index(size_t idx) const
    {
        auto offset = (idx >> BL) & mask<B>;
        while (relaxed_->d.sizes[offset] <= idx) ++offset;
        return offset;
    }

    template <typename Visitor>
    decltype(auto) descend(Visitor v, size_t idx)
    {
        auto offset    = index(idx);
        auto child     = node_->inner() [offset];
        auto left_size = offset ? relaxed_->d.sizes[offset - 1] : 0;
        auto next_idx  = idx - left_size;
        return leaf_descent_pos<NodeT>{child}.visit(v, next_idx);
    }

    template <typename Visitor, typename ...Args>
    decltype(auto) visit(Visitor v, Args&& ...args)
    {
        return Visitor::visit_relaxed(*this, std::forward<Args>(args)...);
    }
};

template <typename NodeT, typename Visitor, typename... Args>
decltype(auto) visit_maybe_relaxed_descent(NodeT* node, shift_t shift,
                                           Visitor v, size_t idx)
{
    constexpr auto B  = NodeT::bits;
    constexpr auto BL = NodeT::bits_leaf;
    assert(node);
    assert(shift >= BL);
    auto r = node->relaxed();
    if (r) {
        switch (shift) {
        case BL + B * 0: return relaxed_descent_pos<NodeT, BL + B * 0>{node, r}.visit(v, idx);
        case BL + B * 1: return relaxed_descent_pos<NodeT, BL + B * 1>{node, r}.visit(v, idx);
        case BL + B * 2: return relaxed_descent_pos<NodeT, BL + B * 2>{node, r}.visit(v, idx);
        case BL + B * 3: return relaxed_descent_pos<NodeT, BL + B * 3>{node, r}.visit(v, idx);
        case BL + B * 4: return relaxed_descent_pos<NodeT, BL + B * 4>{node, r}.visit(v, idx);
        case BL + B * 5: return relaxed_descent_pos<NodeT, BL + B * 5>{node, r}.visit(v, idx);
#if IMMER_DESCENT_DEEP
        default:
            for (auto level = shift; level != endshift<B, BL>; level -= B) {
                auto r = node->relaxed();
                if (r) {
                    auto node_idx = (idx >> level) & mask<B>;
                    while (r->d.sizes[node_idx] <= idx) ++node_idx;
                    if (node_idx) idx -= r->d.sizes[node_idx - 1];
                    node = node->inner() [node_idx];
                } else {
                    do {
                        node = node->inner() [(idx >> level) & mask<B>];
                    } while ((level -= B) != endshift<B, BL>);
                    return make_leaf_descent_pos(node).visit(v, idx);
                }
            }
            return make_leaf_descent_pos(node).visit(v, idx);
#endif // IMMER_DESCENT_DEEP
        }
        IMMER_UNREACHABLE;
    } else {
        return visit_regular_descent(node, shift, v, idx);
    }
}

} // namespace rbts
} // namespace detail
} // namespace immer
