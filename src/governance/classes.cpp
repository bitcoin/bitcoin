// Copyright (c) 2014-2023 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <governance/classes.h>

#include <chainparams.h>
#include <consensus/validation.h>
#include <core_io.h>
#include <deploymentstatus.h>
#include <governance/governance.h>
#include <key_io.h>
#include <primitives/transaction.h>
#include <script/standard.h>
#include <util/moneystr.h>
#include <util/strencodings.h>
#include <util/time.h>
#include <util/underlying.h>
#include <validation.h>
#include <versionbits.h>

#include <univalue.h>

// SPLIT UP STRING BY DELIMITER
std::vector<std::string> SplitBy(const std::string& strCommand, const std::string& strDelimit)
{
    std::vector<std::string> vParts = SplitString(strCommand, strDelimit);

    for (int q = 0; q < (int)vParts.size(); q++) {
        if (strDelimit.find(vParts[q]) != std::string::npos) {
            vParts.erase(vParts.begin() + q);
            --q;
        }
    }

    return vParts;
}

CAmount ParsePaymentAmount(const std::string& strAmount)
{
    CAmount nAmount = 0;
    if (strAmount.empty()) {
        std::ostringstream ostr;
        ostr << "ParsePaymentAmount: Amount is empty";
        throw std::runtime_error(ostr.str());
    }
    if (strAmount.size() > 20) {
        // String is much too long, the functions below impose stricter
        // requirements
        std::ostringstream ostr;
        ostr << "ParsePaymentAmount: Amount string too long";
        throw std::runtime_error(ostr.str());
    }
    // Make sure the string makes sense as an amount
    // Note: No spaces allowed
    // Also note: No scientific notation
    size_t pos = strAmount.find_first_not_of("0123456789.");
    if (pos != std::string::npos) {
        std::ostringstream ostr;
        ostr << "ParsePaymentAmount: Amount string contains invalid character";
        throw std::runtime_error(ostr.str());
    }

    pos = strAmount.find('.');
    if (pos == 0) {
        // JSON doesn't allow values to start with a decimal point
        std::ostringstream ostr;
        ostr << "ParsePaymentAmount: Invalid amount string, leading decimal point not allowed";
        throw std::runtime_error(ostr.str());
    }

    // Make sure there's no more than 1 decimal point
    if ((pos != std::string::npos) && (strAmount.find('.', pos + 1) != std::string::npos)) {
        std::ostringstream ostr;
        ostr << "ParsePaymentAmount: Invalid amount string, too many decimal points";
        throw std::runtime_error(ostr.str());
    }

    // Note this code is taken from AmountFromValue in rpcserver.cpp
    // which is used for parsing the amounts in createrawtransaction.
    if (!ParseFixedPoint(strAmount, 8, &nAmount)) {
        nAmount = 0;
        std::ostringstream ostr;
        ostr << "ParsePaymentAmount: ParseFixedPoint failed for string: " << strAmount;
        throw std::runtime_error(ostr.str());
    }
    if (!MoneyRange(nAmount)) {
        nAmount = 0;
        std::ostringstream ostr;
        ostr << "ParsePaymentAmount: Invalid amount string, value outside of valid money range";
        throw std::runtime_error(ostr.str());
    }

    return nAmount;
}

/**
*   Add Governance Object
*/

bool CGovernanceManager::AddNewTrigger(uint256 nHash)
{
    AssertLockHeld(cs);

    // IF WE ALREADY HAVE THIS HASH, RETURN
    if (mapTrigger.count(nHash)) {
        LogPrint(BCLog::GOBJECT, "CGovernanceManager::%s -- Already have hash, nHash = %s, count = %d, size = %s\n",
                    __func__, nHash.GetHex(), mapTrigger.count(nHash), mapTrigger.size());
        return false;
    }

    CSuperblock_sptr pSuperblock;
    try {
        auto pSuperblockTmp = std::make_shared<CSuperblock>(*this, nHash);
        pSuperblock = pSuperblockTmp;
    } catch (std::exception& e) {
        LogPrintf("CGovernanceManager::%s -- Error creating superblock: %s\n", __func__, e.what());
        return false;
    } catch (...) {
        LogPrintf("CGovernanceManager::%s -- Unknown Error creating superblock\n", __func__);
        return false;
    }

    pSuperblock->SetStatus(SeenObjectStatus::Valid);

    mapTrigger.insert(std::make_pair(nHash, pSuperblock));

    return !pSuperblock->IsExpired(*this);
}

