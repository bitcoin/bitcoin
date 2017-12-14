// Copyright (c) 2014-2017 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "activemasternode.h"
#include "base58.h"
#include "init.h"
#include "netbase.h"
#include "masternode.h"
#include "masternode-payments.h"
#include "masternode-sync.h"
#include "masternodeman.h"
#include "messagesigner.h"
#include "script/standard.h"
#include "util.h"
#ifdef ENABLE_WALLET
#include "wallet/wallet.h"
#endif // ENABLE_WALLET

#include <boost/lexical_cast.hpp>


CMasternode::CMasternode() :
    masternode_info_t{ MASTERNODE_ENABLED, PROTOCOL_VERSION, GetAdjustedTime()},
    fAllowMixingTx(true)
{}

CMasternode::CMasternode(CService addr, COutPoint outpoint, CPubKey pubKeyCollateralAddress, CPubKey pubKeyMasternode, int nProtocolVersionIn) :
    masternode_info_t{ MASTERNODE_ENABLED, nProtocolVersionIn, GetAdjustedTime(),
                       outpoint, addr, pubKeyCollateralAddress, pubKeyMasternode},
    fAllowMixingTx(true)
{}

CMasternode::CMasternode(const CMasternode& other) :
    masternode_info_t{other},
    lastPing(other.lastPing),
    vchSig(other.vchSig),
    nCollateralMinConfBlockHash(other.nCollateralMinConfBlockHash),
    nBlockLastPaid(other.nBlockLastPaid),
    nPoSeBanScore(other.nPoSeBanScore),
    nPoSeBanHeight(other.nPoSeBanHeight),
    fAllowMixingTx(other.fAllowMixingTx),
    fUnitTest(other.fUnitTest)
{}

CMasternode::CMasternode(const CMasternodeBroadcast& mnb) :
    masternode_info_t{ mnb.nActiveState, mnb.nProtocolVersion, mnb.sigTime,
                       mnb.vin.prevout, mnb.addr, mnb.pubKeyCollateralAddress, mnb.pubKeyMasternode,
                       mnb.sigTime /*nTimeLastWatchdogVote*/},
    lastPing(mnb.lastPing),
    vchSig(mnb.vchSig),
    fAllowMixingTx(true)
{}

//
// When a new masternode broadcast is sent, update our information
//
bool CMasternode::UpdateFromNewBroadcast(CMasternodeBroadcast& mnb, CConnman& connman)
{
    if(mnb.sigTime <= sigTime && !mnb.fRecovery) return false;

    pubKeyMasternode = mnb.pubKeyMasternode;
    sigTime = mnb.sigTime;
    vchSig = mnb.vchSig;
    nProtocolVersion = mnb.nProtocolVersion;
    addr = mnb.addr;
    nPoSeBanScore = 0;
    nPoSeBanHeight = 0;
    nTimeLastChecked = 0;
    int nDos = 0;
    if(mnb.lastPing == CMasternodePing() || (mnb.lastPing != CMasternodePing() && mnb.lastPing.CheckAndUpdate(this, true, nDos, connman))) {
        lastPing = mnb.lastPing;
        mnodeman.mapSeenMasternodePing.insert(std::make_pair(lastPing.GetHash(), lastPing));
    }
    // if it matches our Masternode privkey...
    if(fMasterNode && pubKeyMasternode == activeMasternode.pubKeyMasternode) {
        nPoSeBanScore = -MASTERNODE_POSE_BAN_MAX_SCORE;
        if(nProtocolVersion == PROTOCOL_VERSION) {
            // ... and PROTOCOL_VERSION, then we've been remotely activated ...
            activeMasternode.ManageState(connman);
        } else {
            // ... otherwise we need to reactivate our node, do not add it to the list and do not relay
            // but also do not ban the node we get this message from
            LogPrintf("CMasternode::UpdateFromNewBroadcast -- wrong PROTOCOL_VERSION, re-activate your MN: message nProtocolVersion=%d  PROTOCOL_VERSION=%d\n", nProtocolVersion, PROTOCOL_VERSION);
            return false;
        }
    }
    return true;
}

//
// Deterministically calculate a given "score" for a Masternode depending on how close it's hash is to
// the proof of work for that block. The further away they are the better, the furthest will win the election
// and get paid this block
//
arith_uint256 CMasternode::CalculateScore(const uint256& blockHash)
{
    // Deterministically calculate a "score" for a Masternode based on any given (block)hash
    CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
    ss << vin.prevout << nCollateralMinConfBlockHash << blockHash;
    return UintToArith256(ss.GetHash());
}

CMasternode::CollateralStatus CMasternode::CheckCollateral(const COutPoint& outpoint)
{
    int nHeight;
    return CheckCollateral(outpoint, nHeight);
}

