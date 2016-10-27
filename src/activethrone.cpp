
#include "addrman.h"
#include "protocol.h"
#include "activethrone.h"
#include "throneman.h"
#include "throne.h"
#include "throneconfig.h"
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

        CNode *pnode = ConnectNode((CAddress)service, NULL, false);
        if(!pnode){
            notCapableReason = "Could not connect to " + service.ToString();
            LogPrintf("CActiveThrone::ManageStatus() - not capable: %s\n", notCapableReason);
            return;
        }
        pnode->Release();

        // Choose coins to use
        CPubKey pubKeyCollateralAddress;
        CKey keyCollateralAddress;

        if(GetThroNeVin(vin, pubKeyCollateralAddress, keyCollateralAddress)) {

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
            if(!CreateBroadcast(vin, service, keyCollateralAddress, pubKeyCollateralAddress, keyThrone, pubKeyThrone, errorMessage, mnb)) {
                notCapableReason = "Error on CreateBroadcast: " + errorMessage;
                LogPrintf("Register::ManageStatus() - %s\n", notCapableReason);
                return;
            }

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

bool CActiveThrone::CreateBroadcast(std::string strService, std::string strKeyThrone, std::string strTxHash, std::string strOutputIndex, std::string& errorMessage, CThroneBroadcast &mnb, bool fOffline) {
    CTxIn vin;
    CPubKey pubKeyCollateralAddress;
    CKey keyCollateralAddress;
    CPubKey pubKeyThrone;
    CKey keyThrone;

    //need correct blocks to send ping
    if(!fOffline && !throneSync.IsBlockchainSynced()) {
        errorMessage = "Sync in progress. Must wait until sync is complete to start Throne";
        LogPrintf("CActiveThrone::CreateBroadcast() - %s\n", errorMessage);
        return false;
    }

    if(!darkSendSigner.SetKey(strKeyThrone, errorMessage, keyThrone, pubKeyThrone))
    {
        errorMessage = strprintf("Can't find keys for throne %s - %s", strService, errorMessage);
        LogPrintf("CActiveThrone::CreateBroadcast() - %s\n", errorMessage);
        return false;
    }

    if(!GetThroNeVin(vin, pubKeyCollateralAddress, keyCollateralAddress, strTxHash, strOutputIndex)) {
        errorMessage = strprintf("Could not allocate vin %s:%s for throne %s", strTxHash, strOutputIndex, strService);
        LogPrintf("CActiveThrone::CreateBroadcast() - %s\n", errorMessage);
        return false;
    }

    CService service = CService(strService);
    if(Params().NetworkID() == CBaseChainParams::MAIN) {
        if(service.GetPort() != 9340) {
            errorMessage = strprintf("Invalid port %u for throne %s - only 9340 is supported on mainnet.", service.GetPort(), strService);
            LogPrintf("CActiveThrone::CreateBroadcast() - %s\n", errorMessage);
            return false;
        }
    } else if(service.GetPort() == 9340) {
        errorMessage = strprintf("Invalid port %u for throne %s - 9340 is only supported on mainnet.", service.GetPort(), strService);
        LogPrintf("CActiveThrone::CreateBroadcast() - %s\n", errorMessage);
        return false;
    }

    addrman.Add(CAddress(service), CNetAddr("127.0.0.1"), 2*60*60);

    return CreateBroadcast(vin, CService(strService), keyCollateralAddress, pubKeyCollateralAddress, keyThrone, pubKeyThrone, errorMessage, mnb);
}

bool CActiveThrone::CreateBroadcast(CTxIn vin, CService service, CKey keyCollateralAddress, CPubKey pubKeyCollateralAddress, CKey keyThrone, CPubKey pubKeyThrone, std::string &errorMessage, CThroneBroadcast &mnb) {
    // wait for reindex and/or import to finish
    if (fImporting || fReindex) return false;

    CThronePing mnp(vin);
    if(!mnp.Sign(keyThrone, pubKeyThrone)){
        errorMessage = strprintf("Failed to sign ping, vin: %s", vin.ToString());
        LogPrintf("CActiveThrone::CreateBroadcast() -  %s\n", errorMessage);
        mnb = CThroneBroadcast();
        return false;
    }

    mnb = CThroneBroadcast(service, vin, pubKeyCollateralAddress, pubKeyThrone, PROTOCOL_VERSION);
    mnb.lastPing = mnp;
    if(!mnb.Sign(keyCollateralAddress)){
        errorMessage = strprintf("Failed to sign broadcast, vin: %s", vin.ToString());
        LogPrintf("CActiveThrone::CreateBroadcast() - %s\n", errorMessage);
        mnb = CThroneBroadcast();
        return false;
    }

    return true;
}

bool CActiveThrone::GetThroNeVin(CTxIn& vin, CPubKey& pubkey, CKey& secretKey) {
    return GetThroNeVin(vin, pubkey, secretKey, "", "");
}

bool CActiveThrone::GetThroNeVin(CTxIn& vin, CPubKey& pubkey, CKey& secretKey, std::string strTxHash, std::string strOutputIndex) {
    // wait for reindex and/or import to finish
    if (fImporting || fReindex) return false;

    // Find possible candidates
    TRY_LOCK(pwalletMain->cs_wallet, fWallet);
    if(!fWallet) return false;

    vector<COutput> possibleCoins = SelectCoinsThrone();
    COutput *selectedOutput;

    // Find the vin
    if(!strTxHash.empty()) {
        // Let's find it
        uint256 txHash(strTxHash);
        int outputIndex = atoi(strOutputIndex.c_str());
        bool found = false;
        BOOST_FOREACH(COutput& out, possibleCoins) {
            if(out.tx->GetHash() == txHash && out.i == outputIndex)
            {
                selectedOutput = &out;
                found = true;
                break;
            }
        }
        if(!found) {
            LogPrintf("CActiveThrone::GetThroNeVin - Could not locate valid vin\n");
            return false;
        }
    } else {
        // No output specified,  Select the first one
        if(possibleCoins.size() > 0) {
            selectedOutput = &possibleCoins[0];
        } else {
            LogPrintf("CActiveThrone::GetThroNeVin - Could not locate specified vin from possible list\n");
            return false;
        }
    }

    // At this point we have a selected output, retrieve the associated info
    return GetVinFromOutput(*selectedOutput, vin, pubkey, secretKey);
}


// Extract Throne vin information from output
bool CActiveThrone::GetVinFromOutput(COutput out, CTxIn& vin, CPubKey& pubkey, CKey& secretKey) {
    // wait for reindex and/or import to finish
    if (fImporting || fReindex) return false;

    CScript pubScript;

    vin = CTxIn(out.tx->GetHash(),out.i);
    pubScript = out.tx->vout[out.i].scriptPubKey; // the inputs PubKey

    CTxDestination address1;
    ExtractDestination(pubScript, address1);
    CBitcoinAddress address2(address1);

    CKeyID keyID;
    if (!address2.GetKeyID(keyID)) {
        LogPrintf("CActiveThrone::GetThroNeVin - Address does not refer to a key\n");
        return false;
    }

    if (!pwalletMain->GetKey(keyID, secretKey)) {
        LogPrintf ("CActiveThrone::GetThroNeVin - Private key for address is not known\n");
        return false;
    }

    pubkey = secretKey.GetPubKey();
    return true;
}

// get all possible outputs for running Throne
vector<COutput> CActiveThrone::SelectCoinsThrone()
{
    vector<COutput> vCoins;
    vector<COutput> filteredCoins;
    vector<COutPoint> confLockedCoins;

    // Temporary unlock MN coins from throne.conf
    if(GetBoolArg("-mnconflock", true)) {
        uint256 mnTxHash;
        BOOST_FOREACH(CThroneConfig::CThroneEntry mne, throneConfig.getEntries()) {
            mnTxHash.SetHex(mne.getTxHash());
            COutPoint outpoint = COutPoint(mnTxHash, atoi(mne.getOutputIndex().c_str()));
            confLockedCoins.push_back(outpoint);
            pwalletMain->UnlockCoin(outpoint);
        }
    }

    // Retrieve all possible outputs
    pwalletMain->AvailableCoins(vCoins);

    // Lock MN coins from throne.conf back if they where temporary unlocked
    if(!confLockedCoins.empty()) {
        BOOST_FOREACH(COutPoint outpoint, confLockedCoins)
            pwalletMain->LockCoin(outpoint);
    }

    // Filter
    BOOST_FOREACH(const COutput& out, vCoins)
    {
        if(out.tx->vout[out.i].nValue == 10000*COIN) { //exactly
            filteredCoins.push_back(out);
        }
    }
    return filteredCoins;
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
