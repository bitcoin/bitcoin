#ifndef BITCOIN_INDEX_SILENTPAYMENTINDEX_H
#define BITCOIN_INDEX_SILENTPAYMENTINDEX_H

#include <coins.h>
#include <index/base.h>
#include <pubkey.h>

class COutPoint;
class CBlockUndo;

static const char* const DEFAULT_SILENT_PAYMENT_INDEX = "0";

/**
 * SilentPaymentIndex is used to look up the tweaked sum of eligible public keys for a given transaction hash.
 * Eligible public keys are taken from Inputs For Shared Secret Derivation (BIP352).
 * They are tweaked using the outpoints (of all inputs).
 *
 * TODO: figure out how to handle versioning, maybe call the index v0?
 */
class SilentPaymentIndex final : public BaseIndex
{
protected:
    class DB;

private:
    const std::unique_ptr<DB> m_db;

    bool AllowPrune() const override { return false; }

    /**
     * Derive the silent payment tweaked public key for every block transaction.
     *
     * @param[in] txs all block transactions
     * @param[in] block_undo block undo data
     * @param[out] tweaked_pub_key_sums the tweaked public keys, only for transactions that have one
     * @return false if something went wrong
     */
    bool GetSilentPaymentKeys(std::vector<CTransactionRef> txs, CBlockUndo& block_undo, std::vector<CPubKey>& tweaked_pub_key_sums);

    bool ExtractPubkeyFromInput(const Coin& prevCoin, const CTxIn& txin,  XOnlyPubKey& senderPubKey);

protected:
    bool CustomAppend(const interfaces::BlockInfo& block) override;

    BaseIndex::DB& GetDB() const override;
public:

    explicit SilentPaymentIndex(std::unique_ptr<interfaces::Chain> chain, size_t n_cache_size, bool f_memory = false, bool f_wipe = false);

    // Destructor is declared because this class contains a unique_ptr to an incomplete type.
    virtual ~SilentPaymentIndex() override;

    bool FindSilentPayment(const uint256& block_hash, std::vector<CPubKey>& tweaked_pub_key_sums) const;
};

/// The global txo silent payment index. May be null.
extern std::unique_ptr<SilentPaymentIndex> g_silent_payment_index;

#endif // BITCOIN_INDEX_SILENTPAYMENTINDEX_H