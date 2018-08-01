// Copyright (c) 2018 PM-Tech
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef WALLETINTERFACE_H
#define WALLETINTERFACE_H

#include <module-interface.h>
#include <string>

class WalletInterface : public ModuleInterface {
public:
    void ProcessMessage(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv, CConnman* connman) override;
    bool CheckWalletCollateral(COutPoint& outpointRet, CTxDestination &destRet, CPubKey& pubKeyRet, CKey& keyRet, const std::string& strTxHash = "", const std::string& strOutputIndex = "") override;
    bool IsMixingMasternode(const CNode* pnode) override;
    void UpdatedBlockTip(const CBlockIndex *pindexNew) override;
};

#endif // WALLETINTERFACE_H
