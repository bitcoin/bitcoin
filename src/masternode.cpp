// Copyright (c) 2014-2015 The Dash developers
// Copyright (c) 2014-2018 The Crown developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "masternode.h"
#include "masternodeman.h"
#include "legacysigner.h"
#include "util.h"
#include "sync.h"
#include "addrman.h"
#include <boost/lexical_cast.hpp>

// keep track of the scanning errors I've seen
map<uint256, int> mapSeenMasternodeScanningErrors;
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

CMasternode::CMasternode()
{
    LOCK(cs);
    vin = CTxIn();
    addr = CService();
    pubkey = CPubKey();
    pubkey2 = CPubKey();
    sig = std::vector<unsigned char>();
    vchSignover = std::vector<unsigned char>();
    activeState = MASTERNODE_ENABLED;
    sigTime = GetAdjustedTime();
    lastPing = CMasternodePing();
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

CMasternode::CMasternode(const CMasternode& other)
{
    LOCK(cs);
    vin = other.vin;
    addr = other.addr;
    pubkey = other.pubkey;
    pubkey2 = other.pubkey2;
    sig = other.sig;
    vchSignover = other.vchSignover;
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

CMasternode::CMasternode(const CMasternodeBroadcast& mnb)
{
    LOCK(cs);
    vin = mnb.vin;
    addr = mnb.addr;
    pubkey = mnb.pubkey;
    pubkey2 = mnb.pubkey2;
    sig = mnb.sig;
    vchSignover = mnb.vchSignover;
    activeState = MASTERNODE_ENABLED;
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
// When a new masternode broadcast is sent, update our information
//
bool CMasternode::UpdateFromNewBroadcast(const CMasternodeBroadcast& mnb)
{
    if(mnb.sigTime > sigTime) {    
        pubkey2 = mnb.pubkey2;
        sigTime = mnb.sigTime;
        sig = mnb.sig;
        protocolVersion = mnb.protocolVersion;
        addr = mnb.addr;
        lastTimeChecked = 0;
        int nDoS = 0;
        if(mnb.lastPing == CMasternodePing() || (mnb.lastPing != CMasternodePing() && mnb.lastPing.CheckAndUpdate(nDoS, false))) {
            lastPing = mnb.lastPing;
            mnodeman.mapSeenMasternodePing.insert(make_pair(lastPing.GetHash(), lastPing));
        }
        return true;
    }
    return false;
}

//
// Deterministically calculate a given "score" for a Masternode depending on how close it's hash is to
// the proof of work for that block. The further away they are the better, the furthest will win the election
// and get paid this block
//
arith_uint256 CMasternode::CalculateScore(int64_t nBlockHeight) const
{
    if(chainActive.Tip() == NULL)
        return arith_uint256();

    // Find the block hash where tx got MASTERNODE_MIN_CONFIRMATIONS
    CBlockIndex *pblockIndex = chainActive[GetInputHeight(vin) + MASTERNODE_MIN_CONFIRMATIONS - 1];
    if (!pblockIndex)
        return arith_uint256();
    uint256 collateralMinConfBlockHash = pblockIndex->GetBlockHash();

    uint256 hash = uint256();

    if(!GetBlockHash(hash, nBlockHeight)) {
        LogPrintf("CalculateScore ERROR - nHeight %d - Returned 0\n", nBlockHeight);
        return arith_uint256();
    }

    CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
    ss << vin.prevout << collateralMinConfBlockHash << hash;
    return UintToArith256(ss.GetHash());
}

void CMasternode::Check(bool forceCheck)
{
    if(ShutdownRequested()) return;

    if(!forceCheck && (GetTime() - lastTimeChecked < MASTERNODE_CHECK_SECONDS)) return;
    lastTimeChecked = GetTime();


    //once spent, stop doing the checks
    if(activeState == MASTERNODE_VIN_SPENT) return;


    if(!IsPingedWithin(MASTERNODE_REMOVAL_SECONDS)){
        activeState = MASTERNODE_REMOVE;
        return;
    }

    if(!IsPingedWithin(MASTERNODE_EXPIRATION_SECONDS)){
        activeState = MASTERNODE_EXPIRED;
        return;
    }

    if(!unitTest){
        CValidationState state;
        CMutableTransaction tx = CMutableTransaction();
        CTxOut vout = CTxOut((MASTERNODE_COLLATERAL - 0.01)*COIN, legacySigner.collateralPubKey);
        tx.vin.push_back(vin);
        tx.vout.push_back(vout);

        {
            TRY_LOCK(cs_main, lockMain);
            if(!lockMain) return;

            if(!AcceptableInputs(mempool, state, CTransaction(tx), false, NULL)){
                activeState = MASTERNODE_VIN_SPENT;
                return;

            }
        }
    }

    activeState = MASTERNODE_ENABLED; // OK
}

bool CMasternode::IsValidNetAddr() const
{
    // TODO: regtest is fine with any addresses for now,
    // should probably be a bit smarter if one day we start to implement tests for this
    return (addr.IsIPv4() && addr.IsRoutable());
}

int64_t CMasternode::SecondsSincePayment() const
{
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

int64_t CMasternode::GetLastPaid() const
{
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

        if(masternodePayments.mapMasternodeBlocks.count(BlockReading->nHeight)){
            /*
                Search for this payee, with at least 2 votes. This will aid in consensus allowing the network 
                to converge on the same payees quickly, then keep the same schedule.
            */
            if(masternodePayments.mapMasternodeBlocks[BlockReading->nHeight].HasPayeeWithVotes(mnpayee, 2)){
                return BlockReading->nTime + nOffset;
            }
        }

        if (BlockReading->pprev == NULL) { assert(BlockReading); break; }
        BlockReading = BlockReading->pprev;
    }

    return 0;
}


// Find all blocks where MN received reward within defined block depth
// Used for generating stakepointers
bool CMasternode::GetRecentPaymentBlocks(std::vector<const CBlockIndex*>& vPaymentBlocks, bool limitMostRecent) const
{
    vPaymentBlocks.clear();

    int nMinimumValidBlockHeight = chainActive.Height() - Params().ValidStakePointerDuration() + 1;
    if (nMinimumValidBlockHeight < 1)
        nMinimumValidBlockHeight = 1;

    CBlockIndex* pindex = chainActive[nMinimumValidBlockHeight];

    CScript mnpayee;
    mnpayee = GetScriptForDestination(pubkey.GetID());

    bool fBlockFound = false;
    while (chainActive.Next(pindex)) {
        CBlock block;
        if (!ReadBlockFromDisk(block, pindex))
            continue;

        if (block.vtx[0].vout.size() > 1 && block.vtx[0].vout[1].scriptPubKey == mnpayee) {
            vPaymentBlocks.emplace_back(pindex);
            fBlockFound = true;
            if (limitMostRecent)
                return fBlockFound;
        }
        pindex = chainActive.Next(pindex);
    }

    return fBlockFound;
}

CMasternodeBroadcast::CMasternodeBroadcast()
{
    vin = CTxIn();
    addr = CService();
    pubkey = CPubKey();
    pubkey2 = CPubKey();
    sig = std::vector<unsigned char>();
    vchSignover = std::vector<unsigned char>();
    activeState = MASTERNODE_ENABLED;
    sigTime = GetAdjustedTime();
    lastPing = CMasternodePing();
    cacheInputAge = 0;
    cacheInputAgeBlock = 0;
    unitTest = false;
    allowFreeTx = true;
    protocolVersion = PROTOCOL_VERSION;
    nLastDsq = 0;
    nScanningErrorCount = 0;
    nLastScanningErrorBlockHeight = 0;
}

CMasternodeBroadcast::CMasternodeBroadcast(CService newAddr, CTxIn newVin, CPubKey newPubkey, CPubKey newPubkey2, int protocolVersionIn)
{
    vin = newVin;
    addr = newAddr;
    pubkey = newPubkey;
    pubkey2 = newPubkey2;
    sig = std::vector<unsigned char>();
    vchSignover = std::vector<unsigned char>();
    activeState = MASTERNODE_ENABLED;
    sigTime = GetAdjustedTime();
    lastPing = CMasternodePing();
    cacheInputAge = 0;
    cacheInputAgeBlock = 0;
    unitTest = false;
    allowFreeTx = true;
    protocolVersion = protocolVersionIn;
    nLastDsq = 0;
    nScanningErrorCount = 0;
    nLastScanningErrorBlockHeight = 0;
}

CMasternodeBroadcast::CMasternodeBroadcast(const CMasternode& mn)
{
    vin = mn.vin;
    addr = mn.addr;
    pubkey = mn.pubkey;
    pubkey2 = mn.pubkey2;
    sig = mn.sig;
    vchSignover = mn.vchSignover;
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

bool CMasternodeBroadcast::Create(std::string strService, std::string strKeyMasternode, std::string strTxHash, std::string strOutputIndex, std::string& strErrorMessage, CMasternodeBroadcast &mnb, bool fOffline) {
    CTxIn txin;
    CPubKey pubKeyCollateralAddress;
    CKey keyCollateralAddress;
    CPubKey pubKeyMasternodeNew;
    CKey keyMasternodeNew;

    //need correct blocks to send ping
    if(!fOffline && !masternodeSync.IsBlockchainSynced()) {
        strErrorMessage = "Sync in progress. Must wait until sync is complete to start Masternode";
        LogPrintf("CMasternodeBroadcast::Create -- %s\n", strErrorMessage);
        return false;
    }

    if(!legacySigner.SetKey(strKeyMasternode, strErrorMessage, keyMasternodeNew, pubKeyMasternodeNew))
    {
        strErrorMessage = strprintf("Can't find keys for masternode %s - %s", strService, strErrorMessage);
        LogPrintf("CMasternodeBroadcast::Create -- %s\n", strErrorMessage);
        return false;
    }

    if(!pwalletMain->GetMasternodeVinAndKeys(txin, pubKeyCollateralAddress, keyCollateralAddress, strTxHash, strOutputIndex)) {
        strErrorMessage = strprintf("Could not allocate txin %s:%s for masternode %s", strTxHash, strOutputIndex, strService);
        LogPrintf("CMasternodeBroadcast::Create -- %s\n", strErrorMessage);
        return false;
    }

    int age = GetInputAge(txin);
    if (age < MASTERNODE_MIN_CONFIRMATIONS)
    {
        strErrorMessage = strprintf("Input must have at least %d confirmations. Now it has %d",
                                     MASTERNODE_MIN_CONFIRMATIONS, age);
        LogPrintf("CSystemnodeBroadcast::Create -- %s\n", strErrorMessage);
        return false;
    }

    CService service = CService(strService);
    if(Params().NetworkID() == CBaseChainParams::MAIN) {
        if(service.GetPort() != 9340) {
            strErrorMessage = strprintf("Invalid port %u for masternode %s - only 9340 is supported on mainnet.", service.GetPort(), strService);
            LogPrintf("CMasternodeBroadcast::Create -- %s\n", strErrorMessage);
            return false;
        }
    } else if(service.GetPort() == 9340) {
        strErrorMessage = strprintf("Invalid port %u for masternode %s - 9340 is only supported on mainnet.", service.GetPort(), strService);
        LogPrintf("CMasternodeBroadcast::Create -- %s\n", strErrorMessage);
        return false;
    }

    bool fSignOver = true;
    return Create(txin, CService(strService), keyCollateralAddress, pubKeyCollateralAddress, keyMasternodeNew, pubKeyMasternodeNew, fSignOver, strErrorMessage, mnb);
}

bool CMasternodeBroadcast::Create(CTxIn txin, CService service, CKey keyCollateralAddress, CPubKey pubKeyCollateralAddress, CKey keyMasternodeNew, CPubKey pubKeyMasternodeNew, bool fSignOver, std::string &strErrorMessage, CMasternodeBroadcast &mnb) {
    // wait for reindex and/or import to finish
    if (fImporting || fReindex) return false;

    CMasternodePing mnp(txin);
    if(!mnp.Sign(keyMasternodeNew, pubKeyMasternodeNew)){
        strErrorMessage = strprintf("Failed to sign ping, txin: %s", txin.ToString());
        LogPrintf("CMasternodeBroadcast::Create --  %s\n", strErrorMessage);
        mnb = CMasternodeBroadcast();
        return false;
    }

    mnb = CMasternodeBroadcast(service, txin, pubKeyCollateralAddress, pubKeyMasternodeNew, PROTOCOL_VERSION);

    if(!mnb.IsValidNetAddr()) {
        strErrorMessage = strprintf("Invalid IP address, masternode=%s", txin.prevout.ToStringShort());
        LogPrintf("CMasternodeBroadcast::Create -- %s\n", strErrorMessage);
        mnb = CMasternodeBroadcast();
        return false;
    }

    mnb.lastPing = mnp;
    if(!mnb.Sign(keyCollateralAddress)){
        strErrorMessage = strprintf("Failed to sign broadcast, txin: %s", txin.ToString());
        LogPrintf("CMasternodeBroadcast::Create -- %s\n", strErrorMessage);
        mnb = CMasternodeBroadcast();
        return false;
    }

    //Additional signature for use in proof of stake
    if (fSignOver) {
        if (!keyCollateralAddress.Sign(pubKeyMasternodeNew.GetHash(), mnb.vchSignover)) {
            LogPrintf("CMasternodeBroadcast::Create failed signover\n");
            mnb = CMasternodeBroadcast();
            return false;
        }
        LogPrintf("%s: Signed over to key %s\n", __func__, pubKeyMasternodeNew.GetID().GetHex());
    }

    return true;
}

bool CMasternodeBroadcast::CheckAndUpdate(int& nDos) const
{
    nDos = 0;

    // make sure signature isn't in the future (past is OK)
    if (sigTime > GetAdjustedTime() + 60 * 60) {
        LogPrintf("mnb - Signature rejected, too far into the future %s\n", vin.ToString());
        nDos = 1;
        return false;
    }

    if(protocolVersion < masternodePayments.GetMinMasternodePaymentsProto()) {
        LogPrintf("mnb - ignoring outdated Masternode %s protocol version %d\n", vin.ToString(), protocolVersion);
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
    if(lastPing == CMasternodePing() || !lastPing.CheckAndUpdate(nDos, false, true))
        return false;

    std::string strMessage;
    std::string errorMessage = "";

    if(protocolVersion <= 99999999) {
        std::string vchPubKey(pubkey.begin(), pubkey.end());
        std::string vchPubKey2(pubkey2.begin(), pubkey2.end());
        strMessage = addr.ToString(false) + boost::lexical_cast<std::string>(sigTime) +
                        vchPubKey + vchPubKey2 + boost::lexical_cast<std::string>(protocolVersion);

        LogPrint("masternode", "mnb - sanitized strMessage: %s, pubkey address: %s, sig: %s\n",
            SanitizeString(strMessage), CBitcoinAddress(pubkey.GetID()).ToString(),
            EncodeBase64(&sig[0], sig.size()));

        if(!legacySigner.VerifyMessage(pubkey, sig, strMessage, errorMessage)){
            if (addr.ToString() != addr.ToString(false))
            {
                // maybe it's wrong format, try again with the old one
                strMessage = addr.ToString() + boost::lexical_cast<std::string>(sigTime) +
                                vchPubKey + vchPubKey2 + boost::lexical_cast<std::string>(protocolVersion);

                LogPrint("masternode", "mnb - sanitized strMessage: %s, pubkey address: %s, sig: %s\n",
                    SanitizeString(strMessage), CBitcoinAddress(pubkey.GetID()).ToString(),
                    EncodeBase64(&sig[0], sig.size()));

                if(!legacySigner.VerifyMessage(pubkey, sig, strMessage, errorMessage)){
                    // didn't work either
                    LogPrintf("mnb - Got bad Masternode address signature, sanitized error: %s\n", SanitizeString(errorMessage));
                    // there is a bug in old MN signatures, ignore such MN but do not ban the peer we got this from
                    return false;
                }
            } else {
                // nope, sig is actually wrong
                LogPrintf("mnb - Got bad Masternode address signature, sanitized error: %s\n", SanitizeString(errorMessage));
                // there is a bug in old MN signatures, ignore such MN but do not ban the peer we got this from
                return false;
            }
        }
    } else {
        strMessage = addr.ToString(false) + boost::lexical_cast<std::string>(sigTime) +
                        pubkey.GetID().ToString() + pubkey2.GetID().ToString() +
                        boost::lexical_cast<std::string>(protocolVersion);

        LogPrint("masternode", "mnb - strMessage: %s, pubkey address: %s, sig: %s\n",
            strMessage, CBitcoinAddress(pubkey.GetID()).ToString(), EncodeBase64(&sig[0], sig.size()));

        if(!legacySigner.VerifyMessage(pubkey, sig, strMessage, errorMessage)){
            LogPrintf("mnb - Got bad Masternode address signature, error: %s\n", errorMessage);
            nDos = 100;
            return false;
        }
    }

    if(Params().NetworkID() == CBaseChainParams::MAIN) {
        if(addr.GetPort() != 9340) return false;
    } else if(addr.GetPort() == 9340) return false;

    //search existing Masternode list, this is where we update existing Masternodes with new mnb broadcasts
    CMasternode* pmn = mnodeman.Find(vin);

    // no such masternode, nothing to update
    if(pmn == NULL) return true;

    // this broadcast is older or equal than the one that we already have - it's bad and should never happen
    // unless someone is doing something fishy
    // (mapSeenMasternodeBroadcast in CMasternodeMan::ProcessMessage should filter legit duplicates)
    if(pmn->sigTime >= sigTime) {
        LogPrintf("CMasternodeBroadcast::CheckAndUpdate - Bad sigTime %d for Masternode %20s %105s (existing broadcast is at %d)\n",
                      sigTime, addr.ToString(), vin.ToString(), pmn->sigTime);
        return false;
    }

    // masternode is not enabled yet/already, nothing to update
    if(!pmn->IsEnabled()) return true;

    // mn.pubkey = pubkey, IsVinAssociatedWithPubkey is validated once below,
    //   after that they just need to match
    if(pmn->pubkey == pubkey && !pmn->IsBroadcastedWithin(MASTERNODE_MIN_MNB_SECONDS)) {
        //take the newest entry
        LogPrintf("mnb - Got updated entry for %s\n", addr.ToString());
        if(pmn->UpdateFromNewBroadcast((*this))){
            pmn->Check();
            if(pmn->IsEnabled()) Relay();
        }
        masternodeSync.AddedMasternodeList(GetHash());
    }

    return true;
}

bool CMasternodeBroadcast::CheckInputsAndAdd(int& nDoS) const
{
    // we are a masternode with the same vin (i.e. already activated) and this mnb is ours (matches our Masternode privkey)
    // so nothing to do here for us
    if(fMasterNode && vin.prevout == activeMasternode.vin.prevout && pubkey2 == activeMasternode.pubKeyMasternode)
        return true;

    // incorrect ping or its sigTime
    if(lastPing == CMasternodePing() || !lastPing.CheckAndUpdate(nDoS, false, true))
        return false;

    // search existing Masternode list
    CMasternode* pmn = mnodeman.Find(vin);

    if(pmn != NULL) {
        // nothing to do here if we already know about this masternode and it's enabled
        if(pmn->IsEnabled()) return true;
        // if it's not enabled, remove old MN first and continue
        else mnodeman.Remove(pmn->vin);
    }

    CValidationState state;
    CMutableTransaction tx = CMutableTransaction();
    CTxOut vout = CTxOut((MASTERNODE_COLLATERAL - 0.01)*COIN, legacySigner.collateralPubKey);
    tx.vin.push_back(vin);
    tx.vout.push_back(vout);

    {
        TRY_LOCK(cs_main, lockMain);
        if(!lockMain) {
            // not mnb fault, let it to be checked again later
            mnodeman.mapSeenMasternodeBroadcast.erase(GetHash());
            masternodeSync.mapSeenSyncMNB.erase(GetHash());
            return false;
        }

        if(!AcceptableInputs(mempool, state, CTransaction(tx), false, NULL)) {
            //set nDos
            state.IsInvalid(nDoS);
            return false;
        }
    }

    LogPrint("masternode", "mnb - Accepted Masternode entry\n");

    if(GetInputAge(vin) < MASTERNODE_MIN_CONFIRMATIONS){
        LogPrintf("mnb - Input must have at least %d confirmations\n", MASTERNODE_MIN_CONFIRMATIONS);
        // maybe we miss few blocks, let this mnb to be checked again later
        mnodeman.mapSeenMasternodeBroadcast.erase(GetHash());
        masternodeSync.mapSeenSyncMNB.erase(GetHash());
        return false;
    }

    // verify that sig time is legit in past
    // should be at least not earlier than block when 10000 CRW tx got MASTERNODE_MIN_CONFIRMATIONS
    uint256 hashBlock = uint256();
    CTransaction tx2;
    GetTransaction(vin.prevout.hash, tx2, hashBlock, true);
    BlockMap::iterator mi = mapBlockIndex.find(hashBlock);
    if (mi != mapBlockIndex.end() && (*mi).second)
    {
        CBlockIndex* pMNIndex = (*mi).second; // block for 10000 CRW tx -> 1 confirmation
        CBlockIndex* pConfIndex = chainActive[pMNIndex->nHeight + MASTERNODE_MIN_CONFIRMATIONS - 1]; // block where tx got MASTERNODE_MIN_CONFIRMATIONS
        if(pConfIndex->GetBlockTime() > sigTime)
        {
            LogPrintf("mnb - Bad sigTime %d for Masternode %20s %105s (%i conf block is at %d)\n",
                      sigTime, addr.ToString(), vin.ToString(), MASTERNODE_MIN_CONFIRMATIONS, pConfIndex->GetBlockTime());
            return false;
        }
    }

    LogPrintf("mnb - Got NEW Masternode entry - %s - %s - %s - %lli \n", GetHash().ToString(), addr.ToString(), vin.ToString(), sigTime);
    CMasternode mn(*this);
    mnodeman.Add(mn);

    // if it matches our Masternode privkey, then we've been remotely activated
    if(pubkey2 == activeMasternode.pubKeyMasternode && protocolVersion == PROTOCOL_VERSION){
        activeMasternode.EnableHotColdMasterNode(vin, addr);
        if (!vchSignover.empty()) {
            if (pubkey.Verify(pubkey2.GetHash(), vchSignover)) {
                LogPrintf("%s: Verified pubkey2 signover for staking, added to activemasternode\n", __func__);
                activeMasternode.vchSigSignover = vchSignover;
            } else {
                LogPrintf("%s: Failed to verify pubkey on signover!\n", __func__);
            }
        } else {
            LogPrintf("%s: NOT SIGNOVER!\n", __func__);
        }
    }

    bool isLocal = addr.IsRFC1918() || addr.IsLocal();
    if(Params().NetworkID() == CBaseChainParams::REGTEST) isLocal = false;

    if(!isLocal) Relay();

    return true;
}

void CMasternodeBroadcast::Relay() const
{
    CInv inv(MSG_MASTERNODE_ANNOUNCE, GetHash());
    RelayInv(inv);
}

bool CMasternodeBroadcast::Sign(const CKey& keyCollateralAddress)
{
    std::string errorMessage;

    std::string vchPubKey(pubkey.begin(), pubkey.end());
    std::string vchPubKey2(pubkey2.begin(), pubkey2.end());

    sigTime = GetAdjustedTime();

    std::string strMessage = addr.ToString(false) + boost::lexical_cast<std::string>(sigTime) + vchPubKey + vchPubKey2 + boost::lexical_cast<std::string>(protocolVersion);

    if(!legacySigner.SignMessage(strMessage, errorMessage, sig, keyCollateralAddress)) {
        LogPrintf("CMasternodeBroadcast::Sign() - Error: %s\n", errorMessage);
        return false;
    }

    return true;
}

bool CMasternodeBroadcast::VerifySignature() const
{
    std::string errorMessage;

    std::string vchPubKey(pubkey.begin(), pubkey.end());
    std::string vchPubKey2(pubkey2.begin(), pubkey2.end());

    std::string strMessage = addr.ToString() + boost::lexical_cast<std::string>(sigTime) + vchPubKey + vchPubKey2 + boost::lexical_cast<std::string>(protocolVersion);

    if(!legacySigner.VerifyMessage(pubkey, sig, strMessage, errorMessage)) {
        LogPrintf("CMasternodeBroadcast::VerifySignature() - Error: %s\n", errorMessage);
        return false;
    }

    return true;
}

CMasternodePing::CMasternodePing()
{
    vin = CTxIn();
    blockHash = uint256();
    sigTime = 0;
    vchSig = std::vector<unsigned char>();
}

CMasternodePing::CMasternodePing(const CTxIn& newVin)
{
    vin = newVin;
    blockHash = chainActive[chainActive.Height() - 12]->GetBlockHash();
    sigTime = GetAdjustedTime();
    vchSig = std::vector<unsigned char>();
}


bool CMasternodePing::Sign(const CKey& keyMasternode, const CPubKey& pubKeyMasternode)
{
    std::string errorMessage;
    std::string strMasterNodeSignMessage;

    sigTime = GetAdjustedTime();
    std::string strMessage = vin.ToString() + blockHash.ToString() + boost::lexical_cast<std::string>(sigTime);

    if(!legacySigner.SignMessage(strMessage, errorMessage, vchSig, keyMasternode)) {
        LogPrintf("CMasternodePing::Sign() - Error: %s\n", errorMessage);
        return false;
    }

    if(!legacySigner.VerifyMessage(pubKeyMasternode, vchSig, strMessage, errorMessage)) {
        LogPrintf("CMasternodePing::Sign() - Error: %s\n", errorMessage);
        return false;
    }

    return true;
}

bool CMasternodePing::VerifySignature(const CPubKey& pubKeyMasternode, int &nDos) const
{
    std::string strMessage = vin.ToString() + blockHash.ToString() + boost::lexical_cast<std::string>(sigTime);
    std::string errorMessage = "";

    if(!legacySigner.VerifyMessage(pubKeyMasternode, vchSig, strMessage, errorMessage))
    {
        LogPrintf("CMasternodePing::VerifySignature - Got bad Masternode ping signature %s Error: %s\n", vin.ToString(), errorMessage);
        nDos = 33;
        return false;
    }
    return true;
}

bool CMasternodePing::CheckAndUpdate(int& nDos, bool fRequireEnabled, bool fCheckSigTimeOnly) const
{
    if (sigTime > GetAdjustedTime() + 60 * 60) {
        LogPrintf("CMasternodePing::CheckAndUpdate - Signature rejected, too far into the future %s\n", vin.ToString());
        nDos = 1;
        return false;
    }

    if (sigTime <= GetAdjustedTime() - 60 * 60) {
        LogPrintf("CMasternodePing::CheckAndUpdate - Signature rejected, too far into the past %s - %d %d \n", vin.ToString(), sigTime, GetAdjustedTime());
        nDos = 1;
        return false;
    }

    if(fCheckSigTimeOnly) {
        CMasternode* pmn = mnodeman.Find(vin);
        if(pmn) return VerifySignature(pmn->pubkey2, nDos);
        return true;
    }

    LogPrint("masternode", "CMasternodePing::CheckAndUpdate - New Ping - %s - %s - %lli\n", GetHash().ToString(), blockHash.ToString(), sigTime);

    // see if we have this Masternode
    CMasternode* pmn = mnodeman.Find(vin);
    if(pmn != NULL && pmn->protocolVersion >= masternodePayments.GetMinMasternodePaymentsProto())
    {
        if (fRequireEnabled && !pmn->IsEnabled()) return false;

        // LogPrintf("mnping - Found corresponding mn for vin: %s\n", vin.ToString());
        // update only if there is no known ping for this masternode or
        // last ping was more then MASTERNODE_MIN_MNP_SECONDS-60 ago comparing to this one
        if(!pmn->IsPingedWithin(MASTERNODE_MIN_MNP_SECONDS - 60, sigTime))
        {
            if(!VerifySignature(pmn->pubkey2, nDos))
                return false;

            BlockMap::iterator mi = mapBlockIndex.find(blockHash);
            if (mi != mapBlockIndex.end() && (*mi).second)
            {
                if((*mi).second->nHeight < chainActive.Height() - 24)
                {
                    LogPrintf("CMasternodePing::CheckAndUpdate - Masternode %s block hash %s is too old\n", vin.ToString(), blockHash.ToString());
                    // Do nothing here (no Masternode update, no mnping relay)
                    // Let this node to be visible but fail to accept mnping

                    return false;
                }
            } else {
                if (fDebug) LogPrintf("CMasternodePing::CheckAndUpdate - Masternode %s block hash %s is unknown\n", vin.ToString(), blockHash.ToString());
                // maybe we stuck so we shouldn't ban this node, just fail to accept it
                // TODO: or should we also request this block?

                return false;
            }

            pmn->lastPing = *this;

            //mnodeman.mapSeenMasternodeBroadcast.lastPing is probably outdated, so we'll update it
            CMasternodeBroadcast mnb(*pmn);
            uint256 hash = mnb.GetHash();
            if(mnodeman.mapSeenMasternodeBroadcast.count(hash)) {
                mnodeman.mapSeenMasternodeBroadcast[hash].lastPing = *this;
            }

            pmn->Check(true);
            if(!pmn->IsEnabled()) return false;

            LogPrint("masternode", "CMasternodePing::CheckAndUpdate - Masternode ping accepted, vin: %s\n", vin.ToString());

            Relay();
            return true;
        }
        LogPrint("masternode", "CMasternodePing::CheckAndUpdate - Masternode ping arrived too early, vin: %s\n", vin.ToString());
        //nDos = 1; //disable, this is happening frequently and causing banned peers
        return false;
    }
    LogPrint("masternode", "CMasternodePing::CheckAndUpdate - Couldn't find compatible Masternode entry, vin: %s\n", vin.ToString());

    return false;
}

void CMasternodePing::Relay() const
{
    CInv inv(MSG_MASTERNODE_PING, GetHash());
    RelayInv(inv);
}
