// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include <addresstype.h>
#include <consensus/tx_check.h>
#include <kernel/mempool_entry.h>
#include <primitives/transaction.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/fuzz/util/mempool.h>
#include <test/fuzz/util/wallet.h>
#include <test/util/random.h>
#include <test/util/setup_common.h>
#include <test/util/txmempool.h>
#include <txmempool.h>
#include <validation.h>
#include <wallet/context.h>
#include <wallet/feebumper.h>
#include <wallet/spend.h>
#include <wallet/test/util.h>
#include <wallet/wallet.h>

namespace wallet {
namespace {
TestingSetup* g_setup;

void initialize_setup()
{
    static const auto testing_setup = MakeNoLogFileContext<TestingSetup>();
    g_setup = testing_setup.get();
}

TxState ConsumeTxState(FuzzedDataProvider& fdp)
{
    TxState state = TxStateInactive{
        /*abandoned=*/fdp.ConsumeBool(),
    };
    CallOneOf(fdp,
        [&] {
            state = TxStateInMempool{};
        },
        [&] {
            state = TxStateConfirmed{
                /*block_hash=*/ConsumeUInt256(fdp),
                /*height=*/fdp.ConsumeIntegralInRange<int>(1, 100'000'000),
                /*index=*/fdp.ConsumeIntegralInRange<int>(1, 50),
            };
        },
        [&] {
            state = TxStateBlockConflicted{
                /*block_hash=*/ConsumeUInt256(fdp),
                /*height=*/fdp.ConsumeIntegralInRange<int>(1, 100'000'000),
            };
        }
    );
    return state;
}

struct MockedTxPool : public CTxMemPool {
    FuzzedDataProvider& m_fuzzed_data_provider;

public:
    MockedTxPool(FuzzedDataProvider& fuzzed_data_provider,
                 CTxMemPool::Options&& options,
                 bilingual_str& error)
        : CTxMemPool(std::move(options), error),
          m_fuzzed_data_provider(fuzzed_data_provider)
    {
    }

    bool HasDescendants(const Txid& txid) override
    {
        return m_fuzzed_data_provider.ConsumeBool();
    }
};


FUZZ_TARGET(wallet_tx_can_be_bumped, .init = initialize_setup)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    SetMockTime(ConsumeTime(fuzzed_data_provider));
    auto& node = g_setup->m_node;
    CTxMemPool::Options mempool_opts{MemPoolOptionsForTest(node)};
    bilingual_str error;
    node.mempool = std::make_unique<MockedTxPool>(fuzzed_data_provider, std::move(mempool_opts), error);
    MockedTxPool& tx_pool = *static_cast<MockedTxPool*>(node.mempool.get());

    FuzzedWallet fuzzed_wallet{
        *g_setup->m_node.chain,
        "fuzzed_wallet_a",
        "tprv8ZgxMBicQKsPd1QwsGgzfu2pcPYbBosZhJknqreRHgsWx32nNEhMjGQX2cgFL8n6wz9xdDYwLcs78N4nsCo32cxEX8RBtwGsEGgybLiQJfk",
    };

    if (fuzzed_data_provider.ConsumeBool()) {
        fuzzed_wallet.wallet->SetWalletFlag(WALLET_FLAG_AVOID_REUSE);
    }

    auto tx_state{ConsumeTxState(fuzzed_data_provider)};

    bool good_data{true};
    std::optional<Txid> txid_to_spend;
    LIMITED_WHILE(good_data && fuzzed_data_provider.ConsumeBool(), 3)
    {
        CMutableTransaction tx;
        const auto mut_tx{ConsumeDeserializable<CMutableTransaction>(fuzzed_data_provider, TX_WITH_WITNESS)};
        if (!mut_tx.has_value()) {
            good_data = false;
            return;
        }
        tx = *mut_tx;
        if (fuzzed_data_provider.ConsumeBool()) {
            tx.vout.resize(1);
            CAmount n_value{ConsumeMoney(fuzzed_data_provider, /*max=*/MAX_MONEY)};
            tx.vout[0].nValue = n_value;
            tx.vout[0].scriptPubKey = GetScriptForDestination(fuzzed_wallet.GetDestination(fuzzed_data_provider));
            txid_to_spend = tx.GetHash();
        }
        (void)fuzzed_wallet.wallet->AddToWallet(MakeTransactionRef(tx), tx_state);
    }

    CMutableTransaction tx;
    if (fuzzed_data_provider.ConsumeBool()) {
        if (txid_to_spend.has_value()) {
            tx.vin.resize(1);
            tx.vin[0].prevout.hash = txid_to_spend.value();
            tx.vin[0].prevout.n = 0;
        }
        tx.nLockTime = fuzzed_data_provider.ConsumeIntegral<uint32_t>();
        tx.vout.resize(1);
        CAmount n_value{ConsumeMoney(fuzzed_data_provider, /*max=*/MAX_MONEY)};
        tx.vout[0].nValue = n_value;
        tx.vout[0].scriptPubKey = GetScriptForDestination(fuzzed_wallet.GetDestination(fuzzed_data_provider));
    } else {
        const auto mut_tx{ConsumeDeserializable<CMutableTransaction>(fuzzed_data_provider, TX_WITH_WITNESS)};
        if (mut_tx.has_value()) {
            tx = *mut_tx;
        }
    }

    CTransaction tx_to_verify{tx};
    TxValidationState state;
    if (!CheckTransaction(tx_to_verify, state)) return;

    (void)fuzzed_wallet.wallet->AddToWallet(MakeTransactionRef(tx), tx_state);
    if (fuzzed_data_provider.ConsumeBool()) {
        (void)fuzzed_wallet.wallet->AddToWallet(MakeTransactionRef(tx), tx_state);
    }

    const CTransaction final_tx{tx};
    auto mempool_entry{ConsumeTxMemPoolEntry(fuzzed_data_provider, final_tx)};

    AddToMempool(tx_pool, mempool_entry);

    auto txid{final_tx.GetHash()};
    if (fuzzed_data_provider.ConsumeBool()) {
        (void)fuzzed_wallet.wallet->MarkReplaced(txid, /*newHash=*/Txid::FromUint256(ConsumeUInt256(fuzzed_data_provider)));
    }

    if (fuzzed_data_provider.ConsumeBool()) {
        const auto mut_tx{ConsumeDeserializable<CMutableTransaction>(fuzzed_data_provider, TX_WITH_WITNESS)};
        if (mut_tx.has_value()) {
            CMutableTransaction new_tx{*mut_tx};
            if (!new_tx.vin.empty()) {
                new_tx.vin[0].prevout = COutPoint(tx.GetHash(), 0);
            }
            CTransaction new_tx_to_verify{new_tx};
            if (CheckTransaction(new_tx_to_verify, state)) {
                (void)fuzzed_wallet.wallet->AddToWallet(MakeTransactionRef(new_tx), tx_state);
            }
        }
    }

    (void)feebumper::TransactionCanBeBumped(*fuzzed_wallet.wallet, txid);
}
} // namespace
} // namespace wallet
