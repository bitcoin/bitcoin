#include <mweb/mweb_node.h>

#include <chain.h>
#include <consensus/validation.h>
#include <mw/node/BlockValidator.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <script/interpreter.h>
#include <undo.h>
#include <util/system.h>
#include <validation.h>

using namespace MWEB;

bool Node::CheckBlock(const CBlock& block, BlockValidationState& state)
{
    // HasMWEBTx() is true only when mweb txs being shared outside of a block (for use by mempools).
    // Blocks themselves do not store mweb txs like normal txs.
    // They are instead stored and processed separately in the mweb block.
    // So at this point, no txs should contain MWEB data.
    for (const CTransactionRef& pTx : block.vtx) {
        if (pTx->HasMWEBTx()) {
            return state.Invalid(BlockValidationResult::BLOCK_MUTATED, "unexpected-mweb-data", "Block contains transactions with MWEB data attached");
        }
    }

    return true;
}

bool Node::ContextualCheckBlock(const CBlock& block, const Consensus::Params& consensus_params, const CBlockIndex* pindexPrev, BlockValidationState& state)
{
    // If MWEB is enabled, we must have an mweb_block. If it's not enabled, we must not have an MWEB block.
    if (!IsMWEBEnabled(pindexPrev, consensus_params)) {
        // No MWEB data is allowed in blocks that don't commit to MWEB data, as this would otherwise leave room for spam
        if (!block.mweb_block.IsNull()) {
            return state.Invalid(BlockValidationResult::BLOCK_MUTATED, "unexpected-mweb-data", "MWEB not activated, but extension block found");
        }

        return true;
    } else if (block.mweb_block.IsNull()) {
        return state.Invalid(BlockValidationResult::BLOCK_MUTATED, "mweb-missing", "MWEB activated but extension block not found");
    }

    // Validate each transaction in the block.
    for (const auto& tx : block.vtx) {
        // Verify that there are no pegin outputs in the coinbase or HogEx transactions.
        if (tx->IsCoinBase() || tx->IsHogEx()) {
            for (const CTxOut& out : tx->vout) {
                if (out.scriptPubKey.IsMWEBPegin()) {
                    return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-tx-unexpected-pegin", "Pegin found in coinbase or HogEx");
                }
            }
        }
    }

    // Last transaction must be marked as HogEx.
    // Note: We don't commit to the HogEx indicator, so if indicator not set, just mark the block as mutated.
    const CTransactionRef& pHogEx = block.vtx.back();
    if (!pHogEx->IsHogEx()) {
        return state.Invalid(BlockValidationResult::BLOCK_MUTATED, "hogex-missing", "hogex missing");
    }

    // Only the last transaction in the block should be marked as the HogEx.
    for (size_t i = 0; i < block.vtx.size() - 1; i++) {
        if (block.vtx[i]->IsHogEx()) {
            return state.Invalid(BlockValidationResult::BLOCK_MUTATED, "bad-hogex-position", "hogex in wrong position");
        }
    }

    // UnserializeTransaction ensures that vout is never empty when the HogEx indicator is set.
    assert(!pHogEx->vout.empty());

    // Verify that the first HogEx output contains the HogAddr.
    mw::Hash mweb_hash;
    if (!pHogEx->vout.front().scriptPubKey.IsMWEBHogAddr(&mweb_hash)) {
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-hogex", "HogEx missing or invalid");
    }

    // Verify that the MWEB header hash matches.
    if (mweb_hash != block.mweb_block.m_block->GetHash()) {
        return state.Invalid(BlockValidationResult::BLOCK_MUTATED, "mweb-hash-mismatch", "MWEB header hash doesn't match hogex hash");
    }

    // Verify that the MWEB block's height is correct.
    if (block.mweb_block.GetHeight() != (pindexPrev->nHeight + 1)) {
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "mweb-height-mismatch", "Invalid MWEB block height");
    }

    // For the very first HogEx transaction, all inputs are pegins, so start at index of 0.
    // For all other HogEx transaction, the first input is not a pegin, so start looking for pegins at index 1.
    const bool is_first_hogex = !IsMWEBEnabled(pindexPrev->pprev, consensus_params);
    size_t next_pegin_idx = is_first_hogex ? 0 : 1;

    // Loop through the block's txs looking for all outputs with pegin scriptPubKeys (skip Coinbase & HogEx which don't support pegins).
    // While looping though, we perform 2 tasks:
    // 
    // 1. Calculate the total value of the HogEx inputs (hogex_input_amount).
    // This is the previous HogEx's first output amount (pIndexPrev->mweb_amount) plus the sum of all pegin output amounts.
    // 
    // 2. Verify the HogEx inputs (ignoring the input that spends previous HogEx output) exactly match the pegin outputs.
    for (size_t nTx = 1; nTx < block.vtx.size() - 1; nTx++) {
        const CTransactionRef& pTx = block.vtx[nTx];
        for (size_t nOut = 0; nOut < pTx->vout.size(); nOut++) {
            const CTxOut& output = pTx->vout[nOut];
            if (output.scriptPubKey.IsMWEBPegin()) {
                if (pHogEx->vin.size() <= next_pegin_idx) {
                    return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "pegins-missing", "Pegins missing from HogEx");
                }

                const COutPoint& hogex_input = pHogEx->vin[next_pegin_idx++].prevout;
                if (hogex_input.n != nOut || hogex_input.hash != pTx->GetHash()) {
                    return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "pegin-mismatch", "HogEx pegins do not match block's pegins");
                }
            }
        }
    }

    // 'next_pegin_idx' should equal the size of pHogEx->vin, meaning all HogEx inputs match the pegin outputs.
    // If it's not equal, that means we have more HogEx inputs than pegin outputs.
    if (next_pegin_idx != pHogEx->vin.size()) {
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "extra-hogex-input", "HogEx contains unexpected input(s)");
    }

    // Verify that the mw::Block is valid, and the pegins and pegouts all match.
    if (!MWEB::Node::ValidateMWEBBlock(block)) {
        return state.Invalid(BlockValidationResult::BLOCK_MUTATED, "bad-blk-mweb", "BlockValidator::ValidateBlock failed");
    }

    return true;
}

