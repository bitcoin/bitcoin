// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <common/bloom.h>
#include <consensus/amount.h>
#include <node/txrelay.h>
#include <primitives/transaction_identifier.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/random.h>
#include <uint256.h>

#include <cassert>
#include <chrono>
#include <cstdint>
#include <utility>
#include <vector>

FUZZ_TARGET(txrelay)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider provider{buffer.data(), buffer.size()};
    node::TxRelay tx_relay;

    bool relay_txs{false};
    bool bloom_filter_loaded{false};
    CAmount fee_filter_received{0};

    while (provider.remaining_bytes() > 0) {
        CallOneOf(
            provider,
            [&] {
                relay_txs = provider.ConsumeBool();
                tx_relay.SetRelayTxs(relay_txs);
            },
            [&] {
                tx_relay.SetBloomFilter(CBloomFilter{
                    provider.ConsumeIntegralInRange<unsigned int>(1, 1000),
                    provider.ConsumeFloatingPointInRange<double>(0.000001, 0.1),
                    provider.ConsumeIntegral<unsigned int>(),
                    provider.ConsumeIntegralInRange<unsigned char>(BLOOM_UPDATE_NONE, BLOOM_UPDATE_MASK)});
                relay_txs = true;
                bloom_filter_loaded = true;
            },
            [&] {
                tx_relay.ClearBloomFilter();
                relay_txs = true;
                bloom_filter_loaded = false;
            },
            [&] {
                const auto data{ConsumeRandomLengthByteVector(provider, /*max_length=*/32)};
                assert(tx_relay.AddToBloomFilter(data) == bloom_filter_loaded);
            },
            [&] {
                tx_relay.AddKnownTx(ConsumeUInt256(provider));
            },
            [&] {
                tx_relay.SetSendMempool();
                assert(tx_relay.StartTxInventoryBatch(/*send_trickle=*/true).SendMempool());
            },
            [&] {
                fee_filter_received = provider.ConsumeIntegral<CAmount>();
                tx_relay.SetFeeFilterReceived(fee_filter_received);
            },
            [&] {
                tx_relay.SetNextInvSendTime(std::chrono::microseconds{provider.ConsumeIntegral<int64_t>()});
            },
            [&] {
                const uint256 hash{ConsumeUInt256(provider)};
                tx_relay.PushInventory(hash, Wtxid::FromUint256(hash));
            },
            [&] {
                (void)tx_relay.GetInventoryStats();
                (void)tx_relay.IsInventoryPristine();
                (void)tx_relay.GetLastInvSequence();
                assert(tx_relay.GetRelayTxs() == relay_txs);
                assert(tx_relay.GetFeeFilterReceived() == fee_filter_received);
            },
            [&] {
                auto batch{tx_relay.StartTxInventoryBatch(provider.ConsumeBool())};
                for (const auto& wtxid : batch.QueuedCandidates()) {
                    if (provider.ConsumeBool()) batch.EraseQueued(wtxid);
                }
                tx_relay.ReturnTxInventory(std::move(batch));
            });
    }
}