CMasternode::CollateralStatus CMasternode::CheckCollateral(const COutPoint& outpoint, int& nHeightRet)
{
    AssertLockHeld(cs_main);

    Coin coin;
    if(!GetUTXOCoin(outpoint, coin)) {
        return COLLATERAL_UTXO_NOT_FOUND;
    }

    if(coin.out.nValue != 1000 * COIN) {
        return COLLATERAL_INVALID_AMOUNT;
    }

    nHeightRet = coin.nHeight;
    return COLLATERAL_OK;
}

void CMasternode::Check(bool fForce)
{
    LOCK(cs);

    if(ShutdownRequested()) return;

    if(!fForce && (GetTime() - nTimeLastChecked < MASTERNODE_CHECK_SECONDS)) return;
    nTimeLastChecked = GetTime();

    LogPrint("masternode", "CMasternode::Check -- Masternode %s is in %s state\n", vin.prevout.ToStringShort(), GetStateString());

    //once spent, stop doing the checks
    if(IsOutpointSpent()) return;

    int nHeight = 0;
    if(!fUnitTest) {
        TRY_LOCK(cs_main, lockMain);
        if(!lockMain) return;

        CollateralStatus err = CheckCollateral(vin.prevout);
        if (err == COLLATERAL_UTXO_NOT_FOUND) {
            nActiveState = MASTERNODE_OUTPOINT_SPENT;
            LogPrint("masternode", "CMasternode::Check -- Failed to find Masternode UTXO, masternode=%s\n", vin.prevout.ToStringShort());
            return;
        }

        nHeight = chainActive.Height();
    }

    if(IsPoSeBanned()) {
        if(nHeight < nPoSeBanHeight) return; // too early?
        // Otherwise give it a chance to proceed further to do all the usual checks and to change its state.
        // Masternode still will be on the edge and can be banned back easily if it keeps ignoring mnverify
        // or connect attempts. Will require few mnverify messages to strengthen its position in mn list.
        LogPrintf("CMasternode::Check -- Masternode %s is unbanned and back in list now\n", vin.prevout.ToStringShort());
        DecreasePoSeBanScore();
    } else if(nPoSeBanScore >= MASTERNODE_POSE_BAN_MAX_SCORE) {
        nActiveState = MASTERNODE_POSE_BAN;
        // ban for the whole payment cycle
        nPoSeBanHeight = nHeight + mnodeman.size();
        LogPrintf("CMasternode::Check -- Masternode %s is banned till block %d now\n", vin.prevout.ToStringShort(), nPoSeBanHeight);
        return;
    }

    int nActiveStatePrev = nActiveState;
    bool fOurMasternode = fMasterNode && activeMasternode.pubKeyMasternode == pubKeyMasternode;

                   // masternode doesn't meet payment protocol requirements ...
    bool fRequireUpdate = nProtocolVersion < mnpayments.GetMinMasternodePaymentsProto() ||
                   // or it's our own node and we just updated it to the new protocol but we are still waiting for activation ...
                   (fOurMasternode && nProtocolVersion < PROTOCOL_VERSION);

    if(fRequireUpdate) {
        nActiveState = MASTERNODE_UPDATE_REQUIRED;
        if(nActiveStatePrev != nActiveState) {
            LogPrint("masternode", "CMasternode::Check -- Masternode %s is in %s state now\n", vin.prevout.ToStringShort(), GetStateString());
        }
        return;
    }

    // keep old masternodes on start, give them a chance to receive updates...
    bool fWaitForPing = !masternodeSync.IsMasternodeListSynced() && !IsPingedWithin(MASTERNODE_MIN_MNP_SECONDS);

    if(fWaitForPing && !fOurMasternode) {
        // ...but if it was already expired before the initial check - return right away
        if(IsExpired() || IsWatchdogExpired() || IsNewStartRequired()) {
            LogPrint("masternode", "CMasternode::Check -- Masternode %s is in %s state, waiting for ping\n", vin.prevout.ToStringShort(), GetStateString());
            return;
        }
    }

    // don't expire if we are still in "waiting for ping" mode unless it's our own masternode
    if(!fWaitForPing || fOurMasternode) {

        if(!IsPingedWithin(MASTERNODE_NEW_START_REQUIRED_SECONDS)) {
            nActiveState = MASTERNODE_NEW_START_REQUIRED;
            if(nActiveStatePrev != nActiveState) {
                LogPrint("masternode", "CMasternode::Check -- Masternode %s is in %s state now\n", vin.prevout.ToStringShort(), GetStateString());
            }
            return;
        }

        bool fWatchdogActive = masternodeSync.IsSynced() && mnodeman.IsWatchdogActive();
        bool fWatchdogExpired = (fWatchdogActive && ((GetAdjustedTime() - nTimeLastWatchdogVote) > MASTERNODE_WATCHDOG_MAX_SECONDS));

        LogPrint("masternode", "CMasternode::Check -- outpoint=%s, nTimeLastWatchdogVote=%d, GetAdjustedTime()=%d, fWatchdogExpired=%d\n",
                vin.prevout.ToStringShort(), nTimeLastWatchdogVote, GetAdjustedTime(), fWatchdogExpired);

        if(fWatchdogExpired) {
            nActiveState = MASTERNODE_WATCHDOG_EXPIRED;
            if(nActiveStatePrev != nActiveState) {
                LogPrint("masternode", "CMasternode::Check -- Masternode %s is in %s state now\n", vin.prevout.ToStringShort(), GetStateString());
            }
            return;
        }

        if(!IsPingedWithin(MASTERNODE_EXPIRATION_SECONDS)) {
            nActiveState = MASTERNODE_EXPIRED;
            if(nActiveStatePrev != nActiveState) {
                LogPrint("masternode", "CMasternode::Check -- Masternode %s is in %s state now\n", vin.prevout.ToStringShort(), GetStateString());
            }
            return;
        }
    }

    if(lastPing.sigTime - sigTime < MASTERNODE_MIN_MNP_SECONDS) {
        nActiveState = MASTERNODE_PRE_ENABLED;
        if(nActiveStatePrev != nActiveState) {
            LogPrint("masternode", "CMasternode::Check -- Masternode %s is in %s state now\n", vin.prevout.ToStringShort(), GetStateString());
        }
        return;
    }

    nActiveState = MASTERNODE_ENABLED; // OK
    if(nActiveStatePrev != nActiveState) {
        LogPrint("masternode", "CMasternode::Check -- Masternode %s is in %s state now\n", vin.prevout.ToStringShort(), GetStateString());
    }
}

