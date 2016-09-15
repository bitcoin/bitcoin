// Copyright (c) 2014-2016 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "activemasternode.h"
#include "addrman.h"
#include "darksend.h"
#include "governance-classes.h"
#include "masternode-payments.h"
#include "masternode-sync.h"
#include "masternodeman.h"
#include "spork.h"
#include "util.h"

#include <boost/lexical_cast.hpp>

/** Object for who's going to get paid on which blocks */
CMasternodePayments mnpayments;

CCriticalSection cs_vecPayees;
CCriticalSection cs_mapMasternodeBlocks;
CCriticalSection cs_mapMasternodePayeeVotes;

/**
* IsBlockValueValid
*
*   Determine if coinbase outgoing created money is the correct value
*
*   Why is this needed?
*   - In Dash some blocks are superblocks, which output much higher amounts of coins
*   - Otherblocks are 10% lower in outgoing value, so in total, no extra coins are created
*   - When non-superblocks are detected, the normal schedule should be maintained
*/

bool IsBlockValueValid(const CBlock& block, int nBlockHeight, CAmount blockReward)
{
    bool isNormalBlockValueMet = (block.vtx[0].GetValueOut() <= blockReward);
    if(fDebug) LogPrintf("block.vtx[0].GetValueOut() %lld <= blockReward %lld\n", block.vtx[0].GetValueOut(), blockReward);

    // we are still using budgets, but we have no data about them anymore,
    // all we know is predefined budget cycle and window

    const Consensus::Params& consensusParams = Params().GetConsensus();

    if(nBlockHeight < consensusParams.nSuperblockStartBlock) {
        int nOffset = nBlockHeight % consensusParams.nBudgetPaymentsCycleBlocks;
        if(nBlockHeight >= consensusParams.nBudgetPaymentsStartBlock &&
            nOffset < consensusParams.nBudgetPaymentsWindowBlocks) {
            // NOTE: make sure SPORK_13_OLD_SUPERBLOCK_FLAG is disabled when 12.1 starts to go live
            if(masternodeSync.IsSynced() && !sporkManager.IsSporkActive(SPORK_13_OLD_SUPERBLOCK_FLAG)) {
                // no budget blocks should be accepted here, if SPORK_13_OLD_SUPERBLOCK_FLAG is disabled
                LogPrint("gobject", "IsBlockValueValid -- Client synced but budget spork is disabled, checking block value against normal block reward\n");
                return isNormalBlockValueMet;
            }
            LogPrint("gobject", "IsBlockValueValid -- WARNING: Skipping budget block value checks, accepting block\n");
            // TODO: reprocess blocks to make sure they are legit?
            return true;
        }
        // LogPrint("gobject", "IsBlockValueValid -- Block is not in budget cycle window, checking block value against normal block reward\n");
        return isNormalBlockValueMet;
    }

    // superblocks started

    CAmount nSuperblockPaymentsLimit = CSuperblock::GetPaymentsLimit(nBlockHeight);
    bool isSuperblockMaxValueMet = (block.vtx[0].GetValueOut() <= blockReward + nSuperblockPaymentsLimit);

    LogPrint("gobject", "block.vtx[0].GetValueOut() %lld <= nSuperblockPaymentsLimit %lld\n", block.vtx[0].GetValueOut(), nSuperblockPaymentsLimit);

    if(!masternodeSync.IsSynced()) {
        // not enough data but at least it must NOT exceed superblock max value
        if(CSuperblock::IsValidBlockHeight(nBlockHeight)) {
            if(fDebug) LogPrintf("IsBlockPayeeValid -- WARNING: Client not synced, checking superblock max bounds only\n");
            return isSuperblockMaxValueMet;
        }
        // it MUST be a regular block otherwise
        return isNormalBlockValueMet;
    }

    // we are synced, let's try to check as much data as we can

    if(CSuperblockManager::IsSuperblockTriggered(nBlockHeight)) {
        if(CSuperblockManager::IsValid(block.vtx[0], nBlockHeight, blockReward)) {
            LogPrint("gobject", "IsBlockValueValid -- Valid superblock at height %d: %s", nBlockHeight, block.vtx[0].ToString());
            // all checks are done in CSuperblock::IsValid, nothing to do here
            return true;
        }

        // triggered but invalid? that's weird
        if(sporkManager.IsSporkActive(SPORK_9_SUPERBLOCKS_ENABLED)) {
            LogPrintf("IsBlockValueValid -- ERROR: Invalid superblock detected at height %d: %s", nBlockHeight, block.vtx[0].ToString());
            // should NOT allow invalid superblocks, when superblock enforcement is enabled
            return false;
        }

        // should NOT allow superblocks at all, when superblock enforcement is disabled
        LogPrintf("IsBlockValueValid -- Superblock enforcement is disabled, no superblocks allowed\n");
    }

    LogPrint("gobject", "IsBlockValueValid -- No valid superblock detected at height %d\n", nBlockHeight);
    // it MUST be a regular block
    return isNormalBlockValueMet;
}

