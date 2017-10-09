// Copyright (c) 2017 The Particl Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef POS_MINER_H
#define POS_MINER_H

#include "primitives/block.h"
#include <thread>
#include <condition_variable>
#include <atomic>
#include <vector>

class CHDWallet;
class CWallet;

class StakeThread
{
public:
    void condWaitFor(int ms);

    StakeThread() {};
    std::thread thread;
    std::condition_variable condMinerProc;
    std::mutex mtxMinerProc;
    bool fWakeMinerProc = false;
};

extern std::vector<StakeThread*> vStakeThreads;

extern std::atomic<bool> fIsStaking;

extern int nMinStakeInterval;
extern int nMinerSleep;

double GetPoSKernelPS();

bool CheckStake(CBlock *pblock);

void ShutdownThreadStakeMiner();
void WakeThreadStakeMiner(CHDWallet *pwallet);
bool ThreadStakeMinerStopped(); // replace interruption_point

void ThreadStakeMiner(size_t nThreadID, std::vector<CWallet*> &vpwallets, size_t nStart, size_t nEnd);

#endif // POS_MINER_H

