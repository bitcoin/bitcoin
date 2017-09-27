// Copyright (c) 2014-2015 The Crown developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "servicenode.h"
#include "servicenodeman.h"
#include "darksend.h"
#include "util.h"
#include "sync.h"
#include "addrman.h"
#include <boost/lexical_cast.hpp>

CServicenodePing::CServicenodePing()
{
    vin = CTxIn();
    blockHash = uint256();
    sigTime = 0;
    vchSig = std::vector<unsigned char>();
}

CServicenodePing::CServicenodePing(CTxIn& newVin)
{
    vin = newVin;
    blockHash = chainActive[chainActive.Height() - 12]->GetBlockHash();
    sigTime = GetAdjustedTime();
    vchSig = std::vector<unsigned char>();
}

bool CServicenodePing::Sign(CKey& keyServicenode, CPubKey& pubKeyServicenode)
{
    std::string errorMessage;
    std::string strThroNeSignMessage;

    sigTime = GetAdjustedTime();
    std::string strMessage = vin.ToString() + blockHash.ToString() + boost::lexical_cast<std::string>(sigTime);

    if(!darkSendSigner.SignMessage(strMessage, errorMessage, vchSig, keyServicenode)) {
        LogPrintf("CServicenodePing::Sign() - Error: %s\n", errorMessage);
        return false;
    }

    if(!darkSendSigner.VerifyMessage(pubKeyServicenode, vchSig, strMessage, errorMessage)) {
        LogPrintf("CServicenodePing::Sign() - Error: %s\n", errorMessage);
        return false;
    }

    return true;
}

bool CServicenode::IsValidNetAddr()
{
    // TODO: regtest is fine with any addresses for now,
    // should probably be a bit smarter if one day we start to implement tests for this
    return (addr.IsIPv4() && addr.IsRoutable());
}

//
// When a new servicenode broadcast is sent, update our information
//
bool CServicenode::UpdateFromNewBroadcast(CServicenodeBroadcast& snb)
{
    if(snb.sigTime > sigTime) {    
        pubkey2 = snb.pubkey2;
        sigTime = snb.sigTime;
        sig = snb.sig;
        protocolVersion = snb.protocolVersion;
        addr = snb.addr;
        lastTimeChecked = 0;
        int nDoS = 0;
        //if(snb.lastPing == CServicenodePing() || (snb.lastPing != CServicenodePing() && snb.lastPing.CheckAndUpdate(nDoS, false))) {
        //    lastPing = snb.lastPing;
        //    snodeman.mapSeenServicenodePing.insert(make_pair(lastPing.GetHash(), lastPing));
        //}
        return true;
    }
    return false;
}

void CServicenode::Check(bool forceCheck)
{
    if(ShutdownRequested()) return;

    if(!forceCheck && (GetTime() - lastTimeChecked < SERVICENODE_CHECK_SECONDS)) return;
    lastTimeChecked = GetTime();


    //once spent, stop doing the checks
    if(activeState == SERVICENODE_VIN_SPENT) return;


    if(!IsPingedWithin(SERVICENODE_REMOVAL_SECONDS)){
        activeState = SERVICENODE_REMOVE;
        return;
    }

    if(!IsPingedWithin(SERVICENODE_EXPIRATION_SECONDS)){
        activeState = SERVICENODE_EXPIRED;
        return;
    }

    if(!unitTest){
        CValidationState state;
        CMutableTransaction tx = CMutableTransaction();
        CTxOut vout = CTxOut(9999.99*COIN, darkSendPool.collateralPubKey);
        tx.vin.push_back(vin);
        tx.vout.push_back(vout);

        {
            TRY_LOCK(cs_main, lockMain);
            if(!lockMain) return;

            if(!AcceptableInputs(mempool, state, CTransaction(tx), false, NULL)){
                activeState = SERVICENODE_VIN_SPENT;
                return;
            }
        }
    }
    activeState = SERVICENODE_ENABLED; // OK
}

