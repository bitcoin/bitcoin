// Copyright (c) 2014-2015 The Crown developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "throne.h"
#include "throneman.h"
#include "darksend.h"
#include "util.h"
#include "sync.h"
#include "addrman.h"
#include <boost/lexical_cast.hpp>

// keep track of the scanning errors I've seen
map<uint256, int> mapSeenThroneScanningErrors;
// cache block hashes as we calculate them
std::map<int64_t, uint256> mapCacheBlockHashes;

//Get the last hash that matches the modulus given. Processed in reverse order
bool GetBlockHash(uint256& hash, int nBlockHeight)
{
    if (chainActive.Tip() == NULL) return false;

    if(nBlockHeight == 0)
        nBlockHeight = chainActive.Tip()->nHeight;

    if(mapCacheBlockHashes.count(nBlockHeight)){
        hash = mapCacheBlockHashes[nBlockHeight];
        return true;
    }

    const CBlockIndex *BlockLastSolved = chainActive.Tip();
    const CBlockIndex *BlockReading = chainActive.Tip();

    if (BlockLastSolved == NULL || BlockLastSolved->nHeight == 0 || chainActive.Tip()->nHeight+1 < nBlockHeight) return false;

    int nBlocksAgo = 0;
    if(nBlockHeight > 0) nBlocksAgo = (chainActive.Tip()->nHeight+1)-nBlockHeight;
    assert(nBlocksAgo >= 0);

    int n = 0;
    for (unsigned int i = 1; BlockReading && BlockReading->nHeight > 0; i++) {
        if(n >= nBlocksAgo){
            hash = BlockReading->GetBlockHash();
            mapCacheBlockHashes[nBlockHeight] = hash;
            return true;
        }
        n++;

        if (BlockReading->pprev == NULL) { assert(BlockReading); break; }
        BlockReading = BlockReading->pprev;
    }

    return false;
}

CThrone::CThrone()
{
    LOCK(cs);
    vin = CTxIn();
    addr = CService();
    pubkey = CPubKey();
    pubkey2 = CPubKey();
    sig = std::vector<unsigned char>();
    activeState = THRONE_ENABLED;
    sigTime = GetAdjustedTime();
    lastPing = CThronePing();
    cacheInputAge = 0;
    cacheInputAgeBlock = 0;
    unitTest = false;
    allowFreeTx = true;
    protocolVersion = PROTOCOL_VERSION;
    nLastDsq = 0;
    nScanningErrorCount = 0;
    nLastScanningErrorBlockHeight = 0;
    lastTimeChecked = 0;
}

CThrone::CThrone(const CThrone& other)
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
    cacheInputAge = other.cacheInputAge;
    cacheInputAgeBlock = other.cacheInputAgeBlock;
    unitTest = other.unitTest;
    allowFreeTx = other.allowFreeTx;
    protocolVersion = other.protocolVersion;
    nLastDsq = other.nLastDsq;
    nScanningErrorCount = other.nScanningErrorCount;
    nLastScanningErrorBlockHeight = other.nLastScanningErrorBlockHeight;
    lastTimeChecked = 0;
}

CThrone::CThrone(const CThroneBroadcast& mnb)
{
    LOCK(cs);
    vin = mnb.vin;
    addr = mnb.addr;
    pubkey = mnb.pubkey;
    pubkey2 = mnb.pubkey2;
    sig = mnb.sig;
    activeState = THRONE_ENABLED;
    sigTime = mnb.sigTime;
    lastPing = mnb.lastPing;
    cacheInputAge = 0;
    cacheInputAgeBlock = 0;
    unitTest = false;
    allowFreeTx = true;
    protocolVersion = mnb.protocolVersion;
    nLastDsq = mnb.nLastDsq;
    nScanningErrorCount = 0;
    nLastScanningErrorBlockHeight = 0;
    lastTimeChecked = 0;
}

