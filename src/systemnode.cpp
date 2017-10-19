// Copyright (c) 2014-2015 The Crown developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "systemnode.h"
#include "systemnodeman.h"
#include "systemnode-sync.h"
#include "activesystemnode.h"
#include "legacysigner.h"
#include "util.h"
#include "sync.h"
#include "addrman.h"
#include <boost/lexical_cast.hpp>

int GetMinSystemnodePaymentsProto();

//
// CSystemnodePing
//

CSystemnodePing::CSystemnodePing()
{
    vin = CTxIn();
    blockHash = uint256();
    sigTime = 0;
    vchSig = std::vector<unsigned char>();
}

CSystemnodePing::CSystemnodePing(CTxIn& newVin)
{
    vin = newVin;
    blockHash = chainActive[chainActive.Height() - 12]->GetBlockHash();
    sigTime = GetAdjustedTime();
    vchSig = std::vector<unsigned char>();
}

bool CSystemnodePing::Sign(CKey& keySystemnode, CPubKey& pubKeySystemnode)
{
    std::string errorMessage;
    std::string strThroNeSignMessage;

    sigTime = GetAdjustedTime();
    std::string strMessage = vin.ToString() + blockHash.ToString() + boost::lexical_cast<std::string>(sigTime);

    if(!legacySigner.SignMessage(strMessage, errorMessage, vchSig, keySystemnode)) {
        LogPrintf("CSystemnodePing::Sign() - Error: %s\n", errorMessage);
        return false;
    }

    if(!legacySigner.VerifyMessage(pubKeySystemnode, vchSig, strMessage, errorMessage)) {
        LogPrintf("CSystemnodePing::Sign() - Error: %s\n", errorMessage);
        return false;
    }

    return true;
}

void CSystemnodePing::Relay()
{
    CInv inv(MSG_SYSTEMNODE_PING, GetHash());
    RelayInv(inv);
}

bool CSystemnodePing::CheckAndUpdate(int& nDos, bool fRequireEnabled, bool fCheckSigTimeOnly)
{
    if (sigTime > GetAdjustedTime() + 60 * 60) {
        LogPrintf("CSystemnodePing::CheckAndUpdate - Signature rejected, too far into the future %s\n", vin.ToString());
        nDos = 1;
        return false;
    }

    if (sigTime <= GetAdjustedTime() - 60 * 60) {
        LogPrintf("CSystemnodePing::CheckAndUpdate - Signature rejected, too far into the past %s - %d %d \n", vin.ToString(), sigTime, GetAdjustedTime());
        nDos = 1;
        return false;
    }

    if(fCheckSigTimeOnly) {
        CSystemnode* pmn = snodeman.Find(vin);
        if(pmn) return VerifySignature(pmn->pubkey2, nDos);
        return true;
    }

    LogPrint("systemnode", "CSystemnodePing::CheckAndUpdate - New Ping - %s - %s - %lli\n", GetHash().ToString(), blockHash.ToString(), sigTime);

    // see if we have this Systemnode
    CSystemnode* pmn = snodeman.Find(vin);
    if(pmn != NULL && pmn->protocolVersion >= GetMinSystemnodePaymentsProto())
    {
        if (fRequireEnabled && !pmn->IsEnabled()) return false;

        // LogPrintf("mnping - Found corresponding mn for vin: %s\n", vin.ToString());
        // update only if there is no known ping for this systemnode or
        // last ping was more then SYSTEMNODE_MIN_MNP_SECONDS-60 ago comparing to this one
        if(!pmn->IsPingedWithin(SYSTEMNODE_MIN_SNP_SECONDS - 60, sigTime))
        {
            if(!VerifySignature(pmn->pubkey2, nDos))
                return false;

            BlockMap::iterator mi = mapBlockIndex.find(blockHash);
            if (mi != mapBlockIndex.end() && (*mi).second)
            {
                if((*mi).second->nHeight < chainActive.Height() - 24)
                {
                    LogPrintf("CSystemnodePing::CheckAndUpdate - Systemnode %s block hash %s is too old\n", vin.ToString(), blockHash.ToString());
                    // Do nothing here (no Systemnode update, no mnping relay)
                    // Let this node to be visible but fail to accept mnping

                    return false;
                }
            } else {
                if (fDebug) LogPrintf("CSystemnodePing::CheckAndUpdate - Systemnode %s block hash %s is unknown\n", vin.ToString(), blockHash.ToString());
                // maybe we stuck so we shouldn't ban this node, just fail to accept it
                // TODO: or should we also request this block?

                return false;
            }

            pmn->lastPing = *this;

            //snodeman.mapSeenSystemnodeBroadcast.lastPing is probably outdated, so we'll update it
            CSystemnodeBroadcast mnb(*pmn);
            uint256 hash = mnb.GetHash();
            if(snodeman.mapSeenSystemnodeBroadcast.count(hash)) {
                snodeman.mapSeenSystemnodeBroadcast[hash].lastPing = *this;
            }

            pmn->Check(true);
            if(!pmn->IsEnabled()) return false;

            LogPrint("systemnode", "CSystemnodePing::CheckAndUpdate - Systemnode ping accepted, vin: %s\n", vin.ToString());

            Relay();
            return true;
        }
        LogPrint("systemnode", "CSystemnodePing::CheckAndUpdate - Systemnode ping arrived too early, vin: %s\n", vin.ToString());
        //nDos = 1; //disable, this is happening frequently and causing banned peers
        return false;
    }
    LogPrint("systemnode", "CSystemnodePing::CheckAndUpdate - Couldn't find compatible Systemnode entry, vin: %s\n", vin.ToString());

    return false;
}

