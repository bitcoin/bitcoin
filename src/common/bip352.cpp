// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <addresstype.h>
#include <common/bip352.h>
#include <optional>
#include <primitives/transaction.h>
#include <secp256k1_extrakeys.h>
#include <pubkey.h>
#include <secp256k1.h>
#include <span.h>

#include <secp256k1_silentpayments.h>
#include <streams.h>
#include <uint256.h>
#include <logging.h>
#include <policy/policy.h>
#include <script/sign.h>
#include <script/solver.h>
#include <script/script_error.h>
#include <util/check.h>
#include <util/strencodings.h>

extern secp256k1_context* secp256k1_context_sign; // TODO: this is hacky, is there a better solution?

namespace bip352 {

class PrevoutsSummaryImpl
{
private:
    //! The actual secnonce itself
    std::unique_ptr<secp256k1_silentpayments_recipient_prevouts_summary> m_prevouts_summary;

public:
    PrevoutsSummaryImpl() : m_prevouts_summary{std::make_unique<secp256k1_silentpayments_recipient_prevouts_summary>()} {}

    // Delete copy constructors
    PrevoutsSummaryImpl(const PrevoutsSummaryImpl&) = delete;
    PrevoutsSummaryImpl& operator=(const PrevoutsSummaryImpl&) = delete;

