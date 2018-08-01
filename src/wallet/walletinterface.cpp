// Copyright (c) 2018 The PM-Tech
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/walletinterface.h>
#include <wallet/wallet.h>
#include <wallet/privatesend-client.h>

void WalletInterface::ProcessMessage(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv, CConnman* connman)
{
    privateSendClient.ProcessMessage(pfrom, strCommand, vRecv, connman);
}

bool WalletInterface::CheckWalletCollateral(COutPoint& outpointRet, CTxDestination &destRet, CPubKey& pubKeyRet, CKey& keyRet, const std::string& strTxHash, const std::string& strOutputIndex)
{
    bool foundmnout = false;
    for (CWalletRef pwallet : vpwallets) {
        if (pwallet->GetMasternodeOutpointAndKeys(outpointRet, destRet, pubKeyRet, keyRet, strTxHash, strOutputIndex))
            foundmnout = true;
    }
    return foundmnout;
}

bool WalletInterface::IsMixingMasternode(const CNode* pnode)
{
    return privateSendClient.IsMixingMasternode(pnode);
}

void WalletInterface::UpdatedBlockTip(const CBlockIndex *pindexNew)
{
    privateSendClient.UpdatedBlockTip(pindexNew);
}