bool CSystemnodePing::VerifySignature(CPubKey& pubKeySystemnode, int &nDos) {
    std::string strMessage = vin.ToString() + blockHash.ToString() + boost::lexical_cast<std::string>(sigTime);
    std::string errorMessage = "";

    if(!legacySigner.VerifyMessage(pubKeySystemnode, vchSig, strMessage, errorMessage))
    {
        LogPrintf("CSystemnodePing::VerifySignature - Got bad Systemnode ping signature %s Error: %s\n", vin.ToString(), errorMessage);
        nDos = 33;
        return false;
    }
    return true;
}

//
// CSystemnode
//

CSystemnode::CSystemnode()
{
    LOCK(cs);
    vin = CTxIn();
    addr = CService();
    pubkey = CPubKey();
    pubkey2 = CPubKey();
    sig = std::vector<unsigned char>();
    activeState = SYSTEMNODE_ENABLED;
    sigTime = GetAdjustedTime();
    lastPing = CSystemnodePing();
    unitTest = false;
    protocolVersion = PROTOCOL_VERSION;
    lastTimeChecked = 0;
}

CSystemnode::CSystemnode(const CSystemnode& other)
{
    LOCK(cs);
    vin = other.vin;
    addr = other.addr;
    pubkey = other.pubkey;
    pubkey2 = other.pubkey2;
    sig = other.sig;
    activeState = other.activeState;
    sigTime = other.sigTime;
    lastPing = other.lastPing;
    unitTest = other.unitTest;
    protocolVersion = other.protocolVersion;
    lastTimeChecked = 0;
}

CSystemnode::CSystemnode(const CSystemnodeBroadcast& snb)
{
    LOCK(cs);
    vin = snb.vin;
    addr = snb.addr;
    pubkey = snb.pubkey;
    pubkey2 = snb.pubkey2;
    sig = snb.sig;
    activeState = SYSTEMNODE_ENABLED;
    sigTime = snb.sigTime;
    lastPing = snb.lastPing;
    unitTest = false;
    protocolVersion = snb.protocolVersion;
}

bool CSystemnode::IsValidNetAddr()
{
    // TODO: regtest is fine with any addresses for now,
    // should probably be a bit smarter if one day we start to implement tests for this
    return (addr.IsIPv4() && addr.IsRoutable());
}

//
// Deterministically calculate a given "score" for a Systemnode depending on how close it's hash is to
// the proof of work for that block. The further away they are the better, the furthest will win the election
// and get paid this block
//
uint256 CSystemnode::CalculateScore(int mod, int64_t nBlockHeight)
{
    if(chainActive.Tip() == NULL) return uint256();

    uint256 hash = uint256();
    uint256 aux = ArithToUint256(UintToArith256(vin.prevout.hash) + vin.prevout.n);

    if(!GetBlockHash(hash, nBlockHeight)) {
        LogPrintf("CalculateScore ERROR - nHeight %d - Returned 0\n", nBlockHeight);
        return uint256();
    }

    CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
    ss << hash;
    arith_uint256 hash2 = UintToArith256(ss.GetHash());

    CHashWriter ss2(SER_GETHASH, PROTOCOL_VERSION);
    ss2 << hash;
    ss2 << aux;
    arith_uint256 hash3 = UintToArith256(ss2.GetHash());

    arith_uint256 r = (hash3 > hash2 ? hash3 - hash2 : hash2 - hash3);

    return ArithToUint256(r);
}