bool IsBlockPayeeValid(const CTransaction& txNew, int nBlockHeight, CAmount blockReward)
{
    if(!masternodeSync.IsSynced()) {
        //there is no budget data to use to check anything, let's just accept the longest chain
        if(fDebug) LogPrintf("IsBlockPayeeValid -- WARNING: Client not synced, skipping block payee checks\n");
        return true;
    }

    // we are still using budgets, but we have no data about them anymore,
    // we can only check masternode payments

    const Consensus::Params& consensusParams = Params().GetConsensus();

    if(nBlockHeight < consensusParams.nSuperblockStartBlock) {
        if(mnpayments.IsTransactionValid(txNew, nBlockHeight)) {
            LogPrint("mnpayments", "IsBlockPayeeValid -- Valid masternode payment at height %d: %s", nBlockHeight, txNew.ToString());
            return true;
        }

        int nOffset = nBlockHeight % consensusParams.nBudgetPaymentsCycleBlocks;
        if(nBlockHeight >= consensusParams.nBudgetPaymentsStartBlock &&
            nOffset < consensusParams.nBudgetPaymentsWindowBlocks) {
            if(!sporkManager.IsSporkActive(SPORK_13_OLD_SUPERBLOCK_FLAG)) {
                // no budget blocks should be accepted here, if SPORK_13_OLD_SUPERBLOCK_FLAG is disabled
                LogPrint("gobject", "IsBlockPayeeValid -- ERROR: Client synced but budget spork is disabled and masternode payment is invalid\n");
                return false;
            }
            // NOTE: this should never happen in real, SPORK_13_OLD_SUPERBLOCK_FLAG MUST be disabled when 12.1 starts to go live
            LogPrint("gobject", "IsBlockPayeeValid -- WARNING: Probably valid budget block, have no data, accepting\n");
            // TODO: reprocess blocks to make sure they are legit?
            return true;
        }

        if(sporkManager.IsSporkActive(SPORK_8_MASTERNODE_PAYMENT_ENFORCEMENT)) {
            LogPrintf("IsBlockPayeeValid -- ERROR: Invalid masternode payment detected at height %d: %s", nBlockHeight, txNew.ToString());
            return false;
        }

        LogPrintf("IsBlockPayeeValid -- WARNING: Masternode payment enforcement is disabled, accepting any payee\n");
        return true;
    }

    // superblocks started
    // SEE IF THIS IS A VALID SUPERBLOCK

    if(CSuperblockManager::IsSuperblockTriggered(nBlockHeight)) {
        if(CSuperblockManager::IsValid(txNew, nBlockHeight, blockReward)) {
            LogPrint("gobject", "IsBlockPayeeValid -- Valid superblock at height %d: %s", nBlockHeight, txNew.ToString());
            return true;
        }

        if(sporkManager.IsSporkActive(SPORK_9_SUPERBLOCKS_ENABLED)) {
            LogPrintf("IsBlockPayeeValid -- ERROR: Invalid superblock detected at height %d: %s", nBlockHeight, txNew.ToString());
            // should NOT allow such superblocks, when superblock enforcement is enabled
            return false;
        }

        // should NOT allow superblocks at all, when superblock enforcement is disabled
        LogPrintf("IsBlockPayeeValid -- Superblock enforcement is disabled, no superblocks allowed\n");
    }

    // continue validation, should pay MN
    LogPrint("gobject", "IsBlockPayeeValid -- No valid superblock detected at height %d\n", nBlockHeight);

    // IF THIS ISN'T A SUPERBLOCK OR SUPERBLOCK IS INVALID, IT SHOULD PAY A MASTERNODE DIRECTLY
    if(mnpayments.IsTransactionValid(txNew, nBlockHeight)) {
        LogPrint("mnpayments", "IsBlockPayeeValid -- Valid masternode payment at height %d: %s", nBlockHeight, txNew.ToString());
        return true;
    }

    if(sporkManager.IsSporkActive(SPORK_8_MASTERNODE_PAYMENT_ENFORCEMENT)) {
        LogPrintf("IsBlockPayeeValid -- ERROR: Invalid masternode payment detected at height %d: %s", nBlockHeight, txNew.ToString());
        return false;
    }

    LogPrintf("IsBlockPayeeValid -- WARNING: Masternode payment enforcement is disabled, accepting any payee\n");
    return true;
}

