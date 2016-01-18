// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#define VERSIONBITS_UNIT_TEST

#include "chain.h"
#include "consensus/blockruleindex.h"
#include "consensus/versionbits.h"
#include "primitives/block.h"
#include "test/test_bitcoin.h"

#include <bitset>
#include <boost/test/unit_test.hpp>
#include <iomanip>
#include <sstream>
#include <stdlib.h>
#include <string>
#include <time.h>

using namespace Consensus::VersionBits;
using namespace std;

const int NBITS = MAX_BIT + 1 - MIN_BIT;

const int ACTIVATION_INTERVAL = 2016;

SoftForkDeployments g_deployments;

class BitCounter
{
public:
    BitCounter(const SoftForkDeployments& g_deployments) :
        m_g_deployments(g_deployments)
    {
        Clear();
    }

    void Clear()
    {
        for (int i = 0; i < NBITS; i++)
            m_bitCounts[i] = 0;
    }

    void CountBits(int nVersion, uint32_t nTime = 0)
    {
        for (int i = MIN_BIT; i <= MAX_BIT; i++)
        {
            if (((nVersion >> i) & 0x1) && (m_g_deployments.GetAssignedRule(i, nTime) != Consensus::NO_RULE))
                m_bitCounts[i - MIN_BIT]++;
        }
    }

    int GetCountForBit(int bit) const
    {
        return m_bitCounts[bit - MIN_BIT];
    }

    string ToString()
    {
        stringstream ss;
        for (int i = 0; i < NBITS; i++)
        {
            if (m_bitCounts[i + MIN_BIT])
               ss << setw(4) << right << i + MIN_BIT << ": " << setw(5) << right << m_bitCounts[i + MIN_BIT] << endl;
        }

        return ss.str();
    }

private:
    int m_bitCounts[NBITS];
    const SoftForkDeployments& m_g_deployments;
};

class VersionGenerator
{
public:
    VersionGenerator() { ClearBitProbabilities(); }

    void ClearBitProbabilities()
    {
        for (int i = 0; i < NBITS; i++)
            m_bitProbabilities[i] = 0;
    }

    void SetBitProbability(int bit, int probability)
    {
        m_bitProbabilities[bit - MIN_BIT] = probability;
    }

    int Generate() const
    {
        int nVersion = VERSION_HIGH_BITS;
        for (int i = 0; i < NBITS; i++)
        {
            if ((rand() % ACTIVATION_INTERVAL) <= (m_bitProbabilities[i] - 1))
                nVersion |= 0x1 << (MIN_BIT + i);
        }

        return nVersion;
    }
    
private:
    int m_bitProbabilities[NBITS]; // in units of 1/ACTIVATION_INTERVAL
};


std::string RuleStateToString(RuleState state)
{
    switch (state)
    {
    case UNDEFINED:
        return "UNDEFINED";

    case DEFINED:
        return "DEFINED";

    case LOCKED_IN:
        return "LOCKED_IN";

    case ACTIVE:
        return "ACTIVE";

    case FAILED:
        return "FAILED";

    default:
        return "N/A";
    }
}
void StateChanged(const CBlockIndex* pblockIndex, const SoftFork* psoftFork, RuleState prevState, RuleState newState, int bitCount)
{
    int bit = psoftFork->GetBit();
    bool isBitSet = (pblockIndex->pprev->nVersion >> bit) & 0x1;

    BOOST_TEST_MESSAGE("=============");
    BOOST_TEST_MESSAGE("STATE CHANGED - height: " << pblockIndex->nHeight << " median time: " << pblockIndex->GetMedianTimePast()
         << " bit: " << psoftFork->GetBit() << " (" << (isBitSet ? "true" : "false") << ") rule: " << psoftFork->GetRule());
    BOOST_TEST_MESSAGE("  " << RuleStateToString(prevState) << " -> " << RuleStateToString(newState));
    BOOST_TEST_MESSAGE("    " << bitCount << "/" << psoftFork->GetThreshold());

    if ((prevState == DEFINED) && (newState != LOCKED_IN) && (newState != FAILED))
        throw runtime_error("Invalid state transition.");

    if ((prevState == LOCKED_IN) && (newState != ACTIVE))
        throw runtime_error("Invalid state transition.");

    if ((prevState == ACTIVE) || (prevState == FAILED))
        throw runtime_error("Invalid state transition.");

    if ((newState == LOCKED_IN) && (prevState != DEFINED))
        throw runtime_error("Invalid state transition.");

    if ((newState == ACTIVE) && (prevState != LOCKED_IN))
        throw runtime_error("Invalid state transition.");

    if ((newState == LOCKED_IN) && (bitCount < psoftFork->GetThreshold()))
        throw runtime_error("Insufficient bit count for lock-in.");
}

