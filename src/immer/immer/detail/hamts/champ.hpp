//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <immer/config.hpp>
#include <immer/detail/hamts/node.hpp>

#include <algorithm>

namespace immer {
namespace detail {
namespace hamts {

#if IMMER_DEBUG_STATS
struct champ_debug_stats
{
    std::size_t bits       = {};
    std::size_t value_size = {};
    std::size_t child_size = sizeof(void*);

    std::size_t inner_node_count         = {};
    std::size_t inner_node_w_value_count = {};
    std::size_t inner_node_w_child_count = {};
    std::size_t collision_node_count     = {};

    std::size_t child_count     = {};
    std::size_t value_count     = {};
    std::size_t collision_count = {};

    friend champ_debug_stats operator+(champ_debug_stats a, champ_debug_stats b)
    {
        if (a.bits != b.bits || a.value_size != b.value_size ||
            a.child_size != b.child_size)
            throw std::runtime_error{"accumulating incompatible stats"};
        return {
            a.bits,
            a.value_size,
            a.child_size,
            a.inner_node_count + b.inner_node_count,
            a.inner_node_w_value_count + b.inner_node_w_value_count,
            a.inner_node_w_child_count + b.inner_node_w_child_count,
            a.collision_node_count + b.collision_node_count,
            a.child_count + b.child_count,
            a.value_count + b.value_count,
            a.collision_count + b.collision_count,
        };
    }

    struct summary
    {
        double collision_ratio;

        double utilization;
        double child_utilization;
        double value_utilization;

        double dense_utilization;
        double dense_value_utilization;
        double dense_child_utilization;

        friend std::ostream& operator<<(std::ostream& os, const summary& s)
        {
            os << "---\n";
            os << "collisions\n"
               << "  ratio = " << s.collision_ratio << " %\n";
            os << "utilization\n"
               << "  total    = " << s.utilization << " %\n"
               << "  children = " << s.child_utilization << " %\n"
               << "  values   = " << s.value_utilization << " %\n";
            os << "utilization (dense)\n"
               << "  total    = " << s.dense_utilization << " %\n"
               << "  children = " << s.dense_child_utilization << " %\n"
               << "  values   = " << s.dense_value_utilization << " %\n";
            return os;
        }
    };

    summary get_summary() const
    {
        auto m = std::size_t{1} << bits;

        auto collision_ratio = 100. * collision_count / value_count;

        auto capacity          = m * inner_node_count;
        auto child_utilization = 100. * child_count / capacity;
        auto value_utilization = 100. * value_count / capacity;
        auto utilization =
            100. * (value_count * value_size + child_count * child_size) /
            (capacity * value_size + capacity * child_size);

        auto value_capacity = m * inner_node_w_value_count;
        auto child_capacity = m * inner_node_w_child_count;
        auto dense_child_utilization =
            child_capacity == 0 ? 100. : 100. * child_count / child_capacity;
        auto dense_value_utilization =
            value_capacity == 0 ? 100. : 100. * value_count / value_capacity;
        auto dense_utilization =
            value_capacity + child_capacity == 0
                ? 100.
                : 100. * (value_count * value_size + child_count * child_size) /
                      (value_capacity * value_size +
                       child_capacity * child_size);

        return {collision_ratio,
                utilization,
                child_utilization,
                value_utilization,
                dense_utilization,
                dense_value_utilization,
                dense_child_utilization};
    }
};
#endif

template <typename T,
          typename Hash,
          typename Equal,
          typename MemoryPolicy,
          bits_t B>
struct champ
{
    static constexpr auto bits = B;

    using node_t   = node<T, Hash, Equal, MemoryPolicy, B>;
    using edit_t   = typename MemoryPolicy::transience_t::edit;
    using owner_t  = typename MemoryPolicy::transience_t::owner;
    using bitmap_t = typename get_bitmap_type<B>::type;

    static_assert(branches<B> <= sizeof(bitmap_t) * 8, "");

    node_t* root;
    size_t size;

    static node_t* empty()
    {
        static const auto node = node_t::make_inner_n(0);
        return node->inc();
    }

    champ(node_t* r, size_t sz = 0)
        : root{r}
        , size{sz}
    {}

    champ(const champ& other)
        : champ{other.root, other.size}
    {
        inc();
    }

    champ(champ&& other)
        : champ{empty()}
    {
        swap(*this, other);
    }

    champ& operator=(const champ& other)
    {
        auto next = other;
        swap(*this, next);
        return *this;
    }

    champ& operator=(champ&& other)
    {
        swap(*this, other);
        return *this;
    }

    friend void swap(champ& x, champ& y)
    {
        using std::swap;
        swap(x.root, y.root);
        swap(x.size, y.size);
    }

    ~champ() { dec(); }

    void inc() const { root->inc(); }

    void dec() const
    {
        if (root->dec())
            node_t::delete_deep(root, 0);
    }

    std::size_t do_check_champ(node_t* node,
                               count_t depth,
                               size_t path_hash,
                               size_t hash_mask) const
    {
        auto result = std::size_t{};
        if (depth < max_depth<B>) {
            auto nodemap = node->nodemap();
            if (nodemap) {
                auto fst = node->children();
                for (auto idx = std::size_t{}; idx < branches<B>; ++idx) {
                    if (nodemap & (1 << idx)) {
                        auto child = *fst++;
                        result +=
                            do_check_champ(child,
                                           depth + 1,
                                           path_hash | (idx << (B * depth)),
                                           (hash_mask << B) | mask<B>);
                    }
                }
            }
            auto datamap = node->datamap();
            if (datamap) {
                auto fst = node->values();
                for (auto idx = std::size_t{}; idx < branches<B>; ++idx) {
                    if (datamap & (1 << idx)) {
                        auto hash  = Hash{}(*fst++);
                        auto check = (hash & hash_mask) ==
                                     (path_hash | (idx << (B * depth)));
                        // assert(check);
                        result += !!check;
                    }
                }
            }
        } else {
            auto fst = node->collisions();
            auto lst = fst + node->collision_count();
            for (; fst != lst; ++fst) {
                auto hash  = Hash{}(*fst);
                auto check = hash == path_hash;
                // assert(check);
                result += !!check;
            }
        }
        return result;
    }

