// Copyright (c) 2014-2016 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "activemasternode.h"
#include "masternode.h"
#include "masternode-sync.h"
#include "masternodeman.h"
#include "protocol.h"

extern CWallet* pwalletMain;

// Keep track of the active Masternode
CActiveMasternode activeMasternode;

void CActiveMasternode::ManageState()
{
    LogPrint("masternode", "CActiveMasternode::ManageState -- Start\n");
    if(!fMasterNode) {
        LogPrint("masternode", "CActiveMasternode::ManageState -- Not a masternode, returning\n");
        return;
    }

    if(Params().NetworkIDString() != CBaseChainParams::REGTEST && !masternodeSync.IsBlockchainSynced()) {
        nState = ACTIVE_MASTERNODE_SYNC_IN_PROCESS;
        LogPrintf("CActiveMasternode::ManageState -- %s\n", GetStatus());
        return;
    }

    if(nState == ACTIVE_MASTERNODE_SYNC_IN_PROCESS) {
        nState = ACTIVE_MASTERNODE_INITIAL;
    }

    LogPrint("masternode", "CActiveMasternode::ManageState -- status = %s, type = %s, pinger enabled = %d\n", GetStatus(), GetType(), fPingerEnabled);

    if(eType == MASTERNODE_UNKNOWN) {
        ManageStateInitial();
    }

    if(eType == MASTERNODE_REMOTE) {
        ManageStateRemote();
    }
    else {
        ManageStateLocal();
    }

    if(fPingerEnabled) {
        std::string strError;
        if(!SendMasternodePing(strError)) {
            LogPrintf("CActiveMasternode::ManageState -- Error on SendMasternodePing(): %s\n", strError);
        }
    }
}

std::string CActiveMasternode::GetStatus()
{
    switch (nState) {
        case ACTIVE_MASTERNODE_INITIAL: return "Node just started, not yet activated";
        case ACTIVE_MASTERNODE_SYNC_IN_PROCESS: return "Sync in progress. Must wait until sync is complete to start Masternode";
        case ACTIVE_MASTERNODE_INPUT_TOO_NEW: return strprintf("Masternode input must have at least %d confirmations", Params().GetConsensus().nMasternodeMinimumConfirmations);
        case ACTIVE_MASTERNODE_NOT_CAPABLE: return "Not capable masternode: " + strNotCapableReason;
        case ACTIVE_MASTERNODE_STARTED: return "Masternode successfully started";
        default: return "unknown";
    }
}

std::string CActiveMasternode::GetType()
{
    std::string strType;
    switch(eType) {
    case MASTERNODE_UNKNOWN:
        strType = "UNKNOWN";
        break;
    case MASTERNODE_REMOTE:
        strType = "REMOTE";
        break;
    case MASTERNODE_LOCAL:
        strType = "LOCAL";
        break;
    default:
        strType = "UNKNOWN";
        break;
    }
    return strType;
}

bool CActiveMasternode::SendMasternodePing(std::string& strErrorRet)
{
    if(vin == CTxIn()) {
        return false;
    }

    CMasternodePing mnp(vin);
    if(!mnp.Sign(keyMasternode, pubKeyMasternode)) {
        strErrorRet = "Couldn't sign Masternode Ping";
        return false;
    }

    if(!mnodeman.Has(vin)) {
        strErrorRet = "Masternode List doesn't include our Masternode, shutting down Masternode pinging service! " + vin.ToString();
        nState = ACTIVE_MASTERNODE_NOT_CAPABLE;
        strNotCapableReason = strErrorRet;
        return false;
    }

    // Update lastPing for our masternode in Masternode list
    if(mnodeman.IsMasternodePingedWithin(vin, MASTERNODE_MIN_MNP_SECONDS, mnp.sigTime)) {
        strErrorRet = "Too early to send Masternode Ping";
        return false;
    }

    mnodeman.SetMasternodeLastPing(vin, mnp);

    LogPrintf("CActiveMasternode::SendMasternodePing -- Relaying ping, collateral=%s\n", vin.ToString());
    mnp.Relay();

    return true;
}

// when starting a Masternode, this can enable to run as a hot wallet with no funds
bool CActiveMasternode::EnableRemoteMasterNode(CTxIn& vinNew, CService& serviceNew)
{
    if(!fMasterNode) return false;

    nState = ACTIVE_MASTERNODE_STARTED;

    //The values below are needed for signing mnping messages going forward
    vin = vinNew;
    service = serviceNew;

    LogPrintf("CActiveMasternode::EnableHotColdMasterNode -- Enabled! You may shut down the cold daemon.\n");

    return true;
}