    secp256k1_silentpayments_recipient_prevouts_summary* Get() const { return m_prevouts_summary.get(); }
};

PrevoutsSummary::PrevoutsSummary() : m_impl{std::make_unique<PrevoutsSummaryImpl>()} {}

PrevoutsSummary::PrevoutsSummary(PrevoutsSummary&&) noexcept = default;
PrevoutsSummary& PrevoutsSummary::operator=(PrevoutsSummary&&) noexcept = default;

PrevoutsSummary::~PrevoutsSummary() = default;

secp256k1_silentpayments_recipient_prevouts_summary* PrevoutsSummary::Get() const
{
    return m_impl->Get();
}

std::optional<PubKey> GetPubKeyFromInput(const CTxIn& txin, const CScript& spk)
{
    std::vector<std::vector<unsigned char>> solutions;
    TxoutType type = Solver(spk, solutions);
    PubKey pubkey;
    if (type == TxoutType::WITNESS_V1_TAPROOT) {
        // Check for H point in script path spend
        if (txin.scriptWitness.stack.size() > 1) {
            // Check for annex
            bool has_annex = txin.scriptWitness.stack.back()[0] == ANNEX_TAG;
            size_t post_annex_size = txin.scriptWitness.stack.size() - (has_annex ? 1 : 0);
            if (post_annex_size > 1) {
                // Actually a script path spend
                const std::vector<unsigned char>& control = txin.scriptWitness.stack.at(post_annex_size - 1);
                Assert(control.size() >= 33);
                if (std::equal(WitnessV1Taproot::NUMS_H.begin(), WitnessV1Taproot::NUMS_H.end(), control.begin() + 1)) {
                    // Skip script path with H internal key
                    return std::nullopt;
                }
            }
        }

        pubkey = XOnlyPubKey{solutions[0]};
    } else if (type == TxoutType::WITNESS_V0_KEYHASH && !txin.scriptWitness.stack.empty()) {
        // We ensure the witness stack is not empty before exctracting the public key
        // since there are scenarios where it can be, e.g., before the transaction has been signed.
        //
        // TODO: having this check here feels a bit hacky, will revisit with a more comprehensive solution
        pubkey = CPubKey{txin.scriptWitness.stack.back()};
    } else if (type == TxoutType::PUBKEYHASH || type == TxoutType::SCRIPTHASH) {
        // Use the script interpreter to get the stack after executing the scriptSig
        std::vector<std::vector<unsigned char>> stack;
        ScriptError serror;
        Assert(EvalScript(stack, txin.scriptSig, MANDATORY_SCRIPT_VERIFY_FLAGS, DUMMY_CHECKER, SigVersion::BASE, &serror));
        if (type == TxoutType::PUBKEYHASH) {
            pubkey = CPubKey{stack.back()};
        } else if (type == TxoutType::SCRIPTHASH) {
            // Check if the redeemScript is P2WPKH
            CScript redeem_script{stack.back().begin(), stack.back().end()};
            TxoutType rs_type = Solver(redeem_script, solutions);
            if (rs_type == TxoutType::WITNESS_V0_KEYHASH) {
                pubkey = CPubKey{txin.scriptWitness.stack.back()};
            }
        }
    }
    if (std::holds_alternative<XOnlyPubKey>(pubkey)) return pubkey;
    if (auto compressed_pubkey = std::get_if<CPubKey>(&pubkey)) {
        if (compressed_pubkey->IsCompressed() && compressed_pubkey->IsValid()) return *compressed_pubkey;
    }
    return std::nullopt;
}

std::optional<PrevoutsSummary> CreateInputPubkeysTweak(
    const std::vector<CPubKey>& plain_pubkeys,
    const std::vector<XOnlyPubKey>& taproot_pubkeys,
    const COutPoint& smallest_outpoint)
{
    PrevoutsSummary prevouts_summary;
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
    bool ret = secp256k1_silentpayments_recipient_prevouts_summary_create(secp256k1_context_static,
        prevouts_summary.Get(),
        smallest_outpoint_ser.data(),
        taproot_pubkey_ptrs.data(), taproot_pubkey_ptrs.size(),
        plain_pubkey_ptrs.data(), plain_pubkey_ptrs.size()
    );
    if (!ret) return std::nullopt;
    return prevouts_summary;
}

std::optional<PrevoutsSummary> GetSilentPaymentsPrevoutsSummary(const std::vector<CTxIn>& vin, const std::map<COutPoint, Coin>& coins)
{
    // Extract the keys from the inputs
    // or skip if no valid inputs
    std::vector<CPubKey> pubkeys;
    std::vector<XOnlyPubKey> xonly_pubkeys;
    std::vector<COutPoint> tx_outpoints;
    for (const CTxIn& txin : vin) {
        const Coin& coin = coins.at(txin.prevout);
        Assert(!coin.IsSpent());
        tx_outpoints.emplace_back(txin.prevout);
        auto pubkey = GetPubKeyFromInput(txin, coin.out.scriptPubKey);
        if (pubkey.has_value()) {
            std::visit([&pubkeys, &xonly_pubkeys](auto&& pubkey) {
                using T = std::decay_t<decltype(pubkey)>;
                if constexpr (std::is_same_v<T, CPubKey>) {
                    pubkeys.push_back(pubkey);
                } else if constexpr (std::is_same_v<T, XOnlyPubKey>) {
                    xonly_pubkeys.push_back(pubkey);
                }
            }, *pubkey);
        }
    }
    if (pubkeys.size() + xonly_pubkeys.size() == 0) return std::nullopt;
    auto smallest_outpoint = std::min_element(tx_outpoints.begin(), tx_outpoints.end(), bip352::BIP352Comparator());
    return CreateInputPubkeysTweak(pubkeys, xonly_pubkeys, *smallest_outpoint);
}

std::optional<CPubKey> GetSerializedSilentPaymentsPrevoutsSummary(const std::vector<CTxIn>& vin, const std::map<COutPoint, Coin>& coins)
{
    CPubKey serialized_prevouts_summary;
    bool ret;
    const auto& result = GetSilentPaymentsPrevoutsSummary(vin, coins);
    if (!result.has_value()) return std::nullopt;
    ret = secp256k1_silentpayments_recipient_prevouts_summary_serialize(
        secp256k1_context_static,
        (unsigned char *)serialized_prevouts_summary.begin(),
        result.value().Get()
    );
    assert(ret);
    return serialized_prevouts_summary;
}

std::vector<secp256k1_xonly_pubkey> CreateOutputs(
    const std::vector<V0SilentPaymentDestination> recipients,
    const std::vector<CKey>& plain_keys,
    const std::vector<KeyPair>& taproot_keypairs,
    const COutPoint& smallest_outpoint
) {
    bool ret;
    std::vector<const secp256k1_keypair *> taproot_keypair_ptrs;
    std::vector<const unsigned char *> plain_key_ptrs;
    taproot_keypair_ptrs.reserve(taproot_keypairs.size());
    plain_key_ptrs.reserve(plain_keys.size());

    std::vector<secp256k1_silentpayments_recipient> recipient_objs;
    std::vector<const secp256k1_silentpayments_recipient *> recipient_ptrs;
    recipient_objs.reserve(recipients.size());
    recipient_ptrs.reserve(recipients.size());

    std::vector<secp256k1_xonly_pubkey> generated_outputs;
    std::vector<secp256k1_xonly_pubkey *> generated_output_ptrs;
    generated_outputs.reserve(recipients.size());
    generated_output_ptrs.reserve(recipients.size());

    for (size_t i = 0; i < recipients.size(); i++) {
        secp256k1_silentpayments_recipient recipient_obj;
        ret = secp256k1_ec_pubkey_parse(secp256k1_context_static, &recipient_obj.scan_pubkey, recipients[i].m_scan_pubkey.data(), recipients[i].m_scan_pubkey.size());
        ret = secp256k1_ec_pubkey_parse(secp256k1_context_static, &recipient_obj.spend_pubkey, recipients[i].m_spend_pubkey.data(), recipients[i].m_spend_pubkey.size());
        assert(ret);
        recipient_obj.index = i;
        recipient_objs.push_back(recipient_obj);
        recipient_ptrs.push_back(&recipient_objs[i]);

        secp256k1_xonly_pubkey generated_output{};
        generated_outputs.push_back(generated_output);
        generated_output_ptrs.push_back(&generated_outputs[i]);
    }
    for (const auto& key : plain_keys) {
        plain_key_ptrs.push_back(UCharCast(key.begin()));
    }
    for (const auto& keypair : taproot_keypairs) {
        taproot_keypair_ptrs.push_back(reinterpret_cast<const secp256k1_keypair*>(keypair.data()));
    }

    // Serialize the outpoint
    std::vector<unsigned char> smallest_outpoint_ser;
    VectorWriter stream{smallest_outpoint_ser, 0};
    stream << smallest_outpoint;

    ret = secp256k1_silentpayments_sender_create_outputs(secp256k1_context_sign,
        generated_output_ptrs.data(),
        recipient_ptrs.data(), recipient_ptrs.size(),
        smallest_outpoint_ser.data(),
        taproot_keypair_ptrs.data(), taproot_keypair_ptrs.size(),
        plain_key_ptrs.data(), plain_key_ptrs.size()
    );
    if (!ret) return {};
    return generated_outputs;
}

std::optional<std::map<size_t, WitnessV1Taproot>> GenerateSilentPaymentTaprootDestinations(const std::map<size_t, V0SilentPaymentDestination>& sp_dests, const std::vector<CKey>& plain_keys, const std::vector<KeyPair>& taproot_keys, const COutPoint& smallest_outpoint)
{
    bool ret;
    std::map<size_t, WitnessV1Taproot> tr_dests;
    std::vector<V0SilentPaymentDestination> recipients;
    recipients.reserve(sp_dests.size());
    for (const auto& [_, addr] : sp_dests) {
        recipients.push_back(addr);
    }
    std::vector<secp256k1_xonly_pubkey> outputs = CreateOutputs(recipients, plain_keys, taproot_keys, smallest_outpoint);
    // This will only faily if the inputs were maliciously crafted to sum to zero
    if (outputs.empty()) return std::nullopt;
    for (size_t i = 0; i < outputs.size(); i++) {
        unsigned char xonly_pubkey_bytes[32];
        ret = secp256k1_xonly_pubkey_serialize(secp256k1_context_static, xonly_pubkey_bytes, &outputs[i]);
        assert(ret);
        tr_dests[i] = WitnessV1Taproot{XOnlyPubKey{xonly_pubkey_bytes}};
    }
    return tr_dests;
}

const unsigned char* LabelLookupCallback(const unsigned char* key, const void* context) {
    auto label_context = static_cast<const std::map<CPubKey, uint256>*>(context);
    CPubKey label{key, key + CPubKey::COMPRESSED_SIZE};
    // Find the pubkey in the map
    auto it = label_context->find(label);
    if (it != label_context->end()) {
        // Return a pointer to the uint256 label tweak if found
        // so it can be added to t_k
        return it->second.begin();
    }
    return nullptr;
}

std::pair<CPubKey, uint256> CreateLabelTweak(const CKey& scan_key, const int m) {
    secp256k1_pubkey label_obj;
    unsigned char label_tweak[32];
    bool ret = secp256k1_silentpayments_recipient_create_label(secp256k1_context_sign, &label_obj, label_tweak, UCharCast(scan_key.data()), m);
    assert(ret);
    CPubKey label;
    size_t pubkeylen = CPubKey::COMPRESSED_SIZE;
    ret = secp256k1_ec_pubkey_serialize(secp256k1_context_static, (unsigned char*)label.begin(), &pubkeylen, &label_obj, SECP256K1_EC_COMPRESSED);
    assert(ret);
    return {label, uint256{label_tweak}};
}

CPubKey CreateLabeledSpendPubKey(const CPubKey& spend_pubkey, const CPubKey& label) {
    secp256k1_pubkey spend_obj, label_obj, labeled_spend_obj;
    bool ret = secp256k1_ec_pubkey_parse(secp256k1_context_static, &spend_obj, spend_pubkey.data(), spend_pubkey.size());
    assert(ret);
    ret = secp256k1_ec_pubkey_parse(secp256k1_context_static, &label_obj, label.data(), label.size());
    assert(ret);
    ret = secp256k1_silentpayments_recipient_create_labeled_spend_pubkey(secp256k1_context_static, &labeled_spend_obj, &spend_obj, &label_obj);
    assert(ret);
    size_t pubkeylen = CPubKey::COMPRESSED_SIZE;
    CPubKey labeled_spend_pubkey;
    ret = secp256k1_ec_pubkey_serialize(secp256k1_context_static, (unsigned char*)labeled_spend_pubkey.begin(), &pubkeylen, &labeled_spend_obj, SECP256K1_EC_COMPRESSED);
    return labeled_spend_pubkey;
}

V0SilentPaymentDestination GenerateSilentPaymentLabeledAddress(const V0SilentPaymentDestination& recipient, const uint256& label)
{
    CKey label_key;
    label_key.Set(label.begin(), label.end(), true);
    CPubKey labeled_spend_pubkey = CreateLabeledSpendPubKey(recipient.m_spend_pubkey, label_key.GetPubKey());
    return V0SilentPaymentDestination{recipient.m_scan_pubkey, labeled_spend_pubkey};
}

std::optional<std::vector<SilentPaymentOutput>> ScanForSilentPaymentOutputs(
    const CKey& scan_key,
    const PrevoutsSummary& prevouts_summary,
    const CPubKey& recipient_spend_pubkey,
    const std::vector<XOnlyPubKey>& tx_outputs,
    const std::map<CPubKey, uint256>& labels
) {
    bool ret;
    secp256k1_pubkey spend_pubkey_obj;
    std::vector<secp256k1_silentpayments_found_output> found_output_objs;
    std::vector<secp256k1_silentpayments_found_output *> found_output_ptrs;
    std::vector<secp256k1_xonly_pubkey> tx_output_objs;
    std::vector<secp256k1_xonly_pubkey *> tx_output_ptrs;
    found_output_objs.reserve(tx_outputs.size());
    found_output_ptrs.reserve(tx_outputs.size());
    tx_output_objs.reserve(tx_outputs.size());
    tx_output_ptrs.reserve(tx_outputs.size());

    for (size_t i = 0; i < tx_outputs.size(); i++) {
        secp256k1_silentpayments_found_output found_output{};
        secp256k1_xonly_pubkey tx_output_obj;
        found_output_objs.push_back(found_output);
        found_output_ptrs.push_back(&found_output_objs[i]);
        ret = secp256k1_xonly_pubkey_parse(secp256k1_context_static, &tx_output_obj, tx_outputs[i].data());
        assert(ret);
        tx_output_objs.push_back(tx_output_obj);
        tx_output_ptrs.push_back(&tx_output_objs[i]);
    }

    // Parse the pubkeys into secp pubkey and xonly_pubkey objects
    ret = secp256k1_ec_pubkey_parse(secp256k1_context_static, &spend_pubkey_obj, recipient_spend_pubkey.data(), recipient_spend_pubkey.size());
    assert(ret);
    // Scan the outputs!
    size_t n_found_outputs = 0;
    ret = secp256k1_silentpayments_recipient_scan_outputs(secp256k1_context_static,
        found_output_ptrs.data(), &n_found_outputs,
        tx_output_ptrs.data(), tx_output_ptrs.size(),
        UCharCast(scan_key.begin()),
        prevouts_summary.Get(),
        &spend_pubkey_obj,
        LabelLookupCallback,
        &labels
    );
    assert(ret);
    if (n_found_outputs == 0) return {};
    std::vector<SilentPaymentOutput> outputs;
    for (size_t i = 0; i < n_found_outputs; i++) {
        SilentPaymentOutput sp_output;
        ret = secp256k1_xonly_pubkey_serialize(secp256k1_context_static, sp_output.output.begin(), &found_output_objs[i].output);
        assert(ret);
        sp_output.tweak = uint256{found_output_objs[i].tweak};
        if (found_output_objs[i].found_with_label) {
            CPubKey label;
            size_t pubkeylen = CPubKey::COMPRESSED_SIZE;
            ret = secp256k1_ec_pubkey_serialize(secp256k1_context_static, (unsigned char *)label.begin(), &pubkeylen, &found_output_objs[i].label, SECP256K1_EC_COMPRESSED);
            sp_output.label = label;
        }
        outputs.push_back(sp_output);
    }
    return outputs;
}
}; // namespace bip352