void CompareRuleStates(const CBlockIndex* pblockIndex, const RuleStates& prevStates, const RuleStates& newStates, const BitCounter& bitCounter)
{
    for (RuleStates::const_iterator newIt = newStates.begin(); newIt != newStates.end(); ++newIt)
    {
        const SoftFork* psoftFork = g_deployments.GetSoftFork(newIt->first);
        if (!psoftFork)
            throw runtime_error("Invalid internal state.");

        int bitCount = bitCounter.GetCountForBit(psoftFork->GetBit());

        RuleStates::const_iterator prevIt = prevStates.find(newIt->first);
        if (prevIt == prevStates.end())
        {
            StateChanged(pblockIndex, psoftFork, UNDEFINED, newIt->second, bitCount);
        }
        else if (newIt->second != prevIt->second)
        {
            StateChanged(pblockIndex, psoftFork, prevIt->second, newIt->second, bitCount);
        }
        else if ((pblockIndex->nHeight % ACTIVATION_INTERVAL == 0) && (newIt->second == DEFINED) && (bitCount >= psoftFork->GetThreshold()))
        {
            BOOST_TEST_MESSAGE("bit count: " << bitCount << "/" << psoftFork->GetThreshold());
            throw runtime_error("Threshold exceeded but lock-in did not occur.");
        }
    }

    for (RuleStates::const_iterator prevIt = prevStates.begin(); prevIt != prevStates.end(); ++prevIt)
    { 
        RuleStates::const_iterator newIt = newStates.find(prevIt->first);
        if (newIt == newStates.end())
        {
            const SoftFork* psoftFork = g_deployments.GetSoftFork(prevIt->first);
            if (!psoftFork)
                throw runtime_error("Invalid internal state.");

            int bitCount = bitCounter.GetCountForBit(psoftFork->GetBit());

            StateChanged(pblockIndex, psoftFork, prevIt->second, UNDEFINED, bitCount);
        }
    }
}

typedef std::map<uint256, CBlockIndex*> BlockMap;
BlockMap g_blockIndexMap;


BOOST_FIXTURE_TEST_SUITE(versionbits_tests, BasicTestingSetup)


std::string ToString(const CBlockIndex* pblockIndex, const BlockRuleIndex& blockRuleIndex)
{
    using namespace Consensus::VersionBits;

    std::stringstream ss;
    ss << "Height: " << setw(7) << right << pblockIndex->nHeight;
    ss << " Hash: " << pblockIndex->phashBlock->ToString();
    ss << " Version: 0x" << hex << pblockIndex->nVersion;
    ss << " Time: " << dec << pblockIndex->nTime;
    const RuleStates& ruleStates = blockRuleIndex.GetRuleStates(pblockIndex);
    for (RuleStates::const_iterator it = ruleStates.begin(); it != ruleStates.end(); ++it)
    {
        ss << endl << setw(4) << right << it->first << ": " << RuleStateToString(it->second);
    }

    ss << endl;;
    return ss.str();
}

