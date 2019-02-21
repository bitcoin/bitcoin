// Copyright (c) 2014-2017 The Dash Core developers
// Copyright (c) 2017-2018 The Syscoin Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "activemasternode.h"
#include "consensus/validation.h"
#include "governance-classes.h"
#include "masternode-payments.h"
#include "masternode-sync.h"
#include "masternodeman.h"
#include "messagesigner.h"
#include "netfulfilledman.h"
#include "netmessagemaker.h"
#include "spork.h"
#include "util.h"
#include <outputtype.h>
#include <boost/lexical_cast.hpp>
// SYSCOIN
extern void Misbehaving(NodeId nodeid, int howmuch, const std::string& message="") EXCLUSIVE_LOCKS_REQUIRED(cs_main);
extern std::vector<unsigned char> vchFromString(const std::string &str);

/** Object for who's going to get paid on which blocks */
CMasternodePayments mnpayments;

CCriticalSection cs_vecPayees;
CCriticalSection cs_mapMasternodeBlocks;
CCriticalSection cs_mapMasternodePaymentVotes;

/**
* IsBlockValueValid
*
*   Determine if coinbase outgoing created money is the correct value
*
*   Why is this needed?
*   - In Syscoin some blocks are superblocks, which output much higher amounts of coins
*   - Otherblocks are 10% lower in outgoing value, so in total, no extra coins are created
*   - When non-superblocks are detected, the normal schedule should be maintained
*/

bool IsBlockValueValid(const CBlock& block, int nBlockHeight, const CAmount &blockReward, const CAmount &nFee, std::string& strErrorRet)
{
	const CAmount &blockRewardWithFee = blockReward + nFee;
    strErrorRet = "";
	bool isBlockRewardValueMet = (block.vtx[0]->GetValueOut() <= blockRewardWithFee);
	LogPrint( BCLog::MNPAYMENT, "block.vtx[0].GetValueOut() %lld <= blockReward %lld\n", block.vtx[0]->GetValueOut(), blockRewardWithFee);
    // we are still using budgets, but we have no data about them anymore,
    // all we know is predefined budget cycle and window

    // superblocks started

	CAmount nSuperblockMaxValue = blockRewardWithFee + CSuperblock::GetPaymentsLimit(nBlockHeight);
	bool isSuperblockMaxValueMet = (block.vtx[0]->GetValueOut() <= nSuperblockMaxValue);

    LogPrint(BCLog::GOBJECT, "block.vtx[0]->GetValueOut() %lld <= nSuperblockMaxValue %lld\n", block.vtx[0]->GetValueOut(), nSuperblockMaxValue);

    if(!masternodeSync.IsSynced() || fLiteMode) {
        // not enough data but at least it must NOT exceed superblock max value
        if(CSuperblock::IsValidBlockHeight(nBlockHeight)) {
            LogPrint(BCLog::MNPAYMENT,"IsBlockPayeeValid -- WARNING: Not enough data, checking superblock max bounds only\n");
            if(!isSuperblockMaxValueMet) {
                strErrorRet = strprintf("coinbase pays too much at height %d (actual=%d vs limit=%d), exceeded superblock max value",
                                        nBlockHeight, block.vtx[0]->GetValueOut(), nSuperblockMaxValue);
            }
            return isSuperblockMaxValueMet;
        }
		else {
			CAmount nTotalRewardWithMasternodes;
			GetBlockSubsidy(nBlockHeight, Params().GetConsensus(), nTotalRewardWithMasternodes, false, true, 1);
			if (block.vtx[0]->GetValueOut() > (nTotalRewardWithMasternodes+nFee)) {
				strErrorRet = strprintf("IsBlockValueValid: coinbase amount exceeds block subsidy schedule: %lld vs %lld(%lld fee)", block.vtx[0]->GetValueOut(), nTotalRewardWithMasternodes+nFee, nFee);
				return false;
			}
		}
        if(!isBlockRewardValueMet) {
            strErrorRet = strprintf("coinbase pays too much at height %d (actual=%d vs limit=%d), exceeded block reward, only regular blocks are allowed at this height",
				nBlockHeight, block.vtx[0]->GetValueOut(), blockRewardWithFee);
        }
        // it MUST be a regular block otherwise
        return isBlockRewardValueMet;
    }

    // we are synced, let's try to check as much data as we can

    if(sporkManager.IsSporkActive(SPORK_9_SUPERBLOCKS_ENABLED)) {
        if(CSuperblockManager::IsSuperblockTriggered(nBlockHeight)) {
            if(CSuperblockManager::IsValid(*block.vtx[0], nBlockHeight, blockRewardWithFee)) {
                LogPrint(BCLog::GOBJECT, "IsBlockValueValid -- Valid superblock at height %d: %s", nBlockHeight, block.vtx[0]->ToString());
                // all checks are done in CSuperblock::IsValid, nothing to do here
                return true;
            }

            // triggered but invalid? that's weird
            LogPrint(BCLog::MNPAYMENT, "IsBlockValueValid -- ERROR: Invalid superblock detected at height %d: %s", nBlockHeight, block.vtx[0]->ToString());
            // should NOT allow invalid superblocks, when superblocks are enabled
            strErrorRet = strprintf("invalid superblock detected at height %d", nBlockHeight);
            return false;
        }
		else {
			CAmount nTotalRewardWithMasternodes;
			GetBlockSubsidy(nBlockHeight, Params().GetConsensus(), nTotalRewardWithMasternodes, false, true, 1);
			if (block.vtx[0]->GetValueOut() > (nTotalRewardWithMasternodes + nFee)) {
				strErrorRet = strprintf("IsBlockValueValid: coinbase amount exceeds block subsidy schedule");
				return false;
			}
		}
        LogPrint(BCLog::GOBJECT, "IsBlockValueValid -- No triggered superblock detected at height %d\n", nBlockHeight);
        if(!isBlockRewardValueMet) {
            strErrorRet = strprintf("coinbase pays too much at height %d (actual=%d vs limit=%d), exceeded block reward, no triggered superblock detected",
                                    nBlockHeight, block.vtx[0]->GetValueOut(), blockRewardWithFee);
        }
    } else {
        // should NOT allow superblocks at all, when superblocks are disabled
        LogPrint(BCLog::GOBJECT, "IsBlockValueValid -- Superblocks are disabled, no superblocks allowed\n");
        if(!isBlockRewardValueMet) {
            strErrorRet = strprintf("coinbase pays too much at height %d (actual=%d vs limit=%d), exceeded block reward, superblocks are disabled",
                                    nBlockHeight, block.vtx[0]->GetValueOut(), blockRewardWithFee);
        }
    }

    // it MUST be a regular block
    return isBlockRewardValueMet;
}