/**
*
*   Clean And Remove
*
*/

void CGovernanceManager::CleanAndRemoveTriggers()
{
    AssertLockHeld(cs);

    // Remove triggers that are invalid or expired
    LogPrint(BCLog::GOBJECT, "CGovernanceManager::%s -- mapTrigger.size() = %d\n", __func__, mapTrigger.size());

    auto it = mapTrigger.begin();
    while (it != mapTrigger.end()) {
        bool remove = false;
        CGovernanceObject* pObj = nullptr;
        const CSuperblock_sptr& pSuperblock = it->second;
        if (!pSuperblock) {
            LogPrint(BCLog::GOBJECT, "CGovernanceManager::%s -- nullptr superblock\n", __func__);
            remove = true;
        } else {
            pObj = FindGovernanceObject(it->first);
            if (!pObj || pObj->GetObjectType() != GovernanceObject::TRIGGER) {
                LogPrint(BCLog::GOBJECT, "CGovernanceManager::%s -- Unknown or non-trigger superblock\n", __func__);
                pSuperblock->SetStatus(SeenObjectStatus::ErrorInvalid);
            }

            LogPrint(BCLog::GOBJECT, "CGovernanceManager::%s -- superblock status = %d\n", __func__, ToUnderlying(pSuperblock->GetStatus()));
            switch (pSuperblock->GetStatus()) {
            case SeenObjectStatus::ErrorInvalid:
            case SeenObjectStatus::Unknown:
                LogPrint(BCLog::GOBJECT, "CGovernanceManager::%s -- Unknown or invalid trigger found\n", __func__);
                remove = true;
                break;
            case SeenObjectStatus::Valid:
            case SeenObjectStatus::Executed: {
                LogPrint(BCLog::GOBJECT, "CGovernanceManager::%s -- Valid trigger found\n", __func__);
                if (pSuperblock->IsExpired(*this)) {
                    // update corresponding object
                    pObj->SetExpired();
                    remove = true;
                }
                break;
            }
            default:
                break;
            }
        }
        LogPrint(BCLog::GOBJECT, "CGovernanceManager::%s -- %smarked for removal\n", __func__, remove ? "" : "NOT ");

        if (remove) {
            std::string strDataAsPlainString = "nullptr";
            if (pObj) {
                strDataAsPlainString = pObj->GetDataAsPlainString();
                // mark corresponding object for deletion
                pObj->PrepareDeletion(GetTime<std::chrono::seconds>().count());
            }
            LogPrint(BCLog::GOBJECT, "CGovernanceManager::%s -- Removing trigger object %s\n", __func__, strDataAsPlainString);
            // delete the trigger
            mapTrigger.erase(it++);
        } else {
            ++it;
        }
    }
}

/**
*   Get Active Triggers
*
*   - Look through triggers and scan for active ones
*   - Return the triggers in a list
*/

std::vector<CSuperblock_sptr> CGovernanceManager::GetActiveTriggers()
{
    AssertLockHeld(cs);
    std::vector<CSuperblock_sptr> vecResults;

    // LOOK AT THESE OBJECTS AND COMPILE A VALID LIST OF TRIGGERS
    for (const auto& pair : mapTrigger) {
        const CGovernanceObject* pObj = FindConstGovernanceObject(pair.first);
        if (pObj) {
            vecResults.push_back(pair.second);
        }
    }

    return vecResults;
}

/**
*   Is Superblock Triggered
*
*   - Does this block have a non-executed and activated trigger?
*/

