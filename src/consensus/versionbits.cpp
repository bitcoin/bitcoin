// Copyright (c) 2016-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "versionbits.h"

#include "chain.h" // TODO Decouple libconsensus from CBlockIndex
#include "interfaces.h"
#include "script/interpreter.h"

using namespace Consensus;

/**
 * This can be cheaply recalculated instead cached as shown in #7575.
 */
int32_t GetUsedBits(const CVersionBitsState& versionBitsState)
{
    int32_t usedBitsMaskCache = RESERVED_BITS_MASK;

    for (int i = 0; i < MAX_VERSION_BITS_DEPLOYMENTS; ++i)
        usedBitsMaskCache |= versionBitsState.vStates[i];

    return usedBitsMaskCache;
}

/**
 * Initialize state of the deployments as DEFINED, perform some basic checks
 * and initialize the BIP9 states cache.
 * Can result in a consensus validation error if consensusParams contains deployments using incompatible bits.
 */
static void InitVersionBitsState(CVersionBitsState& versionBitsState, const Params& consensusParams)
{
    for (int i = 0; i < MAX_VERSION_BITS_DEPLOYMENTS; ++i) {
        const BIP9Deployment& deployment = consensusParams.vDeployments[i];
        assert(!(deployment.bitmask & RESERVED_BITS_MASK));
        versionBitsState.vStates[i] = DEFINED;
    }
}

static bool MinersConfirmBitUpgrade(const BIP9Deployment& deployment, const Params& consensusParams, const CBlockIndex* pindexPrev)
{
    unsigned nFound = 0;
    unsigned i = 0;
    while (i < consensusParams.nMinerConfirmationWindow && 
           nFound < consensusParams.nRuleChangeActivationThreshold && 
           pindexPrev != NULL) {

        // If the versionbit bit (bit 29) is not set, the miner wasn't using BIP9 (as it mandates the high bits to be 001...)
        if (pindexPrev->nVersion & deployment.bitmask && pindexPrev->nVersion & VERSIONBIT_BIT)
            ++nFound;
        pindexPrev = pindexPrev->pprev;
        ++i;
    }
    return nFound >= consensusParams.nRuleChangeActivationThreshold;
}

/**
 * Calculate the next state from the previous one and the header chain.
 * Preconditions: 
 * - nextVersionBitsState = versionBitsState
 * - (indexPrev->GetHeight() + 1) % consensusParams.nMinerConfirmationWindow == 0
 */
static DeploymentState CalculateNextState(DeploymentState previousState, const BIP9Deployment& deployment, const Params& consensusParams, const CBlockIndex* pindexPrev, int64_t nMedianTime, int32_t& usedBitsMaskCache)
{
    DeploymentState nextState = previousState;
    if (previousState == DEFINED && 
        deployment.nStartTime < nMedianTime && 
        !(usedBitsMaskCache & deployment.bitmask)) {

        usedBitsMaskCache |= deployment.bitmask;
        // If moved to STARTED it could potentially become LOCKED_IN or FAILED in the same round, 
        // so no need to return immediately
        nextState = STARTED;
    }

    // Check for Timeouts
    if (previousState == STARTED && deployment.nTimeout > nMedianTime)
        return  FAILED;

    if (previousState == STARTED && MinersConfirmBitUpgrade(deployment, consensusParams, pindexPrev))
        return LOCKED_IN;

    // If locked LOCKED_IN, advance to ACTIVATED and free the bit after
    // The order in which the BIPS are written in DeploymentPos (consensus/params.h) is potentially consensus critical
    // for this state: always leave older deployments at the beginning to reuse available bits immediately.
    if (previousState == LOCKED_IN) {
        // Don't use XOR to avoid thinking about an impossible case
        usedBitsMaskCache &= ~deployment.bitmask;
        return ACTIVATED;
    }

    // ACTIVATED and FAILED states are final
    return nextState;
}

/**
 * Get the last updated state for a given CBlockIndex.
 *
 * @param pindexPrev cannot be NULL.
 *   The output will be replaced with the oldest found unkown softfork deployment if any.
 */
