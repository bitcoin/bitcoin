// Copyright (c) 2021-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/transaction.h>

#include <consensus/validation.h>
#include <interfaces/chain.h>

using interfaces::FoundBlock;

namespace wallet {
bool CWalletTx::IsEquivalentTo(const CWalletTx& _tx) const
{
        CMutableTransaction tx1 {*this->GetTx()};
        CMutableTransaction tx2 {*_tx.GetTx()};
        for (auto& txin : tx1.vin) {
            txin.scriptSig = CScript();
            txin.scriptWitness.SetNull();
        }
        for (auto& txin : tx2.vin) {
            txin.scriptSig = CScript();
            txin.scriptWitness.SetNull();
        }
        return CTransaction(tx1) == CTransaction(tx2);
}

bool CWalletTx::InMempool() const
{
    return state<TxStateInMempool>();
}

int64_t CWalletTx::GetTxTime() const
{
    int64_t n = nTimeSmart;
    return n ? n : nTimeReceived;
}

void CWalletTx::updateState(interfaces::Chain& chain)
{
    bool active;
    auto lookup_block = [&](const uint256& hash, int& height, TxState& state) {
        // If tx block (or conflicting block) was reorged out of chain
        // while the wallet was shutdown, change tx status to UNCONFIRMED
        // and reset block height, hash, and index. ABANDONED tx don't have
        // associated blocks and don't need to be updated. The case where a
        // transaction was reorged out while online and then reconfirmed
        // while offline is covered by the rescan logic.
        if (!chain.findBlock(hash, FoundBlock().inActiveChain(active).height(height)) || !active) {
            state = TxStateInactive{};
        }
    };
    if (auto* conf = state<TxStateConfirmed>()) {
        lookup_block(conf->confirmed_block_hash, conf->confirmed_block_height, m_state);
    } else if (auto* conf = state<TxStateBlockConflicted>()) {
        lookup_block(conf->conflicting_block_hash, conf->conflicting_block_height, m_state);
    }

    // If the above downgraded a previously-confirmed witness variant back to unconfirmed,
    // the canonical choice is no longer pinned by confirmation. Re-apply the least-weight rule.
    if (!isConfirmed()) RecomputeCanonical();
}

bool CWalletTx::Update(CTransactionRef new_tx, const TxState& new_state)
{
    Assert(new_tx);
    if (!Assume(GetHash() == new_tx->GetHash())) {
        return false;
    }
    bool ret = false;
    const auto& [tx_pair, inserted] = m_txs.emplace(new_tx->GetWitnessHash(), std::move(new_tx));
    if (inserted) {
        ret = true;
    }
    const auto& [wtxid, tx] = *tx_pair;

    if (new_state.index() != m_state.index()) {
        m_state = new_state;
        if (state<TxStateConfirmed>()) {
            m_canonical_wtxid = wtxid;
        }
        ret = true;
    } else {
        assert(TxStateSerializedIndex(m_state) == TxStateSerializedIndex(new_state));
        assert(TxStateSerializedBlockHash(m_state) == TxStateSerializedBlockHash(new_state));
    }

    // While unconfirmed, derive the canonical variant from all known variants
    if (!isConfirmed()) {
        const Wtxid prev_canonical = m_canonical_wtxid;
        RecomputeCanonical();
        if (m_canonical_wtxid != prev_canonical) {
            ret = true;
        }
    }

    return ret;
}

void CWalletTx::RecomputeCanonical()
{
    // Recompute the canonical variant among the witness variants. They share
    // the txid but differ in the wtxid. Prefer variant with witness data and
    // the least weight.
    Assert(!m_txs.empty());

    // Returns true if 'a' should be preferred over 'b'
    auto is_better = [](const CTransactionRef& a, const CTransactionRef& b) {
        // A witnessed variant always beats a witnessless one
        if (a->HasWitness() != b->HasWitness()) return a->HasWitness();
        // Otherwise the lighter one wins
        return GetTransactionWeight(*a) < GetTransactionWeight(*b);
    };

    auto it = m_txs.begin();
    auto best_wtxid = it->first;
    const CTransactionRef* best = &it->second;
    it = std::next(it);
    for (; it != m_txs.end(); it = std::next(it)) {
        if (is_better(it->second, *best)) {
            best = &it->second;
            best_wtxid = it->first;
        }
    }
    m_canonical_wtxid = best_wtxid;
}
} // namespace wallet
