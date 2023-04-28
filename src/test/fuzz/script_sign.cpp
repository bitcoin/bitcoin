// Copyright (c) 2020-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <chainparamsbase.h>
#include <key.h>
#include <psbt.h>
#include <pubkey.h>
#include <script/keyorigin.h>
#include <script/sign.h>
#include <script/signingprovider.h>
#include <streams.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <util/translation.h>

#include <cassert>
#include <cstdint>
#include <iostream>
#include <map>
#include <optional>
#include <string>
#include <vector>

void initialize_script_sign()
{
    ECC_Start();
    SelectParams(CBaseChainParams::REGTEST);
}

FUZZ_TARGET_INIT(script_sign, initialize_script_sign)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    const std::vector<uint8_t> key = ConsumeRandomLengthByteVector(fuzzed_data_provider, 128);

    {
        CDataStream random_data_stream = ConsumeDataStream(fuzzed_data_provider);
        std::map<CPubKey, KeyOriginInfo> hd_keypaths;
        try {
            DeserializeHDKeypaths(random_data_stream, key, hd_keypaths);
        } catch (const std::ios_base::failure&) {
        }
        CDataStream serialized{SER_NETWORK, PROTOCOL_VERSION};
        SerializeHDKeypaths(serialized, hd_keypaths, CompactSizeWriter(fuzzed_data_provider.ConsumeIntegral<uint8_t>()));
    }

    {
        std::map<CPubKey, KeyOriginInfo> hd_keypaths;
        LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 10000) {
            const std::optional<CPubKey> pub_key = ConsumeDeserializable<CPubKey>(fuzzed_data_provider);
            if (!pub_key) {
                break;
            }
            const std::optional<KeyOriginInfo> key_origin_info = ConsumeDeserializable<KeyOriginInfo>(fuzzed_data_provider);
            if (!key_origin_info) {
                break;
            }
            hd_keypaths[*pub_key] = *key_origin_info;
        }
        CDataStream serialized{SER_NETWORK, PROTOCOL_VERSION};
        try {
            SerializeHDKeypaths(serialized, hd_keypaths, CompactSizeWriter(fuzzed_data_provider.ConsumeIntegral<uint8_t>()));
        } catch (const std::ios_base::failure&) {
        }
        std::map<CPubKey, KeyOriginInfo> deserialized_hd_keypaths;
        try {
            DeserializeHDKeypaths(serialized, key, hd_keypaths);
        } catch (const std::ios_base::failure&) {
        }
        assert(hd_keypaths.size() >= deserialized_hd_keypaths.size());
    }

    {
        SignatureData signature_data_1{ConsumeScript(fuzzed_data_provider)};
        SignatureData signature_data_2{ConsumeScript(fuzzed_data_provider)};
        signature_data_1.MergeSignatureData(signature_data_2);
    }

    FillableSigningProvider provider;
    CKey k;
    const std::vector<uint8_t> key_data = ConsumeRandomLengthByteVector(fuzzed_data_provider);
    k.Set(key_data.begin(), key_data.end(), fuzzed_data_provider.ConsumeBool());
    if (k.IsValid()) {
        provider.AddKey(k);
    }

    {
        const std::optional<CMutableTransaction> mutable_transaction = ConsumeDeserializable<CMutableTransaction>(fuzzed_data_provider);
        const std::optional<CTxOut> tx_out = ConsumeDeserializable<CTxOut>(fuzzed_data_provider);
        const unsigned int n_in = fuzzed_data_provider.ConsumeIntegral<unsigned int>();
        if (mutable_transaction && tx_out && mutable_transaction->vin.size() > n_in) {
            SignatureData signature_data_1 = DataFromTransaction(*mutable_transaction, n_in, *tx_out);
            CTxIn input;
            UpdateInput(input, signature_data_1);
            const CScript script = ConsumeScript(fuzzed_data_provider);
            SignatureData signature_data_2{script};
            signature_data_1.MergeSignatureData(signature_data_2);
        }
        if (mutable_transaction) {
            CTransaction tx_from{*mutable_transaction};
            CMutableTransaction tx_to;
            const std::optional<CMutableTransaction> opt_tx_to = ConsumeDeserializable<CMutableTransaction>(fuzzed_data_provider);
            if (opt_tx_to) {
                tx_to = *opt_tx_to;
            }
            CMutableTransaction script_tx_to = tx_to;
            CMutableTransaction sign_transaction_tx_to = tx_to;
            if (n_in < tx_to.vin.size() && tx_to.vin[n_in].prevout.n < tx_from.vout.size()) {
                SignatureData empty;
                (void)SignSignature(provider, tx_from, tx_to, n_in, fuzzed_data_provider.ConsumeIntegral<int>(), empty);
            }
            if (n_in < script_tx_to.vin.size()) {
                SignatureData empty;
                (void)SignSignature(provider, ConsumeScript(fuzzed_data_provider), script_tx_to, n_in, ConsumeMoney(fuzzed_data_provider), fuzzed_data_provider.ConsumeIntegral<int>(), empty);
                MutableTransactionSignatureCreator signature_creator{tx_to, n_in, ConsumeMoney(fuzzed_data_provider), fuzzed_data_provider.ConsumeIntegral<int>()};
                std::vector<unsigned char> vch_sig;
                CKeyID address;
                if (fuzzed_data_provider.ConsumeBool()) {
                    if (k.IsValid()) {
                        address = k.GetPubKey().GetID();
                    }
                } else {
                    address = CKeyID{ConsumeUInt160(fuzzed_data_provider)};
                }
                (void)signature_creator.CreateSig(provider, vch_sig, address, ConsumeScript(fuzzed_data_provider), fuzzed_data_provider.PickValueInArray({SigVersion::BASE, SigVersion::WITNESS_V0}));
            }
            std::map<COutPoint, Coin> coins;
            LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 10000) {
                const std::optional<COutPoint> outpoint = ConsumeDeserializable<COutPoint>(fuzzed_data_provider);
                if (!outpoint) {
                    break;
                }
                const std::optional<Coin> coin = ConsumeDeserializable<Coin>(fuzzed_data_provider);
                if (!coin) {
                    break;
                }
                coins[*outpoint] = *coin;
            }
            std::map<int, bilingual_str> input_errors;
            (void)SignTransaction(sign_transaction_tx_to, &provider, coins, fuzzed_data_provider.ConsumeIntegral<int>(), input_errors);
        }
    }

    {
        SignatureData signature_data_1;
        (void)ProduceSignature(provider, DUMMY_SIGNATURE_CREATOR, ConsumeScript(fuzzed_data_provider), signature_data_1);
        SignatureData signature_data_2;
        (void)ProduceSignature(provider, DUMMY_MAXIMUM_SIGNATURE_CREATOR, ConsumeScript(fuzzed_data_provider), signature_data_2);
    }
}
