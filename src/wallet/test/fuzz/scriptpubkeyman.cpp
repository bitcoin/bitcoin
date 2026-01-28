// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <addresstype.h>
#include <chainparams.h>
#include <coins.h>
#include <key.h>
#include <primitives/transaction.h>
#include <psbt.h>
#include <script/descriptor.h>
#include <script/interpreter.h>
#include <script/script.h>
#include <script/signingprovider.h>
#include <sync.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/fuzz/util/descriptor.h>
#include <test/util/setup_common.h>
#include <util/check.h>
#include <util/time.h>
#include <util/translation.h>
#include <util/string.h>
#include <validation.h>
#include <wallet/context.h>
#include <wallet/scriptpubkeyman.h>
#include <wallet/test/util.h>
#include <wallet/types.h>
#include <wallet/wallet.h>
#include <wallet/walletutil.h>

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>

namespace wallet {
namespace {
const TestingSetup* g_setup;

//! The converter of mocked descriptors, needs to be initialized when the target is.
MockedDescriptorConverter MOCKED_DESC_CONVERTER;

void initialize_spkm()
{
    static const auto testing_setup{MakeNoLogFileContext<const TestingSetup>()};
    g_setup = testing_setup.get();
    MOCKED_DESC_CONVERTER.Init();
}

void initialize_spkm_migration()
{
    static const auto testing_setup{MakeNoLogFileContext<const TestingSetup>()};
    g_setup = testing_setup.get();
}

static std::optional<std::pair<WalletDescriptor, FlatSigningProvider>> CreateWalletDescriptor(FuzzedDataProvider& fuzzed_data_provider)
{
    const std::string mocked_descriptor{fuzzed_data_provider.ConsumeRandomLengthString()};
    const auto desc_str{MOCKED_DESC_CONVERTER.GetDescriptor(mocked_descriptor)};
    if (!desc_str.has_value()) return std::nullopt;
    if (IsTooExpensive(MakeUCharSpan(*desc_str))) return {};

    FlatSigningProvider keys;
    std::string error;
    std::vector<std::unique_ptr<Descriptor>> parsed_descs = Parse(desc_str.value(), keys, error, false);
    if (parsed_descs.empty()) return std::nullopt;

    WalletDescriptor w_desc{std::move(parsed_descs.at(0)), /*creation_time=*/0, /*range_start=*/0, /*range_end=*/1, /*next_index=*/1};
    return std::make_pair(w_desc, keys);
}

static DescriptorScriptPubKeyMan* CreateDescriptor(WalletDescriptor& wallet_desc, FlatSigningProvider& keys, CWallet& keystore)
{
    LOCK(keystore.cs_wallet);
    auto spk_manager_res = keystore.AddWalletDescriptor(wallet_desc, keys, /*label=*/"", /*internal=*/false);
    if (!spk_manager_res) return nullptr;
    return &spk_manager_res.value().get();
};

FUZZ_TARGET(scriptpubkeyman, .init = initialize_spkm)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    SetMockTime(ConsumeTime(fuzzed_data_provider));
    const auto& node{g_setup->m_node};
    Chainstate& chainstate{node.chainman->ActiveChainstate()};
    std::unique_ptr<CWallet> wallet_ptr{std::make_unique<CWallet>(node.chain.get(), "", CreateMockableWalletDatabase())};
    CWallet& wallet{*wallet_ptr};
    {
        LOCK(wallet.cs_wallet);
        wallet.SetWalletFlag(WALLET_FLAG_DESCRIPTORS);
        wallet.SetLastBlockProcessed(chainstate.m_chain.Height(), chainstate.m_chain.Tip()->GetBlockHash());
        wallet.m_keypool_size = 1;
    }

    auto wallet_desc{CreateWalletDescriptor(fuzzed_data_provider)};
    if (!wallet_desc.has_value()) return;
    auto spk_manager{CreateDescriptor(wallet_desc->first, wallet_desc->second, wallet)};
    if (spk_manager == nullptr) return;

    if (fuzzed_data_provider.ConsumeBool()) {
        auto wallet_desc{CreateWalletDescriptor(fuzzed_data_provider)};
        if (!wallet_desc.has_value()) {
            return;
        }
        std::string error;
        if (spk_manager->CanUpdateToWalletDescriptor(wallet_desc->first, error)) {
            auto new_spk_manager{CreateDescriptor(wallet_desc->first, wallet_desc->second, wallet)};
            if (new_spk_manager != nullptr) spk_manager = new_spk_manager;
        }
    }