bool CSuperblockManager::IsSuperblockTriggered(CGovernanceManager& govman, const CDeterministicMNList& tip_mn_list, int nBlockHeight)
{
    LogPrint(BCLog::GOBJECT, "CSuperblockManager::IsSuperblockTriggered -- Start nBlockHeight = %d\n", nBlockHeight);
    if (!CSuperblock::IsValidBlockHeight(nBlockHeight)) {
        return false;
    }

    LOCK(govman.cs);
    // GET ALL ACTIVE TRIGGERS
    std::vector<CSuperblock_sptr> vecTriggers = govman.GetActiveTriggers();

    LogPrint(BCLog::GOBJECT, "CSuperblockManager::IsSuperblockTriggered -- vecTriggers.size() = %d\n", vecTriggers.size());

    for (const auto& pSuperblock : vecTriggers) {
        if (!pSuperblock) {
            LogPrintf("CSuperblockManager::IsSuperblockTriggered -- Non-superblock found, continuing\n");
            continue;
        }

        CGovernanceObject* pObj = pSuperblock->GetGovernanceObject(govman);

        if (!pObj) {
            LogPrintf("CSuperblockManager::IsSuperblockTriggered -- pObj == nullptr, continuing\n");
            continue;
        }

        LogPrint(BCLog::GOBJECT, "CSuperblockManager::IsSuperblockTriggered -- data = %s\n", pObj->GetDataAsPlainString());

        // note : 12.1 - is epoch calculation correct?

        if (nBlockHeight != pSuperblock->GetBlockHeight()) {
            LogPrint(BCLog::GOBJECT, "CSuperblockManager::IsSuperblockTriggered -- block height doesn't match nBlockHeight = %d, blockStart = %d, continuing\n",
                nBlockHeight,
                pSuperblock->GetBlockHeight());
            continue;
        }

        // MAKE SURE THIS TRIGGER IS ACTIVE VIA FUNDING CACHE FLAG

        pObj->UpdateSentinelVariables(tip_mn_list);

        if (pObj->IsSetCachedFunding()) {
            LogPrint(BCLog::GOBJECT, "CSuperblockManager::IsSuperblockTriggered -- fCacheFunding = true, returning true\n");
            return true;
        } else {
            LogPrint(BCLog::GOBJECT, "CSuperblockManager::IsSuperblockTriggered -- fCacheFunding = false, continuing\n");
        }
    }

    return false;
}


bool CSuperblockManager::GetBestSuperblock(CGovernanceManager& govman, const CDeterministicMNList& tip_mn_list, CSuperblock_sptr& pSuperblockRet, int nBlockHeight)
{
    if (!CSuperblock::IsValidBlockHeight(nBlockHeight)) {
        return false;
    }

    AssertLockHeld(govman.cs);
    std::vector<CSuperblock_sptr> vecTriggers = govman.GetActiveTriggers();
    int nYesCount = 0;

    for (const auto& pSuperblock : vecTriggers) {
        if (!pSuperblock || nBlockHeight != pSuperblock->GetBlockHeight()) {
            continue;
        }

        const CGovernanceObject* pObj = pSuperblock->GetGovernanceObject(govman);

        if (!pObj) {
            continue;
        }

        // DO WE HAVE A NEW WINNER?

        int nTempYesCount = pObj->GetAbsoluteYesCount(tip_mn_list, VOTE_SIGNAL_FUNDING);
        if (nTempYesCount > nYesCount) {
            nYesCount = nTempYesCount;
            pSuperblockRet = pSuperblock;
        }
    }

    return nYesCount > 0;
}

/**
*   Get Superblock Payments
*
*   - Returns payments for superblock
*/

