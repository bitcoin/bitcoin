// Copyright (c) 2016-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blockencodings.h>
#include <chainparams.h>
#include <common/system.h>
#include <consensus/consensus.h>
#include <consensus/validation.h>
#include <crypto/sha256.h>
#include <crypto/siphash.h>
#include <logging/categories.h>
#include <random.h>
#include <streams.h>
#include <txmempool.h>
#include <util/log.h>
#include <validation.h>

#include <unordered_map>

CBlockHeaderAndShortTxIDs::CBlockHeaderAndShortTxIDs(const CBlock& block, uint64_t nonce, const std::set<uint32_t>& prefill_candidates)
    : nonce(nonce),
      header(block)
{
    prefilledtxn.reserve(prefill_candidates.size());
    shorttxids.reserve(block.vtx.size() - prefill_candidates.size());
    FillShortTxIDSelector();

    uint16_t prefill_index = 0;
    for (size_t i = 0; i < block.vtx.size(); i++) {
        // Always prefill the coinbase transaction.
        if(prefill_candidates.contains(i) || i == 0) {
            prefilledtxn.push_back({prefill_index, block.vtx[i]});
            prefill_index = 0;
        } else {
            const CTransaction& tx = *block.vtx[i];
            shorttxids.push_back(GetShortID(tx.GetWitnessHash()));
            prefill_index++;
        }
    }
}

void CBlockHeaderAndShortTxIDs::FillShortTxIDSelector() const
{
    DataStream stream{};
    stream << header << nonce;
    CSHA256 hasher;
    hasher.Write((unsigned char*)&(*stream.begin()), stream.end() - stream.begin());
    uint256 shorttxidhash;
    hasher.Finalize(shorttxidhash.begin());
    m_hasher.emplace(shorttxidhash.GetUint64(0), shorttxidhash.GetUint64(1));
}

uint64_t CBlockHeaderAndShortTxIDs::GetShortID(const Wtxid& wtxid) const
{
    static_assert(SHORTTXIDS_LENGTH == 6, "shorttxids calculation assumes 6-byte shorttxids");
    return (*Assert(m_hasher))(wtxid.ToUint256()) & 0xffffffffffffL;
}

/* Reconstructing a compact block is in the hot-path for block relay,
 * so we want to do it as quickly as possible. Because this often
 * involves iterating over the entire mempool, we put all the data we
 * need (ie the wtxid and a reference to the actual transaction data)
 * in a vector and iterate over the vector directly. This allows optimal
 * CPU caching behaviour, at a cost of only 40 bytes per transaction.
 */
