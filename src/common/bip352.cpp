// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <common/bip352.h>

#include <addresstype.h>
#include <coins.h>
#include <key.h>
#include <policy/policy.h>
#include <primitives/transaction.h>
#include <pubkey.h>
#include <script/interpreter.h>
#include <script/script.h>
#include <script/script_error.h>
#include <script/sign.h>
#include <script/solver.h>
#include <script/verify_flags.h>
#include <secp256k1.h>
#include <secp256k1_extrakeys.h>
#include <secp256k1_silentpayments.h>
#include <span.h>
#include <streams.h>
#include <uint256.h>
#include <util/check.h>

#include <algorithm>
#include <optional>
#include <span>
#include <type_traits>
#include <utility>

namespace bip352 {

class PrevoutsSummaryImpl
{
private:
    std::unique_ptr<secp256k1_silentpayments_prevouts_summary> m_prevouts_summary;

public:
    PrevoutsSummaryImpl() : m_prevouts_summary{std::make_unique<secp256k1_silentpayments_prevouts_summary>()} {}

    // Delete copy constructors
    PrevoutsSummaryImpl(const PrevoutsSummaryImpl&) = delete;
    PrevoutsSummaryImpl& operator=(const PrevoutsSummaryImpl&) = delete;

    secp256k1_silentpayments_prevouts_summary* Get() const { return m_prevouts_summary.get(); }
};

PrevoutsSummary::PrevoutsSummary() : m_impl{std::make_unique<PrevoutsSummaryImpl>()} {}

PrevoutsSummary::PrevoutsSummary(PrevoutsSummary&&) noexcept = default;
PrevoutsSummary& PrevoutsSummary::operator=(PrevoutsSummary&&) noexcept = default;

PrevoutsSummary::~PrevoutsSummary() = default;

secp256k1_silentpayments_prevouts_summary* PrevoutsSummary::Get() const
{
    return m_impl->Get();
}

SilentPaymentsLabel::SilentPaymentsLabel(const secp256k1_silentpayments_label& label) {
    m_label = std::make_unique<secp256k1_silentpayments_label>(label);
    int ret = secp256k1_silentpayments_recipient_label_serialize(secp256k1_context_static, m_vch, m_label.get());
    assert(ret);
}

SilentPaymentsLabel::SilentPaymentsLabel(const unsigned char* label) {
    memcpy(m_vch, label, CPubKey::COMPRESSED_SIZE);
}

std::optional<SilentPaymentsLabel> SilentPaymentsLabel::FromBytes(std::span<const unsigned char, CPubKey::COMPRESSED_SIZE> vch)
{
    secp256k1_silentpayments_label label_obj;
    if (!secp256k1_silentpayments_recipient_label_parse(secp256k1_context_static, &label_obj, vch.data())) {
        return std::nullopt;
    }
    return SilentPaymentsLabel(label_obj);
}

SilentPaymentsLabel::SilentPaymentsLabel(SilentPaymentsLabel&&) noexcept = default;
SilentPaymentsLabel& SilentPaymentsLabel::operator=(SilentPaymentsLabel&&) noexcept = default;
SilentPaymentsLabel::~SilentPaymentsLabel() = default;

SilentPaymentsLabel::SilentPaymentsLabel(const SilentPaymentsLabel& label) {
    if (label.m_label.get() != nullptr) {
        m_label = std::make_unique<secp256k1_silentpayments_label>(*label.m_label.get());
    }
    memcpy(m_vch, label.m_vch, CPubKey::COMPRESSED_SIZE);
}
SilentPaymentsLabel& SilentPaymentsLabel::operator=(const SilentPaymentsLabel& label) {
    if (this != &label) {
        if (label.m_label.get() != nullptr) {
            m_label = std::make_unique<secp256k1_silentpayments_label>(*label.m_label.get());
        } else {
            m_label.reset();
        }
        memcpy(m_vch, label.m_vch, CPubKey::COMPRESSED_SIZE);
    }
    return *this;
}

const secp256k1_silentpayments_label* SilentPaymentsLabel::Get() const {
    // SilentPaymentsLabel::SilentPaymentsLabel(const unsigned char* label) is a private
    // constructor that is only used by LabelLookupCallback for comparison
    assert(m_label.get() != nullptr);
    return m_label.get();
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
            bool has_annex = !txin.scriptWitness.stack.back().empty() && txin.scriptWitness.stack.back()[0] == ANNEX_TAG;
            size_t post_annex_size = txin.scriptWitness.stack.size() - (has_annex ? 1 : 0);
            if (post_annex_size > 1) {
                // Actually a script path spend
                const std::vector<unsigned char>& control = txin.scriptWitness.stack.at(post_annex_size - 1);
                if (control.size() < 33) {
                    return std::nullopt;
                }
                if (std::equal(WitnessV1Taproot::NUMS_H.begin(), WitnessV1Taproot::NUMS_H.end(), control.begin() + 1)) {
                    // Skip script path with H internal key
                    return std::nullopt;
                }
            }
        }