//
// When a new throne broadcast is sent, update our information
//
bool CThrone::UpdateFromNewBroadcast(CThroneBroadcast& mnb)
{
    if(mnb.sigTime > sigTime) {    
        pubkey2 = mnb.pubkey2;
        sigTime = mnb.sigTime;
        sig = mnb.sig;
        protocolVersion = mnb.protocolVersion;
        addr = mnb.addr;
        lastTimeChecked = 0;
        int nDoS = 0;
        if(mnb.lastPing == CThronePing() || (mnb.lastPing != CThronePing() && mnb.lastPing.CheckAndUpdate(nDoS, false))) {
            lastPing = mnb.lastPing;
            mnodeman.mapSeenThronePing.insert(make_pair(lastPing.GetHash(), lastPing));
        }
        return true;
    }
    return false;
}

//
// Deterministically calculate a given "score" for a Throne depending on how close it's hash is to
// the proof of work for that block. The further away they are the better, the furthest will win the election
// and get paid this block
//
uint256 CThrone::CalculateScore(int mod, int64_t nBlockHeight)
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

void CThrone::Check(bool forceCheck)
{
    if(ShutdownRequested()) return;

    if(!forceCheck && (GetTime() - lastTimeChecked < THRONE_CHECK_SECONDS)) return;
    lastTimeChecked = GetTime();


    //once spent, stop doing the checks
    if(activeState == THRONE_VIN_SPENT) return;


    if(!IsPingedWithin(THRONE_REMOVAL_SECONDS)){
        activeState = THRONE_REMOVE;
        return;
    }

    if(!IsPingedWithin(THRONE_EXPIRATION_SECONDS)){
        activeState = THRONE_EXPIRED;
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
                activeState = THRONE_VIN_SPENT;
                return;

            }
        }
    }

    activeState = THRONE_ENABLED; // OK
}

bool CThrone::IsValidNetAddr()
{
    // TODO: regtest is fine with any addresses for now,
    // should probably be a bit smarter if one day we start to implement tests for this
    return (addr.IsIPv4() && addr.IsRoutable());
}

int64_t CThrone::SecondsSincePayment() {
    CScript pubkeyScript;
    pubkeyScript = GetScriptForDestination(pubkey.GetID());

    int64_t sec = (GetAdjustedTime() - GetLastPaid());
    int64_t month = 60*60*24*30;
    if(sec < month) return sec; //if it's less than 30 days, give seconds

    CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
    ss << vin;
    ss << sigTime;
    uint256 hash =  ss.GetHash();

    // return some deterministic value for unknown/unpaid but force it to be more than 30 days old
    return month + UintToArith256(hash).GetCompact(false);
}

int64_t CThrone::GetLastPaid() {
    CBlockIndex* pindexPrev = chainActive.Tip();
    if(pindexPrev == NULL) return false;

    CScript mnpayee;
    mnpayee = GetScriptForDestination(pubkey.GetID());

    CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
    ss << vin;
    ss << sigTime;
    uint256 hash =  ss.GetHash();

    // use a deterministic offset to break a tie -- 2.5 minutes
    int64_t nOffset = UintToArith256(hash).GetCompact(false) % 150; 

    if (chainActive.Tip() == NULL) return false;

    const CBlockIndex *BlockReading = chainActive.Tip();

    int nMnCount = mnodeman.CountEnabled()*1.25;
    int n = 0;
    for (unsigned int i = 1; BlockReading && BlockReading->nHeight > 0; i++) {
        if(n >= nMnCount){
            return 0;
        }
        n++;

        if(thronePayments.mapThroneBlocks.count(BlockReading->nHeight)){
            /*
                Search for this payee, with at least 2 votes. This will aid in consensus allowing the network 
                to converge on the same payees quickly, then keep the same schedule.
            */
            if(thronePayments.mapThroneBlocks[BlockReading->nHeight].HasPayeeWithVotes(mnpayee, 2)){
                return BlockReading->nTime + nOffset;
            }
        }

        if (BlockReading->pprev == NULL) { assert(BlockReading); break; }
        BlockReading = BlockReading->pprev;
    }

    return 0;
}

