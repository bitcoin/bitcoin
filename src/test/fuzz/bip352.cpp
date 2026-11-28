// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <addresstype.h>
#include <bech32.h>
#include <chainparams.h>
#include <coins.h>
#include <common/bip352.h>
#include <hash.h>
#include <key.h>
#include <primitives/transaction.h>
#include <pubkey.h>
#include <script/interpreter.h>
#include <script/script.h>
#include <script/sign.h>
#include <script/solver.h>
#include <secp256k1.h>
#include <span.h>
#include <streams.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <uint256.h>
#include <util/chaintype.h>
#include <util/strencodings.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

using namespace bip352;

namespace {
void initialize_bip352()
{
    static ECC_Context ecc_context{};
    SelectParams(ChainType::MAIN);
}

std::optional<CKey> ConsumeValidKey(FuzzedDataProvider& fdp)
{
    const uint256 secret{ConsumeUInt256(fdp)};
    CKey key;
    key.Set(secret.begin(), secret.end(), /*fCompressedIn=*/true);
    if (!key.IsValid()) return std::nullopt;
    return key;
}

//! A valid compressed or uncompressed public key, or 33 arbitrary bytes.
std::vector<unsigned char> ConsumePubKeyBytes(FuzzedDataProvider& fdp)
{
    if (fdp.ConsumeBool()) {
        if (const auto key{ConsumeValidKey(fdp)}) {
            if (fdp.ConsumeBool()) return ToByteVector(key->GetPubKey());
            CKey uncompressed;
            uncompressed.Set(key->begin(), key->end(), /*fCompressedIn=*/false);
            return ToByteVector(uncompressed.GetPubKey());
        }
    }
    std::vector<unsigned char> bytes{fdp.ConsumeBytes<unsigned char>(CPubKey::COMPRESSED_SIZE)};
    bytes.resize(CPubKey::COMPRESSED_SIZE);
    return bytes;
}

std::vector<unsigned char> ConsumeWitnessItem(FuzzedDataProvider& fdp)
{
    return ConsumeRandomLengthByteVector(fdp, TAPROOT_CONTROL_MAX_SIZE + TAPROOT_CONTROL_NODE_SIZE);
}

//! Returns key + tweak (mod the curve order).
CKey TweakKey(const CKey& key, const uint256& tweak)
{
    std::array<std::byte, 32> bytes;
    std::copy(key.begin(), key.end(), bytes.begin());
    const int ret{secp256k1_ec_seckey_tweak_add(secp256k1_context_static, UCharCast(bytes.data()), tweak.data())};
    assert(ret);
    CKey result;
    result.Set(bytes.begin(), bytes.end(), /*fCompressedIn=*/true);
    return result;
}

//! Returns n - key, whose public key is the negation of key's.
CKey NegateKey(const CKey& key)
{
    std::array<std::byte, 32> bytes;
    std::copy(key.begin(), key.end(), bytes.begin());
    const int ret{secp256k1_ec_seckey_negate(secp256k1_context_static, UCharCast(bytes.data()))};
    assert(ret);
    CKey result;
    result.Set(bytes.begin(), bytes.end(), /*fCompressedIn=*/true);
    return result;
}

void AssertValidPubKey(const PubKey& pubkey)
{
    std::visit([](const auto& key) { assert(key.IsFullyValid()); }, pubkey);
    if (const auto* key{std::get_if<CPubKey>(&pubkey)}) assert(key->IsCompressed());
}
} // namespace