bool Node::ValidateMWEBBlock(const CBlock& block)
{
    const CTransactionRef& pHogEx = block.vtx.back();

    // Find all pegin scriptPubKeys in the block.
    // We don't support pegins in the coinbase tx or the HogEx tx, so skip those.
    std::vector<PegInCoin> block_pegins;
    for (size_t i = 1; i < block.vtx.size() - 1; i++) {
        for (const CTxOut& out : block.vtx[i]->vout) {
            mw::Hash kernel_id;
            if (out.scriptPubKey.IsMWEBPegin(&kernel_id)) {
                block_pegins.push_back(PegInCoin{out.nValue, std::move(kernel_id)});
            }
        }
    }

    // Find all pegout outputs in the HogEx transaction.
    // The first output is not a pegout. It contains the MWEB balance and header hash.
    // The remaining outputs are all pegouts.
    std::vector<PegOutCoin> hogex_pegouts;
    for (size_t i = 1; i < pHogEx->vout.size(); i++) {
        const CTxOut& out = pHogEx->vout[i];
        const CScript& pubkey = out.scriptPubKey;
        hogex_pegouts.push_back(PegOutCoin(out.nValue, {pubkey.begin(), pubkey.end()}));
    }

    // Call into the libmw context-free block validator to validate the TxBody,
    // and verify that the pegins and pegouts all match.
    return BlockValidator::ValidateBlock(block.mweb_block.m_block, block_pegins, hogex_pegouts);
}

