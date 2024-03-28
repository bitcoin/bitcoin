// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <pubkey.h>
#include <secp256k1.h>
#include <span.h>
#include <bip352.h>

#include <secp256k1_silentpayments.h>
#include <streams.h>
#include <uint256.h>

#include <algorithm>
#include <logging.h>

extern secp256k1_context* secp256k1_context_sign; // TODO: this is hacky, is there a better solution?

namespace BIP352 {
PrivTweakData CreateInputPrivkeysTweak(
    const std::vector<CKey>& plain_keys,
    const std::vector<CKey>& taproot_keys,
    const COutPoint& smallest_outpoint)
{
    PrivTweakData result_tweak;

    // secp256k1 expects an array of pointers, so first translate
    // the vectors of CKeys into arrays of pointers to the underlying key data
    std::vector<const unsigned char *> plain_key_ptrs;
    std::vector<const unsigned char *> taproot_key_ptrs;
    plain_key_ptrs.reserve(plain_keys.size());
    taproot_key_ptrs.reserve(taproot_keys.size());
    for (const auto& key : plain_keys) {
        plain_key_ptrs.push_back(UCharCast(key.begin()));
    }
    for (const auto& key : taproot_keys) {
        taproot_key_ptrs.push_back(UCharCast(key.begin()));
    }

    // Serialize the outpoint
    std::vector<unsigned char> smallest_outpoint_ser;
    VectorWriter stream{smallest_outpoint_ser, 0};
    stream << smallest_outpoint;


    // Pass everything to secp256k1
    bool ret = secp256k1_silentpayments_create_private_tweak_data(secp256k1_context_sign, result_tweak.first.data(), result_tweak.second.data(),
        plain_key_ptrs.data(), plain_keys.size(), taproot_key_ptrs.data(), taproot_keys.size(), smallest_outpoint_ser.data());
    assert(ret);

    return result_tweak;
}

SharedSecret CreateSharedSecret(const PrivTweakData& priv_tweak_data, const CPubKey& receiver_scan_pubkey)
{
    SharedSecret result_ss;
    secp256k1_pubkey scan_pubkey_obj;
    bool ret = secp256k1_ec_pubkey_parse(secp256k1_context_static, &scan_pubkey_obj,
        receiver_scan_pubkey.data(), receiver_scan_pubkey.size());
    assert(ret);
    ret = secp256k1_silentpayments_create_shared_secret(secp256k1_context_static,
        result_ss.data(), &scan_pubkey_obj, priv_tweak_data.first.data(), priv_tweak_data.second.data());
    assert(ret);

    return result_ss;
}

SharedSecret CreateSharedSecret(const PubTweakData& public_tweak_data, const CKey& receiver_scan_privkey)
{
    SharedSecret result_ss;
    bool ret = secp256k1_silentpayments_create_shared_secret(secp256k1_context_static,
        result_ss.data(), &public_tweak_data.first, UCharCast(receiver_scan_privkey.data()), public_tweak_data.second.data());
    assert(ret);

    return result_ss;
}


PubTweakData CreateInputPubkeysTweak(
    const std::vector<CPubKey>& plain_pubkeys,
    const std::vector<XOnlyPubKey>& taproot_pubkeys,
    const COutPoint& smallest_outpoint)
{
    PubTweakData result_tweak;
    // TODO: see if there is a way to clean this up
    std::vector<secp256k1_pubkey> plain_pubkey_objs;
    std::vector<secp256k1_xonly_pubkey> taproot_pubkey_objs;
    std::vector<secp256k1_pubkey *> plain_pubkey_ptrs;
    std::vector<secp256k1_xonly_pubkey *> taproot_pubkey_ptrs;
    plain_pubkey_objs.reserve(plain_pubkeys.size());
    plain_pubkey_ptrs.reserve(plain_pubkeys.size());
    taproot_pubkey_objs.reserve(taproot_pubkeys.size());
    taproot_pubkey_ptrs.reserve(taproot_pubkeys.size());

    for (const CPubKey& pubkey : plain_pubkeys) {
        secp256k1_pubkey parsed_pubkey;
        bool ret = secp256k1_ec_pubkey_parse(secp256k1_context_static, &parsed_pubkey,
            pubkey.data(), pubkey.size());
        assert(ret);
        plain_pubkey_objs.emplace_back(parsed_pubkey);
    }
    for (const XOnlyPubKey& pubkey : taproot_pubkeys) {
        secp256k1_xonly_pubkey parsed_xonly_pubkey;
        bool ret = secp256k1_xonly_pubkey_parse(secp256k1_context_static, &parsed_xonly_pubkey, pubkey.data());
        assert(ret);
        taproot_pubkey_objs.emplace_back(parsed_xonly_pubkey);
    }
    for (auto &p : plain_pubkey_objs) {
        plain_pubkey_ptrs.push_back(&p);
    }
    for (auto &t : taproot_pubkey_objs) {
        taproot_pubkey_ptrs.push_back(&t);
    }

    std::vector<unsigned char> smallest_outpoint_ser;
    VectorWriter stream{smallest_outpoint_ser, 0};
    stream << smallest_outpoint;
    bool ret = secp256k1_silentpayments_create_public_tweak_data(secp256k1_context_static, &result_tweak.first, result_tweak.second.data(),
        plain_pubkey_ptrs.data(), plain_pubkey_ptrs.size(), taproot_pubkey_ptrs.data(), taproot_pubkey_ptrs.size(), smallest_outpoint_ser.data());
    assert(ret);
    return result_tweak;
}

XOnlyPubKey CreateOutput(const SharedSecret& shared_secret, const CPubKey& receiver_spend_pubkey, uint32_t output_index)
{
    secp256k1_xonly_pubkey xpk_result_obj;
    unsigned char xpk_result_bytes[32];
    secp256k1_pubkey spend_pubkey_obj;

    bool ret = secp256k1_ec_pubkey_parse(secp256k1_context_static, &spend_pubkey_obj,
        receiver_spend_pubkey.data(), receiver_spend_pubkey.size());
    assert(ret);

    // we ignore the pk_result_obj and the t_k outputs for now as these are only needed by the receiver
    ret = secp256k1_silentpayments_sender_create_output_pubkey(secp256k1_context_static,
        &xpk_result_obj, shared_secret.data(), &spend_pubkey_obj, output_index);
    assert(ret);

    ret = secp256k1_xonly_pubkey_serialize(secp256k1_context_static, xpk_result_bytes, &xpk_result_obj);
    assert(ret);

    return XOnlyPubKey{xpk_result_bytes};
}

ScanningResult CheckOutput(const SharedSecret& shared_secret, const CPubKey& receiver_spend_pubkey, const XOnlyPubKey& tx_output, uint32_t output_index)
{
    secp256k1_silentpayments_label_data label_data;
    secp256k1_pubkey spend_pubkey_obj;
    secp256k1_xonly_pubkey tx_output_obj;
    uint256 t_k;

    // Parse the pubkeys into secp pubkey and xonly_pubkey objects
    bool ret = secp256k1_ec_pubkey_parse(secp256k1_context_static, &spend_pubkey_obj,
        receiver_spend_pubkey.data(), receiver_spend_pubkey.size());
    assert(ret);
    ret = secp256k1_xonly_pubkey_parse(secp256k1_context_static, &tx_output_obj, tx_output.data());
    assert(ret);

    // If the generated output matches the given transaction output, the function returns a boolean along with the necessary
    // tweak data we need to spend this output later. If the generated output is not a direct match, it could still be a label
    // output.
    //
    // In this case, the function returns the tweak data and the necessary label data needed to check against the
    // receiver's label cache. If the label matches, the tweak data + label tweak should be added to the wallet along with the output.
    int direct_match{0};
    ret = secp256k1_silentpayments_receiver_scan_output(secp256k1_context_static,
        &direct_match, t_k.data(), &label_data, shared_secret.data(), &spend_pubkey_obj, output_index, &tx_output_obj);
    assert(ret);
    // If the generated output is a match, return early, as there is no label data to parse (and it is not needed).
    bool found(direct_match);
    if (found) return {found, t_k, {}, {}};
    // Despite not being a match, return everything the caller needs to keep checking the output against the label cache
    size_t pubkeylen = CPubKey::COMPRESSED_SIZE;
    CPubKey label;
    CPubKey label_negated;
    ret = secp256k1_ec_pubkey_serialize(secp256k1_context_static, (unsigned char*)label.begin(), &pubkeylen, &label_data.label, SECP256K1_EC_COMPRESSED);
    ret = secp256k1_ec_pubkey_serialize(secp256k1_context_static, (unsigned char*)label_negated.begin(), &pubkeylen, &label_data.label_negated, SECP256K1_EC_COMPRESSED);
    return {found, t_k, label, label_negated};
}

uint256 CombineTweaks(const uint256& t_k, const uint256& label_tweak)
{
    uint256 combined_tweak;
    bool ret = secp256k1_silentpayments_create_output_seckey(secp256k1_context_static, combined_tweak.data(), t_k.data(), label_tweak.data(), nullptr);
    assert(ret);
    return combined_tweak;
}

// Hacky test only function for creating the private key
// TODO: remove this in favor of something that is used only at signing time
CKey CreateFullKey(const CKey& spend_key, const uint256& tweak)
{
    uint256 combined_tweak;
    bool ret = secp256k1_silentpayments_create_output_seckey(secp256k1_context_static, combined_tweak.data(), UCharCast(spend_key.begin()), tweak.data(), nullptr);
    assert(ret);
    CKey fullkey;
    fullkey.Set(combined_tweak.begin(), combined_tweak.end(), true);
    memory_cleanse(&combined_tweak, sizeof(combined_tweak));
    return fullkey;
}

std::pair<CPubKey, uint256> CreateLabelTweak(const CKey& scan_key, const int m) {
    secp256k1_pubkey label_obj;
    unsigned char label_tweak[32];
    bool ret = secp256k1_silentpayments_create_label_tweak(secp256k1_context_sign, &label_obj, label_tweak, UCharCast(scan_key.data()), m);
    assert(ret);
    CPubKey label;
    size_t pubkeylen = CPubKey::COMPRESSED_SIZE;
    ret = secp256k1_ec_pubkey_serialize(secp256k1_context_static, (unsigned char*)label.begin(), &pubkeylen, &label_obj, SECP256K1_EC_COMPRESSED);
    assert(ret);
    return {label, uint256{label_tweak}};
}

CPubKey CreateLabeledSpendPubKey(const CPubKey& spend_pubkey, const CPubKey& label) {
    unsigned char labeled_spend_key_bytes[33];
    secp256k1_pubkey spend_obj;
    secp256k1_pubkey label_obj;
    bool ret = secp256k1_ec_pubkey_parse(secp256k1_context_static, &spend_obj, spend_pubkey.data(), spend_pubkey.size());
    assert(ret);
    ret = secp256k1_ec_pubkey_parse(secp256k1_context_static, &label_obj, label.data(), label.size());
    assert(ret);
    ret = secp256k1_silentpayments_create_address_spend_pubkey(secp256k1_context_static, labeled_spend_key_bytes, &spend_obj, &label_obj);
    assert(ret);
    return CPubKey{labeled_spend_key_bytes};
}
}; // namespace BIP352