void FillBlockPayments(CMutableTransaction& txNew, int nBlockHeight, CAmount blockReward, CTxOut& txoutMasternodeRet, std::vector<CTxOut>& voutSuperblockRet)
{
    // only create superblocks if spork is enabled AND if superblock is actually triggered
    // (height should be validated inside)
    if(sporkManager.IsSporkActive(SPORK_9_SUPERBLOCKS_ENABLED) &&
        CSuperblockManager::IsSuperblockTriggered(nBlockHeight)) {
            LogPrint("gobject", "FillBlockPayments -- triggered superblock creation at height %d\n", nBlockHeight);
            CSuperblockManager::CreateSuperblock(txNew, nBlockHeight, voutSuperblockRet);
            return;
    }

    // FILL BLOCK PAYEE WITH MASTERNODE PAYMENT OTHERWISE
    mnpayments.FillBlockPayee(txNew, nBlockHeight, blockReward, txoutMasternodeRet);
    LogPrint("mnpayments", "FillBlockPayments -- nBlockHeight %d blockReward %lld txoutMasternodeRet %s txNew %s",
                            nBlockHeight, blockReward, txoutMasternodeRet.ToString(), txNew.ToString());
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
    LOCK2(cs_mapMasternodeBlocks, cs_mapMasternodePayeeVotes);
    mapMasternodeBlocks.clear();
    mapMasternodePayeeVotes.clear();
}

bool CMasternodePayments::CanVote(COutPoint outMasternode, int nBlockHeight)
{
    LOCK(cs_mapMasternodePayeeVotes);

    if (mapMasternodesLastVote.count(outMasternode) && mapMasternodesLastVote[outMasternode] == nBlockHeight) {
        return false;
    }

    //record this masternode voted
    mapMasternodesLastVote[outMasternode] = nBlockHeight;
    return true;
}

/**
*   FillBlockPayee
*
*   Fill Masternode ONLY payment block
*/

void CMasternodePayments::FillBlockPayee(CMutableTransaction& txNew, int nBlockHeight, CAmount blockReward, CTxOut& txoutMasternodeRet)
{
    // make sure it's not filled yet
    txoutMasternodeRet = CTxOut();

    CScript payee;

    if(!mnpayments.GetBlockPayee(nBlockHeight, payee)) {
        // no masternode detected...
        CMasternode* winningNode = mnodeman.GetCurrentMasterNode();
        if(!winningNode) {
            // ...and we can't calculate it on our own
            LogPrintf("CMasternodePayments::FillBlockPayee -- Failed to detect masternode to pay\n");
            return;
        }
        // fill payee with locally calculated winner and hope for the best
        payee = GetScriptForDestination(winningNode->pubkey.GetID());
    }

    // GET MASTERNODE PAYMENT VARIABLES SETUP
    CAmount masternodePayment = GetMasternodePayment(nBlockHeight, blockReward);

    // split reward between miner ...
    txNew.vout[0].nValue -= masternodePayment;
    // ... and masternode
    txoutMasternodeRet = CTxOut(masternodePayment, payee);
    txNew.vout.push_back(txoutMasternodeRet);

    CTxDestination address1;
    ExtractDestination(payee, address1);
    CBitcoinAddress address2(address1);

    LogPrintf("CMasternodePayments::FillBlockPayee -- Masternode payment %lld to %s\n", masternodePayment, address2.ToString());
}

int CMasternodePayments::GetMinMasternodePaymentsProto() {
    return sporkManager.IsSporkActive(SPORK_10_MASTERNODE_PAY_UPDATED_NODES)
            ? MIN_MASTERNODE_PAYMENT_PROTO_VERSION_2
            : MIN_MASTERNODE_PAYMENT_PROTO_VERSION_1;
}

