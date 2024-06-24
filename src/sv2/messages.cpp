#include <sv2/messages.h>

#include <arith_uint256.h>
#include <primitives/block.h>
#include <primitives/transaction.h>

node::Sv2NewTemplateMsg::Sv2NewTemplateMsg(const CBlockHeader& header, const CTransactionRef coinbase_tx, std::vector<uint256> coinbase_merkle_path, int witness_commitment_index, uint64_t template_id, bool future_template)
    : m_template_id{template_id}, m_future_template{future_template}
{
    m_version = header.nVersion;

    m_coinbase_tx_version = coinbase_tx->CURRENT_VERSION;
    m_coinbase_prefix = coinbase_tx->vin[0].scriptSig;
    m_coinbase_tx_input_sequence = coinbase_tx->vin[0].nSequence;

    // The coinbase nValue already contains the nFee + the Block Subsidy when built using CreateBlock().
    m_coinbase_tx_value_remaining = static_cast<uint64_t>(coinbase_tx->vout[0].nValue);

    m_coinbase_tx_outputs_count = 0;
    if (witness_commitment_index != NO_WITNESS_COMMITMENT) {
        m_coinbase_tx_outputs_count = 1;

        std::vector<CTxOut> coinbase_tx_outputs{coinbase_tx->vout[witness_commitment_index]};
        m_coinbase_tx_outputs = coinbase_tx_outputs;
    }

    m_coinbase_tx_locktime = coinbase_tx->nLockTime;

    for (const auto& hash : coinbase_merkle_path) {
        m_merkle_path.push_back(hash);
    }

}

node::Sv2SetNewPrevHashMsg::Sv2SetNewPrevHashMsg(const CBlockHeader& header, uint64_t template_id) : m_template_id{template_id}
{
    m_prev_hash = header.hashPrevBlock;
    m_header_timestamp = header.nTime;
    m_nBits = header.nBits;
    m_target = ArithToUint256(arith_uint256().SetCompact(header.nBits));
}