void CActiveMasternode::ManageStateInitial()
{
    LogPrint("masternode", "CActiveMasternode::ManageStateInitial -- Start status = %s, type = %s, pinger enabled = %d\n", GetStatus(), GetType(), fPingerEnabled);

    // Check that our local network configuration is correct
    if(!GetLocal(service)) {
        strNotCapableReason = "Can't detect external address. Please consider using the externalip configuration option if problem persists.";
        LogPrintf("CActiveMasternode::ManageStateInitial -- not capable: %s\n", strNotCapableReason);
        return;
    }

    int mainnetDefaultPort = Params(CBaseChainParams::MAIN).GetDefaultPort();
    if(Params().NetworkIDString() == CBaseChainParams::MAIN) {
        if(service.GetPort() != mainnetDefaultPort) {
            strNotCapableReason = strprintf("Invalid port: %u - only %d is supported on mainnet.", service.GetPort(), mainnetDefaultPort);
            LogPrintf("CActiveMasternode::ManageStateInitial -- not capable: %s\n", strNotCapableReason);
            return;
        }
    } else if(service.GetPort() == mainnetDefaultPort) {
        strNotCapableReason = strprintf("Invalid port: %u - %d is only supported on mainnet.", service.GetPort(), mainnetDefaultPort);
        LogPrintf("CActiveMasternode::ManageStateInitial -- not capable: %s\n", strNotCapableReason);
        return;
    }

    LogPrintf("CActiveMasternode::ManageState -- Checking inbound connection to '%s'\n", service.ToString());

    if(!ConnectNode((CAddress)service, NULL, true)) {
        strNotCapableReason = "Could not connect to " + service.ToString();
        LogPrintf("CActiveMasternode::ManageStateInitial -- not capable: %s\n", strNotCapableReason);
        return;
    }

    // Default to REMOTE
    eType = MASTERNODE_REMOTE;

    // Check if wallet funds are available
    if(!pwalletMain) {
        strNotCapableReason = "Wallet not available.";
        LogPrintf("CActiveMasternode::ManageStateInitial -- not capable: %s\n", strNotCapableReason);
        return;
    }

    if(pwalletMain->IsLocked()) {
        strNotCapableReason = "Wallet is locked.";
        LogPrintf("CActiveMasternode::ManageStateInitial -- not capable: %s\n", strNotCapableReason);
        return;
    }

    if(pwalletMain->GetBalance() < 1000*COIN) {
        strNotCapableReason = "Wallet balance is < 1000 DASH";
        LogPrintf("CActiveMasternode::ManageStateInitial -- not capable: %s\n", strNotCapableReason);
        return;
    }

    // Choose coins to use
    CPubKey pubKeyCollateral;
    CKey keyCollateral;

    // If collateral is found switch to LOCAL mode
    if(pwalletMain->GetMasternodeVinAndKeys(vin, pubKeyCollateral, keyCollateral)) {
        eType = MASTERNODE_LOCAL;
    }

    LogPrint("masternode", "CActiveMasternode::ManageStateInitial -- End status = %s, type = %s, pinger enabled = %d\n", GetStatus(), GetType(), fPingerEnabled);
}

void CActiveMasternode::ManageStateRemote()
{
    LogPrint("masternode", "CActiveMasternode::ManageStateRemote -- status = %s, type = %s, pinger enabled = %d\n", GetStatus(), GetType(), fPingerEnabled);
    mnodeman.CheckMasternode(pubKeyMasternode);
    masternode_info_t infoMn = mnodeman.GetMasternodeInfo(pubKeyMasternode);
    if(infoMn.fInfoValid) {
        if(infoMn.nProtocolVersion != PROTOCOL_VERSION) {
            nState = ACTIVE_MASTERNODE_NOT_CAPABLE;
            strNotCapableReason = "Invalid protocol version";
            return;
        }
        vin = infoMn.vin;
        service = infoMn.addr;
        fPingerEnabled = true;
        if(((infoMn.nActiveState == CMasternode::MASTERNODE_ENABLED) ||
            (infoMn.nActiveState == CMasternode::MASTERNODE_PRE_ENABLED) ||
            (infoMn.nActiveState == CMasternode::MASTERNODE_WATCHDOG_EXPIRED))) {
            nState = ACTIVE_MASTERNODE_STARTED;
        }
        else {
            nState = ACTIVE_MASTERNODE_NOT_CAPABLE;
            strNotCapableReason = "Masternode in EXPIRED state";
        }
    }
    else {
        fPingerEnabled = false;
        strNotCapableReason = "Masternode not in masternode list";
        nState = ACTIVE_MASTERNODE_NOT_CAPABLE;
    }
}

void CActiveMasternode::ManageStateLocal()
{
    LogPrint("masternode", "CActiveMasternode::ManageStateLocal -- status = %s, type = %s, pinger enabled = %d\n", GetStatus(), GetType(), fPingerEnabled);
    if(nState == ACTIVE_MASTERNODE_STARTED) {
        return;
    }

    // Choose coins to use
    CPubKey pubKeyCollateral;
    CKey keyCollateral;

    if(pwalletMain->GetMasternodeVinAndKeys(vin, pubKeyCollateral, keyCollateral)) {
        int nInputAge = GetInputAge(vin);
        if(nInputAge < Params().GetConsensus().nMasternodeMinimumConfirmations){
            nState = ACTIVE_MASTERNODE_INPUT_TOO_NEW;
            strNotCapableReason = strprintf("%s - %d confirmations", GetStatus(), nInputAge);
            LogPrintf("CActiveMasternode::ManageStateLocal -- %s\n", strNotCapableReason);
            return;
        }

        {
            LOCK(pwalletMain->cs_wallet);
            pwalletMain->LockCoin(vin.prevout);
        }

        CMasternodeBroadcast mnb;
        std::string strError;
        if(!CMasternodeBroadcast::Create(vin, service, keyCollateral, pubKeyCollateral, keyMasternode, pubKeyMasternode, strError, mnb)) {
            strNotCapableReason = "Error on CMasternodeBroadcast::Create -- " + strError;
            LogPrintf("CActiveMasternode::ManageStateLocal -- %s\n", strNotCapableReason);
            return;
        }

        //update to masternode list
        LogPrintf("CActiveMasternode::ManageStateLocal -- Update Masternode List\n");
        mnodeman.UpdateMasternodeList(mnb);

        //send to all peers
        LogPrintf("CActiveMasternode::ManageStateLocal -- Relay broadcast, vin=%s\n", vin.ToString());
        mnb.Relay();
        fPingerEnabled = true;
        nState = ACTIVE_MASTERNODE_STARTED;
    }
}