void CMasternodePayments::ProcessMessage(CNode* pfrom, std::string& strCommand, CDataStream& vRecv)
{
    // Ignore any payments messages until masternode list is synced
    if(!masternodeSync.IsMasternodeListSynced()) return;

    if(fLiteMode) return; // disable all Dash specific functionality

    if (strCommand == NetMsgType::MNWINNERSSYNC) { //Masternode Payments Request Sync

        // Ignore such requests until we are fully synced.
        // We could start processing this after masternode list is synced
        // but this is a heavy one so it's better to finish sync first.
        if (!masternodeSync.IsSynced()) return;

        int nCountNeeded;
        vRecv >> nCountNeeded;

        if(Params().NetworkIDString() == CBaseChainParams::MAIN){
            if(pfrom->HasFulfilledRequest(NetMsgType::MNWINNERSSYNC)) {
                LogPrintf("MNWINNERSSYNC -- peer already asked me for the list, peer=%d\n", pfrom->id);
                Misbehaving(pfrom->GetId(), 20);
                return;
            }
        }

        pfrom->FulfilledRequest(NetMsgType::MNWINNERSSYNC);
        Sync(pfrom, nCountNeeded);
        LogPrintf("MNWINNERSSYNC -- Sent Masternode winners to peer %d\n", pfrom->id);

    } else if (strCommand == NetMsgType::MNWINNER) { //Masternode Payments Declare Winner

        CMasternodePaymentWinner winner;
        vRecv >> winner;

        if(pfrom->nVersion < GetMinMasternodePaymentsProto()) return;

        if(!pCurrentBlockIndex) return;

        if(mapMasternodePayeeVotes.count(winner.GetHash())) {
            LogPrint("mnpayments", "MNWINNER -- hash=%s, nHeight=%d seen\n", winner.GetHash().ToString(), pCurrentBlockIndex->nHeight);
            masternodeSync.AddedMasternodeWinner();
            return;
        }

        int nFirstBlock = pCurrentBlockIndex->nHeight - GetStorageLimit();
        if(winner.nBlockHeight < nFirstBlock || winner.nBlockHeight > pCurrentBlockIndex->nHeight+20) {
            LogPrint("mnpayments", "MNWINNER -- winner out of range: nFirstBlock=%d, nBlockHeight=%d, nHeight=%d\n", nFirstBlock, winner.nBlockHeight, pCurrentBlockIndex->nHeight);
            return;
        }

        std::string strError = "";
        if(!winner.IsValid(pfrom, pCurrentBlockIndex->nHeight, strError)) {
            LogPrint("mnpayments", "MNWINNER -- invalid message, error: %s\n", strError);
            return;
        }

        if(!CanVote(winner.vinMasternode.prevout, winner.nBlockHeight)) {
            LogPrintf("MNWINNER -- masternode already voted, masternode=%s\n", winner.vinMasternode.prevout.ToStringShort());
            return;
        }

        if(!winner.CheckSignature()) {
            // do not ban for old mnw, MN simply might be not active anymore
            if(masternodeSync.IsSynced() && winner.nBlockHeight > pCurrentBlockIndex->nHeight) {
                LogPrintf("MNWINNER -- invalid signature\n");
                Misbehaving(pfrom->GetId(), 20);
            }
            // it could just be a non-synced masternode
            mnodeman.AskForMN(pfrom, winner.vinMasternode);
            return;
        }

        CTxDestination address1;
        ExtractDestination(winner.payee, address1);
        CBitcoinAddress address2(address1);

        LogPrint("mnpayments", "MNWINNER -- winning vote: address=%s, nBlockHeight=%d, nHeight=%d, prevout=%s\n", address2.ToString(), winner.nBlockHeight, pCurrentBlockIndex->nHeight, winner.vinMasternode.prevout.ToStringShort());

        if(AddWinningMasternode(winner)){
            winner.Relay();
            masternodeSync.AddedMasternodeWinner();
        }
    }
}