    // Checks that the hashes of the values correspond with what has actually
    // been inserted.  If it doesn't it can mean that corruption has happened
    // due some value being moved out of the champ when it should have not.
    bool check_champ() const
    {
        auto r = do_check_champ(root, 0, 0, mask<B>);
        // assert(r == size);
        return r == size;
    }

#if IMMER_DEBUG_STATS
    void do_get_debug_stats(champ_debug_stats& stats,
                            node_t* node,
                            count_t depth) const
    {
        if (depth < max_depth<B>) {
            ++stats.inner_node_count;
            stats.inner_node_w_value_count += node->data_count() > 0;
            stats.inner_node_w_child_count += node->children_count() > 0;
            stats.value_count += node->data_count();
            stats.child_count += node->children_count();
            auto nodemap = node->nodemap();
            if (nodemap) {
                auto fst = node->children();
                auto lst = fst + node->children_count();
                for (; fst != lst; ++fst)
                    do_get_debug_stats(stats, *fst, depth + 1);
            }
        } else {
            ++stats.collision_node_count;
            stats.collision_count += node->collision_count();
        }
    }

    champ_debug_stats get_debug_stats() const
    {
        auto stats = champ_debug_stats{B, sizeof(T)};
        do_get_debug_stats(stats, root, 0);
        return stats;
    }
#endif

    template <typename U>
    static auto from_initializer_list(std::initializer_list<U> values)
    {
        auto e      = owner_t{};
        auto result = champ{empty()};
        for (auto&& v : values)
            result.add_mut(e, v);
        return result;
    }

    template <typename Iter,
              typename Sent,
              std::enable_if_t<compatible_sentinel_v<Iter, Sent>, bool> = true>
    static auto from_range(Iter first, Sent last)
    {
        auto e      = owner_t{};
        auto result = champ{empty()};
        for (; first != last; ++first)
            result.add_mut(e, *first);
        return result;
    }

    template <typename Fn>
    void for_each_chunk(Fn&& fn) const
    {
        for_each_chunk_traversal(root, 0, fn);
    }

    template <typename Fn>
    void
    for_each_chunk_traversal(const node_t* node, count_t depth, Fn&& fn) const
    {
        if (depth < max_depth<B>) {
            auto datamap = node->datamap();
            if (datamap)
                fn(node->values(), node->values() + node->data_count());
            auto nodemap = node->nodemap();
            if (nodemap) {
                auto fst = node->children();
                auto lst = fst + node->children_count();
                for (; fst != lst; ++fst)
                    for_each_chunk_traversal(*fst, depth + 1, fn);
            }
        } else {
            fn(node->collisions(),
               node->collisions() + node->collision_count());
        }
    }

    template <typename EqualValue, typename Differ>
    void diff(const champ& new_champ, Differ&& differ) const
    {
        diff<EqualValue>(root, new_champ.root, 0, std::forward<Differ>(differ));
    }

    template <typename EqualValue, typename Differ>
    void diff(const node_t* old_node,
              const node_t* new_node,
              count_t depth,
              Differ&& differ) const
    {
        if (old_node == new_node)
            return;
        if (depth < max_depth<B>) {
            auto old_nodemap = old_node->nodemap();
            auto new_nodemap = new_node->nodemap();
            auto old_datamap = old_node->datamap();
            auto new_datamap = new_node->datamap();
            auto old_bits    = old_nodemap | old_datamap;
            auto new_bits    = new_nodemap | new_datamap;
            auto changes     = old_bits ^ new_bits;

            // added bits
            for (auto bit : set_bits_range<bitmap_t>(new_bits & changes)) {
                if (new_nodemap & bit) {
                    auto offset = new_node->children_count(bit);
                    auto child  = new_node->children()[offset];
                    for_each_chunk_traversal(
                        child,
                        depth + 1,
                        [&](auto const& begin, auto const& end) {
                            for (auto it = begin; it != end; it++)
                                differ.added(*it);
                        });
                } else if (new_datamap & bit) {
                    auto offset       = new_node->data_count(bit);
                    auto const& value = new_node->values()[offset];
                    differ.added(value);
                }
            }

            // removed bits
            for (auto bit : set_bits_range<bitmap_t>(old_bits & changes)) {
                if (old_nodemap & bit) {
                    auto offset = old_node->children_count(bit);
                    auto child  = old_node->children()[offset];
                    for_each_chunk_traversal(
                        child,
                        depth + 1,
                        [&](auto const& begin, auto const& end) {
                            for (auto it = begin; it != end; it++)
                                differ.removed(*it);
                        });
                } else if (old_datamap & bit) {
                    auto offset       = old_node->data_count(bit);
                    auto const& value = old_node->values()[offset];
                    differ.removed(value);
                }
            }

            // bits in both nodes
            for (auto bit : set_bits_range<bitmap_t>(old_bits & new_bits)) {
                if ((old_nodemap & bit) && (new_nodemap & bit)) {
                    auto old_offset = old_node->children_count(bit);
                    auto new_offset = new_node->children_count(bit);
                    auto old_child  = old_node->children()[old_offset];
                    auto new_child  = new_node->children()[new_offset];
                    diff<EqualValue>(old_child, new_child, depth + 1, differ);
                } else if ((old_datamap & bit) && (new_nodemap & bit)) {
                    diff_data_node<EqualValue>(
                        old_node, new_node, bit, depth, differ);
                } else if ((old_nodemap & bit) && (new_datamap & bit)) {
                    diff_node_data<EqualValue>(
                        old_node, new_node, bit, depth, differ);
                } else if ((old_datamap & bit) && (new_datamap & bit)) {
                    diff_data_data<EqualValue>(old_node, new_node, bit, differ);
                }
            }
        } else {
            diff_collisions<EqualValue>(old_node, new_node, differ);
        }
    }

