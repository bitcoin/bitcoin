// Copyright (c) 2015-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MODULEINTERFACE_H
#define MODULEINTERFACE_H

#include <script/standard.h>
#include <validationinterface.h>

class CNode;
class CDataStream;
class CPubKey;
class CKey;

class CDSNotificationInterface : public CValidationInterface {
public:
    CDSNotificationInterface(CConnman* connmanIn): connman(connmanIn) {}
    virtual ~CDSNotificationInterface() = default;

    // a small helper to initialize current block height in sub-modules on startup
    void InitializeCurrentBlockTip();

protected:
    // CValidationInterface
    void UpdatedBlockTip(const CBlockIndex *pindexNew, const CBlockIndex *pindexFork, bool fInitialDownload) override;
    void NewPoWValidBlock(const CBlockIndex *pindex, const std::shared_ptr<const CBlock> &block) override;
    void TransactionAddedToMempool(const CTransactionRef& tx) override;

private:
    CConnman* connman;
};

class ModuleInterface {
public:
    /** pass network messages */
    virtual void ProcessMessage(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv, CConnman* connman) = 0;

    /** Check wallet for MN collateral */
    virtual bool CheckWalletCollateral(COutPoint& outpointRet, CTxDestination &destRet, CPubKey& pubKeyRet, CKey& keyRet, const std::string& strTxHash = "", const std::string& strOutputIndex = "") = 0;

    /** protect mixing Masternodes */
    virtual bool IsMixingMasternode(const CNode* pnode) = 0;

    /** notify about chain tip change */
    virtual void UpdatedBlockTip(const CBlockIndex *pindexNew) = 0;

    virtual ~ModuleInterface() {}
};

class DummyModuleInterface : public ModuleInterface {
public:
    void ProcessMessage(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv, CConnman* connman) override {}
    bool CheckWalletCollateral(COutPoint& outpointRet, CTxDestination &destRet, CPubKey& pubKeyRet, CKey& keyRet, const std::string& strTxHash = "", const std::string& strOutputIndex = "") override {return false;}
    bool IsMixingMasternode(const CNode* pnode) override {return false;}
    void UpdatedBlockTip(const CBlockIndex *pindexNew) override {}
};

#endif // MODULEINTERFACE_H
