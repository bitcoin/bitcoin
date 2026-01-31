// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <addresstype.h>
#include <primitives/transaction.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/fuzz/util/wallet.h>
#include <test/util/random.h>
#include <test/util/setup_common.h>
#include <util/time.h>
#include <validation.h>
#include <wallet/receive.h>
#include <wallet/test/util.h>
#include <wallet/transaction.h>
#include <wallet/wallet.h>

namespace wallet {
namespace {
const TestingSetup* g_setup;

void initialize_setup()
{
    static const auto testing_setup = MakeNoLogFileContext<const TestingSetup>();
    g_setup = testing_setup.get();
}

FUZZ_TARGET(wallet_receive, .init = initialize_setup)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    SetMockTime(ConsumeTime(fuzzed_data_provider));
    const auto& node = g_setup->m_node;
    Chainstate& chainstate{node.chainman->ActiveChainstate()};
    FuzzedWallet fuzzed_wallet{
        *g_setup->m_node.chain,
        "fuzzed_wallet_a",
        "tprv8ZgxMBicQKsPd1QwsGgzfu2pcPYbBosZhJknqreRHgsWx32nNEhMjGQX2cgFL8n6wz9xdDYwLcs78N4nsCo32cxEX8RBtwGsEGgybLiQJfk",
    };

    std::vector<CWalletTx*> wallet_txs;

