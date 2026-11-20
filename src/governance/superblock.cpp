// Copyright (c) 2014-2025 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <governance/superblock.h>

#include <chainparams.h>
#include <core_io.h>
#include <evo/deterministicmns.h>
#include <governance/object.h>
#include <governance/vote.h>
#include <key_io.h>
#include <logging.h>
#include <primitives/transaction.h>
#include <util/std23.h>
#include <util/strencodings.h>
#include <util/time.h>
#include <validation.h>

#include <univalue.h>

CAmount ParsePaymentAmount(const std::string& strAmount)
{
    CAmount nAmount = 0;
    if (strAmount.empty()) {
        throw std::runtime_error(strprintf("%s -- Amount is empty", __func__));
    }
    if (strAmount.size() > 20) {
        // String is much too long, the functions below impose stricter
        // requirements
        throw std::runtime_error(strprintf("%s -- Amount string too long", __func__));
    }
    // Make sure the string makes sense as an amount
    // Note: No spaces allowed
    // Also note: No scientific notation
    size_t pos = strAmount.find_first_not_of("0123456789.");
    if (pos != std::string::npos) {
        throw std::runtime_error(strprintf("%s -- Amount string contains invalid character", __func__));
    }

    pos = strAmount.find('.');
    if (pos == 0) {
        // JSON doesn't allow values to start with a decimal point
        throw std::runtime_error(strprintf("%s -- Invalid amount string, leading decimal point not allowed", __func__));
    }

    // Make sure there's no more than 1 decimal point
    if ((pos != std::string::npos) && (strAmount.find('.', pos + 1) != std::string::npos)) {
        throw std::runtime_error(strprintf("%s -- Invalid amount string, too many decimal points", __func__));
    }

    // Note this code is taken from AmountFromValue in rpcserver.cpp
    // which is used for parsing the amounts in createrawtransaction.
    if (!ParseFixedPoint(strAmount, 8, &nAmount)) {
        nAmount = 0;
        throw std::runtime_error(strprintf("%s -- ParseFixedPoint failed for string \"%s\"", __func__, strAmount));
    }
    if (!MoneyRange(nAmount)) {
        nAmount = 0;
        throw std::runtime_error(strprintf("%s -- Invalid amount string, value outside of valid money range", __func__));
    }

    return nAmount;
}

CSuperblock::
    CSuperblock() :
    nGovObjHash(),
    nBlockHeight(0),
    nStatus(SeenObjectStatus::Unknown),
    vecPayments()
{
}

