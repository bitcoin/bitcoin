// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_THREADUTIL_H
#define BITCOIN_THREADUTIL_H

#include <string>

namespace thread_util
{
    /**
     * Rename a thread both in terms of an internal (in-memory) name as well
     * as its system thread name.
     */
    void Rename(const std::string&);

    /**
     * Get the thread's internal (in-memory) name; used e.g. for identification in
     * logging.
     */
    std::string GetInternalName();

} // namespace thread_util

#endif // BITCOIN_THREADUTIL_H
