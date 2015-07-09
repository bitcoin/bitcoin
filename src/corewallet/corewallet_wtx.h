// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COREWALLT_WTX_H
#define BITCOIN_COREWALLT_WTX_H

#include "corewallet/corewallet_basics.h"
#include "corewallet/corewallet_ismine.h"
#include "corewallet/corewallet_wallet.h"

#include "primitives/block.h"
#include "primitives/transaction.h"

#include "chain.h"
#include "utilstrencodings.h"

#include <list>

class Wallet;

namespace CoreWallet {

typedef std::map<std::string, std::string> mapValue_t;

struct COutputEntry
{
    CTxDestination destination;
    CAmount amount;
    int vout;
};

/**
 * A transaction with a bunch of additional info that only the owner cares about.
 * It includes any unrecorded transactions needed to link it back to the block chain.
 */
class WalletTx : public CTransaction
{
private:
    const Wallet* pwallet;
    int GetDepthInMainChainInternal(const CBlockIndex* &pindexRet) const;
    
public:
    static const int CURRENT_VERSION=1;
    uint8_t nVersion;

    mapValue_t mapValue;
    std::vector<std::pair<std::string, std::string> > vOrderForm;
    std::vector<uint32_t> vChangeOutputs; //! stores the positions of the change vouts
    unsigned int fTimeReceivedIsTxTime;
    unsigned int nTimeReceived; //! time received by this node
    unsigned int nTimeSmart;
    char fFromMe;
    int64_t nOrderPos; //! position in ordered transaction list

    uint256 hashBlock;
    std::vector<uint256> vMerkleBranch;
    int nIndex;

    // memory only
    mutable std::map<std::pair<uint8_t, uint8_t>, std::pair<bool, CAmount> > cacheMap; //! cache map for storing credit/debit caches
    mutable bool fMerkleVerified;
    mutable CAmount nChangeCached;

    WalletTx()
    {
        Init(NULL);
    }

    WalletTx(const Wallet* pwalletIn)
    {
        Init(pwalletIn);
    }

    WalletTx(const Wallet* pwalletIn, const CTransaction& txIn) : CTransaction(txIn)
    {
        Init(pwalletIn);
    }

    void Init(const Wallet* pwalletIn)
    {
        nVersion = WalletTx::CURRENT_VERSION;
        hashBlock = uint256();
        nIndex = -1; //!position in block
        fMerkleVerified = false;
        pwallet = pwalletIn;
        mapValue.clear(); //!flexible key/value set for wtx metadata
        vOrderForm.clear();
        vChangeOutputs.clear(); //!todo: store which outputs of this wtx are change outputs
        cacheMap.clear();
        fTimeReceivedIsTxTime = false;
        nTimeReceived = 0;
        nTimeSmart = 0;
        fFromMe = false;
        nChangeCached = 0;
        nOrderPos = -1;
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(*(CTransaction*)this);
        READWRITE(hashBlock);
        READWRITE(vMerkleBranch);
        READWRITE(nIndex);
        std::vector<WalletTx> vUnused; //! Used to be vtxPrev
        READWRITE(vUnused);
        READWRITE(mapValue);
        READWRITE(vOrderForm);
        READWRITE(vChangeOutputs);
        READWRITE(fTimeReceivedIsTxTime);
        READWRITE(nTimeReceived);
        READWRITE(nTimeSmart);
        READWRITE(fFromMe);
        READWRITE(nOrderPos);
    }

    //! make sure balances are recalculated
    void MarkDirty()
    {
        cacheMap.clear();
    }

    void BindWallet(Wallet *pwalletIn)
    {
        pwallet = pwalletIn;
        MarkDirty();
    }

    //! filter decides which addresses will count towards the debit
    CAmount GetChange() const;

    void GetAmounts(std::list<COutputEntry>& listReceived,
                    std::list<COutputEntry>& listSent, CAmount& nFee, std::string& strSentAccount, const isminefilter& filter) const;

    void GetAccountAmounts(const std::string& strAccount, CAmount& nReceived,
                           CAmount& nSent, CAmount& nFee, const isminefilter& filter) const;
    

    int64_t GetTxTime() const;
    int GetRequestCount() const;
    
    bool RelayWalletTransaction();

    /**
     * Return depth of transaction in blockchain:
     * -1  : not in blockchain, and not in memory pool (conflicted transaction)
     *  0  : in memory pool, waiting to be included in a block
     * >=1 : this many blocks deep in the main chain
     */
    int GetDepthInMainChain(const CBlockIndex* &pindexRet) const;
    int GetDepthInMainChain() const { const CBlockIndex *pindexRet; return GetDepthInMainChain(pindexRet); }
    bool IsInMainChain() const { const CBlockIndex *pindexRet; return GetDepthInMainChainInternal(pindexRet) > 0; }
    int GetBlocksToMaturity() const;

    bool SetMerkleBranch(const CBlock& block);

    //store a amount in in-mem cache
    void SetCache(const enum CREDIT_DEBIT_TYPE &balanceType, const isminefilter& filter, const CAmount &amount) const;

    //get a value from in-mem cache
    bool GetCache(const enum CREDIT_DEBIT_TYPE &balanceType, const isminefilter& filter, CAmount &amountOut) const;
};




class COutput
{
public:
    const WalletTx *tx;
    int i;
    int nDepth;
    bool fSpendable;
    
    COutput(const WalletTx *txIn, int iIn, int nDepthIn, bool fSpendableIn)
    {
        tx = txIn; i = iIn; nDepth = nDepthIn; fSpendable = fSpendableIn;
    }
    
    std::string ToString() const;
};

} //end namespace

#endif // BITCOIN_COREWALLT_WTX_H
