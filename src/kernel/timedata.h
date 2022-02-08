// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_KERNEL_TIMEDATA_H
#define BITCOIN_KERNEL_TIMEDATA_H

#include <sync.h>

#include <stdint.h>

extern Mutex g_timeoffset_mutex;
extern int64_t nTimeOffset GUARDED_BY(g_timeoffset_mutex);

/** Functions to keep track of adjusted P2P time */
int64_t GetTimeOffset();
int64_t GetAdjustedTime();

#endif // BITCOIN_KERNEL_TIMEDATA_H
