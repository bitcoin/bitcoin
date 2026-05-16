// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <key.h>
#include <musig.h>
#include <pubkey.h>
#include <secp256k1_musig.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/random.h>
#include <uint256.h>

#include <algorithm>
#include <cstdint>
#include <map>
#include <vector>

namespace {

void initialize_musig()
{
    static ECC_Context ecc_context{};
}

std::vector<CPubKey> ConsumePubKeyVector(FuzzedDataProvider& fdp)
{
    std::vector<CPubKey> pubkeys;
    const size_t num_pubkeys{fdp.ConsumeIntegralInRange<size_t>(0, 16)};
    pubkeys.reserve(num_pubkeys);
    for (size_t i{0}; i < num_pubkeys; ++i) {
        CallOneOf(
            fdp,
            [&] { // ConsumePrivateKey
                const CKey key{ConsumePrivateKey(fdp)};
                if (key.IsValid()) {
                    pubkeys.push_back(key.GetPubKey());
                }
            },
            [&] { // ConsumeBytes
                const std::vector<unsigned char> bytes{
                    fdp.ConsumeBytes<unsigned char>(
                        fdp.ConsumeIntegralInRange<size_t>(0, CPubKey::SIZE))};
                if (CPubKey::ValidSize(bytes)) {
                    pubkeys.emplace_back(bytes);
                }
            },
            [&] { // ConsumeDeserializable
                if (const std::optional<CPubKey> pubkey{ConsumeDeserializable<CPubKey>(fdp)}) {
                    if (pubkey->size() != 0) {
                        pubkeys.push_back(*pubkey);
                    }
                }
            });
    }
    return pubkeys;
}

} // namespace

FUZZ_TARGET(musig, .init = initialize_musig)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fdp{buffer.data(), buffer.size()};

    LIMITED_WHILE (fdp.ConsumeBool(), 50) {
        std::vector<CPubKey> pubkeys{ConsumePubKeyVector(fdp)};

        CallOneOf(
            fdp,
            [&] { // MuSig2AggregatePubkeys
                (void)MuSig2AggregatePubkeys(pubkeys);
            },
            [&] { // MuSig2AggregatePubkeys with keyagg_cache and optional expected
                secp256k1_musig_keyagg_cache keyagg_cache;
                std::optional<CPubKey> expected;
                if (fdp.ConsumeBool()) {
                    expected = MuSig2AggregatePubkeys(pubkeys);
                    // Plant expected != agg so the mismatch return stays reachable.
                    if (expected.has_value() && fdp.ConsumeBool()) {
                        std::vector<unsigned char> ser{expected->begin(), expected->end()};
                        if (!ser.empty()) {
                            ser.back() ^= 1;
                            expected = CPubKey{ser.begin(), ser.end()};
                        }
                    }
                }
                (void)MuSig2AggregatePubkeys(pubkeys, keyagg_cache, expected);
            },
            [&] { // CreateMuSig2SyntheticXpub
                if (pubkeys.empty()) {
                    return;
                }
                const size_t idx{fdp.ConsumeIntegralInRange<size_t>(0, pubkeys.size() - 1)};
                (void)CreateMuSig2SyntheticXpub(pubkeys[idx]);
            },
            [&] { // MuSig2SessionID
                if (pubkeys.empty()) {
                    return;
                }
                const size_t i{fdp.ConsumeIntegralInRange<size_t>(0, pubkeys.size() - 1)};
                const size_t j{fdp.ConsumeIntegralInRange<size_t>(0, pubkeys.size() - 1)};
                const std::vector<uint8_t> pubnonce{
                    fdp.ConsumeBytes<uint8_t>(MUSIG2_PUBNONCE_SIZE)};
                (void)MuSig2SessionID(pubkeys[i], pubkeys[j], ConsumeUInt256(fdp), pubnonce);
            },
            [&] { // CreateMuSig2Nonce then partial (optional uninit) then aggregate
                const CKey our_key{ConsumePrivateKey(fdp, /*compressed=*/true)};
                if (!our_key.IsValid()) {
                    return;
                }

                const CPubKey our_pubkey{our_key.GetPubKey()};
                // Include our key so nonce and partial stay reachable.
                if (std::find(pubkeys.begin(), pubkeys.end(), our_pubkey) == pubkeys.end()) {
                    pubkeys.push_back(our_pubkey);
                }

                const std::optional<CPubKey> aggregate{MuSig2AggregatePubkeys(pubkeys)};
                if (!aggregate) {
                    return;
                }

                const uint256 sighash{ConsumeUInt256(fdp)};
                MuSig2SecNonce secnonce;
                const std::vector<uint8_t> our_pubnonce{CreateMuSig2Nonce(
                    secnonce, sighash, our_key, *aggregate, pubkeys)};
                if (our_pubnonce.empty()) {
                    return;
                }

                std::map<CPubKey, std::vector<uint8_t>> pubnonces;
                for (const CPubKey& pubkey : pubkeys) {
                    if (pubkey == our_pubkey) {
                        pubnonces[pubkey] = our_pubnonce;
                    } else {
                        // Sized bytes, not generated nonces.
                        pubnonces[pubkey] = fdp.ConsumeBytes<uint8_t>(MUSIG2_PUBNONCE_SIZE);
                    }
                }

                std::vector<std::pair<uint256, bool>> tweaks;
                LIMITED_WHILE (fdp.ConsumeBool(), 4) {
                    tweaks.emplace_back(ConsumeUInt256(fdp), fdp.ConsumeBool());
                }

                if (fdp.ConsumeBool()) {
                    MuSig2SecNonce uninitialized_secnonce;
                    (void)CreateMuSig2PartialSig(
                        sighash, our_key, *aggregate, pubkeys, pubnonces, uninitialized_secnonce, tweaks);
                }

                (void)CreateMuSig2PartialSig(
                    sighash, our_key, *aggregate, pubkeys, pubnonces, secnonce, tweaks);

                std::map<CPubKey, uint256> partial_sigs;
                for (const CPubKey& pubkey : pubkeys) {
                    partial_sigs[pubkey] = ConsumeUInt256(fdp);
                }
                (void)CreateMuSig2AggregateSig(
                    pubkeys, *aggregate, tweaks, sighash, pubnonces, partial_sigs);
            });
    }
}
