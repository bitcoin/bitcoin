// Copyright (c) 2014-2016 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "consensus/validation.h"
#include "activemasternode.h"
#include "darksend.h"
#include "init.h"
#include "masternode.h"
#include "masternode-payments.h"
#include "masternode-sync.h"
#include "masternodeman.h"
#include "governance.h"
#include "util.h"
#include "sync.h"
#include "addrman.h"
#include <boost/lexical_cast.hpp>

// keep track of the scanning errors I've seen
map<uint256, int> mapSeenMasternodeScanningErrors;

CMasternode::CMasternode()
{
    LOCK(cs);
    vin = CTxIn();
    addr = CService();
    pubkey = CPubKey();
    pubkey2 = CPubKey();
    vchSig = std::vector<unsigned char>();
    activeState = MASTERNODE_ENABLED;
    sigTime = GetAdjustedTime();
    lastPing = CMasternodePing();
    nTimeLastPaid = 0;
    nBlockLastPaid = 0;
    nCacheCollateralBlock = 0;
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
    vchSig = other.vchSig;
    activeState = other.activeState;
    sigTime = other.sigTime;
    lastPing = other.lastPing;
    nTimeLastPaid = other.nTimeLastPaid;
    nBlockLastPaid = other.nBlockLastPaid;
    nCacheCollateralBlock = other.nCacheCollateralBlock;
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
    vchSig = mnb.vchSig;
    activeState = MASTERNODE_ENABLED;
    sigTime = mnb.sigTime;
    lastPing = mnb.lastPing;
    nTimeLastPaid = 0;
    nBlockLastPaid = 0;
    nCacheCollateralBlock = 0;
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
bool CMasternode::UpdateFromNewBroadcast(CMasternodeBroadcast& mnb)
{
    if(mnb.sigTime > sigTime) {
        pubkey2 = mnb.pubkey2;
        sigTime = mnb.sigTime;
        vchSig = mnb.vchSig;
        protocolVersion = mnb.protocolVersion;
        addr = mnb.addr;
        lastTimeChecked = 0;
        int nDos = 0;
        if(mnb.lastPing == CMasternodePing() || (mnb.lastPing != CMasternodePing() && mnb.lastPing.CheckAndUpdate(nDos, false))) {
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
uint256 CMasternode::CalculateScore(int nBlockHeight)
{
    uint256 hash;
    uint256 aux = ArithToUint256(UintToArith256(vin.prevout.hash) + vin.prevout.n);

    if(!GetBlockHash(hash, nBlockHeight)) {
        LogPrintf("CMasternode::CalculateScore -- ERROR: GetBlockHash() failed at nBlockHeight %d\n", nBlockHeight);
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

void CMasternode::Check(bool forceCheck)
{
    if(ShutdownRequested()) return;

    if(!forceCheck && (GetTime() - lastTimeChecked < MASTERNODE_CHECK_SECONDS)) return;
    lastTimeChecked = GetTime();


    //once spent, stop doing the checks
    if(activeState == MASTERNODE_VIN_SPENT) return;

    // If there are no pings for quite a long time ...
    if(!IsPingedWithin(MASTERNODE_REMOVAL_SECONDS)
        // or doesn't meet payments requirements ...
        || protocolVersion < mnpayments.GetMinMasternodePaymentsProto()
        // or it's our own node and we just updated it to the new protocol but we are still waiting for activation -
        || (pubkey2 == activeMasternode.pubKeyMasternode && protocolVersion < PROTOCOL_VERSION)) {
        // remove it from the list
        activeState = MASTERNODE_REMOVE;

        // RESCAN AFFECTED VOTES
        FlagGovernanceItemsAsDirty();
        return;
    }

    if(!IsPingedWithin(MASTERNODE_EXPIRATION_SECONDS)){
        activeState = MASTERNODE_EXPIRED;

        // RESCAN AFFECTED VOTES
        FlagGovernanceItemsAsDirty();
        return;
    }

    if(lastPing.sigTime - sigTime < MASTERNODE_MIN_MNP_SECONDS){
        activeState = MASTERNODE_PRE_ENABLED;
        return;
    }

    if(!unitTest){
        CValidationState state;
        CMutableTransaction tx = CMutableTransaction();
        CTxOut txout = CTxOut(999.99*COIN, mnodeman.dummyScriptPubkey);
        tx.vin.push_back(vin);
        tx.vout.push_back(txout);

        {
            TRY_LOCK(cs_main, lockMain);
            if(!lockMain) return;

            if(!AcceptToMemoryPool(mempool, state, CTransaction(tx), false, NULL, false, true, true)){
                activeState = MASTERNODE_VIN_SPENT;
                return;
            }
        }
    }

    activeState = MASTERNODE_ENABLED; // OK
}

int CMasternode::GetCollateralAge()
{
    int nHeight;
    {
        TRY_LOCK(cs_main, lockMain);
        if(!lockMain || !chainActive.Tip()) return -1;
        nHeight = chainActive.Height();
    }

    if (nCacheCollateralBlock == 0) {
        int nInputAge = GetInputAge(vin);
        if(nInputAge > 0) {
            nCacheCollateralBlock = nHeight - nInputAge;
        } else {
            return nInputAge;
        }
    }

    return nHeight - nCacheCollateralBlock;
}

void CMasternode::UpdateLastPaid(const CBlockIndex *pindex, int nMaxBlocksToScanBack)
{
    if(!pindex) return;

    const CBlockIndex *BlockReading = pindex;

    CScript mnpayee = GetScriptForDestination(pubkey.GetID());
    // LogPrint("masternode", "CMasternode::UpdateLastPaidBlock -- searching for block with payment to %s\n", vin.prevout.ToStringShort());

    LOCK(cs_mapMasternodeBlocks);

    for (int i = 0; BlockReading && BlockReading->nHeight > nBlockLastPaid && i < nMaxBlocksToScanBack; i++) {
        if(mnpayments.mapMasternodeBlocks.count(BlockReading->nHeight) &&
            mnpayments.mapMasternodeBlocks[BlockReading->nHeight].HasPayeeWithVotes(mnpayee, 2))
        {
            CBlock block;
            if(!ReadBlockFromDisk(block, BlockReading, Params().GetConsensus())) // shouldn't really happen
                continue;

            CAmount nMasternodePayment = GetMasternodePayment(BlockReading->nHeight, block.vtx[0].GetValueOut());

            BOOST_FOREACH(CTxOut txout, block.vtx[0].vout)
                if(mnpayee == txout.scriptPubKey && nMasternodePayment == txout.nValue) {
                    nBlockLastPaid = BlockReading->nHeight;
                    nTimeLastPaid = BlockReading->nTime;
                    LogPrint("masternode", "CMasternode::UpdateLastPaidBlock -- searching for block with payment to %s -- found new %d\n", vin.prevout.ToStringShort(), nBlockLastPaid);
                    return;
                }
        }

        if (BlockReading->pprev == NULL) { assert(BlockReading); break; }
        BlockReading = BlockReading->pprev;
    }

    // Last payment for this masternode wasn't found in latest mnpayments blocks
    // or it was found in mnpayments blocks but wasn't found in the blockchain.
    // LogPrint("masternode", "CMasternode::UpdateLastPaidBlock -- searching for block with payment to %s -- keeping old %d\n", vin.prevout.ToStringShort(), nBlockLastPaid);
}

CMasternodeBroadcast::CMasternodeBroadcast()
{
    vin = CTxIn();
    addr = CService();
    pubkey = CPubKey();
    pubkey2 = CPubKey();
    vchSig = std::vector<unsigned char>();
    activeState = MASTERNODE_ENABLED;
    sigTime = GetAdjustedTime();
    lastPing = CMasternodePing();
    nTimeLastPaid = 0;
    nBlockLastPaid = 0;
    nCacheCollateralBlock = 0;
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
    vchSig = std::vector<unsigned char>();
    activeState = MASTERNODE_ENABLED;
    sigTime = GetAdjustedTime();
    lastPing = CMasternodePing();
    nTimeLastPaid = 0;
    nBlockLastPaid = 0;
    nCacheCollateralBlock = 0;
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
    vchSig = mn.vchSig;
    activeState = mn.activeState;
    sigTime = mn.sigTime;
    lastPing = mn.lastPing;
    nTimeLastPaid = mn.nTimeLastPaid;
    nBlockLastPaid = mn.nBlockLastPaid;
    nCacheCollateralBlock = mn.nCacheCollateralBlock;
    unitTest = mn.unitTest;
    allowFreeTx = mn.allowFreeTx;
    protocolVersion = mn.protocolVersion;
    nLastDsq = mn.nLastDsq;
    nScanningErrorCount = mn.nScanningErrorCount;
    nLastScanningErrorBlockHeight = mn.nLastScanningErrorBlockHeight;
}

bool CMasternodeBroadcast::Create(std::string strService, std::string strKeyMasternode, std::string strTxHash, std::string strOutputIndex, std::string& strErrorRet, CMasternodeBroadcast &mnbRet, bool fOffline)
{
    CTxIn txin;
    CPubKey pubKeyCollateral;
    CKey keyCollateral;
    CPubKey pubKeyMasternodeNew;
    CKey keyMasternodeNew;

    //need correct blocks to send ping
    if(!fOffline && !masternodeSync.IsBlockchainSynced()) {
        strErrorRet = "Sync in progress. Must wait until sync is complete to start Masternode";
        LogPrintf("CMasternodeBroadcast::Create -- %s\n", strErrorRet);
        return false;
    }

    if(!darkSendSigner.GetKeysFromSecret(strKeyMasternode, keyMasternodeNew, pubKeyMasternodeNew)) {
        strErrorRet = strprintf("Invalid masternode key %s", strKeyMasternode);
        LogPrintf("CMasternodeBroadcast::Create -- %s\n", strErrorRet);
        return false;
    }

    if(!pwalletMain->GetMasternodeVinAndKeys(txin, pubKeyCollateral, keyCollateral, strTxHash, strOutputIndex)) {
        strErrorRet = strprintf("Could not allocate txin %s:%s for masternode %s", strTxHash, strOutputIndex, strService);
        LogPrintf("CMasternodeBroadcast::Create -- %s\n", strErrorRet);
        return false;
    }

    CService service = CService(strService);
    int mainnetDefaultPort = Params(CBaseChainParams::MAIN).GetDefaultPort();
    if(Params().NetworkIDString() == CBaseChainParams::MAIN) {
        if(service.GetPort() != mainnetDefaultPort) {
            strErrorRet = strprintf("Invalid port %u for masternode %s, only %d is supported on mainnet.", service.GetPort(), strService, mainnetDefaultPort);
            LogPrintf("CMasternodeBroadcast::Create -- %s\n", strErrorRet);
            return false;
        }
    } else if (service.GetPort() == mainnetDefaultPort) {
        strErrorRet = strprintf("Invalid port %u for masternode %s, %d is the only supported on mainnet.", service.GetPort(), strService, mainnetDefaultPort);
        LogPrintf("CMasternodeBroadcast::Create -- %s\n", strErrorRet);
        return false;
    }

    return Create(txin, CService(strService), keyCollateral, pubKeyCollateral, keyMasternodeNew, pubKeyMasternodeNew, strErrorRet, mnbRet);
}

bool CMasternodeBroadcast::Create(CTxIn txin, CService service, CKey keyCollateral, CPubKey pubKeyCollateral, CKey keyMasternodeNew, CPubKey pubKeyMasternodeNew, std::string &strErrorRet, CMasternodeBroadcast &mnbRet)
{
    // wait for reindex and/or import to finish
    if (fImporting || fReindex) return false;

    CMasternodePing mnp(txin);
    if(!mnp.Sign(keyMasternodeNew, pubKeyMasternodeNew)) {
        strErrorRet = strprintf("Failed to sign ping: %s", txin.ToString());
        LogPrintf("CMasternodeBroadcast::Create --  %s\n", strErrorRet);
        mnbRet = CMasternodeBroadcast();
        return false;
    }

    mnbRet = CMasternodeBroadcast(service, txin, pubKeyCollateral, pubKeyMasternodeNew, PROTOCOL_VERSION);
    mnbRet.lastPing = mnp;
    if(!mnbRet.Sign(keyCollateral)) {
        strErrorRet = strprintf("Failed to sign broadcast: %s", txin.ToString());
        LogPrintf("CMasternodeBroadcast::Create -- %s\n", strErrorRet);
        mnbRet = CMasternodeBroadcast();
        return false;
    }

    return true;
}

bool CMasternodeBroadcast::CheckAndUpdate(int& nDos)
{
    nDos = 0;
    // make sure signature isn't in the future (past is OK)
    if (sigTime > GetAdjustedTime() + 60 * 60) {
        LogPrintf("CMasternodeBroadcast::CheckAndUpdate - Signature rejected, too far into the future %s\n", vin.ToString());
        nDos = 1;
        return false;
    }

    // incorrect ping or its sigTime
    if(lastPing == CMasternodePing() || !lastPing.CheckAndUpdate(nDos, false, true)) {
        return false;
    }

    if(protocolVersion < mnpayments.GetMinMasternodePaymentsProto()) {
        LogPrintf("CMasternodeBroadcast::CheckAndUpdate - ignoring outdated Masternode %s protocol version %d\n", vin.ToString(), protocolVersion);
        return false;
    }

    CScript pubkeyScript;
    pubkeyScript = GetScriptForDestination(pubkey.GetID());

    if(pubkeyScript.size() != 25) {
        LogPrintf("CMasternodeBroadcast::CheckAndUpdate - pubkey the wrong size\n");
        nDos = 100;
        return false;
    }

    CScript pubkeyScript2;
    pubkeyScript2 = GetScriptForDestination(pubkey2.GetID());

    if(pubkeyScript2.size() != 25) {
        LogPrintf("CMasternodeBroadcast::CheckAndUpdate - pubkey2 the wrong size\n");
        nDos = 100;
        return false;
    }

    if(!vin.scriptSig.empty()) {
        LogPrintf("CMasternodeBroadcast::CheckAndUpdate - Ignore Not Empty ScriptSig %s\n",vin.ToString());
        return false;
    }

    if (!VerifySignature(nDos))
    {
        LogPrintf("CMasternodeBroadcast::CheckAndUpdate - VerifySignature failed %s\n",vin.ToString());
        return false;
    }

    int mainnetDefaultPort = Params(CBaseChainParams::MAIN).GetDefaultPort();
    if(Params().NetworkIDString() == CBaseChainParams::MAIN) {
        if(addr.GetPort() != mainnetDefaultPort) return false;
    } else if(addr.GetPort() == mainnetDefaultPort) return false;

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
        LogPrintf("CMasternodeBroadcast::CheckAndUpdate - Got updated entry for %s\n", addr.ToString());
        if(pmn->UpdateFromNewBroadcast((*this))){
            pmn->Check();
            // normally masternode should be in pre-enabled status after update, if not - do not relay
            if(pmn->IsPreEnabled()) {
                Relay();
            }
        }
        masternodeSync.AddedMasternodeList();
    }

    return true;
}

bool CMasternodeBroadcast::CheckInputsAndAdd(int& nDos)
{
    // we are a masternode with the same vin (i.e. already activated) and this mnb is ours (matches our Masternode privkey)
    // so nothing to do here for us
    if(fMasterNode && vin.prevout == activeMasternode.vin.prevout && pubkey2 == activeMasternode.pubKeyMasternode)
        return true;

    // incorrect ping or its sigTime
    if(lastPing == CMasternodePing() || !lastPing.CheckAndUpdate(nDos, false, true)) {
        return false;
    }

    // search existing Masternode list
    CMasternode* pmn = mnodeman.Find(vin);

    if(pmn != NULL) {
        // nothing to do here if we already know about this masternode and it's (pre)enabled
        if(pmn->IsEnabled() || pmn->IsPreEnabled()) return true;
        // if it's not (pre)enabled, remove old MN first and continue
        mnodeman.Remove(pmn->vin);
    }

    if(GetInputAge(vin) < Params().GetConsensus().nMasternodeMinimumConfirmations) {
        LogPrintf("CMasternodeBroadcast::CheckInputsAndAdd -- Input must have at least %d confirmations\n", Params().GetConsensus().nMasternodeMinimumConfirmations);
        // maybe we miss few blocks, let this mnb to be checked again later
        mnodeman.mapSeenMasternodeBroadcast.erase(GetHash());
        return false;
    }

    CValidationState state;
    CMutableTransaction dummyTx = CMutableTransaction();
    CTxOut dummyTxOut = CTxOut(999.99*COIN, mnodeman.dummyScriptPubkey);
    dummyTx.vin.push_back(vin);
    dummyTx.vout.push_back(dummyTxOut);

    {
        TRY_LOCK(cs_main, lockMain);
        if(!lockMain) {
            // not mnb fault, let it to be checked again later
            mnodeman.mapSeenMasternodeBroadcast.erase(GetHash());
            return false;
        }

        if(!AcceptToMemoryPool(mempool, state, CTransaction(dummyTx), false, NULL, false, true, true)) {
            //set nDos
            LogPrint("masternode", "CMasternodeBroadcast::CheckInputsAndAdd -- Failed to accepted Masternode entry to mempool: dummyTx=%s", dummyTx.ToString());
            state.IsInvalid(nDos);
            return false;
        }
    }

    LogPrint("masternode", "CMasternodeBroadcast::CheckInputsAndAdd -- Accepted Masternode entry to mempool (dry-run mode)\n");


    // make sure the vout that was signed is related to the transaction that spawned the Masternode
    //  - this is expensive, so it's only done once per Masternode
    if(!darkSendSigner.IsVinAssociatedWithPubkey(vin, pubkey)) {
        LogPrintf("CMasternodeMan::CheckInputsAndAdd - Got mismatched pubkey and vin\n");
        nDos = 33;
        return false;
    }

    // verify that sig time is legit in past
    // should be at least not earlier than block when 1000 DASH tx got nMasternodeMinimumConfirmations
    uint256 hashBlock = uint256();
    CTransaction tx2;
    GetTransaction(vin.prevout.hash, tx2, Params().GetConsensus(), hashBlock, true);
    {
        LOCK(cs_main);
        BlockMap::iterator mi = mapBlockIndex.find(hashBlock);
        if (mi != mapBlockIndex.end() && (*mi).second) {
            CBlockIndex* pMNIndex = (*mi).second; // block for 1000 DASH tx -> 1 confirmation
            CBlockIndex* pConfIndex = chainActive[pMNIndex->nHeight + Params().GetConsensus().nMasternodeMinimumConfirmations - 1]; // block where tx got nMasternodeMinimumConfirmations
            if(pConfIndex->GetBlockTime() > sigTime)
            {
                LogPrintf("CMasternodeBroadcast::CheckInputsAndAdd - Bad sigTime %d for Masternode %20s %105s (%i conf block is at %d)\n",
                          sigTime, addr.ToString(), vin.ToString(), Params().GetConsensus().nMasternodeMinimumConfirmations, pConfIndex->GetBlockTime());
                return false;
            }
        }
    }

    // if it matches our Masternode privkey...
    if(fMasterNode && pubkey2 == activeMasternode.pubKeyMasternode) {
        if(protocolVersion == PROTOCOL_VERSION) {
            // ... and PROTOCOL_VERSION, then we've been remotely activated ...
            activeMasternode.EnableRemoteMasterNode(vin, addr);
        } else {
            // ... otherwise we need to reactivate our node, don not add it to the list and do not relay
            // but also do not ban the node we get this message from
            LogPrintf("CMasternodeBroadcast::CheckInputsAndAdd - wrong PROTOCOL_VERSION, announce message: %d MN: %d - re-activate your MN\n", protocolVersion, PROTOCOL_VERSION);
            return false;
        }
    }

    LogPrintf("CMasternodeBroadcast::CheckInputsAndAdd - Got NEW Masternode entry - %s - %s - %s - %lli \n", GetHash().ToString(), addr.ToString(), vin.ToString(), sigTime);
    CMasternode mn(*this);
    mnodeman.Add(mn);

    bool isLocal = addr.IsRFC1918() || addr.IsLocal();
    if(Params().NetworkIDString() == CBaseChainParams::REGTEST) {
        isLocal = false;
    }

    if(!isLocal) {
        Relay();
    }

    return true;
}

void CMasternodeBroadcast::Relay()
{
    CInv inv(MSG_MASTERNODE_ANNOUNCE, GetHash());
    RelayInv(inv);
}

bool CMasternodeBroadcast::Sign(CKey& keyCollateralAddress)
{
    std::string strError;
    std::string strMessage;

    sigTime = GetAdjustedTime();

    strMessage = addr.ToString(false) + boost::lexical_cast<std::string>(sigTime) +
                    pubkey.GetID().ToString() + pubkey2.GetID().ToString() +
                    boost::lexical_cast<std::string>(protocolVersion);

    if(!darkSendSigner.SignMessage(strMessage, vchSig, keyCollateralAddress)) {
        LogPrintf("CMasternodeBroadcast::Sign -- SignMessage() failed\n");
        return false;
    }

    if(!darkSendSigner.VerifyMessage(pubkey, vchSig, strMessage, strError)) {
        LogPrintf("CMasternodeBroadcast::Sign -- VerifyMessage() failed, error: %s\n", strError);
        return false;
    }

    return true;
}

bool CMasternodeBroadcast::VerifySignature(int& nDos)
{
    std::string strMessage;
    std::string strError = "";
    nDos = 0;

    //
    // REMOVE AFTER MIGRATION TO 12.1
    //
    if(protocolVersion < 70201) {
        std::string vchPubKey(pubkey.begin(), pubkey.end());
        std::string vchPubKey2(pubkey2.begin(), pubkey2.end());
        strMessage = addr.ToString(false) + boost::lexical_cast<std::string>(sigTime) +
                        vchPubKey + vchPubKey2 + boost::lexical_cast<std::string>(protocolVersion);

        LogPrint("masternode", "CMasternodeBroadcast::VerifySignature - sanitized strMessage: %s, pubkey address: %s, sig: %s\n",
            SanitizeString(strMessage), CBitcoinAddress(pubkey.GetID()).ToString(),
            EncodeBase64(&vchSig[0], vchSig.size()));

        if(!darkSendSigner.VerifyMessage(pubkey, vchSig, strMessage, strError)) {
            if (addr.ToString() != addr.ToString(false))
            {
                // maybe it's wrong format, try again with the old one
                strMessage = addr.ToString() + boost::lexical_cast<std::string>(sigTime) +
                                vchPubKey + vchPubKey2 + boost::lexical_cast<std::string>(protocolVersion);

                LogPrint("masternode", "CMasternodeBroadcast::VerifySignature - second try, sanitized strMessage: %s, pubkey address: %s, sig: %s\n",
                    SanitizeString(strMessage), CBitcoinAddress(pubkey.GetID()).ToString(),
                    EncodeBase64(&vchSig[0], vchSig.size()));

                if(!darkSendSigner.VerifyMessage(pubkey, vchSig, strMessage, strError)) {
                    // didn't work either
                    LogPrintf("CMasternodeBroadcast::VerifySignature -- Got bad Masternode announce signature, second try, sanitized error: %s\n",
                        SanitizeString(strError));
                    // don't ban for old masternodes, their sigs could be broken because of the bug
                    return false;
                }
            } else {
                // nope, sig is actually wrong
                LogPrintf("CMasternodeBroadcast::VerifySignature -- Got bad Masternode announce signature, sanitized error: %s\n",
                    SanitizeString(strError));
                // don't ban for old masternodes, their sigs could be broken because of the bug
                return false;
            }
        }
    } else {
    //
    // END REMOVE
    //
        strMessage = addr.ToString(false) + boost::lexical_cast<std::string>(sigTime) +
                        pubkey.GetID().ToString() + pubkey2.GetID().ToString() +
                        boost::lexical_cast<std::string>(protocolVersion);

        LogPrint("masternode", "CMasternodeBroadcast::VerifySignature - strMessage: %s, pubkey address: %s, sig: %s\n", strMessage, CBitcoinAddress(pubkey.GetID()).ToString(), EncodeBase64(&vchSig[0], vchSig.size()));

        if(!darkSendSigner.VerifyMessage(pubkey, vchSig, strMessage, strError)){
            LogPrintf("CMasternodeBroadcast::VerifySignature -- Got bad Masternode announce signature, error: %s\n", strError);
            nDos = 100;
            return false;
        }
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

CMasternodePing::CMasternodePing(CTxIn& newVin)
{
    int nHeight;
    {
        LOCK(cs_main);
        CBlockIndex* pindexPrev = chainActive.Tip();
        if(!pindexPrev) return;

        nHeight = pindexPrev->nHeight;
    }

    vin = newVin;
    blockHash = chainActive[nHeight - 12]->GetBlockHash();
    sigTime = GetAdjustedTime();
    vchSig = std::vector<unsigned char>();
}


bool CMasternodePing::Sign(CKey& keyMasternode, CPubKey& pubKeyMasternode)
{
    std::string strError;
    std::string strMasterNodeSignMessage;

    sigTime = GetAdjustedTime();
    std::string strMessage = vin.ToString() + blockHash.ToString() + boost::lexical_cast<std::string>(sigTime);

    if(!darkSendSigner.SignMessage(strMessage, vchSig, keyMasternode)) {
        LogPrintf("CMasternodePing::Sign -- SignMessage() failed\n");
        return false;
    }

    if(!darkSendSigner.VerifyMessage(pubKeyMasternode, vchSig, strMessage, strError)) {
        LogPrintf("CMasternodePing::Sign -- VerifyMessage() failed, error: %s\n", strError);
        return false;
    }

    return true;
}

bool CMasternodePing::VerifySignature(CPubKey& pubKeyMasternode, int &nDos) {
    std::string strMessage = vin.ToString() + blockHash.ToString() + boost::lexical_cast<std::string>(sigTime);
    std::string strError = "";
    nDos = 0;

    if(!darkSendSigner.VerifyMessage(pubKeyMasternode, vchSig, strMessage, strError)) {
        LogPrintf("CMasternodePing::VerifySignature -- Got bad Masternode ping signature: %s  error: %s\n", vin.ToString(), strError);
        nDos = 33;
        return false;
    }
    return true;
}

bool CMasternodePing::CheckAndUpdate(int& nDos, bool fRequireEnabled, bool fCheckSigTimeOnly)
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

    LogPrint("masternode", "CMasternodePing::CheckAndUpdate - New Ping %s - %s %s %d\n", vin.ToString(), GetHash().ToString(), blockHash.ToString(), sigTime);

    // see if we have this Masternode
    CMasternode* pmn = mnodeman.Find(vin);
    if(pmn != NULL && pmn->protocolVersion >= mnpayments.GetMinMasternodePaymentsProto())
    {
        if (fRequireEnabled && !pmn->IsEnabled() && !pmn->IsPreEnabled()) return false;

        // LogPrintf("mnping - Found corresponding mn for vin: %s\n", vin.ToString());
        // update only if there is no known ping for this masternode or
        // last ping was more then MASTERNODE_MIN_MNP_SECONDS-60 ago comparing to this one
        if(!pmn->IsPingedWithin(MASTERNODE_MIN_MNP_SECONDS - 60, sigTime))
        {
            if(!VerifySignature(pmn->pubkey2, nDos))
                return false;

            {
                LOCK(cs_main);
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

void CMasternodePing::Relay()
{
    CInv inv(MSG_MASTERNODE_PING, GetHash());
    RelayInv(inv);
}

void CMasternode::AddGovernanceVote(uint256 nGovernanceObjectHash)
{
    if(mapGovernaceObjectsVotedOn.count(nGovernanceObjectHash))
    {
        mapGovernaceObjectsVotedOn[nGovernanceObjectHash]++;
    } else {
        mapGovernaceObjectsVotedOn.insert(make_pair(nGovernanceObjectHash, 1));
    }
}

/** 
*   FLAG GOVERNANCE ITEMS AS DIRTY
*
*   - When masternode come and go on the network, we must flag the items they voted on to recalc it's cached flags
*
*/

void CMasternode::FlagGovernanceItemsAsDirty()
{
    std::map<uint256, int>::iterator it = mapGovernaceObjectsVotedOn.begin();
    while(it != mapGovernaceObjectsVotedOn.end()){
        CGovernanceObject *pObj = governance.FindGovernanceObject((*it).first);

        if(pObj) pObj->fDirtyCache = true;
        ++it;
    }

}

