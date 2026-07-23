// Copyright (c) 2021-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/transaction.h>

#include <interfaces/chain.h>

using interfaces::FoundBlock;

namespace wallet {
bool CWalletTx::IsEquivalentTo(const CWalletTx& _tx) const
{
        CMutableTransaction tx1 {*this->tx};
        CMutableTransaction tx2 {*_tx.tx};
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
    auto lookup_block = [&](const uint256& hash, int& height) {
        // If tx block (or conflicting block) was reorged out of chain
        // while the wallet was shutdown, change tx status to UNCONFIRMED
        // and reset block height, hash, and index. ABANDONED tx don't have
        // associated blocks and don't need to be updated. The case where a
        // transaction was reorged out while online and then reconfirmed
        // while offline is covered by the rescan logic.
        if (!chain.findBlock(hash, FoundBlock().inActiveChain(active).height(height)) || !active) {
            // Note that it is safe to provide nullptr for update_external_states_fn here as this
            // function is only called during loading prior to the TXOs being cached.
            SetState(TxStateInactive{}, nullptr);
        }
    };
    if (auto* conf = state<TxStateConfirmed>()) {
        lookup_block(conf->confirmed_block_hash, conf->confirmed_block_height);
    } else if (auto* conf = state<TxStateBlockConflicted>()) {
        lookup_block(conf->conflicting_block_hash, conf->conflicting_block_height);
    }
}

void CWalletTx::CopyFrom(const CWalletTx& _tx)
{
    *this = _tx;
}

void CWalletTx::SetState(const TxState& state, std::function<void(const COutPoint&, const TxState&)> update_external_states_fn)
{
    m_state = state;
    if (update_external_states_fn) {
        for (uint32_t i = 0; i < tx->vout.size(); ++i) {
            update_external_states_fn(COutPoint{GetHash(), i}, state);
        }
    }
}
} // namespace wallet
