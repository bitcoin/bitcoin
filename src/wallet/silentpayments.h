#ifndef BITCOIN_WALLET_SILENTPAYMENTS_H
#define BITCOIN_WALLET_SILENTPAYMENTS_H

#include <coins.h>
#include <key_io.h>
#include <undo.h>
#include <wallet/types.h>

namespace wallet {
class Recipient {
    protected:
        CKey m_scan_seckey;
        CKey m_spend_seckey;
        CPubKey m_scan_pubkey;
        CPubKey m_spend_pubkey;
        CPubKey m_ecdh_pubkey;

    public:
        Recipient(const CKey& scan_seckey, const CKey& spend_seckey);
        void ComputeECDHSharedSecret(const CPubKey& sender_public_key, const std::vector<COutPoint>& tx_outpoints);
        std::pair<CKey,CPubKey> CreateOutput(const uint32_t output_index) const;
        std::pair<CPubKey,CPubKey> GetAddress() const;
        std::vector<CKey> ScanTxOutputs(std::vector<XOnlyPubKey> output_pub_keys);
};

struct SilentPaymentRecipient
{
    CPubKey m_scan_pubkey;
    std::vector<std::pair<CPubKey, CAmount>> m_outputs;
    SilentPaymentRecipient() {};
    SilentPaymentRecipient(CPubKey scan_pubkey) : m_scan_pubkey(scan_pubkey) {};
};

class Sender {
    protected:
        CPubKey m_ecdh_pubkey;
        SilentPaymentRecipient m_recipient;
        CPubKey CreateOutput(const CPubKey& spend_pubkey, const uint32_t output_index) const;

    public:
        Sender(const CKey& scalar_ecdh_input, const SilentPaymentRecipient& recipient);
        std::vector<wallet::CRecipient> GenerateRecipientScriptPubKeys() const;
};  // class Sender

uint256 HashOutpoints(const std::vector<COutPoint>& tx_outpoints);
std::vector<SilentPaymentRecipient> GroupSilentPaymentAddresses(const std::vector<V0SilentPaymentDestination>& silent_payment_addresses);
CKey PrepareScalarECDHInput(const std::vector<std::pair<CKey, bool>>& sender_secret_keys, const std::vector<COutPoint>& tx_outpoints);
} // namespace wallet
#endif // BITCOIN_WALLET_SILENTPAYMENTS_H
