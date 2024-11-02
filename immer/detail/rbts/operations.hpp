//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <algorithm>
#include <memory>
#include <numeric>
#include <utility>

#include <immer/config.hpp>
#include <immer/detail/rbts/position.hpp>
#include <immer/detail/rbts/visitor.hpp>
#include <immer/detail/util.hpp>
#include <immer/heap/tags.hpp>

namespace immer {
namespace detail {
namespace rbts {

template <typename T>
struct array_for_visitor : visitor_base<array_for_visitor<T>>
{
    using this_t = array_for_visitor;

    template <typename PosT>
    static T* visit_inner(PosT&& pos, size_t idx)
    {
        return pos.descend(this_t{}, idx);
    }

    template <typename PosT>
    static T* visit_leaf(PosT&& pos, size_t)
    {
        return pos.node()->leaf();
    }
};

template <typename T>
struct region_for_visitor : visitor_base<region_for_visitor<T>>
{
    using this_t   = region_for_visitor;
    using result_t = std::tuple<T*, size_t, size_t>;

    template <typename PosT>
    static result_t visit_inner(PosT&& pos, size_t idx)
    {
        return pos.towards(this_t{}, idx);
    }

    template <typename PosT>
    static result_t visit_leaf(PosT&& pos, size_t idx)
    {
        return std::make_tuple(pos.node()->leaf(), pos.index(idx), pos.count());
    }
};

template <typename T>
struct get_visitor : visitor_base<get_visitor<T>>
{
    using this_t = get_visitor;

    template <typename PosT>
    static const T& visit_inner(PosT&& pos, size_t idx)
    {
        return pos.descend(this_t{}, idx);
    }

    template <typename PosT>
    static const T& visit_leaf(PosT&& pos, size_t idx)
    {
        return pos.node()->leaf()[pos.index(idx)];
    }
};

struct for_each_chunk_visitor : visitor_base<for_each_chunk_visitor>
{
    using this_t = for_each_chunk_visitor;

    template <typename Pos, typename Fn>
    static void visit_inner(Pos&& pos, Fn&& fn)
    {
        pos.each(this_t{}, fn);
    }

    template <typename Pos, typename Fn>
    static void visit_leaf(Pos&& pos, Fn&& fn)
    {
        auto data = pos.node()->leaf();
        fn(as_const(data), as_const(data) + pos.count());
    }
};

struct for_each_chunk_p_visitor : visitor_base<for_each_chunk_p_visitor>
{
    using this_t = for_each_chunk_p_visitor;

    template <typename Pos, typename Fn>
    static bool visit_inner(Pos&& pos, Fn&& fn)
    {
        return pos.each_pred(this_t{}, fn);
    }

    template <typename Pos, typename Fn>
    static bool visit_leaf(Pos&& pos, Fn&& fn)
    {
        auto data = as_const(pos.node()->leaf());
        return fn(data, data + pos.count());
    }
};

struct for_each_chunk_left_visitor : visitor_base<for_each_chunk_left_visitor>
{
    using this_t = for_each_chunk_left_visitor;

    template <typename Pos, typename Fn>
    static void visit_inner(Pos&& pos, size_t last, Fn&& fn)
    {
        auto l = pos.index(last);
        pos.each_left(for_each_chunk_visitor{}, l, fn);
        pos.towards_oh(this_t{}, last, l, fn);
    }

    template <typename Pos, typename Fn>
    static void visit_leaf(Pos&& pos, size_t last, Fn&& fn)
    {
        auto data = pos.node()->leaf();
        auto l    = pos.index(last);
        fn(data, data + l + 1);
    }
};

struct for_each_chunk_right_visitor : visitor_base<for_each_chunk_right_visitor>
{
    using this_t = for_each_chunk_right_visitor;

    template <typename Pos, typename Fn>
    static void visit_inner(Pos&& pos, size_t first, Fn&& fn)
    {
        auto f = pos.index(first);
        pos.towards_oh(this_t{}, first, f, fn);
        pos.each_right(for_each_chunk_visitor{}, f + 1, fn);
    }

    template <typename Pos, typename Fn>
    static void visit_leaf(Pos&& pos, size_t first, Fn&& fn)
    {
        auto data = pos.node()->leaf();
        auto f    = pos.index(first);
        fn(data + f, data + pos.count());
    }
};

struct for_each_chunk_i_visitor : visitor_base<for_each_chunk_i_visitor>
{
    using this_t = for_each_chunk_i_visitor;

    template <typename Pos, typename Fn>
    static void visit_relaxed(Pos&& pos, size_t first, size_t last, Fn&& fn)
    {
        // we are going towards *two* indices, so we need to do the
        // relaxed as a special case to correct the second index
        if (first < last) {
            auto f = pos.index(first);
            auto l = pos.index(last - 1);
            if (f == l) {
                auto sbh = pos.size_before(f);
                pos.towards_oh_sbh(this_t{}, first, f, sbh, last - sbh, fn);
            } else {
                assert(f < l);
                pos.towards_oh(for_each_chunk_right_visitor{}, first, f, fn);
                pos.each_i(for_each_chunk_visitor{}, f + 1, l, fn);
                pos.towards_oh(for_each_chunk_left_visitor{}, last - 1, l, fn);
            }
        }
    }

    template <typename Pos, typename Fn>
    static void visit_regular(Pos&& pos, size_t first, size_t last, Fn&& fn)
    {
        if (first < last) {
            auto f = pos.index(first);
            auto l = pos.index(last - 1);
            if (f == l)
                pos.towards_oh(this_t{}, first, f, last, fn);
            else {
                assert(f < l);
                pos.towards_oh(for_each_chunk_right_visitor{}, first, f, fn);
                pos.each_i(for_each_chunk_visitor{}, f + 1, l, fn);
                pos.towards_oh(for_each_chunk_left_visitor{}, last - 1, l, fn);
            }
        }
    }

    template <typename Pos, typename Fn>
    static void visit_leaf(Pos&& pos, size_t first, size_t last, Fn&& fn)
    {
        auto data = pos.node()->leaf();
        if (first < last) {
            auto f = pos.index(first);
            auto l = pos.index(last - 1);
            fn(data + f, data + l + 1);
        }
    }
};

struct for_each_chunk_p_left_visitor
    : visitor_base<for_each_chunk_p_left_visitor>
{
    using this_t = for_each_chunk_p_left_visitor;

    template <typename Pos, typename Fn>
    static bool visit_inner(Pos&& pos, size_t last, Fn&& fn)
    {
        auto l = pos.index(last);
        return pos.each_pred_left(for_each_chunk_p_visitor{}, l, fn) &&
               pos.towards_oh(this_t{}, last, l, fn);
    }

    template <typename Pos, typename Fn>
    static bool visit_leaf(Pos&& pos, size_t last, Fn&& fn)
    {
        auto data = pos.node()->leaf();
        auto l    = pos.index(last);
        return fn(data, data + l + 1);
    }
};

struct for_each_chunk_p_right_visitor
    : visitor_base<for_each_chunk_p_right_visitor>
{
    using this_t = for_each_chunk_p_right_visitor;

    template <typename Pos, typename Fn>
    static bool visit_inner(Pos&& pos, size_t first, Fn&& fn)
    {
        auto f = pos.index(first);
        return pos.towards_oh(this_t{}, first, f, fn) &&
               pos.each_pred_right(for_each_chunk_p_visitor{}, f + 1, fn);
    }

    template <typename Pos, typename Fn>
    static bool visit_leaf(Pos&& pos, size_t first, Fn&& fn)
    {
        auto data = pos.node()->leaf();
        auto f    = pos.index(first);
        return fn(data + f, data + pos.count());
    }
};

struct for_each_chunk_p_i_visitor : visitor_base<for_each_chunk_p_i_visitor>
{
    using this_t = for_each_chunk_p_i_visitor;

    template <typename Pos, typename Fn>
    static bool visit_relaxed(Pos&& pos, size_t first, size_t last, Fn&& fn)
    {
        // we are going towards *two* indices, so we need to do the
        // relaxed as a special case to correct the second index
        if (first < last) {
            auto f = pos.index(first);
            auto l = pos.index(last - 1);
            if (f == l) {
                auto sbh = pos.size_before(f);
                return pos.towards_oh_sbh(
                    this_t{}, first, f, sbh, last - sbh, fn);
            } else {
                assert(f < l);
                return pos.towards_oh(
                           for_each_chunk_p_right_visitor{}, first, f, fn) &&
                       pos.each_pred_i(
                           for_each_chunk_p_visitor{}, f + 1, l, fn) &&
                       pos.towards_oh(
                           for_each_chunk_p_left_visitor{}, last - 1, l, fn);
            }
        }
        return true;
    }

    template <typename Pos, typename Fn>
    static bool visit_regular(Pos&& pos, size_t first, size_t last, Fn&& fn)
    {
        if (first < last) {
            auto f = pos.index(first);
            auto l = pos.index(last - 1);
            if (f == l)
                return pos.towards_oh(this_t{}, first, f, last, fn);
            else {
                assert(f < l);
                return pos.towards_oh(
                           for_each_chunk_p_right_visitor{}, first, f, fn) &&
                       pos.each_pred_i(
                           for_each_chunk_p_visitor{}, f + 1, l, fn) &&
                       pos.towards_oh(
                           for_each_chunk_p_left_visitor{}, last - 1, l, fn);
            }
        }
        return true;
    }

    template <typename Pos, typename Fn>
    static bool visit_leaf(Pos&& pos, size_t first, size_t last, Fn&& fn)
    {
        auto data = pos.node()->leaf();
        if (first < last) {
            auto f = pos.index(first);
            auto l = pos.index(last - 1);
            return fn(data + f, data + l + 1);
        }
        return true;
    }
};

struct equals_visitor : visitor_base<equals_visitor>
{
    using this_t = equals_visitor;

    struct this_aux_t : visitor_base<this_aux_t>
    {
        template <typename PosR, typename PosL, typename Iter>
        static bool visit_inner(
            PosR&& posr, count_t i, PosL&& posl, Iter&& first, size_t idx)
        {
            return posl.nth_sub(i, this_t{}, posr, first, idx);
        }

        template <typename PosR, typename PosL, typename Iter>
        static bool visit_leaf(
            PosR&& posr, count_t i, PosL&& posl, Iter&& first, size_t idx)
        {
            return posl.nth_sub_leaf(i, this_t{}, posr, first, idx);
        }
    };

