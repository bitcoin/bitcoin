// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Copyright (C) 2016 Tom Zander <tomz@freedommail.ch>
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_MINER_H
#define BITCOIN_MINER_H

#include "primitives/block.h"

#include <stdint.h>
#include <mutex>

#include <boost/thread.hpp>

class CBlockIndex;
class CChainParams;
class CReserveKey;
class CScript;
class CWallet;
namespace Consensus { struct Params; }

static const bool DEFAULT_GENERATE = false;
static const int DEFAULT_GENERATE_THREADS = 1;

static const bool DEFAULT_PRINTPRIORITY = false;

struct CBlockTemplate
{
    CBlock block;
    std::vector<CAmount> vTxFees;
    std::vector<int64_t> vTxSigOps;
};


class Mining
{
public:
    /**
     * parse /a the public address and return a script used to make it
     * a coinbase.
     * @throws runtime_error when the input is not usable
     */
    static CScript ScriptForCoinbase(const std::string &publicAddress);
    /** Run the miner threads */
    static void GenerateBitcoins(bool fGenerate, int nThreads, const CChainParams& chainparams, const std::string &GetCoinbase);
    static void Stop();

    static Mining *instance();
    Mining();
    ~Mining();

    /** Generate a new block, without valid proof-of-work */
    CBlockTemplate* CreateNewBlock(const CChainParams& chainparams) const;
    /** Modify the extranonce in a block */
    void IncrementExtraNonce(CBlock* pblock, const CBlockIndex* pindexPrev, unsigned int& nExtraNonce);
    static int64_t UpdateTime(CBlockHeader* pblock, const Consensus::Params& consensusParams, const CBlockIndex* pindexPrev);

    CScript GetCoinbase() const;
    void SetCoinbase(const CScript &coinbase);

private:
    boost::thread_group* m_minerThreads;
    static Mining *s_instance;
    mutable std::mutex m_lock;
    CScript m_coinbase;
    std::vector<unsigned char> m_coinbaseComment;

    uint256 m_hashPrevBlock;
};

#endif // BITCOIN_MINER_H
