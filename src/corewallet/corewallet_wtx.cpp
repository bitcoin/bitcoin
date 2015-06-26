// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "corewallet/corewallet_wallet.h"
#include "corewallet/corewallet_wtx.h"

#include "consensus/validation.h"
#include "consensus/consensus.h"

#include <algorithm>

namespace CoreWallet {

bool WalletTx::RelayWalletTransaction()
{
    assert(pwallet->GetBroadcastTransactions());
    if (!IsCoinBase())
    {
        if (GetDepthInMainChain() == 0) {
            LogPrintf("Relaying wtx %s\n", GetHash().ToString());
            //RelayTransaction((CTransaction)*this);
            return true;
        }
    }
    return false;
}

CAmount WalletTx::GetDebit(const isminefilter& filter) const
{
    if (vin.empty())
        return 0;

    CAmount debit = 0;
    CAmount cacheOut;
    if (GetCache(CREDIT_DEBIT_TYPE_DEBIT, filter, cacheOut))
        debit += cacheOut;
    else
    {
        cacheOut = pwallet->GetDebit(*this, filter);
        SetCache(CREDIT_DEBIT_TYPE_DEBIT, filter, cacheOut);
        debit += cacheOut;
    }
    
    return debit;
}

int64_t WalletTx::GetTxTime() const
{
    int64_t n = nTimeSmart;
    return n ? n : nTimeReceived;
}

int WalletTx::GetDepthInMainChainInternal(const CBlockIndex* &pindexRet) const
{
    if (hashBlock.IsNull() || nIndex == -1)
        return 0;

    // Find the block it claims to be in
    const CBlockIndex* pindex = pwallet->GetBlockIndex(hashBlock, true);
    if (!pindex)
        return 0;

    // Make sure the merkle branch connects to this block
    if (!fMerkleVerified)
    {
        if (CBlock::CheckMerkleBranch(GetHash(), vMerkleBranch, nIndex) != pindex->hashMerkleRoot)
            return 0;
        fMerkleVerified = true;
    }

    pindexRet = pindex;
    return pwallet->GetActiveChainHeight() - pindex->nHeight + 1;
}

int WalletTx::GetDepthInMainChain(const CBlockIndex* &pindexRet) const
{
    int nResult = GetDepthInMainChainInternal(pindexRet);
    if (nResult == 0 && !pwallet->MempoolExists(GetHash()))
        return -1; // Not in chain, not in mempool

    return nResult;
}

int WalletTx::GetBlocksToMaturity() const
{
    if (!IsCoinBase())
        return 0;
    return std::max(0, (COINBASE_MATURITY+1) - GetDepthInMainChain());
}

bool WalletTx::SetMerkleBranch(const CBlock& block)
{
    CBlock blockTmp;

    // Update the tx's hashBlock
    hashBlock = block.GetHash();

    // Locate the transaction
    for (nIndex = 0; nIndex < (int)block.vtx.size(); nIndex++)
        if (block.vtx[nIndex] == *(CTransaction*)this)
            break;
    if (nIndex == (int)block.vtx.size())
    {
        vMerkleBranch.clear();
        nIndex = -1;
        LogPrintf("ERROR: SetMerkleBranch(): couldn't find tx in block\n");
        return false;
    }

    // Fill in merkle branch
    vMerkleBranch = block.GetMerkleBranch(nIndex);
    return true;
}

void WalletTx::SetCache(const enum CREDIT_DEBIT_TYPE &balanceType, const isminefilter& filter, const CAmount &amount) const
{
    std::pair<uint8_t, uint8_t> key(balanceType,filter);
    std::pair<bool, CAmount> value(true, amount);
    cacheMap[key] = value;
}

bool WalletTx::GetCache(const enum CREDIT_DEBIT_TYPE &balanceType, const isminefilter& filter, CAmount &amountOut) const
{
    std::pair<uint8_t, uint8_t> key(balanceType,filter);
    std::pair<bool, CAmount> value = cacheMap[key];
    amountOut = value.second;
    return value.first;
}

}; //end namespace