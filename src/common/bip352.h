// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COMMON_BIP352_H
#define BITCOIN_COMMON_BIP352_H

#include <addresstype.h>
#include <attributes.h>
#include <compat/byteswap.h>
#include <key.h>
#include <primitives/transaction.h>
#include <pubkey.h>
#include <uint256.h>

#include <array>
#include <compare>
#include <cstdint>
#include <cstring>
#include <map>
#include <memory>
#include <optional>
#include <span>
#include <variant>
#include <vector>

struct secp256k1_silentpayments_label;
struct secp256k1_silentpayments_prevouts_summary;
class CScript;
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

    secp256k1_silentpayments_prevouts_summary* Get() const LIFETIMEBOUND;
};

struct BIP352Comparator {
    bool operator()(const COutPoint& a, const COutPoint& b) const {
        // BIP352 defines the "smallest outpoint" based on a lexicographic
        // sort of the outpoints, using the 36-byte serialization:
        // <txid, 32-bytes little-endian>:<vout, 4-bytes little-endian>
        if (a.hash != b.hash) return a.hash < b.hash;
        return internal_bswap_32(a.n) < internal_bswap_32(b.n);
    }
};

class SilentPaymentsLabel {
private:
    std::unique_ptr<secp256k1_silentpayments_label> m_label;
    unsigned char m_vch[CPubKey::COMPRESSED_SIZE];

    //! private constructor only meant to be used by
    //! LabelLookupCallback. It does not parse the label
    //! bytes to avoid EC-point parse. This is safe because
    //! the resulting object is only used for comparison.
    SilentPaymentsLabel(const unsigned char* label);

    //! Parses raw bytes into a fully valid label
    //! returns std::nullopt if vch is not a validly-encoded label.
    static std::optional<SilentPaymentsLabel> FromBytes(std::span<const unsigned char, CPubKey::COMPRESSED_SIZE> vch);

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
    static std::optional<SilentPaymentsLabel> Unserialize(Stream& s)
    {
        std::array<unsigned char, CPubKey::COMPRESSED_SIZE> vch;
        s >> std::span{vch};
        return FromBytes(std::span{vch});
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
 * @pre smallest_outpoint is not null, and at least one of plain_keys or taproot_keys is non-empty.
 * @return The generated silent payments taproot destinations or std::nullopt if the set of provided inputs is invalid
 *         (there is an invalid key in the set or the inputs sum to zero).
 */
std::optional<std::map<size_t, WitnessV1Taproot>> GenerateSilentPaymentsTaprootDestinations(const std::map<size_t, SilentPaymentsDestination>& sp_dests, const std::vector<CKey>& plain_keys, const std::vector<KeyPair>& taproot_keys, const COutPoint& smallest_outpoint);

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
 * @brief A silent payments recipient's scanning identity.
 *
 * Bundles a recipient's scan key, spend public key, and known labels (always including the change
 * label) into a single self-contained object capable of scanning transactions and deriving the
 * recipient's change destination.
 *
 * The change label is created automatically on construction if one is not supplied through
 * the labels map; additional labels are registered on creation with GenerateLabeledAddress().
 */
class SilentPaymentsReceiver {
private:
    using LabelTweakMap = std::map<SilentPaymentsLabel, uint256>;

    CKey m_scan_key;
    CPubKey m_spend_pubkey;
    LabelTweakMap m_labels;
    LabelTweakMap::const_iterator m_change_it;

    SilentPaymentsDestination BuildLabeledDestination(const SilentPaymentsLabel& label) const;

public:
    SilentPaymentsReceiver(const CKey& scan_key, const CPubKey& spend_pubkey, const LabelTweakMap& labels = {});

    /**
     * @brief Get this recipient's registered labels, including the change label.
     *
     * @return const LabelTweakMap& Each label mapped to its scalar tweak.
     */
    const LabelTweakMap& GetLabels() const;

    /**
     * @brief Register label `m` (e.g. for a labeled sub-address to hand out to a payer) and generate
     * the resulting address.
     *
     * Derives the label from this recipient's own scan key and `m`, registers it so Scan() can
     * recognize outputs sent to it, and returns the address to hand out.
     *
     * @param m                        An integer m, greater than 0 (m = 0 is reserved for the change label).
     * @return SilentPaymentsDestination The labeled destination, with `B_spend -> B_spend + label`.
     */
    SilentPaymentsDestination GenerateLabeledAddress(uint32_t m);

    /**
     * @brief Get this recipient's silent payments change destination.
     *
     * @return SilentPaymentsDestination The destination to use for this recipient's own change outputs,
     *         i.e. `B_spend -> B_spend + change_label`.
     */
    SilentPaymentsDestination GetChangeDestination() const;

    /**
     * @brief Scan a transaction for silent payments outputs.
     *
     * Scan the transaction for silent payments outputs intended for this recipient. The output, shared
     * secret tweak, and (optionally) label public key is returned for each output found. If the output
     * was sent to a labeled address, the label tweak is added to the shared secret tweak. The shared
     * secret tweak is needed to spend the output, by adding it to the spend secret key. If no outputs
     * are found, this transaction does not contain silent payments outputs for this recipient.
     *
     * @param prevouts_summary                   The silent payments public data.
     * @param tx_outputs                         The taproot output public keys.
     * @return The found outputs or std::nullopt in the case of an error.
     */
    std::optional<std::vector<SilentPaymentsOutput>> Scan(const PrevoutsSummary& prevouts_summary, const std::vector<XOnlyPubKey>& tx_outputs) const;
};
}; // namespace bip352
#endif // BITCOIN_COMMON_BIP352_H