bool IsBlockPayeeValid(const CTransaction& txNew, int nBlockHeight,  const CAmount &blockReward, const CAmount &fee, CAmount& nTotalRewardWithMasternodes)
{
    if(!masternodeSync.IsSynced() || fLiteMode) {
        //there is no budget data to use to check anything, let's just accept the longest chain
		LogPrint(BCLog::MNPAYMENT,"IsBlockPayeeValid -- WARNING: Not enough data, skipping block payee checks\n");
		nTotalRewardWithMasternodes = txNew.GetValueOut();
        return true;
    }

    // we are still using budgets, but we have no data about them anymore,
    // we can only check masternode payments

  

    // superblocks started
    // SEE IF THIS IS A VALID SUPERBLOCK

    if(sporkManager.IsSporkActive(SPORK_9_SUPERBLOCKS_ENABLED)) {
        if(CSuperblockManager::IsSuperblockTriggered(nBlockHeight)) {
            if(CSuperblockManager::IsValid(txNew, nBlockHeight, blockReward)) {
                LogPrint(BCLog::GOBJECT, "IsBlockPayeeValid -- Valid superblock at height %d: %s", nBlockHeight, txNew.ToString());
                return true;
            }

            LogPrint(BCLog::MNPAYMENT, "IsBlockPayeeValid -- ERROR: Invalid superblock detected at height %d: %s", nBlockHeight, txNew.ToString());
            // should NOT allow such superblocks, when superblocks are enabled
            return false;
        }
        // continue validation, should pay MN
        LogPrint(BCLog::GOBJECT, "IsBlockPayeeValid -- No triggered superblock detected at height %d\n", nBlockHeight);
    } else {
        // should NOT allow superblocks at all, when superblocks are disabled
        LogPrint(BCLog::GOBJECT, "IsBlockPayeeValid -- Superblocks are disabled, no superblocks allowed\n");
    }

    // IF THIS ISN'T A SUPERBLOCK OR SUPERBLOCK IS INVALID, IT SHOULD PAY A MASTERNODE DIRECTLY
    if(mnpayments.IsTransactionValid(txNew, nBlockHeight, fee, nTotalRewardWithMasternodes)) {
        LogPrint(BCLog::MNPAYMENT, "IsBlockPayeeValid -- Valid masternode payment at height %d: %s", nBlockHeight, txNew.ToString());
        return true;
    }

    if(sporkManager.IsSporkActive(SPORK_8_MASTERNODE_PAYMENT_ENFORCEMENT)) {
        LogPrint(BCLog::MNPAYMENT, "IsBlockPayeeValid -- ERROR: Invalid masternode payment detected at height %d: %s", nBlockHeight, txNew.ToString());
        return false;
    }

    LogPrint(BCLog::MNPAYMENT, "IsBlockPayeeValid -- WARNING: Masternode payment enforcement is disabled, accepting any payee\n");
    return true;
}

void FillBlockPayments(CMutableTransaction& txNew, int nBlockHeight, CAmount &blockReward, CAmount &fees, CTxOut& txoutMasternodeRet, std::vector<CTxOut>& voutSuperblockRet)
{
    // only create superblocks if spork is enabled AND if superblock is actually triggered
    // (height should be validated inside)
    if(sporkManager.IsSporkActive(SPORK_9_SUPERBLOCKS_ENABLED) &&
        CSuperblockManager::IsSuperblockTriggered(nBlockHeight)) {
            LogPrint(BCLog::GOBJECT, "FillBlockPayments -- triggered superblock creation at height %d\n", nBlockHeight);
            CSuperblockManager::CreateSuperblock(txNew, nBlockHeight, voutSuperblockRet);
            return;
    }

    // FILL BLOCK PAYEE WITH MASTERNODE PAYMENT OTHERWISE
	mnpayments.FillBlockPayee(txNew, nBlockHeight, blockReward, fees, txoutMasternodeRet);
	LogPrint(BCLog::MNPAYMENT, "FillBlockPayments -- nBlockHeight %d blockReward %lld txoutMasternodeRet %s",
		nBlockHeight, blockReward, txoutMasternodeRet.ToString());
}

std::string GetRequiredPaymentsString(int nBlockHeight)
{
    // IF WE HAVE A ACTIVATED TRIGGER FOR THIS HEIGHT - IT IS A SUPERBLOCK, GET THE REQUIRED PAYEES
    if(CSuperblockManager::IsSuperblockTriggered(nBlockHeight)) {
        return CSuperblockManager::GetRequiredPaymentsString(nBlockHeight);
    }

    // OTHERWISE, PAY MASTERNODE
    return mnpayments.GetRequiredPaymentsString(nBlockHeight);
}

void CMasternodePayments::Clear()
{
    LOCK2(cs_mapMasternodeBlocks, cs_mapMasternodePaymentVotes);
    mapMasternodeBlocks.clear();
    mapMasternodePaymentVotes.clear();
}

bool CMasternodePayments::UpdateLastVote(const CMasternodePaymentVote& vote)
{
    LOCK(cs_mapMasternodePaymentVotes);

    const auto it = mapMasternodesLastVote.find(vote.masternodeOutpoint);
    if (it != mapMasternodesLastVote.end()) {
        if (it->second == vote.nBlockHeight)
            return false;
        it->second = vote.nBlockHeight;
        return true;
    }

    //record this masternode voted
    mapMasternodesLastVote.emplace(vote.masternodeOutpoint, vote.nBlockHeight);
    return true;
}

/**
*   FillBlockPayee
*
*   Fill Masternode ONLY payment block
*/