//! Decoding an arbitrary string must never crash, an encoded address must be accepted exactly when
//! it is valid, and an accepted address must hold exactly the encoded version, keys and extension data.
FUZZ_TARGET(bip352_decode_address, .init = initialize_bip352)
{
    FuzzedDataProvider fdp{buffer.data(), buffer.size()};
    std::string str;
    std::optional<bool> expected_valid;
    std::vector<unsigned char> expected_payload;
    if (fdp.ConsumeBool()) {
        // Build a correctly checksummed address so the payload checks are reached.
        std::vector<unsigned char> payload{fdp.ConsumeIntegral<uint8_t>()};
        const std::vector<unsigned char> body{fdp.ConsumeBool() ? ConsumePubKeyBytes(fdp) : ConsumeRandomLengthByteVector(fdp, 33)};
        payload.insert(payload.end(), body.begin(), body.end());
        const std::vector<unsigned char> body2{fdp.ConsumeBool() ? ConsumePubKeyBytes(fdp) : ConsumeRandomLengthByteVector(fdp, 33)};
        payload.insert(payload.end(), body2.begin(), body2.end());
        const std::vector<unsigned char> extra{ConsumeRandomLengthByteVector(fdp, 64)};
        payload.insert(payload.end(), extra.begin(), extra.end());
        std::vector<unsigned char> data{static_cast<unsigned char>(payload[0] & 0x1f)};
        ConvertBits<8, 5, true>([&](unsigned char c) { data.push_back(c); }, payload.begin() + 1, payload.end());
        std::string hrp{Params().SilentPaymentsHRP()};
        if (fdp.ConsumeBool()) {
            // bech32::Encode requires a lowercase human-readable part.
            hrp.clear();
            static constexpr std::string_view CHARSET{"qpzry9x8gf2tvdw0s3jn54khce6mua7l"};
            for (size_t n{fdp.ConsumeIntegralInRange<size_t>(1, 8)}; n > 0; --n) {
                hrp += CHARSET[fdp.ConsumeIntegralInRange<size_t>(0, CHARSET.size() - 1)];
            }
        }
        const auto encoding{fdp.ConsumeBool() ? bech32::Encoding::BECH32M : bech32::Encoding::BECH32};
        str = bech32::Encode(encoding, hrp, data);
        // Non-zero padding bits and an empty data part are never valid.
        if ((payload.size() - 1) * 8 % 5 != 0) {
            std::vector<unsigned char> bad_padding{data};
            bad_padding.back() |= 1;
            assert(!DecodeSilentPaymentsAddress(bech32::Encode(encoding, hrp, bad_padding), Params()).has_value());
        }
        assert(!DecodeSilentPaymentsAddress(bech32::Encode(encoding, hrp, {}), Params()).has_value());
        // BIP352: Bech32m, the network's prefix, versions 0 to 30, exactly 66 data bytes for v0 and
        // at least 66 for later versions, and two valid compressed public keys.
        const uint8_t version{data[0]};
        const size_t size{payload.size() - 1};
        bool valid{encoding == bech32::Encoding::BECH32M && hrp == Params().SilentPaymentsHRP() && version <= 30 &&
                   (version == 0 ? size == 66 : size >= 66)};
        if (valid) {
            const CPubKey scan{payload.begin() + 1, payload.begin() + 34};
            const CPubKey spend{payload.begin() + 34, payload.begin() + 67};
            valid = scan.IsCompressed() && scan.IsFullyValid() && spend.IsCompressed() && spend.IsFullyValid();
        }
        expected_valid = valid;
        expected_payload = payload;
    } else {
        str = fdp.ConsumeRandomLengthString(1024);
    }

    const auto dest{DecodeSilentPaymentsAddress(str, Params())};
    if (expected_valid.has_value()) assert(dest.has_value() == *expected_valid);
    if (!dest.has_value()) return;
    assert(dest->GetVersion() <= 30);
    assert(dest->GetScanPubKey().IsCompressed() && dest->GetScanPubKey().IsFullyValid());
    assert(dest->GetSpendPubKey().IsCompressed() && dest->GetSpendPubKey().IsFullyValid());
    if (dest->GetVersion() == 0) assert(dest->GetExtensionData().empty());
    if (!expected_payload.empty()) {
        assert(dest->GetVersion() == (expected_payload[0] & 0x1f));
        assert(dest->GetScanPubKey() == CPubKey(expected_payload.begin() + 1, expected_payload.begin() + 34));
        assert(dest->GetSpendPubKey() == CPubKey(expected_payload.begin() + 34, expected_payload.begin() + 67));
        assert(std::ranges::equal(dest->GetExtensionData(), std::span{expected_payload}.subspan(67)));
    }
}