    struct rrb : visitor_base<rrb>
    {
        template <typename PosR, typename Iter, typename Node>
        static bool visit_node(PosR&& posr,
                               Iter&& first,
                               Node* rootl,
                               shift_t shiftl,
                               size_t sizel)
        {
            assert(shiftl <= posr.shift());
            return shiftl == posr.shift()
                       ? visit_maybe_relaxed_sub(rootl,
                                                 shiftl,
                                                 sizel,
                                                 this_t{},
                                                 posr,
                                                 first,
                                                 size_t{})
                       : posr.first_sub_inner(
                             rrb{}, first, rootl, shiftl, sizel);
        }
    };

    template <typename Iter>
    static auto equal_chunk_p(Iter&& iter)
    {
        return [iter](auto f, auto e) mutable {
            if (f == &*iter) {
                iter += e - f;
                return true;
            }
            for (; f != e; ++f, ++iter)
                if (*f != *iter)
                    return false;
            return true;
        };
    }

    template <typename PosL, typename PosR, typename Iter>
    static bool
    visit_relaxed(PosL&& posl, PosR&& posr, Iter&& first, size_t idx)
    {
        auto nl = posl.node();
        auto nr = posr.node();
        if (nl == nr)
            return true;
        auto cl = posl.count();
        auto cr = posr.count();
        assert(cr > 0);
        auto sbr = size_t{};
        auto i   = count_t{};
        auto j   = count_t{};
        for (; i < cl; ++i) {
            auto sbl = posl.size_before(i);
            for (; j + 1 < cr && (sbr = posr.size_before(j)) < sbl; ++j)
                ;
            auto res =
                sbl == sbr
                    ? posr.nth_sub(j, this_aux_t{}, i, posl, first, idx + sbl)
                    : posl.nth_sub(i,
                                   for_each_chunk_p_visitor{},
                                   this_t::equal_chunk_p(first + (idx + sbl)));
            if (!res)
                return false;
        }
        return true;
    }

    template <typename PosL, typename PosR, typename Iter>
    static std::enable_if_t<is_relaxed_v<PosR>, bool>
    visit_regular(PosL&& posl, PosR&& posr, Iter&& first, size_t idx)
    {
        return this_t::visit_relaxed(posl, posr, first, idx);
    }

    template <typename PosL, typename PosR, typename Iter>
    static std::enable_if_t<!is_relaxed_v<PosR>, bool>
    visit_regular(PosL&& posl, PosR&& posr, Iter&& first, size_t idx)
    {
        return posl.count() >= posr.count()
                   ? this_t::visit_regular(posl, posr.node())
                   : this_t::visit_regular(posr, posl.node());
    }

    template <typename PosL, typename PosR, typename Iter>
    static bool visit_leaf(PosL&& posl, PosR&& posr, Iter&& first, size_t idx)
    {
        if (posl.node() == posr.node())
            return true;
        auto cl = posl.count();
        auto cr = posr.count();
        auto mp = std::min(cl, cr);
        return std::equal(posl.node()->leaf(),
                          posl.node()->leaf() + mp,
                          posr.node()->leaf()) &&
               std::equal(posl.node()->leaf() + mp,
                          posl.node()->leaf() + posl.count(),
                          first + (idx + mp));
    }

    template <typename Pos, typename NodeT>
    static bool visit_regular(Pos&& pos, NodeT* other)
    {
        auto node = pos.node();
        return node == other || pos.each_pred_zip(this_t{}, other);
    }

    template <typename Pos, typename NodeT>
    static bool visit_leaf(Pos&& pos, NodeT* other)
    {
        auto node = pos.node();
        return node == other || std::equal(node->leaf(),
                                           node->leaf() + pos.count(),
                                           other->leaf());
    }
};

template <typename NodeT>
struct update_visitor : visitor_base<update_visitor<NodeT>>
{
    using node_t = NodeT;
    using this_t = update_visitor;

    template <typename Pos, typename Fn>
    static node_t* visit_relaxed(Pos&& pos, size_t idx, Fn&& fn)
    {
        auto offset = pos.index(idx);
        auto count  = pos.count();
        auto node   = node_t::make_inner_sr_n(count, pos.relaxed());
        IMMER_TRY {
            auto child = pos.towards_oh(this_t{}, idx, offset, fn);
            node_t::do_copy_inner_replace_sr(
                node, pos.node(), count, offset, child);
            return node;
        }
        IMMER_CATCH (...) {
            node_t::delete_inner_r(node, count);
            IMMER_RETHROW;
        }
    }

    template <typename Pos, typename Fn>
    static node_t* visit_regular(Pos&& pos, size_t idx, Fn&& fn)
    {
        auto offset = pos.index(idx);
        auto count  = pos.count();
        auto node   = node_t::make_inner_n(count);
        IMMER_TRY {
            auto child = pos.towards_oh_ch(this_t{}, idx, offset, count, fn);
            node_t::do_copy_inner_replace(
                node, pos.node(), count, offset, child);
            return node;
        }
        IMMER_CATCH (...) {
            node_t::delete_inner(node, count);
            IMMER_RETHROW;
        }
    }

    template <typename Pos, typename Fn>
    static node_t* visit_leaf(Pos&& pos, size_t idx, Fn&& fn)
    {
        auto offset = pos.index(idx);
        auto node   = node_t::copy_leaf(pos.node(), pos.count());
        IMMER_TRY {
            node->leaf()[offset] =
                std::forward<Fn>(fn)(std::move(node->leaf()[offset]));
            return node;
        }
        IMMER_CATCH (...) {
            node_t::delete_leaf(node, pos.count());
            IMMER_RETHROW;
        }
    }
};

struct dec_visitor : visitor_base<dec_visitor>
{
    using this_t = dec_visitor;

    template <typename Pos>
    static void visit_relaxed(Pos&& p)
    {
        using node_t = node_type<Pos>;
        auto node    = p.node();
        if (node->dec()) {
            p.each(this_t{});
            node_t::delete_inner_r(node, p.count());
        }
    }

    template <typename Pos>
    static void visit_regular(Pos&& p)
    {
        using node_t = node_type<Pos>;
        auto node    = p.node();
        if (node->dec()) {
            p.each(this_t{});
            node_t::delete_inner(node, p.count());
        }
    }

    template <typename Pos>
    static void visit_leaf(Pos&& p)
    {
        using node_t = node_type<Pos>;
        auto node    = p.node();
        if (node->dec()) {
            node_t::delete_leaf(node, p.count());
        }
    }
};

template <typename NodeT>
void dec_leaf(NodeT* node, count_t n)
{
    make_leaf_sub_pos(node, n).visit(dec_visitor{});
}

template <typename NodeT>
void dec_inner(NodeT* node, shift_t shift, size_t size)
{
    visit_maybe_relaxed_sub(node, shift, size, dec_visitor());
}

template <typename NodeT>
void dec_relaxed(NodeT* node, shift_t shift)
{
    make_relaxed_pos(node, shift, node->relaxed()).visit(dec_visitor());
}

template <typename NodeT>
void dec_regular(NodeT* node, shift_t shift, size_t size)
{
    make_regular_pos(node, shift, size).visit(dec_visitor());
}

template <typename NodeT>
void dec_empty_regular(NodeT* node)
{
    make_empty_regular_pos(node).visit(dec_visitor());
}

template <typename NodeT>
struct get_mut_visitor : visitor_base<get_mut_visitor<NodeT>>
{
    using node_t  = NodeT;
    using this_t  = get_mut_visitor;
    using value_t = typename NodeT::value_t;
    using edit_t  = typename NodeT::edit_t;

    template <typename Pos>
    static value_t&
    visit_relaxed(Pos&& pos, size_t idx, edit_t e, node_t** location)
    {
        auto offset = pos.index(idx);
        auto count  = pos.count();
        auto node   = pos.node();
        if (node->can_mutate(e)) {
            return pos.towards_oh(
                this_t{}, idx, offset, e, &node->inner()[offset]);
        } else {
            auto new_node = node_t::copy_inner_sr_e(e, node, count);
            IMMER_TRY {
                auto& res = pos.towards_oh(
                    this_t{}, idx, offset, e, &new_node->inner()[offset]);
                pos.visit(dec_visitor{});
                *location = new_node;
                return res;
            }
            IMMER_CATCH (...) {
                dec_relaxed(new_node, pos.shift());
                IMMER_RETHROW;
            }
        }
    }

    template <typename Pos>
    static value_t&
    visit_regular(Pos&& pos, size_t idx, edit_t e, node_t** location)
    {
        assert(pos.node() == *location);
        auto offset = pos.index(idx);
        auto count  = pos.count();
        auto node   = pos.node();
        if (node->can_mutate(e)) {
            return pos.towards_oh_ch(
                this_t{}, idx, offset, count, e, &node->inner()[offset]);
        } else {
            auto new_node = node_t::copy_inner_e(e, node, count);
            IMMER_TRY {
                auto& res = pos.towards_oh_ch(this_t{},
                                              idx,
                                              offset,
                                              count,
                                              e,
                                              &new_node->inner()[offset]);
                pos.visit(dec_visitor{});
                *location = new_node;
                return res;
            }
            IMMER_CATCH (...) {
                dec_regular(new_node, pos.shift(), pos.size());
                IMMER_RETHROW;
            }
        }
    }