bool CMasternode::IsInputAssociatedWithPubkey()
{
    CScript payee;
    payee = GetScriptForDestination(pubKeyCollateralAddress.GetID());

    CTransaction tx;
    uint256 hash;
    if(GetTransaction(vin.prevout.hash, tx, Params().GetConsensus(), hash, true)) {
        BOOST_FOREACH(CTxOut out, tx.vout)
            if(out.nValue == 1000*COIN && out.scriptPubKey == payee) return true;
    }

    return false;
}

bool CMasternode::IsValidNetAddr()
{
    return IsValidNetAddr(addr);
}

bool CMasternode::IsValidNetAddr(CService addrIn)
{
    // TODO: regtest is fine with any addresses for now,
    // should probably be a bit smarter if one day we start to implement tests for this
    return Params().NetworkIDString() == CBaseChainParams::REGTEST ||
            (addrIn.IsIPv4() && IsReachable(addrIn) && addrIn.IsRoutable());
}

masternode_info_t CMasternode::GetInfo()
{
    masternode_info_t info{*this};
    info.nTimeLastPing = lastPing.sigTime;
    info.fInfoValid = true;
    return info;
}

std::string CMasternode::StateToString(int nStateIn)
{
    switch(nStateIn) {
        case MASTERNODE_PRE_ENABLED:            return "PRE_ENABLED";
        case MASTERNODE_ENABLED:                return "ENABLED";
        case MASTERNODE_EXPIRED:                return "EXPIRED";
        case MASTERNODE_OUTPOINT_SPENT:         return "OUTPOINT_SPENT";
        case MASTERNODE_UPDATE_REQUIRED:        return "UPDATE_REQUIRED";
        case MASTERNODE_WATCHDOG_EXPIRED:       return "WATCHDOG_EXPIRED";
        case MASTERNODE_NEW_START_REQUIRED:     return "NEW_START_REQUIRED";
        case MASTERNODE_POSE_BAN:               return "POSE_BAN";
        default:                                return "UNKNOWN";
    }
}

std::string CMasternode::GetStateString() const
{
    return StateToString(nActiveState);
}

std::string CMasternode::GetStatus() const
{
    // TODO: return smth a bit more human readable here
    return GetStateString();
}