bool CMasternodePaymentWinner::Sign()
{
    std::string strError;
    std::string strMessage = vinMasternode.prevout.ToStringShort() +
                boost::lexical_cast<std::string>(nBlockHeight) +
                ScriptToAsmStr(payee);

    if(!darkSendSigner.SignMessage(strMessage, vchSig, activeMasternode.keyMasternode)) {
        LogPrintf("CMasternodePaymentWinner::Sign -- SignMessage() failed\n");
        return false;
    }

    if(!darkSendSigner.VerifyMessage(activeMasternode.pubKeyMasternode, vchSig, strMessage, strError)) {
        LogPrintf("CMasternodePaymentWinner::Sign -- VerifyMessage() failed, error: %s\n", strError);
        return false;
    }

    return true;
}

bool CMasternodePayments::GetBlockPayee(int nBlockHeight, CScript& payee)
{
    if(mapMasternodeBlocks.count(nBlockHeight)){
        return mapMasternodeBlocks[nBlockHeight].GetPayee(payee);
    }

    return false;
}

// Is this masternode scheduled to get paid soon?
// -- Only look ahead up to 8 blocks to allow for propagation of the latest 2 winners
bool CMasternodePayments::IsScheduled(CMasternode& mn, int nNotBlockHeight)
{
    LOCK(cs_mapMasternodeBlocks);

    if(!pCurrentBlockIndex) return false;

    CScript mnpayee;
    mnpayee = GetScriptForDestination(mn.pubkey.GetID());

    CScript payee;
    for(int64_t h = pCurrentBlockIndex->nHeight; h <= pCurrentBlockIndex->nHeight + 8; h++){
        if(h == nNotBlockHeight) continue;
        if(mapMasternodeBlocks.count(h) && mapMasternodeBlocks[h].GetPayee(payee) && mnpayee == payee) {
            return true;
        }
    }

    return false;
}

bool CMasternodePayments::AddWinningMasternode(CMasternodePaymentWinner& winnerIn)
{
    uint256 blockHash = uint256();
    if(!GetBlockHash(blockHash, winnerIn.nBlockHeight - 101)) return false;

    LOCK2(cs_mapMasternodePayeeVotes, cs_mapMasternodeBlocks);

    if(mapMasternodePayeeVotes.count(winnerIn.GetHash())) return false;

    mapMasternodePayeeVotes[winnerIn.GetHash()] = winnerIn;

    if(!mapMasternodeBlocks.count(winnerIn.nBlockHeight)) {
       CMasternodeBlockPayees blockPayees(winnerIn.nBlockHeight);
       mapMasternodeBlocks[winnerIn.nBlockHeight] = blockPayees;
    }

    mapMasternodeBlocks[winnerIn.nBlockHeight].AddPayee(winnerIn);

    return true;
}

void CMasternodeBlockPayees::AddPayee(CMasternodePaymentWinner winner)
{
    LOCK(cs_vecPayees);

    BOOST_FOREACH(CMasternodePayee& payee, vecPayees) {
        if (payee.scriptPubKey == winner.payee) {
            payee.nVotes++;
            return;
        }
    }
    CMasternodePayee payeeNew(winner.payee, 1);
    vecPayees.push_back(payeeNew);
}

bool CMasternodeBlockPayees::GetPayee(CScript& payeeRet)
{
    LOCK(cs_vecPayees);

    if(!vecPayees.size()) {
        LogPrint("mnpayments", "CMasternodeBlockPayees::GetPayee -- ERROR: couldn't find any payee\n");
        return false;
    }

    int nVotes = -1;
    BOOST_FOREACH(CMasternodePayee& payee, vecPayees) {
        if (payee.nVotes > nVotes) {
            payeeRet = payee.scriptPubKey;
            nVotes = payee.nVotes;
        }
    }

    return (nVotes > -1);
}

bool CMasternodeBlockPayees::HasPayeeWithVotes(CScript payeeIn, int nVotesReq)
{
    LOCK(cs_vecPayees);

    BOOST_FOREACH(CMasternodePayee& payee, vecPayees) {
        if (payee.nVotes >= nVotesReq && payee.scriptPubKey == payeeIn) {
            return true;
        }
    }

    LogPrint("mnpayments", "CMasternodeBlockPayees::HasPayeeWithVotes -- ERROR: couldn't find any payee with %d+ votes\n", nVotesReq);
    return false;
}