    template <typename EqualValue, typename Differ>
    void diff_data_node(const node_t* old_node,
                        const node_t* new_node,
                        bitmap_t bit,
                        count_t depth,
                        Differ&& differ) const
    {
        auto old_offset       = old_node->data_count(bit);
        auto const& old_value = old_node->values()[old_offset];
        auto new_offset       = new_node->children_count(bit);
        auto new_child        = new_node->children()[new_offset];

        bool found = false;
        for_each_chunk_traversal(
            new_child, depth + 1, [&](auto const& begin, auto const& end) {
                for (auto it = begin; it != end; it++) {
                    if (Equal{}(old_value, *it)) {
                        if (!EqualValue{}(old_value, *it))
                            differ.changed(old_value, *it);
                        found = true;
                    } else {
                        differ.added(*it);
                    }
                }
            });
        if (!found)
            differ.removed(old_value);
    }

    template <typename EqualValue, typename Differ>
    void diff_node_data(const node_t* old_node,
                        const node_t* const new_node,
                        bitmap_t bit,
                        count_t depth,
                        Differ&& differ) const
    {
        auto old_offset       = old_node->children_count(bit);
        auto old_child        = old_node->children()[old_offset];
        auto new_offset       = new_node->data_count(bit);
        auto const& new_value = new_node->values()[new_offset];

        bool found = false;
        for_each_chunk_traversal(
            old_child, depth + 1, [&](auto const& begin, auto const& end) {
                for (auto it = begin; it != end; it++) {
                    if (Equal{}(*it, new_value)) {
                        if (!EqualValue{}(*it, new_value))
                            differ.changed(*it, new_value);
                        found = true;
                    } else {
                        differ.removed(*it);
                    }
                }
            });
        if (!found)
            differ.added(new_value);
    }

    template <typename EqualValue, typename Differ>
    void diff_data_data(const node_t* old_node,
                        const node_t* new_node,
                        bitmap_t bit,
                        Differ&& differ) const
    {
        auto old_offset       = old_node->data_count(bit);
        auto new_offset       = new_node->data_count(bit);
        auto const& old_value = old_node->values()[old_offset];
        auto const& new_value = new_node->values()[new_offset];
        if (!Equal{}(old_value, new_value)) {
            differ.removed(old_value);
            differ.added(new_value);
        } else {
            if (!EqualValue{}(old_value, new_value))
                differ.changed(old_value, new_value);
        }
    }

    template <typename EqualValue, typename Differ>
    void diff_collisions(const node_t* old_node,
                         const node_t* new_node,
                         Differ&& differ) const
    {
        auto old_begin = old_node->collisions();
        auto old_end   = old_node->collisions() + old_node->collision_count();
        auto new_begin = new_node->collisions();
        auto new_end   = new_node->collisions() + new_node->collision_count();
        // search changes and removals
        for (auto old_it = old_begin; old_it != old_end; old_it++) {
            bool found = false;
            for (auto new_it = new_begin; new_it != new_end; new_it++) {
                if (Equal{}(*old_it, *new_it)) {
                    if (!EqualValue{}(*old_it, *new_it))
                        differ.changed(*old_it, *new_it);
                    found = true;
                    break;
                }
            }
            if (!found)
                differ.removed(*old_it);
        }
        // search new entries
        for (auto new_it = new_begin; new_it != new_end; new_it++) {
            bool found = false;
            for (auto old_it = old_begin; old_it != old_end; old_it++) {
                if (Equal{}(*old_it, *new_it)) {
                    found = true;
                    break;
                }
            }
            if (!found)
                differ.added(*new_it);
        }
    }

    template <typename Project, typename Default, typename K>
    decltype(auto) get(const K& k) const
    {
        auto node = root;
        auto hash = Hash{}(k);
        for (auto i = count_t{}; i < max_depth<B>; ++i) {
            auto bit = bitmap_t{1u} << (hash & mask<B>);
            if (node->nodemap() & bit) {
                auto offset = node->children_count(bit);
                node        = node->children()[offset];
                hash        = hash >> B;
            } else if (node->datamap() & bit) {
                auto offset = node->data_count(bit);
                auto val    = node->values() + offset;
                if (Equal{}(*val, k))
                    return Project{}(*val);
                else
                    return Default{}();
            } else {
                return Default{}();
            }
        }
        auto fst = node->collisions();
        auto lst = fst + node->collision_count();
        for (; fst != lst; ++fst)
            if (Equal{}(*fst, k))
                return Project{}(*fst);
        return Default{}();
    }

    struct add_result
    {
        node_t* node;
        bool added;
    };

    add_result do_add(node_t* node, T v, hash_t hash, shift_t shift) const
    {
        assert(node);
        if (shift == max_shift<B>) {
            auto fst = node->collisions();
            auto lst = fst + node->collision_count();
            for (; fst != lst; ++fst)
                if (Equal{}(*fst, v))
                    return {
                        node_t::copy_collision_replace(node, fst, std::move(v)),
                        false};
            return {node_t::copy_collision_insert(node, std::move(v)), true};
        } else {
            auto idx = (hash & (mask<B> << shift)) >> shift;
            auto bit = bitmap_t{1u} << idx;
            if (node->nodemap() & bit) {
                auto offset = node->children_count(bit);
                assert(node->children()[offset]);
                auto result = do_add(
                    node->children()[offset], std::move(v), hash, shift + B);
                IMMER_TRY {
                    result.node =
                        node_t::copy_inner_replace(node, offset, result.node);
                    return result;
                }
                IMMER_CATCH (...) {
                    node_t::delete_deep_shift(result.node, shift + B);
                    IMMER_RETHROW;
                }
            } else if (node->datamap() & bit) {
                auto offset = node->data_count(bit);
                auto val    = node->values() + offset;
                if (Equal{}(*val, v))
                    return {node_t::copy_inner_replace_value(
                                node, offset, std::move(v)),
                            false};
                else {
                    auto child = node_t::make_merged(
                        shift + B, std::move(v), hash, *val, Hash{}(*val));
                    IMMER_TRY {
                        return {node_t::copy_inner_replace_merged(
                                    node, bit, offset, child),
                                true};
                    }
                    IMMER_CATCH (...) {
                        node_t::delete_deep_shift(child, shift + B);
                        IMMER_RETHROW;
                    }
                }
            } else {
                return {
                    node_t::copy_inner_insert_value(node, bit, std::move(v)),
                    true};
            }
        }
    }