CBlockIndex* NewBlock(int nVersion, unsigned int nTime, BlockRuleIndex& blockRuleIndex, CBlockIndex* pparent = NULL, BitCounter* pbitCounter = NULL)
{
    CBlockHeader blockHeader;
    blockHeader.nVersion = nVersion;
    blockHeader.nTime = nTime;
    blockHeader.hashPrevBlock = pparent ? pparent->GetBlockHash() : uint256();

    CBlockIndex* pblockIndex = new CBlockIndex(blockHeader);
    pblockIndex->pprev = pparent;
    pblockIndex->phashBlock = new uint256(blockHeader.GetHash());
    pblockIndex->nHeight = pparent ? pparent->nHeight + 1 : 0;
    g_blockIndexMap[pblockIndex->GetBlockHash()] = pblockIndex;
    blockRuleIndex.InsertBlockIndex(pblockIndex);

    if (pbitCounter)
    {
        if (pparent)
        {
            RuleStates prevRuleStates   = blockRuleIndex.GetRuleStates(pparent);
            RuleStates newRuleStates    = blockRuleIndex.GetRuleStates(pblockIndex);
            CompareRuleStates(pblockIndex, prevRuleStates, newRuleStates, *pbitCounter);
        }
        pbitCounter->CountBits(nVersion, pblockIndex->GetMedianTimePast());
    }

    return pblockIndex;
}

CBlockIndex* Generate(CBlockIndex* ptip, int nBlocks, int nTimeIncrement, BlockRuleIndex& blockRuleIndex, const VersionGenerator& vgen, BitCounter* pbitCounter = NULL, bool showOutput = false)
{
    for (int i = 0; i < nBlocks; i++)
    {
        ptip = NewBlock(vgen.Generate(), ptip->nTime + nTimeIncrement, blockRuleIndex, ptip, pbitCounter);

        if (showOutput)
        {
            stringstream ss;
            ss << ToString(ptip, blockRuleIndex);

            if (pbitCounter)
                ss << endl << pbitCounter->ToString();

            BOOST_TEST_MESSAGE(ss.str());
        }
    }

    return ptip;
}

inline void CleanUp()
{
    for (BlockMap::iterator it = g_blockIndexMap.begin(); it != g_blockIndexMap.end(); ++it)
    {
        if (it->second)
        {
            if (it->second->phashBlock) delete it->second->phashBlock;
            delete it->second;
        }
    }
}

BOOST_AUTO_TEST_CASE( deployments )
{
    try
    {
        g_deployments.Clear();
        g_deployments.AddSoftFork(0, 1, 500, 10000, 100000);

        try
        {
            // Test conflicting bit, overlapping deployment window
            g_deployments.AddSoftFork(0, 2, 950, 30, 20000);
            BOOST_FAIL("Bit conflict not detected for overlapping deployment.");
        }
        catch(exception& e) { }

        try
        {
            // Test conflicting bit, overlapping expiration window
            g_deployments.AddSoftFork(0, 3, 500, 70000, 130000);
            BOOST_FAIL("Bit conflict not detected for overlapping expiration.");
        }
        catch(exception& e) { }

        try
        {
            // Test conflicting bit, inner time window containment
            g_deployments.AddSoftFork(0, 4, 500, 60000, 80000);
            BOOST_FAIL("Bit conflict not detected for inner time window containment.");
        }
        catch(exception& e) { }

        try
        {
            // Test conflicting bit, outer time window containment
            g_deployments.AddSoftFork(0, 5, 500, 6000, 800000);
            BOOST_FAIL("Bit conflict not detected for outer time window containment.");
        }
        catch(exception& e) { }
    }
    catch (exception& e)
    {
        BOOST_FAIL("Error: " << e.what());
    }
}