bool CServicenodeBroadcast::CheckAndUpdate(int& nDos)
{
    nDos = 0;

    // make sure signature isn't in the future (past is OK)
    if (sigTime > GetAdjustedTime() + 60 * 60) {
        LogPrintf("snb - Signature rejected, too far into the future %s\n", vin.ToString());
        nDos = 1;
        return false;
    }

    //if(protocolVersion < servicenodePayments.GetMinServicenodePaymentsProto()) {
    //    LogPrintf("snb - ignoring outdated servicenode %s protocol version %d\n", vin.ToString(), protocolVersion);
    //    return false;
    //}

    CScript pubkeyScript;
    pubkeyScript = GetScriptForDestination(pubkey.GetID());

    if(pubkeyScript.size() != 25) {
        LogPrintf("snb - pubkey the wrong size\n");
        nDos = 100;
        return false;
    }

    CScript pubkeyScript2;
    pubkeyScript2 = GetScriptForDestination(pubkey2.GetID());

    if(pubkeyScript2.size() != 25) {
        LogPrintf("snb - pubkey2 the wrong size\n");
        nDos = 100;
        return false;
    }

    if(!vin.scriptSig.empty()) {
        LogPrintf("snb - Ignore Not Empty ScriptSig %s\n",vin.ToString());
        return false;
    }

    // incorrect ping or its sigTime
    //if(lastPing == CservicenodePing() || !lastPing.CheckAndUpdate(nDos, false, true))
    //    return false;

    std::string strMessage;
    std::string errorMessage = "";

    if(protocolVersion <= 99999999) {
        std::string vchPubKey(pubkey.begin(), pubkey.end());
        std::string vchPubKey2(pubkey2.begin(), pubkey2.end());
        strMessage = addr.ToString(false) + boost::lexical_cast<std::string>(sigTime) +
                        vchPubKey + vchPubKey2 + boost::lexical_cast<std::string>(protocolVersion);

        LogPrint("servicenode", "snb - sanitized strMessage: %s, pubkey address: %s, sig: %s\n",
            SanitizeString(strMessage), CBitcoinAddress(pubkey.GetID()).ToString(),
            EncodeBase64(&sig[0], sig.size()));

        if(!darkSendSigner.VerifyMessage(pubkey, sig, strMessage, errorMessage)){
            if (addr.ToString() != addr.ToString(false))
            {
                // maybe it's wrong format, try again with the old one
                strMessage = addr.ToString() + boost::lexical_cast<std::string>(sigTime) +
                                vchPubKey + vchPubKey2 + boost::lexical_cast<std::string>(protocolVersion);

                LogPrint("servicenode", "snb - sanitized strMessage: %s, pubkey address: %s, sig: %s\n",
                    SanitizeString(strMessage), CBitcoinAddress(pubkey.GetID()).ToString(),
                    EncodeBase64(&sig[0], sig.size()));

                if(!darkSendSigner.VerifyMessage(pubkey, sig, strMessage, errorMessage)){
                    // didn't work either
                    LogPrintf("snb - Got bad servicenode address signature, sanitized error: %s\n", SanitizeString(errorMessage));
                    // there is a bug in old MN signatures, ignore such MN but do not ban the peer we got this from
                    return false;
                }
            } else {
                // nope, sig is actually wrong
                LogPrintf("snb - Got bad servicenode address signature, sanitized error: %s\n", SanitizeString(errorMessage));
                // there is a bug in old MN signatures, ignore such MN but do not ban the peer we got this from
                return false;
            }
        }
    } else {
        strMessage = addr.ToString(false) + boost::lexical_cast<std::string>(sigTime) +
                        pubkey.GetID().ToString() + pubkey2.GetID().ToString() +
                        boost::lexical_cast<std::string>(protocolVersion);

        LogPrint("servicenode", "snb - strMessage: %s, pubkey address: %s, sig: %s\n",
            strMessage, CBitcoinAddress(pubkey.GetID()).ToString(), EncodeBase64(&sig[0], sig.size()));

        if(!darkSendSigner.VerifyMessage(pubkey, sig, strMessage, errorMessage)){
            LogPrintf("snb - Got bad servicenode address signature, error: %s\n", errorMessage);
            nDos = 100;
            return false;
        }
    }

    if(Params().NetworkID() == CBaseChainParams::MAIN) {
        if(addr.GetPort() != 9340) return false;
    } else if(addr.GetPort() == 9340) return false;

    //search existing servicenode list, this is where we update existing Servicenodes with new snb broadcasts
    CServicenode* pmn = snodeman.Find(vin);

    // no such servicenode, nothing to update
    if(pmn == NULL) return true;

    // this broadcast is older or equal than the one that we already have - it's bad and should never happen
    // unless someone is doing something fishy
    // (mapSeenservicenodeBroadcast in CServicenodeMan::ProcessMessage should filter legit duplicates)
    if(pmn->sigTime >= sigTime) {
        LogPrintf("CservicenodeBroadcast::CheckAndUpdate - Bad sigTime %d for Servicenode %20s %105s (existing broadcast is at %d)\n",
                      sigTime, addr.ToString(), vin.ToString(), pmn->sigTime);
        return false;
    }

    // servicenode is not enabled yet/already, nothing to update
    if(!pmn->IsEnabled()) return true;

    // mn.pubkey = pubkey, IsVinAssociatedWithPubkey is validated once below,
    //   after that they just need to match
    if(pmn->pubkey == pubkey && !pmn->IsBroadcastedWithin(SERVICENODE_MIN_SNB_SECONDS)) {
        //take the newest entry
        LogPrintf("snb - Got updated entry for %s\n", addr.ToString());
        if(pmn->UpdateFromNewBroadcast((*this))){
            pmn->Check();
            if(pmn->IsEnabled()) Relay();
        }
    //    servicenodeSync.AddedServicenodeList(GetHash());
    }

    return true;

}