void CMasternode::UpdateLastPaid(const CBlockIndex *pindex, int nMaxBlocksToScanBack)
{
    if(!pindex) return;

    const CBlockIndex *BlockReading = pindex;

    CScript mnpayee = GetScriptForDestination(pubKeyCollateralAddress.GetID());
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

#ifdef ENABLE_WALLET
bool CMasternodeBroadcast::Create(std::string strService, std::string strKeyMasternode, std::string strTxHash, std::string strOutputIndex, std::string& strErrorRet, CMasternodeBroadcast &mnbRet, bool fOffline)
{
    COutPoint outpoint;
    CPubKey pubKeyCollateralAddressNew;
    CKey keyCollateralAddressNew;
    CPubKey pubKeyMasternodeNew;
    CKey keyMasternodeNew;

    auto Log = [&strErrorRet](std::string sErr)->bool
    {
        strErrorRet = sErr;
        LogPrintf("CMasternodeBroadcast::Create -- %s\n", strErrorRet);
        return false;
    };

    //need correct blocks to send ping
    if (!fOffline && !masternodeSync.IsBlockchainSynced())
        return Log("Sync in progress. Must wait until sync is complete to start Masternode");

    if (!CMessageSigner::GetKeysFromSecret(strKeyMasternode, keyMasternodeNew, pubKeyMasternodeNew))
        return Log(strprintf("Invalid masternode key %s", strKeyMasternode));

    if (!pwalletMain->GetMasternodeOutpointAndKeys(outpoint, pubKeyCollateralAddressNew, keyCollateralAddressNew, strTxHash, strOutputIndex))
        return Log(strprintf("Could not allocate outpoint %s:%s for masternode %s", strTxHash, strOutputIndex, strService));

    CService service;
    if (!Lookup(strService.c_str(), service, 0, false))
        return Log(strprintf("Invalid address %s for masternode.", strService));
    int mainnetDefaultPort = Params(CBaseChainParams::MAIN).GetDefaultPort();
    if (Params().NetworkIDString() == CBaseChainParams::MAIN) {
        if (service.GetPort() != mainnetDefaultPort)
            return Log(strprintf("Invalid port %u for masternode %s, only %d is supported on mainnet.", service.GetPort(), strService, mainnetDefaultPort));
    } else if (service.GetPort() == mainnetDefaultPort)
        return Log(strprintf("Invalid port %u for masternode %s, %d is the only supported on mainnet.", service.GetPort(), strService, mainnetDefaultPort));

    return Create(outpoint, service, keyCollateralAddressNew, pubKeyCollateralAddressNew, keyMasternodeNew, pubKeyMasternodeNew, strErrorRet, mnbRet);
}

bool CMasternodeBroadcast::Create(const COutPoint& outpoint, const CService& service, const CKey& keyCollateralAddressNew, const CPubKey& pubKeyCollateralAddressNew, const CKey& keyMasternodeNew, const CPubKey& pubKeyMasternodeNew, std::string &strErrorRet, CMasternodeBroadcast &mnbRet)
{
    // wait for reindex and/or import to finish
    if (fImporting || fReindex) return false;

    LogPrint("masternode", "CMasternodeBroadcast::Create -- pubKeyCollateralAddressNew = %s, pubKeyMasternodeNew.GetID() = %s\n",
             CBitcoinAddress(pubKeyCollateralAddressNew.GetID()).ToString(),
             pubKeyMasternodeNew.GetID().ToString());

    auto Log = [&strErrorRet,&mnbRet](std::string sErr)->bool
    {
        strErrorRet = sErr;
        LogPrintf("CMasternodeBroadcast::Create -- %s\n", strErrorRet);
        mnbRet = CMasternodeBroadcast();
        return false;
    };

    CMasternodePing mnp(outpoint);
    if (!mnp.Sign(keyMasternodeNew, pubKeyMasternodeNew))
        return Log(strprintf("Failed to sign ping, masternode=%s", outpoint.ToStringShort()));

    mnbRet = CMasternodeBroadcast(service, outpoint, pubKeyCollateralAddressNew, pubKeyMasternodeNew, PROTOCOL_VERSION);

    if (!mnbRet.IsValidNetAddr())
        return Log(strprintf("Invalid IP address, masternode=%s", outpoint.ToStringShort()));

    mnbRet.lastPing = mnp;
    if (!mnbRet.Sign(keyCollateralAddressNew))
        return Log(strprintf("Failed to sign broadcast, masternode=%s", outpoint.ToStringShort()));

    return true;
}
#endif // ENABLE_WALLET

bool CMasternodeBroadcast::SimpleCheck(int& nDos)
{
    nDos = 0;

    // make sure addr is valid
    if(!IsValidNetAddr()) {
        LogPrintf("CMasternodeBroadcast::SimpleCheck -- Invalid addr, rejected: masternode=%s  addr=%s\n",
                    vin.prevout.ToStringShort(), addr.ToString());
        return false;
    }

    // make sure signature isn't in the future (past is OK)
    if (sigTime > GetAdjustedTime() + 60 * 60) {
        LogPrintf("CMasternodeBroadcast::SimpleCheck -- Signature rejected, too far into the future: masternode=%s\n", vin.prevout.ToStringShort());
        nDos = 1;
        return false;
    }

    // empty ping or incorrect sigTime/unknown blockhash
    if(lastPing == CMasternodePing() || !lastPing.SimpleCheck(nDos)) {
        // one of us is probably forked or smth, just mark it as expired and check the rest of the rules
        nActiveState = MASTERNODE_EXPIRED;
    }

    if(nProtocolVersion < mnpayments.GetMinMasternodePaymentsProto()) {
        LogPrintf("CMasternodeBroadcast::SimpleCheck -- ignoring outdated Masternode: masternode=%s  nProtocolVersion=%d\n", vin.prevout.ToStringShort(), nProtocolVersion);
        return false;
    }

    CScript pubkeyScript;
    pubkeyScript = GetScriptForDestination(pubKeyCollateralAddress.GetID());

    if(pubkeyScript.size() != 25) {
        LogPrintf("CMasternodeBroadcast::SimpleCheck -- pubKeyCollateralAddress has the wrong size\n");
        nDos = 100;
        return false;
    }

    CScript pubkeyScript2;
    pubkeyScript2 = GetScriptForDestination(pubKeyMasternode.GetID());

    if(pubkeyScript2.size() != 25) {
        LogPrintf("CMasternodeBroadcast::SimpleCheck -- pubKeyMasternode has the wrong size\n");
        nDos = 100;
        return false;
    }

    if(!vin.scriptSig.empty()) {
        LogPrintf("CMasternodeBroadcast::SimpleCheck -- Ignore Not Empty ScriptSig %s\n",vin.ToString());
        nDos = 100;
        return false;
    }

    int mainnetDefaultPort = Params(CBaseChainParams::MAIN).GetDefaultPort();
    if(Params().NetworkIDString() == CBaseChainParams::MAIN) {
        if(addr.GetPort() != mainnetDefaultPort) return false;
    } else if(addr.GetPort() == mainnetDefaultPort) return false;

    return true;
}

bool CMasternodeBroadcast::Update(CMasternode* pmn, int& nDos, CConnman& connman)
{
    nDos = 0;

    if(pmn->sigTime == sigTime && !fRecovery) {
        // mapSeenMasternodeBroadcast in CMasternodeMan::CheckMnbAndUpdateMasternodeList should filter legit duplicates
        // but this still can happen if we just started, which is ok, just do nothing here.
        return false;
    }

    // this broadcast is older than the one that we already have - it's bad and should never happen
    // unless someone is doing something fishy
    if(pmn->sigTime > sigTime) {
        LogPrintf("CMasternodeBroadcast::Update -- Bad sigTime %d (existing broadcast is at %d) for Masternode %s %s\n",
                      sigTime, pmn->sigTime, vin.prevout.ToStringShort(), addr.ToString());
        return false;
    }

    pmn->Check();

    // masternode is banned by PoSe
    if(pmn->IsPoSeBanned()) {
        LogPrintf("CMasternodeBroadcast::Update -- Banned by PoSe, masternode=%s\n", vin.prevout.ToStringShort());
        return false;
    }

    // IsVnAssociatedWithPubkey is validated once in CheckOutpoint, after that they just need to match
    if(pmn->pubKeyCollateralAddress != pubKeyCollateralAddress) {
        LogPrintf("CMasternodeBroadcast::Update -- Got mismatched pubKeyCollateralAddress and vin\n");
        nDos = 33;
        return false;
    }

    if (!CheckSignature(nDos)) {
        LogPrintf("CMasternodeBroadcast::Update -- CheckSignature() failed, masternode=%s\n", vin.prevout.ToStringShort());
        return false;
    }

    // if ther was no masternode broadcast recently or if it matches our Masternode privkey...
    if(!pmn->IsBroadcastedWithin(MASTERNODE_MIN_MNB_SECONDS) || (fMasterNode && pubKeyMasternode == activeMasternode.pubKeyMasternode)) {
        // take the newest entry
        LogPrintf("CMasternodeBroadcast::Update -- Got UPDATED Masternode entry: addr=%s\n", addr.ToString());
        if(pmn->UpdateFromNewBroadcast(*this, connman)) {
            pmn->Check();
            Relay(connman);
        }
        masternodeSync.BumpAssetLastTime("CMasternodeBroadcast::Update");
    }

    return true;
}

bool CMasternodeBroadcast::CheckOutpoint(int& nDos)
{
    // we are a masternode with the same vin (i.e. already activated) and this mnb is ours (matches our Masternode privkey)
    // so nothing to do here for us
    if(fMasterNode && vin.prevout == activeMasternode.outpoint && pubKeyMasternode == activeMasternode.pubKeyMasternode) {
        return false;
    }

    if (!CheckSignature(nDos)) {
        LogPrintf("CMasternodeBroadcast::CheckOutpoint -- CheckSignature() failed, masternode=%s\n", vin.prevout.ToStringShort());
        return false;
    }

    {
        TRY_LOCK(cs_main, lockMain);
        if(!lockMain) {
            // not mnb fault, let it to be checked again later
            LogPrint("masternode", "CMasternodeBroadcast::CheckOutpoint -- Failed to aquire lock, addr=%s", addr.ToString());
            mnodeman.mapSeenMasternodeBroadcast.erase(GetHash());
            return false;
        }

        int nHeight;
        CollateralStatus err = CheckCollateral(vin.prevout, nHeight);
        if (err == COLLATERAL_UTXO_NOT_FOUND) {
            LogPrint("masternode", "CMasternodeBroadcast::CheckOutpoint -- Failed to find Masternode UTXO, masternode=%s\n", vin.prevout.ToStringShort());
            return false;
        }

        if (err == COLLATERAL_INVALID_AMOUNT) {
            LogPrint("masternode", "CMasternodeBroadcast::CheckOutpoint -- Masternode UTXO should have 1000 DASH, masternode=%s\n", vin.prevout.ToStringShort());
            return false;
        }

        if(chainActive.Height() - nHeight + 1 < Params().GetConsensus().nMasternodeMinimumConfirmations) {
            LogPrintf("CMasternodeBroadcast::CheckOutpoint -- Masternode UTXO must have at least %d confirmations, masternode=%s\n",
                    Params().GetConsensus().nMasternodeMinimumConfirmations, vin.prevout.ToStringShort());
            // maybe we miss few blocks, let this mnb to be checked again later
            mnodeman.mapSeenMasternodeBroadcast.erase(GetHash());
            return false;
        }
        // remember the hash of the block where masternode collateral had minimum required confirmations
        nCollateralMinConfBlockHash = chainActive[nHeight + Params().GetConsensus().nMasternodeMinimumConfirmations - 1]->GetBlockHash();
    }

    LogPrint("masternode", "CMasternodeBroadcast::CheckOutpoint -- Masternode UTXO verified\n");

    // make sure the input that was signed in masternode broadcast message is related to the transaction
    // that spawned the Masternode - this is expensive, so it's only done once per Masternode
    if(!IsInputAssociatedWithPubkey()) {
        LogPrintf("CMasternodeMan::CheckOutpoint -- Got mismatched pubKeyCollateralAddress and vin\n");
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
            if(pConfIndex->GetBlockTime() > sigTime) {
                LogPrintf("CMasternodeBroadcast::CheckOutpoint -- Bad sigTime %d (%d conf block is at %d) for Masternode %s %s\n",
                          sigTime, Params().GetConsensus().nMasternodeMinimumConfirmations, pConfIndex->GetBlockTime(), vin.prevout.ToStringShort(), addr.ToString());
                return false;
            }
        }
    }

    return true;
}

