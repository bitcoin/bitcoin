#include <common/sv2_messages.h>

#include <arith_uint256.h>
#include <consensus/validation.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <hash.h>
#include <streams.h>
#include <deque>
#include <iostream>

uint256 HashTwoTxIDs(const uint256& txid1, const uint256& txid2) {
    HashWriter hasher{};
    hasher << txid1 << txid2;
    return hasher.GetHash();
}
std::vector<uint256> GetMerklePathForCoinbase(const CBlock& block) {
    auto size = block.vtx.size();
    // If we have only the coinbase tx, we don't have a merkle path
    if (size == 1) {
        return {};
    // If we have coinbase tx and another tx the path is the second node id
    } else if (size == 2) {
        std::vector<uint256> path;
        path.push_back(block.vtx[1]->GetHash());
        return path;
    // Otherwise we calculate the merkle path
    } else {
        std::deque<uint256> id_list;
        for (const auto& tx : block.vtx) {
            id_list.push_back(tx->GetHash());
        }
        // Last id must be duplicated when txs are odds
        if (size % 2 == 1) {
            id_list.push_back(block.vtx[size - 1]->GetHash());
        }

        // Remove coinbase
        id_list.pop_front();

        std::vector<uint256> path;

        // First path element is always the second tx
        path.push_back(id_list.front());
        id_list.pop_front();

        while (!id_list.empty()) {
            for (size_t i = 0; i < id_list.size() / 2; ++i) {
                id_list[i] = HashTwoTxIDs(id_list[i * 2], id_list[i * 2 + 1]);
            }
            id_list.resize(id_list.size()/2);
            path.push_back(id_list.front());
            id_list.pop_front();
            if (id_list.size() % 2 == 1) {
                id_list.push_back(id_list[id_list.size() - 1]);
            }
        }

        return path;
    }
}

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

    std::vector<uint256> merklepath = GetMerklePathForCoinbase(block);

    for (const auto& hash : merklepath) {
        m_merkle_path.push_back(hash);
    }

}

node::Sv2SetNewPrevHashMsg::Sv2SetNewPrevHashMsg(const CBlock& block, uint64_t template_id) : m_template_id{template_id}
{
    m_prev_hash = block.hashPrevBlock;
    m_header_timestamp = block.nTime;
    m_nBits = block.nBits;
    m_target = ArithToUint256(arith_uint256().SetCompact(block.nBits));
}