        XOnlyPubKey xonly_pubkey{solutions[0]};
        if (xonly_pubkey.IsFullyValid()) pubkey = xonly_pubkey;
    } else if (type == TxoutType::WITNESS_V0_KEYHASH && !txin.scriptWitness.stack.empty()) {
        // We ensure the witness stack is not empty before extracting the public key
        // since there are scenarios where it can be, e.g., before the transaction has been signed.
        pubkey = CPubKey{txin.scriptWitness.stack.back()};
    } else if (type == TxoutType::PUBKEYHASH || type == TxoutType::SCRIPTHASH) {
        // Use the script interpreter to get the stack after executing the scriptSig
        std::vector<std::vector<unsigned char>> stack;
        ScriptError serror;
        if (!EvalScript(stack, txin.scriptSig, MANDATORY_SCRIPT_VERIFY_FLAGS, DUMMY_CHECKER, SigVersion::BASE, &serror)) {
            return std::nullopt;
        }
        // The stack can be empty, e.g., before the transaction has been signed.
        if (stack.empty()) return std::nullopt;
        if (type == TxoutType::PUBKEYHASH) {
            pubkey = CPubKey{stack.back()};
        } else if (type == TxoutType::SCRIPTHASH) {
            // Check if the redeemScript is P2WPKH
            CScript redeem_script{stack.back().begin(), stack.back().end()};
            TxoutType rs_type = Solver(redeem_script, solutions);
            if (rs_type == TxoutType::WITNESS_V0_KEYHASH && !txin.scriptWitness.stack.empty()) {
                pubkey = CPubKey{txin.scriptWitness.stack.back()};
            }
        }
    }
    if (std::holds_alternative<XOnlyPubKey>(pubkey)) return pubkey;
    if (auto compressed_pubkey = std::get_if<CPubKey>(&pubkey)) {
        // IsFullyValid() is required here (not just IsValid()) since callers assume the
        // returned pubkey parses successfully as a valid secp256k1 curve point.
        if (compressed_pubkey->IsCompressed() && compressed_pubkey->IsFullyValid()) return *compressed_pubkey;
    }
    return std::nullopt;
}