bool CMasternodeBroadcast::Sign(const CKey& keyCollateralAddress)
{
    std::string strError;
    std::string strMessage;

    sigTime = GetAdjustedTime();

    strMessage = addr.ToString(false) + boost::lexical_cast<std::string>(sigTime) +
                    pubKeyCollateralAddress.GetID().ToString() + pubKeyMasternode.GetID().ToString() +
                    boost::lexical_cast<std::string>(nProtocolVersion);

    if(!CMessageSigner::SignMessage(strMessage, vchSig, keyCollateralAddress)) {
        LogPrintf("CMasternodeBroadcast::Sign -- SignMessage() failed\n");
        return false;
    }

    if(!CMessageSigner::VerifyMessage(pubKeyCollateralAddress, vchSig, strMessage, strError)) {
        LogPrintf("CMasternodeBroadcast::Sign -- VerifyMessage() failed, error: %s\n", strError);
        return false;
    }

    return true;
}

bool CMasternodeBroadcast::CheckSignature(int& nDos)
{
    std::string strMessage;
    std::string strError = "";
    nDos = 0;

    strMessage = addr.ToString(false) + boost::lexical_cast<std::string>(sigTime) +
                    pubKeyCollateralAddress.GetID().ToString() + pubKeyMasternode.GetID().ToString() +
                    boost::lexical_cast<std::string>(nProtocolVersion);

    LogPrint("masternode", "CMasternodeBroadcast::CheckSignature -- strMessage: %s  pubKeyCollateralAddress address: %s  sig: %s\n", strMessage, CBitcoinAddress(pubKeyCollateralAddress.GetID()).ToString(), EncodeBase64(&vchSig[0], vchSig.size()));

    if(!CMessageSigner::VerifyMessage(pubKeyCollateralAddress, vchSig, strMessage, strError)){
        LogPrintf("CMasternodeBroadcast::CheckSignature -- Got bad Masternode announce signature, error: %s\n", strError);
        nDos = 100;
        return false;
    }

    return true;
}