//! SilentPaymentsDestination::From must accept exactly the versions, extension data and public
//! keys that BIP352 allows, and keep what it was given.
FUZZ_TARGET(bip352_destination_from, .init = initialize_bip352)
{
    FuzzedDataProvider fdp{buffer.data(), buffer.size()};
    const auto consume_pubkey{[&] {
        if (fdp.ConsumeBool()) {
            if (const auto key{ConsumeValidKey(fdp)}) {
                CKey uncompressed;
                uncompressed.Set(key->begin(), key->end(), /*fCompressedIn=*/false);
                return fdp.ConsumeBool() ? key->GetPubKey() : uncompressed.GetPubKey();
            }
        }
        std::vector<unsigned char> bytes{fdp.ConsumeBytes<unsigned char>(fdp.ConsumeBool() ? CPubKey::COMPRESSED_SIZE : CPubKey::SIZE)};
        return CPubKey{bytes};
    }};
    const CPubKey scan{consume_pubkey()};
    const CPubKey spend{consume_pubkey()};
    const uint8_t version{fdp.ConsumeBool() ? fdp.PickValueInArray<uint8_t>({0, 1, 30, 31, 255}) : fdp.ConsumeIntegral<uint8_t>()};
    const std::vector<unsigned char> extension{ConsumeRandomLengthByteVector(fdp, 64)};

    const auto dest{SilentPaymentsDestination::From(scan, spend, version, extension)};
    const bool allowed{version <= 30 && (version != 0 || extension.empty()) &&
                       scan.IsCompressed() && scan.IsFullyValid() && spend.IsCompressed() && spend.IsFullyValid()};
    assert(dest.has_value() == allowed);
    if (!dest) return;
    assert(dest->GetScanPubKey() == scan && dest->GetSpendPubKey() == spend && dest->GetVersion() == version);
    assert(std::ranges::equal(dest->GetExtensionData(), extension));
}

