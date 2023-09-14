#ifndef BITCOIN_WALLET_SILENTPAYMENTS_H
#define BITCOIN_WALLET_SILENTPAYMENTS_H

#include <addresstype.h>
#include <coins.h>
#include <key_io.h>
#include <undo.h>

namespace wallet {
CPubKey SumInputPubKeys(const std::vector<CPubKey>& input_pubkeys); // For the unit tests
uint256 GenerateSilentPaymentChangeTweak(const CKey& scan_key);
V0SilentPaymentDestination GenerateSilentPaymentLabeledAddress(const V0SilentPaymentDestination& receiver, const uint256& label);
CPubKey ComputeECDHSharedSecret(const CKey& scan_key, const CPubKey& sender_public_key, const uint256& outpoints_hash);
uint256 ComputeTweak(const CPubKey& ecdh_pubkey, const uint32_t output_index);
std::vector<uint256> GetTxOutputTweaks(const CPubKey& spend_pubkey, const CPubKey& ecdh_pubkey, std::vector<XOnlyPubKey> output_pub_keys, const std::map<CPubKey, uint256>& labels);
std::optional<std::pair<uint256, CPubKey>> GetSilentPaymentsTweakDataFromTxInputs(const std::vector<CTxIn>& vin, const std::map<COutPoint, Coin>& coins);

CPubKey CreateOutput(const CKey& ecdh_scalar, const CPubKey& scan_pubkey, const CPubKey& spend_pubkey, const uint32_t output_index);
uint256 HashOutpoints(const std::vector<COutPoint>& tx_outpoints);
std::map<size_t, WitnessV1Taproot> GenerateSilentPaymentTaprootDestinations(const CKey& ecdh_scalar, const std::map<size_t, V0SilentPaymentDestination>& sp_dests);
CKey PrepareScalarECDHInput(const std::vector<std::pair<CKey, bool>>& sender_secret_keys, const std::vector<COutPoint>& tx_outpoints);
} // namespace wallet
#endif // BITCOIN_WALLET_SILENTPAYMENTS_H