bool CSuperblockManager::GetSuperblockPayments(CGovernanceManager& govman, const CDeterministicMNList& tip_mn_list, int nBlockHeight, std::vector<CTxOut>& voutSuperblockRet)
{
    LOCK(govman.cs);

    // GET THE BEST SUPERBLOCK FOR THIS BLOCK HEIGHT

    CSuperblock_sptr pSuperblock;
    if (!CSuperblockManager::GetBestSuperblock(govman, tip_mn_list, pSuperblock, nBlockHeight)) {
        LogPrint(BCLog::GOBJECT, "CSuperblockManager::GetSuperblockPayments -- Can't find superblock for height %d\n", nBlockHeight);
        return false;
    }

    // make sure it's empty, just in case
    voutSuperblockRet.clear();

    // GET SUPERBLOCK OUTPUTS

    // Superblock payments will be appended to the end of the coinbase vout vector

    // TODO: How many payments can we add before things blow up?
    //       Consider at least following limits:
    //          - max coinbase tx size
    //          - max "budget" available
    for (int i = 0; i < pSuperblock->CountPayments(); i++) {
        CGovernancePayment payment;
        if (pSuperblock->GetPayment(i, payment)) {
            // SET COINBASE OUTPUT TO SUPERBLOCK SETTING

            CTxOut txout = CTxOut(payment.nAmount, payment.script);
            voutSuperblockRet.push_back(txout);

            // PRINT NICE LOG OUTPUT FOR SUPERBLOCK PAYMENT

            CTxDestination dest;
            ExtractDestination(payment.script, dest);

            LogPrint(BCLog::GOBJECT, "CSuperblockManager::GetSuperblockPayments -- NEW Superblock: output %d (addr %s, amount %d.%08d)\n",
                        i, EncodeDestination(dest), payment.nAmount / COIN, payment.nAmount % COIN);
        } else {
            LogPrint(BCLog::GOBJECT, "CSuperblockManager::GetSuperblockPayments -- Payment not found\n");
        }
    }

    return true;
}

bool CSuperblockManager::IsValid(CGovernanceManager& govman, const CChain& active_chain, const CDeterministicMNList& tip_mn_list, const CTransaction& txNew, int nBlockHeight, CAmount blockReward)
{
    // GET BEST SUPERBLOCK, SHOULD MATCH
    LOCK(govman.cs);

    CSuperblock_sptr pSuperblock;
    if (CSuperblockManager::GetBestSuperblock(govman, tip_mn_list, pSuperblock, nBlockHeight)) {
        return pSuperblock->IsValid(govman, active_chain, txNew, nBlockHeight, blockReward);
    }

    return false;
}

void CSuperblockManager::ExecuteBestSuperblock(CGovernanceManager& govman, const CDeterministicMNList& tip_mn_list, int nBlockHeight)
{
    LOCK(govman.cs);

    CSuperblock_sptr pSuperblock;
    if (GetBestSuperblock(govman, tip_mn_list, pSuperblock, nBlockHeight)) {
        // All checks are done in CSuperblock::IsValid via IsBlockValueValid and IsBlockPayeeValid,
        // tip wouldn't be updated if anything was wrong. Mark this trigger as executed.
        pSuperblock->SetExecuted();
        govman.ResetVotedFundingTrigger();
    }
}

CSuperblock::
    CSuperblock() :
    nGovObjHash(),
    nBlockHeight(0),
    nStatus(SeenObjectStatus::Unknown),
    vecPayments()
{
}

CSuperblock::
    CSuperblock(CGovernanceManager& govman, uint256& nHash) :
    nGovObjHash(nHash),
    nBlockHeight(0),
    nStatus(SeenObjectStatus::Unknown),
    vecPayments()
{
    const CGovernanceObject* pGovObj = GetGovernanceObject(govman);

    if (!pGovObj) {
        throw std::runtime_error("CSuperblock: Failed to find Governance Object");
    }

    LogPrint(BCLog::GOBJECT, "CSuperblock -- Constructor pGovObj: %s, nObjectType = %d\n",
                pGovObj->GetDataAsPlainString(), ToUnderlying(pGovObj->GetObjectType()));

    if (pGovObj->GetObjectType() != GovernanceObject::TRIGGER) {
        throw std::runtime_error("CSuperblock: Governance Object not a trigger");
    }

    UniValue obj = pGovObj->GetJSONObject();

    if (obj["type"].get_int() != ToUnderlying(GovernanceObject::TRIGGER)) {
        throw std::runtime_error("CSuperblock: invalid data type");
    }

    // FIRST WE GET THE START HEIGHT, THE BLOCK HEIGHT AT WHICH THE PAYMENT SHALL OCCUR
    nBlockHeight = obj["event_block_height"].get_int();

    // NEXT WE GET THE PAYMENT INFORMATION AND RECONSTRUCT THE PAYMENT VECTOR
    std::string strAddresses = obj["payment_addresses"].get_str();
    std::string strAmounts = obj["payment_amounts"].get_str();
    std::string strProposalHashes = obj["proposal_hashes"].get_str();
    ParsePaymentSchedule(strAddresses, strAmounts, strProposalHashes);

    LogPrint(BCLog::GOBJECT, "CSuperblock -- nBlockHeight = %d, strAddresses = %s, strAmounts = %s, vecPayments.size() = %d\n",
        nBlockHeight, strAddresses, strAmounts, vecPayments.size());
}

