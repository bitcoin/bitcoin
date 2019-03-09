// Copyright (c) 2014-2017 The Dash Core developers
// Copyright (c) 2017-2018 The Syscoin Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "activemasternode.h"
#include "base58.h"
#include "clientversion.h"
#include "init.h"
#include "netbase.h"
#include "masternode.h"
#include "masternode-payments.h"
#include "masternode-sync.h"
#include "masternodeman.h"
#include "messagesigner.h"
#include "script/standard.h"
#include "util.h"
#include <shutdown.h>
#include <key_io.h>
#ifdef ENABLE_WALLET
#include "wallet/wallet.h"
#endif // ENABLE_WALLET
#include <boost/lexical_cast.hpp>
#include <outputtype.h>

CMasternode::CMasternode() :
    masternode_info_t{ MASTERNODE_ENABLED, MIN_PEER_PROTO_VERSION, GetAdjustedTime(), 0}
{}

CMasternode::CMasternode(CService addr, COutPoint outpoint, CPubKey pubKeyCollateralAddress, CPubKey pubKeyMasternode, int nProtocolVersionIn, int retries) :
    masternode_info_t{ MASTERNODE_ENABLED, nProtocolVersionIn, GetAdjustedTime(),
                       outpoint, addr, pubKeyCollateralAddress, pubKeyMasternode, retries}
{}

CMasternode::CMasternode(const CMasternode& other) :
    masternode_info_t{other},
    lastPing(other.lastPing),
    vchSig(other.vchSig),
    nCollateralMinConfBlockHash(other.nCollateralMinConfBlockHash),
    nBlockLastPaid(other.nBlockLastPaid),
    nPoSeBanScore(other.nPoSeBanScore),
    nPoSeBanHeight(other.nPoSeBanHeight)
{}