    template <typename Pos>
    static value_t&
    visit_leaf(Pos&& pos, size_t idx, edit_t e, node_t** location)
    {
        assert(pos.node() == *location);
        auto node = pos.node();
        if (node->can_mutate(e)) {
            return node->leaf()[pos.index(idx)];
        } else {
            auto new_node = node_t::copy_leaf_e(e, pos.node(), pos.count());
            pos.visit(dec_visitor{});
            *location = new_node;
            return new_node->leaf()[pos.index(idx)];
        }
    }
};

template <typename NodeT, bool Mutating = true>
struct push_tail_mut_visitor
    : visitor_base<push_tail_mut_visitor<NodeT, Mutating>>
{
    static constexpr auto B  = NodeT::bits;
    static constexpr auto BL = NodeT::bits_leaf;

    using this_t        = push_tail_mut_visitor;
    using this_no_mut_t = push_tail_mut_visitor<NodeT, false>;
    using node_t        = NodeT;
    using edit_t        = typename NodeT::edit_t;

    template <typename Pos>
    static node_t* visit_relaxed(Pos&& pos, edit_t e, node_t* tail, count_t ts)
    {
        auto node     = pos.node();
        auto level    = pos.shift();
        auto idx      = pos.count() - 1;
        auto children = pos.size(idx);
        auto new_idx =
            children == size_t{1} << level || level == BL ? idx + 1 : idx;
        auto new_child = static_cast<node_t*>(nullptr);
        auto mutate    = Mutating && node->can_mutate(e);

        if (new_idx >= branches<B>)
            return nullptr;
        else if (idx == new_idx) {
            new_child =
                mutate ? pos.last_oh_csh(this_t{}, idx, children, e, tail, ts)
                       : pos.last_oh_csh(
                             this_no_mut_t{}, idx, children, e, tail, ts);
            if (!new_child) {
                if (++new_idx < branches<B>)
                    new_child = node_t::make_path_e(e, level - B, tail);
                else
                    return nullptr;
            }
        } else
            new_child = node_t::make_path_e(e, level - B, tail);

        if (mutate) {
            auto count             = new_idx + 1;
            auto relaxed           = node->ensure_mutable_relaxed_n(e, new_idx);
            node->inner()[new_idx] = new_child;
            relaxed->d.sizes[new_idx] = pos.size() + ts;
            relaxed->d.count          = count;
            assert(relaxed->d.sizes[new_idx]);
            return node;
        } else {
            IMMER_TRY {
                auto count    = new_idx + 1;
                auto new_node = node_t::copy_inner_r_e(e, pos.node(), new_idx);
                auto relaxed  = new_node->relaxed();
                new_node->inner()[new_idx] = new_child;
                relaxed->d.sizes[new_idx]  = pos.size() + ts;
                relaxed->d.count           = count;
                assert(relaxed->d.sizes[new_idx]);
                if (Mutating)
                    pos.visit(dec_visitor{});
                return new_node;
            }
            IMMER_CATCH (...) {
                auto shift = pos.shift();
                auto size  = new_idx == idx ? children + ts : ts;
                if (shift > BL) {
                    tail->inc();
                    dec_inner(new_child, shift - B, size);
                }
                IMMER_RETHROW;
            }
        }
    }

    template <typename Pos, typename... Args>
    static node_t* visit_regular(Pos&& pos, edit_t e, node_t* tail, Args&&...)
    {
        assert((pos.size() & mask<BL>) == 0);
        auto node    = pos.node();
        auto idx     = pos.index(pos.size() - 1);
        auto new_idx = pos.index(pos.size() + branches<BL> - 1);
        auto mutate  = Mutating && node->can_mutate(e);
        if (mutate) {
            node->inner()[new_idx] =
                idx == new_idx ? pos.last_oh(this_t{}, idx, e, tail)
                               /* otherwise */
                               : node_t::make_path_e(e, pos.shift() - B, tail);
            return node;
        } else {
            auto new_parent = node_t::make_inner_e(e);
            IMMER_TRY {
                new_parent->inner()[new_idx] =
                    idx == new_idx
                        ? pos.last_oh(this_no_mut_t{}, idx, e, tail)
                        /* otherwise */
                        : node_t::make_path_e(e, pos.shift() - B, tail);
                node_t::do_copy_inner(new_parent, node, new_idx);
                if (Mutating)
                    pos.visit(dec_visitor{});
                return new_parent;
            }
            IMMER_CATCH (...) {
                node_t::delete_inner_e(new_parent);
                IMMER_RETHROW;
            }
        }
    }

    template <typename Pos, typename... Args>
    static node_t* visit_leaf(Pos&& pos, edit_t e, node_t* tail, Args&&...)
    {
        IMMER_UNREACHABLE;
    }
};

template <typename NodeT>
struct push_tail_visitor : visitor_base<push_tail_visitor<NodeT>>
{
    static constexpr auto B  = NodeT::bits;
    static constexpr auto BL = NodeT::bits_leaf;

    using this_t = push_tail_visitor;
    using node_t = NodeT;

    template <typename Pos>
    static node_t* visit_relaxed(Pos&& pos, node_t* tail, count_t ts)
    {
        auto level    = pos.shift();
        auto idx      = pos.count() - 1;
        auto children = pos.size(idx);
        auto new_idx =
            children == size_t{1} << level || level == BL ? idx + 1 : idx;
        auto new_child = static_cast<node_t*>(nullptr);
        if (new_idx >= branches<B>)
            return nullptr;
        else if (idx == new_idx) {
            new_child = pos.last_oh_csh(this_t{}, idx, children, tail, ts);
            if (!new_child) {
                if (++new_idx < branches<B>)
                    new_child = node_t::make_path(level - B, tail);
                else
                    return nullptr;
            }
        } else
            new_child = node_t::make_path(level - B, tail);
        IMMER_TRY {
            auto count = new_idx + 1;
            auto new_parent =
                node_t::copy_inner_r_n(count, pos.node(), new_idx);
            auto new_relaxed              = new_parent->relaxed();
            new_parent->inner()[new_idx]  = new_child;
            new_relaxed->d.sizes[new_idx] = pos.size() + ts;
            new_relaxed->d.count          = count;
            assert(new_relaxed->d.sizes[new_idx]);
            return new_parent;
        }
        IMMER_CATCH (...) {
            auto shift = pos.shift();
            auto size  = new_idx == idx ? children + ts : ts;
            if (shift > BL) {
                tail->inc();
                dec_inner(new_child, shift - B, size);
            }
            IMMER_RETHROW;
        }
    }

    template <typename Pos, typename... Args>
    static node_t* visit_regular(Pos&& pos, node_t* tail, Args&&...)
    {
        assert((pos.size() & mask<BL>) == 0);
        auto idx        = pos.index(pos.size() - 1);
        auto new_idx    = pos.index(pos.size() + branches<BL> - 1);
        auto count      = new_idx + 1;
        auto new_parent = node_t::make_inner_n(count);
        IMMER_TRY {
            new_parent->inner()[new_idx] =
                idx == new_idx ? pos.last_oh(this_t{}, idx, tail)
                               /* otherwise */
                               : node_t::make_path(pos.shift() - B, tail);
        }
        IMMER_CATCH (...) {
            node_t::delete_inner(new_parent, count);
            IMMER_RETHROW;
        }
        return node_t::do_copy_inner(new_parent, pos.node(), new_idx);
    }

    template <typename Pos, typename... Args>
    static node_t* visit_leaf(Pos&& pos, node_t* tail, Args&&...)
    {
        IMMER_UNREACHABLE;
    }
};

struct dec_right_visitor : visitor_base<dec_right_visitor>
{
    using this_t = dec_right_visitor;
    using dec_t  = dec_visitor;

    template <typename Pos>
    static void visit_relaxed(Pos&& p, count_t idx)
    {
        using node_t = node_type<Pos>;
        auto node    = p.node();
        if (node->dec()) {
            p.each_right(dec_t{}, idx);
            node_t::delete_inner_r(node, p.count());
        }
    }

    template <typename Pos>
    static void visit_regular(Pos&& p, count_t idx)
    {
        using node_t = node_type<Pos>;
        auto node    = p.node();
        if (node->dec()) {
            p.each_right(dec_t{}, idx);
            node_t::delete_inner(node, p.count());
        }
    }