    champ add(T v) const
    {
        auto hash     = Hash{}(v);
        auto res      = do_add(root, std::move(v), hash, 0);
        auto new_size = size + (res.added ? 1 : 0);
        return {res.node, new_size};
    }

    struct add_mut_result
    {
        node_t* node;
        bool added;
        bool mutated;
    };

    add_mut_result
    do_add_mut(edit_t e, node_t* node, T v, hash_t hash, shift_t shift) const
    {
        assert(node);
        if (shift == max_shift<B>) {
            auto fst = node->collisions();
            auto lst = fst + node->collision_count();
            for (; fst != lst; ++fst)
                if (Equal{}(*fst, v)) {
                    if (node->can_mutate(e)) {
                        *fst = std::move(v);
                        return {node, false, true};
                    } else {
                        auto r = node_t::copy_collision_replace(
                            node, fst, std::move(v));
                        return {node_t::owned(r, e), false, false};
                    }
                }
            auto mutate = node->can_mutate(e);
            auto r = mutate ? node_t::move_collision_insert(node, std::move(v))
                            : node_t::copy_collision_insert(node, std::move(v));
            return {node_t::owned(r, e), true, mutate};
        } else {
            auto idx = (hash & (mask<B> << shift)) >> shift;
            auto bit = bitmap_t{1u} << idx;
            if (node->nodemap() & bit) {
                auto offset = node->children_count(bit);
                auto child  = node->children()[offset];
                if (node->can_mutate(e)) {
                    auto result =
                        do_add_mut(e, child, std::move(v), hash, shift + B);
                    node->children()[offset] = result.node;
                    if (!result.mutated && child->dec())
                        node_t::delete_deep_shift(child, shift + B);
                    return {node, result.added, true};
                } else {
                    assert(node->children()[offset]);
                    auto result = do_add(child, std::move(v), hash, shift + B);
                    IMMER_TRY {
                        result.node = node_t::copy_inner_replace(
                            node, offset, result.node);
                        node_t::owned(result.node, e);
                        return {result.node, result.added, false};
                    }
                    IMMER_CATCH (...) {
                        node_t::delete_deep_shift(result.node, shift + B);
                        IMMER_RETHROW;
                    }
                }
            } else if (node->datamap() & bit) {
                auto offset = node->data_count(bit);
                auto val    = node->values() + offset;
                if (Equal{}(*val, v)) {
                    if (node->can_mutate(e)) {
                        auto vals    = node->ensure_mutable_values(e);
                        vals[offset] = std::move(v);
                        return {node, false, true};
                    } else {
                        auto r = node_t::copy_inner_replace_value(
                            node, offset, std::move(v));
                        return {node_t::owned_values(r, e), false, false};
                    }
                } else {
                    auto mutate        = node->can_mutate(e);
                    auto mutate_values = mutate && node->can_mutate_values(e);
                    auto hash2         = Hash{}(*val);
                    auto child         = node_t::make_merged_e(
                        e,
                        shift + B,
                        std::move(v),
                        hash,
                        mutate_values ? std::move(*val) : *val,
                        hash2);
                    IMMER_TRY {
                        auto r = mutate ? node_t::move_inner_replace_merged(
                                              e, node, bit, offset, child)
                                        : node_t::copy_inner_replace_merged(
                                              node, bit, offset, child);
                        return {node_t::owned_values_safe(r, e), true, mutate};
                    }
                    IMMER_CATCH (...) {
                        node_t::delete_deep_shift(child, shift + B);
                        IMMER_RETHROW;
                    }
                }
            } else {
                auto mutate = node->can_mutate(e);
                auto r      = mutate ? node_t::move_inner_insert_value(
                                      e, node, bit, std::move(v))
                                     : node_t::copy_inner_insert_value(
                                      node, bit, std::move(v));
                return {node_t::owned_values(r, e), true, mutate};
            }
        }
    }

    void add_mut(edit_t e, T v)
    {
        auto hash = Hash{}(v);
        auto res  = do_add_mut(e, root, std::move(v), hash, 0);
        if (!res.mutated && root->dec())
            node_t::delete_deep(root, 0);
        root = res.node;
        size += res.added ? 1 : 0;
    }

    using update_result = add_result;