bool Node::ConnectBlock(const CBlock& block, const Consensus::Params& consensus_params, const CBlockIndex* pindexPrev, CBlockUndo& blockundo, mw::CoinsViewCache& mweb_view, BlockValidationState& state)
{
    if (!block.mweb_block.IsNull()) {
        const CTransactionRef& pHogEx = block.vtx.back();

        // Verify that the first input of the HogEx tx spends the first output of the previous block's HogEx tx.
        // There is one important exception case we must handle, though.
        // The very first HogEx tx won't have a previous one to spend.
        // For that special case, this check will be skipped, and its first input will be treated as a pegin.
        const bool is_first_hogex = !IsMWEBEnabled(pindexPrev->pprev, consensus_params);
        if (!is_first_hogex) {
            const COutPoint& prev_hogex_out = pHogEx->vin.front().prevout;
            if (prev_hogex_out.n != 0 || pindexPrev->hogex_hash != prev_hogex_out.hash) {
                return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "invalid-hogex-input", "First input of HogEx does not point to previous HogEx");
            }
        }
        
        // Loop through the block's txs looking for all outputs with pegin scriptPubKeys (skip Coinbase & HogEx which don't support pegins).
        // While looping though, we perform 2 tasks:
        //
        // 1. Calculate the total value of the HogEx inputs (hogex_input_amount).
        // This is the previous HogEx's first output amount (pIndexPrev->mweb_amount) plus the sum of all pegin output amounts.
        //
        // 2. Verify the HogEx inputs (ignoring the input that spends previous HogEx output) exactly match the pegin outputs.
        CAmount hogex_input_amount = pindexPrev->mweb_amount;
        for (size_t nTx = 1; nTx < block.vtx.size() - 1; nTx++) {
            const CTransactionRef& pTx = block.vtx[nTx];
            for (const CTxOut& output : pTx->vout) {
                if (output.scriptPubKey.IsMWEBPegin()) {
                    hogex_input_amount += output.nValue;
                    if (!MoneyRange(hogex_input_amount)) {
                        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "accumulated-pegin-outofrange", "Accumulated pegin amount for the block is out of range");
                    }
                }
            }
        }

        // For the HogEx transaction, the fee must be equal to the total fee of the extension block.
        CAmount hogex_fee = hogex_input_amount - pHogEx->GetValueOut();
        if (!MoneyRange(hogex_fee) || hogex_fee != block.mweb_block.GetTotalFee()) {
            return state.Invalid(BlockValidationResult::BLOCK_MUTATED, "bad-txns-mweb-fee-mismatch", "HogEx fee does not match MWEB fee."); // TODO: This can be CONSENSUS
        }

        // Verify that the value of the first HogEx output matches the expected new value of the MWEB.
        // This is calculated simply as: 'mweb_amount = previous_amount + supply_change'
        // where 'supply_change = (pegins - pegouts) - fees'
        CAmount mweb_amount = pindexPrev->mweb_amount + block.mweb_block.GetSupplyChange();
        if (mweb_amount != pHogEx->vout.front().nValue) {
            return state.Invalid(BlockValidationResult::BLOCK_MUTATED, "mweb-amount-mismatch", "HogEx amount does not match expected MWEB amount"); // TODO: This can be CONSENSUS
        }

        try {
            blockundo.mwundo = mweb_view.ApplyBlock(block.mweb_block.m_block);
        } catch (const std::exception& e) {
            // MWEB: Need to distinguish between invalid blocks and mutated blocks
            return state.Invalid(BlockValidationResult::BLOCK_MUTATED, "mweb-connect-failed", strprintf("MWEB::Node::ConnectBlock(): Failed to connect MWEB block: %s", e.what()));
        }
    }

    return true;
}

bool Node::CheckTransaction(const CTransaction& tx, TxValidationState& state)
{
    std::unordered_map<mw::Hash, CAmount> tx_pegins;
    for (const CTxOut& out : tx.vout) {
        mw::Hash kernel_id;
        if (out.scriptPubKey.IsMWEBPegin(&kernel_id)) {
            if (tx_pegins.find(kernel_id) != tx_pegins.end()) {
                return state.Invalid(TxValidationResult::TX_RECENT_CONSENSUS_CHANGE, "duplicate-tx-pegins");
            }

            tx_pegins.insert({std::move(kernel_id), out.nValue});
        }
    }
    
    auto pegin_coins = tx.mweb_tx.GetPegIns();
    if (pegin_coins.size() != tx_pegins.size()) {
        return state.Invalid(TxValidationResult::TX_WITNESS_MUTATED, "pegin-count-mismatch");
    }

    for (const auto& pegin : pegin_coins) {
        auto iter = tx_pegins.find(pegin.GetKernelID());
        if (iter == tx_pegins.end() || pegin.GetAmount() != iter->second) {
            return state.Invalid(TxValidationResult::TX_WITNESS_MUTATED, "pegin-mismatch");
        }
    }

    // If the transaction has MWEB data, call the libmw transaction validation logic.
    if (tx.HasMWEBTx()) {
        try {
            tx.mweb_tx.m_transaction->Validate();
        } catch (const std::exception& e) {
            return state.Invalid(TxValidationResult::TX_WITNESS_MUTATED, "bad-mweb-txn");
        }
    }

    return true;
}