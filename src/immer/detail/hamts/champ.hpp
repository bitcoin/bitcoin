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

template <typename T,
          typename Hash,
          typename Equal,
          typename MemoryPolicy,
          bits_t B>
struct champ
{
    static_assert(branches<B> <= sizeof(bitmap_t) * 8, "");

    static constexpr auto bits = B;

    using node_t = node<T, Hash, Equal, MemoryPolicy, B>;

    node_t* root;
    size_t  size;

    static const champ empty;

    champ(node_t* r, size_t sz)
        : root{r}, size{sz}
    {
    }

    champ(const champ& other)
        : champ{other.root, other.size}
    {
        inc();
    }

    champ(champ&& other)
        : champ{empty}
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

    ~champ()
    {
        dec();
    }

    void inc() const
    {
        root->inc();
    }

    void dec() const
    {
        if (root->dec())
            node_t::delete_deep(root, 0);
    }

    template <typename Fn>
    void for_each_chunk(Fn&& fn) const
    {
        for_each_chunk_traversal(root, 0, fn);
    }

    template <typename Fn>
    void for_each_chunk_traversal(node_t* node, count_t depth, Fn&& fn) const
    {
        if (depth < max_depth<B>) {
            auto datamap = node->datamap();
            if (datamap)
                fn(node->values(), node->values() + popcount(datamap));
            auto nodemap = node->nodemap();
            if (nodemap) {
                auto fst = node->children();
                auto lst = fst + popcount(nodemap);
                for (; fst != lst; ++fst)
                    for_each_chunk_traversal(*fst, depth + 1, fn);
            }
        } else {
            fn(node->collisions(), node->collisions() + node->collision_count());
        }
    }