CMasternode::CMasternode(const CMasternodeBroadcast& mnb) :
    masternode_info_t{ mnb.nActiveState, mnb.nProtocolVersion, mnb.sigTime,
                       mnb.outpoint, mnb.addr, mnb.pubKeyCollateralAddress, mnb.pubKeyMasternode, mnb.nPingRetries},
    lastPing(mnb.lastPing),
    vchSig(mnb.vchSig)
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
    nPingRetries = mnb.nPingRetries;
    int nDos = 0;
    if(!mnb.lastPing || (mnb.lastPing && mnb.lastPing.CheckAndUpdate(this, true, nDos, connman))) {
        lastPing = mnb.lastPing;
        mnodeman.mapSeenMasternodePing.insert(std::make_pair(lastPing.GetHash(), lastPing));
    }
    // if it matches our Masternode privkey...
    if(fMasternodeMode && pubKeyMasternode == activeMasternode.pubKeyMasternode) {
        nPoSeBanScore = -MASTERNODE_POSE_BAN_MAX_SCORE;
        if(nProtocolVersion == MIN_PEER_PROTO_VERSION) {
            // ... and MIN_PEER_PROTO_VERSION, then we've been remotely activated ...
            activeMasternode.ManageState(connman);
        } else {
            // ... otherwise we need to reactivate our node, do not add it to the list and do not relay
            // but also do not ban the node we get this message from
            LogPrint(BCLog::MN, "CMasternode::UpdateFromNewBroadcast -- wrong MIN_PEER_PROTO_VERSION, re-activate your MN: message nProtocolVersion=%d  MIN_PEER_PROTO_VERSION=%d\n", nProtocolVersion, MIN_PEER_PROTO_VERSION);
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
arith_uint256 CMasternode::CalculateScore(const uint256& blockHash) const
{
    // Deterministically calculate a "score" for a Masternode based on any given (block)hash
    CHashWriter ss(SER_GETHASH, MIN_PEER_PROTO_VERSION);
    ss << outpoint << nCollateralMinConfBlockHash << blockHash;
    return UintToArith256(ss.GetHash());
}

CMasternode::CollateralStatus CMasternode::CheckCollateral(const COutPoint& outpoint, const CPubKey& pubkey)
{
    int nHeight;
    return CheckCollateral(outpoint, pubkey, nHeight);
}

CMasternode::CollateralStatus CMasternode::CheckCollateral(const COutPoint& outpoint, const CPubKey& pubkey, int& nHeightRet)
{
    AssertLockHeld(cs_main);

    Coin coin;
    if(!GetUTXOCoin(outpoint, coin)) {
        return COLLATERAL_UTXO_NOT_FOUND;
    }

    if(coin.out.nValue != 100000 * COIN) {
        return COLLATERAL_INVALID_AMOUNT;
    }

    if(pubkey == CPubKey() || coin.out.scriptPubKey != GetScriptForDestination(pubkey.GetID())) {
        return COLLATERAL_INVALID_PUBKEY;
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

    LogPrint(BCLog::MN, "CMasternode::Check -- Masternode %s is in %s state\n", outpoint.ToStringShort(), GetStateString());

    //once spent, stop doing the checks
    if(IsOutpointSpent()) return;
    
    if(!fForce){
    	TRY_LOCK(cs_main, lockMain);
    	if (!lockMain) return;
        Coin coin;
        if(!GetUTXOCoin(outpoint, coin)) {
            nActiveState = MASTERNODE_OUTPOINT_SPENT;
            LogPrint(BCLog::MN, "CMasternode::Check -- Failed to find Masternode UTXO, masternode=%s\n", outpoint.ToStringShort());
            return;
        }
    }
    const int nActiveStatePrev = nActiveState;
    int nHeight = chainActive.Height();
    

    if(IsPoSeBanned()) {
        if(nHeight < nPoSeBanHeight) return; // too early?
        // Otherwise give it a chance to proceed further to do all the usual checks and to change its state.
        // Masternode still will be on the edge and can be banned back easily if it keeps ignoring mnverify
        // or connect attempts. Will require few mnverify messages to strengthen its position in mn list.
        LogPrint(BCLog::MN, "CMasternode::Check -- Masternode %s is unbanned and back in list now\n", outpoint.ToStringShort());
        DecreasePoSeBanScore();
    } else if(nPoSeBanScore >= MASTERNODE_POSE_BAN_MAX_SCORE) {
        nActiveState = MASTERNODE_POSE_BAN;
        // ban for the whole payment cycle
        nPoSeBanHeight = nHeight + mnodeman.size();
        LogPrint(BCLog::MN, "CMasternode::Check -- Masternode %s is banned till block %d now\n", outpoint.ToStringShort(), nPoSeBanHeight);
        return;
    }

   
    bool fOurMasternode = fMasternodeMode && activeMasternode.pubKeyMasternode == pubKeyMasternode;

                   // masternode doesn't meet payment protocol requirements ...
    bool fRequireUpdate = nProtocolVersion < mnpayments.GetMinMasternodePaymentsProto() ||
                   // or it's our own node and we just updated it to the new protocol but we are still waiting for activation ...
                   (fOurMasternode && nProtocolVersion < MIN_PEER_PROTO_VERSION);

    if(fRequireUpdate) {
        nActiveState = MASTERNODE_UPDATE_REQUIRED;
        if(nActiveStatePrev != nActiveState) {
            LogPrint(BCLog::MN, "CMasternode::Check -- Masternode %s is in %s state now\n", outpoint.ToStringShort(), GetStateString());
        }
        return;
    }

    // keep old masternodes on start, give them a chance to receive updates...
    bool fWaitForPing = !masternodeSync.IsMasternodeListSynced() && !IsPingedWithin(MASTERNODE_MIN_MNP_SECONDS);

    if(fWaitForPing && !fOurMasternode) {
        // ...but if it was already expired before the initial check - return right away
        if(IsSentinelPingExpired() || IsNewStartRequired()) {
            LogPrint(BCLog::MN, "CMasternode::Check -- Masternode %s is in %s state, waiting for ping\n", outpoint.ToStringShort(), GetStateString());
            return;
        }
    }
    
    if (Params().NetworkIDString() != CBaseChainParams::REGTEST) {  
        if (sigTime <= 0 || IsBroadcastedWithin(MASTERNODE_MIN_MNP_SECONDS*10)) {  
            nActiveState = MASTERNODE_PRE_ENABLED;  
            if (nActiveStatePrev != nActiveState) { 
                LogPrint(BCLog::MN, "CMasternode::Check -- Masternode %s is in %s state now\n", outpoint.ToStringShort(), GetStateString());    
            }   
            return; 
        }   
    }   
    
    // don't expire if we are still in "waiting for ping" mode unless it's our own masternode
    if(!fWaitForPing || fOurMasternode) {

        if(nPingRetries >= MASTERNODE_MAX_RETRIES) {
            nActiveState = MASTERNODE_NEW_START_REQUIRED;
            if(nActiveStatePrev != nActiveState) {
                LogPrint(BCLog::MN, "CMasternode::Check -- Masternode %s is in %s state now\n", outpoint.ToStringShort(), GetStateString());
            }
            return;
        }
        

        // part 1: expire based on syscoind ping
        bool fSentinelPingExpired = masternodeSync.IsSynced() && !IsPingedWithin(MASTERNODE_SENTINEL_PING_MAX_SECONDS);
        LogPrint(BCLog::MN, "CMasternode::Check -- outpoint=%s, GetAdjustedTime()=%d, fSentinelPingExpired=%d\n",
                outpoint.ToStringShort(), GetAdjustedTime(), fSentinelPingExpired);

        if(fSentinelPingExpired) {
            // only update if its not forced (the timer that calls maintenance which calls every masternode passes false for force)
            if(!fForce){
                const CScript &mnScript = GetScriptForDestination(pubKeyCollateralAddress.GetID());
                // only check masternodes in winners list
                bool foundPayee = false;
                {
                    LOCK(cs_mapMasternodeBlocks);

                    for (int i = -10; i < 20; i++) {
                        if(foundPayee)
                            break;
                        if(mnpayments.mapMasternodeBlocks.count(chainActive.Height()+i))
                        {
                            const CMasternodeBlockPayees &payees = mnpayments.mapMasternodeBlocks[chainActive.Height()+i];
                            for(auto& payee: payees.vecPayees){
                                if (payee.GetVoteCount() >= MNPAYMENTS_SIGNATURES_REQUIRED) {
                                    if(mnScript == payee.GetPayee())
                                    {
                                        foundPayee = true;
                                        break;
                                    }
                                }
                            }
                            
                        }
                    }
                }
                if(foundPayee){
                    nPingRetries++;
                    CMasternodeBroadcast mnb(*this);
                    const uint256 &hash = mnb.GetHash();
                    bool existsInMnbList = mnodeman.mapSeenMasternodeBroadcast.count(hash);
                    if (existsInMnbList) {
                        mnodeman.mapSeenMasternodeBroadcast[hash].second.nPingRetries = nPingRetries;
                        
                    }                        
                    
                }           

            }
            if(nPingRetries >= MASTERNODE_MAX_RETRIES) {
                nActiveState = MASTERNODE_NEW_START_REQUIRED;
                if(nActiveStatePrev != nActiveState) {
                    LogPrint(BCLog::MN, "CMasternode::Check -- Masternode %s is in %s state now\n", outpoint.ToStringShort(), GetStateString());
                }
            }                
        }
        else{
            // part 2: expire based on sentinel ping  
            fSentinelPingExpired = masternodeSync.IsSynced() && mnodeman.IsSentinelPingActive() && lastPing && !lastPing.fSentinelIsCurrent;
            if(fSentinelPingExpired) {
                nActiveState = MASTERNODE_SENTINEL_PING_EXPIRED;
                if(nActiveStatePrev != nActiveState) {
                    LogPrint(BCLog::MN, "CMasternode::Check -- Masternode %s is in %s state now\n", outpoint.ToStringShort(), GetStateString());
                }
                return;
            }
        }
    }

    nActiveState = MASTERNODE_ENABLED;
    if(nActiveStatePrev != nActiveState) {
        LogPrint(BCLog::MN, "CMasternode::Check -- Masternode %s is in %s state now\n", outpoint.ToStringShort(), GetStateString());
    }
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

masternode_info_t CMasternode::GetInfo() const
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
        case MASTERNODE_OUTPOINT_SPENT:         return "OUTPOINT_SPENT";
        case MASTERNODE_UPDATE_REQUIRED:        return "UPDATE_REQUIRED";
        case MASTERNODE_SENTINEL_PING_EXPIRED:  return "SENTINEL_PING_EXPIRED";
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
	
	const CChainParams& chainparams = Params();
    const CScript &mnpayee = GetScriptForDestination(pubKeyCollateralAddress.GetID());
    // LogPrint(BCLog::MNPAYMENT, "CMasternode::UpdateLastPaidBlock -- searching for block with payment to %s\n", outpoint.ToStringShort());

    LOCK(cs_mapMasternodeBlocks);
	CMasternodePayee payee;
	CAmount nTotal;
    for (int i = 0; BlockReading && BlockReading->nHeight > nBlockLastPaid && i < nMaxBlocksToScanBack; i++) {
        if(mnpayments.mapMasternodeBlocks.count(BlockReading->nHeight) &&
            mnpayments.mapMasternodeBlocks[BlockReading->nHeight].HasPayeeWithVotes(mnpayee, 2, payee))
        {
            CBlock block;
			if (!ReadBlockFromDisk(block, BlockReading, Params().GetConsensus())) {
				if (BlockReading->pprev == NULL) { assert(BlockReading); break; }
				BlockReading = BlockReading->pprev;
				LogPrint(BCLog::MNPAYMENT, "CMasternode::UpdateLastPaidBlock -- Could not read block from disk\n");
				continue; // shouldn't really happen
			}

			const CAmount & nMasternodePayment = GetBlockSubsidy(BlockReading->nHeight, chainparams.GetConsensus(), nTotal, false, true, payee.nStartHeight);

            for (const auto& txout : block.vtx[0]->vout)
                if(mnpayee == txout.scriptPubKey && nMasternodePayment <= txout.nValue) {
                    nBlockLastPaid = BlockReading->nHeight;
                    nTimeLastPaid = BlockReading->nTime;
                    LogPrint(BCLog::MNPAYMENT, "CMasternode::UpdateLastPaidBlock -- searching for block with payment to %s -- found new %d\n", outpoint.ToStringShort(), nBlockLastPaid);
                    return;
                }
        }

        if (BlockReading->pprev == NULL) { assert(BlockReading); break; }
        BlockReading = BlockReading->pprev;
    }

    // Last payment for this masternode wasn't found in latest mnpayments blocks
    // or it was found in mnpayments blocks but wasn't found in the blockchain.
    // LogPrint(BCLog::MNPAYMENT, "CMasternode::UpdateLastPaidBlock -- searching for block with payment to %s -- keeping old %d\n", outpoint.ToStringShort(), nBlockLastPaid);
}

#ifdef ENABLE_WALLET
bool CMasternodeBroadcast::Create(const std::string& strService, const std::string& strKeyMasternode, const std::string& strTxHash, const std::string& strOutputIndex, std::string& strErrorRet, CMasternodeBroadcast &mnbRet, bool fOffline)
{
    COutPoint outpoint;
    CPubKey pubKeyCollateralAddressNew;
    CKey keyCollateralAddressNew;
    CPubKey pubKeyMasternodeNew;
    CKey keyMasternodeNew;

    auto Log = [&strErrorRet](std::string sErr)->bool
    {
        strErrorRet = sErr;
        LogPrint(BCLog::MN, "CMasternodeBroadcast::Create -- %s\n", strErrorRet);
        return false;
    };

    // Wait for sync to finish because mnb simply won't be relayed otherwise
    if (!fOffline && !masternodeSync.IsSynced())
        return Log("Sync in progress. Must wait until sync is complete to start Masternode");

    if (!CMessageSigner::GetKeysFromSecret(strKeyMasternode, keyMasternodeNew, pubKeyMasternodeNew))
        return Log(strprintf("Invalid masternode key %s", strKeyMasternode));
    CWallet* const pwallet = GetDefaultWallet();
    if (!pwallet || !pwallet->GetMasternodeOutpointAndKeys(outpoint, pubKeyCollateralAddressNew, keyCollateralAddressNew, strTxHash, strOutputIndex))
        return Log(strprintf("Could not allocate outpoint %s:%s for masternode %s", strTxHash, strOutputIndex, strService));

    CService service;
    if (!Lookup(strService.c_str(), service, 0, false))
        return Log(strprintf("Invalid address %s for masternode.", strService));
    const auto chainParams = CreateChainParams(CBaseChainParams::MAIN);
    int mainnetDefaultPort = chainParams->GetDefaultPort();
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

    LogPrint(BCLog::MN, "CMasternodeBroadcast::Create -- pubKeyCollateralAddressNew = %s, pubKeyMasternodeNew.GetID() = %s\n",
             EncodeDestination(pubKeyCollateralAddressNew.GetID()),
             pubKeyMasternodeNew.GetID().ToString());

    auto Log = [&strErrorRet,&mnbRet](std::string sErr)->bool
    {
        strErrorRet = sErr;
        LogPrint(BCLog::MN, "CMasternodeBroadcast::Create -- %s\n", strErrorRet);
        mnbRet = CMasternodeBroadcast();
        return false;
    };

    CMasternodePing mnp(outpoint);
    if (!mnp.Sign(keyMasternodeNew, pubKeyMasternodeNew))
        return Log(strprintf("Failed to sign ping, masternode=%s", outpoint.ToStringShort()));

    mnbRet = CMasternodeBroadcast(service, outpoint, pubKeyCollateralAddressNew, pubKeyMasternodeNew, MIN_PEER_PROTO_VERSION);

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
        LogPrint(BCLog::MN, "CMasternodeBroadcast::SimpleCheck -- Invalid addr, rejected: masternode=%s  addr=%s\n",
                    outpoint.ToStringShort(), addr.ToString());
        return false;
    }

    // make sure signature isn't in the future (past is OK)
    if (sigTime > GetAdjustedTime() + MASTERNODE_MIN_MNB_SECONDS) {
        LogPrint(BCLog::MN, "CMasternodeBroadcast::SimpleCheck -- Signature rejected, too far into the future: masternode=%s\n", outpoint.ToStringShort());
        nDos = 1;
        return false;
    }
    

    if(nProtocolVersion < mnpayments.GetMinMasternodePaymentsProto()) {
        LogPrint(BCLog::MN, "CMasternodeBroadcast::SimpleCheck -- outdated Masternode: masternode=%s  nProtocolVersion=%d\n", outpoint.ToStringShort(), nProtocolVersion);
        nActiveState = MASTERNODE_UPDATE_REQUIRED;
    }
    
    const CScript &pubkeyScript = GetScriptForDestination(pubKeyCollateralAddress.GetID());

    if(pubkeyScript.size() != 25) {
        LogPrint(BCLog::MN, "CMasternodeBroadcast::SimpleCheck -- pubKeyCollateralAddress has the wrong size\n");
        nDos = 100;
        return false;
    }
    
    const CScript &pubkeyScript2 = GetScriptForDestination(pubKeyMasternode.GetID());

    if(pubkeyScript2.size() != 25) {
        LogPrint(BCLog::MN, "CMasternodeBroadcast::SimpleCheck -- pubKeyMasternode has the wrong size\n");
        nDos = 100;
        return false;
    }

    const auto chainParams = CreateChainParams(CBaseChainParams::MAIN);
    int mainnetDefaultPort = chainParams->GetDefaultPort();
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
        LogPrint(BCLog::MN, "CMasternodeBroadcast::Update -- Bad sigTime %d (existing broadcast is at %d) for Masternode %s %s\n",
                      sigTime, pmn->sigTime, outpoint.ToStringShort(), addr.ToString());
        return false;
    }

    pmn->Check();

    // masternode is banned by PoSe
    if(pmn->IsPoSeBanned()) {
        LogPrint(BCLog::MN, "CMasternodeBroadcast::Update -- Banned by PoSe, masternode=%s\n", outpoint.ToStringShort());
        return false;
    }

    // IsVnAssociatedWithPubkey is validated once in CheckOutpoint, after that they just need to match
    if(pmn->pubKeyCollateralAddress != pubKeyCollateralAddress) {
        LogPrint(BCLog::MN, "CMasternodeBroadcast::Update -- Got mismatched pubKeyCollateralAddress and outpoint\n");
        nDos = 33;
        return false;
    }

    if (!CheckSignature(nDos)) {
        LogPrint(BCLog::MN, "CMasternodeBroadcast::Update -- CheckSignature() failed, masternode=%s\n", outpoint.ToStringShort());
        return false;
    }

    // if ther was no masternode broadcast recently or if it matches our Masternode privkey...
    if(!pmn->IsBroadcastedWithin(MASTERNODE_MIN_MNB_SECONDS) || (fMasternodeMode && pubKeyMasternode == activeMasternode.pubKeyMasternode)) {
        // take the newest entry
        LogPrint(BCLog::MN, "CMasternodeBroadcast::Update -- Got UPDATED Masternode entry: addr=%s\n", addr.ToString());
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
    // we are a masternode with the same outpoint (i.e. already activated) and this mnb is ours (matches our Masternode privkey)
    // so nothing to do here for us
    if(fMasternodeMode && outpoint == activeMasternode.outpoint && pubKeyMasternode == activeMasternode.pubKeyMasternode) {
        return false;
    }

    int nHeight;
    CollateralStatus err = CheckCollateral(outpoint, pubKeyCollateralAddress, nHeight);
    if (err == COLLATERAL_UTXO_NOT_FOUND) {
        LogPrint(BCLog::MN, "CMasternodeBroadcast::CheckOutpoint -- Failed to find Masternode UTXO, masternode=%s\n", outpoint.ToStringShort());
        return false;
    }

    if (err == COLLATERAL_INVALID_AMOUNT) {
        LogPrint(BCLog::MN, "CMasternodeBroadcast::CheckOutpoint -- Masternode UTXO should have 100000 SYS, masternode=%s\n", outpoint.ToStringShort());
        nDos = 33;
        return false;
    }

    if(err == COLLATERAL_INVALID_PUBKEY) {
        LogPrint(BCLog::MN, "CMasternodeBroadcast::CheckOutpoint -- Masternode UTXO should match pubKeyCollateralAddress, masternode=%s\n", outpoint.ToStringShort());
        nDos = 33;
        return false;
    }
	{
		TRY_LOCK(cs_main, lockMain);
		if (!lockMain) {
			// not mnb fault, let it to be checked again later
			LogPrint(BCLog::MN, "CMasternodeBroadcast::CheckOutpoint -- Failed to aquire lock, addr=%s", addr.ToString());
			mnodeman.mapSeenMasternodeBroadcast.erase(GetHash());
			return false;
		}
		if (chainActive.Height() - nHeight + 1 < Params().GetConsensus().nMasternodeMinimumConfirmations) {
			LogPrint(BCLog::MN, "CMasternodeBroadcast::CheckOutpoint -- Masternode UTXO must have at least %d confirmations, masternode=%s\n",
				Params().GetConsensus().nMasternodeMinimumConfirmations, outpoint.ToStringShort());
			// UTXO is legit but has not enough confirmations.
			// Maybe we miss few blocks, let this mnb be checked again later.
			mnodeman.mapSeenMasternodeBroadcast.erase(GetHash());
			return false;
		}
	}

    LogPrint(BCLog::MN, "CMasternodeBroadcast::CheckOutpoint -- Masternode UTXO verified\n");

    // Verify that sig time is legit, should be at least not earlier than the timestamp of the block
    // at which collateral became nMasternodeMinimumConfirmations blocks deep.
    // NOTE: this is not accurate because block timestamp is NOT guaranteed to be 100% correct one.
    CBlockIndex* pRequiredConfIndex = chainActive[nHeight + Params().GetConsensus().nMasternodeMinimumConfirmations - 1]; // block where tx got nMasternodeMinimumConfirmations
    if(pRequiredConfIndex->GetBlockTime() > sigTime) {
        LogPrint(BCLog::MN, "CMasternodeBroadcast::CheckOutpoint -- Bad sigTime %d (%d conf block is at %d) for Masternode %s %s\n",
                  sigTime, Params().GetConsensus().nMasternodeMinimumConfirmations, pRequiredConfIndex->GetBlockTime(), outpoint.ToStringShort(), addr.ToString());
        return false;
    }

    if (!CheckSignature(nDos)) {
        LogPrint(BCLog::MN, "CMasternodeBroadcast::CheckOutpoint -- CheckSignature() failed, masternode=%s\n", outpoint.ToStringShort());
        return false;
    }

    // remember the block hash when collateral for this masternode had minimum required confirmations
    nCollateralMinConfBlockHash = pRequiredConfIndex->GetBlockHash();

    return true;
}

uint256 CMasternodeBroadcast::GetHash() const
{
    // Note: doesn't match serialization

    CHashWriter ss(SER_GETHASH, MIN_PEER_PROTO_VERSION);
    ss << outpoint;
    ss << pubKeyCollateralAddress;
    ss << sigTime;
    return ss.GetHash();
}

uint256 CMasternodeBroadcast::GetSignatureHash() const
{
    return SerializeHash(*this);
}

bool CMasternodeBroadcast::Sign(const CKey& keyCollateralAddress)
{
    std::string strError;

    sigTime = GetAdjustedTime();

    if (sporkManager.IsSporkActive(SPORK_6_NEW_SIGS)) {
        uint256 hash = GetSignatureHash();

        if (!CHashSigner::SignHash(hash, keyCollateralAddress, vchSig)) {
            LogPrint(BCLog::MN, "CMasternodeBroadcast::Sign -- SignHash() failed\n");
            return false;
        }

        if (!CHashSigner::VerifyHash(hash, pubKeyCollateralAddress, vchSig, strError)) {
            LogPrint(BCLog::MN, "CMasternodeBroadcast::Sign -- VerifyMessage() failed, error: %s\n", strError);
            return false;
        }
    } 

    return true;
}

bool CMasternodeBroadcast::CheckSignature(int& nDos) const
{
    std::string strError = "";
    nDos = 0;

    if (sporkManager.IsSporkActive(SPORK_6_NEW_SIGS)) {
        uint256 hash = GetSignatureHash();

        if (!CHashSigner::VerifyHash(hash, pubKeyCollateralAddress, vchSig, strError)) {
            LogPrint(BCLog::MN, "CMasternodeBroadcast::CheckSignature -- Got bad Masternode announce signature, error: %s\n", strError);
            nDos = 100;
            return false;          
        }
    } 

    return true;
}

void CMasternodeBroadcast::Relay(CConnman& connman) const
{
    // Do not relay until fully synced
    if(!masternodeSync.IsSynced()) {
        LogPrint(BCLog::MN, "CMasternodeBroadcast::Relay -- won't relay until fully synced\n");
        return;
    }

    CInv inv(MSG_MASTERNODE_ANNOUNCE, GetHash());
    connman.RelayInv(inv);
}

uint256 CMasternodePing::GetHash() const
{
    
    return SerializeHash(*this);
    
}

uint256 CMasternodePing::GetSignatureHash() const
{
    return GetHash();
}

CMasternodePing::CMasternodePing(const COutPoint& outpoint)
{
    LOCK(cs_main);
    if (!chainActive.Tip() || chainActive.Height() < 12) return;

    masternodeOutpoint = outpoint;
    blockHash = chainActive[chainActive.Height() - 12]->GetBlockHash();
    sigTime = GetAdjustedTime();
    nDaemonVersion = CLIENT_MASTERNODE_VERSION;
}

bool CMasternodePing::Sign(const CKey& keyMasternode, const CPubKey& pubKeyMasternode)
{
    std::string strError;

    sigTime = GetAdjustedTime();

    if (sporkManager.IsSporkActive(SPORK_6_NEW_SIGS)) {
        uint256 hash = GetSignatureHash();

        if (!CHashSigner::SignHash(hash, keyMasternode, vchSig)) {
            LogPrint(BCLog::MN, "CMasternodePing::Sign -- SignHash() failed\n");
            return false;
        }

        if (!CHashSigner::VerifyHash(hash, pubKeyMasternode, vchSig, strError)) {
            LogPrint(BCLog::MN, "CMasternodePing::Sign -- VerifyHash() failed, error: %s\n", strError);
            return false;
        }
    } 

    return true;
}

bool CMasternodePing::CheckSignature(const CPubKey& pubKeyMasternode, int &nDos) const
{
    std::string strError = "";
    nDos = 0;

    if (sporkManager.IsSporkActive(SPORK_6_NEW_SIGS)) {
        uint256 hash = GetSignatureHash();

        if (!CHashSigner::VerifyHash(hash, pubKeyMasternode, vchSig, strError)) {
            LogPrint(BCLog::MN, "CMasternodePing::CheckSignature -- Got bad Masternode ping signature, masternode=%s, error: %s\n", masternodeOutpoint.ToStringShort(), strError);
            nDos = 33;
            return false;
        }
    } 

    return true;
}

bool CMasternodePing::SimpleCheck(int& nDos)
{
    // don't ban by default
    nDos = 0;

    if (sigTime > GetAdjustedTime() + MASTERNODE_MIN_MNB_SECONDS) {
        LogPrint(BCLog::MN, "CMasternodePing::SimpleCheck -- Signature rejected, too far into the future, masternode=%s\n", masternodeOutpoint.ToStringShort());
        nDos = 1;
        return false;
    }

    {
        AssertLockHeld(cs_main);
        BlockMap::iterator mi = mapBlockIndex.find(blockHash);
        if (mi == mapBlockIndex.end()) {
            LogPrint(BCLog::MN, "CMasternodePing::SimpleCheck -- Masternode ping is invalid, unknown block hash: masternode=%s blockHash=%s\n", masternodeOutpoint.ToStringShort(), blockHash.ToString());
            // maybe we stuck or forked so we shouldn't ban this node, just fail to accept this ping
            // TODO: or should we also request this block?
            return false;
        }
    }

    LogPrint(BCLog::MN, "CMasternodePing::SimpleCheck -- Masternode ping verified: masternode=%s  blockHash=%s  sigTime=%d\n", masternodeOutpoint.ToStringShort(), blockHash.ToString(), sigTime);
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
        LogPrint(BCLog::MN, "CMasternodePing::CheckAndUpdate -- Couldn't find Masternode entry, masternode=%s\n", masternodeOutpoint.ToStringShort());
        return false;
    }
    // allow pings during pre-enabled state and if its a new broadcast ping
    if(!fFromNewBroadcast && pmn->nActiveState != MASTERNODE_PRE_ENABLED && masternodeSync.IsSynced()){
        // ensure that masternode being pinged also exists in the payee list of up to 10 blocks in the future otherwise we don't like this ping
        const CScript &mnpayee = GetScriptForDestination(pmn->pubKeyCollateralAddress.GetID());
        bool foundPayee = false;
        {
            LOCK(cs_mapMasternodeBlocks);
            CMasternodePayee payee;  
            // only allow ping if MNPAYMENTS_SIGNATURES_REQUIRED votes are on this masternode in last 10 blocks of winners list and 20 blocks into future (match default of masternode winners)     
            for (int i = -10; i < 20; i++) {
                if(mnpayments.mapMasternodeBlocks.count(chainActive.Height()+i) &&
                      mnpayments.mapMasternodeBlocks[chainActive.Height()+i].HasPayeeWithVotes(mnpayee, MNPAYMENTS_SIGNATURES_REQUIRED, payee))
                {
                    foundPayee = true;
                    break;
                }
            }
           for (int i = -10; i < 20; i++) {
                if(mnpayments.mapMasternodeBlocks.count(chainActive.Height()+i) &&
                      mnpayments.mapMasternodeBlocks[chainActive.Height()+i].HasPayeeWithVotes(mnpayee, 0, payee))
                {
                    foundPayeeInWinnersList = true;
                    break;
                }
            }
        }
        if(!foundPayee){
            LogPrint(BCLog::MN, "CMasternodePing::CheckAndUpdate -- Couldn't find Masternode entry in the payee list, masternode=%s\n", masternodeOutpoint.ToStringShort());
            return false;
        }
    }


    if(!fFromNewBroadcast) {
        if (pmn->IsUpdateRequired()) {
            LogPrint(BCLog::MN, "CMasternodePing::CheckAndUpdate -- masternode protocol is outdated, masternode=%s\n", masternodeOutpoint.ToStringShort());
            return false;
        }
        if (pmn->IsNewStartRequired()) {
            LogPrint(BCLog::MN, "CMasternodePing::CheckAndUpdate -- masternode is completely expired, new start is required, masternode=%s\n", masternodeOutpoint.ToStringShort());
            return false;
        }
    }

    {
		LOCK(cs_main);
        BlockMap::iterator mi = mapBlockIndex.find(blockHash);
        if ((*mi).second && (*mi).second->nHeight < chainActive.Height() - 24) {
            LogPrint(BCLog::MN, "CMasternodePing::CheckAndUpdate -- Masternode ping is invalid, block hash is too old: masternode=%s  blockHash=%s\n", masternodeOutpoint.ToStringShort(), blockHash.ToString());
            // nDos = 1;
            return false;
        }
    }

    LogPrint(BCLog::MN, "CMasternodePing::CheckAndUpdate -- New ping: masternode=%s  blockHash=%s  sigTime=%d\n", masternodeOutpoint.ToStringShort(), blockHash.ToString(), sigTime);

    // LogPrint(BCLog::MN, "mnping - Found corresponding mn for outpoint: %s\n", masternodeOutpoint.ToStringShort());
    // update only if there is no known ping for this masternode or
    // last ping was more then MASTERNODE_MIN_MNP_SECONDS-30 ago comparing to this one
    if (pmn->IsPingedWithin(MASTERNODE_MIN_MNP_SECONDS - 30, sigTime)) {
        LogPrint(BCLog::MN, "CMasternodePing::CheckAndUpdate -- Masternode ping arrived too early, masternode=%s\n", masternodeOutpoint.ToStringShort());
        //nDos = 1; //disable, this is happening frequently and causing banned peers
        return false;
    }

    if (!CheckSignature(pmn->pubKeyMasternode, nDos)) return false;

    // so, ping seems to be ok

    // if we are still syncing and there was no known ping for this mn for quite a while
    // (NOTE: assuming that MASTERNODE_SENTINEL_PING_MAX_SECONDS*20 should be enough to finish mn list sync)
    if(!masternodeSync.IsMasternodeListSynced() && pmn->lastPing && !pmn->IsPingedWithin(MASTERNODE_SENTINEL_PING_MAX_SECONDS*20)) {
        LogPrint(BCLog::MN, "CMasternodePing::CheckAndUpdate -- bumping sync timeout, masternode=%s\n", masternodeOutpoint.ToStringShort());
        masternodeSync.BumpAssetLastTime("CMasternodePing::CheckAndUpdate");
    }
    LogPrint(BCLog::MN, "CMasternodePing::CheckAndUpdate -- Masternode ping accepted, masternode=%s\n", masternodeOutpoint.ToStringShort());
    pmn->lastPing = *this;

    // and update mnodeman.mapSeenMasternodeBroadcast.lastPing which is probably outdated
    CMasternodeBroadcast mnb(*pmn);
    const uint256 &hash = mnb.GetHash();
    bool existsInMnbList = mnodeman.mapSeenMasternodeBroadcast.count(hash);
    if (existsInMnbList) {
        mnodeman.mapSeenMasternodeBroadcast[hash].second.lastPing = *this;
        
    }
    
    // force update, ignoring cache
    pmn->Check(true);
    
    // relay ping for nodes in ENABLED/SENTINEL_PING_EXPIRED state only, skip everyone else
    if (!pmn->IsEnabled() && !pmn->IsSentinelPingExpired()) return false;

    LogPrint(BCLog::MN, "CMasternodePing::CheckAndUpdate -- Masternode ping accepted and relayed, masternode=%s\n", masternodeOutpoint.ToStringShort());
    
    if(pmn->nPingRetries > 0){
        LogPrint(BCLog::MN, "CMasternodePing::CheckAndUpdate -- Ping retries reduced from %d\n", pmn->nPingRetries);
        pmn->nPingRetries -= 1;
        if(existsInMnbList)
            mnodeman.mapSeenMasternodeBroadcast[hash].second.nPingRetries = pmn->nPingRetries;
    }
    
    Relay(connman);

    return true;
}

void CMasternodePing::Relay(CConnman& connman)
{
    if(!masternodeSync.IsSynced()) {
        LogPrint(BCLog::MN, "CMasternodePing::Relay -- won't relay until fully synced\n");
        return;
    }

    CInv inv(MSG_MASTERNODE_PING, GetHash());
    connman.RelayInv(inv);
}

std::string CMasternodePing::GetSentinelString() const
{
    return FormatVersion(nSentinelVersion);
}

std::string CMasternodePing::GetDaemonString() const
{
    return FormatVersion(nDaemonVersion);
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
