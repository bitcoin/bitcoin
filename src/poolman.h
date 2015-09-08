// Copyright (c) 2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POOLMAN_H
#define BITCOIN_POOLMAN_H

#include "scheduler.h"

extern void InitTxMempoolJanitor(CScheduler& scheduler);

#endif // BITCOIN_POOLMAN_H
