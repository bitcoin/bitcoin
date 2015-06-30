// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COREWALLETWALLET_COREINTERFACE_H
#define BITCOIN_COREWALLETWALLET_COREINTERFACE_H

#include "chain.h"
#include "primitives/block.h"
#include "primitives/transaction.h"
#include "consensus/validation.h"

namespace CoreWallet
{
//bridge/layer for interacting with the full node
//this will allow later process seperation; APIing of the command below
class CoreInterface {
public:
    //!returns a block index if the block was found
    const CBlockIndex* GetBlockIndex(uint256 blockhash, bool inActiveChain = false) const;
    bool CheckFinalTx(const CTransaction &tx) const;
    int GetActiveChainHeight() const;
    bool MempoolExists(uint256 hash) const;
    bool AcceptToMemoryPool(CValidationState &state, const CTransaction &tx, bool fLimitFree, bool fRejectAbsurdFee=false);
    void RelayTransaction(const CTransaction &tx) const;

    CFeeRate GetMinRelayTxFee() const;
    unsigned int GetMaxStandardTxSize() const;
    CAmount EstimateFee(int confirmationTarget, size_t txSize) const;
    double EstimatePriority(int confirmationTarget) const;
    bool AllowFree(double dPriority) const;
};

} // end namespace
#endif // BITCOIN_COREWALLETWALLET_COREINTERFACE_H