CSuperblock::CSuperblock(int nBlockHeight, std::vector<CGovernancePayment> vecPayments) : nBlockHeight(nBlockHeight), vecPayments(std::move(vecPayments))
{
    nStatus = SeenObjectStatus::Valid; //TODO: Investigate this
    nGovObjHash = GetHash();
}

CGovernanceObject* CSuperblock::GetGovernanceObject(CGovernanceManager& govman)
{
    AssertLockHeld(govman.cs);
    CGovernanceObject* pObj = govman.FindGovernanceObject(nGovObjHash);
    return pObj;
}


/**
 *   Is Valid Superblock Height
 *
 *   - See if a block at this height can be a superblock
 */

bool CSuperblock::IsValidBlockHeight(int nBlockHeight)
{
    // SUPERBLOCKS CAN HAPPEN ONLY after hardfork and only ONCE PER CYCLE
    return nBlockHeight >= Params().GetConsensus().nSuperblockStartBlock &&
           ((nBlockHeight % Params().GetConsensus().nSuperblockCycle) == 0);
}

void CSuperblock::GetNearestSuperblocksHeights(int nBlockHeight, int& nLastSuperblockRet, int& nNextSuperblockRet)
{
    const Consensus::Params& consensusParams = Params().GetConsensus();
    int nSuperblockStartBlock = consensusParams.nSuperblockStartBlock;
    int nSuperblockCycle = consensusParams.nSuperblockCycle;

    // Get first superblock
    int nFirstSuperblockOffset = (nSuperblockCycle - nSuperblockStartBlock % nSuperblockCycle) % nSuperblockCycle;
    int nFirstSuperblock = nSuperblockStartBlock + nFirstSuperblockOffset;

    if (nBlockHeight < nFirstSuperblock) {
        nLastSuperblockRet = 0;
        nNextSuperblockRet = nFirstSuperblock;
    } else {
        nLastSuperblockRet = nBlockHeight - nBlockHeight % nSuperblockCycle;
        nNextSuperblockRet = nLastSuperblockRet + nSuperblockCycle;
    }
}

CAmount CSuperblock::GetPaymentsLimit(const CChain& active_chain, int nBlockHeight)
{
    const Consensus::Params& consensusParams = Params().GetConsensus();

    if (!IsValidBlockHeight(nBlockHeight)) {
        return 0;
    }

    const CBlockIndex* pindex = active_chain.Tip();
    if (pindex->nHeight > nBlockHeight) pindex = pindex->GetAncestor(nBlockHeight);

    const auto v20_state = g_versionbitscache.State(pindex, consensusParams, Consensus::DEPLOYMENT_V20);
    bool fV20Active{v20_state == ThresholdState::ACTIVE};
    if (!fV20Active && nBlockHeight > pindex->nHeight) {
        // If fV20Active isn't active yet and nBlockHeight refers to a future SuperBlock
        // then we need to check if the fork is locked_in and see if it will be active by the time of the future SuperBlock
        if (v20_state == ThresholdState::LOCKED_IN) {
            int activation_height = g_versionbitscache.StateSinceHeight(pindex, consensusParams, Consensus::DEPLOYMENT_V20) + static_cast<int>(Params().GetConsensus().vDeployments[Consensus::DEPLOYMENT_V20].nWindowSize);
            if (nBlockHeight >= activation_height) {
                fV20Active = true;
            }
        }
    }

    // min subsidy for high diff networks and vice versa
    int nBits = consensusParams.fPowAllowMinDifficultyBlocks ? UintToArith256(consensusParams.powLimit).GetCompact() : 1;
    // some part of all blocks issued during the cycle goes to superblock, see GetBlockSubsidy
    CAmount nSuperblockPartOfSubsidy = GetSuperblockSubsidyInner(nBits, nBlockHeight - 1, consensusParams, fV20Active);
    CAmount nPaymentsLimit = nSuperblockPartOfSubsidy * consensusParams.nSuperblockCycle;
    LogPrint(BCLog::GOBJECT, "CSuperblock::GetPaymentsLimit -- Valid superblock height %d, payments max %lld\n", nBlockHeight, nPaymentsLimit);

    return nPaymentsLimit;
}

