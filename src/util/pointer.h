// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_POINTER_H
#define BITCOIN_UTIL_POINTER_H

#include <memory>

namespace util {
/* Deep equality comparison helper for std::shared_ptr<T> */
template <typename T>
bool shared_ptr_equal(const std::shared_ptr<T>& lhs, const std::shared_ptr<T>& rhs)
    requires requires { *lhs == *rhs; }
{
    /* Same object or both blank */
    if (lhs == rhs) return true;
    /* Inequal initialization state */
    if (!lhs || !rhs) return false;
    /* Deep comparison */
    return *lhs == *rhs;
}

/* Deep inequality comparison helper for std::shared_ptr<T> */
template <typename T>
bool shared_ptr_not_equal(const std::shared_ptr<T>& lhs, const std::shared_ptr<T>& rhs)
    requires requires { *lhs != *rhs; }
{
    /* Same object or both blank */
    if (lhs == rhs) return false;
    /* Inequal initialization state */
    if (!lhs || !rhs) return true;
    /* Deep comparison */
    return *lhs != *rhs;
}
} // namespace util

#endif // BITCOIN_UTIL_POINTER_H