CThroneBroadcast::CThroneBroadcast()
{
    vin = CTxIn();
    addr = CService();
    pubkey = CPubKey();
    pubkey2 = CPubKey();
    sig = std::vector<unsigned char>();
    activeState = THRONE_ENABLED;
    sigTime = GetAdjustedTime();
    lastPing = CThronePing();
    cacheInputAge = 0;
    cacheInputAgeBlock = 0;
    unitTest = false;
    allowFreeTx = true;
    protocolVersion = PROTOCOL_VERSION;
    nLastDsq = 0;
    nScanningErrorCount = 0;
    nLastScanningErrorBlockHeight = 0;
}

CThroneBroadcast::CThroneBroadcast(CService newAddr, CTxIn newVin, CPubKey newPubkey, CPubKey newPubkey2, int protocolVersionIn)
{
    vin = newVin;
    addr = newAddr;
    pubkey = newPubkey;
    pubkey2 = newPubkey2;
    sig = std::vector<unsigned char>();
    activeState = THRONE_ENABLED;
    sigTime = GetAdjustedTime();
    lastPing = CThronePing();
    cacheInputAge = 0;
    cacheInputAgeBlock = 0;
    unitTest = false;
    allowFreeTx = true;
    protocolVersion = protocolVersionIn;
    nLastDsq = 0;
    nScanningErrorCount = 0;
    nLastScanningErrorBlockHeight = 0;
}

CThroneBroadcast::CThroneBroadcast(const CThrone& mn)
{
    vin = mn.vin;
    addr = mn.addr;
    pubkey = mn.pubkey;
    pubkey2 = mn.pubkey2;
    sig = mn.sig;
    activeState = mn.activeState;
    sigTime = mn.sigTime;
    lastPing = mn.lastPing;
    cacheInputAge = mn.cacheInputAge;
    cacheInputAgeBlock = mn.cacheInputAgeBlock;
    unitTest = mn.unitTest;
    allowFreeTx = mn.allowFreeTx;
    protocolVersion = mn.protocolVersion;
    nLastDsq = mn.nLastDsq;
    nScanningErrorCount = mn.nScanningErrorCount;
    nLastScanningErrorBlockHeight = mn.nLastScanningErrorBlockHeight;
}

bool CThroneBroadcast::Create(std::string strService, std::string strKeyThrone, std::string strTxHash, std::string strOutputIndex, std::string& strErrorMessage, CThroneBroadcast &mnb, bool fOffline) {
    CTxIn txin;
    CPubKey pubKeyCollateralAddress;
    CKey keyCollateralAddress;
    CPubKey pubKeyThroneNew;
    CKey keyThroneNew;

    //need correct blocks to send ping
    if(!fOffline && !throneSync.IsBlockchainSynced()) {
        strErrorMessage = "Sync in progress. Must wait until sync is complete to start Throne";
        LogPrintf("CThroneBroadcast::Create -- %s\n", strErrorMessage);
        return false;
    }

    if(!darkSendSigner.SetKey(strKeyThrone, strErrorMessage, keyThroneNew, pubKeyThroneNew))
    {
        strErrorMessage = strprintf("Can't find keys for throne %s - %s", strService, strErrorMessage);
        LogPrintf("CThroneBroadcast::Create -- %s\n", strErrorMessage);
        return false;
    }

    if(!pwalletMain->GetThroneVinAndKeys(txin, pubKeyCollateralAddress, keyCollateralAddress, strTxHash, strOutputIndex)) {
        strErrorMessage = strprintf("Could not allocate txin %s:%s for throne %s", strTxHash, strOutputIndex, strService);
        LogPrintf("CThroneBroadcast::Create -- %s\n", strErrorMessage);
        return false;
    }

    CService service = CService(strService);
    if(Params().NetworkID() == CBaseChainParams::MAIN) {
        if(service.GetPort() != 9340) {
            strErrorMessage = strprintf("Invalid port %u for throne %s - only 9340 is supported on mainnet.", service.GetPort(), strService);
            LogPrintf("CThroneBroadcast::Create -- %s\n", strErrorMessage);
            return false;
        }
    } else if(service.GetPort() == 9340) {
        strErrorMessage = strprintf("Invalid port %u for throne %s - 9340 is only supported on mainnet.", service.GetPort(), strService);
        LogPrintf("CThroneBroadcast::Create -- %s\n", strErrorMessage);
        return false;
    }

    return Create(txin, CService(strService), keyCollateralAddress, pubKeyCollateralAddress, keyThroneNew, pubKeyThroneNew, strErrorMessage, mnb);
}

