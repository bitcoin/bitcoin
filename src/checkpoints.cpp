// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "checkpoints.h"

#include "main.h"
#include "uint256.h"

#include <stdint.h>

#include <boost/assign/list_of.hpp> // for 'map_list_of()'
#include <boost/foreach.hpp>

namespace Checkpoints
{
    typedef std::map<int, uint256> Bitcredit_MapCheckpoints;

    // How many times we expect transactions after the last checkpoint to
    // be slower. This number is a compromise, as it can't be accurate for
    // every system. When reindexing from a fast disk with a slow CPU, it
    // can be up to 20, while when downloading from a slow network with a
    // fast multicore CPU, it won't be much higher than 1.
    static const double BITCREDIT_SIGCHECK_VERIFICATION_FACTOR = 5.0;

    struct Bitcredit_CCheckpointData {
        const Bitcredit_MapCheckpoints *mapCheckpoints;
        int64_t nTimeLastCheckpoint;
        int64_t nTransactionsLastCheckpoint;
        double fTransactionsPerDay;
    };

    bool bitcredit_fEnabled = true;

    // What makes a good checkpoint block?
    // + Is surrounded by blocks with reasonable timestamps
    //   (no blocks before with a timestamp after, none after with
    //    timestamp before)
    // + Contains no strange transactions
    static Bitcredit_MapCheckpoints bitcredit_mapCheckpoints =
        boost::assign::map_list_of
        ( 0, uint256("0x00000000983b1b95f3b177e948e48591c939c17e8764cb72008ee0cf21308a45"))
        ( 7123, uint256("0x00000000170303448618e185c1c60702b1e1ea850db18d902d98c563a4e2209b"))
        ;
    static const Bitcredit_CCheckpointData bitcredit_data = {
        &bitcredit_mapCheckpoints,
		1428973422, // * UNIX timestamp of last checkpoint block
        30000,   // * total number of transactions between genesis and last checkpoint
                    //   (the tx=... number in the SetBestChain debug.log lines)
        1000.0     // * estimated number of transactions per day after checkpoint
    };

    //TODO - Add checkpoints
    static Bitcredit_MapCheckpoints bitcredit_mapCheckpointsTestnet =
        boost::assign::map_list_of
        ( 0, uint256("0x0000000092999ac46443e5fbdc4510ba3bc7e9826b5bda69fe4e8ba0476951ce"))
        ;
    static const Bitcredit_CCheckpointData bitcredit_dataTestnet = {
        &bitcredit_mapCheckpointsTestnet,
        0,
        0,
        0
    };

    static Bitcredit_MapCheckpoints bitcredit_mapCheckpointsRegtest =
        boost::assign::map_list_of
        ( 0, uint256("0x0000013459dd9e819a9fff403f00f87c5bc1b8e4259d204054f97ac8d18f0a3c"))
        ;
    static const Bitcredit_CCheckpointData bitcredit_dataRegtest = {
        &bitcredit_mapCheckpointsRegtest,
        0,
        0,
        0
    };

    const Bitcredit_CCheckpointData &Bitcredit_Checkpoints() {
        if (Bitcredit_Params().NetworkID() == CChainParams::TESTNET)
            return bitcredit_dataTestnet;
        else if (Bitcredit_Params().NetworkID() == CChainParams::MAIN)
            return bitcredit_data;
        else
            return bitcredit_dataRegtest;
    }

    bool Bitcredit_CheckBlock(int nHeight, const uint256& hash)
    {
        if (!bitcredit_fEnabled)
            return true;

        const Bitcredit_MapCheckpoints& checkpoints = *Bitcredit_Checkpoints().mapCheckpoints;

        Bitcredit_MapCheckpoints::const_iterator i = checkpoints.find(nHeight);
        if (i == checkpoints.end()) return true;
        return hash == i->second;
    }

    // Guess how far we are in the verification process at the given block index
    double Bitcredit_GuessVerificationProgress(Credits_CBlockIndex *pindex, bool fSigchecks) {
        if (pindex==NULL)
            return 0.0;

        int64_t nNow = time(NULL);

        double fSigcheckVerificationFactor = fSigchecks ? BITCREDIT_SIGCHECK_VERIFICATION_FACTOR : 1.0;
        double fWorkBefore = 0.0; // Amount of work done before pindex
        double fWorkAfter = 0.0;  // Amount of work left after pindex (estimated)
        // Work is defined as: 1.0 per transaction before the last checkpoint, and
        // fSigcheckVerificationFactor per transaction after.

        const Bitcredit_CCheckpointData &data = Bitcredit_Checkpoints();

        if (pindex->nChainTx <= data.nTransactionsLastCheckpoint) {
            double nCheapBefore = pindex->nChainTx;
            double nCheapAfter = data.nTransactionsLastCheckpoint - pindex->nChainTx;
            double nExpensiveAfter = (nNow - data.nTimeLastCheckpoint)/86400.0*data.fTransactionsPerDay;
            fWorkBefore = nCheapBefore;
            fWorkAfter = nCheapAfter + nExpensiveAfter*fSigcheckVerificationFactor;
        } else {
            double nCheapBefore = data.nTransactionsLastCheckpoint;
            double nExpensiveBefore = pindex->nChainTx - data.nTransactionsLastCheckpoint;
            double nExpensiveAfter = (nNow - pindex->nTime)/86400.0*data.fTransactionsPerDay;
            fWorkBefore = nCheapBefore + nExpensiveBefore*fSigcheckVerificationFactor;
            fWorkAfter = nExpensiveAfter*fSigcheckVerificationFactor;
        }

        return fWorkBefore / (fWorkBefore + fWorkAfter);
    }

    int Bitcredit_GetTotalBlocksEstimate()
    {
        if (!bitcredit_fEnabled)
            return 0;

        const Bitcredit_MapCheckpoints& checkpoints = *Bitcredit_Checkpoints().mapCheckpoints;

        return checkpoints.rbegin()->first;
    }

    Credits_CBlockIndex* Bitcredit_GetLastCheckpoint(const std::map<uint256, Credits_CBlockIndex*>& mapBlockIndex)
    {
        if (!bitcredit_fEnabled)
            return NULL;

        const Bitcredit_MapCheckpoints& checkpoints = *Bitcredit_Checkpoints().mapCheckpoints;

        BOOST_REVERSE_FOREACH(const Bitcredit_MapCheckpoints::value_type& i, checkpoints)
        {
            const uint256& hash = i.second;
            std::map<uint256, Credits_CBlockIndex*>::const_iterator t = mapBlockIndex.find(hash);
            if (t != mapBlockIndex.end())
                return t->second;
        }
        return NULL;
    }
}