    template <typename Pos>
    static void visit_leaf(Pos&& p, count_t idx)
    {
        IMMER_UNREACHABLE;
    }
};

template <typename NodeT, bool Collapse = true, bool Mutating = true>
struct slice_right_mut_visitor
    : visitor_base<slice_right_mut_visitor<NodeT, Collapse, Mutating>>
{
    using node_t = NodeT;
    using this_t = slice_right_mut_visitor;
    using edit_t = typename NodeT::edit_t;

    // returns a new shift, new root, the new tail size and the new tail
    using result_t             = std::tuple<shift_t, NodeT*, count_t, NodeT*>;
    using no_collapse_t        = slice_right_mut_visitor<NodeT, false, true>;
    using no_collapse_no_mut_t = slice_right_mut_visitor<NodeT, false, false>;
    using no_mut_t = slice_right_mut_visitor<NodeT, Collapse, false>;

    static constexpr auto B  = NodeT::bits;
    static constexpr auto BL = NodeT::bits_leaf;

    template <typename PosT>
    static result_t visit_relaxed(PosT&& pos, size_t last, edit_t e)
    {
        auto idx    = pos.index(last);
        auto node   = pos.node();
        auto mutate = Mutating && node->can_mutate(e);
        if (Collapse && idx == 0) {
            auto res = mutate ? pos.towards_oh(this_t{}, last, idx, e)
                              : pos.towards_oh(no_mut_t{}, last, idx, e);
            if (Mutating)
                pos.visit(dec_right_visitor{}, count_t{1});
            return res;
        } else {
            using std::get;
            auto subs =
                mutate ? pos.towards_oh(no_collapse_t{}, last, idx, e)
                       : pos.towards_oh(no_collapse_no_mut_t{}, last, idx, e);
            auto next = get<1>(subs);
            auto ts   = get<2>(subs);
            auto tail = get<3>(subs);
            IMMER_TRY {
                if (next) {
                    if (mutate) {
                        auto nodr = node->ensure_mutable_relaxed_n(e, idx);
                        pos.each_right(dec_visitor{}, idx + 1);
                        node->inner()[idx] = next;
                        nodr->d.sizes[idx] = last + 1 - ts;
                        nodr->d.count      = idx + 1;
                        assert(nodr->d.sizes[idx]);
                        return std::make_tuple(pos.shift(), node, ts, tail);
                    } else {
                        auto newn = node_t::copy_inner_r_e(e, node, idx);
                        auto newr = newn->relaxed();
                        newn->inner()[idx] = next;
                        newr->d.sizes[idx] = last + 1 - ts;
                        newr->d.count      = idx + 1;
                        assert(newr->d.sizes[idx]);
                        if (Mutating)
                            pos.visit(dec_visitor{});
                        return std::make_tuple(pos.shift(), newn, ts, tail);
                    }
                } else if (idx == 0) {
                    if (Mutating)
                        pos.visit(dec_right_visitor{}, count_t{1});
                    return std::make_tuple(pos.shift(), nullptr, ts, tail);
                } else if (Collapse && idx == 1 && pos.shift() > BL) {
                    auto newn = pos.node()->inner()[0];
                    if (!mutate)
                        newn->inc();
                    if (Mutating)
                        pos.visit(dec_right_visitor{}, count_t{2});
                    return std::make_tuple(pos.shift() - B, newn, ts, tail);
                } else {
                    if (mutate) {
                        pos.each_right(dec_visitor{}, idx + 1);
                        node->ensure_mutable_relaxed_n(e, idx)->d.count = idx;
                        return std::make_tuple(pos.shift(), node, ts, tail);
                    } else {
                        auto newn = node_t::copy_inner_r_e(e, node, idx);
                        if (Mutating)
                            pos.visit(dec_visitor{});
                        return std::make_tuple(pos.shift(), newn, ts, tail);
                    }
                }
            }
            IMMER_CATCH (...) {
                assert(!mutate);
                assert(!next || pos.shift() > BL);
                if (next)
                    dec_inner(next,
                              pos.shift() - B,
                              last + 1 - ts - pos.size_before(idx));
                dec_leaf(tail, ts);
                IMMER_RETHROW;
            }
        }
    }

    template <typename PosT>
    static result_t visit_regular(PosT&& pos, size_t last, edit_t e)
    {
        auto idx    = pos.index(last);
        auto node   = pos.node();
        auto mutate = Mutating && node->can_mutate(e);
        if (Collapse && idx == 0) {
            auto res = mutate ? pos.towards_oh(this_t{}, last, idx, e)
                              : pos.towards_oh(no_mut_t{}, last, idx, e);
            if (Mutating)
                pos.visit(dec_right_visitor{}, count_t{1});
            return res;
        } else {
            using std::get;
            auto subs =
                mutate ? pos.towards_oh(no_collapse_t{}, last, idx, e)
                       : pos.towards_oh(no_collapse_no_mut_t{}, last, idx, e);
            auto next = get<1>(subs);
            auto ts   = get<2>(subs);
            auto tail = get<3>(subs);
            IMMER_TRY {
                if (next) {
                    if (mutate) {
                        node->inner()[idx] = next;
                        pos.each_right(dec_visitor{}, idx + 1);
                        return std::make_tuple(pos.shift(), node, ts, tail);
                    } else {
                        auto newn          = node_t::copy_inner_e(e, node, idx);
                        newn->inner()[idx] = next;
                        if (Mutating)
                            pos.visit(dec_visitor{});
                        return std::make_tuple(pos.shift(), newn, ts, tail);
                    }
                } else if (idx == 0) {
                    if (Mutating)
                        pos.visit(dec_right_visitor{}, count_t{1});
                    return std::make_tuple(pos.shift(), nullptr, ts, tail);
                } else if (Collapse && idx == 1 && pos.shift() > BL) {
                    auto newn = pos.node()->inner()[0];
                    if (!mutate)
                        newn->inc();
                    if (Mutating)
                        pos.visit(dec_right_visitor{}, count_t{2});
                    return std::make_tuple(pos.shift() - B, newn, ts, tail);
                } else {
                    if (mutate) {
                        pos.each_right(dec_visitor{}, idx + 1);
                        return std::make_tuple(pos.shift(), node, ts, tail);
                    } else {
                        auto newn = node_t::copy_inner_e(e, node, idx);
                        if (Mutating)
                            pos.visit(dec_visitor{});
                        return std::make_tuple(pos.shift(), newn, ts, tail);
                    }
                }
            }
            IMMER_CATCH (...) {
                assert(!mutate);
                assert(!next || pos.shift() > BL);
                assert(tail);
                if (next)
                    dec_regular(next, pos.shift() - B, last + 1 - ts);
                dec_leaf(tail, ts);
                IMMER_RETHROW;
            }
        }
    }

    template <typename PosT>
    static result_t visit_leaf(PosT&& pos, size_t last, edit_t e)
    {
        auto old_tail_size = pos.count();
        auto new_tail_size = pos.index(last) + 1;
        auto node          = pos.node();
        auto mutate        = Mutating && node->can_mutate(e);
        if (new_tail_size == old_tail_size) {
            if (!Mutating)
                node->inc();
            return std::make_tuple(0, nullptr, new_tail_size, node);
        } else if (mutate) {
            detail::destroy_n(node->leaf() + new_tail_size,
                              old_tail_size - new_tail_size);
            return std::make_tuple(0, nullptr, new_tail_size, node);
        } else {
            auto new_tail = node_t::copy_leaf_e(e, node, new_tail_size);
            if (Mutating)
                pos.visit(dec_visitor{});
            return std::make_tuple(0, nullptr, new_tail_size, new_tail);
        }
    }
};

template <typename NodeT, bool Collapse = true>
struct slice_right_visitor : visitor_base<slice_right_visitor<NodeT, Collapse>>
{
    using node_t = NodeT;
    using this_t = slice_right_visitor;

    // returns a new shift, new root, the new tail size and the new tail
    using result_t      = std::tuple<shift_t, NodeT*, count_t, NodeT*>;
    using no_collapse_t = slice_right_visitor<NodeT, false>;

    static constexpr auto B  = NodeT::bits;
    static constexpr auto BL = NodeT::bits_leaf;

    template <typename PosT>
    static result_t visit_relaxed(PosT&& pos, size_t last)
    {
        auto idx = pos.index(last);
        if (Collapse && idx == 0) {
            return pos.towards_oh(this_t{}, last, idx);
        } else {
            using std::get;
            auto subs = pos.towards_oh(no_collapse_t{}, last, idx);
            auto next = get<1>(subs);
            auto ts   = get<2>(subs);
            auto tail = get<3>(subs);
            IMMER_TRY {
                if (next) {
                    auto count = idx + 1;
                    auto newn  = node_t::copy_inner_r_n(count, pos.node(), idx);
                    auto newr  = newn->relaxed();
                    newn->inner()[idx] = next;
                    newr->d.sizes[idx] = last + 1 - ts;
                    newr->d.count      = count;
                    assert(newr->d.sizes[idx]);
                    return std::make_tuple(pos.shift(), newn, ts, tail);
                } else if (idx == 0) {
                    return std::make_tuple(pos.shift(), nullptr, ts, tail);
                } else if (Collapse && idx == 1 && pos.shift() > BL) {
                    auto newn = pos.node()->inner()[0];
                    return std::make_tuple(
                        pos.shift() - B, newn->inc(), ts, tail);
                } else {
                    auto newn = node_t::copy_inner_r(pos.node(), idx);
                    return std::make_tuple(pos.shift(), newn, ts, tail);
                }
            }
            IMMER_CATCH (...) {
                assert(!next || pos.shift() > BL);
                if (next)
                    dec_inner(next,
                              pos.shift() - B,
                              last + 1 - ts - pos.size_before(idx));
                if (tail)
                    dec_leaf(tail, ts);
                IMMER_RETHROW;
            }
        }
    }

    template <typename PosT>
    static result_t visit_regular(PosT&& pos, size_t last)
    {
        auto idx = pos.index(last);
        if (Collapse && idx == 0) {
            return pos.towards_oh(this_t{}, last, idx);
        } else {
            using std::get;
            auto subs = pos.towards_oh(no_collapse_t{}, last, idx);
            auto next = get<1>(subs);
            auto ts   = get<2>(subs);
            auto tail = get<3>(subs);
            IMMER_TRY {
                if (next) {
                    auto newn = node_t::copy_inner_n(idx + 1, pos.node(), idx);
                    newn->inner()[idx] = next;
                    return std::make_tuple(pos.shift(), newn, ts, tail);
                } else if (idx == 0) {
                    return std::make_tuple(pos.shift(), nullptr, ts, tail);
                } else if (Collapse && idx == 1 && pos.shift() > BL) {
                    auto newn = pos.node()->inner()[0];
                    return std::make_tuple(
                        pos.shift() - B, newn->inc(), ts, tail);
                } else {
                    auto newn = node_t::copy_inner_n(idx, pos.node(), idx);
                    return std::make_tuple(pos.shift(), newn, ts, tail);
                }
            }
            IMMER_CATCH (...) {
                assert(!next || pos.shift() > BL);
                assert(tail);
                if (next)
                    dec_regular(next, pos.shift() - B, last + 1 - ts);
                dec_leaf(tail, ts);
                IMMER_RETHROW;
            }
        }
    }

    template <typename PosT>
    static result_t visit_leaf(PosT&& pos, size_t last)
    {
        auto old_tail_size = pos.count();
        auto new_tail_size = pos.index(last) + 1;
        auto new_tail      = new_tail_size == old_tail_size
                                 ? pos.node()->inc()
                                 : node_t::copy_leaf(pos.node(), new_tail_size);
        return std::make_tuple(0, nullptr, new_tail_size, new_tail);
    }
};

struct dec_left_visitor : visitor_base<dec_left_visitor>
{
    using this_t = dec_left_visitor;
    using dec_t  = dec_visitor;

    template <typename Pos>
    static void visit_relaxed(Pos&& p, count_t idx)
    {
        using node_t = node_type<Pos>;
        auto node    = p.node();
        if (node->dec()) {
            p.each_left(dec_t{}, idx);
            node_t::delete_inner_r(node, p.count());
        }
    }

    template <typename Pos>
    static void visit_regular(Pos&& p, count_t idx)
    {
        using node_t = node_type<Pos>;
        auto node    = p.node();
        if (node->dec()) {
            p.each_left(dec_t{}, idx);
            node_t::delete_inner(node, p.count());
        }
    }