void CMasternodePayments::FillBlockPayee(CMutableTransaction& txNew, int nBlockHeight,  CAmount &blockReward, CAmount &fees, CTxOut& txoutMasternodeRet) const
{
    // make sure it's not filled yet
    txoutMasternodeRet = CTxOut();
	const CAmount &nHalfFee = fees / 2;
    CScript payee;
	int nStartHeightBlock = 0;
    if(!GetBlockPayee(nBlockHeight, payee, nStartHeightBlock)) {
        // make sure to run masternode checks before trying to get next masternode in queue
        mnodeman.Check();
        // no masternode detected...
        int nCount = 0;
        masternode_info_t mnInfo;
        if(!mnodeman.GetNextMasternodeInQueueForPayment(nBlockHeight, true, nCount, mnInfo)) {
            // ...and we can't calculate it on our own
            LogPrint(BCLog::MNPAYMENT, "CMasternodePayments::FillBlockPayee -- Failed to detect masternode to pay\n");
            return;
        }
        // fill payee with locally calculated winner and hope for the best
        payee = GetScriptForDestination(mnInfo.pubKeyCollateralAddress.GetID());
		nStartHeightBlock = 0;
    }

	// miner takes 25% of the reward and half fees
	txNew.vout[0].nValue = (blockReward*0.25);
	
	txNew.vout[0].nValue += nHalfFee;
	// masternode takes 75% of reward, add/remove some reward depending on seniority and half fees.
	CAmount nTotalReward;
	blockReward = GetBlockSubsidy(nBlockHeight, Params().GetConsensus(), nTotalReward, false, true, nStartHeightBlock);

    // ... and masternode
    txoutMasternodeRet = CTxOut(blockReward, payee);
	txoutMasternodeRet.nValue += nHalfFee;

    txNew.vout.push_back(txoutMasternodeRet);

    CTxDestination address1;
    ExtractDestination(payee, address1);


    LogPrint(BCLog::MNPAYMENT, "CMasternodePayments::FillBlockPayee -- Masternode payment %lld to %s\n", blockReward, EncodeDestination(address1));
}

int CMasternodePayments::GetMinMasternodePaymentsProto() const {
	return MIN_MASTERNODE_PAYMENT_PROTO_VERSION_2;
}

void CMasternodePayments::ProcessMessage(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv, CConnman& connman)
{
    if(fLiteMode) return; // disable all Syscoin specific functionality

    if (strCommand == NetMsgType::MASTERNODEPAYMENTSYNC) { //Masternode Payments Request Sync

        if(pfrom->nVersion < GetMinMasternodePaymentsProto()) {
            LogPrint(BCLog::MNPAYMENT, "MASTERNODEPAYMENTSYNC -- peer=%d using obsolete version %i\n", pfrom->GetId(), pfrom->nVersion);
            connman.PushMessage(pfrom, CNetMsgMaker(pfrom->GetSendVersion()).Make(NetMsgType::REJECT, strCommand, REJECT_OBSOLETE,
                               strprintf("Version must be %d or greater", GetMinMasternodePaymentsProto())));
            return;
        }

        // Ignore such requests until we are fully synced.
        // We could start processing this after masternode list is synced
        // but this is a heavy one so it's better to finish sync first.
        if (!masternodeSync.IsSynced()) return;

        if(netfulfilledman.HasFulfilledRequest(pfrom->addr, NetMsgType::MASTERNODEPAYMENTSYNC)) {
            // Asking for the payments list multiple times in a short period of time is no good
            LogPrint(BCLog::MNPAYMENT, "MASTERNODEPAYMENTSYNC -- peer already asked me for the list, peer=%d\n", pfrom->GetId());
            LOCK(cs_main);
            Misbehaving(pfrom->GetId(), 20);
            return;
        }
        netfulfilledman.AddFulfilledRequest(pfrom->addr, NetMsgType::MASTERNODEPAYMENTSYNC);

        Sync(pfrom, connman);
        LogPrint(BCLog::MNPAYMENT, "MASTERNODEPAYMENTSYNC -- Sent Masternode payment votes to peer=%d\n", pfrom->GetId());

    } else if (strCommand == NetMsgType::MASTERNODEPAYMENTVOTE) { // Masternode Payments Vote for the Winner

        CMasternodePaymentVote vote;
        vRecv >> vote;

        if(pfrom->nVersion < GetMinMasternodePaymentsProto()) {
            LogPrint(BCLog::MNPAYMENT, "MASTERNODEPAYMENTVOTE -- peer=%d using obsolete version %i\n", pfrom->GetId(), pfrom->nVersion);
            connman.PushMessage(pfrom, CNetMsgMaker(pfrom->GetSendVersion()).Make(NetMsgType::REJECT, strCommand, REJECT_OBSOLETE,
                               strprintf("Version must be %d or greater", GetMinMasternodePaymentsProto())));
            return;
        }

        uint256 nHash = vote.GetHash();

        pfrom->setAskFor.erase(nHash);

        // TODO: clear setAskFor for MSG_MASTERNODE_PAYMENT_BLOCK too

        // Ignore any payments messages until masternode list is synced
        if(!masternodeSync.IsMasternodeListSynced()) return;
		{
			LOCK(cs_mapMasternodePaymentVotes);

			auto res = mapMasternodePaymentVotes.emplace(nHash, vote);

			// Avoid processing same vote multiple times if it was already verified earlier
			if (!res.second && res.first->second.IsVerified()) {
				LogPrint(BCLog::MNPAYMENT, "MASTERNODEPAYMENTVOTE -- hash=%s, nBlockHeight=%d/%d vote=%s, seen\n",
					nHash.ToString(), vote.nBlockHeight, nCachedBlockHeight, vote.ToString());
				return;
			}

			// Mark vote as non-verified when it's seen for the first time,
			// AddOrUpdatePaymentVote() below should take care of it if vote is actually ok
			res.first->second.MarkAsNotVerified();
		}
        int nFirstBlock = nCachedBlockHeight - GetStorageLimit();
        if(vote.nBlockHeight < nFirstBlock || vote.nBlockHeight > nCachedBlockHeight+20) {
            LogPrint(BCLog::MNPAYMENT, "MASTERNODEPAYMENTVOTE -- vote out of range: nFirstBlock=%d, nBlockHeight=%d, nHeight=%d\n", nFirstBlock, vote.nBlockHeight, nCachedBlockHeight);
            return;
        }

        std::string strError = "";
        if(!vote.IsValid(pfrom, nCachedBlockHeight, strError, connman)) {
            LogPrint(BCLog::MNPAYMENT, "MASTERNODEPAYMENTVOTE -- invalid message, error: %s\n", strError);
            return;
        }
        masternode_info_t mnInfo;
        if(!mnodeman.GetMasternodeInfo(vote.masternodeOutpoint, mnInfo)) {
            // mn was not found, so we can't check vote, some info is probably missing
            LogPrint(BCLog::MNPAYMENT, "MASTERNODEPAYMENTVOTE -- masternode is missing %s\n", vote.masternodeOutpoint.ToStringShort());
            mnodeman.AskForMN(pfrom, vote.masternodeOutpoint, connman);
            return;
        }

        int nDos = 0;
        if(!vote.CheckSignature(mnInfo.pubKeyMasternode, nCachedBlockHeight, nDos)) {
            if(nDos) {
                LogPrint(BCLog::MNPAYMENT, "MASTERNODEPAYMENTVOTE -- ERROR: invalid signature\n");
                LOCK(cs_main);
                Misbehaving(pfrom->GetId(), nDos);
            } else {
                // only warn about anything non-critical (i.e. nDos == 0) in debug mode
                LogPrint(BCLog::MNPAYMENT, "MASTERNODEPAYMENTVOTE -- WARNING: invalid signature\n");
            }
            // Either our info or vote info could be outdated.
            // In case our info is outdated, ask for an update,
            mnodeman.AskForMN(pfrom, vote.masternodeOutpoint, connman);
            // but there is nothing we can do if vote info itself is outdated
            // (i.e. it was signed by a mn which changed its key),
            // so just quit here.
            return;
        }
		// SYSCOIN update last vote after sig check
		if (!UpdateLastVote(vote)) {
			LogPrint(BCLog::MNPAYMENT, "MASTERNODEPAYMENTVOTE -- masternode already voted, masternode=%s\n", vote.masternodeOutpoint.ToStringShort());
			return;
		}

		LogPrint(BCLog::MNPAYMENT, "MASTERNODEPAYMENTVOTE -- hash=%s, nBlockHeight=%d/%d vote=%s, new\n",
			nHash.ToString(), vote.nBlockHeight, nCachedBlockHeight, vote.ToString());

        if(AddOrUpdatePaymentVote(vote, connman)){
            vote.Relay(connman);
            masternodeSync.BumpAssetLastTime("MASTERNODEPAYMENTVOTE");
        }
    }
}

