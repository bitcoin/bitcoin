// Copyright (c) 2019-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_VECTOR_H
#define BITCOIN_UTIL_VECTOR_H

#include <functional>
#include <initializer_list>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

/** Construct a vector with the specified elements.
 *
 * This is preferable over the list initializing constructor of std::vector:
 * - It automatically infers the element type from its arguments.
 * - If any arguments are rvalue references, they will be moved into the vector
 *   (list initialization always copies).
 */
template<typename... Args>
inline std::vector<typename std::common_type<Args...>::type> Vector(Args&&... args)
{
    std::vector<typename std::common_type<Args...>::type> ret;
    ret.reserve(sizeof...(args));
    // The line below uses the trick from https://www.experts-exchange.com/articles/32502/None-recursive-variadic-templates-with-std-initializer-list.html
    (void)std::initializer_list<int>{(ret.emplace_back(std::forward<Args>(args)), 0)...};
    return ret;
}

/** Concatenate two vectors, moving elements. */
template<typename V>
inline V Cat(V v1, V&& v2)
{
    v1.reserve(v1.size() + v2.size());
    for (auto& arg : v2) {
        v1.push_back(std::move(arg));
    }
    return v1;
}

/** Concatenate two vectors. */
template<typename V>
inline V Cat(V v1, const V& v2)
{
    v1.reserve(v1.size() + v2.size());
    for (const auto& arg : v2) {
        v1.push_back(arg);
    }
    return v1;
}

/** Clear a vector (or std::deque) and release its allocated memory. */
template<typename V>
inline void ClearShrink(V& v) noexcept
{
    // There are various ways to clear a vector and release its memory:
    //
    // 1. V{}.swap(v)
    // 2. v = V{}
    // 3. v = {}; v.shrink_to_fit();
    // 4. v.clear(); v.shrink_to_fit();
    //
    // (2) does not appear to release memory in glibc debug mode, even if v.shrink_to_fit()
    // follows. (3) and (4) rely on std::vector::shrink_to_fit, which is only a non-binding
    // request. Therefore, we use method (1).

    V{}.swap(v);
}

template<typename V, typename L>
inline std::optional<V> FindFirst(const std::vector<V>& vec, const L fnc)
{
    for (const auto& el : vec) {
        if (fnc(el)) {
            return el;
        }
    }
    return std::nullopt;
}

#endif // BITCOIN_UTIL_VECTOR_H