//
// When a new systemnode broadcast is sent, update our information
//
bool CSystemnode::UpdateFromNewBroadcast(CSystemnodeBroadcast& snb)
{
    if(snb.sigTime > sigTime) {    
        pubkey2 = snb.pubkey2;
        sigTime = snb.sigTime;
        sig = snb.sig;
        protocolVersion = snb.protocolVersion;
        addr = snb.addr;
        lastTimeChecked = 0;
        int nDoS = 0;
        if(snb.lastPing == CSystemnodePing() || (snb.lastPing != CSystemnodePing() && snb.lastPing.CheckAndUpdate(nDoS, false))) {
            lastPing = snb.lastPing;
            snodeman.mapSeenSystemnodePing.insert(make_pair(lastPing.GetHash(), lastPing));
        }
        return true;
    }
    return false;
}

void CSystemnode::Check(bool forceCheck)
{
    if(ShutdownRequested()) return;

    if(!forceCheck && (GetTime() - lastTimeChecked < SYSTEMNODE_CHECK_SECONDS)) return;
    lastTimeChecked = GetTime();


    //once spent, stop doing the checks
    if(activeState == SYSTEMNODE_VIN_SPENT) return;


    if(!IsPingedWithin(SYSTEMNODE_REMOVAL_SECONDS)){
        activeState = SYSTEMNODE_REMOVE;
        return;
    }

    if(!IsPingedWithin(SYSTEMNODE_EXPIRATION_SECONDS)){
        activeState = SYSTEMNODE_EXPIRED;
        return;
    }

    if(!unitTest){
        CValidationState state;
        CMutableTransaction tx = CMutableTransaction();
        CTxOut vout = CTxOut(499.99*COIN, legacySigner.collateralPubKey);
        tx.vin.push_back(vin);
        tx.vout.push_back(vout);

        {
            TRY_LOCK(cs_main, lockMain);
            if(!lockMain) return;

            if(!AcceptableInputs(mempool, state, CTransaction(tx), false, NULL)){
                activeState = SYSTEMNODE_VIN_SPENT;
                return;
            }
        }
    }
    activeState = SYSTEMNODE_ENABLED; // OK
}

//
// CSystemnodeBroadcast
//