ReadStatus PartiallyDownloadedBlock::InitData(const CBlockHeaderAndShortTxIDs& cmpctblock, const std::vector<std::pair<Wtxid, CTransactionRef>>& extra_txn)
{
    LogDebug(BCLog::CMPCTBLOCK, "Initializing PartiallyDownloadedBlock for block %s using a cmpctblock of %u bytes\n", cmpctblock.header.GetHash().ToString(), GetSerializeSize(cmpctblock));
    if (cmpctblock.header.IsNull() || (cmpctblock.shorttxids.empty() && cmpctblock.prefilledtxn.empty()))
        return READ_STATUS_INVALID;
    if (cmpctblock.shorttxids.size() + cmpctblock.prefilledtxn.size() > MAX_BLOCK_WEIGHT / MIN_SERIALIZABLE_TRANSACTION_WEIGHT)
        return READ_STATUS_INVALID;

    if (!header.IsNull() || !txn_available.empty()) return READ_STATUS_INVALID;

    header = cmpctblock.header;
    txn_available.resize(cmpctblock.BlockTxCount());

    std::vector<Wtxid> extra_wtxids{};

    bool debug_log = util::log::ShouldDebugLog(BCLog::CMPCTBLOCK);

    if (debug_log) {
        prefilled_count = cmpctblock.prefilledtxn.size();

        // A sorted vec of extra_txn's for cheaply checking if prefills
        // were redundant with the extrapool.
        for (const auto& [id, tx] : extra_txn) {
            extra_wtxids.push_back(id);
        }
        std::sort(extra_wtxids.begin(), extra_wtxids.end());
    }


    {
        int32_t lastprefilledindex = -1;
        LOCK(pool->cs);
        for (size_t i = 0; i < cmpctblock.prefilledtxn.size(); i++) {
            if (cmpctblock.prefilledtxn[i].tx->IsNull())
                return READ_STATUS_INVALID;

            lastprefilledindex += cmpctblock.prefilledtxn[i].index + 1; //index is a uint16_t, so can't overflow here
            if (lastprefilledindex > std::numeric_limits<uint16_t>::max())
                return READ_STATUS_INVALID;
            if ((uint32_t)lastprefilledindex > cmpctblock.shorttxids.size() + i) {
                // If we are inserting a tx at an index greater than our full list of shorttxids
                // plus the number of prefilled txn we've inserted, then we have txn for which we
                // have neither a prefilled txn or a shorttxid!
                return READ_STATUS_INVALID;
            }
            txn_available[lastprefilledindex] = cmpctblock.prefilledtxn[i].tx;

            size_t tx_size = debug_log ? cmpctblock.prefilledtxn[i].tx->ComputeTotalSize() : 0;
            // Only consider prefilled transactions that were NOT in our mempool as candidates
            // that WE want to prefill.
            auto tx_wtxid =  cmpctblock.prefilledtxn[i].tx->GetWitnessHash();
            if (!pool->exists(tx_wtxid)) {
                prefill_candidates.insert(lastprefilledindex);
                if (std::binary_search(extra_wtxids.begin(), extra_wtxids.end(), tx_wtxid)) {
                    redundant_prefilled_ep_count++;
                    redundant_prefilled_ep_size += tx_size;
                } else {
                    redundant_prefilled_mp_count++;
                    redundant_prefilled_mp_size += tx_size;
                }
            }
        }
    }

    // Calculate map of txids -> positions and check mempool to see what we have (or don't)
    // Because well-formed cmpctblock messages will have a (relatively) uniform distribution
    // of short IDs, any highly-uneven distribution of elements can be safely treated as a
    // READ_STATUS_FAILED.
    std::unordered_map<uint64_t, uint16_t> shorttxids(cmpctblock.shorttxids.size());
    uint16_t index_offset = 0;
    for (size_t i = 0; i < cmpctblock.shorttxids.size(); i++) {
        while (txn_available[i + index_offset])
            index_offset++;
        shorttxids[cmpctblock.shorttxids[i]] = i + index_offset;
        // To determine the chance that the number of entries in a bucket exceeds N,
        // we use the fact that the number of elements in a single bucket is
        // binomially distributed (with n = the number of shorttxids S, and p =
        // 1 / the number of buckets), that in the worst case the number of buckets is
        // equal to S (due to std::unordered_map having a default load factor of 1.0),
        // and that the chance for any bucket to exceed N elements is at most
        // buckets * (the chance that any given bucket is above N elements).
        // Thus: P(max_elements_per_bucket > N) <= S * (1 - cdf(binomial(n=S,p=1/S), N)).
        // If we assume blocks of up to 16000, allowing 12 elements per bucket should
        // only fail once per ~1 million block transfers (per peer and connection).
        if (shorttxids.bucket_size(shorttxids.bucket(cmpctblock.shorttxids[i])) > 12)
            return READ_STATUS_FAILED;
    }
    if (shorttxids.size() != cmpctblock.shorttxids.size())
        return READ_STATUS_FAILED; // Short ID collision

    std::vector<bool> have_txn(txn_available.size());
    {
    LOCK(pool->cs);
    for (const auto& [wtxid, txit] : pool->txns_randomized) {
        uint64_t shortid = cmpctblock.GetShortID(wtxid);
        std::unordered_map<uint64_t, uint16_t>::iterator idit = shorttxids.find(shortid);
        if (idit != shorttxids.end()) {
            if (!have_txn[idit->second]) {
                txn_available[idit->second] = txit->GetSharedTx();
                have_txn[idit->second]  = true;
                mempool_count++;
                if (debug_log) {
                    mempool_size += txn_available[idit->second]->ComputeTotalSize();
                }
            } else if (txn_available[idit->second]) {
                // If we find two mempool txn that match the short id, just request it.
                // This should be rare enough that the extra bandwidth doesn't matter,
                // but eating a round-trip due to FillBlock failure would be annoying
                if (debug_log) {
                    mempool_size -= txn_available[idit->second]->ComputeTotalSize();
                }
                txn_available[idit->second].reset();
                mempool_count--;
            }
        }
        // Though ideally we'd continue scanning for the two-txn-match-shortid case,
        // the performance win of an early exit here is too good to pass up and worth
        // the extra risk.
        if (mempool_count == shorttxids.size())
            break;
    }
    }

    for (size_t i = 0; i < extra_txn.size(); i++) {
        uint64_t shortid = cmpctblock.GetShortID(extra_txn[i].first);
        std::unordered_map<uint64_t, uint16_t>::iterator idit = shorttxids.find(shortid);
        if (idit != shorttxids.end()) {
            if (!have_txn[idit->second]) {
                txn_available[idit->second] = extra_txn[i].second;
                prefill_candidates.insert(idit->second);
                have_txn[idit->second]  = true;
                extra_count++;
                if (debug_log) {
                    extra_size += txn_available[idit->second]->ComputeTotalSize();
                }
            } else {
                // If we find two mempool/extra txn that match the short id, just
                // request it.
                // This should be rare enough that the extra bandwidth doesn't matter,
                // but eating a round-trip due to FillBlock failure would be annoying
                // Note that we don't want duplication between extra_txn and mempool to
                // trigger this case, so we compare witness hashes first
                if (txn_available[idit->second] &&
                        txn_available[idit->second]->GetWitnessHash() != extra_txn[i].second->GetWitnessHash()) {
                    if (debug_log) {
                        extra_size -= txn_available[idit->second]->ComputeTotalSize();
                    }
                    txn_available[idit->second].reset();
                    extra_count--;
                }
            }
        }
        // Though ideally we'd continue scanning for the two-txn-match-shortid case,
        // the performance win of an early exit here is too good to pass up and worth
        // the extra risk.
        if (mempool_count + extra_count == shorttxids.size())
            break;
    }

    LogDebug(BCLog::CMPCTBLOCK, "Initialized PartiallyDownloadedBlock for block %s using a cmpctblock of %u bytes\n", cmpctblock.header.GetHash().ToString(), GetSerializeSize(cmpctblock));

    return READ_STATUS_OK;
}

