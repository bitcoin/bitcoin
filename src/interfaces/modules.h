// Copyright (c) 2015-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_MODULEINTERFACE_H
#define BITCOIN_MODULEINTERFACE_H

#include <validationinterface.h>

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

#endif // BITCOIN_MODULEINTERFACE_H