bool CThroneBroadcast::Create(CTxIn txin, CService service, CKey keyCollateralAddress, CPubKey pubKeyCollateralAddress, CKey keyThroneNew, CPubKey pubKeyThroneNew, std::string &strErrorMessage, CThroneBroadcast &mnb) {
    // wait for reindex and/or import to finish
    if (fImporting || fReindex) return false;

    CThronePing mnp(txin);
    if(!mnp.Sign(keyThroneNew, pubKeyThroneNew)){
        strErrorMessage = strprintf("Failed to sign ping, txin: %s", txin.ToString());
        LogPrintf("CThroneBroadcast::Create --  %s\n", strErrorMessage);
        mnb = CThroneBroadcast();
        return false;
    }

    mnb = CThroneBroadcast(service, txin, pubKeyCollateralAddress, pubKeyThroneNew, PROTOCOL_VERSION);

    if(!mnb.IsValidNetAddr()) {
        strErrorMessage = strprintf("Invalid IP address, throne=%s", txin.prevout.ToStringShort());
        LogPrintf("CThroneBroadcast::Create -- %s\n", strErrorMessage);
        mnb = CThroneBroadcast();
        return false;
    }

    mnb.lastPing = mnp;
    if(!mnb.Sign(keyCollateralAddress)){
        strErrorMessage = strprintf("Failed to sign broadcast, txin: %s", txin.ToString());
        LogPrintf("CThroneBroadcast::Create -- %s\n", strErrorMessage);
        mnb = CThroneBroadcast();
        return false;
    }

    return true;
}

