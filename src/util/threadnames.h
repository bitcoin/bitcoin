// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITGREEN_UTIL_THREADNAMES_H
#define BITGREEN_UTIL_THREADNAMES_H

#include <string>

namespace util {
//! Rename a thread both in terms of an internal (in-memory) name as well
//! as its system thread name.
void ThreadRename(std::string&&);

//! Get the thread's internal (in-memory) name; used e.g. for identification in
//! logging.
const std::string& ThreadGetInternalName();

} // namespace util

namespace ctpl {
    class thread_pool;
}
void RenameThreadPool(ctpl::thread_pool& tp, const char* baseName);

#endif // BITGREEN_UTIL_THREADNAMES_H