uint256 CMasternodePaymentVote::GetHash() const
{
    // Note: doesn't match serialization

    CHashWriter ss(SER_GETHASH, MIN_PEER_PROTO_VERSION);
    ss << *(CScriptBase*)(&payee);
    ss << nBlockHeight;
	ss << nStartHeight;
    ss << masternodeOutpoint;
    return ss.GetHash();
}

uint256 CMasternodePaymentVote::GetSignatureHash() const
{
    return SerializeHash(*this);
}

bool CMasternodePaymentVote::Sign()
{
    std::string strError;

    if (sporkManager.IsSporkActive(SPORK_6_NEW_SIGS)) {
        uint256 hash = GetSignatureHash();

        if(!CHashSigner::SignHash(hash, activeMasternode.keyMasternode, vchSig)) {
            LogPrint(BCLog::MNPAYMENT, "CMasternodePaymentVote::Sign -- SignHash() failed\n");
            return false;
        }

        if (!CHashSigner::VerifyHash(hash, activeMasternode.pubKeyMasternode, vchSig, strError)) {
            LogPrint(BCLog::MNPAYMENT, "CMasternodePaymentVote::Sign -- VerifyHash() failed, error: %s\n", strError);
            return false;
        }
		LogPrint(BCLog::MNPAYMENT, "CMasternodePayments::Sign -- signed, hash=%s, vote=%s\n", GetHash().GetHex(), ToString());
    }
	

    return true;
}

bool CMasternodePayments::GetBlockPayee(int nBlockHeight, CScript& payeeRet) const
{
    auto it = mapMasternodeBlocks.find(nBlockHeight);
    return it != mapMasternodeBlocks.end() && it->second.GetBestPayee(payeeRet);
}
bool CMasternodePayments::GetBlockPayee(int nBlockHeight, CScript& payeeRet, int &nStartHeightBlock) const
{
	auto it = mapMasternodeBlocks.find(nBlockHeight);
	return it != mapMasternodeBlocks.end() && it->second.GetBestPayee(payeeRet, nStartHeightBlock);
}
// Is this masternode scheduled to get paid soon?
// -- Only look ahead up to 8 blocks to allow for propagation of the latest 2 blocks of votes
bool CMasternodePayments::IsScheduled(const masternode_info_t& mnInfo, int nNotBlockHeight) const
{
    LOCK(cs_mapMasternodeBlocks);

    if(!masternodeSync.IsMasternodeListSynced()) return false;

    CScript mnpayee;
    mnpayee = GetScriptForDestination(mnInfo.pubKeyCollateralAddress.GetID());

    CScript payee;
    for(int64_t h = nCachedBlockHeight; h <= nCachedBlockHeight + 8; h++){
        if(h == nNotBlockHeight) continue;
        if(GetBlockPayee(h, payee) && mnpayee == payee) {
            return true;
        }
    }

    return false;
}

bool CMasternodePayments::AddOrUpdatePaymentVote(const CMasternodePaymentVote& vote, CConnman& connman)
{
    uint256 blockHash = uint256();
    if(!GetBlockHash(blockHash, vote.nBlockHeight - 101)) return false;

    uint256 nVoteHash = vote.GetHash();

    if(HasVerifiedPaymentVote(nVoteHash)) return false;

    LOCK2(cs_mapMasternodeBlocks, cs_mapMasternodePaymentVotes);

    mapMasternodePaymentVotes[nVoteHash] = vote;

    auto it = mapMasternodeBlocks.emplace(vote.nBlockHeight, CMasternodeBlockPayees(vote.nBlockHeight)).first;
    it->second.AddPayee(vote, connman);

    LogPrint(BCLog::MNPAYMENT, "CMasternodePayments::AddOrUpdatePaymentVote -- added, hash=%s\n", nVoteHash.ToString());

    return true;
}

bool CMasternodePayments::HasVerifiedPaymentVote(const uint256& hashIn) const
{
    LOCK(cs_mapMasternodePaymentVotes);
    const auto it = mapMasternodePaymentVotes.find(hashIn);
    return it != mapMasternodePaymentVotes.end() && it->second.IsVerified();
}