    template <typename Project,
              typename Default,
              typename Combine,
              typename K,
              typename Fn>
    update_result
    do_update(node_t* node, K&& k, Fn&& fn, hash_t hash, shift_t shift) const
    {
        if (shift == max_shift<B>) {
            auto fst = node->collisions();
            auto lst = fst + node->collision_count();
            for (; fst != lst; ++fst)
                if (Equal{}(*fst, k))
                    return {node_t::copy_collision_replace(
                                node,
                                fst,
                                Combine{}(std::forward<K>(k),
                                          std::forward<Fn>(fn)(Project{}(
                                              detail::as_const(*fst))))),
                            false};
            return {node_t::copy_collision_insert(
                        node,
                        Combine{}(std::forward<K>(k),
                                  std::forward<Fn>(fn)(Default{}()))),
                    true};
        } else {
            auto idx = (hash & (mask<B> << shift)) >> shift;
            auto bit = bitmap_t{1u} << idx;
            if (node->nodemap() & bit) {
                auto offset = node->children_count(bit);
                auto result = do_update<Project, Default, Combine>(
                    node->children()[offset],
                    k,
                    std::forward<Fn>(fn),
                    hash,
                    shift + B);
                IMMER_TRY {
                    result.node =
                        node_t::copy_inner_replace(node, offset, result.node);
                    return result;
                }
                IMMER_CATCH (...) {
                    node_t::delete_deep_shift(result.node, shift + B);
                    IMMER_RETHROW;
                }
            } else if (node->datamap() & bit) {
                auto offset = node->data_count(bit);
                auto val    = node->values() + offset;
                if (Equal{}(*val, k))
                    return {node_t::copy_inner_replace_value(
                                node,
                                offset,
                                Combine{}(std::forward<K>(k),
                                          std::forward<Fn>(fn)(Project{}(
                                              detail::as_const(*val))))),
                            false};
                else {
                    auto child = node_t::make_merged(
                        shift + B,
                        Combine{}(std::forward<K>(k),
                                  std::forward<Fn>(fn)(Default{}())),
                        hash,
                        *val,
                        Hash{}(*val));
                    IMMER_TRY {
                        return {node_t::copy_inner_replace_merged(
                                    node, bit, offset, child),
                                true};
                    }
                    IMMER_CATCH (...) {
                        node_t::delete_deep_shift(child, shift + B);
                        IMMER_RETHROW;
                    }
                }
            } else {
                return {node_t::copy_inner_insert_value(
                            node,
                            bit,
                            Combine{}(std::forward<K>(k),
                                      std::forward<Fn>(fn)(Default{}()))),
                        true};
            }
        }
    }

    template <typename Project,
              typename Default,
              typename Combine,
              typename K,
              typename Fn>
    champ update(const K& k, Fn&& fn) const
    {
        auto hash = Hash{}(k);
        auto res  = do_update<Project, Default, Combine>(
            root, k, std::forward<Fn>(fn), hash, 0);
        auto new_size = size + (res.added ? 1 : 0);
        return {res.node, new_size};
    }

    template <typename Project, typename Combine, typename K, typename Fn>
    node_t* do_update_if_exists(
        node_t* node, K&& k, Fn&& fn, hash_t hash, shift_t shift) const
    {
        if (shift == max_shift<B>) {
            auto fst = node->collisions();
            auto lst = fst + node->collision_count();
            for (; fst != lst; ++fst)
                if (Equal{}(*fst, k))
                    return node_t::copy_collision_replace(
                        node,
                        fst,
                        Combine{}(std::forward<K>(k),
                                  std::forward<Fn>(fn)(
                                      Project{}(detail::as_const(*fst)))));
            return nullptr;
        } else {
            auto idx = (hash & (mask<B> << shift)) >> shift;
            auto bit = bitmap_t{1u} << idx;
            if (node->nodemap() & bit) {
                auto offset = node->children_count(bit);
                auto result = do_update_if_exists<Project, Combine>(
                    node->children()[offset],
                    k,
                    std::forward<Fn>(fn),
                    hash,
                    shift + B);
                IMMER_TRY {
                    return result ? node_t::copy_inner_replace(
                                        node, offset, result)
                                  : nullptr;
                }
                IMMER_CATCH (...) {
                    node_t::delete_deep_shift(result, shift + B);
                    IMMER_RETHROW;
                }
            } else if (node->datamap() & bit) {
                auto offset = node->data_count(bit);
                auto val    = node->values() + offset;
                if (Equal{}(*val, k))
                    return node_t::copy_inner_replace_value(
                        node,
                        offset,
                        Combine{}(std::forward<K>(k),
                                  std::forward<Fn>(fn)(
                                      Project{}(detail::as_const(*val)))));
                else {
                    return nullptr;
                }
            } else {
                return nullptr;
            }
        }
    }

    template <typename Project, typename Combine, typename K, typename Fn>
    champ update_if_exists(const K& k, Fn&& fn) const
    {
        auto hash = Hash{}(k);
        auto res  = do_update_if_exists<Project, Combine>(
            root, k, std::forward<Fn>(fn), hash, 0);
        if (res) {
            return {res, size};
        } else {
            return {root->inc(), size};
        };
    }

    using update_mut_result = add_mut_result;

