// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "corewallet/coreinterface.h"

#include "main.h"
#include "net.h"

namespace CoreWallet
{
//TODO: create a proper interface over ZMQ
const CBlockIndex* CoreInterface::GetBlockIndex(uint256 blockhash, bool inActiveChain) const
{
    //for now, this interacts directly with the mapBlockIndex in global scope
    LOCK(cs_main);
    BlockMap::iterator mi = mapBlockIndex.find(blockhash);
    if (mi == mapBlockIndex.end())
        return NULL;
    CBlockIndex* pindex = (*mi).second;
    if( (!inActiveChain && pindex) || (inActiveChain && pindex && chainActive.Contains(pindex)))
       return pindex;
    return NULL;
}

bool CoreInterface::CheckFinalTx(const CTransaction &tx) const
{
    LOCK(cs_main);
    return ::CheckFinalTx(tx);
}

int CoreInterface::GetActiveChainHeight() const
{
    LOCK(cs_main);
    return chainActive.Height();
}

bool CoreInterface::MempoolExists(uint256 hash) const
{
    LOCK(mempool.cs);
    return mempool.exists(hash);
}

bool CoreInterface::AcceptToMemoryPool(CValidationState &state, const CTransaction &tx, bool fLimitFree, bool fRejectAbsurdFee)
{
    LOCK(cs_main);
    return ::AcceptToMemoryPool(mempool, state, tx, fLimitFree, NULL, fRejectAbsurdFee);
}

void CoreInterface::RelayTransaction(const CTransaction& tx) const
{
    ::RelayTransaction(tx);
}

CFeeRate CoreInterface::GetMinRelayTxFee() const
{
    return ::minRelayTxFee;
}

unsigned int CoreInterface::GetMaxStandardTxSize() const
{
    return MAX_STANDARD_TX_SIZE;
}

CAmount CoreInterface::EstimateFee(int confirmationTarget, size_t txSize) const
{
    return mempool.estimateFee(confirmationTarget).GetFee(txSize);
}

double CoreInterface::EstimatePriority(int confirmationTarget) const
{
    return mempool.estimatePriority(confirmationTarget);
}

bool CoreInterface::AllowFree(double dPriority) const
{
    return AllowFree(dPriority);
}

} // end namespace