void CSuperblock::ParsePaymentSchedule(const std::string& strPaymentAddresses, const std::string& strPaymentAmounts, const std::string& strProposalHashes)
{
    // SPLIT UP ADDR/AMOUNT STRINGS AND PUT IN VECTORS

    std::vector<std::string> vecParsed1;
    std::vector<std::string> vecParsed2;
    std::vector<std::string> vecParsed3;
    vecParsed1 = SplitBy(strPaymentAddresses, "|");
    vecParsed2 = SplitBy(strPaymentAmounts, "|");
    vecParsed3 = SplitBy(strProposalHashes, "|");

    // IF THESE DON'T MATCH, SOMETHING IS WRONG

    if (vecParsed1.size() != vecParsed2.size() || vecParsed1.size() != vecParsed3.size()) {
        std::ostringstream ostr;
        ostr << "CSuperblock::ParsePaymentSchedule -- Mismatched payments, amounts and proposalHashes";
        LogPrintf("%s\n", ostr.str());
        throw std::runtime_error(ostr.str());
    }

    if (vecParsed1.empty()) {
        std::ostringstream ostr;
        ostr << "CSuperblock::ParsePaymentSchedule -- Error no payments";
        LogPrintf("%s\n", ostr.str());
        throw std::runtime_error(ostr.str());
    }

    // LOOP THROUGH THE ADDRESSES/AMOUNTS AND CREATE PAYMENTS
    /*
      ADDRESSES = [ADDR1|2|3|4|5|6]
      AMOUNTS = [AMOUNT1|2|3|4|5|6]
    */

    for (int i = 0; i < (int)vecParsed1.size(); i++) {
        CTxDestination dest = DecodeDestination(vecParsed1[i]);
        if (!IsValidDestination(dest)) {
            std::ostringstream ostr;
            ostr << "CSuperblock::ParsePaymentSchedule -- Invalid Dash Address : " << vecParsed1[i];
            LogPrintf("%s\n", ostr.str());
            throw std::runtime_error(ostr.str());
        }

        CAmount nAmount = ParsePaymentAmount(vecParsed2[i]);

        uint256 proposalHash;
        if (!ParseHashStr(vecParsed3[i], proposalHash)){
            std::ostringstream ostr;
            ostr << "CSuperblock::ParsePaymentSchedule -- Invalid proposal hash : " << vecParsed3[i];
            LogPrintf("%s\n", ostr.str());
            throw std::runtime_error(ostr.str());
        }

        LogPrint(BCLog::GOBJECT, "CSuperblock::ParsePaymentSchedule -- i = %d, amount string = %s, nAmount = %lld, proposalHash = %s\n", i, vecParsed2[i], nAmount, proposalHash.ToString());

        CGovernancePayment payment(dest, nAmount, proposalHash);
        if (payment.IsValid()) {
            vecPayments.push_back(payment);
        } else {
            vecPayments.clear();
            std::ostringstream ostr;
            ostr << "CSuperblock::ParsePaymentSchedule -- Invalid payment found: address = " << EncodeDestination(dest)
                 << ", amount = " << nAmount;
            LogPrintf("%s\n", ostr.str());
            throw std::runtime_error(ostr.str());
        }
    }
}

