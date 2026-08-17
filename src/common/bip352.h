// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COMMON_BIP352_H
#define BITCOIN_COMMON_BIP352_H

#include <addresstype.h>
#include <crypto/common.h>
#include <primitives/transaction.h>
#include <pubkey.h>
#include <uint256.h>

#include <compare>
#include <cstdint>
#include <cstring>
#include <map>
#include <memory>
#include <optional>
#include <span>
#include <utility>
#include <variant>
#include <vector>

struct secp256k1_silentpayments_label;
struct secp256k1_silentpayments_prevouts_summary;
class CKey;
class CScript;
class KeyPair;
class Coin;

namespace bip352 {

using PubKey = std::variant<CPubKey, XOnlyPubKey>;

class PrevoutsSummaryImpl;
class PrevoutsSummary
{
private:
    std::unique_ptr<PrevoutsSummaryImpl> m_impl;

public:
    PrevoutsSummary();
    PrevoutsSummary(PrevoutsSummary&&) noexcept;
    PrevoutsSummary& operator=(PrevoutsSummary&&) noexcept;
    ~PrevoutsSummary();

    // Delete copy constructors
    PrevoutsSummary(const PrevoutsSummary&) = delete;
    PrevoutsSummary& operator=(const PrevoutsSummary&) = delete;

    secp256k1_silentpayments_prevouts_summary* Get() const;
};

struct BIP352Comparator {
    bool operator()(const COutPoint& a, const COutPoint& b) const {
        // BIP352 defines the "smallest outpoint" based on a lexicographic
        // sort of the outpoints, using the 36-byte serialization:
        // <txid, 32-bytes little-endian>:<vout, 4-bytes little-endian>
        if (a.hash != b.hash) {
            return a.hash < b.hash;
        }
        uint8_t a_vout[4], b_vout[4];
        WriteLE32(a_vout, a.n);
        WriteLE32(b_vout, b.n);
        return std::memcmp(a_vout, b_vout, 4) < 0;
    }
};

class SilentPaymentsLabel {
private:
    std::unique_ptr<secp256k1_silentpayments_label> m_label;
    unsigned char m_vch[CPubKey::COMPRESSED_SIZE];

    //! private constructor only meant to be used by
    //! LabelLookupCallback. It does not parse the label
    //! bytes to avoid EC-point parse. This is safe because
    //!the resulting object is only used for comparison.
    SilentPaymentsLabel(const unsigned char* label);

    //! Parses raw bytes into a fully valid label, throws
    //! std::ios_base::failure if vch33 is not a validly-encoded label.
    static SilentPaymentsLabel FromBytes(const unsigned char* vch33);

public:
    SilentPaymentsLabel(const secp256k1_silentpayments_label& label);

    SilentPaymentsLabel(SilentPaymentsLabel&&) noexcept;
    SilentPaymentsLabel& operator=(SilentPaymentsLabel&&) noexcept;
    SilentPaymentsLabel(const SilentPaymentsLabel&);
    SilentPaymentsLabel& operator=(const SilentPaymentsLabel&);

    ~SilentPaymentsLabel();

    friend const unsigned char* LabelLookupCallback(const unsigned char* key, const void* context);

    friend bool operator==(const SilentPaymentsLabel& a, const SilentPaymentsLabel& b) {
        return memcmp(a.m_vch,  b.m_vch, CPubKey::COMPRESSED_SIZE) == 0;
    }
    friend bool operator<(const SilentPaymentsLabel& a, const SilentPaymentsLabel& b) {
        return memcmp(a.m_vch,  b.m_vch, CPubKey::COMPRESSED_SIZE) < 0;
    }
    friend bool operator>(const SilentPaymentsLabel& a, const SilentPaymentsLabel& b) {
        return b < a;
    }

    template <typename Stream>
    void Serialize(Stream& s) const
    {
        s << std::span{m_vch, CPubKey::COMPRESSED_SIZE};
    }

    template <typename Stream>
    static SilentPaymentsLabel Unserialize(Stream& s)
    {
        unsigned char vch[CPubKey::COMPRESSED_SIZE];
        s >> std::span{vch, CPubKey::COMPRESSED_SIZE};
        return FromBytes(vch);
    }