void CMasternodeBroadcast::Relay(CConnman& connman)
{
    // Do not relay until fully synced
    if(!masternodeSync.IsSynced()) {
        LogPrint("masternode", "CMasternodeBroadcast::Relay -- won't relay until fully synced\n");
        return;
    }

    CInv inv(MSG_MASTERNODE_ANNOUNCE, GetHash());
    connman.RelayInv(inv);
}

CMasternodePing::CMasternodePing(const COutPoint& outpoint)
{
    LOCK(cs_main);
    if (!chainActive.Tip() || chainActive.Height() < 12) return;

    vin = CTxIn(outpoint);
    blockHash = chainActive[chainActive.Height() - 12]->GetBlockHash();
    sigTime = GetAdjustedTime();
}

bool CMasternodePing::Sign(const CKey& keyMasternode, const CPubKey& pubKeyMasternode)
{
    std::string strError;
    std::string strMasterNodeSignMessage;

    // TODO: add sentinel data
    sigTime = GetAdjustedTime();
    std::string strMessage = vin.ToString() + blockHash.ToString() + boost::lexical_cast<std::string>(sigTime);

    if(!CMessageSigner::SignMessage(strMessage, vchSig, keyMasternode)) {
        LogPrintf("CMasternodePing::Sign -- SignMessage() failed\n");
        return false;
    }

    if(!CMessageSigner::VerifyMessage(pubKeyMasternode, vchSig, strMessage, strError)) {
        LogPrintf("CMasternodePing::Sign -- VerifyMessage() failed, error: %s\n", strError);
        return false;
    }

    return true;
}

