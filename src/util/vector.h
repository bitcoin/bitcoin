// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_VECTOR_H
#define BITCOIN_UTIL_VECTOR_H

#include <vector>
#include <type_traits>

/** Emplace back multiple elements to a container. */
template<typename V>
inline void EmplaceBackMany(V& vec) {}
template<typename V, typename Arg, typename... Args>
inline void EmplaceBackMany(V& vec, Arg&& arg, Args&&... args)
{
    vec.emplace_back(std::forward<Arg>(arg));
    EmplaceBackMany(vec, std::forward<Args>(args)...);
}

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
    EmplaceBackMany(ret, std::forward<Args>(args)...);
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

#endif // BITCOIN_UTIL_VECTOR_H