bool CServicenodeBroadcast::CheckInputsAndAdd(int& nDoS)
{
    // we are a servicenode with the same vin (i.e. already activated) and this snb is ours (matches our servicenode privkey)
    // so nothing to do here for us
//    if(fservicenode && vin.prevout == activeservicenode.vin.prevout && pubkey2 == activeservicenode.pubKeyservicenode)
//        return true;
//
//    // incorrect ping or its sigTime
//    if(lastPing == CservicenodePing() || !lastPing.CheckAndUpdate(nDoS, false, true))
//        return false;
//
    // search existing servicenode list
    CServicenode* pmn = snodeman.Find(vin);

    if(pmn != NULL) {
        // nothing to do here if we already know about this servicenode and it's enabled
        if(pmn->IsEnabled()) return true;
        // if it's not enabled, remove old MN first and continue
        else snodeman.Remove(pmn->vin);
    }

    CValidationState state;
    CMutableTransaction tx = CMutableTransaction();
    CTxOut vout = CTxOut(9999.99*COIN, darkSendPool.collateralPubKey);
    tx.vin.push_back(vin);
    tx.vout.push_back(vout);

    {
        TRY_LOCK(cs_main, lockMain);
        if(!lockMain) {
            // not snb fault, let it to be checked again later
            snodeman.mapSeenServicenodeBroadcast.erase(GetHash());
//            servicenodeSync.mapSeenSyncMNB.erase(GetHash());
            return false;
        }

        if(!AcceptableInputs(mempool, state, CTransaction(tx), false, NULL)) {
            //set nDos
            state.IsInvalid(nDoS);
            return false;
        }
    }

    LogPrint("servicenode", "snb - Accepted servicenode entry\n");

    if(GetInputAge(vin) < SERVICENODE_MIN_CONFIRMATIONS){
        LogPrintf("snb - Input must have at least %d confirmations\n", SERVICENODE_MIN_CONFIRMATIONS);
        // maybe we miss few blocks, let this snb to be checked again later
        snodeman.mapSeenServicenodeBroadcast.erase(GetHash());
        //servicenodeSync.mapSeenSyncMNB.erase(GetHash());
        return false;
    }

    // verify that sig time is legit in past
    // should be at least not earlier than block when 10000 CRW tx got SERVICENODE_MIN_CONFIRMATIONS
    uint256 hashBlock = uint256();
    CTransaction tx2;
    GetTransaction(vin.prevout.hash, tx2, hashBlock, true);
    BlockMap::iterator mi = mapBlockIndex.find(hashBlock);
    if (mi != mapBlockIndex.end() && (*mi).second)
    {
        CBlockIndex* pMNIndex = (*mi).second; // block for 10000 CRW tx -> 1 confirmation
        CBlockIndex* pConfIndex = chainActive[pMNIndex->nHeight + SERVICENODE_MIN_CONFIRMATIONS - 1]; // block where tx got SERVICENODE_MIN_CONFIRMATIONS
        if(pConfIndex->GetBlockTime() > sigTime)
        {
            LogPrintf("snb - Bad sigTime %d for servicenode %20s %105s (%i conf block is at %d)\n",
                      sigTime, addr.ToString(), vin.ToString(), SERVICENODE_MIN_CONFIRMATIONS, pConfIndex->GetBlockTime());
            return false;
        }
    }

    LogPrintf("snb - Got NEW servicenode entry - %s - %s - %s - %lli \n", GetHash().ToString(), addr.ToString(), vin.ToString(), sigTime);
    CServicenode mn(*this);
    snodeman.Add(mn);

    // if it matches our servicenode privkey, then we've been remotely activated
//    if(pubkey2 == activeservicenode.pubKeyservicenode && protocolVersion == PROTOCOL_VERSION){
//        activeservicenode.EnableHotColdservicenode(vin, addr);
//    }
//
    bool isLocal = addr.IsRFC1918() || addr.IsLocal();
    if(Params().NetworkID() == CBaseChainParams::REGTEST) isLocal = false;

    if(!isLocal) Relay();

    return true;

}