    template <typename Pos>
    static void visit_leaf(Pos&& p, count_t idx)
    {
        IMMER_UNREACHABLE;
    }
};

template <typename NodeT, bool Collapse = true, bool Mutating = true>
struct slice_left_mut_visitor
    : visitor_base<slice_left_mut_visitor<NodeT, Collapse, Mutating>>
{
    using node_t    = NodeT;
    using this_t    = slice_left_mut_visitor;
    using edit_t    = typename NodeT::edit_t;
    using value_t   = typename NodeT::value_t;
    using relaxed_t = typename NodeT::relaxed_t;
    // returns a new shift and new root
    using result_t = std::tuple<shift_t, NodeT*>;

    using no_collapse_t        = slice_left_mut_visitor<NodeT, false, true>;
    using no_collapse_no_mut_t = slice_left_mut_visitor<NodeT, false, false>;
    using no_mut_t             = slice_left_mut_visitor<NodeT, Collapse, false>;

    static constexpr auto B  = NodeT::bits;
    static constexpr auto BL = NodeT::bits_leaf;

    template <typename PosT>
    static result_t visit_relaxed(PosT&& pos, size_t first, edit_t e)
    {
        auto idx                = pos.subindex(first);
        auto count              = pos.count();
        auto node               = pos.node();
        auto mutate             = Mutating && node->can_mutate(e);
        auto left_size          = pos.size_before(idx);
        auto child_size         = pos.size_sbh(idx, left_size);
        auto dropped_size       = first;
        auto child_dropped_size = dropped_size - left_size;
        if (Collapse && pos.shift() > BL && idx == pos.count() - 1) {
            auto r = mutate ? pos.towards_sub_oh(this_t{}, first, idx, e)
                            : pos.towards_sub_oh(no_mut_t{}, first, idx, e);
            if (mutate)
                pos.visit(dec_left_visitor{}, idx);
            else if (Mutating)
                pos.visit(dec_visitor{});
            return r;
        } else {
            using std::get;
            auto newn     = mutate ? (node->ensure_mutable_relaxed(e), node)
                                   : node_t::make_inner_r_e(e);
            auto newr     = newn->relaxed();
            auto newcount = count - idx;
            auto new_child_size = child_size - child_dropped_size;
            IMMER_TRY {
                auto subs =
                    mutate ? pos.towards_sub_oh(no_collapse_t{}, first, idx, e)
                           : pos.towards_sub_oh(
                                 no_collapse_no_mut_t{}, first, idx, e);
                if (mutate)
                    pos.each_left(dec_visitor{}, idx);
                pos.copy_sizes(
                    idx + 1, newcount - 1, new_child_size, newr->d.sizes + 1);
                std::copy(node->inner() + idx + 1,
                          node->inner() + count,
                          newn->inner() + 1);
                newn->inner()[0] = get<1>(subs);
                newr->d.sizes[0] = new_child_size;
                newr->d.count    = newcount;
                assert(new_child_size);
                if (!mutate) {
                    node_t::inc_nodes(newn->inner() + 1, newcount - 1);
                    if (Mutating)
                        pos.visit(dec_visitor{});
                }
                return std::make_tuple(pos.shift(), newn);
            }
            IMMER_CATCH (...) {
                if (!mutate)
                    node_t::delete_inner_r_e(newn);
                IMMER_RETHROW;
            }
        }
    }

    template <typename PosT>
    static result_t visit_regular(PosT&& pos, size_t first, edit_t e)
    {
        auto idx    = pos.subindex(first);
        auto count  = pos.count();
        auto node   = pos.node();
        auto mutate = Mutating
                      // this is more restrictive than actually needed because
                      // it causes the algorithm to also avoid mutating the leaf
                      // in place
                      && !node_t::embed_relaxed && node->can_mutate(e);
        auto left_size          = pos.size_before(idx);
        auto child_size         = pos.size_sbh(idx, left_size);
        auto dropped_size       = first;
        auto child_dropped_size = dropped_size - left_size;
        if (Collapse && pos.shift() > BL && idx == pos.count() - 1) {
            auto r = mutate ? pos.towards_sub_oh(this_t{}, first, idx, e)
                            : pos.towards_sub_oh(no_mut_t{}, first, idx, e);
            if (mutate)
                pos.visit(dec_left_visitor{}, idx);
            else if (Mutating)
                pos.visit(dec_visitor{});
            return r;
        } else {
            using std::get;
            // if possible, we convert the node to a relaxed one simply by
            // allocating a `relaxed_t` size table for it... maybe some of this
            // magic should be moved as a `node<...>` static method...
            auto newcount = count - idx;
            auto newn =
                mutate ? (node->impl.d.data.inner.relaxed = new (
                              node_t::heap::allocate(node_t::max_sizeof_relaxed,
                                                     norefs_tag{})) relaxed_t,
                          node)
                       : node_t::make_inner_r_e(e);
            auto newr = newn->relaxed();
            IMMER_TRY {
                auto subs =
                    mutate ? pos.towards_sub_oh(no_collapse_t{}, first, idx, e)
                           : pos.towards_sub_oh(
                                 no_collapse_no_mut_t{}, first, idx, e);
                if (mutate)
                    pos.each_left(dec_visitor{}, idx);
                newr->d.sizes[0] = child_size - child_dropped_size;
                assert(newr->d.sizes[0]);
                pos.copy_sizes(
                    idx + 1, newcount - 1, newr->d.sizes[0], newr->d.sizes + 1);
                newr->d.count    = newcount;
                newn->inner()[0] = get<1>(subs);
                std::copy(node->inner() + idx + 1,
                          node->inner() + count,
                          newn->inner() + 1);
                if (!mutate) {
                    node_t::inc_nodes(newn->inner() + 1, newcount - 1);
                    if (Mutating)
                        pos.visit(dec_visitor{});
                }
                return std::make_tuple(pos.shift(), newn);
            }
            IMMER_CATCH (...) {
                if (!mutate)
                    node_t::delete_inner_r_e(newn);
                else {
                    // restore the regular node that we were
                    // attempting to relax...
                    node_t::heap::deallocate(node_t::max_sizeof_relaxed,
                                             node->impl.d.data.inner.relaxed);
                    node->impl.d.data.inner.relaxed = nullptr;
                }
                IMMER_RETHROW;
            }
        }
    }

    template <typename PosT>
    static result_t visit_leaf(PosT&& pos, size_t first, edit_t e)
    {
        auto node   = pos.node();
        auto idx    = pos.index(first);
        auto count  = pos.count();
        auto mutate = Mutating &&
                      std::is_nothrow_move_constructible<value_t>::value &&
                      node->can_mutate(e);
        if (mutate) {
            auto data     = node->leaf();
            auto newcount = count - idx;
            std::move(data + idx, data + count, data);
            detail::destroy_n(data + newcount, idx);
            return std::make_tuple(0, node);
        } else {
            auto newn = node_t::copy_leaf_e(e, node, idx, count);
            if (Mutating)
                pos.visit(dec_visitor{});
            return std::make_tuple(0, newn);
        }
    }
};

template <typename NodeT, bool Collapse = true>
struct slice_left_visitor : visitor_base<slice_left_visitor<NodeT, Collapse>>
{
    using node_t = NodeT;
    using this_t = slice_left_visitor;

    // returns a new shift and new root
    using result_t      = std::tuple<shift_t, NodeT*>;
    using no_collapse_t = slice_left_visitor<NodeT, false>;

    static constexpr auto B  = NodeT::bits;
    static constexpr auto BL = NodeT::bits_leaf;

    template <typename PosT>
    static result_t visit_inner(PosT&& pos, size_t first)
    {
        auto idx                = pos.subindex(first);
        auto count              = pos.count();
        auto left_size          = pos.size_before(idx);
        auto child_size         = pos.size_sbh(idx, left_size);
        auto dropped_size       = first;
        auto child_dropped_size = dropped_size - left_size;
        if (Collapse && pos.shift() > BL && idx == pos.count() - 1) {
            return pos.towards_sub_oh(this_t{}, first, idx);
        } else {
            using std::get;
            auto n    = pos.node();
            auto newc = count - idx;
            auto newn = node_t::make_inner_r_n(newc);
            IMMER_TRY {
                auto subs     = pos.towards_sub_oh(no_collapse_t{}, first, idx);
                auto newr     = newn->relaxed();
                newr->d.count = count - idx;
                newr->d.sizes[0] = child_size - child_dropped_size;
                assert(newr->d.sizes[0]);
                pos.copy_sizes(idx + 1,
                               newr->d.count - 1,
                               newr->d.sizes[0],
                               newr->d.sizes + 1);
                assert(newr->d.sizes[newr->d.count - 1] ==
                       pos.size() - dropped_size);
                newn->inner()[0] = get<1>(subs);
                std::copy(n->inner() + idx + 1,
                          n->inner() + count,
                          newn->inner() + 1);
                node_t::inc_nodes(newn->inner() + 1, newr->d.count - 1);
                return std::make_tuple(pos.shift(), newn);
            }
            IMMER_CATCH (...) {
                node_t::delete_inner_r(newn, newc);
                IMMER_RETHROW;
            }
        }
    }

    template <typename PosT>
    static result_t visit_leaf(PosT&& pos, size_t first)
    {
        auto n = node_t::copy_leaf(pos.node(), pos.index(first), pos.count());
        return std::make_tuple(0, n);
    }
};

template <typename Node>
struct concat_center_pos
{
    static constexpr auto B  = Node::bits;
    static constexpr auto BL = Node::bits_leaf;

    static constexpr count_t max_children = 3;

    using node_t = Node;
    using edit_t = typename Node::edit_t;

    shift_t shift_ = 0u;
    count_t count_ = 0u;
    node_t* nodes_[max_children];
    size_t sizes_[max_children];

    auto shift() const { return shift_; }

    concat_center_pos(shift_t s, Node* n0, size_t s0)
        : shift_{s}
        , count_{1}
        , nodes_{n0}
        , sizes_{s0}
    {}

    concat_center_pos(shift_t s, Node* n0, size_t s0, Node* n1, size_t s1)
        : shift_{s}
        , count_{2}
        , nodes_{n0, n1}
        , sizes_{s0, s0 + s1}
    {}

    concat_center_pos(shift_t s,
                      Node* n0,
                      size_t s0,
                      Node* n1,
                      size_t s1,
                      Node* n2,
                      size_t s2)
        : shift_{s}
        , count_{3}
        , nodes_{n0, n1, n2}
        , sizes_{s0, s0 + s1, s0 + s1 + s2}
    {}

    template <typename Visitor, typename... Args>
    void each_sub(Visitor v, Args&&... args)
    {
        if (shift_ == BL) {
            auto s = size_t{};
            for (auto i = count_t{0}; i < count_; ++i) {
                make_leaf_sub_pos(nodes_[i], sizes_[i] - s).visit(v, args...);
                s = sizes_[i];
            }
        } else {
            for (auto i = count_t{0}; i < count_; ++i)
                make_relaxed_pos(nodes_[i], shift_ - B, nodes_[i]->relaxed())
                    .visit(v, args...);
        }
    }

    relaxed_pos<Node> realize() &&
    {
        if (count_ > 1) {
            IMMER_TRY {
                auto result = node_t::make_inner_r_n(count_);
                auto r      = result->relaxed();
                r->d.count  = count_;
                std::copy(nodes_, nodes_ + count_, result->inner());
                std::copy(sizes_, sizes_ + count_, r->d.sizes);
                return {result, shift_, r};
            }
            IMMER_CATCH (...) {
                each_sub(dec_visitor{});
                IMMER_RETHROW;
            }
        } else {
            assert(shift_ >= B + BL);
            return {nodes_[0], shift_ - B, nodes_[0]->relaxed()};
        }
    }