    int next_locktime{0};
    CAmount all_values{0};
    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 10000)
    {
        CMutableTransaction tx;
        tx.nLockTime = next_locktime++;

        const size_t num_outputs = fuzzed_data_provider.ConsumeIntegralInRange<size_t>(1, 5);
        tx.vout.resize(num_outputs);

        for (size_t i = 0; i < num_outputs; ++i) {
            CAmount n_value{ConsumeMoney(fuzzed_data_provider)};
            all_values += n_value;
            if (all_values > MAX_MONEY) return;
            tx.vout[i].nValue = n_value;

            if (fuzzed_data_provider.ConsumeIntegralInRange(0, 19) < 17) {
                tx.vout[i].scriptPubKey = GetScriptForDestination(fuzzed_wallet.GetDestination(fuzzed_data_provider));
            } else {
                tx.vout[i].scriptPubKey = ConsumeScript(fuzzed_data_provider);
            }
        }

        const size_t num_inputs = fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, 3);
        std::vector<std::vector<uint32_t>> wallet_owned_indices;
        if (!wallet_txs.empty() && num_inputs > 0) {
            LOCK(fuzzed_wallet.wallet->cs_wallet);
            wallet_owned_indices.reserve(wallet_txs.size());
            for (const auto* prev_wtx : wallet_txs) {
                std::vector<uint32_t> owned;
                if (!prev_wtx->tx->vout.empty()) {
                    for (uint32_t j = 0; j < prev_wtx->tx->vout.size(); ++j) {
                        if (fuzzed_wallet.wallet->IsMine(prev_wtx->tx->vout[j])) {
                            owned.push_back(j);
                        }
                    }
                }
                wallet_owned_indices.push_back(std::move(owned));
            }
        }
        for (size_t i = 0; i < num_inputs; ++i) {
            CTxIn txin;
            if (!wallet_txs.empty() && fuzzed_data_provider.ConsumeIntegralInRange(0, 9) < 9) {
                const size_t prev_idx = fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, wallet_txs.size() - 1);
                auto* prev_wtx = wallet_txs[prev_idx];
                if (!prev_wtx->tx->vout.empty()) {
                    const auto& owned = wallet_owned_indices[prev_idx];
                    if (!owned.empty() && fuzzed_data_provider.ConsumeIntegralInRange(0, 19) < 17) {
                        uint32_t prevout_n = owned[fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, owned.size() - 1)];
                        txin.prevout = COutPoint(prev_wtx->GetHash(), prevout_n);
                    } else {
                        uint32_t prevout_n = fuzzed_data_provider.ConsumeIntegralInRange<uint32_t>(0, prev_wtx->tx->vout.size() - 1);
                        txin.prevout = COutPoint(prev_wtx->GetHash(), prevout_n);
                    }
                } else {
                    if (!wallet_txs.empty() && !wallet_txs[0]->tx->vout.empty()) {
                        txin.prevout = COutPoint(wallet_txs[0]->GetHash(), 0);
                    } else {
                        txin.prevout = COutPoint(Txid::FromUint256(ConsumeUInt256(fuzzed_data_provider)), fuzzed_data_provider.ConsumeIntegral<uint32_t>());
                    }
                }
            } else {
                txin.prevout = COutPoint(Txid::FromUint256(ConsumeUInt256(fuzzed_data_provider)), fuzzed_data_provider.ConsumeIntegral<uint32_t>());
            }
            tx.vin.push_back(txin);
        }

        TxState state{TxStateInactive{}};
        const int state_choice = fuzzed_data_provider.ConsumeIntegralInRange(0, 9);
        const bool has_inputs = !tx.vin.empty();
        if (state_choice < 5) {
            state = TxState{TxStateConfirmed{
                chainstate.m_chain.Tip()->GetBlockHash(),
                chainstate.m_chain.Height(),
                fuzzed_data_provider.ConsumeIntegral<int>()}};
        } else if (state_choice < (has_inputs ? 8 : 7)) {
            state = TxState{TxStateInMempool{}};
        } else if (state_choice < 9) {
            state = TxState{TxStateInactive{fuzzed_data_provider.ConsumeBool()}};
        } else {
            int block_height = fuzzed_data_provider.ConsumeBool() ? -1 : fuzzed_data_provider.ConsumeIntegralInRange<int>(0, chainstate.m_chain.Height() + 1000);
            state = TxState{TxStateBlockConflicted{
                ConsumeUInt256(fuzzed_data_provider),
                block_height}};
        }

        LOCK(fuzzed_wallet.wallet->cs_wallet);
        auto txid{tx.GetHash()};
        auto ret{fuzzed_wallet.wallet->mapWallet.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(txid),
            std::forward_as_tuple(MakeTransactionRef(std::move(tx)), state))};
        assert(ret.second);
        CWalletTx* wtx = &ret.first->second;
        wallet_txs.push_back(wtx);
        fuzzed_wallet.wallet->RefreshTXOsFromTx(*wtx);
    }

    if (wallet_txs.empty()) return;

    LOCK(fuzzed_wallet.wallet->cs_wallet);

    for (auto* wtx : wallet_txs) {
        for (const auto& txin : wtx->tx->vin) {
            [[maybe_unused]] bool is_mine = InputIsMine(*fuzzed_wallet.wallet, txin);
        }
    }

    for (auto* wtx : wallet_txs) {
        [[maybe_unused]] bool all_mine = AllInputsMine(*fuzzed_wallet.wallet, *wtx->tx);
    }

    for (auto* wtx : wallet_txs) {
        for (const auto& txout : wtx->tx->vout) {
            [[maybe_unused]] CAmount credit = OutputGetCredit(*fuzzed_wallet.wallet, txout);
        }
    }

    if (!wallet_txs.empty() && fuzzed_data_provider.ConsumeBool()) {
        CTxOut invalid_out;
        invalid_out.nValue = MAX_MONEY + 1;
        invalid_out.scriptPubKey = ConsumeScript(fuzzed_data_provider);
        try {
            [[maybe_unused]] CAmount credit = OutputGetCredit(*fuzzed_wallet.wallet, invalid_out);
        } catch (const std::runtime_error&) {
        }
    }

    for (auto* wtx : wallet_txs) {
        try {
            [[maybe_unused]] CAmount credit = TxGetCredit(*fuzzed_wallet.wallet, *wtx->tx);
        } catch (const std::runtime_error&) {
        }
    }

    for (auto* wtx : wallet_txs) {
        for (const auto& txout : wtx->tx->vout) {
            [[maybe_unused]] bool is_change = ScriptIsChange(*fuzzed_wallet.wallet, txout.scriptPubKey);
        }
    }

    if (fuzzed_data_provider.ConsumeBool()) {
        CScript non_standard_script;
        non_standard_script << OP_RETURN << std::vector<unsigned char>(10);
        [[maybe_unused]] bool is_change = ScriptIsChange(*fuzzed_wallet.wallet, non_standard_script);
    }

    for (auto* wtx : wallet_txs) {
        for (const auto& txout : wtx->tx->vout) {
            [[maybe_unused]] bool is_change = OutputIsChange(*fuzzed_wallet.wallet, txout);
        }
    }

    for (auto* wtx : wallet_txs) {
        for (const auto& txout : wtx->tx->vout) {
            [[maybe_unused]] CAmount change = OutputGetChange(*fuzzed_wallet.wallet, txout);
        }
    }

    for (auto* wtx : wallet_txs) {
        [[maybe_unused]] CAmount change = TxGetChange(*fuzzed_wallet.wallet, *wtx->tx);
    }

    const bool avoid_reuse = fuzzed_data_provider.ConsumeBool();
    for (auto* wtx : wallet_txs) {
        [[maybe_unused]] CAmount credit = CachedTxGetCredit(*fuzzed_wallet.wallet, *wtx, avoid_reuse);
    }

    if (!wallet_txs.empty() && fuzzed_data_provider.ConsumeBool()) {
        CMutableTransaction coinbase_tx;
        coinbase_tx.vin.resize(1);
        coinbase_tx.vin[0].prevout.SetNull();
        coinbase_tx.vin[0].scriptSig = CScript() << 1 << std::vector<unsigned char>(10);
        coinbase_tx.vout.resize(1);
        coinbase_tx.vout[0].nValue = ConsumeMoney(fuzzed_data_provider);
        coinbase_tx.vout[0].scriptPubKey = GetScriptForDestination(fuzzed_wallet.GetDestination(fuzzed_data_provider));
        auto coinbase_txid{coinbase_tx.GetHash()};
        auto coinbase_ret{fuzzed_wallet.wallet->mapWallet.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(coinbase_txid),
            std::forward_as_tuple(MakeTransactionRef(std::move(coinbase_tx)), TxStateConfirmed{chainstate.m_chain.Tip()->GetBlockHash(), chainstate.m_chain.Height(), /*index=*/0}))};
        if (coinbase_ret.second) {
            fuzzed_wallet.wallet->RefreshTXOsFromTx(coinbase_ret.first->second);
            [[maybe_unused]] CAmount credit = CachedTxGetCredit(*fuzzed_wallet.wallet, coinbase_ret.first->second, avoid_reuse);
        }
    }

    for (auto* wtx : wallet_txs) {
        try {
            [[maybe_unused]] CAmount debit = CachedTxGetDebit(*fuzzed_wallet.wallet, *wtx, avoid_reuse);
        } catch (const std::runtime_error&) {
        }
    }

    for (auto* wtx : wallet_txs) {
        [[maybe_unused]] CAmount change = CachedTxGetChange(*fuzzed_wallet.wallet, *wtx);
    }

    for (auto* wtx : wallet_txs) {
        std::list<COutputEntry> listReceived;
        std::list<COutputEntry> listSent;
        CAmount nFee;
        const bool include_change = fuzzed_data_provider.ConsumeBool();
        try {
            CachedTxGetAmounts(*fuzzed_wallet.wallet, *wtx, listReceived, listSent, nFee, include_change);
        } catch (const std::runtime_error&) {
        }
    }

    for (auto* wtx : wallet_txs) {
        [[maybe_unused]] bool from_me = CachedTxIsFromMe(*fuzzed_wallet.wallet, *wtx);
    }

    for (auto* wtx : wallet_txs) {
        [[maybe_unused]] bool trusted = CachedTxIsTrusted(*fuzzed_wallet.wallet, *wtx);

        std::set<Txid> trusted_parents;
        [[maybe_unused]] bool trusted_with_parents = CachedTxIsTrusted(*fuzzed_wallet.wallet, *wtx, trusted_parents);
    }

    const int min_depth = fuzzed_data_provider.ConsumeIntegralInRange<int>(0, 100);
    [[maybe_unused]] Balance balance = GetBalance(*fuzzed_wallet.wallet, min_depth, avoid_reuse);

    [[maybe_unused]] std::map<CTxDestination, CAmount> address_balances = GetAddressBalances(*fuzzed_wallet.wallet);

    [[maybe_unused]] std::set<std::set<CTxDestination>> groupings = GetAddressGroupings(*fuzzed_wallet.wallet);
}
} // namespace
} // namespace wallet
