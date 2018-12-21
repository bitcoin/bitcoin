// Copyright (c) 2014-2018 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "activemasternode.h"
#include "evo/deterministicmns.h"
#include "init.h"
#include "masternode-sync.h"
#include "masternode.h"
#include "masternodeman.h"
#include "netbase.h"
#include "protocol.h"
#include "warnings.h"

// Keep track of the active Masternode
CActiveMasternodeInfo activeMasternodeInfo;
CActiveLegacyMasternodeManager legacyActiveMasternodeManager;
CActiveDeterministicMasternodeManager* activeMasternodeManager;

std::string CActiveDeterministicMasternodeManager::GetStateString() const
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

std::string CActiveDeterministicMasternodeManager::GetStatus() const
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

void CActiveDeterministicMasternodeManager::Init()
{
    LOCK(cs_main);

    if (!fMasternodeMode) return;

    if (!deterministicMNManager->IsDeterministicMNsSporkActive()) return;

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

    LogPrintf("CActiveDeterministicMasternodeManager::Init -- proTxHash=%s, proTx=%s\n", mnListEntry->proTxHash.ToString(), mnListEntry->ToString());

    if (activeMasternodeInfo.service != mnListEntry->pdmnState->addr) {
        state = MASTERNODE_ERROR;
        strError = "Local address does not match the address from ProTx";
        LogPrintf("CActiveDeterministicMasternodeManager::Init -- ERROR: %s", strError);
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

void CActiveDeterministicMasternodeManager::UpdatedBlockTip(const CBlockIndex* pindexNew, const CBlockIndex* pindexFork, bool fInitialDownload)
{
    LOCK(cs_main);

    if (!fMasternodeMode) return;

    if (!deterministicMNManager->IsDeterministicMNsSporkActive(pindexNew->nHeight)) return;

    if (state == MASTERNODE_READY) {
        auto mnList = deterministicMNManager->GetListForBlock(pindexNew->GetBlockHash());
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

bool CActiveDeterministicMasternodeManager::GetLocalAddress(CService& addrRet)
{
    // First try to find whatever local address is specified by externalip option
    bool fFoundLocal = GetLocal(addrRet) && CMasternode::IsValidNetAddr(addrRet);
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
                fFoundLocal = GetLocal(activeMasternodeInfo.service, &pnode->addr) && CMasternode::IsValidNetAddr(activeMasternodeInfo.service);
            return !fFoundLocal;
        });
        // nothing and no live connections, can't do anything for now
        if (empty) {
            strError = "Can't detect valid external address. Please consider using the externalip configuration option if problem persists. Make sure to use IPv4 address only.";
            LogPrintf("CActiveDeterministicMasternodeManager::GetLocalAddress -- ERROR: %s\n", strError);
            return false;
        }
    }
    return true;
}

/********* LEGACY *********/

void CActiveLegacyMasternodeManager::ManageState(CConnman& connman)
{
    if (deterministicMNManager->IsDeterministicMNsSporkActive()) return;

    LogPrint("masternode", "CActiveLegacyMasternodeManager::ManageState -- Start\n");
    if (!fMasternodeMode) {
        LogPrint("masternode", "CActiveLegacyMasternodeManager::ManageState -- Not a masternode, returning\n");
        return;
    }
    if (Params().NetworkIDString() != CBaseChainParams::REGTEST && !masternodeSync.IsBlockchainSynced()) {
        nState = ACTIVE_MASTERNODE_SYNC_IN_PROCESS;
        LogPrintf("CActiveLegacyMasternodeManager::ManageState -- %s: %s\n", GetStateString(), GetStatus());
        return;
    }

    if (nState == ACTIVE_MASTERNODE_SYNC_IN_PROCESS) {
        nState = ACTIVE_MASTERNODE_INITIAL;
    }

    LogPrint("masternode", "CActiveLegacyMasternodeManager::ManageState -- status = %s, type = %s, pinger enabled = %d\n", GetStatus(), GetTypeString(), fPingerEnabled);

    if (eType == MASTERNODE_UNKNOWN) {
        ManageStateInitial(connman);
    }

    if (eType == MASTERNODE_REMOTE) {
        ManageStateRemote();
    }

    SendMasternodePing(connman);
}

std::string CActiveLegacyMasternodeManager::GetStateString() const
{
    switch (nState) {
    case ACTIVE_MASTERNODE_INITIAL:
        return "INITIAL";
    case ACTIVE_MASTERNODE_SYNC_IN_PROCESS:
        return "SYNC_IN_PROCESS";
    case ACTIVE_MASTERNODE_INPUT_TOO_NEW:
        return "INPUT_TOO_NEW";
    case ACTIVE_MASTERNODE_NOT_CAPABLE:
        return "NOT_CAPABLE";
    case ACTIVE_MASTERNODE_STARTED:
        return "STARTED";
    default:
        return "UNKNOWN";
    }
}

std::string CActiveLegacyMasternodeManager::GetStatus() const
{
    switch (nState) {
    case ACTIVE_MASTERNODE_INITIAL:
        return "Node just started, not yet activated";
    case ACTIVE_MASTERNODE_SYNC_IN_PROCESS:
        return "Sync in progress. Must wait until sync is complete to start Masternode";
    case ACTIVE_MASTERNODE_INPUT_TOO_NEW:
        return strprintf("Masternode input must have at least %d confirmations", Params().GetConsensus().nMasternodeMinimumConfirmations);
    case ACTIVE_MASTERNODE_NOT_CAPABLE:
        return "Not capable masternode: " + strNotCapableReason;
    case ACTIVE_MASTERNODE_STARTED:
        return "Masternode successfully started";
    default:
        return "Unknown";
    }
}

std::string CActiveLegacyMasternodeManager::GetTypeString() const
{
    std::string strType;
    switch (eType) {
    case MASTERNODE_REMOTE:
        strType = "REMOTE";
        break;
    default:
        strType = "UNKNOWN";
        break;
    }
    return strType;
}

bool CActiveLegacyMasternodeManager::SendMasternodePing(CConnman& connman)
{
    if (deterministicMNManager->IsDeterministicMNsSporkActive()) return false;

    if (!fPingerEnabled) {
        LogPrint("masternode", "CActiveLegacyMasternodeManager::SendMasternodePing -- %s: masternode ping service is disabled, skipping...\n", GetStateString());
        return false;
    }

    if (!mnodeman.Has(activeMasternodeInfo.outpoint)) {
        strNotCapableReason = "Masternode not in masternode list";
        nState = ACTIVE_MASTERNODE_NOT_CAPABLE;
        LogPrintf("CActiveLegacyMasternodeManager::SendMasternodePing -- %s: %s\n", GetStateString(), strNotCapableReason);
        return false;
    }

    CMasternodePing mnp(activeMasternodeInfo.outpoint);
    mnp.nSentinelVersion = nSentinelVersion;
    mnp.fSentinelIsCurrent =
        (abs(GetAdjustedTime() - nSentinelPingTime) < MASTERNODE_SENTINEL_PING_MAX_SECONDS);
    if (!mnp.Sign(activeMasternodeInfo.legacyKeyOperator, activeMasternodeInfo.legacyKeyIDOperator)) {
        LogPrintf("CActiveLegacyMasternodeManager::SendMasternodePing -- ERROR: Couldn't sign Masternode Ping\n");
        return false;
    }

    // Update lastPing for our masternode in Masternode list
    if (mnodeman.IsMasternodePingedWithin(activeMasternodeInfo.outpoint, MASTERNODE_MIN_MNP_SECONDS, mnp.sigTime)) {
        LogPrintf("CActiveLegacyMasternodeManager::SendMasternodePing -- Too early to send Masternode Ping\n");
        return false;
    }

    mnodeman.SetMasternodeLastPing(activeMasternodeInfo.outpoint, mnp);

    LogPrintf("CActiveLegacyMasternodeManager::SendMasternodePing -- Relaying ping, collateral=%s\n", activeMasternodeInfo.outpoint.ToStringShort());
    mnp.Relay(connman);

    return true;
}

bool CActiveLegacyMasternodeManager::UpdateSentinelPing(int version)
{
    nSentinelVersion = version;
    nSentinelPingTime = GetAdjustedTime();

    return true;
}

void CActiveLegacyMasternodeManager::DoMaintenance(CConnman& connman)
{
    if (ShutdownRequested()) return;

    ManageState(connman);
}

void CActiveLegacyMasternodeManager::ManageStateInitial(CConnman& connman)
{
    if (deterministicMNManager->IsDeterministicMNsSporkActive()) return;

    LogPrint("masternode", "CActiveLegacyMasternodeManager::ManageStateInitial -- status = %s, type = %s, pinger enabled = %d\n", GetStatus(), GetTypeString(), fPingerEnabled);

    // Check that our local network configuration is correct
    if (!fListen) {
        // listen option is probably overwritten by smth else, no good
        nState = ACTIVE_MASTERNODE_NOT_CAPABLE;
        strNotCapableReason = "Masternode must accept connections from outside. Make sure listen configuration option is not overwritten by some another parameter.";
        LogPrintf("CActiveLegacyMasternodeManager::ManageStateInitial -- %s: %s\n", GetStateString(), strNotCapableReason);
        return;
    }

    // First try to find whatever local address is specified by externalip option
    bool fFoundLocal = GetLocal(activeMasternodeInfo.service) && CMasternode::IsValidNetAddr(activeMasternodeInfo.service);
    if (!fFoundLocal) {
        bool empty = true;
        // If we have some peers, let's try to find our local address from one of them
        connman.ForEachNodeContinueIf(CConnman::AllNodes, [&fFoundLocal, &empty](CNode* pnode) {
            empty = false;
            if (pnode->addr.IsIPv4())
                fFoundLocal = GetLocal(activeMasternodeInfo.service, &pnode->addr) && CMasternode::IsValidNetAddr(activeMasternodeInfo.service);
            return !fFoundLocal;
        });
        // nothing and no live connections, can't do anything for now
        if (empty) {
            nState = ACTIVE_MASTERNODE_NOT_CAPABLE;
            strNotCapableReason = "Can't detect valid external address. Will retry when there are some connections available.";
            LogPrintf("CActiveLegacyMasternodeManager::ManageStateInitial -- %s: %s\n", GetStateString(), strNotCapableReason);
            return;
        }
    }

    if (!fFoundLocal && Params().NetworkIDString() == CBaseChainParams::REGTEST) {
        if (Lookup("127.0.0.1", activeMasternodeInfo.service, GetListenPort(), false)) {
            fFoundLocal = true;
        }
    }

    if (!fFoundLocal) {
        nState = ACTIVE_MASTERNODE_NOT_CAPABLE;
        strNotCapableReason = "Can't detect valid external address. Please consider using the externalip configuration option if problem persists. Make sure to use IPv4 address only.";
        LogPrintf("CActiveLegacyMasternodeManager::ManageStateInitial -- %s: %s\n", GetStateString(), strNotCapableReason);
        return;
    }

    int mainnetDefaultPort = Params(CBaseChainParams::MAIN).GetDefaultPort();
    if (Params().NetworkIDString() == CBaseChainParams::MAIN) {
        if (activeMasternodeInfo.service.GetPort() != mainnetDefaultPort) {
            nState = ACTIVE_MASTERNODE_NOT_CAPABLE;
            strNotCapableReason = strprintf("Invalid port: %u - only %d is supported on mainnet.", activeMasternodeInfo.service.GetPort(), mainnetDefaultPort);
            LogPrintf("CActiveLegacyMasternodeManager::ManageStateInitial -- %s: %s\n", GetStateString(), strNotCapableReason);
            return;
        }
    } else if (activeMasternodeInfo.service.GetPort() == mainnetDefaultPort) {
        nState = ACTIVE_MASTERNODE_NOT_CAPABLE;
        strNotCapableReason = strprintf("Invalid port: %u - %d is only supported on mainnet.", activeMasternodeInfo.service.GetPort(), mainnetDefaultPort);
        LogPrintf("CActiveLegacyMasternodeManager::ManageStateInitial -- %s: %s\n", GetStateString(), strNotCapableReason);
        return;
    }

    if (Params().NetworkIDString() != CBaseChainParams::REGTEST) {
        // Check socket connectivity
        LogPrintf("CActiveLegacyMasternodeManager::ManageStateInitial -- Checking inbound connection to '%s'\n", activeMasternodeInfo.service.ToString());
        SOCKET hSocket;
        bool fConnected = ConnectSocket(activeMasternodeInfo.service, hSocket, nConnectTimeout) && IsSelectableSocket(hSocket);
        CloseSocket(hSocket);

        if (!fConnected) {
            nState = ACTIVE_MASTERNODE_NOT_CAPABLE;
            strNotCapableReason = "Could not connect to " + activeMasternodeInfo.service.ToString();
            LogPrintf("CActiveLegacyMasternodeManager::ManageStateInitial -- %s: %s\n", GetStateString(), strNotCapableReason);
            return;
        }
    }
    // Default to REMOTE
    eType = MASTERNODE_REMOTE;

    LogPrint("masternode", "CActiveLegacyMasternodeManager::ManageStateInitial -- End status = %s, type = %s, pinger enabled = %d\n", GetStatus(), GetTypeString(), fPingerEnabled);
}

void CActiveLegacyMasternodeManager::ManageStateRemote()
{
    if (deterministicMNManager->IsDeterministicMNsSporkActive()) return;

    LogPrint("masternode", "CActiveLegacyMasternodeManager::ManageStateRemote -- Start status = %s, type = %s, pinger enabled = %d, keyIDOperator = %s\n",
        GetStatus(), GetTypeString(), fPingerEnabled, activeMasternodeInfo.legacyKeyIDOperator.ToString());

    mnodeman.CheckMasternode(activeMasternodeInfo.legacyKeyIDOperator, true);
    masternode_info_t infoMn;
    if (mnodeman.GetMasternodeInfo(activeMasternodeInfo.legacyKeyIDOperator, infoMn)) {
        if (infoMn.nProtocolVersion != PROTOCOL_VERSION) {
            nState = ACTIVE_MASTERNODE_NOT_CAPABLE;
            strNotCapableReason = "Invalid protocol version";
            LogPrintf("CActiveLegacyMasternodeManager::ManageStateRemote -- %s: %s\n", GetStateString(), strNotCapableReason);
            return;
        }
        if (activeMasternodeInfo.service != infoMn.addr) {
            nState = ACTIVE_MASTERNODE_NOT_CAPABLE;
            strNotCapableReason = "Broadcasted IP doesn't match our external address. Make sure you issued a new broadcast if IP of this masternode changed recently.";
            LogPrintf("CActiveLegacyMasternodeManager::ManageStateRemote -- %s: %s\n", GetStateString(), strNotCapableReason);
            return;
        }
        if (!CMasternode::IsValidStateForAutoStart(infoMn.nActiveState)) {
            nState = ACTIVE_MASTERNODE_NOT_CAPABLE;
            strNotCapableReason = strprintf("Masternode in %s state", CMasternode::StateToString(infoMn.nActiveState));
            LogPrintf("CActiveLegacyMasternodeManager::ManageStateRemote -- %s: %s\n", GetStateString(), strNotCapableReason);
            return;
        }
        auto dmn = deterministicMNManager->GetListAtChainTip().GetMNByCollateral(infoMn.outpoint);
        if (dmn) {
            if (dmn->pdmnState->addr != infoMn.addr) {
                nState = ACTIVE_MASTERNODE_NOT_CAPABLE;
                strNotCapableReason = strprintf("Masternode collateral is a ProTx and ProTx address does not match local address");
                LogPrintf("CActiveLegacyMasternodeManager::ManageStateRemote -- %s: %s\n", GetStateString(), strNotCapableReason);
                return;
            }
            LogPrintf("CActiveLegacyMasternodeManager::ManageStateRemote -- Collateral is a ProTx\n");
        }
        if (nState != ACTIVE_MASTERNODE_STARTED) {
            LogPrintf("CActiveLegacyMasternodeManager::ManageStateRemote -- STARTED!\n");
            activeMasternodeInfo.outpoint = infoMn.outpoint;
            activeMasternodeInfo.service = infoMn.addr;
            fPingerEnabled = true;
            nState = ACTIVE_MASTERNODE_STARTED;
        }
    } else {
        nState = ACTIVE_MASTERNODE_NOT_CAPABLE;
        strNotCapableReason = "Masternode not in masternode list";
        LogPrintf("CActiveLegacyMasternodeManager::ManageStateRemote -- %s: %s\n", GetStateString(), strNotCapableReason);
    }
}
