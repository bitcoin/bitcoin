// Copyright (c) 2014-2019 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "activemasternode.h"
#include "evo/deterministicmns.h"
#include "init.h"
#include "masternode-sync.h"
#include "netbase.h"
#include "protocol.h"
#include "validation.h"
#include "warnings.h"

// Keep track of the active Masternode
CActiveMasternodeInfo activeMasternodeInfo;
CActiveMasternodeManager* activeMasternodeManager;

std::string CActiveMasternodeManager::GetStateString() const
{
    switch (state) {
    case MASTERNODE_WAITING_FOR_PROTX:
        return "WAITING_FOR_PROTX";
    case MASTERNODE_POSE_BANNED:
        return "POSE_BANNED";
    case MASTERNODE_REMOVED:
        return "REMOVED";
    case MASTERNODE_OPERATOR_KEY_CHANGED:
        return "OPERATOR_KEY_CHANGED";
    case MASTERNODE_READY:
        return "READY";
    case MASTERNODE_ERROR:
        return "ERROR";
    default:
        return "UNKNOWN";
    }
}

std::string CActiveMasternodeManager::GetStatus() const
{
    switch (state) {
    case MASTERNODE_WAITING_FOR_PROTX:
        return "Waiting for ProTx to appear on-chain";
    case MASTERNODE_POSE_BANNED:
        return "Masternode was PoSe banned";
    case MASTERNODE_REMOVED:
        return "Masternode removed from list";
    case MASTERNODE_OPERATOR_KEY_CHANGED:
        return "Operator key changed or revoked";
    case MASTERNODE_READY:
        return "Ready";
    case MASTERNODE_ERROR:
        return "Error. " + strError;
    default:
        return "Unknown";
    }
}

void CActiveMasternodeManager::Init()
{
    LOCK(cs_main);

    if (!fMasternodeMode) return;

    if (!deterministicMNManager->IsDIP3Enforced()) return;

    // Check that our local network configuration is correct
    if (!fListen) {
        // listen option is probably overwritten by smth else, no good
        state = MASTERNODE_ERROR;
        strError = "Masternode must accept connections from outside. Make sure listen configuration option is not overwritten by some another parameter.";
        LogPrintf("CActiveDeterministicMasternodeManager::Init -- ERROR: %s\n", strError);
        return;
    }

    if (!GetLocalAddress(activeMasternodeInfo.service)) {
        state = MASTERNODE_ERROR;
        return;
    }

    CDeterministicMNList mnList = deterministicMNManager->GetListAtChainTip();

    CDeterministicMNCPtr dmn = mnList.GetMNByOperatorKey(*activeMasternodeInfo.blsPubKeyOperator);
    if (!dmn) {
        // MN not appeared on the chain yet
        return;
    }

    if (!mnList.IsMNValid(dmn->proTxHash)) {
        if (mnList.IsMNPoSeBanned(dmn->proTxHash)) {
            state = MASTERNODE_POSE_BANNED;
        } else {
            state = MASTERNODE_REMOVED;
        }
        return;
    }

    mnListEntry = dmn;

    LogPrintf("CActiveMasternodeManager::Init -- proTxHash=%s, proTx=%s\n", mnListEntry->proTxHash.ToString(), mnListEntry->ToString());

    if (activeMasternodeInfo.service != mnListEntry->pdmnState->addr) {
        state = MASTERNODE_ERROR;
        strError = "Local address does not match the address from ProTx";
        LogPrintf("CActiveMasternodeManager::Init -- ERROR: %s", strError);
        return;
    }

    if (Params().NetworkIDString() != CBaseChainParams::REGTEST) {
        // Check socket connectivity
        LogPrintf("CActiveDeterministicMasternodeManager::Init -- Checking inbound connection to '%s'\n", activeMasternodeInfo.service.ToString());
        SOCKET hSocket;
        bool fConnected = ConnectSocket(activeMasternodeInfo.service, hSocket, nConnectTimeout) && IsSelectableSocket(hSocket);
        CloseSocket(hSocket);

        if (!fConnected) {
            state = MASTERNODE_ERROR;
            strError = "Could not connect to " + activeMasternodeInfo.service.ToString();
            LogPrintf("CActiveDeterministicMasternodeManager::Init -- ERROR: %s\n", strError);
            return;
        }
    }

    activeMasternodeInfo.proTxHash = mnListEntry->proTxHash;
    activeMasternodeInfo.outpoint = mnListEntry->collateralOutpoint;
    state = MASTERNODE_READY;
}

void CActiveMasternodeManager::UpdatedBlockTip(const CBlockIndex* pindexNew, const CBlockIndex* pindexFork, bool fInitialDownload)
{
    LOCK(cs_main);

    if (!fMasternodeMode) return;

    if (!deterministicMNManager->IsDIP3Enforced(pindexNew->nHeight)) return;

    if (state == MASTERNODE_READY) {
        auto mnList = deterministicMNManager->GetListForBlock(pindexNew);
        if (!mnList.IsMNValid(mnListEntry->proTxHash)) {
            // MN disappeared from MN list
            state = MASTERNODE_REMOVED;
            activeMasternodeInfo.proTxHash = uint256();
            activeMasternodeInfo.outpoint.SetNull();
            // MN might have reappeared in same block with a new ProTx
            Init();
        } else if (mnList.GetMN(mnListEntry->proTxHash)->pdmnState->pubKeyOperator != mnListEntry->pdmnState->pubKeyOperator) {
            // MN operator key changed or revoked
            state = MASTERNODE_OPERATOR_KEY_CHANGED;
            activeMasternodeInfo.proTxHash = uint256();
            activeMasternodeInfo.outpoint.SetNull();
            // MN might have reappeared in same block with a new ProTx
            Init();
        }
    } else {
        // MN might have (re)appeared with a new ProTx or we've found some peers and figured out our local address
        Init();
    }
}

bool CActiveMasternodeManager::GetLocalAddress(CService& addrRet)
{
    // First try to find whatever local address is specified by externalip option
    bool fFoundLocal = GetLocal(addrRet) && IsValidNetAddr(addrRet);
    if (!fFoundLocal && Params().NetworkIDString() == CBaseChainParams::REGTEST) {
        if (Lookup("127.0.0.1", addrRet, GetListenPort(), false)) {
            fFoundLocal = true;
        }
    }
    if (!fFoundLocal) {
        bool empty = true;
        // If we have some peers, let's try to find our local address from one of them
        g_connman->ForEachNodeContinueIf(CConnman::AllNodes, [&fFoundLocal, &empty](CNode* pnode) {
            empty = false;
            if (pnode->addr.IsIPv4())
                fFoundLocal = GetLocal(activeMasternodeInfo.service, &pnode->addr) && IsValidNetAddr(activeMasternodeInfo.service);
            return !fFoundLocal;
        });
        // nothing and no live connections, can't do anything for now
        if (empty) {
            strError = "Can't detect valid external address. Please consider using the externalip configuration option if problem persists. Make sure to use IPv4 address only.";
            LogPrintf("CActiveMasternodeManager::GetLocalAddress -- ERROR: %s\n", strError);
            return false;
        }
    }
    return true;
}

bool CActiveMasternodeManager::IsValidNetAddr(CService addrIn)
{
    // TODO: regtest is fine with any addresses for now,
    // should probably be a bit smarter if one day we start to implement tests for this
    return Params().NetworkIDString() == CBaseChainParams::REGTEST ||
           (addrIn.IsIPv4() && IsReachable(addrIn) && addrIn.IsRoutable());
}