//! Extracting the public key from an arbitrary input must never crash, and any returned key must
//! be valid. For inputs built to follow a BIP352 rule exactly, the result must be the one the rule
//! requires: the key, or no key.
FUZZ_TARGET(bip352_pubkey_from_input, .init = initialize_bip352)
{
    FuzzedDataProvider fdp{buffer.data(), buffer.size()};
    const std::vector<unsigned char> pubkey_bytes{ConsumePubKeyBytes(fdp)};
    const uint160 pubkey_hash{Hash160(pubkey_bytes)};
    const CPubKey pubkey{pubkey_bytes};
    const bool pubkey_ok{pubkey.IsCompressed() && pubkey.IsFullyValid()};

    CTxIn txin;
    std::vector<std::vector<unsigned char>> stack;
    CScript spk;
    // Expected result when the input is built to follow a rule exactly; unset when not determined.
    std::optional<std::optional<PubKey>> expected;

    switch (fdp.ConsumeIntegralInRange<int>(0, 4)) {
    case 0: { // P2PKH, with arbitrary pushes around the key.
        spk = GetScriptForDestination(PKHash{pubkey_hash});
        CScript script_sig;
        // Other pushes, including other valid public keys, may come before and after the key.
        bool pushed{false};
        const auto push_other{[&] {
            const std::vector<unsigned char> other{fdp.ConsumeBool() ? ConsumePubKeyBytes(fdp) : ConsumeRandomLengthByteVector(fdp, 80)};
            pushed |= other == pubkey_bytes;
            script_sig << other;
        }};
        while (fdp.ConsumeBool()) push_other();
        if (fdp.ConsumeBool()) {
            script_sig << pubkey_bytes;
            pushed = true;
        }
        while (fdp.ConsumeBool()) push_other();
        const bool tail{fdp.ConsumeBool()};
        if (tail) {
            const CScript extra{ConsumeScript(fdp)};
            script_sig.insert(script_sig.end(), extra.begin(), extra.end());
        }
        txin.scriptSig = script_sig;
        // Only a push of the key itself can match the hash, so an uncompressed or invalid key is
        // never returned. A valid compressed key must be found wherever it is pushed, even after
        // unrelated pushes; when it is not pushed, the arbitrary tail could still push it.
        if (!pubkey_ok) {
            expected = std::optional<PubKey>{};
        } else if (pushed) {
            expected = PubKey{pubkey};
        } else if (!tail) {
            expected = std::optional<PubKey>{};
        }
        break;
    }
    case 1: { // P2WPKH: the key is the last witness item.
        spk = GetScriptForDestination(WitnessV0KeyHash{pubkey_hash});
        stack.push_back(ConsumeRandomLengthByteVector(fdp, 80));
        stack.push_back(fdp.ConsumeBool() ? pubkey_bytes : ConsumeWitnessItem(fdp));
        // An invalid key is never returned, and the key that matches the program always is. A valid
        // key that does not match the program is left undetermined: such a spend is invalid, and
        // returning it or not are both acceptable.
        const CPubKey last{stack.back()};
        if (!last.IsCompressed() || !last.IsFullyValid()) {
            expected = std::optional<PubKey>{};
        } else if (last.GetID() == CKeyID{pubkey_hash}) {
            expected = PubKey{last};
        }
        break;
    }
    case 2: { // P2SH: only a P2WPKH redeem script is eligible.
        const bool wraps_p2wpkh{fdp.ConsumeBool()};
        const CScript redeem{wraps_p2wpkh ? GetScriptForDestination(WitnessV0KeyHash{pubkey_hash}) : ConsumeScript(fdp)};
        spk = GetScriptForDestination(ScriptHash{redeem});
        CScript script_sig;
        script_sig << ToByteVector(redeem);
        const bool tail{fdp.ConsumeBool()};
        if (tail) {
            const CScript extra{ConsumeScript(fdp)};
            script_sig.insert(script_sig.end(), extra.begin(), extra.end());
        }
        txin.scriptSig = script_sig;
        stack.push_back(ConsumeRandomLengthByteVector(fdp, 80));
        stack.push_back(fdp.ConsumeBool() ? pubkey_bytes : ConsumeWitnessItem(fdp));
        // A scriptSig that fails to evaluate never yields a key. This mirrors the code, which evaluates
        // it with the dummy checker; any other checker could fail on a tail that the code accepts.
        // Any other scriptSig with more than the single push of the redeem script is left
        // undetermined.
        std::vector<std::vector<unsigned char>> eval_stack;
        if (tail && !EvalScript(eval_stack, script_sig, SCRIPT_VERIFY_NONE, DUMMY_CHECKER, SigVersion::BASE)) {
            expected = std::optional<PubKey>{};
        }
        if (!tail) {
            std::vector<std::vector<unsigned char>> solutions;
            const bool is_p2wpkh{Solver(redeem, solutions) == TxoutType::WITNESS_V0_KEYHASH};
            const CPubKey last{stack.back()};
            if (!is_p2wpkh || !last.IsCompressed() || !last.IsFullyValid()) {
                expected = std::optional<PubKey>{};
            } else if (last.GetID() == CKeyID{uint160{solutions[0]}}) {
                expected = PubKey{last};
            }
        }
        break;
    }
    case 3: { // P2TR: key path or script path, with or without an annex.
        std::vector<unsigned char> output_key{fdp.ConsumeBool() ? std::vector<unsigned char>{} : fdp.ConsumeBytes<unsigned char>(32)};
        if (output_key.empty()) {
            if (const auto key{ConsumeValidKey(fdp)}) {
                const XOnlyPubKey xonly{key->GetPubKey()};
                output_key.assign(xonly.begin(), xonly.end());
            }
        }
        output_key.resize(32);
        spk = CScript() << OP_1 << output_key;
        for (int n_items{fdp.ConsumeIntegralInRange<int>(0, 4)}; n_items > 0; --n_items) {
            if (fdp.ConsumeBool()) {
                // Control block: leaf version byte, internal key (possibly NUMS_H), path.
                std::vector<unsigned char> control{static_cast<unsigned char>(fdp.ConsumeIntegral<uint8_t>() | TAPROOT_LEAF_TAPSCRIPT)};
                const std::vector<unsigned char> nums_h(XOnlyPubKey::NUMS_H.begin(), XOnlyPubKey::NUMS_H.end());
                std::vector<unsigned char> internal{fdp.ConsumeBool() ? nums_h :
                                                                        fdp.ConsumeBytes<unsigned char>(32)};
                internal.resize(32);
                control.insert(control.end(), internal.begin(), internal.end());
                // Favour the edges: an empty path, the longest valid path and one node too many.
                constexpr size_t MAX_NODES{TAPROOT_CONTROL_MAX_NODE_COUNT};
                const size_t path_nodes{fdp.ConsumeBool() ? fdp.PickValueInArray<size_t>({0, 1, MAX_NODES, MAX_NODES + 1}) :
                                                            fdp.ConsumeIntegralInRange<size_t>(0, MAX_NODES + 1)};
                control.resize(control.size() + path_nodes * TAPROOT_CONTROL_NODE_SIZE + (fdp.ConsumeBool() ? 1 : 0));
                stack.push_back(control);
            } else {
                stack.push_back(ConsumeWitnessItem(fdp));
            }
        }
        if (fdp.ConsumeBool()) {
            std::vector<unsigned char> annex{ConsumeRandomLengthByteVector(fdp, 64)};
            annex.insert(annex.begin(), ANNEX_TAG);
            stack.push_back(annex);
        }
        const XOnlyPubKey xonly{std::span<const unsigned char>{output_key}};
        const std::optional<PubKey> key_result{xonly.IsFullyValid() ? std::optional<PubKey>{xonly} : std::optional<PubKey>{}};
        // BIP341: with two or more items, a last item starting with ANNEX_TAG is the annex.
        size_t items{stack.size()};
        if (items >= 2 && !stack.back().empty() && stack.back()[0] == ANNEX_TAG) --items;
        if (items == 0) {
            expected = std::optional<PubKey>{};
        } else if (items == 1) {
            expected = key_result; // key path
        } else {
            // Script path: the last item is the control block. Malformed control blocks
            // (consensus-invalid anyway) and the NUMS_H internal key are not eligible.
            const auto& control{stack[items - 1]};
            const bool control_ok{control.size() >= TAPROOT_CONTROL_BASE_SIZE && control.size() <= TAPROOT_CONTROL_MAX_SIZE &&
                                  (control.size() - TAPROOT_CONTROL_BASE_SIZE) % TAPROOT_CONTROL_NODE_SIZE == 0};
            const bool nums{control_ok && std::equal(XOnlyPubKey::NUMS_H.begin(), XOnlyPubKey::NUMS_H.end(), control.begin() + 1)};
            expected = (control_ok && !nums) ? key_result : std::optional<PubKey>{};
        }
        break;
    }
    default: // Anything else.
        spk = ConsumeScript(fdp);
        txin.scriptSig = ConsumeScript(fdp);
        while (fdp.ConsumeBool() && stack.size() < 5) stack.push_back(ConsumeWitnessItem(fdp));
        break;
    }
    txin.scriptWitness = CScriptWitness{stack};

    const auto result{GetPubKeyFromInput(txin, spk)};
    if (result.has_value()) AssertValidPubKey(*result);
    if (expected.has_value()) assert(result == *expected);
    // Without a witness, no P2WPKH, P2SH-P2WPKH or P2TR input yields a key.
    int version;
    std::vector<unsigned char> program;
    if (spk.IsPayToScriptHash() || spk.IsWitnessProgram(version, program)) {
        CTxIn no_witness{txin};
        no_witness.scriptWitness = CScriptWitness{};
        assert(!GetPubKeyFromInput(no_witness, spk).has_value());
    }
}

