// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <validation_globals.h>

/** Constant stuff for coinbase transactions we create: */
CScript COINBASE_FLAGS;

std::atomic_bool g_is_mempool_loaded{false};
const std::string strMessageMagic = "Bitcoin Signed Message:\n";

CWaitableCriticalSection g_best_block_mutex;
CConditionVariable g_best_block_cv;
uint256 g_best_block;
std::atomic_bool fImporting(false);
std::atomic_bool fReindex(false);
int nScriptCheckThreads = 0;
bool fIsBareMultisigStd = DEFAULT_PERMIT_BAREMULTISIG;
bool fRequireStandard = true;
bool fCheckBlockIndex = false;
bool fCheckpointsEnabled = DEFAULT_CHECKPOINTS_ENABLED;
size_t nCoinCacheUsage = 5000 * 300;