CSuperblock::CSuperblock(const CGovernanceObject& govObj, uint256& nHash) :
    nGovObjHash(nHash),
    nBlockHeight(0),
    nStatus(SeenObjectStatus::Unknown),
    vecPayments()
{
    LogPrint(BCLog::GOBJECT, "CSuperblock -- Constructor govobj: %s, nObjectType = %d\n", govObj.GetDataAsPlainString(),
             std23::to_underlying(govObj.GetObjectType()));

    if (govObj.GetObjectType() != GovernanceObject::TRIGGER) {
        throw std::runtime_error("CSuperblock: Governance Object not a trigger");
    }

    UniValue obj = govObj.GetJSONObject();

    if (obj["type"].getInt<int>() != std23::to_underlying(GovernanceObject::TRIGGER)) {
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

    const bool fV20Active{nBlockHeight >= consensusParams.V20Height};

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

    const auto vecPaymentAddresses = SplitString(strPaymentAddresses, "|");
    const auto vecPaymentAmounts = SplitString(strPaymentAmounts, "|");
    const auto vecProposalHashes = SplitString(strProposalHashes, "|");

    // IF THESE DON'T MATCH, SOMETHING IS WRONG

    if (vecPaymentAddresses.size() != vecPaymentAmounts.size() || vecPaymentAddresses.size() != vecProposalHashes.size()) {
        std::string msg{strprintf("CSuperblock::%s -- Mismatched payments, amounts and proposalHashes", __func__)};
        LogPrintf("%s\n", msg);
        throw std::runtime_error(msg);
    }

    if (vecPaymentAddresses.empty()) {
        std::string msg{strprintf("CSuperblock::%s -- Error no payments", __func__)};
        LogPrintf("%s\n", msg);
        throw std::runtime_error(msg);
    }

    // LOOP THROUGH THE ADDRESSES/AMOUNTS AND CREATE PAYMENTS
    /*
      ADDRESSES = [ADDR1|2|3|4|5|6]
      AMOUNTS = [AMOUNT1|2|3|4|5|6]
    */

    for (int i = 0; i < static_cast<int>(vecPaymentAddresses.size()); i++) {
        CTxDestination dest = DecodeDestination(vecPaymentAddresses[i]);
        if (!IsValidDestination(dest)) {
            std::string msg{strprintf("CSuperblock::%s -- Invalid Dash Address: %s", __func__, vecPaymentAddresses[i])};
            LogPrintf("%s\n", msg);
            throw std::runtime_error(msg);
        }

        CAmount nAmount = ParsePaymentAmount(vecPaymentAmounts[i]);

        uint256 proposalHash;
        if (!ParseHashStr(vecProposalHashes[i], proposalHash)) {
            std::string msg{strprintf("CSuperblock::%s -- Invalid proposal hash: %s", __func__, vecProposalHashes[i])};
            LogPrintf("%s\n", msg);
            throw std::runtime_error(msg);
        }

        LogPrint(BCLog::GOBJECT, /* Continued */
                 "CSuperblock::%s -- i = %d, amount string = %s, nAmount = %lld, proposalHash = %s\n", __func__,
                 i, vecPaymentAmounts[i], nAmount, proposalHash.ToString());

        CGovernancePayment payment(dest, nAmount, proposalHash);
        if (payment.IsValid()) {
            vecPayments.push_back(payment);
        } else {
            vecPayments.clear();
            std::string msg{strprintf("CSuperblock::%s -- Invalid payment found: address = %s, amount = %d", __func__,
                EncodeDestination(dest), nAmount)};
            LogPrintf("%s\n", msg);
            throw std::runtime_error(msg);
        }
    }
}

bool CSuperblock::GetPayment(int nPaymentIndex, CGovernancePayment& paymentRet)
{
    if ((nPaymentIndex < 0) || (nPaymentIndex >= static_cast<int>(vecPayments.size()))) {
        return false;
    }

    paymentRet = vecPayments[nPaymentIndex];
    return true;
}

CAmount CSuperblock::GetPaymentsTotalAmount()
{
    return std23::ranges::fold_left(vecPayments, CAmount{0}, [](CAmount s, const auto& p) { return s + p.nAmount; });
}

/**
*   Is Transaction Valid
*
*   - Does this transaction match the superblock?
*/

bool CSuperblock::IsValid(const CChain& active_chain, const CTransaction& txNew, int block_height, CAmount blockReward, bool is_v24)
{
    // TODO : LOCK(cs);
    // No reason for a lock here now since this method only accesses data
    // internal to *this and since CSuperblock's are accessed only through
    // shared pointers there's no way our object can get deleted while this
    // code is running.
    if (!IsValidBlockHeight(block_height)) {
        LogPrintf("CSuperblock::IsValid -- ERROR: Block invalid, incorrect block height\n");
        return false;
    }

    // CONFIGURE SUPERBLOCK OUTPUTS

    int nOutputs = txNew.vout.size();
    int nPayments = CountPayments();
    int nMinerAndMasternodePayments = nOutputs - nPayments;

    LogPrint(BCLog::GOBJECT, "CSuperblock::IsValid -- nOutputs = %d, nPayments = %d, hash = %s\n", nOutputs, nPayments,
             nGovObjHash.ToString());

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
    CAmount nPaymentsLimit = GetPaymentsLimit(active_chain, block_height);
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

    int nVoutIndex = -1;
    for (int i = 0; i < nPayments; i++) {
        CGovernancePayment payment;
        if (!GetPayment(i, payment)) {
            // This shouldn't happen so log a warning
            LogPrintf("CSuperblock::IsValid -- WARNING: Failed to find payment: %d of %d total payments\n", i, nPayments);
            continue;
        }

        bool fPaymentMatch = false;

        // From V24 on, start past the previously matched output so each expected
        // payment consumes a distinct vout (two adjacent payments with the same
        // script and amount must match two separate outputs, not the same one
        // twice). Before V24 the scan restarted at the previously matched index
        // (inclusive), which is kept for backwards compatibility.
        // TODO: After V24 is hardened/finalized so historical duplicate-output
        // blocks cannot be encountered, simplify this path to the V24 scan only.
        const int nVoutStart = is_v24 ? nVoutIndex + 1 : std::max(nVoutIndex, 0);
        for (int j = nVoutStart; j < nOutputs; j++) {
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

bool CSuperblock::IsExpired(int heightToTest) const
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

    if (heightToTest > nExpirationBlock) {
        LogPrint(BCLog::GOBJECT, "CSuperblock::IsExpired -- Outdated trigger found\n");
        return true;
    }

    if (Params().NetworkIDString() != CBaseChainParams::MAIN) {
        // NOTE: this can happen on testnet/devnets due to reorgs, should never happen on mainnet
        if (heightToTest + Params().GetConsensus().nSuperblockCycle * 2 < nBlockHeight) {
            LogPrint(BCLog::GOBJECT, "CSuperblock::IsExpired -- Trigger is too far into the future\n");
            return true;
        }
    }

    return false;
}

std::vector<uint256> CSuperblock::GetProposalHashes() const
{
    std::vector<uint256> res;
    res.reserve(vecPayments.size());

    for (const auto& payment : vecPayments) {
        res.push_back(payment.proposalHash);
    }

    return res;
}

std::string CSuperblock::GetHexStrData() const
{
    // {\"event_block_height\": 879720, \"payment_addresses\": \"yd5KMREs3GLMe6mTJYr3YrH1juwNwrFCfB\", \"payment_amounts\": \"5.00000000\", \"proposal_hashes\": \"485817fddbcab6c55c9a6856dabc8b19ed79548bda8c01712daebc9f74f287f4\", \"type\": 2}

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

namespace governance {

int64_t EstimateBlockTime(int height, int ref_height, int64_t ref_time)
{
    // Blocks arrive slightly slower than the target spacing in practice
    return static_cast<int64_t>(ref_time + (height - ref_height) * 2.62 * 60);
}

ProposalWindow EstimateProposalWindow(int first_superblock, int payments, int superblock_cycle, int ref_height,
                                      int64_t ref_time)
{
    const int last_superblock{first_superblock + (payments - 1) * superblock_cycle};
    return {
        .start_epoch = EstimateBlockTime(first_superblock - superblock_cycle / 2, ref_height, ref_time),
        .end_epoch = EstimateBlockTime(last_superblock + superblock_cycle / 2, ref_height, ref_time),
    };
}

bool SuperblockManager::AddTrigger(std::shared_ptr<CGovernanceObject> obj, int cachedHeight)
{
    AssertLockNotHeld(cs_sb);
    if (!obj) return false;

    uint256 nHash = obj->GetHash();

    LOCK(cs_sb);
    if (m_triggers.contains(nHash)) {
        LogPrint(BCLog::GOBJECT, "SuperblockManager::%s -- Already have hash, nHash = %s, size = %s\n", __func__,
                 nHash.GetHex(), m_triggers.size());
        return false;
    }

    CSuperblock_sptr pSuperblock;
    try {
        pSuperblock = std::make_shared<CSuperblock>(*obj, nHash);
    } catch (std::exception& e) {
        LogPrintf("SuperblockManager::%s -- Error creating superblock: %s\n", __func__, e.what());
        return false;
    } catch (...) {
        LogPrintf("SuperblockManager::%s -- Unknown Error creating superblock\n", __func__);
        return false;
    }

    pSuperblock->SetStatus(SeenObjectStatus::Valid);
    m_triggers.emplace(nHash, TriggerEntry{pSuperblock, std::move(obj)});

    return !pSuperblock->IsExpired(cachedHeight);
}

void SuperblockManager::RemoveTrigger(const uint256& hash)
{
    LOCK(cs_sb);
    m_triggers.erase(hash);
}

void SuperblockManager::Clean(int cachedHeight)
{
    AssertLockNotHeld(cs_sb);
    LOCK(cs_sb);

    LogPrint(BCLog::GOBJECT, "SuperblockManager::%s -- m_triggers.size() = %d\n", __func__, m_triggers.size());

    for (auto it = m_triggers.begin(); it != m_triggers.end();) {
        bool remove = false;
        const auto& [sb, obj] = it->second;

        if (!sb) {
            LogPrint(BCLog::GOBJECT, "SuperblockManager::%s -- nullptr superblock\n", __func__);
            remove = true;
        } else {
            if (!obj || obj->GetObjectType() != GovernanceObject::TRIGGER) {
                LogPrint(BCLog::GOBJECT, "SuperblockManager::%s -- Unknown or non-trigger superblock\n", __func__);
                sb->SetStatus(SeenObjectStatus::ErrorInvalid);
            }
            LogPrint(BCLog::GOBJECT, "SuperblockManager::%s -- superblock status = %d\n", __func__,
                     std23::to_underlying(sb->GetStatus()));
            switch (sb->GetStatus()) {
            case SeenObjectStatus::ErrorInvalid:
            case SeenObjectStatus::Unknown:
                LogPrint(BCLog::GOBJECT, "SuperblockManager::%s -- Unknown or invalid trigger found\n", __func__);
                remove = true;
                break;
            case SeenObjectStatus::Valid:
            case SeenObjectStatus::Executed:
                LogPrint(BCLog::GOBJECT, "SuperblockManager::%s -- Valid trigger found\n", __func__);
                if (sb->IsExpired(cachedHeight)) {
                    if (obj) obj->SetExpired();
                    remove = true;
                }
                break;
            default:
                break;
            }
        }
        LogPrint(BCLog::GOBJECT, "SuperblockManager::%s -- %smarked for removal\n", __func__, remove ? "" : "NOT ");

        if (remove) {
            std::string strDataAsPlainString = "nullptr";
            if (obj) {
                strDataAsPlainString = obj->GetDataAsPlainString();
                obj->PrepareDeletion(GetTime<std::chrono::seconds>().count());
            }
            LogPrint(BCLog::GOBJECT, "SuperblockManager::%s -- Removing trigger object %s\n", __func__,
                     strDataAsPlainString);
            it = m_triggers.erase(it);
        } else {
            ++it;
        }
    }
}

void SuperblockManager::Clear()
{
    LOCK(cs_sb);
    m_triggers.clear();
    m_loaded = false;
}

std::vector<CSuperblock_sptr> SuperblockManager::GetActiveTriggers() const
{
    LOCK(cs_sb);
    std::vector<CSuperblock_sptr> vecResults;
    vecResults.reserve(m_triggers.size());
    for (const auto& [_, entry] : m_triggers) {
        if (entry.sb && entry.obj) {
            vecResults.push_back(entry.sb);
        }
    }
    return vecResults;
}

bool SuperblockManager::GetBestSuperblock(const CDeterministicMNList& tip_mn_list, CSuperblock_sptr& sbRet,
                                          int nBlockHeight) const
{
    LOCK(cs_sb);
    return GetBestSuperblockInternal(tip_mn_list, sbRet, nBlockHeight);
}

bool SuperblockManager::GetBestSuperblockInternal(const CDeterministicMNList& tip_mn_list, CSuperblock_sptr& sbRet,
                                                  int nBlockHeight) const
{
    AssertLockHeld(cs_sb);
    if (!CSuperblock::IsValidBlockHeight(nBlockHeight)) {
        return false;
    }

    int nYesCount = 0;
    for (const auto& [_, entry] : m_triggers) {
        if (!entry.sb || !entry.obj || nBlockHeight != entry.sb->GetBlockHeight()) {
            continue;
        }
        int nTempYesCount = entry.obj->GetAbsoluteYesCount(tip_mn_list, VOTE_SIGNAL_FUNDING);
        if (nTempYesCount > nYesCount) {
            nYesCount = nTempYesCount;
            sbRet = entry.sb;
        }
    }
    return nYesCount > 0;
}

bool SuperblockManager::IsSuperblockTriggered(const CDeterministicMNList& tip_mn_list, int nBlockHeight)
{
    LogPrint(BCLog::GOBJECT, "IsSuperblockTriggered -- Start nBlockHeight = %d\n", nBlockHeight);
    if (!CSuperblock::IsValidBlockHeight(nBlockHeight)) {
        return false;
    }

    LOCK(cs_sb);

    LogPrint(BCLog::GOBJECT, "IsSuperblockTriggered -- m_triggers.size() = %d\n", m_triggers.size());

    for (const auto& [_, entry] : m_triggers) {
        if (!entry.sb) {
            LogPrintf("IsSuperblockTriggered -- Non-superblock found, continuing\n");
            continue;
        }
        if (!entry.obj) {
            LogPrintf("IsSuperblockTriggered -- pObj == nullptr, continuing\n");
            continue;
        }

        LogPrint(BCLog::GOBJECT, "IsSuperblockTriggered -- data = %s\n", entry.obj->GetDataAsPlainString());

        if (nBlockHeight != entry.sb->GetBlockHeight()) {
            LogPrint(BCLog::GOBJECT, /* Continued */
                     "IsSuperblockTriggered -- block height doesn't match nBlockHeight = %d, blockStart = %d, "
                     "continuing\n",
                     nBlockHeight, entry.sb->GetBlockHeight());
            continue;
        }

        entry.obj->UpdateSentinelVariables(tip_mn_list);

        if (entry.obj->IsSetCachedFunding()) {
            LogPrint(BCLog::GOBJECT, "IsSuperblockTriggered -- fCacheFunding = true, returning true\n");
            return true;
        } else {
            LogPrint(BCLog::GOBJECT, "IsSuperblockTriggered -- fCacheFunding = false, continuing\n");
        }
    }

    return false;
}

bool SuperblockManager::IsValidSuperblock(const CChain& active_chain, const CDeterministicMNList& tip_mn_list,
                                          const CTransaction& txNew, int nBlockHeight, CAmount blockReward, bool is_v24) const
{
    LOCK(cs_sb);
    CSuperblock_sptr pSuperblock;
    if (GetBestSuperblockInternal(tip_mn_list, pSuperblock, nBlockHeight)) {
        return pSuperblock->IsValid(active_chain, txNew, nBlockHeight, blockReward, is_v24);
    }
    return false;
}

bool SuperblockManager::GetSuperblockPayments(const CDeterministicMNList& tip_mn_list, int nBlockHeight,
                                              std::vector<CTxOut>& voutSuperblockRet) const
{
    LOCK(cs_sb);

    CSuperblock_sptr pSuperblock;
    if (!GetBestSuperblockInternal(tip_mn_list, pSuperblock, nBlockHeight)) {
        LogPrint(BCLog::GOBJECT, "GetSuperblockPayments -- Can't find superblock for height %d\n", nBlockHeight);
        return false;
    }

    voutSuperblockRet.clear();

    // TODO: How many payments can we add before things blow up?
    //       Consider at least following limits:
    //          - max coinbase tx size
    //          - max "budget" available
    for (int i = 0; i < pSuperblock->CountPayments(); i++) {
        CGovernancePayment payment;
        if (pSuperblock->GetPayment(i, payment)) {
            voutSuperblockRet.emplace_back(payment.nAmount, payment.script);

            CTxDestination dest;
            ExtractDestination(payment.script, dest);

            LogPrint(BCLog::GOBJECT, "GetSuperblockPayments -- NEW Superblock: output %d (addr %s, amount %d.%08d)\n",
                     i, EncodeDestination(dest), payment.nAmount / COIN, payment.nAmount % COIN);
        } else {
            LogPrint(BCLog::GOBJECT, "GetSuperblockPayments -- Payment not found\n");
        }
    }

    return true;
}

void SuperblockManager::ExecuteBestSuperblock(const CDeterministicMNList& tip_mn_list, int nBlockHeight)
{
    LOCK(cs_sb);
    CSuperblock_sptr pSuperblock;
    if (GetBestSuperblockInternal(tip_mn_list, pSuperblock, nBlockHeight)) {
        // All checks are done in CSuperblock::IsValid via IsBlockValueValid and IsBlockPayeeValid,
        // tip wouldn't be updated if anything was wrong. Mark this trigger as executed.
        pSuperblock->SetExecuted();
    }
}

} // namespace governance