    template <typename Project, typename Default, typename K>
    decltype(auto) get(const K& k) const
    {
        auto node = root;
        auto hash = Hash{}(k);
        for (auto i = count_t{}; i < max_depth<B>; ++i) {
            auto bit = 1 << (hash & mask<B>);
            if (node->nodemap() & bit) {
                auto offset = popcount(node->nodemap() & (bit - 1));
                node = node->children() [offset];
                hash = hash >> B;
            } else if (node->datamap() & bit) {
                auto offset = popcount(node->datamap() & (bit - 1));
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

    std::pair<node_t*, bool>
    do_add(node_t* node, T v, hash_t hash, shift_t shift) const
    {
        if (shift == max_shift<B>) {
            auto fst = node->collisions();
            auto lst = fst + node->collision_count();
            for (; fst != lst; ++fst)
                if (Equal{}(*fst, v))
                    return {
                        node_t::copy_collision_replace(node, fst, std::move(v)),
                        false
                    };
            return {
                node_t::copy_collision_insert(node, std::move(v)),
                true
            };
        } else {
            auto idx = (hash & (mask<B> << shift)) >> shift;
            auto bit = 1 << idx;
            if (node->nodemap() & bit) {
                auto offset = popcount(node->nodemap() & (bit - 1));
                auto result = do_add(node->children() [offset],
                                     std::move(v), hash,
                                     shift + B);
                try {
                    result.first = node_t::copy_inner_replace(
                        node, offset, result.first);
                    return result;
                } catch (...) {
                    node_t::delete_deep_shift(result.first, shift + B);
                    throw;
                }
            } else if (node->datamap() & bit) {
                auto offset = popcount(node->datamap() & (bit - 1));
                auto val    = node->values() + offset;
                if (Equal{}(*val, v))
                    return {
                        node_t::copy_inner_replace_value(
                            node, offset, std::move(v)),
                        false
                    };
                else {
                    auto child = node_t::make_merged(shift + B,
                                                    std::move(v), hash,
                                                    *val, Hash{}(*val));
                    try {
                        return {
                            node_t::copy_inner_replace_merged(
                                node, bit, offset, child),
                            true
                        };
                    } catch (...) {
                        node_t::delete_deep_shift(child, shift + B);
                        throw;
                    }
                }
            } else {
                return {
                    node_t::copy_inner_insert_value(node, bit, std::move(v)),
                    true
                };
            }
        }
    }

    champ add(T v) const
    {
        auto hash = Hash{}(v);
        auto res = do_add(root, std::move(v), hash, 0);
        auto new_size = size + (res.second ? 1 : 0);
        return { res.first, new_size };
    }

    template <typename Project, typename Default, typename Combine,
              typename K, typename Fn>
    std::pair<node_t*, bool>
    do_update(node_t* node, K&& k, Fn&& fn,
              hash_t hash, shift_t shift) const
    {
        if (shift == max_shift<B>) {
            auto fst = node->collisions();
            auto lst = fst + node->collision_count();
            for (; fst != lst; ++fst)
                if (Equal{}(*fst, k))
                    return {
                        node_t::copy_collision_replace(
                            node, fst, Combine{}(std::forward<K>(k),
                                                 std::forward<Fn>(fn)(
                                                     Project{}(*fst)))),
                        false
                    };
            return {
                node_t::copy_collision_insert(
                    node, Combine{}(std::forward<K>(k),
                                    std::forward<Fn>(fn)(
                                        Default{}()))),
                true
            };
        } else {
            auto idx = (hash & (mask<B> << shift)) >> shift;
            auto bit = 1 << idx;
            if (node->nodemap() & bit) {
                auto offset = popcount(node->nodemap() & (bit - 1));
                auto result = do_update<Project, Default, Combine>(
                    node->children() [offset], k, std::forward<Fn>(fn),
                    hash, shift + B);
                try {
                    result.first = node_t::copy_inner_replace(
                        node, offset, result.first);
                    return result;
                } catch (...) {
                    node_t::delete_deep_shift(result.first, shift + B);
                    throw;
                }
            } else if (node->datamap() & bit) {
                auto offset = popcount(node->datamap() & (bit - 1));
                auto val    = node->values() + offset;
                if (Equal{}(*val, k))
                    return {
                        node_t::copy_inner_replace_value(
                            node, offset, Combine{}(std::forward<K>(k),
                                                    std::forward<Fn>(fn)(
                                                        Project{}(*val)))),
                        false
                    };
                else {
                    auto child = node_t::make_merged(
                        shift + B, Combine{}(std::forward<K>(k),
                                             std::forward<Fn>(fn)(
                                                 Default{}())),
                        hash, *val, Hash{}(*val));
                    try {
                        return {
                            node_t::copy_inner_replace_merged(
                                node, bit, offset, child),
                            true
                        };
                    } catch (...) {
                        node_t::delete_deep_shift(child, shift + B);
                        throw;
                    }
                }
            } else {
                return {
                    node_t::copy_inner_insert_value(
                        node, bit, Combine{}(std::forward<K>(k),
                                             std::forward<Fn>(fn)(
                                                 Default{}()))),
                    true
                };
            }
        }
    }

    template <typename Project, typename Default, typename Combine,
              typename K, typename Fn>
    champ update(const K& k, Fn&& fn) const
    {
        auto hash = Hash{}(k);
        auto res = do_update<Project, Default, Combine>(
            root, k, std::forward<Fn>(fn), hash, 0);
        auto new_size = size + (res.second ? 1 : 0);
        return { res.first, new_size };
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
            T*      singleton;
            node_t* tree;
        };

        kind_t kind;
        data_t data;

        sub_result()          : kind{nothing}   {};
        sub_result(T* x)      : kind{singleton} { data.singleton = x; };
        sub_result(node_t* x) : kind{tree}      { data.tree = x; };
    };

    template <typename K>
    sub_result do_sub(node_t* node, const K& k, hash_t hash, shift_t shift) const
    {
        if (shift == max_shift<B>) {
            auto fst = node->collisions();
            auto lst = fst + node->collision_count();
            for (auto cur = fst; cur != lst; ++cur)
                if (Equal{}(*cur, k))
                    return node->collision_count() > 2
                        ? node_t::copy_collision_remove(node, cur)
                        : sub_result{fst + (cur == fst)};
            return {};
        } else {
            auto idx = (hash & (mask<B> << shift)) >> shift;
            auto bit = 1 << idx;
            if (node->nodemap() & bit) {
                auto offset = popcount(node->nodemap() & (bit - 1));
                auto result = do_sub(node->children() [offset],
                                     k, hash, shift + B);
                switch (result.kind) {
                case sub_result::nothing:
                    return {};
                case sub_result::singleton:
                    return node->datamap() == 0 &&
                           popcount(node->nodemap()) == 1 &&
                           shift > 0
                        ? result
                        : node_t::copy_inner_replace_inline(
                            node, bit, offset, *result.data.singleton);
                case sub_result::tree:
                    try {
                        return node_t::copy_inner_replace(node, offset,
                                                          result.data.tree);
                    } catch (...) {
                        node_t::delete_deep_shift(result.data.tree, shift + B);
                        throw;
                    }
                }
            } else if (node->datamap() & bit) {
                auto offset = popcount(node->datamap() & (bit - 1));
                auto val    = node->values() + offset;
                if (Equal{}(*val, k)) {
                    auto nv = popcount(node->datamap());
                    if (node->nodemap() || nv > 2)
                        return node_t::copy_inner_remove_value(node, bit, offset);
                    else if (nv == 2) {
                        return shift > 0
                            ? sub_result{node->values() + !offset}
                            : node_t::make_inner_n(0,
                                                  node->datamap() & ~bit,
                                                  node->values()[!offset]);
                    } else {
                        assert(shift == 0);
                        return empty.root->inc();
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
        auto res = do_sub(root, k, hash, 0);
        switch (res.kind) {
        case sub_result::nothing:
            return *this;
        case sub_result::tree:
            return {
                res.data.tree,
                size - 1
            };
        default:
            IMMER_UNREACHABLE;
        }
    }

    template <typename Eq=Equal>
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
            if (a->nodemap() != b->nodemap() ||
                a->datamap() != b->datamap())
                return false;
            auto n = popcount(a->nodemap());
            for (auto i = count_t{}; i < n; ++i)
                if (!equals_tree<Eq>(a->children()[i], b->children()[i], depth + 1))
                    return false;
            auto nv = popcount(a->datamap());
            return equals_values<Eq>(a->values(), b->values(), nv);
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
        good: continue;
        }
        return true;
    }
};

template <typename T, typename H, typename Eq, typename MP, bits_t B>
const champ<T, H, Eq, MP, B> champ<T, H, Eq, MP, B>::empty = {
    node_t::make_inner_n(0),
    0,
};

} // namespace hamts
} // namespace detail
} // namespace immer