bool CSystemnodeBroadcast::CheckAndUpdate(int& nDos)
{
    nDos = 0;

    // make sure signature isn't in the future (past is OK)
    if (sigTime > GetAdjustedTime() + 60 * 60) {
        LogPrintf("snb - Signature rejected, too far into the future %s\n", vin.ToString());
        nDos = 1;
        return false;
    }

    if(protocolVersion < GetMinSystemnodePaymentsProto()) {
        LogPrintf("snb - ignoring outdated systemnode %s protocol version %d\n", vin.ToString(), protocolVersion);
        return false;
    }

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
    if(lastPing == CSystemnodePing() || !lastPing.CheckAndUpdate(nDos, false, true))
        return false;

    std::string strMessage;
    std::string errorMessage = "";

    if(protocolVersion <= 99999999) {
        std::string vchPubKey(pubkey.begin(), pubkey.end());
        std::string vchPubKey2(pubkey2.begin(), pubkey2.end());
        strMessage = addr.ToString(false) + boost::lexical_cast<std::string>(sigTime) +
                        vchPubKey + vchPubKey2 + boost::lexical_cast<std::string>(protocolVersion);

        LogPrint("systemnode", "snb - sanitized strMessage: %s, pubkey address: %s, sig: %s\n",
            SanitizeString(strMessage), CBitcoinAddress(pubkey.GetID()).ToString(),
            EncodeBase64(&sig[0], sig.size()));

        if(!legacySigner.VerifyMessage(pubkey, sig, strMessage, errorMessage)){
            if (addr.ToString() != addr.ToString(false))
            {
                // maybe it's wrong format, try again with the old one
                strMessage = addr.ToString() + boost::lexical_cast<std::string>(sigTime) +
                                vchPubKey + vchPubKey2 + boost::lexical_cast<std::string>(protocolVersion);

                LogPrint("systemnode", "snb - sanitized strMessage: %s, pubkey address: %s, sig: %s\n",
                    SanitizeString(strMessage), CBitcoinAddress(pubkey.GetID()).ToString(),
                    EncodeBase64(&sig[0], sig.size()));

                if(!legacySigner.VerifyMessage(pubkey, sig, strMessage, errorMessage)){
                    // didn't work either
                    LogPrintf("snb - Got bad systemnode address signature, sanitized error: %s\n", SanitizeString(errorMessage));
                    // there is a bug in old MN signatures, ignore such MN but do not ban the peer we got this from
                    return false;
                }
            } else {
                // nope, sig is actually wrong
                LogPrintf("snb - Got bad systemnode address signature, sanitized error: %s\n", SanitizeString(errorMessage));
                // there is a bug in old MN signatures, ignore such MN but do not ban the peer we got this from
                return false;
            }
        }
    } else {
        strMessage = addr.ToString(false) + boost::lexical_cast<std::string>(sigTime) +
                        pubkey.GetID().ToString() + pubkey2.GetID().ToString() +
                        boost::lexical_cast<std::string>(protocolVersion);

        LogPrint("systemnode", "snb - strMessage: %s, pubkey address: %s, sig: %s\n",
            strMessage, CBitcoinAddress(pubkey.GetID()).ToString(), EncodeBase64(&sig[0], sig.size()));

        if(!legacySigner.VerifyMessage(pubkey, sig, strMessage, errorMessage)){
            LogPrintf("snb - Got bad systemnode address signature, error: %s\n", errorMessage);
            nDos = 100;
            return false;
        }
    }

    if(Params().NetworkID() == CBaseChainParams::MAIN) {
        if(addr.GetPort() != 9340) return false;
    } else if(addr.GetPort() == 9340) return false;

    //search existing systemnode list, this is where we update existing Systemnodes with new snb broadcasts
    CSystemnode* pmn = snodeman.Find(vin);

    // no such systemnode, nothing to update
    if(pmn == NULL) return true;

    // this broadcast is older or equal than the one that we already have - it's bad and should never happen
    // unless someone is doing something fishy
    // (mapSeensystemnodeBroadcast in CSystemnodeMan::ProcessMessage should filter legit duplicates)
    if(pmn->sigTime >= sigTime) {
        LogPrintf("CsystemnodeBroadcast::CheckAndUpdate - Bad sigTime %d for Systemnode %20s %105s (existing broadcast is at %d)\n",
                      sigTime, addr.ToString(), vin.ToString(), pmn->sigTime);
        return false;
    }

    // systemnode is not enabled yet/already, nothing to update
    if(!pmn->IsEnabled()) return true;

    // mn.pubkey = pubkey, IsVinAssociatedWithPubkey is validated once below,
    //   after that they just need to match
    if(pmn->pubkey == pubkey && !pmn->IsBroadcastedWithin(SYSTEMNODE_MIN_SNB_SECONDS)) {
        //take the newest entry
        LogPrintf("snb - Got updated entry for %s\n", addr.ToString());
        if(pmn->UpdateFromNewBroadcast((*this))){
            pmn->Check();
            if(pmn->IsEnabled()) Relay();
        }
        systemnodeSync.AddedSystemnodeList(GetHash());
    }

    return true;

}