void CServicenodeBroadcast::Relay()
{
    CInv inv(MSG_SERVICENODE_ANNOUNCE, GetHash());
    RelayInv(inv);
}

CServicenodeBroadcast::CServicenodeBroadcast()
{
    vin = CTxIn();
    addr = CService();
    pubkey = CPubKey();
    pubkey2 = CPubKey();
    sig = std::vector<unsigned char>();
    activeState = SERVICENODE_ENABLED;
    sigTime = GetAdjustedTime();
    lastPing = CServicenodePing();
    unitTest = false;
    protocolVersion = PROTOCOL_VERSION;
}

CServicenodeBroadcast::CServicenodeBroadcast(CService newAddr, CTxIn newVin, CPubKey newPubkey, CPubKey newPubkey2, int protocolVersionIn)
{
    vin = newVin;
    addr = newAddr;
    pubkey = newPubkey;
    pubkey2 = newPubkey2;
    sig = std::vector<unsigned char>();
    activeState = SERVICENODE_ENABLED;
    sigTime = GetAdjustedTime();
    lastPing = CServicenodePing();
    unitTest = false;
    protocolVersion = protocolVersionIn;
}

CServicenodeBroadcast::CServicenodeBroadcast(const CServicenode& mn)
{
    vin = mn.vin;
    addr = mn.addr;
    pubkey = mn.pubkey;
    pubkey2 = mn.pubkey2;
    sig = mn.sig;
    activeState = mn.activeState;
    sigTime = mn.sigTime;
    lastPing = mn.lastPing;
    unitTest = mn.unitTest;
    protocolVersion = mn.protocolVersion;
}


