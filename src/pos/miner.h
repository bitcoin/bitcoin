// Copyright (c) 2017 The Particl Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef POS_MINER_H
#define POS_MINER_H

#include "wallet/hdwallet.h"

#include <thread>

extern std::thread threadStakeMiner;

extern bool fIsStaking;

extern int nMinStakeInterval;
extern int nMinerSleep;
extern int64_t nLastCoinStakeSearchInterval;
extern int64_t nLastCoinStakeSearchTime;

bool CheckStake(CBlock *pblock);

void ShutdownThreadStakeMiner();
void WakeThreadStakeMiner();
bool ThreadStakeMinerStopped(); // replace interruption_point

void ThreadStakeMiner(CHDWallet *pwallet);

#endif // POS_MINER_H