bool CSuperblock::GetPayment(int nPaymentIndex, CGovernancePayment& paymentRet)
{
    if ((nPaymentIndex < 0) || (nPaymentIndex >= (int)vecPayments.size())) {
        return false;
    }

    paymentRet = vecPayments[nPaymentIndex];
    return true;
}

CAmount CSuperblock::GetPaymentsTotalAmount()
{
    CAmount nPaymentsTotalAmount = 0;
    int nPayments = CountPayments();

    for (int i = 0; i < nPayments; i++) {
        nPaymentsTotalAmount += vecPayments[i].nAmount;
    }

    return nPaymentsTotalAmount;
}

/**
*   Is Transaction Valid
*
*   - Does this transaction match the superblock?
*/

bool CSuperblock::IsValid(CGovernanceManager& govman, const CChain& active_chain, const CTransaction& txNew, int nBlockHeight, CAmount blockReward)
{
    // TODO : LOCK(cs);
    // No reason for a lock here now since this method only accesses data
    // internal to *this and since CSuperblock's are accessed only through
    // shared pointers there's no way our object can get deleted while this
    // code is running.
    if (!IsValidBlockHeight(nBlockHeight)) {
        LogPrintf("CSuperblock::IsValid -- ERROR: Block invalid, incorrect block height\n");
        return false;
    }

    // CONFIGURE SUPERBLOCK OUTPUTS

    int nOutputs = txNew.vout.size();
    int nPayments = CountPayments();
    int nMinerAndMasternodePayments = nOutputs - nPayments;

    LogPrint(BCLog::GOBJECT, "CSuperblock::IsValid -- nOutputs = %d, nPayments = %d, GetDataAsHexString = %s\n",
        nOutputs, nPayments, GetGovernanceObject(govman)->GetDataAsHexString());

    // We require an exact match (including order) between the expected
    // superblock payments and the payments actually in the block.

    if (nMinerAndMasternodePayments < 0) {
        // This means the block cannot have all the superblock payments
        // so it is not valid.
        // TODO: could that be that we just hit coinbase size limit?
        LogPrintf("CSuperblock::IsValid -- ERROR: Block invalid, too few superblock payments\n");
        return false;
    }

    // payments should not exceed limit
    CAmount nPaymentsTotalAmount = GetPaymentsTotalAmount();
    CAmount nPaymentsLimit = GetPaymentsLimit(active_chain, nBlockHeight);
    if (nPaymentsTotalAmount > nPaymentsLimit) {
        LogPrintf("CSuperblock::IsValid -- ERROR: Block invalid, payments limit exceeded: payments %lld, limit %lld\n", nPaymentsTotalAmount, nPaymentsLimit);
        return false;
    }

    // miner and masternodes should not get more than they would usually get
    CAmount nBlockValue = txNew.GetValueOut();
    if (nBlockValue > blockReward + nPaymentsTotalAmount) {
        LogPrintf("CSuperblock::IsValid -- ERROR: Block invalid, block value limit exceeded: block %lld, limit %lld\n", nBlockValue, blockReward + nPaymentsTotalAmount);
        return false;
    }

    int nVoutIndex = 0;
    for (int i = 0; i < nPayments; i++) {
        CGovernancePayment payment;
        if (!GetPayment(i, payment)) {
            // This shouldn't happen so log a warning
            LogPrintf("CSuperblock::IsValid -- WARNING: Failed to find payment: %d of %d total payments\n", i, nPayments);
            continue;
        }

        bool fPaymentMatch = false;

        for (int j = nVoutIndex; j < nOutputs; j++) {
            // Find superblock payment
            fPaymentMatch = ((payment.script == txNew.vout[j].scriptPubKey) &&
                             (payment.nAmount == txNew.vout[j].nValue));

            if (fPaymentMatch) {
                nVoutIndex = j;
                break;
            }
        }

        if (!fPaymentMatch) {
            // Superblock payment not found!

            CTxDestination dest;
            ExtractDestination(payment.script, dest);
            LogPrintf("CSuperblock::IsValid -- ERROR: Block invalid: %d payment %d to %s not found\n", i, payment.nAmount, EncodeDestination(dest));

            return false;
        }
    }

    return true;
}