bool CMasternodeBlockPayees::IsTransactionValid(const CTransaction& txNew)
{
    LOCK(cs_vecPayees);

    int nMaxSignatures = 0;
    std::string strPayeesPossible = "";

    CAmount nMasternodePayment = GetMasternodePayment(nBlockHeight, txNew.GetValueOut());

    //require at least MNPAYMENTS_SIGNATURES_REQUIRED signatures

    BOOST_FOREACH(CMasternodePayee& payee, vecPayees) {
        if (payee.nVotes >= nMaxSignatures) {
            nMaxSignatures = payee.nVotes;
        }
    }

    // if we don't have at least MNPAYMENTS_SIGNATURES_REQUIRED signatures on a payee, approve whichever is the longest chain
    if(nMaxSignatures < MNPAYMENTS_SIGNATURES_REQUIRED) return true;

    BOOST_FOREACH(CMasternodePayee& payee, vecPayees) {
        if (payee.nVotes >= MNPAYMENTS_SIGNATURES_REQUIRED) {
            BOOST_FOREACH(CTxOut txout, txNew.vout) {
                if (payee.scriptPubKey == txout.scriptPubKey && nMasternodePayment == txout.nValue) {
                    LogPrint("mnpayments", "CMasternodeBlockPayees::IsTransactionValid -- Found required payment\n");
                    return true;
                }
            }

            CTxDestination address1;
            ExtractDestination(payee.scriptPubKey, address1);
            CBitcoinAddress address2(address1);

            if(strPayeesPossible == "") {
                strPayeesPossible = address2.ToString();
            } else {
                strPayeesPossible += "," + address2.ToString();
            }
        }
    }

    LogPrintf("CMasternodeBlockPayees::IsTransactionValid -- ERROR: Missing required payment, possible payees: '%s', amount: %f DASH\n", strPayeesPossible, (float)nMasternodePayment/COIN);
    return false;
}

std::string CMasternodeBlockPayees::GetRequiredPaymentsString()
{
    LOCK(cs_vecPayees);

    std::string strRequiredPayments = "Unknown";

    BOOST_FOREACH(CMasternodePayee& payee, vecPayees)
    {
        CTxDestination address1;
        ExtractDestination(payee.scriptPubKey, address1);
        CBitcoinAddress address2(address1);

        if (strRequiredPayments != "Unknown") {
            strRequiredPayments += ", " + address2.ToString() + ":" + boost::lexical_cast<std::string>(payee.nVotes);
        } else {
            strRequiredPayments = address2.ToString() + ":" + boost::lexical_cast<std::string>(payee.nVotes);
        }
    }

    return strRequiredPayments;
}

std::string CMasternodePayments::GetRequiredPaymentsString(int nBlockHeight)
{
    LOCK(cs_mapMasternodeBlocks);

    if(mapMasternodeBlocks.count(nBlockHeight)){
        return mapMasternodeBlocks[nBlockHeight].GetRequiredPaymentsString();
    }

    return "Unknown";
}

bool CMasternodePayments::IsTransactionValid(const CTransaction& txNew, int nBlockHeight)
{
    LOCK(cs_mapMasternodeBlocks);

    if(mapMasternodeBlocks.count(nBlockHeight)){
        return mapMasternodeBlocks[nBlockHeight].IsTransactionValid(txNew);
    }

    return true;
}

void CMasternodePayments::CheckAndRemove()
{
    if(!pCurrentBlockIndex) return;

    LOCK2(cs_mapMasternodePayeeVotes, cs_mapMasternodeBlocks);

    int nLimit = GetStorageLimit();

    std::map<uint256, CMasternodePaymentWinner>::iterator it = mapMasternodePayeeVotes.begin();
    while(it != mapMasternodePayeeVotes.end()) {
        CMasternodePaymentWinner winner = (*it).second;

        if(pCurrentBlockIndex->nHeight - winner.nBlockHeight > nLimit) {
            LogPrint("mnpayments", "CMasternodePayments::CheckAndRemove -- Removing old Masternode payment: nBlockHeight=%d\n", winner.nBlockHeight);
            mapMasternodePayeeVotes.erase(it++);
            mapMasternodeBlocks.erase(winner.nBlockHeight);
        } else {
            ++it;
        }
    }
    LogPrintf("CMasternodePayments::CheckAndRemove -- %s\n", ToString());
}