bool CServicenodeBroadcast::Create(std::string strService, std::string strKeyServicenode, std::string strTxHash, std::string strOutputIndex, std::string& strErrorMessage, CServicenodeBroadcast &mnb, bool fOffline) {
    CTxIn txin;
    CPubKey pubKeyCollateralAddress;
    CKey keyCollateralAddress;
    CPubKey pubKeyServicenodeNew;
    CKey keyServicenodeNew;

    //need correct blocks to send ping
    //if(!fOffline && !servicenodeSync.IsBlockchainSynced()) {
    //    strErrorMessage = "Sync in progress. Must wait until sync is complete to start Servicenode";
    //    LogPrintf("CServicenodeBroadcast::Create -- %s\n", strErrorMessage);
    //    return false;
    //}

    if(!darkSendSigner.SetKey(strKeyServicenode, strErrorMessage, keyServicenodeNew, pubKeyServicenodeNew))
    {
        strErrorMessage = strprintf("Can't find keys for servicenode %s - %s", strService, strErrorMessage);
        LogPrintf("CServicenodeBroadcast::Create -- %s\n", strErrorMessage);
        return false;
    }

    if(!pwalletMain->GetServicenodeVinAndKeys(txin, pubKeyCollateralAddress, keyCollateralAddress, strTxHash, strOutputIndex)) {
        strErrorMessage = strprintf("Could not allocate txin %s:%s for servicenode %s", strTxHash, strOutputIndex, strService);
        LogPrintf("CServicenodeBroadcast::Create -- %s\n", strErrorMessage);
        return false;
    }

    CService service = CService(strService);
    if(Params().NetworkID() == CBaseChainParams::MAIN) {
        if(service.GetPort() != 9340) {
            strErrorMessage = strprintf("Invalid port %u for servicenode %s - only 9340 is supported on mainnet.", service.GetPort(), strService);
            LogPrintf("CServicenodeBroadcast::Create -- %s\n", strErrorMessage);
            return false;
        }
    } else if(service.GetPort() == 9340) {
        strErrorMessage = strprintf("Invalid port %u for servicenode %s - 9340 is only supported on mainnet.", service.GetPort(), strService);
        LogPrintf("CServicenodeBroadcast::Create -- %s\n", strErrorMessage);
        return false;
    }

    return Create(txin, CService(strService), keyCollateralAddress, pubKeyCollateralAddress, keyServicenodeNew, pubKeyServicenodeNew, strErrorMessage, mnb);
}

bool CServicenodeBroadcast::Create(CTxIn txin, CService service, CKey keyCollateralAddress, CPubKey pubKeyCollateralAddress, CKey keyServicenodeNew, CPubKey pubKeyServicenodeNew, std::string &strErrorMessage, CServicenodeBroadcast &snb) {
    // wait for reindex and/or import to finish
    if (fImporting || fReindex) return false;

    CServicenodePing snp(txin);
    if(!snp.Sign(keyServicenodeNew, pubKeyServicenodeNew)){
        strErrorMessage = strprintf("Failed to sign ping, txin: %s", txin.ToString());
        LogPrintf("CServicenodeBroadcast::Create --  %s\n", strErrorMessage);
        snb = CServicenodeBroadcast();
        return false;
    }

    snb = CServicenodeBroadcast(service, txin, pubKeyCollateralAddress, pubKeyServicenodeNew, PROTOCOL_VERSION);

    if(!snb.IsValidNetAddr()) {
        strErrorMessage = strprintf("Invalid IP address, servicenode=%s", txin.prevout.ToStringShort());
        LogPrintf("CServicenodeBroadcast::Create -- %s\n", strErrorMessage);
        snb = CServicenodeBroadcast();
        return false;
    }

    snb.lastPing = snp;
    if(!snb.Sign(keyCollateralAddress)){
        strErrorMessage = strprintf("Failed to sign broadcast, txin: %s", txin.ToString());
        LogPrintf("CServicenodeBroadcast::Create -- %s\n", strErrorMessage);
        snb = CServicenodeBroadcast();
        return false;
    }

    return true;
}

bool CServicenodeBroadcast::Sign(CKey& keyCollateralAddress)
{
    std::string errorMessage;

    std::string vchPubKey(pubkey.begin(), pubkey.end());
    std::string vchPubKey2(pubkey2.begin(), pubkey2.end());

    sigTime = GetAdjustedTime();

    std::string strMessage = addr.ToString(false) + boost::lexical_cast<std::string>(sigTime) + vchPubKey + vchPubKey2 + boost::lexical_cast<std::string>(protocolVersion);

    if(!darkSendSigner.SignMessage(strMessage, errorMessage, sig, keyCollateralAddress)) {
        LogPrintf("CServicenodeBroadcast::Sign() - Error: %s\n", errorMessage);
        return false;
    }

    return true;
}

