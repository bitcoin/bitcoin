// Copyright (c) 2014-2023 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <governance/governanceclasses.h>
#include <evo/deterministicmns.h>
#include <chainparams.h>
#include <consensus/validation.h>
#include <core_io.h>
#include <governance/governance.h>
#include <key_io.h>
#include <primitives/transaction.h>
#include <util/moneystr.h>
#include <util/strencodings.h>
#include <util/time.h>
#include <validation.h>

#include <univalue.h>
#include <logging.h>
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
        auto pSuperblockTmp = std::make_shared<CSuperblock>(nHash);
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

    return !pSuperblock->IsExpired();
}

/**
*
*   Clean And Remove
*
*/

void CGovernanceManager::CleanAndRemoveTriggers()
{
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
            if (!pObj || pObj->GetObjectType() != GOVERNANCE_OBJECT_TRIGGER) {
                LogPrint(BCLog::GOBJECT, "CGovernanceManager::%s -- Unknown or non-trigger superblock\n", __func__);
                pSuperblock->SetStatus(SeenObjectStatus::ErrorInvalid);
            }

            LogPrint(BCLog::GOBJECT, "CGovernanceManager::%s -- superblock status = %d\n", __func__, (int)pSuperblock->GetStatus());
            switch (pSuperblock->GetStatus()) {
            case SeenObjectStatus::ErrorInvalid:
            case SeenObjectStatus::Unknown:
                LogPrint(BCLog::GOBJECT, "CGovernanceManager::%s -- Unknown or invalid trigger found\n", __func__);
                remove = true;
                break;
            case SeenObjectStatus::Valid:
            case SeenObjectStatus::Executed: {
                LogPrint(BCLog::GOBJECT, "CGovernanceManager::%s -- Valid trigger found\n", __func__);
                if (pSuperblock->IsExpired()) {
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

bool CSuperblockManager::IsSuperblockTriggered(int nBlockHeight)
{
    LogPrint(BCLog::GOBJECT, "CSuperblockManager::IsSuperblockTriggered -- Start nBlockHeight = %d\n", nBlockHeight);
    if (!CSuperblock::IsValidBlockHeight(nBlockHeight)) {
        return false;
    }

    LOCK(governance->cs);
    const auto tip_mn_list = deterministicMNManager->GetListAtChainTip();
    // GET ALL ACTIVE TRIGGERS
    std::vector<CSuperblock_sptr> vecTriggers = governance->GetActiveTriggers();

    LogPrint(BCLog::GOBJECT, "CSuperblockManager::IsSuperblockTriggered -- vecTriggers.size() = %d\n", vecTriggers.size());

    for (const auto& pSuperblock : vecTriggers) {
        if (!pSuperblock) {
            LogPrintf("CSuperblockManager::IsSuperblockTriggered -- Non-superblock found, continuing\n");
            continue;
        }

        CGovernanceObject* pObj = pSuperblock->GetGovernanceObject();

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


bool CSuperblockManager::GetBestSuperblock(CSuperblock_sptr& pSuperblockRet, int nBlockHeight)
{
    if (!CSuperblock::IsValidBlockHeight(nBlockHeight)) {
        return false;
    }

    AssertLockHeld(governance->cs);
    std::vector<CSuperblock_sptr> vecTriggers = governance->GetActiveTriggers();
    int nYesCount = 0;

    for (const auto& pSuperblock : vecTriggers) {
        if (!pSuperblock || nBlockHeight != pSuperblock->GetBlockHeight()) {
            continue;
        }

        const CGovernanceObject* pObj = pSuperblock->GetGovernanceObject();

        if (!pObj) {
            continue;
        }

        // DO WE HAVE A NEW WINNER?

        int nTempYesCount = pObj->GetAbsoluteYesCount(VOTE_SIGNAL_FUNDING);
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

bool CSuperblockManager::GetSuperblockPayments(int nBlockHeight, std::vector<CTxOut>& voutSuperblockRet)
{
    LOCK(governance->cs);

    // GET THE BEST SUPERBLOCK FOR THIS BLOCK HEIGHT

    CSuperblock_sptr pSuperblock;
    if (!CSuperblockManager::GetBestSuperblock(pSuperblock, nBlockHeight)) {
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

bool CSuperblockManager::IsValid(const CTransaction& txNew, int nBlockHeight, const CAmount &blockReward)
{
    // GET BEST SUPERBLOCK, SHOULD MATCH
    LOCK(governance->cs);

    CSuperblock_sptr pSuperblock;
    if (CSuperblockManager::GetBestSuperblock(pSuperblock, nBlockHeight)) {
        return pSuperblock->IsValid(txNew, nBlockHeight, blockReward);
    }

    return false;
}

void CSuperblockManager::ExecuteBestSuperblock(int nBlockHeight)
{
    LOCK(governance->cs);

    CSuperblock_sptr pSuperblock;
    if (GetBestSuperblock(pSuperblock, nBlockHeight)) {
        // All checks are done in CSuperblock::IsValid via IsBlockValueValid and IsBlockPayeeValid,
        // tip wouldn't be updated if anything was wrong. Mark this trigger as executed.
        pSuperblock->SetExecuted();
        governance->ResetVotedFundingTrigger();
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
    CSuperblock(uint256& nHash) :
    nGovObjHash(nHash),
    nBlockHeight(0),
    nStatus(SeenObjectStatus::Unknown),
    vecPayments()
{
    const CGovernanceObject* pGovObj = GetGovernanceObject();

    if (!pGovObj) {
        throw std::runtime_error("CSuperblock: Failed to find Governance Object");
    }

    LogPrint(BCLog::GOBJECT, "CSuperblock -- Constructor pGovObj: %s, nObjectType = %d\n",
                pGovObj->GetDataAsPlainString(), pGovObj->GetObjectType());

    if (pGovObj->GetObjectType() != GOVERNANCE_OBJECT_TRIGGER) {
        throw std::runtime_error("CSuperblock: Governance Object not a trigger");
    }

    UniValue obj = pGovObj->GetJSONObject();

    if (obj["type"].getInt<int>() != GOVERNANCE_OBJECT_TRIGGER) {
        throw std::runtime_error("CSuperblock: invalid data type");
    }

    // FIRST WE GET THE START HEIGHT, THE BLOCK HEIGHT AT WHICH THE PAYMENT SHALL OCCUR
    nBlockHeight = obj["event_block_height"].getInt<int>();

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

CGovernanceObject* CSuperblock::GetGovernanceObject()
{
    AssertLockHeld(governance->cs);
    CGovernanceObject* pObj = governance->FindGovernanceObject(nGovObjHash);
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
           ((nBlockHeight % Params().GetConsensus().SuperBlockCycle(nBlockHeight)) == 0);
}

void CSuperblock::GetNearestSuperblocksHeights(int nBlockHeight, int& nLastSuperblockRet, int& nNextSuperblockRet)
{
    const Consensus::Params& consensusParams = Params().GetConsensus();
    int nSuperblockStartBlock = consensusParams.nSuperblockStartBlock;
    int nSuperblockCycle = consensusParams.SuperBlockCycle(nBlockHeight);

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

CAmount CSuperblock::GetPaymentsLimit(int nBlockHeight)
{
    const CChainParams& chainParams = Params();
    const Consensus::Params& consensusParams = chainParams.GetConsensus();
    if (!IsValidBlockHeight(nBlockHeight)) {
        return 0;
    }

   const int nSuperblock = nBlockHeight / consensusParams.SuperBlockCycle(nBlockHeight);
    CAmount nPaymentsLimit = 0;
    if(nSuperblock > 120){
    	// some part of all blocks issued during the cycle goes to superblock, see GetBlockSubsidy
    	const CAmount &nSuperblockPartOfSubsidy = GetBlockSubsidy(nBlockHeight, chainParams.GetConsensus(), true);
    	nPaymentsLimit = nSuperblockPartOfSubsidy * consensusParams.SuperBlockCycle(nBlockHeight);
    }
    // bootstrapping period
    else {
        switch(nSuperblock)
        {
            case 1:
                        nPaymentsLimit =  1500000.00 *COIN;
                        break;
            case 2:
                        nPaymentsLimit =  1470000.00 *COIN;
                        break;
            case 3:
                        nPaymentsLimit =  1440600.00 *COIN;
                        break;
            case 4:
                        nPaymentsLimit =  1411788.00 *COIN;
                        break;
            case 5:
                        nPaymentsLimit =  1383552.24 *COIN;
                        break;
            case 6:
                        nPaymentsLimit =  1355881.20 *COIN;
                        break;
            case 7:
                        nPaymentsLimit =  1328763.57 *COIN;
                        break;
            case 8:
                        nPaymentsLimit =  1302188.30 *COIN;
                        break;
            case 9:
                        nPaymentsLimit =  1276144.53 *COIN;
                        break;
            case 10:
                        nPaymentsLimit =  1186814.42 *COIN;
                        break;
            case 11:
                        nPaymentsLimit =  1103737.41 *COIN;
                        break;
            case 12:
                        nPaymentsLimit =  1026475.79 *COIN;
                        break;
            case 13:
                        nPaymentsLimit =  954622.48 *COIN;
                        break;
            case 14:
                        nPaymentsLimit =  840067.79 *COIN;
                        break;
            case 15:
                        nPaymentsLimit =  739259.65 *COIN;
                        break;
            case 16:
                        nPaymentsLimit =  650548.49 *COIN;
                        break;
            case 17:
                        nPaymentsLimit =  572482.67 *COIN;
                        break;
            case 18:
                        nPaymentsLimit =  503784.75 *COIN;
                        break;
            case 19:
                        nPaymentsLimit =  443330.58 *COIN;
                        break;
            case 20:
                        nPaymentsLimit =  376831.00 *COIN;
                        break;
            case 21:
                        nPaymentsLimit =  320306.35 *COIN;
                        break;
            case 22:
                        nPaymentsLimit =  272260.39 *COIN;
                        break;
            case 23:
                        nPaymentsLimit =  0;
                        break;
            case 24:
                        /* Syscoin 4.2 Upgrade - combine superblock 23 and 24 */
                        nPaymentsLimit =  (231421.33 + 196708.13) *COIN;
                        break;
            case 25:
                        nPaymentsLimit =  151767.00 *COIN;
                        break;
            case 26:
                        nPaymentsLimit =  151767.00 *COIN;
                        break;
            case 27:
                        nPaymentsLimit =  151767.00 *COIN;
                        break;
            case 28:
                        nPaymentsLimit =  151767.00 *COIN;
                        break;
            case 29:
                        nPaymentsLimit =  151767.00 *COIN;
                        break;
            case 30:
                        nPaymentsLimit =  151767.00 *COIN;
                        break;
            case 31:
                        nPaymentsLimit =  151767.00 *COIN;
                        break;
            case 32:
                        nPaymentsLimit =  151767.00 *COIN;
                        break;
            case 33:
                        nPaymentsLimit =  151767.00 *COIN;
                        break;
            case 34:
                        nPaymentsLimit =  151767.00 *COIN;
                        break;
            case 35:
                        nPaymentsLimit =  151767.00 *COIN;
                        break;
            case 36:
                        nPaymentsLimit =  151767.00 *COIN;
                        break;
            case 37:
                        nPaymentsLimit =  144178.65 *COIN;
                        break;
            case 38:
                        nPaymentsLimit =  144178.65 *COIN;
                        break;
            case 39:
                        nPaymentsLimit =  144178.65 *COIN;
                        break;
            case 40:
                        nPaymentsLimit =  144178.65 *COIN;
                        break;
            case 41:
                        nPaymentsLimit =  144178.65 *COIN;
                        break;
            case 42:
                        nPaymentsLimit =  144178.65 *COIN;
                        break;
            case 43:
                        nPaymentsLimit =  144178.65 *COIN;
                        break;
            case 44:
                        nPaymentsLimit =  144178.65 *COIN;
                        break;
            case 45:
                        nPaymentsLimit =  144178.65 *COIN;
                        break;
            case 46:
                        nPaymentsLimit =  144178.65 *COIN;
                        break;
            case 47:
                        nPaymentsLimit =  144178.65 *COIN;
                        break;
            case 48:
                        nPaymentsLimit =  144178.65 *COIN;
                        break;
            case 49:
                        nPaymentsLimit =  136969.72 *COIN;
                        break;
            case 50:
                        nPaymentsLimit =  136969.72 *COIN;
                        break;
            case 51:
                        nPaymentsLimit =  136969.72 *COIN;
                        break;
            case 52:
                        nPaymentsLimit =  136969.72 *COIN;
                        break;
            case 53:
                        nPaymentsLimit =  136969.72 *COIN;
                        break;
            case 54:
                        nPaymentsLimit =  136969.72 *COIN;
                        break;
            case 55:
                        nPaymentsLimit =  136969.72 *COIN;
                        break;
            case 56:
                        nPaymentsLimit =  136969.72 *COIN;
                        break;
            case 57:
                        nPaymentsLimit =  136969.72 *COIN;
                        break;
            case 58:
                        nPaymentsLimit =  136969.72 *COIN;
                        break;
            case 59:
                        nPaymentsLimit =  136969.72 *COIN;
                        break;
            case 60:
                        nPaymentsLimit =  136969.72 *COIN;
                        break;
            case 61:
                        nPaymentsLimit =  130121.23 *COIN;
                        break;
            case 62:
                        nPaymentsLimit =  130121.23 *COIN;
                        break;
            case 63:
                        nPaymentsLimit =  130121.23 *COIN;
                        break;
            case 64:
                        nPaymentsLimit =  130121.23 *COIN;
                        break;
            case 65:
                        nPaymentsLimit =  130121.23 *COIN;
                        break;
            case 66:
                        nPaymentsLimit =  130121.23 *COIN;
                        break;
            case 67:
                        nPaymentsLimit =  130121.23 *COIN;
                        break;
            case 68:
                        nPaymentsLimit =  130121.23 *COIN;
                        break;
            case 69:
                        nPaymentsLimit =  130121.23 *COIN;
                        break;
            case 70:
                        nPaymentsLimit =  130121.23 *COIN;
                        break;
            case 71:
                        nPaymentsLimit =  130121.23 *COIN;
                        break;
            case 72:
                        nPaymentsLimit =  130121.23 *COIN;
                        break;
            case 73:
                        nPaymentsLimit =  123615.17 *COIN;
                        break;
            case 74:
                        nPaymentsLimit =  123615.17 *COIN;
                        break;
            case 75:
                        nPaymentsLimit =  123615.17 *COIN;
                        break;
            case 76:
                        nPaymentsLimit =  123615.17 *COIN;
                        break;
            case 77:
                        nPaymentsLimit =  123615.17 *COIN;
                        break;
            case 78:
                        nPaymentsLimit =  123615.17 *COIN;
                        break;
            case 79:
                        nPaymentsLimit =  123615.17 *COIN;
                        break;
            case 80:
                        nPaymentsLimit =  123615.17 *COIN;
                        break;
            case 81:
                        nPaymentsLimit =  123615.17 *COIN;
                        break;
            case 82:
                        nPaymentsLimit =  123615.17 *COIN;
                        break;
            case 83:
                        nPaymentsLimit =  123615.17 *COIN;
                        break;
            case 84:
                        nPaymentsLimit =  123615.17 *COIN;
                        break;
            case 85:
                        nPaymentsLimit =  117434.41 *COIN;
                        break;
            case 86:
                        nPaymentsLimit =  117434.41 *COIN;
                        break;
            case 87:
                        nPaymentsLimit =  117434.41 *COIN;
                        break;
            case 88:
                        nPaymentsLimit =  117434.41 *COIN;
                        break;
            case 89:
                        nPaymentsLimit =  117434.41 *COIN;
                        break;
            case 90:
                        nPaymentsLimit =  117434.41 *COIN;
                        break;
            case 91:
                        nPaymentsLimit =  117434.41 *COIN;
                        break;
            case 92:
                        nPaymentsLimit =  117434.41 *COIN;
                        break;
            case 93:
                        nPaymentsLimit =  117434.41 *COIN;
                        break;
            case 94:
                        nPaymentsLimit =  117434.41 *COIN;
                        break;
            case 95:
                        nPaymentsLimit =  117434.41 *COIN;
                        break;
            case 96:
                        nPaymentsLimit =  117434.41 *COIN;
                        break;
            case 97:
                        nPaymentsLimit =  111562.69 *COIN;
                        break;
            case 98:
                        nPaymentsLimit =  111562.69 *COIN;
                        break;
            case 99:
                        nPaymentsLimit =  111562.69 *COIN;
                        break;
            case 100:
                        nPaymentsLimit =  111562.69 *COIN;
                        break;
            case 101:
                        nPaymentsLimit =  111562.69 *COIN;
                        break;
            case 102:
                        nPaymentsLimit =  111562.69 *COIN;
                        break;
            case 103:
                        nPaymentsLimit =  111562.69 *COIN;
                        break;
            case 104:
                        nPaymentsLimit =  111562.69 *COIN;
                        break;
            case 105:
                        nPaymentsLimit =  111562.69 *COIN;
                        break;
            case 106:
                        nPaymentsLimit =  111562.69 *COIN;
                        break;
            case 107:
                        nPaymentsLimit =  111562.69 *COIN;
                        break;
            case 108:
                        nPaymentsLimit =  111562.69 *COIN;
                        break;
            case 109:
                        nPaymentsLimit =  105984.56 *COIN;
                        break;
            case 110:
                        nPaymentsLimit =  105984.56 *COIN;
                        break;
            case 111:
                        nPaymentsLimit =  105984.56 *COIN;
                        break;
            case 112:
                        nPaymentsLimit =  105984.56 *COIN;
                        break;
            case 113:
                        nPaymentsLimit =  105984.56 *COIN;
                        break;
            case 114:
                        nPaymentsLimit =  105984.56 *COIN;
                        break;
            case 115:
                        nPaymentsLimit =  105984.56 *COIN;
                        break;
            case 116:
                        nPaymentsLimit =  105984.56 *COIN;
                        break;
            case 117:
                        nPaymentsLimit =  105984.56 *COIN;
                        break;
            case 118:
                        nPaymentsLimit =  105984.56 *COIN;
                        break;
            case 119:
                        nPaymentsLimit =  105984.56 *COIN;
                        break;
            case 120:
                        nPaymentsLimit =  105984.56 *COIN;
                        break;
        }
    }
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
            ostr << "CSuperblock::ParsePaymentSchedule -- Invalid Syscoin Address : " << vecParsed1[i];
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

bool CSuperblock::IsValid(const CTransaction& txNew, int nBlockHeight, const CAmount &blockReward)
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

    {
        LOCK(governance->cs);
        LogPrint(BCLog::GOBJECT, "CSuperblock::IsValid -- nOutputs = %d, nPayments = %d, GetDataAsHexString = %s\n",
            nOutputs, nPayments, GetGovernanceObject()->GetDataAsHexString());
    }
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
    CAmount nPaymentsLimit = GetPaymentsLimit(nBlockHeight);
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

bool CSuperblock::IsExpired() const
{
    int nExpirationBlocks{0};
    // Executed triggers are kept for another superblock cycle (approximately 1 month for mainnet).
    // Other valid triggers are kept for ~1 day only (for mainnet, but no longer than a superblock cycle for other networks).
    // Everything else is pruned after ~1h (for mainnet, but no longer than a superblock cycle for other networks).
    switch (nStatus) {
    case SeenObjectStatus::Executed:
        nExpirationBlocks = Params().GetConsensus().SuperBlockCycle(nBlockHeight);
        break;
    case SeenObjectStatus::Valid:
        nExpirationBlocks = std::min(576, Params().GetConsensus().SuperBlockCycle(nBlockHeight));
        break;
    default:
        nExpirationBlocks = std::min(24, Params().GetConsensus().SuperBlockCycle(nBlockHeight));
        break;
    }

    int nExpirationBlock = nBlockHeight + nExpirationBlocks;

    LogPrint(BCLog::GOBJECT, "CSuperblock::IsExpired -- nBlockHeight = %d, nExpirationBlock = %d\n", nBlockHeight, nExpirationBlock);

    if (governance->GetCachedBlockHeight() > nExpirationBlock) {
        LogPrint(BCLog::GOBJECT, "CSuperblock::IsExpired -- Outdated trigger found\n");
        return true;
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