bool CMasternodePaymentWinner::IsValid(CNode* pnode, int nValidationHeight, std::string& strError)
{
    CMasternode* pmn = mnodeman.Find(vinMasternode);

    if(!pmn) {
        strError = strprintf("Unknown Masternode: prevout=%s", vinMasternode.prevout.ToStringShort());
        // Only ask if we are already synced and still have no idea about that Masternode
        if(masternodeSync.IsSynced()) {
            mnodeman.AskForMN(pnode, vinMasternode);
        }

        return false;
    }

    int nMinRequiredProtocol;
    if(nBlockHeight > nValidationHeight) {
        // new winners must comply SPORK_10_MASTERNODE_PAY_UPDATED_NODES rules
        nMinRequiredProtocol = mnpayments.GetMinMasternodePaymentsProto();
    } else {
        // allow non-updated masternodes for old blocks
        nMinRequiredProtocol = MIN_MASTERNODE_PAYMENT_PROTO_VERSION_1;
    }

    if(pmn->protocolVersion < nMinRequiredProtocol) {
        strError = strprintf("Masternode protocol is too old: nProtocolVersion=%d, nMinRequiredProtocol=%d", pmn->protocolVersion, nMinRequiredProtocol);
        return false;
    }

    int nRank = mnodeman.GetMasternodeRank(vinMasternode, nBlockHeight - 101, nMinRequiredProtocol);

    if(nRank > MNPAYMENTS_SIGNATURES_TOTAL) {
        // It's common to have masternodes mistakenly think they are in the top 10
        // We don't want to print all of these messages in normal mode, debug mode should print though
        strError = strprintf("Masternode is not in the top %d (%d)", MNPAYMENTS_SIGNATURES_TOTAL, nRank);
        // Only ban for new mnw which is out of bounds, for old mnw MN list itself might be way too much off
        if(nRank > MNPAYMENTS_SIGNATURES_TOTAL*2 && nBlockHeight > nValidationHeight) {
            strError = strprintf("Masternode is not in the top %d (%d)", MNPAYMENTS_SIGNATURES_TOTAL*2, nRank);
            LogPrintf("CMasternodePaymentWinner::IsValid -- Error: %s\n", strError);
            Misbehaving(pnode->GetId(), 20);
        }
        // Still invalid however
        return false;
    }

    return true;
}

bool CMasternodePayments::ProcessBlock(int nBlockHeight)
{
    // DETERMINE IF WE SHOULD BE VOTING FOR THE NEXT PAYEE

    if(fLiteMode || !fMasterNode) return false;

    // We have little chances to pick the right winner if winners list is out of sync
    // but we have no choice, so we'll try. However it doesn't make sense to even try to do so
    // if we have not enough data about masternodes.
    if(!masternodeSync.IsMasternodeListSynced()) return false;

    int nRank = mnodeman.GetMasternodeRank(activeMasternode.vin, nBlockHeight - 101, GetMinMasternodePaymentsProto());

    if (nRank == -1) {
        LogPrint("mnpayments", "CMasternodePayments::ProcessBlock -- Unknown Masternode\n");
        return false;
    }

    if (nRank > MNPAYMENTS_SIGNATURES_TOTAL) {
        LogPrint("mnpayments", "CMasternodePayments::ProcessBlock -- Masternode not in the top %d (%d)\n", MNPAYMENTS_SIGNATURES_TOTAL, nRank);
        return false;
    }


    // LOCATE THE NEXT MASTERNODE WHICH SHOULD BE PAID

    LogPrintf("CMasternodePayments::ProcessBlock -- Start: nBlockHeight=%d, masternode=%s\n", nBlockHeight, activeMasternode.vin.prevout.ToStringShort());

    // pay to the oldest MN that still had no payment but its input is old enough and it was active long enough
    int nCount = 0;
    CMasternode *pmn = mnodeman.GetNextMasternodeInQueueForPayment(nBlockHeight, true, nCount);

    if (pmn == NULL) {
        LogPrintf("CMasternodePayments::ProcessBlock -- ERROR: Failed to find masternode to pay\n");
        return false;
    }

    LogPrintf("CMasternodePayments::ProcessBlock -- Masternode found by GetNextMasternodeInQueueForPayment(): %s\n", pmn->vin.prevout.ToStringShort());


    CScript payee = GetScriptForDestination(pmn->pubkey.GetID());

    CMasternodePaymentWinner newWinner(activeMasternode.vin, nBlockHeight, payee);

    CTxDestination address1;
    ExtractDestination(payee, address1);
    CBitcoinAddress address2(address1);

    LogPrintf("CMasternodePayments::ProcessBlock -- Winner: payee=%s, nBlockHeight=%d\n", address2.ToString(), nBlockHeight);

    // SIGN MESSAGE TO NETWORK WITH OUR MASTERNODE KEYS

    LogPrintf("CMasternodePayments::ProcessBlock -- Signing Winner\n");
    if (newWinner.Sign()) {
        LogPrintf("CMasternodePayments::ProcessBlock -- AddWinningMasternode()\n");

        if (AddWinningMasternode(newWinner)) {
            newWinner.Relay();
            return true;
        }
    }

    return false;
}

