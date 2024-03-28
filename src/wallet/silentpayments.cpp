#include <wallet/silentpayments.h>
#include <addresstype.h>
#include <arith_uint256.h>
#include <bip352.h>
#include <coins.h>
#include <crypto/common.h>
#include <crypto/hmac_sha512.h>
#include <key_io.h>
#include <undo.h>
#include <logging.h>
#include <pubkey.h>
#include <policy/policy.h>
#include <script/sign.h>
#include <script/solver.h>
#include <util/check.h>

namespace wallet {

std::map<size_t, WitnessV1Taproot> GenerateSilentPaymentTaprootDestinations(const BIP352::PrivTweakData& tweak_data, const std::map<size_t, V0SilentPaymentDestination>& sp_dests)
{
    std::map<CPubKey, std::vector<std::pair<CPubKey, size_t>>> sp_groups;
    std::map<size_t, WitnessV1Taproot> tr_dests;

    for (const auto& [out_idx, sp_dest] : sp_dests) {
        sp_groups[sp_dest.m_scan_pubkey].emplace_back(sp_dest.m_spend_pubkey, out_idx);
    }

    for (const auto& [scan_pubkey, spend_pubkeys] : sp_groups) {
        auto shared_secret = BIP352::CreateSharedSecret(tweak_data, scan_pubkey);
        for (size_t k = 0; k < spend_pubkeys.size(); ++k) {
            const auto& [spend_pubkey, out_idx] = spend_pubkeys.at(k);
            tr_dests.emplace(out_idx, BIP352::CreateOutput(shared_secret, spend_pubkey, k));
        }
    }
    return tr_dests;
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
                if (std::equal(NUMS_H.begin(), NUMS_H.end(), control.begin() + 1)) {
                    // Skip script path with H internal key
                    return std::nullopt;
                }
            }
        }

        pubkey = XOnlyPubKey{solutions[0]};
    } else if (type == TxoutType::WITNESS_V0_KEYHASH) {
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

std::optional<BIP352::PubTweakData> GetSilentPaymentTweakDataFromTxInputs(const std::vector<CTxIn>& vin, const std::map<COutPoint, Coin>& coins)
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
    auto smallest_outpoint = std::min_element(tx_outpoints.begin(), tx_outpoints.end());
    return BIP352::CreateInputPubkeysTweak(pubkeys, xonly_pubkeys, *smallest_outpoint);
}

std::optional<std::vector<uint256>> GetTxOutputTweaks(const BIP352::SharedSecret& shared_secret, CPubKey& spend_pubkey, std::vector<XOnlyPubKey> output_pub_keys, const std::map<CPubKey, uint256>& labels)
{
    // Because a sender can create multiple outputs for us, we first check the outputs vector for an output with
    // output index 0. If we find it, we remove it from the vector and then iterate over the vector again looking for
    // an output with index 1, and so on until one of the following happens:
    //
    //     1. We have determined all outputs belong to us (the vector is empty)
    //     2. We have passed over the vector and found no outputs belonging to us
    //

    bool keep_going;
    uint32_t k{0};
    std::vector<uint256> tweaks;
    do {
        // We haven't found anything yet on this pass and if we make to the end without finding any
        // silent payment outputs everything left in the vector is not for us, so we stop scanning.
        keep_going = false;

        // Scan the transaction outputs, only continue scanning if there is a match
        output_pub_keys.erase(std::remove_if(output_pub_keys.begin(), output_pub_keys.end(), [&](XOnlyPubKey output_pubkey) {
            BIP352::ScanningResult result = BIP352::CheckOutput(shared_secret, spend_pubkey, output_pubkey, k);
            if (!result.found) {
                auto it = labels.find(result.label);
                if (it == labels.end()) {
                    it = labels.find(result.label_negated);
                }
                if (it != labels.end()) {
                    result.t_k = BIP352::CombineTweaks(result.t_k, it->second);
                    result.found = true;
                }
            }
            if (result.found) {
                // Since we found an output, we need to increment k and check the vector again
                tweaks.emplace_back(result.t_k);
                keep_going = true;
                k++;
                // Return true so that this output pubkey is removed the from vector and not checked again
                return true;
            }
            return false;
        }), output_pub_keys.end());
    } while (!output_pub_keys.empty() && keep_going);
    if (tweaks.empty()) return std::nullopt;
    return tweaks;
}
}