bool PartiallyDownloadedBlock::IsTxAvailable(size_t index) const
{
    if (header.IsNull()) return false;

    assert(index < txn_available.size());
    return txn_available[index] != nullptr;
}

std::set<uint32_t> PartiallyDownloadedBlock::PrefillCandidates() const
{
    return prefill_candidates;
}

ReadStatus PartiallyDownloadedBlock::FillBlock(CBlock& block, const std::vector<CTransactionRef>& vtx_missing, bool segwit_active)
{
    if (header.IsNull()) return READ_STATUS_INVALID;

    block = header;
    block.vtx.resize(txn_available.size());

    size_t tx_missing_offset = 0;
    for (size_t i = 0; i < txn_available.size(); i++) {
        if (!txn_available[i]) {
            if (tx_missing_offset >= vtx_missing.size()) {
                return READ_STATUS_INVALID;
            }
            prefill_candidates.insert(i);
            block.vtx[i] = vtx_missing[tx_missing_offset++];
        } else {
            block.vtx[i] = std::move(txn_available[i]);
        }
    }

    // Make sure we can't call FillBlock again.
    header.SetNull();
    txn_available.clear();

    if (vtx_missing.size() != tx_missing_offset) {
        return READ_STATUS_INVALID;
    }

    // Check for possible mutations early now that we have a seemingly good block
    IsBlockMutatedFn check_mutated{m_check_block_mutated_mock ? m_check_block_mutated_mock : IsBlockMutated};
    if (check_mutated(/*block=*/block, /*check_witness_root=*/segwit_active)) {
        return READ_STATUS_FAILED; // Possible Short ID collision
    }

    if (util::log::ShouldDebugLog(BCLog::CMPCTBLOCK)) {
        const uint256 hash{block.GetHash()};
        uint32_t tx_missing_size{0};
        for (const auto& tx : vtx_missing) { tx_missing_size += tx->ComputeTotalSize(); }
        LogDebug(BCLog::CMPCTBLOCK,
            "Successfully reconstructed block %s with %u txn prefilled (%u bytes), "
            "%u txn from mempool (%u bytes), "
            "%u txn from extrapool (%u bytes), "
            "and %u txn requested (%u bytes)",
            hash.ToString(),
            prefilled_count, prefilled_size,
            mempool_count, mempool_size,
            extra_count, extra_size,
            vtx_missing.size(), tx_missing_size);
        LogDebug(BCLog::CMPCTBLOCK,
            "%u txn (%u bytes) of the prefill were redundant, "
            "%u txn (%u bytes) were present in the mempool, "
            "%u txn (%u bytes) were present in the extrapool. ",
            redundant_prefilled_mp_count + redundant_prefilled_ep_count,
            redundant_prefilled_mp_size + redundant_prefilled_ep_size,
            redundant_prefilled_mp_count, redundant_prefilled_mp_size,
            redundant_prefilled_ep_count, redundant_prefilled_ep_size);


        if (util::log::ShouldTraceLog(BCLog::CMPCTBLOCK)) {
            for (const auto& tx : vtx_missing) {
                LogTrace(BCLog::CMPCTBLOCK, "Reconstructed block %s required tx %s\n", hash.ToString(), tx->GetHash().ToString());
            }
        }
    }

    return READ_STATUS_OK;
}
