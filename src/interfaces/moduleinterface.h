// Copyright (c) 2015-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_MODULEINTERFACE_H
#define BITCOIN_MODULEINTERFACE_H

#include <script/standard.h>           // For CTxDestination
#include <validationinterface.h>

class CKey;

class ModuleInterface : public CValidationInterface {
public:
    ModuleInterface(CConnman* _connman): connman(_connman) {}

    virtual ~ModuleInterface() {}

    // a small helper to initialize current block height in sub-modules on startup
    void InitializeCurrentBlockTip();

protected:
    // CValidationInterface
    void ProcessModuleMessage(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv, CConnman* connman) override;
    void UpdatedBlockTip(const CBlockIndex *pindexNew, const CBlockIndex *pindexFork, bool fInitialDownload) override;
    void TransactionAddedToMempool(const CTransactionRef& tx) override;
    void BlockDisconnected(const std::shared_ptr<const CBlock>& pblock) override;

private:
    CConnman* connman;
};

class WalletInterface {
public:
    /** Check MN Collateral */
    virtual bool CheckMNCollateral(COutPoint& outpointRet, CTxDestination &destRet, CPubKey& pubKeyRet, CKey& keyRet, const std::string& strTxHash, const std::string& strOutputIndex) const = 0;
    /** Return MN mixing state */
    virtual bool IsMixingMasternode(const CNode* pnode) const = 0;

    virtual ~WalletInterface() {}
};

extern const WalletInterface& g_wallet_interface;

#endif // BITCOIN_MODULEINTERFACE_H