    bool good_data{true};
    LIMITED_WHILE(good_data && fuzzed_data_provider.ConsumeBool(), 20) {
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                const CScript script{ConsumeScript(fuzzed_data_provider)};
                if (spk_manager->IsMine(script)) {
                    assert(spk_manager->GetScriptPubKeys().contains(script));
                }
            },
            [&] {
                auto spks{spk_manager->GetScriptPubKeys()};
                for (const CScript& spk : spks) {
                    assert(spk_manager->IsMine(spk));
                    CTxDestination dest;
                    bool extract_dest{ExtractDestination(spk, dest)};
                    if (extract_dest) {
                        const std::string msg{fuzzed_data_provider.ConsumeRandomLengthString()};
                        PKHash pk_hash{std::get_if<PKHash>(&dest) && fuzzed_data_provider.ConsumeBool() ?
                                           *std::get_if<PKHash>(&dest) :
                                           PKHash{ConsumeUInt160(fuzzed_data_provider)}};
                        std::string str_sig;
                        (void)spk_manager->SignMessage(msg, pk_hash, str_sig);
                        (void)spk_manager->GetMetadata(dest);
                    }
                }
            },
            [&] {
                auto spks{spk_manager->GetScriptPubKeys()};
                if (!spks.empty()) {
                    auto& spk{PickValue(fuzzed_data_provider, spks)};
                    (void)spk_manager->MarkUnusedAddresses(spk);
                }
            },
            [&] {
                LOCK(spk_manager->cs_desc_man);
                auto wallet_desc{spk_manager->GetWalletDescriptor()};
                if (wallet_desc.descriptor->IsSingleType()) {
                    auto output_type{wallet_desc.descriptor->GetOutputType()};
                    if (output_type.has_value()) {
                        auto dest{spk_manager->GetNewDestination(*output_type)};
                        if (dest) {
                            assert(IsValidDestination(*dest));
                            assert(spk_manager->IsHDEnabled());
                        }
                    }
                }
            },
            [&] {
                CMutableTransaction tx_to;
                const std::optional<CMutableTransaction> opt_tx_to{ConsumeDeserializable<CMutableTransaction>(fuzzed_data_provider, TX_WITH_WITNESS)};
                if (!opt_tx_to) {
                    good_data = false;
                    return;
                }
                tx_to = *opt_tx_to;

                std::map<COutPoint, Coin> coins{ConsumeCoins(fuzzed_data_provider)};
                const int sighash{fuzzed_data_provider.ConsumeIntegral<int>()};
                std::map<int, bilingual_str> input_errors;
                (void)spk_manager->SignTransaction(tx_to, coins, sighash, input_errors);
            },
            [&] {
                std::optional<PartiallySignedTransaction> opt_psbt{ConsumeDeserializable<PartiallySignedTransaction>(fuzzed_data_provider)};
                if (!opt_psbt) {
                    good_data = false;
                    return;
                }
                auto psbt{*opt_psbt};
                const PrecomputedTransactionData txdata{PrecomputePSBTData(psbt)};
                std::optional<int> sighash_type{fuzzed_data_provider.ConsumeIntegralInRange<int>(0, 151)};
                if (sighash_type == 151) sighash_type = std::nullopt;
                auto sign  = fuzzed_data_provider.ConsumeBool();
                auto bip32derivs = fuzzed_data_provider.ConsumeBool();
                auto finalize = fuzzed_data_provider.ConsumeBool();
                (void)spk_manager->FillPSBT(psbt, txdata, sighash_type, sign, bip32derivs, nullptr, finalize);
            }
        );
    }

    std::string descriptor;
    (void)spk_manager->GetDescriptorString(descriptor, /*priv=*/fuzzed_data_provider.ConsumeBool());
    (void)spk_manager->GetEndRange();
    (void)spk_manager->GetKeyPoolSize();
}