//! For any set of eligible inputs and recipients, every receiver must find exactly the outputs
//! created for it, with the right label, and nothing else.
FUZZ_TARGET(bip352_send_receive, .init = initialize_bip352)
{
    FuzzedDataProvider fdp{buffer.data(), buffer.size()};

    // Inputs: P2WPKH (plain keys) and P2TR key path (taproot keys).
    std::vector<CKey> plain_keys;
    std::vector<KeyPair> taproot_keys;
    std::vector<CTxIn> vin;
    std::map<COutPoint, Coin> coins;
    // Optionally spend only keys k and n - k, whose public keys cancel out.
    const bool cancelling{fdp.ConsumeBool()};
    std::optional<CKey> cancel_key;
    const int n_inputs{cancelling ? 2 : fdp.ConsumeIntegralInRange<int>(1, 4)};
    for (int i = 0; i < n_inputs; ++i) {
        std::optional<CKey> key;
        if (cancelling && i == 1) {
            key = NegateKey(*cancel_key);
        } else {
            key = ConsumeValidKey(fdp);
        }
        if (!key) return;
        if (cancelling && i == 0) cancel_key = key;
        const COutPoint outpoint{Txid::FromUint256(ConsumeUInt256(fdp)), fdp.ConsumeIntegral<uint32_t>()};
        // Real transactions never spend the null outpoint, and the sender requires a non-null one.
        if (outpoint.IsNull() || coins.contains(outpoint)) return;
        CTxIn txin{outpoint};
        if (cancelling || fdp.ConsumeBool()) {
            const CPubKey pubkey{key->GetPubKey()};
            txin.scriptWitness = CScriptWitness{{std::vector<unsigned char>(64, 0), ToByteVector(pubkey)}};
            coins[outpoint] = Coin{CTxOut{{}, GetScriptForDestination(WitnessV0KeyHash{pubkey})}, 0, false};
            plain_keys.push_back(*key);
        } else {
            // Untweaked key pair: the output key is the key itself, as in the BIP352 test vectors.
            const KeyPair keypair{key->ComputeKeyPair(/*merkle_root=*/nullptr)};
            const XOnlyPubKey output_key{key->GetPubKey()};
            txin.scriptWitness = CScriptWitness{{std::vector<unsigned char>(64, 0)}};
            coins[outpoint] = Coin{CTxOut{{}, GetScriptForDestination(WitnessV1Taproot{output_key})}, 0, false};
            taproot_keys.push_back(keypair);
        }
        vin.push_back(txin);
    }
    // An ineligible input (here a P2WSH spend) contributes no key but still counts for the
    // smallest outpoint.
    if (!cancelling && fdp.ConsumeBool()) {
        const COutPoint outpoint{Txid::FromUint256(ConsumeUInt256(fdp)), fdp.ConsumeIntegral<uint32_t>()};
        if (outpoint.IsNull() || coins.contains(outpoint)) return;
        CTxIn txin{outpoint};
        txin.scriptWitness = CScriptWitness{{ConsumeRandomLengthByteVector(fdp, 80), ConsumeRandomLengthByteVector(fdp, 80)}};
        coins[outpoint] = Coin{CTxOut{{}, GetScriptForDestination(WitnessV0ScriptHash{ConsumeUInt256(fdp)})}, 0, false};
        // A transaction without any eligible input is not eligible.
        const auto lone{GetSilentPaymentsPrevoutsSummary({txin}, coins)};
        assert(!lone.has_value() && lone.error() == PrevoutsSummaryError::NOT_ELIGIBLE);
        vin.insert(vin.begin() + fdp.ConsumeIntegralInRange<size_t>(0, vin.size()), txin);
    }
    // BIP352: the lexicographically smallest serialized outpoint, among all inputs.
    const auto serialize{[](const COutPoint& outpoint) {
        DataStream stream;
        stream << outpoint;
        return std::vector<std::byte>(stream.begin(), stream.end());
    }};
    const COutPoint smallest{std::ranges::min_element(vin, {}, [&](const CTxIn& txin) { return serialize(txin.prevout); })->prevout};

    // Receivers, each with a base address, labeled addresses and the change address.
    struct Receiver {
        CKey scan_key;
        CKey spend_key;
    };
    std::vector<Receiver> receivers;
    std::set<CPubKey> seen_scan;
    const int n_receivers{fdp.ConsumeIntegralInRange<int>(1, 3)};
    for (int r = 0; r < n_receivers; ++r) {
        const auto scan_key{ConsumeValidKey(fdp)};
        const auto spend_key{ConsumeValidKey(fdp)};
        if (!scan_key || !spend_key) return;
        // A scan key belongs to a single receiver; a receiver uses labels for more spend keys.
        if (!seen_scan.insert(scan_key->GetPubKey()).second) return;
        receivers.push_back({*scan_key, *spend_key});
    }
    std::vector<std::unique_ptr<SilentPaymentsReceiver>> scanners;
    std::vector<SilentPaymentsLabel> change_labels;
    for (const auto& r : receivers) {
        scanners.push_back(std::make_unique<SilentPaymentsReceiver>(r.scan_key, r.spend_key.GetPubKey()));
        // A new receiver knows only its change label (m = 0).
        assert(scanners.back()->GetLabels().size() == 1);
        change_labels.push_back(scanners.back()->GetLabels().begin()->first);
    }

    std::map<size_t, SilentPaymentsDestination> sp_dests;
    // Output index -> (receiver, label used); std::nullopt means the base address.
    std::map<size_t, std::pair<size_t, std::optional<SilentPaymentsLabel>>> expected;
    std::map<std::pair<size_t, uint32_t>, SilentPaymentsLabel> labels_by_m;
    const size_t n_outputs{fdp.ConsumeIntegralInRange<size_t>(1, 6)};
    for (size_t i = 0; i < n_outputs; ++i) {
        const size_t r{fdp.ConsumeIntegralInRange<size_t>(0, receivers.size() - 1)};
        auto& scanner{*scanners[r]};
        switch (fdp.ConsumeIntegralInRange<int>(0, 2)) {
        case 0:
            sp_dests.emplace(i, SilentPaymentsDestination::From(receivers[r].scan_key.GetPubKey(), receivers[r].spend_key.GetPubKey()).value());
            expected.emplace(i, std::make_pair(r, std::nullopt));
            break;
        case 1: {
            const uint32_t m{fdp.ConsumeIntegralInRange<uint32_t>(1, 3)};
            const auto before{scanner.GetLabels()};
            sp_dests.emplace(i, scanner.GenerateLabeledAddress(m));
            if (!labels_by_m.contains({r, m})) {
                std::optional<SilentPaymentsLabel> label;
                for (const auto& [l, tweak] : scanner.GetLabels()) {
                    if (!before.contains(l)) label = l;
                }
                assert(label.has_value());
                labels_by_m.emplace(std::make_pair(r, m), *label);
            } else {
                // Generating the same label twice must not add a new one.
                assert(scanner.GetLabels().size() == before.size());
            }
            expected.emplace(i, std::make_pair(r, std::optional<SilentPaymentsLabel>{labels_by_m.at({r, m})}));
            break;
        }
        default:
            sp_dests.emplace(i, scanner.GetChangeDestination());
            expected.emplace(i, std::make_pair(r, std::optional<SilentPaymentsLabel>{change_labels[r]}));
            break;
        }
    }

    const auto summary{GetSilentPaymentsPrevoutsSummary(vin, coins)};
    if (cancelling) {
        assert(!summary.has_value() && summary.error() == PrevoutsSummaryError::NOT_ELIGIBLE);
    }
    {
        // A missing coin is reported as such, and spending a future witness version makes the
        // whole transaction ineligible.
        auto missing{coins};
        missing.erase(vin[fdp.ConsumeIntegralInRange<size_t>(0, vin.size() - 1)].prevout);
        const auto missing_summary{GetSilentPaymentsPrevoutsSummary(vin, missing)};
        assert(!missing_summary.has_value() && missing_summary.error() == PrevoutsSummaryError::MISSING_COIN);

        const unsigned int version{fdp.ConsumeIntegralInRange<unsigned int>(2, 16)};
        auto future_coins{coins};
        auto future_vin{vin};
        const COutPoint future_outpoint{vin.back().prevout.hash, vin.back().prevout.n ^ 1};
        if (!future_outpoint.IsNull() && !coins.contains(future_outpoint)) {
            const CScript future_spk{GetScriptForDestination(WitnessUnknown{version, std::vector<unsigned char>(32, 1)})};
            future_coins[future_outpoint] = Coin{CTxOut{{}, future_spk}, 0, false};
            future_vin.insert(future_vin.begin() + fdp.ConsumeIntegralInRange<size_t>(0, future_vin.size()), CTxIn{future_outpoint});
            const auto future_summary{GetSilentPaymentsPrevoutsSummary(future_vin, future_coins)};
            assert(!future_summary.has_value() && future_summary.error() == PrevoutsSummaryError::NOT_ELIGIBLE);
        }
    }
    const auto tr_dests{GenerateSilentPaymentsTaprootDestinations(sp_dests, plain_keys, taproot_keys, smallest)};
    // The sender and the receiver must agree on whether the input keys are usable.
    assert(summary.has_value() == tr_dests.has_value());
    if (!tr_dests) return;

    std::vector<XOnlyPubKey> tx_outputs;
    for (const auto& [_, dest] : *tr_dests) tx_outputs.emplace_back(dest);
    // All generated outputs must be distinct.
    assert(std::set<XOnlyPubKey>(tx_outputs.begin(), tx_outputs.end()).size() == tr_dests->size());
    // Unrelated outputs, including ones that are not valid x-only keys, must be ignored.
    while (fdp.ConsumeBool() && tx_outputs.size() < 10) {
        const auto pos{tx_outputs.begin() + fdp.ConsumeIntegralInRange<size_t>(0, tx_outputs.size())};
        if (fdp.ConsumeBool()) {
            // x = 0 is not on the curve.
            tx_outputs.insert(pos, XOnlyPubKey{std::vector<unsigned char>(32, 0)});
        } else if (const auto key{ConsumeValidKey(fdp)}) {
            tx_outputs.insert(pos, XOnlyPubKey{key->GetPubKey()});
        }
    }

    for (size_t r = 0; r < receivers.size(); ++r) {
        // A transaction whose outputs are all invalid x-only keys has nothing to find.
        const auto none{scanners[r]->Scan(*summary, {XOnlyPubKey{std::vector<unsigned char>(32, 0)}})};
        assert(none.has_value() && none->empty());
        const auto found{scanners[r]->Scan(*summary, tx_outputs)};
        std::map<XOnlyPubKey, std::optional<SilentPaymentsLabel>> want;
        for (const auto& [i, owner] : expected) {
            if (owner.first == r) want.emplace(XOnlyPubKey{tr_dests->at(i)}, owner.second);
        }
        // Finding nothing is not an error, and no output is reported twice.
        assert(found.has_value());
        std::set<XOnlyPubKey> found_keys;
        for (const auto& output : *found) found_keys.insert(output.output);
        assert(found_keys.size() == found->size());
        if (want.empty()) {
            assert(found->empty());
            continue;
        }
        assert(found->size() == want.size());
        for (const auto& output : *found) {
            const auto it{want.find(output.output)};
            assert(it != want.end());
            assert(output.label.has_value() == it->second.has_value());
            if (output.label) assert(*output.label == *it->second);
            // The returned tweak must let the receiver spend: spend key + tweak is the output key.
            assert(XOnlyPubKey{TweakKey(receivers[r].spend_key, output.tweak).GetPubKey()} == output.output);
        }
    }
}