bool CThroneBroadcast::CheckAndUpdate(int& nDos)
{
    nDos = 0;

    // make sure signature isn't in the future (past is OK)
    if (sigTime > GetAdjustedTime() + 60 * 60) {
        LogPrintf("mnb - Signature rejected, too far into the future %s\n", vin.ToString());
        nDos = 1;
        return false;
    }

    if(protocolVersion < thronePayments.GetMinThronePaymentsProto()) {
        LogPrintf("mnb - ignoring outdated Throne %s protocol version %d\n", vin.ToString(), protocolVersion);
        return false;
    }

    CScript pubkeyScript;
    pubkeyScript = GetScriptForDestination(pubkey.GetID());

    if(pubkeyScript.size() != 25) {
        LogPrintf("mnb - pubkey the wrong size\n");
        nDos = 100;
        return false;
    }

    CScript pubkeyScript2;
    pubkeyScript2 = GetScriptForDestination(pubkey2.GetID());

    if(pubkeyScript2.size() != 25) {
        LogPrintf("mnb - pubkey2 the wrong size\n");
        nDos = 100;
        return false;
    }

    if(!vin.scriptSig.empty()) {
        LogPrintf("mnb - Ignore Not Empty ScriptSig %s\n",vin.ToString());
        return false;
    }

    // incorrect ping or its sigTime
    if(lastPing == CThronePing() || !lastPing.CheckAndUpdate(nDos, false, true))
        return false;

    std::string strMessage;
    std::string errorMessage = "";

    if(protocolVersion <= 99999999) {
        std::string vchPubKey(pubkey.begin(), pubkey.end());
        std::string vchPubKey2(pubkey2.begin(), pubkey2.end());
        strMessage = addr.ToString(false) + boost::lexical_cast<std::string>(sigTime) +
                        vchPubKey + vchPubKey2 + boost::lexical_cast<std::string>(protocolVersion);

        LogPrint("throne", "mnb - sanitized strMessage: %s, pubkey address: %s, sig: %s\n",
            SanitizeString(strMessage), CBitcoinAddress(pubkey.GetID()).ToString(),
            EncodeBase64(&sig[0], sig.size()));

        if(!darkSendSigner.VerifyMessage(pubkey, sig, strMessage, errorMessage)){
            if (addr.ToString() != addr.ToString(false))
            {
                // maybe it's wrong format, try again with the old one
                strMessage = addr.ToString() + boost::lexical_cast<std::string>(sigTime) +
                                vchPubKey + vchPubKey2 + boost::lexical_cast<std::string>(protocolVersion);

                LogPrint("throne", "mnb - sanitized strMessage: %s, pubkey address: %s, sig: %s\n",
                    SanitizeString(strMessage), CBitcoinAddress(pubkey.GetID()).ToString(),
                    EncodeBase64(&sig[0], sig.size()));

                if(!darkSendSigner.VerifyMessage(pubkey, sig, strMessage, errorMessage)){
                    // didn't work either
                    LogPrintf("mnb - Got bad Throne address signature, sanitized error: %s\n", SanitizeString(errorMessage));
                    // there is a bug in old MN signatures, ignore such MN but do not ban the peer we got this from
                    return false;
                }
            } else {
                // nope, sig is actually wrong
                LogPrintf("mnb - Got bad Throne address signature, sanitized error: %s\n", SanitizeString(errorMessage));
                // there is a bug in old MN signatures, ignore such MN but do not ban the peer we got this from
                return false;
            }
        }
    } else {
        strMessage = addr.ToString(false) + boost::lexical_cast<std::string>(sigTime) +
                        pubkey.GetID().ToString() + pubkey2.GetID().ToString() +
                        boost::lexical_cast<std::string>(protocolVersion);

        LogPrint("throne", "mnb - strMessage: %s, pubkey address: %s, sig: %s\n",
            strMessage, CBitcoinAddress(pubkey.GetID()).ToString(), EncodeBase64(&sig[0], sig.size()));

        if(!darkSendSigner.VerifyMessage(pubkey, sig, strMessage, errorMessage)){
            LogPrintf("mnb - Got bad Throne address signature, error: %s\n", errorMessage);
            nDos = 100;
            return false;
        }
    }

    if(Params().NetworkID() == CBaseChainParams::MAIN) {
        if(addr.GetPort() != 9340) return false;
    } else if(addr.GetPort() == 9340) return false;

    //search existing Throne list, this is where we update existing Thrones with new mnb broadcasts
    CThrone* pmn = mnodeman.Find(vin);

    // no such throne, nothing to update
    if(pmn == NULL) return true;

    // this broadcast is older or equal than the one that we already have - it's bad and should never happen
    // unless someone is doing something fishy
    // (mapSeenThroneBroadcast in CThroneMan::ProcessMessage should filter legit duplicates)
    if(pmn->sigTime >= sigTime) {
        LogPrintf("CThroneBroadcast::CheckAndUpdate - Bad sigTime %d for Throne %20s %105s (existing broadcast is at %d)\n",
                      sigTime, addr.ToString(), vin.ToString(), pmn->sigTime);
        return false;
    }

    // throne is not enabled yet/already, nothing to update
    if(!pmn->IsEnabled()) return true;

    // mn.pubkey = pubkey, IsVinAssociatedWithPubkey is validated once below,
    //   after that they just need to match
    if(pmn->pubkey == pubkey && !pmn->IsBroadcastedWithin(THRONE_MIN_MNB_SECONDS)) {
        //take the newest entry
        LogPrintf("mnb - Got updated entry for %s\n", addr.ToString());
        if(pmn->UpdateFromNewBroadcast((*this))){
            pmn->Check();
            if(pmn->IsEnabled()) Relay();
        }
        throneSync.AddedThroneList(GetHash());
    }

    return true;
}

