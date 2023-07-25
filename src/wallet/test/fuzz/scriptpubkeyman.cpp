// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/setup_common.h>
#include <wallet/context.h>
#include <wallet/scriptpubkeyman.h>
#include <wallet/wallet.h>
#include <wallet/test/util.h>
#include <validation.h>

namespace wallet {
namespace {
const TestingSetup* g_setup;

void initialize_spkm()
{
    static const auto testing_setup = MakeNoLogFileContext<const TestingSetup>();
    g_setup = testing_setup.get();
}

FUZZ_TARGET(scriptpubkeyman, .init = initialize_spkm)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    const auto& node{g_setup->m_node};
    Chainstate* chainstate = &node.chainman->ActiveChainstate();
    std::unique_ptr<CWallet> wallet{std::make_unique<CWallet>(node.chain.get(), "", CreateMockableWalletDatabase())};
    {
        LOCK(wallet->cs_wallet);
        wallet->SetLastBlockProcessed(chainstate->m_chain.Height(), chainstate->m_chain.Tip()->GetBlockHash());
    }

    LegacyScriptPubKeyMan& legacy_spkm{*wallet->GetOrCreateLegacyScriptPubKeyMan()};
    SignatureData data;
    CScript watch_script;

    const auto num_keys{fuzzed_data_provider.ConsumeIntegralInRange<size_t>(1, 5)};
    std::vector<CKey> keys;
    keys.resize(num_keys);
    for (auto& key : keys) {
        const std::vector<uint8_t> random_bytes{ConsumeRandomLengthByteVector(fuzzed_data_provider)};
        key.Set(random_bytes.begin(), random_bytes.end(), fuzzed_data_provider.ConsumeBool());
        if (!key.IsValid()) return;
    }

    CKey& key{PickValue(fuzzed_data_provider, keys)};
    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 10000) {
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                (void)legacy_spkm.AddKey(key);
            },
            [&] {
                CScript script{ConsumeScript(fuzzed_data_provider)};
                (void)legacy_spkm.AddCScript(script);
            },
            [&] {
                CScript script{ConsumeScript(fuzzed_data_provider)};
                (void)legacy_spkm.IsMine(script);
            },
            [&] {
                CScript script{ConsumeScript(fuzzed_data_provider)};
                (void)legacy_spkm.CanProvide(script, data);
            },
            [&] {
                key = PickValue(fuzzed_data_provider, keys);
            },
            [&] {
                if (fuzzed_data_provider.ConsumeBool()) {
                    CPubKey pubkey{key.GetPubKey()};
                    watch_script = GetScriptForRawPubKey(pubkey);
                } else {
                    watch_script = ConsumeScript(fuzzed_data_provider);
                }
            },
            [&] {
                CPubKey pubkey{key.GetPubKey()};
                CKeyID key_id{pubkey.GetID()};
                CPubKey found_pubkey;
                (void)legacy_spkm.GetWatchPubKey(key_id, found_pubkey);
            },
            [&] {
                (void)legacy_spkm.LoadWatchOnly(watch_script);
            },
            [&] {
                (void)legacy_spkm.RemoveWatchOnly(watch_script);
            },
            [&] {
                (void)legacy_spkm.HaveWatchOnly(watch_script);
            }
        );
    }
}

} // namespace
} // namespace wallet