bool CSystemnodeBroadcast::CheckInputsAndAdd(int& nDoS)
{
    // we are a systemnode with the same vin (i.e. already activated) and this snb is ours (matches our systemnode privkey)
    // so nothing to do here for us
    if(fSystemNode && vin.prevout == activeSystemnode.vin.prevout && pubkey2 == activeSystemnode.pubKeySystemnode)
        return true;

    // incorrect ping or its sigTime
    if(lastPing == CSystemnodePing() || !lastPing.CheckAndUpdate(nDoS, false, true))
        return false;

    // search existing systemnode list
    CSystemnode* pmn = snodeman.Find(vin);

    if(pmn != NULL) {
        // nothing to do here if we already know about this systemnode and it's enabled
        if(pmn->IsEnabled()) return true;
        // if it's not enabled, remove old MN first and continue
        else snodeman.Remove(pmn->vin);
    }

    CValidationState state;
    CMutableTransaction tx = CMutableTransaction();
    CTxOut vout = CTxOut(499.99*COIN, legacySigner.collateralPubKey);
    tx.vin.push_back(vin);
    tx.vout.push_back(vout);

    {
        TRY_LOCK(cs_main, lockMain);
        if(!lockMain) {
            // not snb fault, let it to be checked again later
            snodeman.mapSeenSystemnodeBroadcast.erase(GetHash());
            systemnodeSync.mapSeenSyncMNB.erase(GetHash());
            return false;
        }

        if(!AcceptableInputs(mempool, state, CTransaction(tx), false, NULL)) {
            //set nDos
            state.IsInvalid(nDoS);
            return false;
        }
    }

    LogPrint("systemnode", "snb - Accepted systemnode entry\n");

    if(GetInputAge(vin) < SYSTEMNODE_MIN_CONFIRMATIONS){
        LogPrintf("snb - Input must have at least %d confirmations\n", SYSTEMNODE_MIN_CONFIRMATIONS);
        // maybe we miss few blocks, let this snb to be checked again later
        snodeman.mapSeenSystemnodeBroadcast.erase(GetHash());
        systemnodeSync.mapSeenSyncMNB.erase(GetHash());
        return false;
    }

    // verify that sig time is legit in past
    // should be at least not earlier than block when 10000 CRW tx got SYSTEMNODE_MIN_CONFIRMATIONS
    uint256 hashBlock = uint256();
    CTransaction tx2;
    GetTransaction(vin.prevout.hash, tx2, hashBlock, true);
    BlockMap::iterator mi = mapBlockIndex.find(hashBlock);
    if (mi != mapBlockIndex.end() && (*mi).second)
    {
        CBlockIndex* pMNIndex = (*mi).second; // block for 10000 CRW tx -> 1 confirmation
        CBlockIndex* pConfIndex = chainActive[pMNIndex->nHeight + SYSTEMNODE_MIN_CONFIRMATIONS - 1]; // block where tx got SYSTEMNODE_MIN_CONFIRMATIONS
        if(pConfIndex->GetBlockTime() > sigTime)
        {
            LogPrintf("snb - Bad sigTime %d for systemnode %20s %105s (%i conf block is at %d)\n",
                      sigTime, addr.ToString(), vin.ToString(), SYSTEMNODE_MIN_CONFIRMATIONS, pConfIndex->GetBlockTime());
            return false;
        }
    }

    LogPrintf("snb - Got NEW systemnode entry - %s - %s - %s - %lli \n", GetHash().ToString(), addr.ToString(), vin.ToString(), sigTime);
    CSystemnode mn(*this);
    snodeman.Add(mn);

    // if it matches our systemnode privkey, then we've been remotely activated
    if(pubkey2 == activeSystemnode.pubKeySystemnode && protocolVersion == PROTOCOL_VERSION){
        activeSystemnode.EnableHotColdSystemNode(vin, addr);
    }

    bool isLocal = addr.IsRFC1918() || addr.IsLocal();
    if(Params().NetworkID() == CBaseChainParams::REGTEST) isLocal = false;

    if(!isLocal) Relay();

    return true;

}

void CSystemnodeBroadcast::Relay()
{
    CInv inv(MSG_SYSTEMNODE_ANNOUNCE, GetHash());
    RelayInv(inv);
}

CSystemnodeBroadcast::CSystemnodeBroadcast()
{
    vin = CTxIn();
    addr = CService();
    pubkey = CPubKey();
    pubkey2 = CPubKey();
    sig = std::vector<unsigned char>();
    activeState = SYSTEMNODE_ENABLED;
    sigTime = GetAdjustedTime();
    lastPing = CSystemnodePing();
    unitTest = false;
    protocolVersion = PROTOCOL_VERSION;
}

CSystemnodeBroadcast::CSystemnodeBroadcast(CService newAddr, CTxIn newVin, CPubKey newPubkey, CPubKey newPubkey2, int protocolVersionIn)
{
    vin = newVin;
    addr = newAddr;
    pubkey = newPubkey;
    pubkey2 = newPubkey2;
    sig = std::vector<unsigned char>();
    activeState = SYSTEMNODE_ENABLED;
    sigTime = GetAdjustedTime();
    lastPing = CSystemnodePing();
    unitTest = false;
    protocolVersion = protocolVersionIn;
}

CSystemnodeBroadcast::CSystemnodeBroadcast(const CSystemnode& mn)
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

