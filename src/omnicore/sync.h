// Copyright (c) 2017-2020 The BitcoinHD Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_OMINICORE_SYNC_H
#define BITCOIN_OMINICORE_SYNC_H

#include <sync.h>

//! Global lock for state objects
extern CCriticalSection cs_tally;

#endif // BITCOIN_OMINICORE_SYNC_H