void CMasternodeBlockPayees::AddPayee(const CMasternodePaymentVote& vote, CConnman& connman)
{
    LOCK(cs_vecPayees);

    uint256 nVoteHash = vote.GetHash();

    for (auto& payee : vecPayees) {
        if (payee.GetPayee() == vote.payee) {
            payee.AddVoteHash(nVoteHash);
            return;
        }
    }
	CMasternodePayee payeeNew(vote.payee, nVoteHash, vote.nStartHeight);
    vecPayees.push_back(payeeNew);
}

bool CMasternodeBlockPayees::GetBestPayee(CScript& payeeRet) const
{
    LOCK(cs_vecPayees);

    if(vecPayees.empty()) {
        LogPrint(BCLog::MNPAYMENT, "CMasternodeBlockPayees::GetBestPayee -- ERROR: couldn't find any payee\n");
        return false;
    }

    int nVotes = -1;
    for (const auto& payee : vecPayees) {
        if (payee.GetVoteCount() > nVotes) {
            payeeRet = payee.GetPayee();
            nVotes = payee.GetVoteCount();
        }
    }

    return (nVotes > -1);
}
bool CMasternodeBlockPayees::GetBestPayee(CScript& payeeRet, int& nStartHeightBlock) const
{
	LOCK(cs_vecPayees);

	if (vecPayees.empty()) {
		LogPrint(BCLog::MNPAYMENT, "CMasternodeBlockPayees::GetBestPayee -- ERROR: couldn't find any payee\n");
		return false;
	}

	int nVotes = -1;
	for (const auto& payee : vecPayees) {
		if (payee.GetVoteCount() > nVotes) {
			payeeRet = payee.GetPayee();
			nVotes = payee.GetVoteCount();
			nStartHeightBlock = payee.nStartHeight;
		}
	}

	return (nVotes > -1);
}
bool CMasternodeBlockPayees::HasPayeeWithVotes(const CScript& payeeIn, int nVotesReq, CMasternodePayee& payeeOut) const
{
    LOCK(cs_vecPayees);

    for (const auto& payee : vecPayees) {
        if (payee.GetVoteCount() >= nVotesReq && payee.GetPayee() == payeeIn) {
			payeeOut = payee;
            return true;
        }
    }

    LogPrint(BCLog::MNPAYMENT, "CMasternodeBlockPayees::HasPayeeWithVotes -- ERROR: couldn't find any payee with %d+ votes\n", nVotesReq);
    return false;
}

bool CMasternodeBlockPayees::IsTransactionValid(const CTransaction& txNew, const int64_t &nHeight, const CAmount& fee, CAmount& nTotalRewardWithMasternodes, bool retry) const
{
    LOCK(cs_vecPayees);
	const CAmount& nHalfFee = fee / 2;
    int nMaxSignatures = 0;
    std::string strPayeesPossible = "";

	const CChainParams& chainparams = Params();

    //require at least MNPAYMENTS_SIGNATURES_REQUIRED signatures


    for (const auto& payee : vecPayees) {
        if (payee.GetVoteCount() >= nMaxSignatures) {
            nMaxSignatures = payee.GetVoteCount();
        }
    }
	CAmount nMasternodePayment = 0;
	for (const auto& payee : vecPayees) {
		const CScript& payeeScript = payee.GetPayee();
		nMasternodePayment = GetBlockSubsidy(nHeight, chainparams.GetConsensus(), nTotalRewardWithMasternodes, false, true, payee.nStartHeight);
		nMasternodePayment += nHalfFee;
		bool bFoundPayment = false;
		for(const auto &txout: txNew.vout) {
			if (payeeScript == txout.scriptPubKey && nMasternodePayment == txout.nValue) {
				LogPrint(BCLog::MNPAYMENT, "CMasternodeBlockPayees::IsTransactionValid -- Found required payment\n");
				bFoundPayment = true;
			}
			if (bFoundPayment)
				break;
		}

        if (payee.GetVoteCount() >= MNPAYMENTS_SIGNATURES_REQUIRED) {
			if (bFoundPayment)
				return true;           

            CTxDestination address1;
            ExtractDestination(payeeScript, address1);

            if(strPayeesPossible == "") {
                strPayeesPossible = EncodeDestination(address1);
            } else {
                strPayeesPossible += "," + EncodeDestination(address1);
            }
        }
    }

	// if not enough sigs approve longest chain
	if (nMaxSignatures < MNPAYMENTS_SIGNATURES_REQUIRED) {
		nTotalRewardWithMasternodes = txNew.GetValueOut();
		nTotalRewardWithMasternodes  -= fee;
		return true;
	}
    if(!retry){
        // do check and try again incase of inconsistencies
        mnodeman.Check();
        if(IsTransactionValid(txNew, nHeight, fee, nTotalRewardWithMasternodes, true))
            return true;
    }
    LogPrint(BCLog::MNPAYMENT, "CMasternodeBlockPayees::IsTransactionValid -- ERROR: Missing required payment, possible payees: '%s', amount: %f SYS\n", strPayeesPossible, (float)nMasternodePayment/COIN);
    return false;
}

std::string CMasternodeBlockPayees::GetRequiredPaymentsString() const
{
    LOCK(cs_vecPayees);

    std::string strRequiredPayments = "";

    for (const auto& payee : vecPayees)
    {
        CTxDestination address1;
        ExtractDestination(payee.GetPayee(), address1);

        if (!strRequiredPayments.empty())
            strRequiredPayments += ", ";

        strRequiredPayments += strprintf("%s:%d:%d", EncodeDestination(address1), payee.GetVoteCount(), payee.nStartHeight);
    }

    if (strRequiredPayments.empty())
        return "Unknown";

    return strRequiredPayments;
}

std::string CMasternodePayments::GetRequiredPaymentsString(int nBlockHeight) const
{
    LOCK(cs_mapMasternodeBlocks);

    const auto it = mapMasternodeBlocks.find(nBlockHeight);
    return it == mapMasternodeBlocks.end() ? "Unknown" : it->second.GetRequiredPaymentsString();
}

