// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COMMON_BIP352_H
#define BITCOIN_COMMON_BIP352_H

#include <key.h>
#include <pubkey.h>
#include <primitives/transaction.h>
#include <uint256.h>
#include <addresstype.h>
#include <coins.h>

#include <vector>
#include <variant>

struct secp256k1_silentpayments_recipient_prevouts_summary;
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

    secp256k1_silentpayments_recipient_prevouts_summary* Get() const;
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

struct SilentPaymentOutput {
    XOnlyPubKey output;
    uint256 tweak;
    std::optional<CPubKey> label;
};

/**
 * @brief Get the public key from an input.
 *
 * Get the public key from a silent payment eligible input. This requires knowledge of the prevout
 * scriptPubKey to determine the type of input and whether or not it is eligible for silent payments.
 *
 * If the input is not eligible for silent payments, the input is skipped (indicated by returning a nullopt).
 *
 * @param txin                    The transaction input.
 * @param spk                     The scriptPubKey of the prevout.
 * @return std::optional<CPubKey> The public key, or nullopt if not found.
 */
std::optional<PubKey> GetPubKeyFromInput(const CTxIn& txin, const CScript& spk);

/**
 * @brief Generate silent payment taproot destinations.
 *
 * Given a set of silent payment destinations, generate the requested number of outputs. If a silent payment
 * destination is repeated, this indicates multiple outputs are requested for the same recipient. The silent payment
 * desintaions are passed in map where the key indicates their desired position in the final tx.vout array.
 *
 * @param sp_dests                            The silent payment destinations.
 * @param plain_keys                          The private keys for non-taproot inputs.
 * @param taproot_keys                        The keypairs for taproot inputs.
 * @param smallest_outpoint                   The smallest_outpoint from the transaction inputs.
 * @return std::map<size_t, WitnessV1Taproot> The generated silent payment taproot destinations.
 */
std::optional<std::map<size_t, WitnessV1Taproot>> GenerateSilentPaymentTaprootDestinations(const std::map<size_t, V0SilentPaymentDestination>& sp_dests, const std::vector<CKey>& plain_keys, const std::vector<KeyPair>& taproot_keys, const COutPoint& smallest_outpoint);

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
 * @return std::<CPubKey, uint156> The label public key and label tweak.
 *
 * @see GenerateSilentPaymentLabeledAddress
 */
std::pair<CPubKey, uint256> CreateLabelTweak(const CKey& scan_key, const int m);

/**
 * @brief Generate a silent payment labeled address.
 *
 * @param recipient                   The recipient's silent payment destination (i.e. scan and spend public keys).
 * @param label                       The label tweak
 * @return V0SilentPaymentDestination The silent payment destination, with `B_spend -> B_spend + label * G`.
 *
 * @see CreateLabelTweak(const CKey& scan_key, const int m);
 */
V0SilentPaymentDestination GenerateSilentPaymentLabeledAddress(const V0SilentPaymentDestination& recipient, const uint256& label);

/**
 * @brief Get silent payment public data from transaction inputs.
 *
 * Get the necessary data from the transaction inputs to be able to scan the transaction outputs for silent payment outputs.
 * This requires knowledge of the prevout scriptPubKey, which is passed via `coins`.
 *
 * This function returns the public key sum and the input hash separately and is intended to be used by the wallet when scanning
 * a transaction. To get the silent payments public data for sending to light clients or saving in an index, use
 * `GetSerializedSilentPaymentsPrevoutsSummary`.
 *
 * If there are no eligible inputs, nullopt is returned, indicating that this transaction does not
 * contain silent payment outputs.
 *
 * @param vin                        The transaction inputs.
 * @param coins                      The coins (potentially) spent in this transaction.
 * @return std::optional<PrevoutsSummary> The silent payment public data, nullopt if not found.
 */
std::optional<PrevoutsSummary> GetSilentPaymentsPrevoutsSummary(const std::vector<CTxIn>& vin, const std::map<COutPoint, Coin>& coins);

/**
 * @brief Get and serialize public data from the transaction inputs.
 *
 * This function returns the public key sum multiplied by the input hash and is intended to be used by an index store the
 * input data for wallet rescans or for serving the data to light clients. To get the silent payments public data for
 * scanning a transaction, use `GetSilentPaymentsPrevoutsSummary`.
 *
 * If there are no eligible inputs, nullopt is returned, indicating that this transaction does not
 * contain silent payment outputs.
 *
 * @param vin                          The transaction inputs.
 * @param coins                        The coins spent in this transaction.
 * @return std::optional<PrevoutsSummary> The silent payment tweak data, nullopt if not found.
 */
std::optional<CPubKey> GetSerializedSilentPaymentsPrevoutsSummary(const std::vector<CTxIn>& vin, const std::map<COutPoint, Coin>& coins);

/**
 * @brief Scan a transaction for silent payment outputs.
 *
 * Scan the transaction for silent payment outputs intendended for the recipient (i.e. the owner of `scan_key`).
 * The output, shared secret tweak, and (optionally) label public key is returned for each output found. If the
 * output was sent to a labeled address, the label tweak is added to the shared secret tweak.
 * The shared secret tweak is needed to spend the output, by adding it to the spend secret key.
 * If no outputs are found, this transaction does not contain silent payment outputs for the recipient.
 *
 * While labels are optional for the recipient, it is strongly recommended that the change label is always checked
 * when scanning.
 *
 * @param scan_key                                          The recipient's scan key.
 * @param spend_pubkey                                      The recipient's spend public key.
 * @param output_pub_keys                                   The taproot output public keys.
 * @param labels                                            The recipient's labels.
 * @return std::<optional<std::vector<SilentPaymentOutput>> The found outputs, nullopt if none found.
 */
std::optional<std::vector<SilentPaymentOutput>> ScanForSilentPaymentOutputs(const CKey& scan_key, const PrevoutsSummary& prevouts_summary, const CPubKey& spend_pubkey, const std::vector<XOnlyPubKey>& output_pub_keys, const std::map<CPubKey, uint256>& labels);
}; // namespace bip352
#endif // BITCOIN_COMMON_BIP352_H