bool CMasternodePing::CheckSignature(CPubKey& pubKeyMasternode, int &nDos)
{
    // TODO: add sentinel data
    std::string strMessage = vin.ToString() + blockHash.ToString() + boost::lexical_cast<std::string>(sigTime);
    std::string strError = "";
    nDos = 0;

    if(!CMessageSigner::VerifyMessage(pubKeyMasternode, vchSig, strMessage, strError)) {
        LogPrintf("CMasternodePing::CheckSignature -- Got bad Masternode ping signature, masternode=%s, error: %s\n", vin.prevout.ToStringShort(), strError);
        nDos = 33;
        return false;
    }
    return true;
}

bool CMasternodePing::SimpleCheck(int& nDos)
{
    // don't ban by default
    nDos = 0;

    if (sigTime > GetAdjustedTime() + 60 * 60) {
        LogPrintf("CMasternodePing::SimpleCheck -- Signature rejected, too far into the future, masternode=%s\n", vin.prevout.ToStringShort());
        nDos = 1;
        return false;
    }

    {
        AssertLockHeld(cs_main);
        BlockMap::iterator mi = mapBlockIndex.find(blockHash);
        if (mi == mapBlockIndex.end()) {
            LogPrint("masternode", "CMasternodePing::SimpleCheck -- Masternode ping is invalid, unknown block hash: masternode=%s blockHash=%s\n", vin.prevout.ToStringShort(), blockHash.ToString());
            // maybe we stuck or forked so we shouldn't ban this node, just fail to accept this ping
            // TODO: or should we also request this block?
            return false;
        }
    }
    LogPrint("masternode", "CMasternodePing::SimpleCheck -- Masternode ping verified: masternode=%s  blockHash=%s  sigTime=%d\n", vin.prevout.ToStringShort(), blockHash.ToString(), sigTime);
    return true;
}