bool CMasternodePayments::IsTransactionValid(const CTransaction& txNew, int nBlockHeight, const CAmount& fee, CAmount& nTotalRewardWithMasternodes) const
{
    LOCK(cs_mapMasternodeBlocks);

    const auto it = mapMasternodeBlocks.find(nBlockHeight);
	if (it == mapMasternodeBlocks.end()) {
		nTotalRewardWithMasternodes = txNew.GetValueOut();
		return true;
	}
	else {
		return it->second.IsTransactionValid(txNew, nBlockHeight, fee, nTotalRewardWithMasternodes);
	}
}

void CMasternodePayments::CheckAndRemove()
{
    if(!masternodeSync.IsBlockchainSynced()) return;

    LOCK2(cs_mapMasternodeBlocks, cs_mapMasternodePaymentVotes);

    int nLimit = GetStorageLimit();

    std::map<uint256, CMasternodePaymentVote>::iterator it = mapMasternodePaymentVotes.begin();
    while(it != mapMasternodePaymentVotes.end()) {
        CMasternodePaymentVote vote = (*it).second;

        if(nCachedBlockHeight - vote.nBlockHeight > nLimit) {
            LogPrint(BCLog::MNPAYMENT, "CMasternodePayments::CheckAndRemove -- Removing old Masternode payment: nBlockHeight=%d\n", vote.nBlockHeight);
            mapMasternodePaymentVotes.erase(it++);
            mapMasternodeBlocks.erase(vote.nBlockHeight);
        } else {
            ++it;
        }
    }
    LogPrint(BCLog::MNPAYMENT, "CMasternodePayments::CheckAndRemove -- %s\n", ToString());
}

bool CMasternodePaymentVote::IsValid(CNode* pnode, int nValidationHeight, std::string& strError, CConnman& connman) const
{
    masternode_info_t mnInfo;

    if(!mnodeman.GetMasternodeInfo(masternodeOutpoint, mnInfo)) {
        strError = strprintf("Unknown masternode=%s", masternodeOutpoint.ToStringShort());
        // Only ask if we are already synced and still have no idea about that Masternode
        if(masternodeSync.IsMasternodeListSynced()) {
            mnodeman.AskForMN(pnode, masternodeOutpoint, connman);
        }

        return false;
    }

    int nMinRequiredProtocol;
    if(nBlockHeight >= nValidationHeight) {
        // new votes must comply SPORK_10_MASTERNODE_PAY_UPDATED_NODES rules
        nMinRequiredProtocol = mnpayments.GetMinMasternodePaymentsProto();
    } else {
        // allow non-updated masternodes for old blocks
        nMinRequiredProtocol = MIN_MASTERNODE_PAYMENT_PROTO_VERSION_1;
    }

    if(mnInfo.nProtocolVersion < nMinRequiredProtocol) {
        strError = strprintf("Masternode protocol is too old: nProtocolVersion=%d, nMinRequiredProtocol=%d", mnInfo.nProtocolVersion, nMinRequiredProtocol);
        return false;
    }

    // Only masternodes should try to check masternode rank for old votes - they need to pick the right winner for future blocks.
    // Regular clients (miners included) need to verify masternode rank for future block votes only.
    if(!fMasternodeMode && nBlockHeight < nValidationHeight) return true;

    int nRank;

    if(!mnodeman.GetMasternodeRank(masternodeOutpoint, nRank, nBlockHeight - 101, nMinRequiredProtocol)) {
        LogPrint(BCLog::MNPAYMENT, "CMasternodePaymentVote::IsValid -- Can't calculate rank for masternode %s\n",
                    masternodeOutpoint.ToStringShort());
        return false;
    }

    if(nRank > MNPAYMENTS_SIGNATURES_TOTAL) {
        // It's common to have masternodes mistakenly think they are in the top 10
        // We don't want to print all of these messages in normal mode, debug mode should print though
        strError = strprintf("Masternode %s is not in the top %d (%d)", masternodeOutpoint.ToStringShort(), MNPAYMENTS_SIGNATURES_TOTAL, nRank);
        // Only ban for new mnw which is out of bounds, for old mnw MN list itself might be way too much off
        if(nRank > MNPAYMENTS_SIGNATURES_TOTAL*2 && nBlockHeight > nValidationHeight) {
            strError = strprintf("Masternode %s is not in the top %d (%d)", masternodeOutpoint.ToStringShort(), MNPAYMENTS_SIGNATURES_TOTAL*2, nRank);
            LogPrint(BCLog::MNPAYMENT, "CMasternodePaymentVote::IsValid -- Error: %s\n", strError);
            LOCK(cs_main);
            Misbehaving(pnode->GetId(), 20);
        }
        // Still invalid however
        return false;
    }

    return true;
}

bool CMasternodePayments::ProcessBlock(int nBlockHeight, CConnman& connman)
{
    // DETERMINE IF WE SHOULD BE VOTING FOR THE NEXT PAYEE

    if(fLiteMode || !fMasternodeMode) return false;

    // We have little chances to pick the right winner if winners list is out of sync
    // but we have no choice, so we'll try. However it doesn't make sense to even try to do so
    // if we have not enough data about masternodes.
    if(!masternodeSync.IsMasternodeListSynced()) return false;

    int nRank;

    if (!mnodeman.GetMasternodeRank(activeMasternode.outpoint, nRank, nBlockHeight - 101, GetMinMasternodePaymentsProto())) {
        LogPrint(BCLog::MNPAYMENT, "CMasternodePayments::ProcessBlock -- Unknown Masternode\n");
        return false;
    }

    if (nRank > MNPAYMENTS_SIGNATURES_TOTAL) {
        LogPrint(BCLog::MNPAYMENT, "CMasternodePayments::ProcessBlock -- Masternode not in the top %d (%d)\n", MNPAYMENTS_SIGNATURES_TOTAL, nRank);
        return false;
    }


    // LOCATE THE NEXT MASTERNODE WHICH SHOULD BE PAID

    LogPrint(BCLog::MNPAYMENT, "CMasternodePayments::ProcessBlock -- Start: nBlockHeight=%d, masternode=%s\n", nBlockHeight, activeMasternode.outpoint.ToStringShort());

    // pay to the oldest MN that still had no payment but its input is old enough and it was active long enough
    int nCount = 0;
    masternode_info_t mnInfo;

    if (!mnodeman.GetNextMasternodeInQueueForPayment(nBlockHeight, true, nCount, mnInfo)) {
        // run masternode check and check again incase of inconsistencies
        mnodeman.Check();
        if (!mnodeman.GetNextMasternodeInQueueForPayment(nBlockHeight, true, nCount, mnInfo)) {
            LogPrint(BCLog::MNPAYMENT, "CMasternodePayments::ProcessBlock -- ERROR: Failed to find masternode to pay\n");
            return false;
        }
    }

    LogPrint(BCLog::MNPAYMENT, "CMasternodePayments::ProcessBlock -- Masternode found by GetNextMasternodeInQueueForPayment(): %s\n", mnInfo.outpoint.ToStringShort());


    CScript payee = GetScriptForDestination(mnInfo.pubKeyCollateralAddress.GetID());

    CMasternodePaymentVote voteNew(activeMasternode.outpoint, nBlockHeight, payee, mnodeman.GetStartHeight(mnInfo));

    CTxDestination address1;
    ExtractDestination(payee, address1);

    LogPrint(BCLog::MNPAYMENT, "CMasternodePayments::ProcessBlock -- vote: payee=%s, nBlockHeight=%d\n", EncodeDestination(address1), nBlockHeight);

    // SIGN MESSAGE TO NETWORK WITH OUR MASTERNODE KEYS

    LogPrint(BCLog::MNPAYMENT, "CMasternodePayments::ProcessBlock -- Signing vote\n");
    if (voteNew.Sign()) {
        LogPrint(BCLog::MNPAYMENT, "CMasternodePayments::ProcessBlock -- AddOrUpdatePaymentVote()\n");

        if (AddOrUpdatePaymentVote(voteNew, connman)) {
            voteNew.Relay(connman);
            return true;
        }
    }

    return false;
}

