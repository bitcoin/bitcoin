// Copyright (c) 2011-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_TRANSACTIONRECORD_H
#define BITCOIN_QT_TRANSACTIONRECORD_H

#include <consensus/amount.h>
#include <uint256.h>
#include <key_io.h>

#include <QList>
#include <QString>

namespace interfaces {
class Node;
class Wallet;
struct WalletTx;
struct WalletTxStatus;
}

/** UI model for transaction status. The transaction status is the part of a transaction that will change over time.
 */
struct TransactionStatus {
    enum Status {
        Confirmed,          /**< Have 6 or more confirmations (normal tx) or fully mature (mined tx) **/
        /// Normal (sent/received) transactions
        Unconfirmed,        /**< Not yet mined into a block **/
        Confirming,         /**< Confirmed, but waiting for the recommended number of confirmations **/
        Conflicted,         /**< Conflicts with other transaction or mempool **/
        Abandoned,          /**< Abandoned from the wallet **/
        /// Generated (mined) transactions
        Immature,           /**< Mined but waiting for maturity */
        NotAccepted         /**< Mined but not accepted */
    };

    /// Transaction counts towards available balance
    bool countsForBalance{false};
    /// Transaction was locked via InstantSend
    bool lockedByInstantSend{false};
    /// Transaction was locked via ChainLocks
    bool lockedByChainLocks{false};
    /// Sorting key based on status
    std::string sortKey;

    /** @name Generated (mined) transactions
       @{*/
    int matures_in{0};
    /**@}*/

    /** @name Reported status
       @{*/
    Status status{Unconfirmed};
    qint64 depth{0};
    /**@}*/

    /** Current block hash (to know whether cached status is still valid) */
    uint256 m_cur_block_hash{};

    //** Know when to update transaction for chainlocks **/
    int cachedChainLockHeight{-1};

    bool needsUpdate{false};
};

/** UI model for a transaction. A core transaction can be represented by multiple UI transactions if it has
    multiple outputs.
 */
class TransactionRecord
{
public:
    // Update COMMON_TYPES in TransactionFilterProxyWhen when adding a new type
    enum Type
    {
        Other,
        Generated,
        SendToAddress,
        SendToOther,
        RecvWithAddress,
        RecvFromOther,
        SendToSelf,
        RecvWithCoinJoin,
        CoinJoinMixing,
        CoinJoinCollateralPayment,
        CoinJoinMakeCollaterals,
        CoinJoinCreateDenominations,
        CoinJoinSend,
        PlatformTransfer,
    };

    /** Number of confirmation recommended for accepting a transaction */
    static const int RecommendedNumConfirmations = 6;

    TransactionRecord():
            hash(), time(0), type(Other), debit(0), credit(0), idx(0)
    {
        txDest = DecodeDestination(strAddress);
    }

    TransactionRecord(uint256 _hash, qint64 _time):
            hash(_hash), time(_time), type(Other), debit(0),
            credit(0), idx(0)
    {
        txDest = DecodeDestination(strAddress);
    }

    TransactionRecord(uint256 _hash, qint64 _time,
                Type _type, const std::string &_strAddress,
                const CAmount& _debit, const CAmount& _credit):
            hash(_hash), time(_time), type(_type), strAddress(_strAddress), debit(_debit), credit(_credit),
            idx(0)
    {
        txDest = DecodeDestination(strAddress);
    }

    /** Decompose CWallet transaction to model transaction records.
     */
    static bool showTransaction();
    static QList<TransactionRecord> decomposeTransaction(interfaces::Node& node, interfaces::Wallet& wallet, const interfaces::WalletTx& wtx);

    /** @name Immutable transaction attributes
      @{*/
    uint256 hash;
    qint64 time;
    Type type;
    std::string strAddress;
    CTxDestination txDest;

    CAmount debit;
    CAmount credit;
    /**@}*/

    /** Subtransaction index, for sort key */
    int idx;

    /** Status: can change with block chain update */
    TransactionStatus status;

    /** Whether the transaction was sent/received with a watch-only address */
    bool involvesWatchAddress;

    /// Label
    QString label;

    /** Return the unique identifier for this transaction (part) */
    QString getTxHash() const;

    /** Return the output index of the subtransaction  */
    int getOutputIndex() const;

    /** Update status from core wallet tx.
     */
    void updateStatus(const interfaces::WalletTxStatus& wtx, const uint256& block_hash, int numBlocks, int chainLockHeight, int64_t block_time);

    /** Return whether a status update is needed.
     */
    bool statusUpdateNeeded(const uint256& block_hash, int chainLockHeight) const;

    /** Update label from address book.
     */
    void updateLabel(interfaces::Wallet& wallet);
};

#endif // BITCOIN_QT_TRANSACTIONRECORD_H