bool CThroneBroadcast::CheckInputsAndAdd(int& nDoS)
{
    // we are a throne with the same vin (i.e. already activated) and this mnb is ours (matches our Throne privkey)
    // so nothing to do here for us
    if(fThroNe && vin.prevout == activeThrone.vin.prevout && pubkey2 == activeThrone.pubKeyThrone)
        return true;

    // incorrect ping or its sigTime
    if(lastPing == CThronePing() || !lastPing.CheckAndUpdate(nDoS, false, true))
        return false;

    // search existing Throne list
    CThrone* pmn = mnodeman.Find(vin);

    if(pmn != NULL) {
        // nothing to do here if we already know about this throne and it's enabled
        if(pmn->IsEnabled()) return true;
        // if it's not enabled, remove old MN first and continue
        else mnodeman.Remove(pmn->vin);
    }

    CValidationState state;
    CMutableTransaction tx = CMutableTransaction();
    CTxOut vout = CTxOut(9999.99*COIN, darkSendPool.collateralPubKey);
    tx.vin.push_back(vin);
    tx.vout.push_back(vout);

    {
        TRY_LOCK(cs_main, lockMain);
        if(!lockMain) {
            // not mnb fault, let it to be checked again later
            mnodeman.mapSeenThroneBroadcast.erase(GetHash());
            throneSync.mapSeenSyncMNB.erase(GetHash());
            return false;
        }

        if(!AcceptableInputs(mempool, state, CTransaction(tx), false, NULL)) {
            //set nDos
            state.IsInvalid(nDoS);
            return false;
        }
    }

    LogPrint("throne", "mnb - Accepted Throne entry\n");

    if(GetInputAge(vin) < THRONE_MIN_CONFIRMATIONS){
        LogPrintf("mnb - Input must have at least %d confirmations\n", THRONE_MIN_CONFIRMATIONS);
        // maybe we miss few blocks, let this mnb to be checked again later
        mnodeman.mapSeenThroneBroadcast.erase(GetHash());
        throneSync.mapSeenSyncMNB.erase(GetHash());
        return false;
    }

    // verify that sig time is legit in past
    // should be at least not earlier than block when 10000 CRW tx got THRONE_MIN_CONFIRMATIONS
    uint256 hashBlock = uint256();
    CTransaction tx2;
    GetTransaction(vin.prevout.hash, tx2, hashBlock, true);
    BlockMap::iterator mi = mapBlockIndex.find(hashBlock);
    if (mi != mapBlockIndex.end() && (*mi).second)
    {
        CBlockIndex* pMNIndex = (*mi).second; // block for 10000 CRW tx -> 1 confirmation
        CBlockIndex* pConfIndex = chainActive[pMNIndex->nHeight + THRONE_MIN_CONFIRMATIONS - 1]; // block where tx got THRONE_MIN_CONFIRMATIONS
        if(pConfIndex->GetBlockTime() > sigTime)
        {
            LogPrintf("mnb - Bad sigTime %d for Throne %20s %105s (%i conf block is at %d)\n",
                      sigTime, addr.ToString(), vin.ToString(), THRONE_MIN_CONFIRMATIONS, pConfIndex->GetBlockTime());
            return false;
        }
    }

    LogPrintf("mnb - Got NEW Throne entry - %s - %s - %s - %lli \n", GetHash().ToString(), addr.ToString(), vin.ToString(), sigTime);
    CThrone mn(*this);
    mnodeman.Add(mn);

    // if it matches our Throne privkey, then we've been remotely activated
    if(pubkey2 == activeThrone.pubKeyThrone && protocolVersion == PROTOCOL_VERSION){
        activeThrone.EnableHotColdThroNe(vin, addr);
    }

    bool isLocal = addr.IsRFC1918() || addr.IsLocal();
    if(Params().NetworkID() == CBaseChainParams::REGTEST) isLocal = false;

    if(!isLocal) Relay();

    return true;
}