FUZZ_TARGET(spkm_migration, .init = initialize_spkm_migration)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    SetMockTime(ConsumeTime(fuzzed_data_provider));
    const auto& node{g_setup->m_node};
    Chainstate& chainstate{node.chainman->ActiveChainstate()};

    std::unique_ptr<CWallet> wallet_ptr{std::make_unique<CWallet>(node.chain.get(), "", CreateMockableWalletDatabase())};
    CWallet& wallet{*wallet_ptr};
    wallet.m_keypool_size = 1;
    {
        LOCK(wallet.cs_wallet);
        wallet.UnsetWalletFlag(WALLET_FLAG_DESCRIPTORS);
        wallet.SetLastBlockProcessed(chainstate.m_chain.Height(), chainstate.m_chain.Tip()->GetBlockHash());
    }

    auto& legacy_data{*wallet.GetOrCreateLegacyDataSPKM()};

    std::vector<CKey> keys;
    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 30) {
        const auto key{ConsumePrivateKey(fuzzed_data_provider)};
        if (!key.IsValid()) return;
        auto pub_key{key.GetPubKey()};
        if (!pub_key.IsFullyValid()) return;
        if (legacy_data.LoadKey(key, pub_key) && std::find(keys.begin(), keys.end(), key) == keys.end()) keys.push_back(key);
    }

    bool add_hd_chain{fuzzed_data_provider.ConsumeBool() && !keys.empty()};
    CHDChain hd_chain;
    auto version{fuzzed_data_provider.ConsumeBool() ? CHDChain::VERSION_HD_CHAIN_SPLIT : CHDChain::VERSION_HD_BASE};
    CKey hd_key;
    if (add_hd_chain) {
        hd_key = PickValue(fuzzed_data_provider, keys);
        hd_chain.nVersion = version;
        hd_chain.seed_id = hd_key.GetPubKey().GetID();
        legacy_data.LoadHDChain(hd_chain);
    }

    bool add_inactive_hd_chain{fuzzed_data_provider.ConsumeBool() && !keys.empty()};
    if (add_inactive_hd_chain) {
        hd_key = PickValue(fuzzed_data_provider, keys);
        hd_chain.nVersion = fuzzed_data_provider.ConsumeBool() ? CHDChain::VERSION_HD_CHAIN_SPLIT : CHDChain::VERSION_HD_BASE;
        hd_chain.seed_id = hd_key.GetPubKey().GetID();
        legacy_data.AddInactiveHDChain(hd_chain);
    }

    bool watch_only = false;
    const auto pub_key = ConsumeDeserializable<CPubKey>(fuzzed_data_provider);
    if (!pub_key || !pub_key->IsFullyValid()) return;
    auto script_dest{GetScriptForDestination(WitnessV0KeyHash{*pub_key})};
    if (fuzzed_data_provider.ConsumeBool()) {
        script_dest = GetScriptForDestination(CTxDestination{PKHash(*pub_key)});
    }
    if (legacy_data.LoadWatchOnly(script_dest)) watch_only = true;

    size_t added_script{0};
    bool good_data{true};
    LIMITED_WHILE(good_data && fuzzed_data_provider.ConsumeBool(), 30) {
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                CKey key;
                if (!keys.empty()) {
                    key = PickValue(fuzzed_data_provider, keys);
                } else {
                    key = ConsumePrivateKey(fuzzed_data_provider, /*compressed=*/fuzzed_data_provider.ConsumeBool());
                }
                if (!key.IsValid()) return;
                auto pub_key{key.GetPubKey()};
                CScript script;
                CallOneOf(
                    fuzzed_data_provider,
                    [&] {
                        script = GetScriptForDestination(CTxDestination{PKHash(pub_key)});
                    },
                    [&] {
                        script = GetScriptForDestination(WitnessV0KeyHash(pub_key));
                    },
                    [&] {
                        std::optional<CScript> script_opt{ConsumeDeserializable<CScript>(fuzzed_data_provider)};
                        if (!script_opt) {
                            good_data = false;
                            return;
                        }
                        script = script_opt.value();
                    }
                );
                if (fuzzed_data_provider.ConsumeBool()) script = GetScriptForDestination(ScriptHash(script));
                if (!legacy_data.HaveCScript(CScriptID(script)) && legacy_data.AddCScript(script)) added_script++;
            },
            [&] {
                CKey key;
                if (!keys.empty()) {
                    key = PickValue(fuzzed_data_provider, keys);
                } else {
                    key = ConsumePrivateKey(fuzzed_data_provider, /*compressed=*/fuzzed_data_provider.ConsumeBool());
                }
                if (!key.IsValid()) return;
                const auto num_keys{fuzzed_data_provider.ConsumeIntegralInRange<size_t>(1, MAX_PUBKEYS_PER_MULTISIG)};
                std::vector<CPubKey> pubkeys;
                pubkeys.emplace_back(key.GetPubKey());
                for (size_t i = 1; i < num_keys; i++) {
                    if (fuzzed_data_provider.ConsumeBool()) {
                        pubkeys.emplace_back(key.GetPubKey());
                    } else {
                        CKey private_key{ConsumePrivateKey(fuzzed_data_provider, /*compressed=*/fuzzed_data_provider.ConsumeBool())};
                        if (!private_key.IsValid()) return;
                        pubkeys.emplace_back(private_key.GetPubKey());
                    }
                }
                if (pubkeys.size() < num_keys) return;
                CScript multisig_script{GetScriptForMultisig(num_keys, pubkeys)};
                if (!legacy_data.HaveCScript(CScriptID(multisig_script)) && legacy_data.AddCScript(multisig_script)) {
                    added_script++;
                }
            }
        );
    }

    auto result{legacy_data.MigrateToDescriptor()};
    assert(result);
    size_t added_chains{static_cast<size_t>(add_hd_chain) + static_cast<size_t>(add_inactive_hd_chain)};
    if ((add_hd_chain && version >= CHDChain::VERSION_HD_CHAIN_SPLIT) || (!add_hd_chain && add_inactive_hd_chain)) {
        added_chains *= 2;
    }
    size_t added_size{keys.size() + added_chains};
    if (added_script > 0) {
        assert(result->desc_spkms.size() >= added_size);
    } else {
        assert(result->desc_spkms.size() == added_size);
    }
    if (watch_only) assert(!result->watch_descs.empty());
    if (!result->solvable_descs.empty()) assert(added_script > 0);
}

} // namespace
} // namespace wallet
