
#include "addrman.h"
#include "protocol.h"
#include "activethrone.h"
#include "throneman.h"
#include "throne.h"
#include "spork.h"

//
// Bootup the Throne, look for a 10000 CRW input and register on the network
//
void CActiveThrone::ManageStatus()
{    
    std::string errorMessage;

    if(!fThroNe) return;

    if (fDebug) LogPrintf("CActiveThrone::ManageStatus() - Begin\n");

    //need correct blocks to send ping
    if(Params().NetworkID() != CBaseChainParams::REGTEST && !throneSync.IsBlockchainSynced()) {
        status = ACTIVE_THRONE_SYNC_IN_PROCESS;
        LogPrintf("CActiveThrone::ManageStatus() - %s\n", GetStatus());
        return;
    }

    if(status == ACTIVE_THRONE_SYNC_IN_PROCESS) status = ACTIVE_THRONE_INITIAL;

    if(status == ACTIVE_THRONE_INITIAL) {
        CThrone *pmn;
        pmn = mnodeman.Find(pubKeyThrone);
        if(pmn != NULL) {
            pmn->Check();
            if(pmn->IsEnabled() && pmn->protocolVersion == PROTOCOL_VERSION) EnableHotColdThroNe(pmn->vin, pmn->addr);
        }
    }

    if(status != ACTIVE_THRONE_STARTED) {

        // Set defaults
        status = ACTIVE_THRONE_NOT_CAPABLE;
        notCapableReason = "";

        if(pwalletMain->IsLocked()){
            notCapableReason = "Wallet is locked.";
            LogPrintf("CActiveThrone::ManageStatus() - not capable: %s\n", notCapableReason);
            return;
        }

        if(pwalletMain->GetBalance() == 0){
            notCapableReason = "Hot node, waiting for remote activation.";
            LogPrintf("CActiveThrone::ManageStatus() - not capable: %s\n", notCapableReason);
            return;
        }

        if(strThroNeAddr.empty()) {
            if(!GetLocal(service)) {
                notCapableReason = "Can't detect external address. Please use the throneaddr configuration option.";
                LogPrintf("CActiveThrone::ManageStatus() - not capable: %s\n", notCapableReason);
                return;
            }
        } else {
            service = CService(strThroNeAddr);
        }

        if(Params().NetworkID() == CBaseChainParams::MAIN) {
            if(service.GetPort() != 9340) {
                notCapableReason = strprintf("Invalid port: %u - only 9340 is supported on mainnet.", service.GetPort());
                LogPrintf("CActiveThrone::ManageStatus() - not capable: %s\n", notCapableReason);
                return;
            }
        } else if(service.GetPort() == 9340) {
            notCapableReason = strprintf("Invalid port: %u - 9340 is only supported on mainnet.", service.GetPort());
            LogPrintf("CActiveThrone::ManageStatus() - not capable: %s\n", notCapableReason);
            return;
        }

        LogPrintf("CActiveThrone::ManageStatus() - Checking inbound connection to '%s'\n", service.ToString());

        if(!ConnectNode((CAddress)service, NULL, true)){
            notCapableReason = "Could not connect to " + service.ToString();
            LogPrintf("CActiveThrone::ManageStatus() - not capable: %s\n", notCapableReason);
            return;
        }

        // Choose coins to use
        CPubKey pubKeyCollateralAddress;
        CKey keyCollateralAddress;

        if(pwalletMain->GetThroneVinAndKeys(vin, pubKeyCollateralAddress, keyCollateralAddress)) {

            if(GetInputAge(vin) < THRONE_MIN_CONFIRMATIONS){
                status = ACTIVE_THRONE_INPUT_TOO_NEW;
                notCapableReason = strprintf("%s - %d confirmations", GetStatus(), GetInputAge(vin));
                LogPrintf("CActiveThrone::ManageStatus() - %s\n", notCapableReason);
                return;
            }

            LOCK(pwalletMain->cs_wallet);
            pwalletMain->LockCoin(vin.prevout);

            // send to all nodes
            CPubKey pubKeyThrone;
            CKey keyThrone;

            if(!darkSendSigner.SetKey(strThroNePrivKey, errorMessage, keyThrone, pubKeyThrone))
            {
                notCapableReason = "Error upon calling SetKey: " + errorMessage;
                LogPrintf("Register::ManageStatus() - %s\n", notCapableReason);
                return;
            }

            CThroneBroadcast mnb;
            if(!CThroneBroadcast::Create(vin, service, keyCollateralAddress, pubKeyCollateralAddress, keyThrone, pubKeyThrone, errorMessage, mnb)) {
                notCapableReason = "Error on CreateBroadcast: " + errorMessage;
                LogPrintf("Register::ManageStatus() - %s\n", notCapableReason);
                return;
            }

            //update to masternode list
            LogPrintf("CActiveThrone::ManageStatus() - Update Throne List\n");
            mnodeman.UpdateThroneList(mnb);

            //send to all peers
            LogPrintf("CActiveThrone::ManageStatus() - Relay broadcast vin = %s\n", vin.ToString());
            mnb.Relay();

            LogPrintf("CActiveThrone::ManageStatus() - Is capable master node!\n");
            status = ACTIVE_THRONE_STARTED;

            return;
        } else {
            notCapableReason = "Could not find suitable coins!";
            LogPrintf("CActiveThrone::ManageStatus() - %s\n", notCapableReason);
            return;
        }
    }

    //send to all peers
    if(!SendThronePing(errorMessage)) {
        LogPrintf("CActiveThrone::ManageStatus() - Error on Ping: %s\n", errorMessage);
    }
}

