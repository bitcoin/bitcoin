// Copyright (c) 2021 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_UTIL_RANGES_H
#define SYSCOIN_UTIL_RANGES_H

#include <algorithm>
#include <optional>

//#if __cplusplus > 201703L // C++20 compiler
//namespace ranges = std::ranges;
//#else

#define MK_RANGE(FUN) \
template <typename X, typename Z>               \
inline auto FUN(const X& ds, const Z& fn) {     \
    return std::FUN(cbegin(ds), cend(ds), fn);  \
}                                               \
template <typename X, typename Z>               \
inline auto FUN(X& ds, const Z& fn) {           \
    return std::FUN(begin(ds), end(ds), fn);    \
}


namespace ranges {
    MK_RANGE(all_of)
    MK_RANGE(any_of)
    MK_RANGE(count_if)
    MK_RANGE(find_if)

    template <typename X, typename Z>
    inline auto find_if_opt(const X& ds, const Z& fn) {
        const auto it = ranges::find_if(ds, fn);
        if (it != end(ds)) {
            return std::make_optional(*it);
        }
        return std::optional<std::decay_t<decltype(*it)>>{};
    }

}

//#endif // C++20 compiler
#endif // SYSCOIN_UTIL_RANGES_H