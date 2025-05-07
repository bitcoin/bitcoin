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
    SelectParams(ChainType::MAIN);
    MOCKED_DESC_CONVERTER.Init();
}

void initialize_spkm_migration()
{
    static const auto testing_setup{MakeNoLogFileContext<const TestingSetup>()};
    g_setup = testing_setup.get();
    SelectParams(ChainType::MAIN);
}

/**
 * Key derivation is expensive. Deriving deep derivation paths take a lot of compute and we'd rather spend time
 * elsewhere in this target, like on actually fuzzing the DescriptorScriptPubKeyMan. So rule out strings which could
 * correspond to a descriptor containing a too large derivation path.
 */
static bool TooDeepDerivPath(std::string_view desc)
{
    const FuzzBufferType desc_buf{reinterpret_cast<const unsigned char *>(desc.data()), desc.size()};
    return HasDeepDerivPath(desc_buf);
}

static std::optional<std::pair<WalletDescriptor, FlatSigningProvider>> CreateWalletDescriptor(FuzzedDataProvider& fuzzed_data_provider)
{
    const std::string mocked_descriptor{fuzzed_data_provider.ConsumeRandomLengthString()};
    if (TooDeepDerivPath(mocked_descriptor)) return {};
    const auto desc_str{MOCKED_DESC_CONVERTER.GetDescriptor(mocked_descriptor)};
    if (!desc_str.has_value()) return std::nullopt;

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
    DescriptorScriptPubKeyMan* descriptor_spk_manager = nullptr;
    auto spk_manager = *Assert(keystore.AddWalletDescriptor(wallet_desc, keys, /*label=*/"", /*internal=*/false));
    if (spk_manager) {
        descriptor_spk_manager = dynamic_cast<DescriptorScriptPubKeyMan*>(spk_manager);
    }
    return descriptor_spk_manager;
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
                auto is_mine{spk_manager->IsMine(script)};
                if (is_mine == isminetype::ISMINE_SPENDABLE) {
                    assert(spk_manager->GetScriptPubKeys().count(script));
                }
            },
            [&] {
                auto spks{spk_manager->GetScriptPubKeys()};
                for (const CScript& spk : spks) {
                    assert(spk_manager->IsMine(spk) == ISMINE_SPENDABLE);
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
                const int sighash_type{fuzzed_data_provider.ConsumeIntegralInRange<int>(0, 150)};
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
    WalletContext context;
    auto& args{g_setup->m_args};
    context.args = const_cast<ArgsManager*>(&args);
    context.chain = g_setup->m_node.chain.get();

    std::unique_ptr<CWallet> wallet_ptr{std::make_unique<CWallet>(node.chain.get(), "", CreateMockableWalletDatabase())};
    wallet_ptr->chainStateFlushed(ChainstateRole::NORMAL, CBlockLocator{});
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
    CKey hd_key;
    if (add_hd_chain) {
        hd_key = PickValue(fuzzed_data_provider, keys);
        hd_chain.nVersion = fuzzed_data_provider.ConsumeBool() ? CHDChain::VERSION_HD_CHAIN_SPLIT : CHDChain::VERSION_HD_BASE;
        hd_chain.seed_id = hd_key.GetPubKey().GetID();
        legacy_data.LoadHDChain(hd_chain);
    }

    bool good_data{true};
    LIMITED_WHILE(good_data && fuzzed_data_provider.ConsumeBool(), 30) {
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                CKey private_key{ConsumePrivateKey(fuzzed_data_provider)};
                if (!private_key.IsValid()) return;
                const auto& dest{GetDestinationForKey(private_key.GetPubKey(), OutputType::LEGACY)};
                (void)legacy_data.LoadWatchOnly(GetScriptForDestination(dest));
            },
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
                (void)legacy_data.AddCScript(script);
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
                for (size_t i = 0; i < num_keys; i++) {
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
                (void)legacy_data.AddCScript(multisig_script);
            }
        );
    }

    good_data = true;
    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool() && good_data, 50) {
        std::optional<CMutableTransaction> mtx{ConsumeDeserializable<CMutableTransaction>(fuzzed_data_provider, TX_WITH_WITNESS)};
        if (!mtx) {
            good_data = false;
            return;
        }
        if (!mtx->vout.empty() && fuzzed_data_provider.ConsumeBool()) {
            const auto idx{fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, mtx->vout.size() - 1)};
            const auto output_type{fuzzed_data_provider.PickValueInArray({OutputType::BECH32, OutputType::LEGACY})};
            const auto label{fuzzed_data_provider.ConsumeRandomLengthString()};
            CScript spk{GetScriptForDestination(*Assert(wallet.GetNewDestination(output_type, label)))};
            mtx->vout[idx].scriptPubKey = spk;
        }
        if (mtx->vin.size() != 1 || !mtx->vin[0].prevout.IsNull()) {
            wallet.AddToWallet(MakeTransactionRef(*mtx), TxStateInactive{}, /*update_wtx=*/nullptr, /*fFlushOnClose=*/false, /*rescanning_old_block=*/true);
        }
    }

    MigrationResult result;
    bilingual_str error;
    (void)MigrateLegacyToDescriptor(std::move(wallet_ptr), /*passphrase=*/"", context, /*was_loaded=*/false, /*in_memory=*/true);
}

} // namespace
} // namespace wallet
