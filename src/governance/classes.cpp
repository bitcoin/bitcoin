// Copyright (c) 2014-2024 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <governance/classes.h>

#include <chainparams.h>
#include <core_io.h>
#include <key_io.h>
#include <primitives/transaction.h>
#include <script/standard.h>
#include <util/strencodings.h>
#include <util/underlying.h>
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
             ToUnderlying(govObj.GetObjectType()));

    if (govObj.GetObjectType() != GovernanceObject::TRIGGER) {
        throw std::runtime_error("CSuperblock: Governance Object not a trigger");
    }

    UniValue obj = govObj.GetJSONObject();

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

    for (int i = 0; i < (int)vecPaymentAddresses.size(); i++) {
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

bool CSuperblock::IsValid(const CChain& active_chain, const CTransaction& txNew, int nBlockHeight, CAmount blockReward)
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

