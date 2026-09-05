// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_FUZZ_UTIL_WALLET_H
#define BITCOIN_TEST_FUZZ_UTIL_WALLET_H

#include <outputtype.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <policy/policy.h>
#include <wallet/coincontrol.h>
#include <wallet/fees.h>
#include <wallet/scriptpubkeyman.h>
#include <wallet/spend.h>
#include <wallet/test/util.h>
#include <wallet/wallet.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace wallet {

/**
 * Wraps a descriptor wallet for fuzzing.
 */
struct FuzzedWallet {
    static constexpr size_t DESTINATION_POOL_SIZE{16};
    static constexpr uint64_t REBUILD_INTERVAL{1000};

    std::shared_ptr<CWallet> wallet;

    FuzzedWallet(interfaces::Chain& chain, const std::string& name, const std::string& seed_insecure)
        : m_chain{chain}, m_name{name}, m_seed_insecure{seed_insecure}
    {
        Build();
    }

    void Reset()
    {
        if (++m_iterations >= REBUILD_INTERVAL) {
            Build();
            return;
        }
        m_cursors.fill(0);
        m_change_cursors.fill(0);
        {
            LOCK(wallet->cs_wallet);
            wallet->ClearInMemoryTxStateForTest();
            assert(wallet->mapWallet.empty());
            assert(wallet->GetTXOs().empty());
        }
        // A successful CreateTransaction() keeps one change index, so each
        // iteration may extend at most one descriptor's range by one. Growing
        // faster than that means an iteration is leaving behind state this
        // class does not know about.
        const int64_t drift{RangeTotal() - m_baseline_range_total};
        assert(drift >= 0);
        assert(static_cast<uint64_t>(drift) <= m_iterations * m_spkms.size());
    }

    CTxDestination GetDestination(FuzzedDataProvider& fuzzed_data_provider)
    {
        const auto type{fuzzed_data_provider.PickValueInArray(OUTPUT_TYPES)};
        const bool external{fuzzed_data_provider.ConsumeBool()};
        const size_t i{TypeIndex(type)};
        auto& pool{external ? m_dests : m_change_dests};
        auto& cursor{external ? m_cursors.at(i) : m_change_cursors.at(i)};
        const CTxDestination& dest{pool.at(i).at(cursor % DESTINATION_POOL_SIZE)};
        ++cursor;
        return dest;
    }

    CScript GetScriptPubKey(FuzzedDataProvider& fuzzed_data_provider) { return GetScriptForDestination(GetDestination(fuzzed_data_provider)); }

private:
    using DestinationPool = std::array<std::vector<CTxDestination>, OUTPUT_TYPES.size()>;

    interfaces::Chain& m_chain;
    const std::string m_name;
    const std::string m_seed_insecure;

    using PoolCursors = std::array<size_t, OUTPUT_TYPES.size()>;

    DestinationPool m_dests;
    DestinationPool m_change_dests;
    PoolCursors m_cursors{};
    PoolCursors m_change_cursors{};

    static size_t TypeIndex(OutputType type)
    {
        for (size_t i = 0; i < OUTPUT_TYPES.size(); ++i) {
            if (OUTPUT_TYPES.at(i) == type) return i;
        }
        assert(false);
    }

    std::vector<DescriptorScriptPubKeyMan*> m_spkms;
    int64_t m_baseline_range_total{0};
    uint64_t m_iterations{0};

    void Build()
    {
        m_spkms.clear();
        for (auto& pool : m_dests) pool.clear();
        for (auto& pool : m_change_dests) pool.clear();
        m_cursors.fill(0);
        m_change_cursors.fill(0);

        wallet = std::make_shared<CWallet>(&m_chain, m_name, CreateMockableWalletDatabase());
        {
            LOCK(wallet->cs_wallet);
            wallet->SetWalletFlag(WALLET_FLAG_DESCRIPTORS);
            auto height{*Assert(m_chain.getHeight())};
            wallet->SetLastBlockProcessed(height, m_chain.getBlockHash(height));
        }
        wallet->m_keypool_size = 1; // Avoid timeout in TopUp()
        assert(wallet->IsWalletFlagSet(WALLET_FLAG_DESCRIPTORS));
        ImportDescriptors(m_seed_insecure);
        PrecomputeDestinations();

        m_baseline_range_total = RangeTotal();
        m_iterations = 0;
    }

    void ImportDescriptors(const std::string& seed_insecure)
    {
        const std::vector<std::string> DESCS{
            "pkh(%s/%s/*)",
            "sh(wpkh(%s/%s/*))",
            "tr(%s/%s/*)",
            "wpkh(%s/%s/*)",
        };

        for (const std::string& desc_fmt : DESCS) {
            for (bool internal : {true, false}) {
                const auto descriptor{strprintf(tfm::RuntimeFormat{desc_fmt}, "[5aa9973a/66h/4h/2h]" + seed_insecure, int{internal})};

                FlatSigningProvider keys;
                std::string error;
                auto parsed_desc = std::move(Parse(descriptor, keys, error, /*require_checksum=*/false).at(0));
                assert(parsed_desc);
                assert(error.empty());
                assert(parsed_desc->IsRange());
                assert(parsed_desc->IsSingleType());
                assert(!keys.keys.empty());
                WalletDescriptor w_desc{std::move(parsed_desc), /*creation_time=*/0, /*range_start=*/0, /*range_end=*/1, /*next_index=*/0};
                assert(!wallet->GetDescriptorScriptPubKeyMan(w_desc));
                LOCK(wallet->cs_wallet);
                auto& spk_manager = Assert(wallet->AddWalletDescriptor(w_desc, keys, /*label=*/"", internal))->get();
                wallet->AddActiveScriptPubKeyMan(spk_manager.GetID(), *Assert(w_desc.descriptor->GetOutputType()), internal);
                m_spkms.push_back(&spk_manager);
            }
        }
    }

    void PrecomputeDestinations()
    {
        for (size_t i = 0; i < OUTPUT_TYPES.size(); ++i) {
            const OutputType type{OUTPUT_TYPES.at(i)};
            for (size_t j = 0; j < DESTINATION_POOL_SIZE; ++j) {
                m_dests.at(i).push_back(*Assert(wallet->GetNewDestination(type, "")));
                m_change_dests.at(i).push_back(*Assert(wallet->GetNewChangeDestination(type)));
            }
        }
    }

    int64_t RangeTotal() const
    {
        int64_t total{0};
        for (const DescriptorScriptPubKeyMan* spkm : m_spkms) total += spkm->GetEndRange();
        return total;
    }
};
} // namespace wallet

#endif // BITCOIN_TEST_FUZZ_UTIL_WALLET_H
