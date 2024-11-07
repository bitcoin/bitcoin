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
#include <util/translation.h>
#include <validation.h>
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
    keystore.AddWalletDescriptor(wallet_desc, keys, /*label=*/"", /*internal=*/false);
    return keystore.GetDescriptorScriptPubKeyMan(wallet_desc);
};

FUZZ_TARGET(scriptpubkeyman, .init = initialize_spkm)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
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

} // namespace
} // namespace wallet
