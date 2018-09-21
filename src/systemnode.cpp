// Copyright (c) 2014-2018 The Crown developers
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

CSystemnodePing::CSystemnodePing(const CTxIn& newVin)
{
    vin = newVin;
    blockHash = chainActive[chainActive.Height() - 12]->GetBlockHash();
    sigTime = GetAdjustedTime();
    vchSig = std::vector<unsigned char>();
}

bool CSystemnodePing::Sign(const CKey& keySystemnode, const CPubKey& pubKeySystemnode)
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

void CSystemnodePing::Relay() const
{
    CInv inv(MSG_SYSTEMNODE_PING, GetHash());
    RelayInv(inv);
}

bool CSystemnodePing::CheckAndUpdate(int& nDos, bool fRequireEnabled, bool fCheckSigTimeOnly) const
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
        CSystemnode* psn = snodeman.Find(vin);
        if(psn) return VerifySignature(psn->pubkey2, nDos);
        return true;
    }

    LogPrint("systemnode", "CSystemnodePing::CheckAndUpdate - New Ping - %s - %s - %lli\n", GetHash().ToString(), blockHash.ToString(), sigTime);

    // see if we have this Systemnode
    CSystemnode* psn = snodeman.Find(vin);
    if(psn != NULL && psn->protocolVersion >= systemnodePayments.GetMinSystemnodePaymentsProto())
    {
        if (fRequireEnabled && !psn->IsEnabled()) return false;

        // LogPrintf("snping - Found corresponding sn for vin: %s\n", vin.ToString());
        // update only if there is no known ping for this systemnode or
        // last ping was more then SYSTEMNODE_MIN_MNP_SECONDS-60 ago comparing to this one
        if(!psn->IsPingedWithin(SYSTEMNODE_MIN_SNP_SECONDS - 60, sigTime))
        {
            if(!VerifySignature(psn->pubkey2, nDos))
                return false;

            BlockMap::iterator mi = mapBlockIndex.find(blockHash);
            if (mi != mapBlockIndex.end() && (*mi).second)
            {
                if((*mi).second->nHeight < chainActive.Height() - 24)
                {
                    LogPrintf("CSystemnodePing::CheckAndUpdate - Systemnode %s block hash %s is too old\n", vin.ToString(), blockHash.ToString());
                    // Do nothing here (no Systemnode update, no snping relay)
                    // Let this node to be visible but fail to accept snping

                    return false;
                }
            } else {
                if (fDebug) LogPrintf("CSystemnodePing::CheckAndUpdate - Systemnode %s block hash %s is unknown\n", vin.ToString(), blockHash.ToString());
                // maybe we stuck so we shouldn't ban this node, just fail to accept it
                // TODO: or should we also request this block?

                return false;
            }

            psn->lastPing = *this;

            //snodeman.mapSeenSystemnodeBroadcast.lastPing is probably outdated, so we'll update it
            CSystemnodeBroadcast snb(*psn);
            uint256 hash = snb.GetHash();
            if(snodeman.mapSeenSystemnodeBroadcast.count(hash)) {
                snodeman.mapSeenSystemnodeBroadcast[hash].lastPing = *this;
            }

            psn->Check(true);
            if(!psn->IsEnabled()) return false;

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

bool CSystemnodePing::VerifySignature(const CPubKey& pubKeySystemnode, int &nDos) const
{
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
    lastTimeChecked = 0;
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
arith_uint256 CSystemnode::CalculateScore(int64_t nBlockHeight) const
{
    if(chainActive.Tip() == NULL)
        return arith_uint256();

    // Find the block hash where tx got SYSTEMNODE_MIN_CONFIRMATIONS
    CBlockIndex *pblockIndex = chainActive[GetInputHeight(vin) + SYSTEMNODE_MIN_CONFIRMATIONS - 1];
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

//
// When a new systemnode broadcast is sent, update our information
//
bool CSystemnode::UpdateFromNewBroadcast(const CSystemnodeBroadcast& snb)
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
        CTxOut vout = CTxOut((SYSTEMNODE_COLLATERAL - 0.01)*COIN, legacySigner.collateralPubKey);
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

int64_t CSystemnode::SecondsSincePayment() const
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

int64_t CSystemnode::GetLastPaid(const CBlockIndex *BlockReading) const
{
    CBlockIndex* pindexPrev = chainActive.Tip();
    if(pindexPrev == NULL) return false;

    CScript snpayee;
    snpayee = GetScriptForDestination(pubkey.GetID());

    CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
    ss << vin;
    ss << sigTime;
    uint256 hash =  ss.GetHash();

    // use a deterministic offset to break a tie -- 2.5 minutes
    int64_t nOffset = UintToArith256(hash).GetCompact(false) % 150; 

    if (chainActive.Tip() == NULL) return false;

    BlockReading = chainActive.Tip();

    int nMnCount = snodeman.CountEnabled()*1.25;
    int n = 0;
    for (unsigned int i = 1; BlockReading && BlockReading->nHeight > 0; i++) {
        if(n >= nMnCount){
            return 0;
        }
        n++;

        if(systemnodePayments.mapSystemnodeBlocks.count(BlockReading->nHeight)){
            /*
                Search for this payee, with at least 2 votes. This will aid in consensus allowing the network 
                to converge on the same payees quickly, then keep the same schedule.
            */
            if(systemnodePayments.mapSystemnodeBlocks[BlockReading->nHeight].HasPayeeWithVotes(snpayee, 2)){
                return BlockReading->nTime + nOffset;
            }
        }

        if (BlockReading->pprev == NULL) { assert(BlockReading); break; }
        BlockReading = BlockReading->pprev;
    }

    return 0;
}

//
// CSystemnodeBroadcast
//

bool CSystemnodeBroadcast::CheckAndUpdate(int& nDos) const
{
    nDos = 0;

    // make sure signature isn't in the future (past is OK)
    if (sigTime > GetAdjustedTime() + 60 * 60) {
        LogPrintf("snb - Signature rejected, too far into the future %s\n", vin.ToString());
        nDos = 1;
        return false;
    }

    if(protocolVersion < systemnodePayments.GetMinSystemnodePaymentsProto()) {
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
    CSystemnode* psn = snodeman.Find(vin);

    // no such systemnode, nothing to update
    if(psn == NULL) return true;

    // this broadcast is older or equal than the one that we already have - it's bad and should never happen
    // unless someone is doing something fishy
    // (mapSeensystemnodeBroadcast in CSystemnodeMan::ProcessMessage should filter legit duplicates)
    if(psn->sigTime >= sigTime) {
        LogPrintf("CsystemnodeBroadcast::CheckAndUpdate - Bad sigTime %d for Systemnode %20s %105s (existing broadcast is at %d)\n",
                      sigTime, addr.ToString(), vin.ToString(), psn->sigTime);
        return false;
    }

    // systemnode is not enabled yet/already, nothing to update
    if(!psn->IsEnabled()) return true;

    // sn.pubkey = pubkey, IsVinAssociatedWithPubkey is validated once below,
    //   after that they just need to match
    if(psn->pubkey == pubkey && !psn->IsBroadcastedWithin(SYSTEMNODE_MIN_SNB_SECONDS)) {
        //take the newest entry
        LogPrintf("snb - Got updated entry for %s\n", addr.ToString());
        if(psn->UpdateFromNewBroadcast((*this))){
            psn->Check();
            if(psn->IsEnabled()) Relay();
        }
        systemnodeSync.AddedSystemnodeList(GetHash());
    }

    return true;

}

bool CSystemnodeBroadcast::CheckInputsAndAdd(int& nDoS) const
{
    // we are a systemnode with the same vin (i.e. already activated) and this snb is ours (matches our systemnode privkey)
    // so nothing to do here for us
    if(fSystemNode && vin.prevout == activeSystemnode.vin.prevout && pubkey2 == activeSystemnode.pubKeySystemnode)
        return true;

    // incorrect ping or its sigTime
    if(lastPing == CSystemnodePing() || !lastPing.CheckAndUpdate(nDoS, false, true))
        return false;

    // search existing systemnode list
    CSystemnode* psn = snodeman.Find(vin);

    if(psn != NULL) {
        // nothing to do here if we already know about this systemnode and it's enabled
        if(psn->IsEnabled()) return true;
        // if it's not enabled, remove old MN first and continue
        else snodeman.Remove(psn->vin);
    }

    CValidationState state;
    CMutableTransaction tx = CMutableTransaction();
    CTxOut vout = CTxOut((SYSTEMNODE_COLLATERAL - 0.01)*COIN, legacySigner.collateralPubKey);
    tx.vin.push_back(vin);
    tx.vout.push_back(vout);

    {
        TRY_LOCK(cs_main, lockMain);
        if(!lockMain) {
            // not snb fault, let it to be checked again later
            snodeman.mapSeenSystemnodeBroadcast.erase(GetHash());
            systemnodeSync.mapSeenSyncSNB.erase(GetHash());
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
        systemnodeSync.mapSeenSyncSNB.erase(GetHash());
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
    CSystemnode sn(*this);
    snodeman.Add(sn);

    // if it matches our systemnode privkey, then we've been remotely activated
    if(pubkey2 == activeSystemnode.pubKeySystemnode && protocolVersion == PROTOCOL_VERSION){
        activeSystemnode.EnableHotColdSystemNode(vin, addr);
    }

    bool isLocal = addr.IsRFC1918() || addr.IsLocal();
    if(Params().NetworkID() == CBaseChainParams::REGTEST) isLocal = false;

    if(!isLocal) Relay();

    return true;

}

void CSystemnodeBroadcast::Relay() const
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

CSystemnodeBroadcast::CSystemnodeBroadcast(const CSystemnode& sn)
{
    vin = sn.vin;
    addr = sn.addr;
    pubkey = sn.pubkey;
    pubkey2 = sn.pubkey2;
    sig = sn.sig;
    activeState = sn.activeState;
    sigTime = sn.sigTime;
    lastPing = sn.lastPing;
    unitTest = sn.unitTest;
    protocolVersion = sn.protocolVersion;
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

    int age = GetInputAge(txin);
    if (age < SYSTEMNODE_MIN_CONFIRMATIONS)
    {
        strErrorMessage = strprintf("Input must have at least %d confirmations. Now it has %d",
                                     SYSTEMNODE_MIN_CONFIRMATIONS, age);
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

bool CSystemnodeBroadcast::Sign(const CKey& keyCollateralAddress)
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

bool CSystemnodeBroadcast::VerifySignature() const
{
    std::string errorMessage;

    std::string vchPubKey(pubkey.begin(), pubkey.end());
    std::string vchPubKey2(pubkey2.begin(), pubkey2.end());

    std::string strMessage = addr.ToString() + boost::lexical_cast<std::string>(sigTime) + vchPubKey + vchPubKey2 + boost::lexical_cast<std::string>(protocolVersion);

    if(!legacySigner.VerifyMessage(pubkey, sig, strMessage, errorMessage)) {
        LogPrintf("CSystemnodeBroadcast::VerifySignature() - Error: %s\n", errorMessage);
        return false;
    }

    return true;
}
