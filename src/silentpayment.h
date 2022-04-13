#ifndef BITCOIN_SILENTPAYMENT_H
#define BITCOIN_SILENTPAYMENT_H

#include <coins.h>
#include <key_io.h>
#include <undo.h>

namespace silentpayment {

class Recipient {
    protected:
        CKey m_negated_scan_seckey;
        unsigned char m_shared_secret[32];
        std::vector<std::pair<CKey, XOnlyPubKey>> m_spend_keys;
        XOnlyPubKey m_scan_pubkey;

    public:
        Recipient(const CKey& spend_seckey, size_t pool_size);
        void SetSenderPublicKey(const CPubKey& sender_public_key, const uint256& outpoint_hash);
        void SetSenderPublicKey(const CPubKey& sender_public_key, const std::vector<COutPoint>& tx_outpoints);
        std::tuple<CKey,XOnlyPubKey> Tweak(const int32_t& identifier) const;
        std::pair<XOnlyPubKey,XOnlyPubKey> GetAddress(const int32_t& identifier) const;
        int32_t GetIdentifier(const XOnlyPubKey& spend_pubkey) const;

        static CPubKey CombinePublicKeys(const std::vector<CPubKey>& sender_public_keys, const std::vector<XOnlyPubKey>& sender_x_only_public_key);
        static CPubKey CombinePublicKeys(const CTransaction& tx, const std::vector<Coin>& coins);
}; // class Recipient

class Sender {
    protected:
        unsigned char m_shared_secret[32];

    public:
        Sender(const std::vector<std::tuple<CKey, bool>>& sender_secret_keys, const std::vector<COutPoint>& tx_outpoints, const XOnlyPubKey& recipient_scan_xonly_pubkey);
        XOnlyPubKey Tweak(const XOnlyPubKey spend_xonly_pubkey) const;
};  // class Sender

std::pair<std::array<uint8_t, 8>, uint256> HashOutpoints(const std::vector<COutPoint>& tx_outpoints);
/** Extract Pubkey from an input according to the transaction type **/
std::variant<CPubKey, XOnlyPubKey> ExtractPubkeyFromInput(const Coin& prevCoin, const CTxIn& txin);
/** For each transaction in the block, return <txid, sum_public_keys, outpoint_hash, truncated_hash> **/
std::vector<std::tuple<uint256, CPubKey, uint256, std::array<uint8_t, 8>>> GetSilentPaymentKeysPerBlock(const uint256& block_hash, const CBlockUndo& blockUndo, const std::vector<CTransactionRef> vtx);
} // namespace silentpayment

#endif // BITCOIN_SILENTPAYMENT_H