bool CSuperblock::IsExpired(const CGovernanceManager& govman) const
{
    int nExpirationBlocks;
    // Executed triggers are kept for another superblock cycle (approximately 1 month for mainnet).
    // Other valid triggers are kept for ~1 day only (for mainnet, but no longer than a superblock cycle for other networks).
    // Everything else is pruned after ~1h (for mainnet, but no longer than a superblock cycle for other networks).
    switch (nStatus) {
    case SeenObjectStatus::Executed:
        nExpirationBlocks = Params().GetConsensus().nSuperblockCycle;
        break;
    case SeenObjectStatus::Valid:
        nExpirationBlocks = std::min(576, Params().GetConsensus().nSuperblockCycle);
        break;
    default:
        nExpirationBlocks = std::min(24, Params().GetConsensus().nSuperblockCycle);
        break;
    }

    int nExpirationBlock = nBlockHeight + nExpirationBlocks;

    LogPrint(BCLog::GOBJECT, "CSuperblock::IsExpired -- nBlockHeight = %d, nExpirationBlock = %d\n", nBlockHeight, nExpirationBlock);

    if (govman.GetCachedBlockHeight() > nExpirationBlock) {
        LogPrint(BCLog::GOBJECT, "CSuperblock::IsExpired -- Outdated trigger found\n");
        return true;
    }

    if (Params().NetworkIDString() != CBaseChainParams::MAIN) {
        // NOTE: this can happen on testnet/devnets due to reorgs, should never happen on mainnet
        if (govman.GetCachedBlockHeight() + Params().GetConsensus().nSuperblockCycle * 2 < nBlockHeight) {
            LogPrint(BCLog::GOBJECT, "CSuperblock::IsExpired -- Trigger is too far into the future\n");
            return true;
        }
    }

    return false;
}

std::vector<uint256> CSuperblock::GetProposalHashes() const
{
    std::vector<uint256> res;

    for (const auto& payment : vecPayments) {
        res.push_back(payment.proposalHash);
    }

    return res;
}

std::string CSuperblock::GetHexStrData() const
{
    // {\"event_block_height\": 879720, \"payment_addresses\": \"yd5KMREs3GLMe6mTJYr3YrH1juwNwrFCfB\", \"payment_amounts\": \"5.00000000\", \"proposal_hashes\": \"485817fddbcab6c55c9a6856dabc8b19ed79548bda8c01712daebc9f74f287f4\", \"type\": 2}\u0000

    std::string str_addresses = Join(vecPayments, "|", [&](const auto& payment) {
        CTxDestination dest;
        ExtractDestination(payment.script, dest);
        return EncodeDestination(dest);
    });
    std::string str_amounts = Join(vecPayments, "|", [&](const auto& payment) {
        return ValueFromAmount(payment.nAmount).write();
    });
    std::string str_hashes = Join(vecPayments, "|", [&](const auto& payment) { return payment.proposalHash.ToString(); });

    std::stringstream ss;
    ss << "{";
    ss << "\"event_block_height\": " << nBlockHeight << ", ";
    ss << "\"payment_addresses\": \"" << str_addresses << "\", ";
    ss << "\"payment_amounts\": \"" << str_amounts << "\", ";
    ss << "\"proposal_hashes\": \"" << str_hashes << "\", ";
    ss << "\"type\":" << 2;
    ss << "}";

    return HexStr(ss.str());
}

CGovernancePayment::CGovernancePayment(const CTxDestination& destIn, CAmount nAmountIn, const uint256& proposalHash) :
        fValid(false),
        script(),
        nAmount(0),
        proposalHash(proposalHash)
{
    try {
        script = GetScriptForDestination(destIn);
        nAmount = nAmountIn;
        fValid = true;
    } catch (std::exception& e) {
        LogPrintf("CGovernancePayment Payment not valid: destIn = %s, nAmountIn = %d, what = %s\n",
                  EncodeDestination(destIn), nAmountIn, e.what());
    } catch (...) {
        LogPrintf("CGovernancePayment Payment not valid: destIn = %s, nAmountIn = %d\n",
                  EncodeDestination(destIn), nAmountIn);
    }
}

