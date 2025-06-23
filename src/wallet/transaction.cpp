// Copyright (c) 2021 The Bitcoin Core developers
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
}

void CWalletTx::CopyFrom(const CWalletTx& _tx)
{
    *this = _tx;
}

int32_t GetTxStateType(const TxState& state)
{
    return std::visit(util::Overloaded{
        [](const TxStateConfirmed& confirmed) { return 0; },
        // Inactive and InMempool use the same type as on reloading we cannot know whether an unconfirmed tx is still in the mempool.
        [](const TxStateInactive& inactive) { return 1; },
        [](const TxStateInMempool& in_mempool) { return 1;},
        [](const TxStateBlockConflicted& conflicted) { return 2; },
        [](const TxStateUnrecognized& unrecognized) { return unrecognized.index; }
    }, state);
}

class TxStateDataVisitor
{
public:
    // InMempool needs to be written as InActive(abandoned=false)
    std::vector<unsigned char> operator()(const TxStateInMempool& state)
    {
        return operator()(TxStateInactive{/*abandoned=*/false});
    }

    template<typename State>
    std::vector<unsigned char> operator()(const State& state)
    {
        std::vector<unsigned char> data;
        VectorWriter stream(data, 0);
        stream << state;
        return data;
    }
};

std::vector<unsigned char> GetTxStateData(const TxState& state)
{
    return std::visit(TxStateDataVisitor(), state);
}

TxState ConstructTxState(int32_t type, std::vector<unsigned char> data)
{
    SpanReader stream(data);
    switch (type) {
        case 0: {
            TxStateConfirmed confirmed;
            stream >> confirmed;
            return confirmed;
        }
        case 1:{
            TxStateInactive inactive;
            stream >> inactive;
            return inactive;
        }
        case 2: {
            TxStateBlockConflicted conflicted;
            stream >> conflicted;
            return conflicted;
        }
        default: {
            TxStateUnrecognized unrecognized(type);
            stream >> unrecognized;
            return unrecognized;
        }
    }
}
} // namespace wallet