static const CVersionBitsState* GetVersionBitsState(const CBlockIndex* pindexPrev, const Params& consensusParams, CVersionBitsCacheInterface& versionBitsCache)
{
    // Make sure the interface's function pointers aren't NULL
    assert(versionBitsCache.Get && versionBitsCache.Set); 

    // The height of the last ascendant with an updated CVersionBitsState is always 
    // a multiple of consensusParams.nMinerConfirmationWindow (see BIP9)
    const uint32_t nPeriod = consensusParams.nMinerConfirmationWindow;
    const uint32_t nTargetStateHeight = pindexPrev->nHeight - ((pindexPrev->nHeight + 1) % nPeriod);
    // We don't have to check anything during the first 2015 blocks
    if (nTargetStateHeight < nPeriod)
        return NULL;

    if (nTargetStateHeight != (uint32_t)pindexPrev->nHeight)
        pindexPrev = pindexPrev->GetAncestor(nTargetStateHeight);
    if (!pindexPrev)
        return NULL;

    // Search backards for a cached state
    const CBlockIndex* itBlockIndex = pindexPrev;
    const CVersionBitsState* pVersionBitsState = versionBitsCache.Get(itBlockIndex);
    while(!pVersionBitsState && (uint32_t)itBlockIndex->nHeight > nPeriod) {
        // Only look in index with height % nPeriod == 0
        itBlockIndex = itBlockIndex->GetAncestor(itBlockIndex->nHeight - nPeriod);
        pVersionBitsState = versionBitsCache.Get(itBlockIndex);
    }
    assert(pindexPrev);

    // Create initial state if there's not one cached
    CVersionBitsState newVersionBitsState;
    if (!pVersionBitsState) {
        InitVersionBitsState(newVersionBitsState, consensusParams);
        versionBitsCache.Set(itBlockIndex, &newVersionBitsState);
        pVersionBitsState = versionBitsCache.Get(itBlockIndex);
    }

    uint32_t nCurrentStateHeight = itBlockIndex->nHeight;
    // Calculate new states forward
    while (nCurrentStateHeight < nTargetStateHeight) {

        nCurrentStateHeight += nPeriod; // Move to the next Height that calculates a state
        itBlockIndex = pindexPrev->GetAncestor(nCurrentStateHeight);
        const int64_t nMedianTime = itBlockIndex->GetMedianTimePast();

        // Create the new state in the cache from the old one
        int32_t usedBitsMaskCache = GetUsedBits(*pVersionBitsState);
        for (int i = 0; i < MAX_VERSION_BITS_DEPLOYMENTS; ++i) {
            const BIP9Deployment& deployment = consensusParams.vDeployments[i];
            newVersionBitsState.vStates[i] = CalculateNextState(pVersionBitsState->vStates[i], deployment, consensusParams, itBlockIndex, nMedianTime, usedBitsMaskCache);
        }

        versionBitsCache.Set(itBlockIndex, &newVersionBitsState);
        pVersionBitsState = versionBitsCache.Get(itBlockIndex);
    }
    assert(nCurrentStateHeight == nTargetStateHeight);
    return pVersionBitsState;
}

unsigned int Consensus::GetFlags(const CBlockIndex* pindexPrev, const Params& consensusParams, CVersionBitsCacheInterface& versionBitsCache)
{
    unsigned int flags = SCRIPT_VERIFY_NONE;
    if (!pindexPrev)
        return flags;

    // BIP16 didn't become active until Apr 1 2012
    int64_t nBIP16SwitchTime = 1333238400;
    if (pindexPrev->nTime >= nBIP16SwitchTime)
        flags |= SCRIPT_VERIFY_P2SH;

    const Consensus::CVersionBitsState* pVersionBitsState = GetVersionBitsState(pindexPrev, consensusParams, versionBitsCache);
    if (!pVersionBitsState)
        return flags;

    if (pVersionBitsState->vStates[BIP68_BIP112] == ACTIVATED)
        flags |= SCRIPT_VERIFY_CHECKSEQUENCEVERIFY | LOCKTIME_VERIFY_SEQUENCE;

    return flags;
}

int32_t ComputeBlockVersion(const CBlockIndex* pindexPrev, const Consensus::Params& consensusParams, Consensus::CVersionBitsCacheInterface& versionBitsCache)
{
    int32_t nVersion = 0;
    const Consensus::CVersionBitsState* pVersionBitsState = GetVersionBitsState(pindexPrev, consensusParams, versionBitsCache);
    if (!pVersionBitsState)
        return nVersion;

    for (int i = 0; i < MAX_VERSION_BITS_DEPLOYMENTS; ++i)
        if (pVersionBitsState->vStates[i] == STARTED || pVersionBitsState->vStates[i] == LOCKED_IN)
            nVersion |= consensusParams.vDeployments[i].bitmask;

    return nVersion == 0 ? VERSIONBITS_LAST_OLD_BLOCK_VERSION : nVersion | VERSIONBIT_BIT;
}

std::string GetLastUnknownDeploymentWarning(const CBlockIndex* pindexPrev, const Consensus::Params& consensusParams, Consensus::CVersionBitsCacheInterface& versionBitsCache)
{
    const uint32_t unusedBits = ~(ComputeBlockVersion(pindexPrev, consensusParams, versionBitsCache) & RESERVED_BITS_MASK);
    if (unusedBits) {
        BIP9Deployment deployment;
        deployment.nStartTime = 0;
        deployment.nTimeout = std::numeric_limits<int64_t>::max();
        deployment.bitmask = unusedBits;
        int32_t usedBitsMaskCache = ~unusedBits;
        const int64_t nMedianTime = pindexPrev->GetMedianTimePast();
        if (LOCKED_IN == CalculateNextState(STARTED, deployment, consensusParams, pindexPrev, nMedianTime, usedBitsMaskCache))
            return "Warning: unknown softfork has been locked in.";
    }
    return "";
}