void CMasternodePayments::CheckBlockVotes(int nBlockHeight)
{
    if (!masternodeSync.IsWinnersListSynced()) return;

    CMasternodeMan::rank_pair_vec_t mns;
    if (!mnodeman.GetMasternodeRanks(mns, nBlockHeight - 101, GetMinMasternodePaymentsProto())) {
        LogPrint(BCLog::MNPAYMENT, "CMasternodePayments::CheckBlockVotes -- nBlockHeight=%d, GetMasternodeRanks failed\n", nBlockHeight);
        return;
    }

    std::string debugStr;

    debugStr += strprintf("CMasternodePayments::CheckBlockVotes -- nBlockHeight=%d,\n  Expected voting MNs:\n", nBlockHeight);

    LOCK2(cs_mapMasternodeBlocks, cs_mapMasternodePaymentVotes);

    int i{0};
    for (const auto& mn : mns) {
        CScript payee;
        bool found = false;

        const auto it = mapMasternodeBlocks.find(nBlockHeight);
        if (it != mapMasternodeBlocks.end()) {
            for (const auto& p : it->second.vecPayees) {
                for (const auto& voteHash : p.GetVoteHashes()) {
                    const auto itVote = mapMasternodePaymentVotes.find(voteHash);
                    if (itVote == mapMasternodePaymentVotes.end()) {
                        debugStr += strprintf("    - could not find vote %s\n",
                                              voteHash.ToString());
                        continue;
                    }
                    if (itVote->second.masternodeOutpoint == mn.second.outpoint) {
                        payee = itVote->second.payee;
                        found = true;
                        break;
                    }
                }
            }
        }

        if (found) {
            CTxDestination address1;
            ExtractDestination(payee, address1);

            debugStr += strprintf("    - %s - voted for %s\n",
                                  mn.second.outpoint.ToStringShort(), EncodeDestination(address1));
        } else {
            mapMasternodesDidNotVote.emplace(mn.second.outpoint, 0).first->second++;

            debugStr += strprintf("    - %s - no vote received\n",
                                  mn.second.outpoint.ToStringShort());
        }

        if (++i >= MNPAYMENTS_SIGNATURES_TOTAL) break;
    }

    if (mapMasternodesDidNotVote.empty()) {
        LogPrint(BCLog::MNPAYMENT, "%s", debugStr);
        return;
    }

    debugStr += "  Masternodes which missed a vote in the past:\n";
    for (const auto& item : mapMasternodesDidNotVote) {
        debugStr += strprintf("    - %s: %d\n", item.first.ToStringShort(), item.second);
    }

    LogPrint(BCLog::MNPAYMENT, "%s", debugStr);
}

void CMasternodePaymentVote::Relay(CConnman& connman) const
{
    // Do not relay until fully synced
    if(!masternodeSync.IsSynced()) {
        LogPrint(BCLog::MNPAYMENT, "CMasternodePayments::Relay -- won't relay until fully synced\n");
        return;
    }

    CInv inv(MSG_MASTERNODE_PAYMENT_VOTE, GetHash());
    g_connman->RelayInv(inv);
}

bool CMasternodePaymentVote::CheckSignature(const CPubKey& pubKeyMasternode, int nValidationHeight, int &nDos) const
{
    // do not ban by default
    nDos = 0;
    std::string strError = "";

    if (sporkManager.IsSporkActive(SPORK_6_NEW_SIGS)) {
        uint256 hash = GetSignatureHash();

        if (!CHashSigner::VerifyHash(hash, pubKeyMasternode, vchSig, strError)) {
            // nope, not in old format either
            // Only ban for future block vote when we are already synced.
            // Otherwise it could be the case when MN which signed this vote is using another key now
            // and we have no idea about the old one.
            if(masternodeSync.IsMasternodeListSynced() && nBlockHeight > nValidationHeight) {
                nDos = 20;
            }
            return error("CMasternodePaymentVote::CheckSignature -- Got bad Masternode payment signature, signaturehash=%s, hash=%s, vote=%s, error: %s",
                        hash.GetHex(), GetHash().GetHex(), ToString(), strError);
            
        }
    } 

    return true;
}

std::string CMasternodePaymentVote::ToString() const
{
    std::ostringstream info;
	    info << masternodeOutpoint.ToStringShort() <<
            ", " << nBlockHeight <<
			", " << nStartHeight <<
            ", " << ScriptToAsmStr(payee) <<
            ", " << (int)vchSig.size();

    return info.str();
}

