#pragma once

#include <amount.h>
#include <primitives/transaction.h>
#include <univalue.h>
#include <boost/optional.hpp>
#include <boost/variant.hpp>

// Forward Declarations
class CWallet;
class CWalletTx;

struct PegoutIndex
{
    // The ID of the kernel containing the pegout.
    mw::Hash kernel_id;

    // The position of the PegOutCoin within the kernel.
    // Ex: If a kernel has 3 pegouts, the last one will have a pos of 2.
    size_t pos;
};

struct WalletTxStatus
{
    enum Status {
        Confirmed, /**< Have 6 or more confirmations (normal tx) or fully mature (mined tx) **/
        /// Normal (sent/received) transactions
        OpenUntilDate,  /**< Transaction not yet final, waiting for date */
        OpenUntilBlock, /**< Transaction not yet final, waiting for block */
        Unconfirmed,    /**< Not yet mined into a block **/
        Confirming,     /**< Confirmed, but waiting for the recommended number of confirmations **/
        Conflicted,     /**< Conflicts with other transaction or mempool **/
        Abandoned,      /**< Abandoned from the wallet **/
        /// Generated (mined) transactions
        Immature,   /**< Mined but waiting for maturity */
        NotAccepted /**< Mined but not accepted */
    };

    /// Transaction counts towards available balance
    bool countsForBalance;
    /// Sorting key based on status
    std::string sortKey;

    /** @name Generated (mined) transactions
       @{*/
    int matures_in;
    /**@}*/

    /** @name Reported status
       @{*/
    Status status;
    int64_t depth;
    int64_t open_for; /**< Timestamp if status==OpenUntilDate, otherwise number
                      of additional blocks that need to be mined before
                      finalization */
    /**@}*/

    /** Current block hash (to know whether cached status is still valid) */
    uint256 m_cur_block_hash{};

    bool needsUpdate;
};

class WalletTxRecord
{
public:
    WalletTxRecord(const CWallet* pWallet, const CWalletTx* wtx, const OutputIndex& output_index)
        : m_pWallet(pWallet), m_wtx(wtx)
    {
        if (output_index.type() == typeid(mw::Hash)) {
            index = boost::make_optional(boost::get<mw::Hash>(output_index));
        } else {
            index = boost::make_optional((int)boost::get<COutPoint>(output_index).n);
        }
    }
    WalletTxRecord(const CWallet* pWallet, const CWalletTx* wtx, const PegoutIndex& pegout_index)
        : m_pWallet(pWallet), m_wtx(wtx), index(boost::make_optional(pegout_index)) {}
    WalletTxRecord(const CWallet* pWallet, const CWalletTx* wtx)
        : m_pWallet(pWallet), m_wtx(wtx), index(boost::none) {}

    static const int RecommendedNumConfirmations = 6;

    enum Type {
        Other,
        Generated,
        SendToAddress,
        SendToOther,
        RecvWithAddress,
        RecvFromOther,
        SendToSelf,
    };

    Type type{Type::Other};
    std::string address{};
    CAmount debit{0};
    CAmount credit{0};
    CAmount fee{0};
    bool involvesWatchAddress{false};

    // Cached status attributes
    WalletTxStatus status;

    // Updates the transaction record's cached status attributes.
    bool UpdateStatusIfNeeded(const uint256& block_hash);

    const CWalletTx& GetWTX() const noexcept { return *m_wtx; }
    const uint256& GetTxHash() const;
    std::string GetTxString() const;
    int64_t GetTxTime() const;
    CAmount GetAmount() const noexcept { return credit > 0 ? credit : debit; }
    //CAmount GetAmount() const noexcept { return credit + debit; }
    CAmount GetNet() const noexcept { return credit + debit + fee; }

    UniValue ToUniValue() const;

    // Returns the formatted component index.
    std::string GetComponentIndex() const;

private:
    // Pointer to the CWallet instance
    const CWallet* m_pWallet;

    // The actual CWalletTx
    const CWalletTx* m_wtx;

    // The index of the transaction component this record is for.
    boost::optional<boost::variant<int, mw::Hash, PegoutIndex>> index;

    std::string GetType() const;
};