BOOST_AUTO_TEST_CASE( transitions )
{
    BlockRuleIndex blockRuleIndex;
    srand(time(NULL));

    try
    {
        g_deployments.Clear();

        RuleStates ruleStates;
        BitCounter bitCounter(g_deployments);
        VersionGenerator vgen;

        // Create genesis block and generate a full retarget interval
        CBlockIndex* pstart = NewBlock(0, time(NULL), blockRuleIndex);

        // Set version distribution and add g_deployments
        vgen.SetBitProbability(0, 100);

        vgen.SetBitProbability(5, 900);
        g_deployments.AddSoftFork(5, 1, 900, 0, 0xffffffff);

        vgen.SetBitProbability(6, 1034);
        g_deployments.AddSoftFork(6, 2, 1034, 0, 0xffffffff);

        blockRuleIndex.SetSoftForkDeployments(ACTIVATION_INTERVAL, &g_deployments);

        pstart = Generate(pstart, 2016, 100, blockRuleIndex, vgen);

        ////////////////////////////////////
        // TEST 1: DEFINED -> LOCKED_IN
        BOOST_TEST_MESSAGE("============================");
        BOOST_TEST_MESSAGE("TEST 1: DEFINED -> LOCKED_IN");

        for (int i = 0; i < 20; i++)
        {
            bitCounter.Clear();
            bitCounter.CountBits(pstart->nVersion, pstart->GetMedianTimePast());

            ruleStates.clear();
            ruleStates[1] = DEFINED;
            ruleStates[2] = DEFINED;
            blockRuleIndex.InsertBlockIndexWithRuleStates(pstart, ruleStates);

            // Generate another 2020 blocks
            Generate(pstart, 2020, 100, blockRuleIndex, vgen, &bitCounter);//, true);
        }

        ////////////////////////////////////
        // TEST 2: LOCKED_IN -> ACTIVE
        BOOST_TEST_MESSAGE("===========================");
        BOOST_TEST_MESSAGE("TEST 2: LOCKED_IN -> ACTIVE");

        for (int i = 0; i < 20; i++)
        {
            bitCounter.Clear();
            bitCounter.CountBits(pstart->nVersion, pstart->GetMedianTimePast());

            ruleStates.clear();
            ruleStates[1] = LOCKED_IN;
            ruleStates[2] = DEFINED;
            blockRuleIndex.InsertBlockIndexWithRuleStates(pstart, ruleStates);

            // Generate another 2020 blocks
            Generate(pstart, 2020, 100, blockRuleIndex, vgen, &bitCounter);//, true);
        }

        ////////////////////////////////////////////////
        // TEST 3: FAILED -> FAILED and ACTIVE -> ACTIVE
        BOOST_TEST_MESSAGE("=============================================");
        BOOST_TEST_MESSAGE("TEST 3: FAILED -> FAILED and ACTIVE -> ACTIVE");

        for (int i = 0; i < 20; i++)
        {
            bitCounter.Clear();
            bitCounter.CountBits(pstart->nVersion, pstart->GetMedianTimePast());

            ruleStates.clear();
            ruleStates[1] = FAILED;
            ruleStates[2] = ACTIVE;
            blockRuleIndex.InsertBlockIndexWithRuleStates(pstart, ruleStates);

            // Generate another 2020 blocks
            Generate(pstart, 2020, 100, blockRuleIndex, vgen, &bitCounter);//, true);
        }

        ////////////////////////////////////////////////////
        // TEST 4: DEFINED -> LOCKED_IN or DEFINED -> FAILED
        BOOST_TEST_MESSAGE("=================================================");
        BOOST_TEST_MESSAGE("TEST 4: DEFINED -> LOCKED_IN or DEFINED -> FAILED");

        g_deployments.Clear();
        vgen.SetBitProbability(10, 800);
        g_deployments.AddSoftFork(10, 3, 400, 0, pstart->nTime + (100 * ACTIVATION_INTERVAL)/2);

        blockRuleIndex.SetSoftForkDeployments(ACTIVATION_INTERVAL, &g_deployments);

        for (int i = 0; i < 20; i++)
        {
            bitCounter.Clear();
            bitCounter.CountBits(pstart->nVersion, pstart->GetMedianTimePast());

            ruleStates.clear();
            ruleStates[3] = DEFINED;
            blockRuleIndex.InsertBlockIndexWithRuleStates(pstart, ruleStates);

            // Generate another 2020 blocks
            Generate(pstart, 2020, 100, blockRuleIndex, vgen, &bitCounter);//, true);
        }

    }
    catch (exception& e)
    {
        BOOST_FAIL("Error: " << e.what());
    }
}

BOOST_AUTO_TEST_SUITE_END()