    relaxed_pos<Node> realize_e(edit_t e)
    {
        if (count_ > 1) {
            auto result = node_t::make_inner_r_e(e);
            auto r      = result->relaxed();
            r->d.count  = count_;
            std::copy(nodes_, nodes_ + count_, result->inner());
            std::copy(sizes_, sizes_ + count_, r->d.sizes);
            return {result, shift_, r};
        } else {
            assert(shift_ >= B + BL);
            return {nodes_[0], shift_ - B, nodes_[0]->relaxed()};
        }
    }
};

template <typename Node>
struct concat_merger
{
    using node_t             = Node;
    static constexpr auto B  = Node::bits;
    static constexpr auto BL = Node::bits_leaf;

    using result_t = concat_center_pos<Node>;

    count_t* curr_;
    count_t n_;
    result_t result_;

    concat_merger(shift_t shift, count_t* counts, count_t n)
        : curr_{counts}
        , n_{n}
        , result_{
              shift + B, node_t::make_inner_r_n(std::min(n_, branches<B>)), 0}
    {}

    node_t* to_        = {};
    count_t to_offset_ = {};
    size_t to_size_    = {};

    void add_child(node_t* p, size_t size)
    {
        assert(size);
        ++curr_;
        auto parent  = result_.nodes_[result_.count_ - 1];
        auto relaxed = parent->relaxed();
        if (relaxed->d.count == branches<B>) {
            assert(result_.count_ < result_t::max_children);
            n_ -= branches<B>;
            parent  = node_t::make_inner_r_n(std::min(n_, branches<B>));
            relaxed = parent->relaxed();
            result_.nodes_[result_.count_] = parent;
            result_.sizes_[result_.count_] = result_.sizes_[result_.count_ - 1];
            assert(result_.sizes_[result_.count_]);
            ++result_.count_;
        }
        auto idx = relaxed->d.count++;
        result_.sizes_[result_.count_ - 1] += size;
        assert(result_.sizes_[result_.count_ - 1]);
        relaxed->d.sizes[idx] = size + (idx ? relaxed->d.sizes[idx - 1] : 0);
        assert(relaxed->d.sizes[idx]);
        parent->inner()[idx] = p;
    };

    template <typename Pos>
    void merge_leaf(Pos&& p)
    {
        auto from       = p.node();
        auto from_size  = p.size();
        auto from_count = p.count();
        assert(from_size);
        if (!to_ && *curr_ == from_count) {
            add_child(from, from_size);
            from->inc();
        } else {
            auto from_offset = count_t{};
            auto from_data   = from->leaf();
            do {
                if (!to_) {
                    to_        = node_t::make_leaf_n(*curr_);
                    to_offset_ = 0;
                }
                auto data = to_->leaf();
                auto to_copy =
                    std::min(from_count - from_offset, *curr_ - to_offset_);
                detail::uninitialized_copy(from_data + from_offset,
                                           from_data + from_offset + to_copy,
                                           data + to_offset_);
                to_offset_ += to_copy;
                from_offset += to_copy;
                if (*curr_ == to_offset_) {
                    add_child(to_, to_offset_);
                    to_ = nullptr;
                }
            } while (from_offset != from_count);
        }
    }

    template <typename Pos>
    void merge_inner(Pos&& p)
    {
        auto from       = p.node();
        auto from_size  = p.size();
        auto from_count = p.count();
        assert(from_size);
        if (!to_ && *curr_ == from_count) {
            add_child(from, from_size);
            from->inc();
        } else {
            auto from_offset = count_t{};
            auto from_data   = from->inner();
            do {
                if (!to_) {
                    to_        = node_t::make_inner_r_n(*curr_);
                    to_offset_ = 0;
                    to_size_   = 0;
                }
                auto data = to_->inner();
                auto to_copy =
                    std::min(from_count - from_offset, *curr_ - to_offset_);
                std::copy(from_data + from_offset,
                          from_data + from_offset + to_copy,
                          data + to_offset_);
                node_t::inc_nodes(from_data + from_offset, to_copy);
                auto sizes = to_->relaxed()->d.sizes;
                p.copy_sizes(
                    from_offset, to_copy, to_size_, sizes + to_offset_);
                to_offset_ += to_copy;
                from_offset += to_copy;
                to_size_ = sizes[to_offset_ - 1];
                assert(to_size_);
                if (*curr_ == to_offset_) {
                    to_->relaxed()->d.count = to_offset_;
                    add_child(to_, to_size_);
                    to_ = nullptr;
                }
            } while (from_offset != from_count);
        }
    }

    concat_center_pos<Node> finish() const
    {
        assert(!to_);
        return result_;
    }

    void abort()
    {
        auto shift = result_.shift_ - B;
        if (to_) {
            if (shift == BL)
                node_t::delete_leaf(to_, to_offset_);
            else {
                to_->relaxed()->d.count = to_offset_;
                dec_relaxed(to_, shift - B);
            }
        }
        result_.each_sub(dec_visitor());
    }
};

struct concat_merger_visitor : visitor_base<concat_merger_visitor>
{
    using this_t = concat_merger_visitor;

    template <typename Pos, typename Merger>
    static void visit_inner(Pos&& p, Merger& merger)
    {
        merger.merge_inner(p);
    }

    template <typename Pos, typename Merger>
    static void visit_leaf(Pos&& p, Merger& merger)
    {
        merger.merge_leaf(p);
    }
};

struct concat_rebalance_plan_fill_visitor
    : visitor_base<concat_rebalance_plan_fill_visitor>
{
    using this_t = concat_rebalance_plan_fill_visitor;

    template <typename Pos, typename Plan>
    static void visit_node(Pos&& p, Plan& plan)
    {
        auto count = p.count();
        assert(plan.n < Plan::max_children);
        plan.counts[plan.n++] = count;
        plan.total += count;
    }
};

template <bits_t B, bits_t BL>
struct concat_rebalance_plan
{
    static constexpr auto max_children = 2 * branches<B> + 1;

    count_t counts[max_children];
    count_t n     = 0u;
    count_t total = 0u;

    template <typename LPos, typename CPos, typename RPos>
    void fill(LPos&& lpos, CPos&& cpos, RPos&& rpos)
    {
        assert(n == 0u);
        assert(total == 0u);
        using visitor_t = concat_rebalance_plan_fill_visitor;
        lpos.each_left_sub(visitor_t{}, *this);
        cpos.each_sub(visitor_t{}, *this);
        rpos.each_right_sub(visitor_t{}, *this);
    }

    void shuffle(shift_t shift)
    {
        // gcc seems to not really understand this code... :(
#if !defined(_MSC_VER)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
#endif
        constexpr count_t rrb_extras    = 2;
        constexpr count_t rrb_invariant = 1;
        const auto bits                 = shift == BL ? BL : B;
        const auto branches             = count_t{1} << bits;
        const auto optimal              = ((total - 1) >> bits) + 1;
        count_t i                       = 0;
        while (n >= optimal + rrb_extras) {
            // skip ok nodes
            while (counts[i] > branches - rrb_invariant)
                i++;
            assert(i < n);
            // short node, redistribute
            auto remaining = counts[i];
            do {
                auto next  = counts[i + 1];
                auto count = std::min(remaining + next, branches);
                counts[i]  = count;
                assert(counts[i]);
                remaining += next - count;
                ++i;
            } while (remaining > 0);
            // remove node
            std::move(counts + i + 1, counts + n, counts + i);
            --n;
            --i;
        }
#if !defined(_MSC_VER)
#pragma GCC diagnostic pop
#endif
    }

    template <typename LPos, typename CPos, typename RPos>
    concat_center_pos<node_type<CPos>>
    merge(LPos&& lpos, CPos&& cpos, RPos&& rpos)
    {
        using node_t    = node_type<CPos>;
        using merger_t  = concat_merger<node_t>;
        using visitor_t = concat_merger_visitor;
        auto merger     = merger_t{cpos.shift(), counts, n};
        IMMER_TRY {
            lpos.each_left_sub(visitor_t{}, merger);
            cpos.each_sub(visitor_t{}, merger);
            rpos.each_right_sub(visitor_t{}, merger);
            cpos.each_sub(dec_visitor{});
            return merger.finish();
        }
        IMMER_CATCH (...) {
            merger.abort();
            IMMER_RETHROW;
        }
    }
};

template <typename Node, typename LPos, typename CPos, typename RPos>
concat_center_pos<Node> concat_rebalance(LPos&& lpos, CPos&& cpos, RPos&& rpos)
{
    auto plan = concat_rebalance_plan<Node::bits, Node::bits_leaf>{};
    plan.fill(lpos, cpos, rpos);
    plan.shuffle(cpos.shift());
    IMMER_TRY {
        return plan.merge(lpos, cpos, rpos);
    }
    IMMER_CATCH (...) {
        cpos.each_sub(dec_visitor{});
        IMMER_RETHROW;
    }
}

template <typename Node, typename LPos, typename TPos, typename RPos>
concat_center_pos<Node> concat_leafs(LPos&& lpos, TPos&& tpos, RPos&& rpos)
{
    static_assert(Node::bits >= 2, "");
    assert(lpos.shift() == tpos.shift());
    assert(lpos.shift() == rpos.shift());
    assert(lpos.shift() == 0);
    if (tpos.count() > 0)
        return {
            Node::bits_leaf,
            lpos.node()->inc(),
            lpos.count(),
            tpos.node()->inc(),
            tpos.count(),
            rpos.node()->inc(),
            rpos.count(),
        };
    else
        return {
            Node::bits_leaf,
            lpos.node()->inc(),
            lpos.count(),
            rpos.node()->inc(),
            rpos.count(),
        };
}

template <typename Node>
struct concat_left_visitor;
template <typename Node>
struct concat_right_visitor;
template <typename Node>
struct concat_both_visitor;

template <typename Node, typename LPos, typename TPos, typename RPos>
concat_center_pos<Node> concat_inners(LPos&& lpos, TPos&& tpos, RPos&& rpos)
{
    auto lshift = lpos.shift();
    auto rshift = rpos.shift();
    if (lshift > rshift) {
        auto cpos = lpos.last_sub(concat_left_visitor<Node>{}, tpos, rpos);
        return concat_rebalance<Node>(lpos, cpos, null_sub_pos{});
    } else if (lshift < rshift) {
        auto cpos = rpos.first_sub(concat_right_visitor<Node>{}, lpos, tpos);
        return concat_rebalance<Node>(null_sub_pos{}, cpos, rpos);
    } else {
        assert(lshift == rshift);
        assert(Node::bits_leaf == 0u || lshift > 0);
        auto cpos = lpos.last_sub(concat_both_visitor<Node>{}, tpos, rpos);
        return concat_rebalance<Node>(lpos, cpos, rpos);
    }
}

template <typename Node>
struct concat_left_visitor : visitor_base<concat_left_visitor<Node>>
{
    using this_t = concat_left_visitor;

