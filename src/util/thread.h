// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_UTIL_THREAD_H
#define SYSCOIN_UTIL_THREAD_H

#include <functional>

namespace util {
/**
 * A wrapper for do-something-once thread functions.
 */
void TraceThread(const char* thread_name, std::function<void()> thread_func);

} // namespace util

#endif // SYSCOIN_UTIL_THREAD_H
