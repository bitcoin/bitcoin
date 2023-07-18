#include <wallet/silentpayments.h>
#include <arith_uint256.h>
#include <coins.h>
#include <crypto/common.h>
#include <crypto/hmac_sha512.h>
#include <key_io.h>
#include <undo.h>
#include <logging.h>

namespace wallet {

Recipient::Recipient(const CKey& scan_seckey, const CKey& spend_seckey)
{
    m_scan_seckey = scan_seckey;
    m_scan_pubkey = CPubKey{m_scan_seckey.GetPubKey()};
    m_spend_seckey = spend_seckey;
    m_spend_pubkey = CPubKey{m_spend_seckey.GetPubKey()};
}

void Recipient::ComputeECDHSharedSecret(const CPubKey& sender_public_key, const std::vector<COutPoint>& tx_outpoints)
{
    const auto& outpoint_hash = HashOutpoints(tx_outpoints);
    auto tweaked_scan_seckey = m_scan_seckey.MultiplyTweak(outpoint_hash.begin());
    CPubKey result = tweaked_scan_seckey.SilentPaymentECDH(sender_public_key);

    m_ecdh_pubkey = result;
    assert(m_ecdh_pubkey.IsValid());
}

std::pair<CKey,CPubKey> Recipient::CreateOutput(const uint32_t output_index) const
{
    HashWriter h;
    h.write(Span{m_ecdh_pubkey});
    unsigned char num[4];
    WriteBE32(num, output_index);
    h << num;
    uint256 shared_secret = h.GetSHA256();

    const auto& result_pubkey{m_spend_pubkey.AddTweak(shared_secret.begin())};
    const auto& result_seckey{m_spend_seckey.AddTweak(shared_secret.begin())};

    return {result_seckey, result_pubkey};
}

std::pair<CPubKey,CPubKey> Recipient::GetAddress() const
{
    return {m_scan_pubkey, m_spend_pubkey};
}

std::vector<CKey> Recipient::ScanTxOutputs(std::vector<XOnlyPubKey> output_pub_keys)
{
    // Because a sender can create multiple outputs for us, we first check the outputs vector for an output with
    // output index 0. If we find it, we remove it from the vector and then iterate over the vector again looking for
    // an output with index 1, and so on until one of the following happens:
    //
    //     1. We have determined all outputs belong to us (the vector is empty)
    //     2. We have passed over the vector and found no outputs belonging to us
    //

    // The ECDH shared secret must be set before we start scanning
    assert(m_ecdh_pubkey.IsValid());

    bool removed;
    uint32_t output_index{0};
    std::vector<CKey> raw_tr_keys;
    do {
        // We haven't removed anything yet on this pass and if we don't remove anything, we didn't find
        // any silent payment outputs and should stop checking
        removed = false;
        std::pair<CKey, CPubKey> tweakResult = CreateOutput(output_index);
        const CKey& silent_payment_priv_key = tweakResult.first;
        const XOnlyPubKey& silent_payment_pub_key = XOnlyPubKey{tweakResult.second};
        output_pub_keys.erase(std::remove_if(output_pub_keys.begin(), output_pub_keys.end(), [&](auto outputPubKey) {
            if (silent_payment_pub_key == outputPubKey) {
                // Since we found an output, we need to increment the output index and check the vector again
                raw_tr_keys.emplace_back(silent_payment_priv_key);
                removed = true;
                output_index++;
                // Return true so that this output pubkey is removed the from vector and not checked again
                return true;
            }
            return false;
        }), output_pub_keys.end());
    } while (!output_pub_keys.empty() && removed);
    return raw_tr_keys;
}

Sender::Sender(const CKey& scalar_ecdh_input, const SilentPaymentRecipient& recipient)
{
    m_ecdh_pubkey = scalar_ecdh_input.SilentPaymentECDH(recipient.m_scan_pubkey);
    m_recipient = recipient;
}

CPubKey Sender::CreateOutput(const CPubKey& spend_pubkey, const uint32_t output_index) const
{
    HashWriter h;
    h.write(Span{m_ecdh_pubkey});
    unsigned char num[4];
    WriteBE32(num, output_index);
    h << num;
    uint256 shared_secret = h.GetSHA256();
    return spend_pubkey.AddTweak(shared_secret.begin());
}

std::vector<wallet::CRecipient> Sender::GenerateRecipientScriptPubKeys() const
{
    int n = 0;
    std::vector<wallet::CRecipient> spks;
    for (const auto& pair : m_recipient.m_outputs) {
        XOnlyPubKey output_pubkey = XOnlyPubKey{CreateOutput(pair.first, n)};
        auto tap = WitnessV1Taproot(output_pubkey);
        CScript spk = GetScriptForDestination(tap);
        spks.push_back(wallet::CRecipient{spk, pair.second, false});
        n++;
    }
    return spks;
}

CKey SumInputPrivKeys(const std::vector<std::pair<CKey, bool>>& sender_secret_keys)
{
    const auto& [seckey, is_taproot] = sender_secret_keys.at(0);
    CKey sum_seckey{seckey};
    if (is_taproot && sum_seckey.GetPubKey()[0] == 3) {
        sum_seckey.Negate();
    }
    if (sender_secret_keys.size() > 1) {
        for (size_t i = 1; i < sender_secret_keys.size(); i++) {
            const auto& [sender_seckey, sender_is_taproot] = sender_secret_keys.at(i);
            auto temp_key{sender_seckey};
            if (sender_is_taproot && sender_seckey.GetPubKey()[0] == 3) {
                temp_key.Negate();
            }
            sum_seckey = sum_seckey.AddTweak(temp_key.begin());
        }
    }
    return sum_seckey;
}

CKey PrepareScalarECDHInput(const std::vector<std::pair<CKey, bool>>& sender_secret_keys, const std::vector<COutPoint>& tx_outpoints)
{
    CKey sum_input_secret_keys = SumInputPrivKeys(sender_secret_keys);
    uint256 outpoints_hash = HashOutpoints(tx_outpoints);
    return sum_input_secret_keys.MultiplyTweak(outpoints_hash.begin());
}

std::vector<SilentPaymentRecipient> GroupSilentPaymentAddresses(const std::vector<wallet::V0SilentPaymentDestination>& silent_payment_destinations)
{
    std::map<CPubKey, std::vector<std::pair<CPubKey, CAmount>>> recipient_groups;
    std::vector<SilentPaymentRecipient> recipients;
    for (const auto& destination : silent_payment_destinations) {
        recipient_groups[destination.m_scan_pubkey].emplace_back(destination.m_spend_pubkey, destination.m_amount);
    }
    for (const auto& pair : recipient_groups) {
        SilentPaymentRecipient recipient{pair.first};
        for (const auto& output : pair.second) {
            recipient.m_outputs.push_back(output);
        }
        recipients.push_back(recipient);
    }
    return recipients;
}

uint256 HashOutpoints(const std::vector<COutPoint>& tx_outpoints)
{

    // Make a local copy of the outpoints so we can sort them before hashing.
    // This is to ensure the sender and receiver deterministically arrive at the same outpoint hash,
    // regardless of how the outpoints are ordered in the transaction.

    std::vector<COutPoint> outpoints{tx_outpoints};
    std::sort(outpoints.begin(), outpoints.end());

    HashWriter h;
    for (const auto& outpoint: outpoints) {
        h << outpoint;
    }
    return h.GetSHA256();
}
}