    template <typename LPos, typename TPos, typename RPos>
    static concat_center_pos<Node>
    visit_inner(LPos&& lpos, TPos&& tpos, RPos&& rpos)
    {
        return concat_inners<Node>(lpos, tpos, rpos);
    }

    template <typename LPos, typename TPos, typename RPos>
    static concat_center_pos<Node>
    visit_leaf(LPos&& lpos, TPos&& tpos, RPos&& rpos)
    {
        IMMER_UNREACHABLE;
    }
};

template <typename Node>
struct concat_right_visitor : visitor_base<concat_right_visitor<Node>>
{
    using this_t = concat_right_visitor;

    template <typename RPos, typename LPos, typename TPos>
    static concat_center_pos<Node>
    visit_inner(RPos&& rpos, LPos&& lpos, TPos&& tpos)
    {
        return concat_inners<Node>(lpos, tpos, rpos);
    }

    template <typename RPos, typename LPos, typename TPos>
    static concat_center_pos<Node>
    visit_leaf(RPos&& rpos, LPos&& lpos, TPos&& tpos)
    {
        return concat_leafs<Node>(lpos, tpos, rpos);
    }
};

template <typename Node>
struct concat_both_visitor : visitor_base<concat_both_visitor<Node>>
{
    using this_t = concat_both_visitor;

    template <typename LPos, typename TPos, typename RPos>
    static concat_center_pos<Node>
    visit_inner(LPos&& lpos, TPos&& tpos, RPos&& rpos)
    {
        return rpos.first_sub(concat_right_visitor<Node>{}, lpos, tpos);
    }

    template <typename LPos, typename TPos, typename RPos>
    static concat_center_pos<Node>
    visit_leaf(LPos&& lpos, TPos&& tpos, RPos&& rpos)
    {
        return rpos.first_sub_leaf(concat_right_visitor<Node>{}, lpos, tpos);
    }
};

template <typename Node>
struct concat_trees_right_visitor
    : visitor_base<concat_trees_right_visitor<Node>>
{
    using this_t = concat_trees_right_visitor;

    template <typename RPos, typename LPos, typename TPos>
    static concat_center_pos<Node>
    visit_node(RPos&& rpos, LPos&& lpos, TPos&& tpos)
    {
        return concat_inners<Node>(lpos, tpos, rpos);
    }
};

template <typename Node>
struct concat_trees_left_visitor : visitor_base<concat_trees_left_visitor<Node>>
{
    using this_t = concat_trees_left_visitor;

    template <typename LPos, typename TPos, typename... Args>
    static concat_center_pos<Node>
    visit_node(LPos&& lpos, TPos&& tpos, Args&&... args)
    {
        return visit_maybe_relaxed_sub(
            args..., concat_trees_right_visitor<Node>{}, lpos, tpos);
    }
};

template <typename Node>
relaxed_pos<Node> concat_trees(Node* lroot,
                               shift_t lshift,
                               size_t lsize,
                               Node* ltail,
                               count_t ltcount,
                               Node* rroot,
                               shift_t rshift,
                               size_t rsize)
{
    return visit_maybe_relaxed_sub(lroot,
                                   lshift,
                                   lsize,
                                   concat_trees_left_visitor<Node>{},
                                   make_leaf_pos(ltail, ltcount),
                                   rroot,
                                   rshift,
                                   rsize)
        .realize();
}

template <typename Node>
relaxed_pos<Node> concat_trees(
    Node* ltail, count_t ltcount, Node* rroot, shift_t rshift, size_t rsize)
{
    return make_singleton_regular_sub_pos(ltail, ltcount)
        .visit(concat_trees_left_visitor<Node>{},
               empty_leaf_pos<Node>{},
               rroot,
               rshift,
               rsize)
        .realize();
}

template <typename Node>
using concat_center_mut_pos = concat_center_pos<Node>;

template <typename Node>
struct concat_merger_mut
{
    using node_t = Node;
    using edit_t = typename Node::edit_t;

    static constexpr auto B  = Node::bits;
    static constexpr auto BL = Node::bits_leaf;

    using result_t = concat_center_pos<Node>;

    edit_t ec_ = {};

    count_t* curr_;
    count_t n_;
    result_t result_;
    count_t count_      = 0;
    node_t* candidate_  = nullptr;
    edit_t candidate_e_ = Node::memory::transience_t::noone;

    concat_merger_mut(edit_t ec,
                      shift_t shift,
                      count_t* counts,
                      count_t n,
                      edit_t candidate_e,
                      node_t* candidate)
        : ec_{ec}
        , curr_{counts}
        , n_{n}
        , result_{shift + B, nullptr, 0}
    {
        if (candidate) {
            candidate->ensure_mutable_relaxed_e(candidate_e, ec);
            result_.nodes_[0] = candidate;
        } else {
            result_.nodes_[0] = node_t::make_inner_r_e(ec);
        }
    }

    node_t* to_        = {};
    count_t to_offset_ = {};
    size_t to_size_    = {};

    void set_candidate(edit_t candidate_e, node_t* candidate)
    {
        candidate_   = candidate;
        candidate_e_ = candidate_e;
    }

    void add_child(node_t* p, size_t size)
    {
        assert(size);
        ++curr_;
        auto parent  = result_.nodes_[result_.count_ - 1];
        auto relaxed = parent->relaxed();
        if (count_ == branches<B>) {
            parent->relaxed()->d.count = count_;
            assert(result_.count_ < result_t::max_children);
            n_ -= branches<B>;
            if (candidate_) {
                parent = candidate_;
                parent->ensure_mutable_relaxed_e(candidate_e_, ec_);
                candidate_ = nullptr;
            } else
                parent = node_t::make_inner_r_e(ec_);
            count_                         = 0;
            relaxed                        = parent->relaxed();
            result_.nodes_[result_.count_] = parent;
            result_.sizes_[result_.count_] = result_.sizes_[result_.count_ - 1];
            assert(result_.sizes_[result_.count_]);
            ++result_.count_;
        }
        auto idx = count_++;
        result_.sizes_[result_.count_ - 1] += size;
        assert(size);
        assert(result_.sizes_[result_.count_ - 1]);
        relaxed->d.sizes[idx] = size + (idx ? relaxed->d.sizes[idx - 1] : 0);
        assert(relaxed->d.sizes[idx]);
        parent->inner()[idx] = p;
    };

    template <typename Pos>
    void merge_leaf(Pos&& p, edit_t e)
    {
        auto from       = p.node();
        auto from_size  = p.size();
        auto from_count = p.count();
        assert(from);
        assert(from_size);
        if (!to_ && *curr_ == from_count) {
            add_child(from, from_size);
        } else {
            auto from_offset = count_t{};
            auto from_data   = from->leaf();
            auto from_mutate = from->can_mutate(e);
            do {
                if (!to_) {
                    if (from_mutate) {
                        node_t::ownee(from) = ec_;
                        to_                 = from->inc();
                        assert(from_count);
                    } else {
                        to_ = node_t::make_leaf_e(ec_);
                    }
                    to_offset_ = 0;
                }
                auto data = to_->leaf();
                auto to_copy =
                    std::min(from_count - from_offset, *curr_ - to_offset_);
                if (from == to_) {
                    if (from_offset != to_offset_)
                        std::move(from_data + from_offset,
                                  from_data + from_offset + to_copy,
                                  data + to_offset_);
                } else {
                    if (!from_mutate)
                        detail::uninitialized_copy(from_data + from_offset,
                                                   from_data + from_offset +
                                                       to_copy,
                                                   data + to_offset_);
                    else
                        detail::uninitialized_move(from_data + from_offset,
                                                   from_data + from_offset +
                                                       to_copy,
                                                   data + to_offset_);
                }
                to_offset_ += to_copy;
                from_offset += to_copy;
                if (*curr_ == to_offset_) {
                    add_child(to_, to_offset_);
                    to_ = nullptr;
                }
            } while (from_offset != from_count);
        }
    }

    template <typename Pos>
    void merge_inner(Pos&& p, edit_t e)
    {
        auto from       = p.node();
        auto from_size  = p.size();
        auto from_count = p.count();
        assert(from_size);
        if (!to_ && *curr_ == from_count) {
            add_child(from, from_size);
        } else {
            auto from_offset = count_t{};
            auto from_data   = from->inner();
            auto from_mutate = from->can_relax() && from->can_mutate(e);
            do {
                if (!to_) {
                    if (from_mutate) {
                        node_t::ownee(from) = ec_;
                        from->ensure_mutable_relaxed_e(e, ec_);
                        to_ = from;
                    } else {
                        to_ = node_t::make_inner_r_e(ec_);
                    }
                    to_offset_ = 0;
                    to_size_   = 0;
                }
                auto data = to_->inner();
                auto to_copy =
                    std::min(from_count - from_offset, *curr_ - to_offset_);
                auto sizes = to_->relaxed()->d.sizes;
                if (from != to_ || from_offset != to_offset_) {
                    std::copy(from_data + from_offset,
                              from_data + from_offset + to_copy,
                              data + to_offset_);
                    p.copy_sizes(
                        from_offset, to_copy, to_size_, sizes + to_offset_);
                }
                to_offset_ += to_copy;
                from_offset += to_copy;
                to_size_ = sizes[to_offset_ - 1];
                if (*curr_ == to_offset_) {
                    to_->relaxed()->d.count = to_offset_;
                    add_child(to_, to_size_);
                    to_ = nullptr;
                }
            } while (from_offset != from_count);
        }
    }

    concat_center_pos<Node> finish() const
    {
        assert(!to_);
        result_.nodes_[result_.count_ - 1]->relaxed()->d.count = count_;
        return result_;
    }

    void abort()
    {
        // We may have mutated stuff the tree in place, leaving
        // everything in a corrupted state...  It should be possible
        // to define cleanup properly, but that is a task for some
        // other day... ;)
        std::terminate();
    }
};

struct concat_merger_mut_visitor : visitor_base<concat_merger_mut_visitor>
{
    using this_t = concat_merger_mut_visitor;