// Send only votes for future blocks, node should request every other missing payment block individually
void CMasternodePayments::Sync(CNode* pnode, CConnman& connman) const
{
    LOCK(cs_mapMasternodeBlocks);

    if(!masternodeSync.IsWinnersListSynced()) return;

    int nInvCount = 0;

    for(int h = nCachedBlockHeight; h < nCachedBlockHeight + 20; h++) {
        const auto it = mapMasternodeBlocks.find(h);
        if(it != mapMasternodeBlocks.end()) {
            for (const auto& payee : it->second.vecPayees) {
                std::vector<uint256> vecVoteHashes = payee.GetVoteHashes();
                for (const auto& hash : vecVoteHashes) {
                    if(!HasVerifiedPaymentVote(hash)) continue;
                    pnode->PushInventory(CInv(MSG_MASTERNODE_PAYMENT_VOTE, hash));
                    nInvCount++;
                }
            }
        }
    }

    LogPrint(BCLog::MNPAYMENT, "CMasternodePayments::Sync -- Sent %d votes to peer=%d\n", nInvCount, pnode->GetId());
    CNetMsgMaker msgMaker(pnode->GetSendVersion());
    connman.PushMessage(pnode, msgMaker.Make(NetMsgType::SYNCSTATUSCOUNT, MASTERNODE_SYNC_MNW, nInvCount));
}

// Request low data/unknown payment blocks in batches directly from some node instead of/after preliminary Sync.
void CMasternodePayments::RequestLowDataPaymentBlocks(CNode* pnode, CConnman& connman) const
{
    if(!masternodeSync.IsMasternodeListSynced()) return;

    CNetMsgMaker msgMaker(pnode->GetSendVersion());
    LOCK2(cs_main, cs_mapMasternodeBlocks);

    std::vector<CInv> vToFetch;
    int nLimit = GetStorageLimit();

    const CBlockIndex *pindex = chainActive.Tip();

    while(nCachedBlockHeight - pindex->nHeight < nLimit) {
        const auto it = mapMasternodeBlocks.find(pindex->nHeight);
        if(it == mapMasternodeBlocks.end()) {
            // We have no idea about this block height, let's ask
            vToFetch.push_back(CInv(MSG_MASTERNODE_PAYMENT_BLOCK, pindex->GetBlockHash()));
            // We should not violate GETDATA rules
            if(vToFetch.size() == MAX_INV_SZ) {
                LogPrint(BCLog::MNPAYMENT, "CMasternodePayments::RequestLowDataPaymentBlocks -- asking peer=%d for %d blocks\n", pnode->GetId(), MAX_INV_SZ);
                connman.PushMessage(pnode, msgMaker.Make(NetMsgType::GETDATA, vToFetch));
                // Start filling new batch
                vToFetch.clear();
            }
        }
        if(!pindex->pprev) break;
        pindex = pindex->pprev;
    }

    for (auto& mnBlockPayees : mapMasternodeBlocks) {
        int nBlockHeight = mnBlockPayees.first;
        int nTotalVotes = 0;
        bool fFound = false;
        for (const auto& payee : mnBlockPayees.second.vecPayees) {
            if(payee.GetVoteCount() >= MNPAYMENTS_SIGNATURES_REQUIRED) {
                fFound = true;
                break;
            }
            nTotalVotes += payee.GetVoteCount();
        }
        // A clear winner (MNPAYMENTS_SIGNATURES_REQUIRED+ votes) was found
        // or no clear winner was found but there are at least avg number of votes
        if(fFound || nTotalVotes >= (MNPAYMENTS_SIGNATURES_TOTAL + MNPAYMENTS_SIGNATURES_REQUIRED)/2) {
            // so just move to the next block
            continue;
        }
        // DEBUG
        /*DBG (
            // Let's see why this failed
            for (const auto& payee : mnBlockPayees.second.vecPayees) {
                CTxDestination address1;
                ExtractDestination(payee.GetPayee(), address1);
                printf("payee %s votes %d\n", EncodeDestination(address1).c_str(), payee.GetVoteCount());
            }
            printf("block %d votes total %d\n", nBlockHeight, nTotalVotes);
        )*/
        // END DEBUG
        // Low data block found, let's try to sync it
        uint256 hash;
        if(GetBlockHash(hash, nBlockHeight)) {
            vToFetch.push_back(CInv(MSG_MASTERNODE_PAYMENT_BLOCK, hash));
        }
        // We should not violate GETDATA rules
        if(vToFetch.size() == MAX_INV_SZ) {
            LogPrint(BCLog::MNPAYMENT, "CMasternodePayments::RequestLowDataPaymentBlocks -- asking peer=%d for %d payment blocks\n", pnode->GetId(), MAX_INV_SZ);
            connman.PushMessage(pnode, msgMaker.Make(NetMsgType::GETDATA, vToFetch));
            // Start filling new batch
            vToFetch.clear();
        }
    }
    // Ask for the rest of it
    if(!vToFetch.empty()) {
        LogPrint(BCLog::MNPAYMENT, "CMasternodePayments::RequestLowDataPaymentBlocks -- asking peer=%d for %d payment blocks\n", pnode->GetId(), vToFetch.size());
        connman.PushMessage(pnode, msgMaker.Make(NetMsgType::GETDATA, vToFetch));
    }
}

std::string CMasternodePayments::ToString() const
{
    std::ostringstream info;

    info << "Votes: " << (int)mapMasternodePaymentVotes.size() <<
            ", Blocks: " << (int)mapMasternodeBlocks.size();

    return info.str();
}

bool CMasternodePayments::IsEnoughData() const
{
    float nAverageVotes = (MNPAYMENTS_SIGNATURES_TOTAL + MNPAYMENTS_SIGNATURES_REQUIRED) / 2;
    int nStorageLimit = GetStorageLimit();
    return GetBlockCount() > nStorageLimit && GetVoteCount() > nStorageLimit * nAverageVotes;
}

int CMasternodePayments::GetStorageLimit() const
{
    return std::max(int(mnodeman.size() * nStorageCoeff), nMinBlocksToStore);
}

void CMasternodePayments::UpdatedBlockTip(const CBlockIndex *pindex, CConnman& connman)
{
    if(!pindex) return;

    nCachedBlockHeight = pindex->nHeight;
    LogPrint(BCLog::MNPAYMENT, "CMasternodePayments::UpdatedBlockTip -- nCachedBlockHeight=%d\n", nCachedBlockHeight);
 
    int nFutureBlock = nCachedBlockHeight + 10;

    CheckBlockVotes(nFutureBlock - 1);
    ProcessBlock(nFutureBlock, connman);
}