bool CSystemnodeBroadcast::Create(std::string strService, std::string strKeySystemnode, std::string strTxHash, std::string strOutputIndex, std::string& strErrorMessage, CSystemnodeBroadcast &snb, bool fOffline) {
    CTxIn txin;
    CPubKey pubKeyCollateralAddress;
    CKey keyCollateralAddress;
    CPubKey pubKeySystemnodeNew;
    CKey keySystemnodeNew;

    //need correct blocks to send ping
    if(!fOffline && !systemnodeSync.IsBlockchainSynced()) {
        strErrorMessage = "Sync in progress. Must wait until sync is complete to start Systemnode";
        LogPrintf("CSystemnodeBroadcast::Create -- %s\n", strErrorMessage);
        return false;
    }

    if(!legacySigner.SetKey(strKeySystemnode, strErrorMessage, keySystemnodeNew, pubKeySystemnodeNew))
    {
        strErrorMessage = strprintf("Can't find keys for systemnode %s - %s", strService, strErrorMessage);
        LogPrintf("CSystemnodeBroadcast::Create -- %s\n", strErrorMessage);
        return false;
    }

    if(!pwalletMain->GetSystemnodeVinAndKeys(txin, pubKeyCollateralAddress, keyCollateralAddress, strTxHash, strOutputIndex)) {
        strErrorMessage = strprintf("Could not allocate txin %s:%s for systemnode %s", strTxHash, strOutputIndex, strService);
        LogPrintf("CSystemnodeBroadcast::Create -- %s\n", strErrorMessage);
        return false;
    }

    CService service = CService(strService);
    if(Params().NetworkID() == CBaseChainParams::MAIN) {
        if(service.GetPort() != 9340) {
            strErrorMessage = strprintf("Invalid port %u for systemnode %s - only 9340 is supported on mainnet.", service.GetPort(), strService);
            LogPrintf("CSystemnodeBroadcast::Create -- %s\n", strErrorMessage);
            return false;
        }
    } else if(service.GetPort() == 9340) {
        strErrorMessage = strprintf("Invalid port %u for systemnode %s - 9340 is only supported on mainnet.", service.GetPort(), strService);
        LogPrintf("CSystemnodeBroadcast::Create -- %s\n", strErrorMessage);
        return false;
    }

    return Create(txin, CService(strService), keyCollateralAddress, pubKeyCollateralAddress, keySystemnodeNew, pubKeySystemnodeNew, strErrorMessage, snb);
}

bool CSystemnodeBroadcast::Create(CTxIn txin, CService service, CKey keyCollateralAddress, CPubKey pubKeyCollateralAddress, CKey keySystemnodeNew, CPubKey pubKeySystemnodeNew, std::string &strErrorMessage, CSystemnodeBroadcast &snb) {
    // wait for reindex and/or import to finish
    if (fImporting || fReindex) return false;

    CSystemnodePing snp(txin);
    if(!snp.Sign(keySystemnodeNew, pubKeySystemnodeNew)){
        strErrorMessage = strprintf("Failed to sign ping, txin: %s", txin.ToString());
        LogPrintf("CSystemnodeBroadcast::Create --  %s\n", strErrorMessage);
        snb = CSystemnodeBroadcast();
        return false;
    }

    snb = CSystemnodeBroadcast(service, txin, pubKeyCollateralAddress, pubKeySystemnodeNew, PROTOCOL_VERSION);

    if(!snb.IsValidNetAddr()) {
        strErrorMessage = strprintf("Invalid IP address, systemnode=%s", txin.prevout.ToStringShort());
        LogPrintf("CSystemnodeBroadcast::Create -- %s\n", strErrorMessage);
        snb = CSystemnodeBroadcast();
        return false;
    }

    snb.lastPing = snp;
    if(!snb.Sign(keyCollateralAddress)){
        strErrorMessage = strprintf("Failed to sign broadcast, txin: %s", txin.ToString());
        LogPrintf("CSystemnodeBroadcast::Create -- %s\n", strErrorMessage);
        snb = CSystemnodeBroadcast();
        return false;
    }

    return true;
}

bool CSystemnodeBroadcast::Sign(CKey& keyCollateralAddress)
{
    std::string errorMessage;

    std::string vchPubKey(pubkey.begin(), pubkey.end());
    std::string vchPubKey2(pubkey2.begin(), pubkey2.end());

    sigTime = GetAdjustedTime();

    std::string strMessage = addr.ToString(false) + boost::lexical_cast<std::string>(sigTime) + vchPubKey + vchPubKey2 + boost::lexical_cast<std::string>(protocolVersion);

    if(!legacySigner.SignMessage(strMessage, errorMessage, sig, keyCollateralAddress)) {
        LogPrintf("CSystemnodeBroadcast::Sign() - Error: %s\n", errorMessage);
        return false;
    }

    return true;
}