std::optional<PrevoutsSummary> CreateInputPubkeysTweak(
    const std::vector<CPubKey>& plain_pubkeys,
    const std::vector<XOnlyPubKey>& taproot_pubkeys,
    const COutPoint& smallest_outpoint)
{
    PrevoutsSummary prevouts_summary;
    std::vector<secp256k1_pubkey> plain_pubkey_objs;
    std::vector<secp256k1_pubkey*> plain_pubkey_ptrs;
    plain_pubkey_objs.reserve(plain_pubkeys.size());
    plain_pubkey_ptrs.reserve(plain_pubkeys.size());
    for (const CPubKey& pubkey : plain_pubkeys) {
        bool ret = secp256k1_ec_pubkey_parse(secp256k1_context_static,
            &plain_pubkey_objs.emplace_back(), pubkey.data(), pubkey.size());
        // This pubkey is expected to be valid because GetPubKeyFromInput()
        // already called IsFullyValid() before including it here
        assert(ret);
        plain_pubkey_ptrs.push_back(&plain_pubkey_objs.back());
    }

    std::vector<secp256k1_xonly_pubkey> taproot_pubkey_objs;
    std::vector<secp256k1_xonly_pubkey*> taproot_pubkey_ptrs;
    taproot_pubkey_objs.reserve(taproot_pubkeys.size());
    taproot_pubkey_ptrs.reserve(taproot_pubkeys.size());
    for (const XOnlyPubKey& pubkey : taproot_pubkeys) {
        bool ret = secp256k1_xonly_pubkey_parse(secp256k1_context_static,
            &taproot_pubkey_objs.emplace_back(), pubkey.data());
        // This xonlypubkey is expected to be valid because
        // GetPubKeyFromInput() already called IsFullyValid()
        // before including it here
        assert(ret);
        taproot_pubkey_ptrs.push_back(&taproot_pubkey_objs.back());
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
        const auto coin_it = coins.find(txin.prevout);
        if (coin_it == coins.end()) return std::nullopt;
        const Coin& coin = coin_it->second;
        Assert(!coin.IsSpent());
        int witness_version{0};
        std::vector<unsigned char> witness_program;
        // BIP352 v0 skips transactions spending future witness versions.
        if (coin.out.scriptPubKey.IsWitnessProgram(witness_version, witness_program) && witness_version > 1) {
            return std::nullopt;
        }
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

std::optional<std::vector<secp256k1_xonly_pubkey>> CreateOutputs(
    const std::vector<SilentPaymentsDestination>& recipients,
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
        ret = secp256k1_ec_pubkey_parse(secp256k1_context_static, &recipient_obj.scan_pubkey, recipients[i].GetScanPubKey().data(), recipients[i].GetScanPubKey().size());
        assert(ret);
        ret = secp256k1_ec_pubkey_parse(secp256k1_context_static, &recipient_obj.spend_pubkey, recipients[i].GetSpendPubKey().data(), recipients[i].GetSpendPubKey().size());
        assert(ret);
        recipient_obj.index = i;
        recipient_objs.push_back(recipient_obj);
        recipient_ptrs.push_back(&recipient_objs[i]);

        secp256k1_xonly_pubkey generated_output{};
        generated_outputs.push_back(generated_output);
        generated_output_ptrs.push_back(&generated_outputs[i]);
    }
    for (const auto& key : plain_keys) {
        if (!key.IsValid()) return std::nullopt;
        plain_key_ptrs.push_back(UCharCast(key.begin()));
    }
    for (const auto& keypair : taproot_keypairs) {
        if (!keypair.IsValid()) return std::nullopt;
        taproot_keypair_ptrs.push_back(reinterpret_cast<const secp256k1_keypair*>(keypair.data()));
    }

    // Serialize the outpoint
    std::vector<unsigned char> smallest_outpoint_ser;
    VectorWriter stream{smallest_outpoint_ser, 0};
    stream << smallest_outpoint;

    ret = secp256k1_silentpayments_sender_create_outputs(GetSecp256k1SignContext(),
        generated_output_ptrs.data(),
        recipient_ptrs.data(), recipient_ptrs.size(),
        smallest_outpoint_ser.data(),
        taproot_keypair_ptrs.data(), taproot_keypair_ptrs.size(),
        plain_key_ptrs.data(), plain_key_ptrs.size()
    );
    if (!ret) return std::nullopt;
    return generated_outputs;
}

std::optional<std::map<size_t, WitnessV1Taproot>> GenerateSilentPaymentsTaprootDestinations(const std::map<size_t, SilentPaymentsDestination>& sp_dests, const std::vector<CKey>& plain_keys, const std::vector<KeyPair>& taproot_keys, const COutPoint& smallest_outpoint)
{
    if (sp_dests.empty()) return std::map<size_t, WitnessV1Taproot>();

    assert(!smallest_outpoint.IsNull());
    assert(!plain_keys.empty() || !taproot_keys.empty());

    bool ret;
    std::map<size_t, WitnessV1Taproot> tr_dests;
    std::vector<SilentPaymentsDestination> recipients;
    recipients.reserve(sp_dests.size());
    for (const auto& [i, addr] : sp_dests) {
        tr_dests.emplace(i, WitnessV1Taproot());
        recipients.push_back(addr);
    }
    auto outputs = CreateOutputs(recipients, plain_keys, taproot_keys, smallest_outpoint);
    // This will fail if any input pubkey is null or
    // inputs were maliciously crafted to sum to zero
    if (!outputs) return std::nullopt;
    assert(sp_dests.size() == outputs->size());
    size_t output_i{0};
    for (const auto& [i, addr] : sp_dests) {
        unsigned char xonly_pubkey_bytes[32];
        ret = secp256k1_xonly_pubkey_serialize(secp256k1_context_static, xonly_pubkey_bytes, &outputs.value()[output_i]);
        assert(ret);
        tr_dests[i] = WitnessV1Taproot{XOnlyPubKey{xonly_pubkey_bytes}};
        output_i++;
    }
    return tr_dests;
}

const unsigned char* LabelLookupCallback(const unsigned char* key, const void* context) {
    auto label_context = static_cast<const std::map<SilentPaymentsLabel, uint256>*>(context);
    SilentPaymentsLabel label{key};
    auto it = label_context->find(label);
    if (it != label_context->end()) {
        return it->second.begin();
    }
    return nullptr;
}

std::pair<SilentPaymentsLabel, uint256> CreateLabel(const CKey& scan_key, const uint32_t m) {
    secp256k1_silentpayments_label label_obj;
    unsigned char label_tweak[32];
    bool ret = secp256k1_silentpayments_recipient_label_create(GetSecp256k1SignContext(), &label_obj, label_tweak, UCharCast(scan_key.data()), m);
    assert(ret);
    return {SilentPaymentsLabel(label_obj), uint256{label_tweak}};
}

CPubKey CreateLabeledSpendPubKey(const CPubKey& spend_pubkey, const SilentPaymentsLabel& label) {
    secp256k1_pubkey spend_obj, labeled_spend_obj;
    bool ret = secp256k1_ec_pubkey_parse(secp256k1_context_static, &spend_obj, spend_pubkey.data(), spend_pubkey.size());
    assert(ret);
    ret = secp256k1_silentpayments_recipient_create_labeled_spend_pubkey(secp256k1_context_static, &labeled_spend_obj, &spend_obj, label.Get());
    assert(ret);
    size_t pubkeylen = CPubKey::COMPRESSED_SIZE;
    CPubKey labeled_spend_pubkey;
    ret = secp256k1_ec_pubkey_serialize(secp256k1_context_static, (unsigned char*)labeled_spend_pubkey.begin(), &pubkeylen, &labeled_spend_obj, SECP256K1_EC_COMPRESSED);
    assert(ret);
    return labeled_spend_pubkey;
}

SilentPaymentsReceiver::SilentPaymentsReceiver(const CKey& scan_key, const CPubKey& spend_pubkey,
    const LabelTweakMap& labels) : m_scan_key(scan_key), m_spend_pubkey(spend_pubkey), m_labels(labels)
{
    m_change_it = m_labels.emplace(CreateLabel(scan_key, 0)).first;
}

const SilentPaymentsReceiver::LabelTweakMap& SilentPaymentsReceiver::GetLabels() const {
    return m_labels;
}

SilentPaymentsDestination SilentPaymentsReceiver::BuildLabeledDestination(const SilentPaymentsLabel& label) const {
    CPubKey labeled_spend_pubkey = CreateLabeledSpendPubKey(m_spend_pubkey, label);
    auto dest{SilentPaymentsDestination::From(m_scan_key.GetPubKey(), labeled_spend_pubkey)};
    assert(dest);
    return *dest;
}

SilentPaymentsDestination SilentPaymentsReceiver::GenerateLabeledAddress(uint32_t m) {
    assert(m >= 1);
    auto it = m_labels.emplace(CreateLabel(m_scan_key, m)).first;
    return BuildLabeledDestination(it->first);
}

SilentPaymentsDestination SilentPaymentsReceiver::GetChangeDestination() const {
    return BuildLabeledDestination(m_change_it->first);
}

std::optional<std::vector<SilentPaymentsOutput>> SilentPaymentsReceiver::Scan(
    const PrevoutsSummary& prevouts_summary,
    const std::vector<XOnlyPubKey>& tx_outputs
) const {
    bool ret;
    secp256k1_pubkey spend_pubkey_obj;
    std::vector<secp256k1_silentpayments_found_output> found_output_objs;
    std::vector<secp256k1_silentpayments_found_output *> found_output_ptrs;
    std::vector<secp256k1_xonly_pubkey> tx_output_objs;
    std::vector<const secp256k1_xonly_pubkey *> tx_output_ptrs;
    found_output_objs.reserve(tx_outputs.size());
    found_output_ptrs.reserve(tx_outputs.size());
    tx_output_objs.reserve(tx_outputs.size());
    tx_output_ptrs.reserve(tx_outputs.size());

    assert(m_scan_key.IsValid());

    // Parse the pubkeys into secp pubkey and xonly_pubkey objects
    ret = secp256k1_ec_pubkey_parse(secp256k1_context_static, &spend_pubkey_obj, m_spend_pubkey.data(), m_spend_pubkey.size());
    assert(ret);

    for (const XOnlyPubKey& tx_output : tx_outputs) {
        secp256k1_xonly_pubkey tx_output_obj;
        ret = secp256k1_xonly_pubkey_parse(secp256k1_context_static, &tx_output_obj, tx_output.data());
        if (!ret) {
            // It is possible that a P2TR output encodes an invalid x-only pubkey.
            continue;
        }
        tx_output_objs.push_back(tx_output_obj);
        tx_output_ptrs.push_back(&tx_output_objs.back());
        found_output_objs.emplace_back();
        found_output_ptrs.push_back(&found_output_objs.back());
    }
    if (tx_output_ptrs.empty()) return std::vector<SilentPaymentsOutput>{};

    // Scan the outputs!
    uint32_t n_found_outputs = 0;
    ret = secp256k1_silentpayments_recipient_scan_outputs(secp256k1_context_static,
        found_output_ptrs.data(), &n_found_outputs,
        tx_output_ptrs.data(), tx_output_ptrs.size(),
        UCharCast(m_scan_key.begin()),
        prevouts_summary.Get(),
        &spend_pubkey_obj,
        LabelLookupCallback,
        &m_labels
    );
    if (!ret) return std::nullopt;

    std::vector<SilentPaymentsOutput> outputs;
    for (size_t i = 0; i < n_found_outputs; i++) {
        SilentPaymentsOutput sp_output;
        ret = secp256k1_xonly_pubkey_serialize(secp256k1_context_static, sp_output.output.begin(), &found_output_objs[i].output);
        assert(ret);
        sp_output.tweak = uint256{found_output_objs[i].tweak};
        if (found_output_objs[i].found_with_label) {
            sp_output.label = SilentPaymentsLabel(found_output_objs[i].label);
        }
        outputs.emplace_back(std::move(sp_output));
    }
    return outputs;
}
}; // namespace bip352