std::string CActiveThrone::GetStatus() {
    switch (status) {
    case ACTIVE_THRONE_INITIAL: return "Node just started, not yet activated";
    case ACTIVE_THRONE_SYNC_IN_PROCESS: return "Sync in progress. Must wait until sync is complete to start Throne";
    case ACTIVE_THRONE_INPUT_TOO_NEW: return strprintf("Throne input must have at least %d confirmations", THRONE_MIN_CONFIRMATIONS);
    case ACTIVE_THRONE_NOT_CAPABLE: return "Not capable throne: " + notCapableReason;
    case ACTIVE_THRONE_STARTED: return "Throne successfully started";
    default: return "unknown";
    }
}

bool CActiveThrone::SendThronePing(std::string& errorMessage) {
    if(status != ACTIVE_THRONE_STARTED) {
        errorMessage = "Throne is not in a running status";
        return false;
    }

    CPubKey pubKeyThrone;
    CKey keyThrone;

    if(!darkSendSigner.SetKey(strThroNePrivKey, errorMessage, keyThrone, pubKeyThrone))
    {
        errorMessage = strprintf("Error upon calling SetKey: %s\n", errorMessage);
        return false;
    }

    LogPrintf("CActiveThrone::SendThronePing() - Relay Throne Ping vin = %s\n", vin.ToString());
    
    CThronePing mnp(vin);
    if(!mnp.Sign(keyThrone, pubKeyThrone))
    {
        errorMessage = "Couldn't sign Throne Ping";
        return false;
    }

    // Update lastPing for our throne in Throne list
    CThrone* pmn = mnodeman.Find(vin);
    if(pmn != NULL)
    {
        if(pmn->IsPingedWithin(THRONE_PING_SECONDS, mnp.sigTime)){
            errorMessage = "Too early to send Throne Ping";
            return false;
        }

        pmn->lastPing = mnp;
        mnodeman.mapSeenThronePing.insert(make_pair(mnp.GetHash(), mnp));

        //mnodeman.mapSeenThroneBroadcast.lastPing is probably outdated, so we'll update it
        CThroneBroadcast mnb(*pmn);
        uint256 hash = mnb.GetHash();
        if(mnodeman.mapSeenThroneBroadcast.count(hash)) mnodeman.mapSeenThroneBroadcast[hash].lastPing = mnp;

        mnp.Relay();

        return true;
    }
    else
    {
        // Seems like we are trying to send a ping while the Throne is not registered in the network
        errorMessage = "Darksend Throne List doesn't include our Throne, shutting down Throne pinging service! " + vin.ToString();
        status = ACTIVE_THRONE_NOT_CAPABLE;
        notCapableReason = errorMessage;
        return false;
    }

}

// when starting a Throne, this can enable to run as a hot wallet with no funds
bool CActiveThrone::EnableHotColdThroNe(CTxIn& newVin, CService& newService)
{
    if(!fThroNe) return false;

    status = ACTIVE_THRONE_STARTED;

    //The values below are needed for signing mnping messages going forward
    vin = newVin;
    service = newService;

    LogPrintf("CActiveThrone::EnableHotColdThroNe() - Enabled! You may shut down the cold daemon.\n");

    return true;
}