bool CMasternodePing::CheckAndUpdate(CMasternode* pmn, bool fFromNewBroadcast, int& nDos, CConnman& connman)
{
    // don't ban by default
    nDos = 0;

    if (!SimpleCheck(nDos)) {
        return false;
    }

    if (pmn == NULL) {
        LogPrint("masternode", "CMasternodePing::CheckAndUpdate -- Couldn't find Masternode entry, masternode=%s\n", vin.prevout.ToStringShort());
        return false;
    }

    if(!fFromNewBroadcast) {
        if (pmn->IsUpdateRequired()) {
            LogPrint("masternode", "CMasternodePing::CheckAndUpdate -- masternode protocol is outdated, masternode=%s\n", vin.prevout.ToStringShort());
            return false;
        }

        if (pmn->IsNewStartRequired()) {
            LogPrint("masternode", "CMasternodePing::CheckAndUpdate -- masternode is completely expired, new start is required, masternode=%s\n", vin.prevout.ToStringShort());
            return false;
        }
    }

    {
        LOCK(cs_main);
        BlockMap::iterator mi = mapBlockIndex.find(blockHash);
        if ((*mi).second && (*mi).second->nHeight < chainActive.Height() - 24) {
            LogPrintf("CMasternodePing::CheckAndUpdate -- Masternode ping is invalid, block hash is too old: masternode=%s  blockHash=%s\n", vin.prevout.ToStringShort(), blockHash.ToString());
            // nDos = 1;
            return false;
        }
    }

    LogPrint("masternode", "CMasternodePing::CheckAndUpdate -- New ping: masternode=%s  blockHash=%s  sigTime=%d\n", vin.prevout.ToStringShort(), blockHash.ToString(), sigTime);

    // LogPrintf("mnping - Found corresponding mn for vin: %s\n", vin.prevout.ToStringShort());
    // update only if there is no known ping for this masternode or
    // last ping was more then MASTERNODE_MIN_MNP_SECONDS-60 ago comparing to this one
    if (pmn->IsPingedWithin(MASTERNODE_MIN_MNP_SECONDS - 60, sigTime)) {
        LogPrint("masternode", "CMasternodePing::CheckAndUpdate -- Masternode ping arrived too early, masternode=%s\n", vin.prevout.ToStringShort());
        //nDos = 1; //disable, this is happening frequently and causing banned peers
        return false;
    }

    if (!CheckSignature(pmn->pubKeyMasternode, nDos)) return false;

    // so, ping seems to be ok

    // if we are still syncing and there was no known ping for this mn for quite a while
    // (NOTE: assuming that MASTERNODE_EXPIRATION_SECONDS/2 should be enough to finish mn list sync)
    if(!masternodeSync.IsMasternodeListSynced() && !pmn->IsPingedWithin(MASTERNODE_EXPIRATION_SECONDS/2)) {
        // let's bump sync timeout
        LogPrint("masternode", "CMasternodePing::CheckAndUpdate -- bumping sync timeout, masternode=%s\n", vin.prevout.ToStringShort());
        masternodeSync.BumpAssetLastTime("CMasternodePing::CheckAndUpdate");
    }

    // let's store this ping as the last one
    LogPrint("masternode", "CMasternodePing::CheckAndUpdate -- Masternode ping accepted, masternode=%s\n", vin.prevout.ToStringShort());
    pmn->lastPing = *this;

    // and update mnodeman.mapSeenMasternodeBroadcast.lastPing which is probably outdated
    CMasternodeBroadcast mnb(*pmn);
    uint256 hash = mnb.GetHash();
    if (mnodeman.mapSeenMasternodeBroadcast.count(hash)) {
        mnodeman.mapSeenMasternodeBroadcast[hash].second.lastPing = *this;
    }

    // force update, ignoring cache
    pmn->Check(true);
    // relay ping for nodes in ENABLED/EXPIRED/WATCHDOG_EXPIRED state only, skip everyone else
    if (!pmn->IsEnabled() && !pmn->IsExpired() && !pmn->IsWatchdogExpired()) return false;

    LogPrint("masternode", "CMasternodePing::CheckAndUpdate -- Masternode ping acceepted and relayed, masternode=%s\n", vin.prevout.ToStringShort());
    Relay(connman);

    return true;
}

void CMasternodePing::Relay(CConnman& connman)
{
    // Do not relay until fully synced
    if(!masternodeSync.IsSynced()) {
        LogPrint("masternode", "CMasternodePing::Relay -- won't relay until fully synced\n");
        return;
    }

    CInv inv(MSG_MASTERNODE_PING, GetHash());
    connman.RelayInv(inv);
}

void CMasternode::AddGovernanceVote(uint256 nGovernanceObjectHash)
{
    if(mapGovernanceObjectsVotedOn.count(nGovernanceObjectHash)) {
        mapGovernanceObjectsVotedOn[nGovernanceObjectHash]++;
    } else {
        mapGovernanceObjectsVotedOn.insert(std::make_pair(nGovernanceObjectHash, 1));
    }
}

void CMasternode::RemoveGovernanceObject(uint256 nGovernanceObjectHash)
{
    std::map<uint256, int>::iterator it = mapGovernanceObjectsVotedOn.find(nGovernanceObjectHash);
    if(it == mapGovernanceObjectsVotedOn.end()) {
        return;
    }
    mapGovernanceObjectsVotedOn.erase(it);
}

void CMasternode::UpdateWatchdogVoteTime(uint64_t nVoteTime)
{
    LOCK(cs);
    nTimeLastWatchdogVote = (nVoteTime == 0) ? GetAdjustedTime() : nVoteTime;
}

/**
*   FLAG GOVERNANCE ITEMS AS DIRTY
*
*   - When masternode come and go on the network, we must flag the items they voted on to recalc it's cached flags
*
*/
void CMasternode::FlagGovernanceItemsAsDirty()
{
    std::vector<uint256> vecDirty;
    {
        std::map<uint256, int>::iterator it = mapGovernanceObjectsVotedOn.begin();
        while(it != mapGovernanceObjectsVotedOn.end()) {
            vecDirty.push_back(it->first);
            ++it;
        }
    }
    for(size_t i = 0; i < vecDirty.size(); ++i) {
        mnodeman.AddDirtyGovernanceObjectHash(vecDirty[i]);
    }
}