void CThroneBroadcast::Relay()
{
    CInv inv(MSG_THRONE_ANNOUNCE, GetHash());
    RelayInv(inv);
}

bool CThroneBroadcast::Sign(CKey& keyCollateralAddress)
{
    std::string errorMessage;

    std::string vchPubKey(pubkey.begin(), pubkey.end());
    std::string vchPubKey2(pubkey2.begin(), pubkey2.end());

    sigTime = GetAdjustedTime();

    std::string strMessage = addr.ToString(false) + boost::lexical_cast<std::string>(sigTime) + vchPubKey + vchPubKey2 + boost::lexical_cast<std::string>(protocolVersion);

    if(!darkSendSigner.SignMessage(strMessage, errorMessage, sig, keyCollateralAddress)) {
        LogPrintf("CThroneBroadcast::Sign() - Error: %s\n", errorMessage);
        return false;
    }

    return true;
}

bool CThroneBroadcast::VerifySignature()
{
    std::string errorMessage;

    std::string vchPubKey(pubkey.begin(), pubkey.end());
    std::string vchPubKey2(pubkey2.begin(), pubkey2.end());

    std::string strMessage = addr.ToString() + boost::lexical_cast<std::string>(sigTime) + vchPubKey + vchPubKey2 + boost::lexical_cast<std::string>(protocolVersion);

    if(!darkSendSigner.VerifyMessage(pubkey, sig, strMessage, errorMessage)) {
        LogPrintf("CThroneBroadcast::VerifySignature() - Error: %s\n", errorMessage);
        return false;
    }

    return true;
}

CThronePing::CThronePing()
{
    vin = CTxIn();
    blockHash = uint256();
    sigTime = 0;
    vchSig = std::vector<unsigned char>();
}

CThronePing::CThronePing(CTxIn& newVin)
{
    vin = newVin;
    blockHash = chainActive[chainActive.Height() - 12]->GetBlockHash();
    sigTime = GetAdjustedTime();
    vchSig = std::vector<unsigned char>();
}


bool CThronePing::Sign(CKey& keyThrone, CPubKey& pubKeyThrone)
{
    std::string errorMessage;
    std::string strThroNeSignMessage;

    sigTime = GetAdjustedTime();
    std::string strMessage = vin.ToString() + blockHash.ToString() + boost::lexical_cast<std::string>(sigTime);

    if(!darkSendSigner.SignMessage(strMessage, errorMessage, vchSig, keyThrone)) {
        LogPrintf("CThronePing::Sign() - Error: %s\n", errorMessage);
        return false;
    }

    if(!darkSendSigner.VerifyMessage(pubKeyThrone, vchSig, strMessage, errorMessage)) {
        LogPrintf("CThronePing::Sign() - Error: %s\n", errorMessage);
        return false;
    }

    return true;
}

bool CThronePing::VerifySignature(CPubKey& pubKeyThrone, int &nDos) {
    std::string strMessage = vin.ToString() + blockHash.ToString() + boost::lexical_cast<std::string>(sigTime);
    std::string errorMessage = "";

    if(!darkSendSigner.VerifyMessage(pubKeyThrone, vchSig, strMessage, errorMessage))
    {
        LogPrintf("CThronePing::VerifySignature - Got bad Throne ping signature %s Error: %s\n", vin.ToString(), errorMessage);
        nDos = 33;
        return false;
    }
    return true;
}