    template <typename Project,
              typename Default,
              typename Combine,
              typename K,
              typename Fn>
    update_mut_result do_update_mut(edit_t e,
                                    node_t* node,
                                    K&& k,
                                    Fn&& fn,
                                    hash_t hash,
                                    shift_t shift) const
    {
        if (shift == max_shift<B>) {
            auto fst = node->collisions();
            auto lst = fst + node->collision_count();
            for (; fst != lst; ++fst)
                if (Equal{}(*fst, k)) {
                    if (node->can_mutate(e)) {
                        *fst = Combine{}(
                            std::forward<K>(k),
                            std::forward<Fn>(fn)(Project{}(std::move(*fst))));
                        return {node, false, true};
                    } else {
                        auto r = node_t::copy_collision_replace(
                            node,
                            fst,
                            Combine{}(std::forward<K>(k),
                                      std::forward<Fn>(fn)(
                                          Project{}(detail::as_const(*fst)))));
                        return {node_t::owned(r, e), false, false};
                    }
                }
            auto v      = Combine{}(std::forward<K>(k),
                               std::forward<Fn>(fn)(Default{}()));
            auto mutate = node->can_mutate(e);
            auto r = mutate ? node_t::move_collision_insert(node, std::move(v))
                            : node_t::copy_collision_insert(node, std::move(v));
            return {node_t::owned(r, e), true, mutate};
        } else {
            auto idx = (hash & (mask<B> << shift)) >> shift;
            auto bit = bitmap_t{1u} << idx;
            if (node->nodemap() & bit) {
                auto offset = node->children_count(bit);
                auto child  = node->children()[offset];
                if (node->can_mutate(e)) {
                    auto result = do_update_mut<Project, Default, Combine>(
                        e, child, k, std::forward<Fn>(fn), hash, shift + B);
                    node->children()[offset] = result.node;
                    if (!result.mutated && child->dec())
                        node_t::delete_deep_shift(child, shift + B);
                    return {node, result.added, true};
                } else {
                    auto result = do_update<Project, Default, Combine>(
                        child, k, std::forward<Fn>(fn), hash, shift + B);
                    IMMER_TRY {
                        result.node = node_t::copy_inner_replace(
                            node, offset, result.node);
                        node_t::owned(result.node, e);
                        return {result.node, result.added, false};
                    }
                    IMMER_CATCH (...) {
                        node_t::delete_deep_shift(result.node, shift + B);
                        IMMER_RETHROW;
                    }
                }
            } else if (node->datamap() & bit) {
                auto offset = node->data_count(bit);
                auto val    = node->values() + offset;
                if (Equal{}(*val, k)) {
                    if (node->can_mutate(e)) {
                        auto vals    = node->ensure_mutable_values(e);
                        vals[offset] = Combine{}(std::forward<K>(k),
                                                 std::forward<Fn>(fn)(Project{}(
                                                     std::move(vals[offset]))));
                        return {node, false, true};
                    } else {
                        auto r = node_t::copy_inner_replace_value(
                            node,
                            offset,
                            Combine{}(std::forward<K>(k),
                                      std::forward<Fn>(fn)(
                                          Project{}(detail::as_const(*val)))));
                        return {node_t::owned_values(r, e), false, false};
                    }
                } else {
                    auto mutate        = node->can_mutate(e);
                    auto mutate_values = mutate && node->can_mutate_values(e);
                    auto hash2         = Hash{}(*val);
                    auto child         = node_t::make_merged_e(
                        e,
                        shift + B,
                        Combine{}(std::forward<K>(k),
                                  std::forward<Fn>(fn)(Default{}())),
                        hash,
                        mutate_values ? std::move(*val) : *val,
                        hash2);
                    IMMER_TRY {
                        auto r = mutate ? node_t::move_inner_replace_merged(
                                              e, node, bit, offset, child)
                                        : node_t::copy_inner_replace_merged(
                                              node, bit, offset, child);
                        return {node_t::owned_values_safe(r, e), true, mutate};
                    }
                    IMMER_CATCH (...) {
                        node_t::delete_deep_shift(child, shift + B);
                        IMMER_RETHROW;
                    }
                }
            } else {
                auto mutate = node->can_mutate(e);
                auto v      = Combine{}(std::forward<K>(k),
                                   std::forward<Fn>(fn)(Default{}()));
                auto r      = mutate ? node_t::move_inner_insert_value(
                                      e, node, bit, std::move(v))
                                     : node_t::copy_inner_insert_value(
                                      node, bit, std::move(v));
                return {node_t::owned_values(r, e), true, mutate};
            }
        }
    }

    template <typename Project,
              typename Default,
              typename Combine,
              typename K,
              typename Fn>
    void update_mut(edit_t e, const K& k, Fn&& fn)
    {
        auto hash = Hash{}(k);
        auto res  = do_update_mut<Project, Default, Combine>(
            e, root, k, std::forward<Fn>(fn), hash, 0);
        if (!res.mutated && root->dec())
            node_t::delete_deep(root, 0);
        root = res.node;
        size += res.added ? 1 : 0;
    }

    struct update_if_exists_mut_result
    {
        node_t* node;
        bool mutated;
    };

    template <typename Project, typename Combine, typename K, typename Fn>
    update_if_exists_mut_result do_update_if_exists_mut(edit_t e,
                                                        node_t* node,
                                                        K&& k,
                                                        Fn&& fn,
                                                        hash_t hash,
                                                        shift_t shift) const
    {
        if (shift == max_shift<B>) {
            auto fst = node->collisions();
            auto lst = fst + node->collision_count();
            for (; fst != lst; ++fst)
                if (Equal{}(*fst, k)) {
                    if (node->can_mutate(e)) {
                        *fst = Combine{}(
                            std::forward<K>(k),
                            std::forward<Fn>(fn)(Project{}(std::move(*fst))));
                        return {node, true};
                    } else {
                        auto r = node_t::copy_collision_replace(
                            node,
                            fst,
                            Combine{}(std::forward<K>(k),
                                      std::forward<Fn>(fn)(
                                          Project{}(detail::as_const(*fst)))));
                        return {node_t::owned(r, e), false};
                    }
                }
            return {nullptr, false};
        } else {
            auto idx = (hash & (mask<B> << shift)) >> shift;
            auto bit = bitmap_t{1u} << idx;
            if (node->nodemap() & bit) {
                auto offset = node->children_count(bit);
                auto child  = node->children()[offset];
                if (node->can_mutate(e)) {
                    auto result = do_update_if_exists_mut<Project, Combine>(
                        e, child, k, std::forward<Fn>(fn), hash, shift + B);
                    if (result.node) {
                        node->children()[offset] = result.node;
                        if (!result.mutated && child->dec())
                            node_t::delete_deep_shift(child, shift + B);
                        return {node, true};
                    } else {
                        return {nullptr, false};
                    }
                } else {
                    auto result = do_update_if_exists<Project, Combine>(
                        child, k, std::forward<Fn>(fn), hash, shift + B);
                    IMMER_TRY {
                        if (result) {
                            result = node_t::copy_inner_replace(
                                node, offset, result);
                            node_t::owned(result, e);
                            return {result, false};
                        } else {
                            return {nullptr, false};
                        }
                    }
                    IMMER_CATCH (...) {
                        node_t::delete_deep_shift(result, shift + B);
                        IMMER_RETHROW;
                    }
                }
            } else if (node->datamap() & bit) {
                auto offset = node->data_count(bit);
                auto val    = node->values() + offset;
                if (Equal{}(*val, k)) {
                    if (node->can_mutate(e)) {
                        auto vals    = node->ensure_mutable_values(e);
                        vals[offset] = Combine{}(std::forward<K>(k),
                                                 std::forward<Fn>(fn)(Project{}(
                                                     std::move(vals[offset]))));
                        return {node, true};
                    } else {
                        auto r = node_t::copy_inner_replace_value(
                            node,
                            offset,
                            Combine{}(std::forward<K>(k),
                                      std::forward<Fn>(fn)(
                                          Project{}(detail::as_const(*val)))));
                        return {node_t::owned_values(r, e), false};
                    }
                } else {
                    return {nullptr, false};
                }
            } else {
                return {nullptr, false};
            }
        }
    }

