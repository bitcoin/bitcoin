#include <arith_uint256.h>
#include <consensus/validation.h>
#include <node/sv2_messages.h>
#include <primitives/block.h>
#include <primitives/transaction.h>

node::Sv2NewTemplateMsg::Sv2NewTemplateMsg(const CBlock& block, uint64_t template_id, bool future_template)
    : m_template_id{template_id}, m_future_template{future_template}
{
    m_version = block.GetBlockHeader().nVersion;

    const CTransactionRef coinbase_tx = block.vtx[0];
    m_coinbase_tx_version = coinbase_tx->CURRENT_VERSION;
    m_coinbase_prefix = coinbase_tx->vin[0].scriptSig;
    m_coinbase_tx_input_sequence = coinbase_tx->vin[0].nSequence;

    // The coinbase nValue already contains the nFee + the Block Subsidy when built using CreateBlock().
    m_coinbase_tx_value_remaining = static_cast<uint64_t>(block.vtx[0]->vout[0].nValue);

    m_coinbase_tx_outputs_count = 0;
    int commitpos = GetWitnessCommitmentIndex(block);
    if (commitpos != NO_WITNESS_COMMITMENT) {
        m_coinbase_tx_outputs_count = 1;

        std::vector<CTxOut> coinbase_tx_outputs{block.vtx[0]->vout[commitpos]};
        m_coinbase_tx_outputs = coinbase_tx_outputs;
    }

    m_coinbase_tx_locktime = coinbase_tx->nLockTime;

    // Skip the coinbase_tx hash from the merkle path since the downstream client
    // will build their own coinbase tx.
    for (auto it = block.vtx.begin() + 1; it != block.vtx.end(); ++it) {
        m_merkle_path.push_back((*it)->GetHash());
    }
}

node::Sv2SetNewPrevHashMsg::Sv2SetNewPrevHashMsg(const CBlock& block, uint64_t template_id) : m_template_id{template_id}
{
    m_prev_hash = block.hashPrevBlock;
    m_header_timestamp = block.nTime;
    m_nBits = block.nBits;
    m_target = ArithToUint256(arith_uint256().SetCompact(block.nBits));
}
