#ifndef TRANSACTIONRECORD_H
#define TRANSACTIONRECORD_H

#include "uint256.h"

#include <QList>

class CWallet;
class CWalletTx;

/** UI model for transaction status. The transaction status is the part of a transaction that will change over time.
 */
class TransactionStatus
{
public:
    TransactionStatus():
        confirmed(false), sortKey(""), maturity(Mature),
        matures_in(0), status(Offline), depth(0), open_for(0), cur_num_blocks(-1)
    { }

    enum Maturity
    {
        Immature,
        Mature,
        MaturesWarning, /**< Transaction will likely not mature because no nodes have confirmed */
        NotAccepted
    };

    enum Status {
        OpenUntilDate,
        OpenUntilBlock,
        Offline,
        Unconfirmed,
        HaveConfirmations
    };

    bool confirmed;
    std::string sortKey;

    /** @name Generated (mined) transactions
       @{*/
    Maturity maturity;
    int matures_in;
    /**@}*/

    /** @name Reported status
       @{*/
    Status status;
    int64 depth;
    int64 open_for; /**< Timestamp if status==OpenUntilDate, otherwise number
                      of additional blocks that need to be mined before
                      finalization */
    /**@}*/

    /** Current number of blocks (to know whether cached status is still valid) */
    int cur_num_blocks;
};

/** UI model for a transaction. A core transaction can be represented by multiple UI transactions if it has
    multiple outputs.
 */
class TransactionRecord
{
public:
    enum Type
    {
        Other,
        Generated,
        SendToAddress,
        SendToOther,
        RecvWithAddress,
        RecvFromOther,
        SendToSelf
    };

    /** Number of confirmation needed for transaction */
    static const int NumConfirmations = 6;

    TransactionRecord():
            hash(), time(0), type(Other), address(""), debit(0), credit(0), idx(0)
    {
    }

    TransactionRecord(uint256 hash, int64 time):
            hash(hash), time(time), type(Other), address(""), debit(0),
            credit(0), idx(0)
    {
    }

    TransactionRecord(uint256 hash, int64 time,
                Type type, const std::string &address,
                int64 debit, int64 credit):
            hash(hash), time(time), type(type), address(address), debit(debit), credit(credit),
            idx(0)
    {
    }

    /** Decompose CWallet transaction to model transaction records.
     */
    static bool showTransaction(const CWalletTx &wtx);
    static QList<TransactionRecord> decomposeTransaction(const CWallet *wallet, const CWalletTx &wtx);

    /** @name Immutable transaction attributes
      @{*/
    uint256 hash;
    int64 time;
    Type type;
    std::string address;
    int64 debit;
    int64 credit;
    /**@}*/

    /** Subtransaction index, for sort key */
    int idx;

    /** Status: can change with block chain update */
    TransactionStatus status;

    /** Return the unique identifier for this transaction (part) */
    std::string getTxID();

    /** Update status from core wallet tx.
     */
    void updateStatus(const CWalletTx &wtx);

    /** Return whether a status update is needed.
     */
    bool statusUpdateNeeded();
};

#endif // TRANSACTIONRECORD_H