    template <typename Project, typename Combine, typename K, typename Fn>
    void update_if_exists_mut(edit_t e, const K& k, Fn&& fn)
    {
        auto hash = Hash{}(k);
        auto res  = do_update_if_exists_mut<Project, Combine>(
            e, root, k, std::forward<Fn>(fn), hash, 0);
        if (res.node) {
            if (!res.mutated && root->dec())
                node_t::delete_deep(root, 0);
            root = res.node;
        }
    }

    // basically:
    //      variant<monostate_t, T*, node_t*>
    // boo bad we are not using... C++17 :'(
    struct sub_result
    {
        enum kind_t
        {
            nothing,
            singleton,
            tree
        };

        union data_t
        {
            T* singleton;
            node_t* tree;
        };

        kind_t kind;
        data_t data;

        sub_result()
            : kind{nothing} {};
        sub_result(T* x)
            : kind{singleton}
        {
            data.singleton = x;
        };
        sub_result(node_t* x)
            : kind{tree}
        {
            data.tree = x;
        };
    };

    template <typename K>
    sub_result
    do_sub(node_t* node, const K& k, hash_t hash, shift_t shift) const
    {
        if (shift == max_shift<B>) {
            auto fst = node->collisions();
            auto lst = fst + node->collision_count();
            for (auto cur = fst; cur != lst; ++cur)
                if (Equal{}(*cur, k))
                    return node->collision_count() > 2
                               ? node_t::copy_collision_remove(node, cur)
                               : sub_result{fst + (cur == fst)};
#if !defined(_MSC_VER)
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
#endif
            // Apparently GCC is generating this warning sometimes when
            // compiling the benchmarks. It makes however no sense at all.
            return {};
#if !defined(_MSC_VER)
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif
#endif
        } else {
            auto idx = (hash & (mask<B> << shift)) >> shift;
            auto bit = bitmap_t{1u} << idx;
            if (node->nodemap() & bit) {
                auto offset = node->children_count(bit);
                auto result =
                    do_sub(node->children()[offset], k, hash, shift + B);
                switch (result.kind) {
                case sub_result::nothing:
                    return {};
                case sub_result::singleton:
                    return node->datamap() == 0 &&
                                   node->children_count() == 1 && shift > 0
                               ? result
                               : node_t::copy_inner_replace_inline(
                                     node, bit, offset, *result.data.singleton);
                case sub_result::tree:
                    IMMER_TRY {
                        return node_t::copy_inner_replace(
                            node, offset, result.data.tree);
                    }
                    IMMER_CATCH (...) {
                        node_t::delete_deep_shift(result.data.tree, shift + B);
                        IMMER_RETHROW;
                    }
                }
            } else if (node->datamap() & bit) {
                auto offset = node->data_count(bit);
                auto val    = node->values() + offset;
                if (Equal{}(*val, k)) {
                    auto nv = node->data_count();
                    if (node->nodemap() || nv > 2)
                        return node_t::copy_inner_remove_value(
                            node, bit, offset);
                    else if (nv == 2) {
                        return shift > 0 ? sub_result{node->values() + !offset}
                                         : node_t::make_inner_n(
                                               0,
                                               node->datamap() & ~bit,
                                               node->values()[!offset]);
                    } else {
                        assert(shift == 0);
                        return empty();
                    }
                }
            }
            return {};
        }
    }

    template <typename K>
    champ sub(const K& k) const
    {
        auto hash = Hash{}(k);
        auto res  = do_sub(root, k, hash, 0);
        switch (res.kind) {
        case sub_result::nothing:
            return *this;
        case sub_result::tree:
            return {res.data.tree, size - 1};
        default:
            IMMER_UNREACHABLE;
        }
    }

    struct sub_result_mut
    {
        using kind_t = typename sub_result::kind_t;
        using data_t = typename sub_result::data_t;

        kind_t kind;
        data_t data;
        bool owned;
        bool mutated;

        sub_result_mut(sub_result a)
            : kind{a.kind}
            , data{a.data}
            , owned{false}
            , mutated{false}
        {}
        sub_result_mut(sub_result a, bool m)
            : kind{a.kind}
            , data{a.data}
            , owned{false}
            , mutated{m}
        {}
        sub_result_mut()
            : kind{kind_t::nothing}
            , mutated{false} {};
        sub_result_mut(T* x, bool m)
            : kind{kind_t::singleton}
            , owned{m}
            , mutated{m}
        {
            data.singleton = x;
        };
        sub_result_mut(T* x, bool o, bool m)
            : kind{kind_t::singleton}
            , owned{o}
            , mutated{m}
        {
            data.singleton = x;
        };
        sub_result_mut(node_t* x, bool m)
            : kind{kind_t::tree}
            , owned{false}
            , mutated{m}
        {
            data.tree = x;
        };
    };

