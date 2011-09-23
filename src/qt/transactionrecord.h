#ifndef TRANSACTIONRECORD_H
#define TRANSACTIONRECORD_H

#include "uint256.h"

#include <QList>

class CWallet;
class CWalletTx;

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
        MaturesWarning, /* Will likely not mature because no nodes have confirmed */
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

    /* For "Generated" transactions */
    Maturity maturity;
    int matures_in;

    /* Reported status */
    Status status;
    int64 depth;
    int64 open_for; /* Timestamp if status==OpenUntilDate, otherwise number of blocks */

    /* Current number of blocks (to know whether cached status is still valid. */
    int cur_num_blocks;
};

class TransactionRecord
{
public:
    enum Type
    {
        Other,
        Generated,
        SendToAddress,
        SendToIP,
        RecvWithAddress,
        RecvFromIP,
        SendToSelf
    };

    /* Number of confirmation needed for transaction */
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

    /* Decompose CWallet transaction to model transaction records.
     */
    static bool showTransaction(const CWalletTx &wtx);
    static QList<TransactionRecord> decomposeTransaction(const CWallet *wallet, const CWalletTx &wtx);

    /* Fixed */
    uint256 hash;
    int64 time;
    Type type;
    std::string address;
    int64 debit;
    int64 credit;

    /* Subtransaction index, for sort key */
    int idx;

    /* Status: can change with block chain update */
    TransactionStatus status;

    /* Return the unique identifier for this transaction (part) */
    std::string getTxID();

    /* Update status from wallet tx.
     */
    void updateStatus(const CWalletTx &wtx);

    /* Is a status update needed?
     */
    bool statusUpdateNeeded();
};

#endif // TRANSACTIONRECORD_H
