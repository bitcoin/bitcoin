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

// Bootup the Masternode, look for a 1000DASH input and register on the network
void CActiveMasternode::ManageState()
{
    std::string strErrorMessage;

    if(!fMasterNode) return;

    if (fDebug) LogPrintf("CActiveMasternode::ManageState -- Begin\n");

    //need correct blocks to send ping
    if(Params().NetworkIDString() != CBaseChainParams::REGTEST && !masternodeSync.IsBlockchainSynced()) {
        nState = ACTIVE_MASTERNODE_SYNC_IN_PROCESS;
        LogPrintf("CActiveMasternode::ManageState -- %s\n", GetStatus());
        return;
    }

    if(nState == ACTIVE_MASTERNODE_SYNC_IN_PROCESS) nState = ACTIVE_MASTERNODE_INITIAL;

    if(nState == ACTIVE_MASTERNODE_INITIAL) {
        CMasternode *pmn;
        pmn = mnodeman.Find(pubKeyMasternode);
        if(pmn != NULL) {
            pmn->Check();
            if((pmn->IsEnabled() || pmn->IsPreEnabled()) && pmn->protocolVersion == PROTOCOL_VERSION)
                    EnableHotColdMasterNode(pmn->vin, pmn->addr);
        }
    }

    if(nState != ACTIVE_MASTERNODE_STARTED) {

        // Set defaults
        nState = ACTIVE_MASTERNODE_NOT_CAPABLE;
        strNotCapableReason = "";

        if(pwalletMain->IsLocked()) {
            strNotCapableReason = "Wallet is locked.";
            LogPrintf("CActiveMasternode::ManageState -- not capable: %s\n", strNotCapableReason);
            return;
        }

        if(pwalletMain->GetBalance() == 0) {
            nState = ACTIVE_MASTERNODE_INITIAL;
            LogPrintf("CActiveMasternode::ManageState() -- %s\n", GetStatus());
            return;
        }

        if(strMasterNodeAddr.empty()) {
            if(!GetLocal(service)) {
                strNotCapableReason = "Can't detect external address. Please use the masternodeaddr configuration option.";
                LogPrintf("CActiveMasternode::ManageState -- not capable: %s\n", strNotCapableReason);
                return;
            }
        } else {
            service = CService(strMasterNodeAddr);
        }

        int mainnetDefaultPort = Params(CBaseChainParams::MAIN).GetDefaultPort();
        if(Params().NetworkIDString() == CBaseChainParams::MAIN) {
            if(service.GetPort() != mainnetDefaultPort) {
                strNotCapableReason = strprintf("Invalid port: %u - only %d is supported on mainnet.", service.GetPort(), mainnetDefaultPort);
                LogPrintf("CActiveMasternode::ManageState -- not capable: %s\n", strNotCapableReason);
                return;
            }
        } else if(service.GetPort() == mainnetDefaultPort) {
            strNotCapableReason = strprintf("Invalid port: %u - %d is only supported on mainnet.", service.GetPort(), mainnetDefaultPort);
            LogPrintf("CActiveMasternode::ManageState -- not capable: %s\n", strNotCapableReason);
            return;
        }

        LogPrintf("CActiveMasternode::ManageState -- Checking inbound connection to '%s'\n", service.ToString());

        if(!ConnectNode((CAddress)service, NULL, true)) {
            strNotCapableReason = "Could not connect to " + service.ToString();
            LogPrintf("CActiveMasternode::ManageState -- not capable: %s\n", strNotCapableReason);
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
                LogPrintf("CActiveMasternode::ManageState -- %s\n", strNotCapableReason);
                return;
            }

            LOCK(pwalletMain->cs_wallet);
            pwalletMain->LockCoin(vin.prevout);

            CMasternodeBroadcast mnb;
            if(!CMasternodeBroadcast::Create(vin, service, keyCollateral, pubKeyCollateral, keyMasternode, pubKeyMasternode, strErrorMessage, mnb)) {
                strNotCapableReason = "Error on CMasternodeBroadcast::Create -- " + strErrorMessage;
                LogPrintf("CActiveMasternode::ManageState -- %s\n", strNotCapableReason);
                return;
            }

            //update to masternode list
            LogPrintf("CActiveMasternode::ManageState -- Update Masternode List\n");
            mnodeman.UpdateMasternodeList(mnb);

            //send to all peers
            LogPrintf("CActiveMasternode::ManageState -- Relay broadcast, vin=%s\n", vin.ToString());
            mnb.Relay();

            LogPrintf("CActiveMasternode::ManageState -- Is capable master node!\n");
            nState = ACTIVE_MASTERNODE_STARTED;

            return;
        } else {
            strNotCapableReason = "Could not find suitable coins!";
            LogPrintf("CActiveMasternode::ManageState -- %s\n", strNotCapableReason);
            return;
        }
    }

    //send to all peers
    if(!SendMasternodePing(strErrorMessage)) {
        LogPrintf("CActiveMasternode::ManageState -- Error on SendMasternodePing(): %s\n", strErrorMessage);
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

bool CActiveMasternode::SendMasternodePing(std::string& strErrorMessage)
{
    if(nState != ACTIVE_MASTERNODE_STARTED) {
        strErrorMessage = "Masternode is not in a running status";
        return false;
    }

    CMasternodePing mnp(vin);
    if(!mnp.Sign(keyMasternode, pubKeyMasternode)) {
        strErrorMessage = "Couldn't sign Masternode Ping";
        return false;
    }

    // Update lastPing for our masternode in Masternode list
    CMasternode* pmn = mnodeman.Find(vin);
    if(pmn != NULL) {
        if(pmn->IsPingedWithin(MASTERNODE_MIN_MNP_SECONDS, mnp.sigTime)) {
            strErrorMessage = "Too early to send Masternode Ping";
            return false;
        }

        pmn->lastPing = mnp;
        mnodeman.mapSeenMasternodePing.insert(std::make_pair(mnp.GetHash(), mnp));

        //mnodeman.mapSeenMasternodeBroadcast.lastPing is probably outdated, so we'll update it
        CMasternodeBroadcast mnb(*pmn);
        uint256 hash = mnb.GetHash();
        if(mnodeman.mapSeenMasternodeBroadcast.count(hash))
            mnodeman.mapSeenMasternodeBroadcast[hash].lastPing = mnp;

        LogPrintf("CActiveMasternode::SendMasternodePing -- Relaying ping, collateral=%s\n", vin.ToString());
        mnp.Relay();

        return true;
    } else {
        // Seems like we are trying to send a ping while the Masternode is not registered in the network
        strErrorMessage = "PrivateSend Masternode List doesn't include our Masternode, shutting down Masternode pinging service! " + vin.ToString();
        nState = ACTIVE_MASTERNODE_NOT_CAPABLE;
        strNotCapableReason = strErrorMessage;
        return false;
    }
}

// when starting a Masternode, this can enable to run as a hot wallet with no funds
bool CActiveMasternode::EnableHotColdMasterNode(CTxIn& newVin, CService& newService)
{
    if(!fMasterNode) return false;

    nState = ACTIVE_MASTERNODE_STARTED;

    //The values below are needed for signing mnping messages going forward
    vin = newVin;
    service = newService;

    LogPrintf("CActiveMasternode::EnableHotColdMasterNode -- Enabled! You may shut down the cold daemon.\n");

    return true;
}