    const secp256k1_silentpayments_label* Get() const;
};

struct SilentPaymentsOutput {
    XOnlyPubKey output;
    uint256 tweak;
    std::optional<SilentPaymentsLabel> label;
};

/**
 * @brief Get the public key from an input.
 *
 * Get the public key from a silent payments eligible input. This requires knowledge of the prevout
 * scriptPubKey to determine the type of input and whether or not it is eligible for silent payments.
 *
 * If the input is not eligible for silent payments, the input is skipped (indicated by returning a nullopt).
 *
 * @param txin                    The transaction input.
 * @param spk                     The scriptPubKey of the prevout.
 * @return The public key, or nullopt if not found.
 */
std::optional<PubKey> GetPubKeyFromInput(const CTxIn& txin, const CScript& spk);

/**
 * @brief Generate silent payments taproot destinations.
 *
 * Given a set of silent payments destinations, generate the requested number of outputs. If a silent payment
 * destination is repeated, this indicates multiple outputs are requested for the same recipient. The silent payment
 * destinations are passed in a map where the key indicates their desired position in the final tx.vout array.
 *
 * @param sp_dests                            The silent payments destinations.
 * @param plain_keys                          The private keys for non-taproot inputs.
 * @param taproot_keys                        The keypairs for taproot inputs.
 * @param smallest_outpoint                   The smallest_outpoint from the transaction inputs.
 * @return std::map<size_t, WitnessV1Taproot> The generated silent payments taproot destinations.
 */
std::optional<std::map<size_t, WitnessV1Taproot>> GenerateSilentPaymentsTaprootDestinations(const std::map<size_t, V0SilentPaymentsDestination>& sp_dests, const std::vector<CKey>& plain_keys, const std::vector<KeyPair>& taproot_keys, const COutPoint& smallest_outpoint);

/**
 * @brief Create a silent payments label pair.
 *
 * Create a label tweak and the corresponding public key from an integer `m` and the recipient scan key.
 * `m = 0` is reserved for the change label.
 *
 * Label public keys can be stored in a cache, mapping the public key to the label tweak. This cache
 * is used during scanning to determine if a label was used and if so to retrieve the label tweak.
 *
 * @param scan_key                 The recipient's scan_key, used to salt the hash
 * @param m                        An integer m (only use m = 0 for the change label)
 * @return The silent payments label and label tweak.
 *
 * @see GenerateSilentPaymentsLabeledAddress
 */
std::pair<SilentPaymentsLabel, uint256> CreateLabel(const CKey& scan_key, uint32_t m);

/**
 * @brief Generate a silent payments labeled address.
 *
 * @param recipient                   The recipient's silent payments destination (i.e. scan and spend public keys).
 * @param label                       The label
 * @return V0SilentPaymentsDestination The silent payments destination, with `B_spend -> B_spend + label`.
 *
 * @see CreateLabel(const CKey& scan_key, const int m);
 */
V0SilentPaymentsDestination GenerateSilentPaymentsLabeledAddress(const V0SilentPaymentsDestination& recipient, const SilentPaymentsLabel& label);

/**
 * @brief Get silent payments public data from transaction inputs.
 *
 * Get the necessary data from the transaction inputs to be able to scan the transaction outputs for silent payments outputs.
 * This requires knowledge of the prevout scriptPubKey, which is passed via `coins`.
 *
 * This function returns the public key sum and the input hash separately and is intended to be used by the wallet when scanning
 * a transaction.
 *
 * If there are no eligible inputs, or one of the inputs spends an unknown segwit version (i.e > 1),  nullopt is returned,
 * indicating that this transaction is not eligible to be scanned for silent payments outputs.
 *
 * @param vin                        The transaction inputs.
 * @param coins                      The coins (potentially) spent in this transaction.
 * @return std::optional<PrevoutsSummary> The silent payments public data, nullopt if not found.
 */
std::optional<PrevoutsSummary> GetSilentPaymentsPrevoutsSummary(const std::vector<CTxIn>& vin, const std::map<COutPoint, Coin>& coins);

/**
 * @brief Scan a transaction for silent payments outputs.
 *
 * Scan the transaction for silent payments outputs intended for the recipient (i.e. the owner of `scan_key`).
 * The output, shared secret tweak, and (optionally) label public key is returned for each output found. If the
 * output was sent to a labeled address, the label tweak is added to the shared secret tweak.
 * The shared secret tweak is needed to spend the output, by adding it to the spend secret key.
 * If no outputs are found, this transaction does not contain silent payments outputs for the recipient.
 *
 * While labels are optional for the recipient, it is strongly recommended that the change label is always checked
 * when scanning.
 *
 * @param scan_key                                          The recipient's scan key.
 * @param prevouts_summary                                  The silent payments public data.
 * @param spend_pubkey                                      The recipient's spend public key.
 * @param tx_outputs                                        The taproot output public keys.
 * @param labels                                            The recipient's labels.
 * @return std::vector<SilentPaymentsOutput>                The found outputs.
 */
std::vector<SilentPaymentsOutput> ScanForSilentPaymentsOutputs(const CKey& scan_key, const PrevoutsSummary& prevouts_summary, const CPubKey& spend_pubkey, const std::vector<XOnlyPubKey>& tx_outputs, const std::map<SilentPaymentsLabel, uint256>& labels);
}; // namespace bip352
#endif // BITCOIN_COMMON_BIP352_H