    template <typename K>
    sub_result_mut do_sub_mut(edit_t e,
                              node_t* node,
                              const K& k,
                              hash_t hash,
                              shift_t shift,
                              void* store) const
    {
        auto mutate = node->can_mutate(e);
        if (shift == max_shift<B>) {
            auto fst = node->collisions();
            auto lst = fst + node->collision_count();
            for (auto cur = fst; cur != lst; ++cur) {
                if (Equal{}(*cur, k)) {
                    if (node->collision_count() <= 2) {
                        if (mutate) {
                            auto r = new (store)
                                T{std::move(node->collisions()[cur == fst])};
                            node_t::delete_collision(node);
                            return sub_result_mut{r, true};
                        } else {
                            return sub_result_mut{fst + (cur == fst), false};
                        }
                    } else {
                        auto r = mutate
                                     ? node_t::move_collision_remove(node, cur)
                                     : node_t::copy_collision_remove(node, cur);
                        return {node_t::owned(r, e), mutate};
                    }
                }
            }
            return {};
        } else {
            auto idx = (hash & (mask<B> << shift)) >> shift;
            auto bit = bitmap_t{1u} << idx;
            if (node->nodemap() & bit) {
                auto offset   = node->children_count(bit);
                auto children = node->children();
                auto child    = children[offset];
                auto result =
                    mutate ? do_sub_mut(e, child, k, hash, shift + B, store)
                           : do_sub(child, k, hash, shift + B);
                switch (result.kind) {
                case sub_result::nothing:
                    return {};
                case sub_result::singleton:
                    if (node->datamap() == 0 && node->children_count() == 1 &&
                        shift > 0) {
                        if (mutate) {
                            node_t::delete_inner(node);
                            if (!result.mutated && child->dec())
                                node_t::delete_deep_shift(child, shift + B);
                        }
                        return {result.data.singleton, result.owned, mutate};
                    } else {
                        auto r =
                            mutate ? node_t::move_inner_replace_inline(
                                         e,
                                         node,
                                         bit,
                                         offset,
                                         result.owned
                                             ? std::move(*result.data.singleton)
                                             : *result.data.singleton)
                                   : node_t::copy_inner_replace_inline(
                                         node,
                                         bit,
                                         offset,
                                         *result.data.singleton);
                        if (result.owned)
                            detail::destroy_at(result.data.singleton);
                        if (!result.mutated && mutate && child->dec())
                            node_t::delete_deep_shift(child, shift + B);
                        return {node_t::owned_values(r, e), mutate};
                    }
                case sub_result::tree:
                    if (mutate) {
                        children[offset] = result.data.tree;
                        if (!result.mutated && child->dec())
                            node_t::delete_deep_shift(child, shift + B);
                        return {node, true};
                    } else {
                        IMMER_TRY {
                            auto r = node_t::copy_inner_replace(
                                node, offset, result.data.tree);
                            return {node_t::owned(r, e), false};
                        }
                        IMMER_CATCH (...) {
                            node_t::delete_deep_shift(result.data.tree,
                                                      shift + B);
                            IMMER_RETHROW;
                        }
                    }
                }
            } else if (node->datamap() & bit) {
                auto offset        = node->data_count(bit);
                auto val           = node->values() + offset;
                auto mutate_values = mutate && node->can_mutate_values(e);
                if (Equal{}(*val, k)) {
                    auto nv = node->data_count();
                    if (node->nodemap() || nv > 2) {
                        auto r = mutate ? node_t::move_inner_remove_value(
                                              e, node, bit, offset)
                                        : node_t::copy_inner_remove_value(
                                              node, bit, offset);
                        return {node_t::owned_values_safe(r, e), mutate};
                    } else if (nv == 2) {
                        if (shift > 0) {
                            if (mutate_values) {
                                auto r = new (store)
                                    T{std::move(node->values()[!offset])};
                                node_t::delete_inner(node);
                                return {r, true};
                            } else {
                                return {node->values() + !offset, false};
                            }
                        } else {
                            auto& v = node->values()[!offset];
                            auto r  = node_t::make_inner_n(
                                0,
                                node->datamap() & ~bit,
                                mutate_values ? std::move(v) : v);
                            assert(!node->nodemap());
                            if (mutate)
                                node_t::delete_inner(node);
                            return {node_t::owned_values(r, e), mutate};
                        }
                    } else {
                        assert(shift == 0);
                        if (mutate)
                            node_t::delete_inner(node);
                        return {empty(), mutate};
                    }
                }
            }
            return {};
        }
    }

    template <typename K>
    void sub_mut(edit_t e, const K& k)
    {
        auto store = aligned_storage_for<T>{};
        auto hash  = Hash{}(k);
        auto res   = do_sub_mut(e, root, k, hash, 0, &store);
        switch (res.kind) {
        case sub_result::nothing:
            break;
        case sub_result::tree:
            if (!res.mutated && root->dec())
                node_t::delete_deep(root, 0);
            root = res.data.tree;
            --size;
            break;
        default:
            IMMER_UNREACHABLE;
        }
    }

    template <typename Eq = Equal>
    bool equals(const champ& other) const
    {
        return size == other.size && equals_tree<Eq>(root, other.root, 0);
    }

    template <typename Eq>
    static bool equals_tree(const node_t* a, const node_t* b, count_t depth)
    {
        if (a == b)
            return true;
        else if (depth == max_depth<B>) {
            auto nv = a->collision_count();
            return nv == b->collision_count() &&
                   equals_collisions<Eq>(a->collisions(), b->collisions(), nv);
        } else {
            if (a->nodemap() != b->nodemap() || a->datamap() != b->datamap())
                return false;
            auto n = a->children_count();
            for (auto i = count_t{}; i < n; ++i)
                if (!equals_tree<Eq>(
                        a->children()[i], b->children()[i], depth + 1))
                    return false;
            auto nv = a->data_count();
            return !nv || equals_values<Eq>(a->values(), b->values(), nv);
        }
    }

    template <typename Eq>
    static bool equals_values(const T* a, const T* b, count_t n)
    {
        return std::equal(a, a + n, b, Eq{});
    }

    template <typename Eq>
    static bool equals_collisions(const T* a, const T* b, count_t n)
    {
        auto ae = a + n;
        auto be = b + n;
        for (; a != ae; ++a) {
            for (auto fst = b; fst != be; ++fst)
                if (Eq{}(*a, *fst))
                    goto good;
            return false;
        good:
            continue;
        }
        return true;
    }
};

} // namespace hamts
} // namespace detail
} // namespace immer