void CMasternodePaymentWinner::Relay()
{
    CInv inv(MSG_MASTERNODE_WINNER, GetHash());
    RelayInv(inv);
}

bool CMasternodePaymentWinner::CheckSignature()
{

    CMasternode* pmn = mnodeman.Find(vinMasternode);

    if (!pmn) return false;

    std::string strMessage = vinMasternode.prevout.ToStringShort() +
                boost::lexical_cast<std::string>(nBlockHeight) +
                ScriptToAsmStr(payee);

    std::string strError = "";
    if (!darkSendSigner.VerifyMessage(pmn->pubkey2, vchSig, strMessage, strError)) {
        return error("CMasternodePaymentWinner::CheckSignature -- Got bad Masternode payment signature, masternode=%s, error: %s", vinMasternode.prevout.ToStringShort().c_str(), strError);
    }

    return true;
}

std::string CMasternodePaymentWinner::ToString() const
{
    std::ostringstream info;

    info << vinMasternode.prevout.ToStringShort() <<
            ", " << nBlockHeight <<
            ", " << ScriptToAsmStr(payee) <<
            ", " << (int)vchSig.size();

    return info.str();
}

void CMasternodePayments::Sync(CNode* pnode, int nCountNeeded)
{
    LOCK(cs_mapMasternodePayeeVotes);

    if(!pCurrentBlockIndex) return;

    int nLimit = GetStorageLimit();
    if(nCountNeeded > nLimit) nCountNeeded = nLimit;

    int nInvCount = 0;
    std::map<uint256, CMasternodePaymentWinner>::iterator it = mapMasternodePayeeVotes.begin();
    while(it != mapMasternodePayeeVotes.end()) {
        CMasternodePaymentWinner winner = (*it).second;
        if(winner.nBlockHeight >= pCurrentBlockIndex->nHeight - nCountNeeded && winner.nBlockHeight <= pCurrentBlockIndex->nHeight + 20) {
            pnode->PushInventory(CInv(MSG_MASTERNODE_WINNER, winner.GetHash()));
            nInvCount++;
        }
        ++it;
    }

    LogPrintf("CMasternodePayments::Sync -- Sent %d winners to peer %d\n", nInvCount, pnode->id);
    pnode->PushMessage(NetMsgType::SYNCSTATUSCOUNT, MASTERNODE_SYNC_MNW, nInvCount);
}

std::string CMasternodePayments::ToString() const
{
    std::ostringstream info;

    info << "Votes: " << (int)mapMasternodePayeeVotes.size() <<
            ", Blocks: " << (int)mapMasternodeBlocks.size();

    return info.str();
}

bool CMasternodePayments::IsEnoughData(int nMnCount) {
    if(GetBlockCount() > nMnCount * nStorageCoeff && GetBlockCount() > nMinBlocksToStore)
    {
        float nAverageVotes = (MNPAYMENTS_SIGNATURES_TOTAL + MNPAYMENTS_SIGNATURES_REQUIRED) / 2;
        if(GetVoteCount() > nMnCount * nStorageCoeff * nAverageVotes && GetVoteCount() > nMinBlocksToStore * nAverageVotes)
        {
            return true;
        }
    }
    return false;
}

int CMasternodePayments::GetStorageLimit()
{
    return std::max(int(mnodeman.size() * nStorageCoeff), nMinBlocksToStore);
}

void CMasternodePayments::UpdatedBlockTip(const CBlockIndex *pindex)
{
    pCurrentBlockIndex = pindex;
    LogPrint("mnpayments", "CMasternodePayments::UpdatedBlockTip -- pCurrentBlockIndex->nHeight=%d\n", pCurrentBlockIndex->nHeight);

    ProcessBlock(pindex->nHeight + 10);
    // normal wallet does not need to update this every block, doing update on rpc call should be enough
    if(fMasterNode) mnodeman.UpdateLastPaid(pindex);
}