    template <typename Pos, typename Merger>
    static void visit_inner(Pos&& p, Merger& merger, edit_type<Pos> e)
    {
        merger.merge_inner(p, e);
    }

    template <typename Pos, typename Merger>
    static void visit_leaf(Pos&& p, Merger& merger, edit_type<Pos> e)
    {
        merger.merge_leaf(p, e);
    }
};

template <bits_t B, bits_t BL>
struct concat_rebalance_plan_mut : concat_rebalance_plan<B, BL>
{
    using this_t = concat_rebalance_plan_mut;

    template <typename LPos, typename CPos, typename RPos>
    concat_center_mut_pos<node_type<CPos>> merge(edit_type<CPos> ec,
                                                 edit_type<CPos> el,
                                                 LPos&& lpos,
                                                 CPos&& cpos,
                                                 edit_type<CPos> er,
                                                 RPos&& rpos)
    {
        using node_t    = node_type<CPos>;
        using merger_t  = concat_merger_mut<node_t>;
        using visitor_t = concat_merger_mut_visitor;
        auto lnode      = ((node_t*) lpos.node());
        auto rnode      = ((node_t*) rpos.node());
        auto lmut2      = lnode && lnode->can_relax() && lnode->can_mutate(el);
        auto rmut2      = rnode && rnode->can_relax() && rnode->can_mutate(er);
        auto merger     = merger_t{ec,
                               cpos.shift(),
                               this->counts,
                               this->n,
                               el,
                               lmut2 ? lnode : nullptr};
        IMMER_TRY {
            lpos.each_left_sub(visitor_t{}, merger, el);
            cpos.each_sub(visitor_t{}, merger, ec);
            if (rmut2)
                merger.set_candidate(er, rnode);
            rpos.each_right_sub(visitor_t{}, merger, er);
            return merger.finish();
        }
        IMMER_CATCH (...) {
            merger.abort();
            IMMER_RETHROW;
        }
    }
};

template <typename Node, typename LPos, typename CPos, typename RPos>
concat_center_pos<Node> concat_rebalance_mut(edit_type<Node> ec,
                                             edit_type<Node> el,
                                             LPos&& lpos,
                                             CPos&& cpos,
                                             edit_type<Node> er,
                                             RPos&& rpos)
{
    auto plan = concat_rebalance_plan_mut<Node::bits, Node::bits_leaf>{};
    plan.fill(lpos, cpos, rpos);
    plan.shuffle(cpos.shift());
    return plan.merge(ec, el, lpos, cpos, er, rpos);
}

template <typename Node, typename LPos, typename TPos, typename RPos>
concat_center_mut_pos<Node> concat_leafs_mut(edit_type<Node> ec,
                                             edit_type<Node> el,
                                             LPos&& lpos,
                                             TPos&& tpos,
                                             edit_type<Node> er,
                                             RPos&& rpos)
{
    static_assert(Node::bits >= 2, "");
    assert(lpos.shift() == tpos.shift());
    assert(lpos.shift() == rpos.shift());
    assert(lpos.shift() == 0);
    if (tpos.count() > 0)
        return {
            Node::bits_leaf,
            lpos.node(),
            lpos.count(),
            tpos.node(),
            tpos.count(),
            rpos.node(),
            rpos.count(),
        };
    else
        return {
            Node::bits_leaf,
            lpos.node(),
            lpos.count(),
            rpos.node(),
            rpos.count(),
        };
}

template <typename Node>
struct concat_left_mut_visitor;
template <typename Node>
struct concat_right_mut_visitor;
template <typename Node>
struct concat_both_mut_visitor;

template <typename Node, typename LPos, typename TPos, typename RPos>
concat_center_mut_pos<Node> concat_inners_mut(edit_type<Node> ec,
                                              edit_type<Node> el,
                                              LPos&& lpos,
                                              TPos&& tpos,
                                              edit_type<Node> er,
                                              RPos&& rpos)
{
    auto lshift = lpos.shift();
    auto rshift = rpos.shift();
    // lpos.node() can be null it is a singleton_regular_sub_pos<...>,
    // this is, when the tree is just a tail...
    if (lshift > rshift) {
        auto cpos = lpos.last_sub(
            concat_left_mut_visitor<Node>{}, ec, el, tpos, er, rpos);
        return concat_rebalance_mut<Node>(
            ec, el, lpos, cpos, er, null_sub_pos{});
    } else if (lshift < rshift) {
        auto cpos = rpos.first_sub(
            concat_right_mut_visitor<Node>{}, ec, el, lpos, tpos, er);
        return concat_rebalance_mut<Node>(
            ec, el, null_sub_pos{}, cpos, er, rpos);
    } else {
        assert(lshift == rshift);
        assert(Node::bits_leaf == 0u || lshift > 0);
        auto cpos = lpos.last_sub(
            concat_both_mut_visitor<Node>{}, ec, el, tpos, er, rpos);
        return concat_rebalance_mut<Node>(ec, el, lpos, cpos, er, rpos);
    }
}

template <typename Node>
struct concat_left_mut_visitor : visitor_base<concat_left_mut_visitor<Node>>
{
    using this_t = concat_left_mut_visitor;
    using edit_t = typename Node::edit_t;

    template <typename LPos, typename TPos, typename RPos>
    static concat_center_mut_pos<Node> visit_inner(
        LPos&& lpos, edit_t ec, edit_t el, TPos&& tpos, edit_t er, RPos&& rpos)
    {
        return concat_inners_mut<Node>(ec, el, lpos, tpos, er, rpos);
    }

    template <typename LPos, typename TPos, typename RPos>
    static concat_center_mut_pos<Node> visit_leaf(
        LPos&& lpos, edit_t ec, edit_t el, TPos&& tpos, edit_t er, RPos&& rpos)
    {
        IMMER_UNREACHABLE;
    }
};

template <typename Node>
struct concat_right_mut_visitor : visitor_base<concat_right_mut_visitor<Node>>
{
    using this_t = concat_right_mut_visitor;
    using edit_t = typename Node::edit_t;

    template <typename RPos, typename LPos, typename TPos>
    static concat_center_mut_pos<Node> visit_inner(
        RPos&& rpos, edit_t ec, edit_t el, LPos&& lpos, TPos&& tpos, edit_t er)
    {
        return concat_inners_mut<Node>(ec, el, lpos, tpos, er, rpos);
    }

    template <typename RPos, typename LPos, typename TPos>
    static concat_center_mut_pos<Node> visit_leaf(
        RPos&& rpos, edit_t ec, edit_t el, LPos&& lpos, TPos&& tpos, edit_t er)
    {
        return concat_leafs_mut<Node>(ec, el, lpos, tpos, er, rpos);
    }
};

template <typename Node>
struct concat_both_mut_visitor : visitor_base<concat_both_mut_visitor<Node>>
{
    using this_t = concat_both_mut_visitor;
    using edit_t = typename Node::edit_t;

    template <typename LPos, typename TPos, typename RPos>
    static concat_center_mut_pos<Node> visit_inner(
        LPos&& lpos, edit_t ec, edit_t el, TPos&& tpos, edit_t er, RPos&& rpos)
    {
        return rpos.first_sub(
            concat_right_mut_visitor<Node>{}, ec, el, lpos, tpos, er);
    }

    template <typename LPos, typename TPos, typename RPos>
    static concat_center_mut_pos<Node> visit_leaf(
        LPos&& lpos, edit_t ec, edit_t el, TPos&& tpos, edit_t er, RPos&& rpos)
    {
        return rpos.first_sub_leaf(
            concat_right_mut_visitor<Node>{}, ec, el, lpos, tpos, er);
    }
};

template <typename Node>
struct concat_trees_right_mut_visitor
    : visitor_base<concat_trees_right_mut_visitor<Node>>
{
    using this_t = concat_trees_right_mut_visitor;
    using edit_t = typename Node::edit_t;

    template <typename RPos, typename LPos, typename TPos>
    static concat_center_mut_pos<Node> visit_node(
        RPos&& rpos, edit_t ec, edit_t el, LPos&& lpos, TPos&& tpos, edit_t er)
    {
        return concat_inners_mut<Node>(ec, el, lpos, tpos, er, rpos);
    }
};

template <typename Node>
struct concat_trees_left_mut_visitor
    : visitor_base<concat_trees_left_mut_visitor<Node>>
{
    using this_t = concat_trees_left_mut_visitor;
    using edit_t = typename Node::edit_t;

    template <typename LPos, typename TPos, typename... Args>
    static concat_center_mut_pos<Node> visit_node(LPos&& lpos,
                                                  edit_t ec,
                                                  edit_t el,
                                                  TPos&& tpos,
                                                  edit_t er,
                                                  Args&&... args)
    {
        return visit_maybe_relaxed_sub(args...,
                                       concat_trees_right_mut_visitor<Node>{},
                                       ec,
                                       el,
                                       lpos,
                                       tpos,
                                       er);
    }
};

template <typename Node>
relaxed_pos<Node> concat_trees_mut(edit_type<Node> ec,
                                   edit_type<Node> el,
                                   Node* lroot,
                                   shift_t lshift,
                                   size_t lsize,
                                   Node* ltail,
                                   count_t ltcount,
                                   edit_type<Node> er,
                                   Node* rroot,
                                   shift_t rshift,
                                   size_t rsize)
{
    return visit_maybe_relaxed_sub(lroot,
                                   lshift,
                                   lsize,
                                   concat_trees_left_mut_visitor<Node>{},
                                   ec,
                                   el,
                                   make_leaf_pos(ltail, ltcount),
                                   er,
                                   rroot,
                                   rshift,
                                   rsize)
        .realize_e(ec);
}

template <typename Node>
relaxed_pos<Node> concat_trees_mut(edit_type<Node> ec,
                                   edit_type<Node> el,
                                   Node* ltail,
                                   count_t ltcount,
                                   edit_type<Node> er,
                                   Node* rroot,
                                   shift_t rshift,
                                   size_t rsize)
{
    return make_singleton_regular_sub_pos(ltail, ltcount)
        .visit(concat_trees_left_mut_visitor<Node>{},
               ec,
               el,
               empty_leaf_pos<Node>{},
               er,
               rroot,
               rshift,
               rsize)
        .realize_e(ec);
}

} // namespace rbts
} // namespace detail
} // namespace immer