bool CThronePing::CheckAndUpdate(int& nDos, bool fRequireEnabled, bool fCheckSigTimeOnly)
{
    if (sigTime > GetAdjustedTime() + 60 * 60) {
        LogPrintf("CThronePing::CheckAndUpdate - Signature rejected, too far into the future %s\n", vin.ToString());
        nDos = 1;
        return false;
    }

    if (sigTime <= GetAdjustedTime() - 60 * 60) {
        LogPrintf("CThronePing::CheckAndUpdate - Signature rejected, too far into the past %s - %d %d \n", vin.ToString(), sigTime, GetAdjustedTime());
        nDos = 1;
        return false;
    }

    if(fCheckSigTimeOnly) {
        CThrone* pmn = mnodeman.Find(vin);
        if(pmn) return VerifySignature(pmn->pubkey2, nDos);
        return true;
    }

    LogPrint("throne", "CThronePing::CheckAndUpdate - New Ping - %s - %s - %lli\n", GetHash().ToString(), blockHash.ToString(), sigTime);

    // see if we have this Throne
    CThrone* pmn = mnodeman.Find(vin);
    if(pmn != NULL && pmn->protocolVersion >= thronePayments.GetMinThronePaymentsProto())
    {
        if (fRequireEnabled && !pmn->IsEnabled()) return false;

        // LogPrintf("mnping - Found corresponding mn for vin: %s\n", vin.ToString());
        // update only if there is no known ping for this throne or
        // last ping was more then THRONE_MIN_MNP_SECONDS-60 ago comparing to this one
        if(!pmn->IsPingedWithin(THRONE_MIN_MNP_SECONDS - 60, sigTime))
        {
            if(!VerifySignature(pmn->pubkey2, nDos))
                return false;

            BlockMap::iterator mi = mapBlockIndex.find(blockHash);
            if (mi != mapBlockIndex.end() && (*mi).second)
            {
                if((*mi).second->nHeight < chainActive.Height() - 24)
                {
                    LogPrintf("CThronePing::CheckAndUpdate - Throne %s block hash %s is too old\n", vin.ToString(), blockHash.ToString());
                    // Do nothing here (no Throne update, no mnping relay)
                    // Let this node to be visible but fail to accept mnping

                    return false;
                }
            } else {
                if (fDebug) LogPrintf("CThronePing::CheckAndUpdate - Throne %s block hash %s is unknown\n", vin.ToString(), blockHash.ToString());
                // maybe we stuck so we shouldn't ban this node, just fail to accept it
                // TODO: or should we also request this block?

                return false;
            }

            pmn->lastPing = *this;

            //mnodeman.mapSeenThroneBroadcast.lastPing is probably outdated, so we'll update it
            CThroneBroadcast mnb(*pmn);
            uint256 hash = mnb.GetHash();
            if(mnodeman.mapSeenThroneBroadcast.count(hash)) {
                mnodeman.mapSeenThroneBroadcast[hash].lastPing = *this;
            }

            pmn->Check(true);
            if(!pmn->IsEnabled()) return false;

            LogPrint("throne", "CThronePing::CheckAndUpdate - Throne ping accepted, vin: %s\n", vin.ToString());

            Relay();
            return true;
        }
        LogPrint("throne", "CThronePing::CheckAndUpdate - Throne ping arrived too early, vin: %s\n", vin.ToString());
        //nDos = 1; //disable, this is happening frequently and causing banned peers
        return false;
    }
    LogPrint("throne", "CThronePing::CheckAndUpdate - Couldn't find compatible Throne entry, vin: %s\n", vin.ToString());

    return false;
}

void CThronePing::Relay()
{
    CInv inv(MSG_THRONE_PING, GetHash());
    RelayInv(inv);
}
