#ifndef BITCOIN_WALLET_SILENTPAYMENTS_H
#define BITCOIN_WALLET_SILENTPAYMENTS_H

#include <addresstype.h>
#include <bip352.h>
#include <coins.h>
#include <key_io.h>
#include <undo.h>

namespace wallet {

using PubKey = std::variant<CPubKey, XOnlyPubKey>;

/**
 * @brief Generate a silent payment labeled address.
 *
 * @param receiver The receiver's silent payment destination (i.e. scan and spend public keys).
 * @param label The label tweak
 * @return V0SilentPaymentDestination The silent payment destination, with `B_spend -> B_spend + label * G`.
 *
 * @see ComputeSilentPaymentLabelTweak(const CKey& scan_key, const int m);
 */
V0SilentPaymentDestination GenerateSilentPaymentLabeledAddress(const V0SilentPaymentDestination& receiver, const uint256& label);

/**
 * @brief Generate silent payment taproot destinations.
 *
 * Given a set of silent payment destinations, generate the requested number of outputs. If a silent payment
 * destination is repeated, this indicates multiple outputs are requested for the same recipient. The silent payment
 * desintaions are passed in map where the key indicates their desired position in the final tx.vout array.
 *
 * @param priv_tweak_data                     The private tweak data. This is a pair containing a private key and input hash.
 *                                            The private key is either the receiver's scan key or the sum of the sender's
 *                                            input private keys (excluding inputs not eligible for shared secret derivation).
 *                                            The input hash is the tagged hash of hash of the smallest outpoint and sum
 *                                            of the eligible input public keys.
 * @param sp_dests                            The silent payment destinations.
 * @return std::map<size_t, WitnessV1Taproot> The generated silent payment taproot destinations.
 *
 */
std::map<size_t, WitnessV1Taproot> GenerateSilentPaymentTaprootDestinations(const BIP352::PrivTweakData& priv_tweak_data, const std::map<size_t, V0SilentPaymentDestination>& sp_dests);

/**
 * @brief Get silent payment tweak data from transaction inputs.
 *
 * Get the necessary data from the transaction inputs to be able to scan the transaction outputs for silent payment outputs.
 * This requires knowledge of the prevout scriptPubKey, which is passed via `coins`.
 *
 * If the transaction has silent payment eligible inputs, the public keys from these inputs are returned as a sum along with
 * the transaction input hash. If there are no eligible inputs, nullopt is returned, indicating that this transaction does not
 * contain silent payment outputs.
 *
 * @param vin                                              The transaction inputs.
 * @param coins                                            The coins (potentially) spent in this transaction.
 * @return std::optional<std::pair<uint256, PubTweakData>> The silent payment tweak data, or nullopt if not found.
 */
std::optional<BIP352::PubTweakData> GetSilentPaymentTweakDataFromTxInputs(const std::vector<CTxIn>& vin, const std::map<COutPoint, Coin>& coins);

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
 * @brief Get the silent payment output tweaks from a transaction.
 *
 * Scan the transaction for silent payment outputs intendended for us. The shared secret tweak is returned
 * for each output found. This tweak data is needed to spend the output, by adding it to the spend secret key.
 * If no tweak data is returned, this transaction does not contain silent payment outputs intended for us.
 *
 * While labels are optional for the receiver, it is strongly recommended that the change label is always checked
 * when scanning.
 *
 * @param spend_pubkey          The receiver's spend public key.
 * @param shared_secret         The ECDH public key.
 * @param output_pub_keys       The taproot output public keys.
 * @param labels                The receiver's labels.
 * @return std::vector<uint256> The transaction output tweaks.
 *
 */
std::optional<std::vector<uint256>> GetTxOutputTweaks(const BIP352::SharedSecret& shared_secret, CPubKey& spend_pubkey, std::vector<XOnlyPubKey> output_pub_keys, const std::map<CPubKey, uint256>& labels);
} // namespace wallet
#endif // BITCOIN_WALLET_SILENTPAYMENTS_H

