#ifndef TRANSACTIONRECORD_H
#define TRANSACTIONRECORD_H

#include "main.h"

#include <QList>

class TransactionStatus
{
public:
    TransactionStatus():
            confirmed(false), sortKey(""), maturity(Mature),
            matures_in(0), status(Offline), depth(0), open_for(0)
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

    TransactionRecord():
            hash(), time(0), type(Other), address(""), debit(0), credit(0)
    {
    }

    TransactionRecord(uint256 hash, int64 time, const TransactionStatus &status):
            hash(hash), time(time), type(Other), address(""), debit(0),
            credit(0), status(status)
    {
    }

    TransactionRecord(uint256 hash, int64 time, const TransactionStatus &status,
                Type type, const std::string &address,
                int64 debit, int64 credit):
            hash(hash), time(time), type(type), address(address), debit(debit), credit(credit),
            status(status)
    {
    }

    /* Decompose CWallet transaction to model transaction records.
     */
    static bool showTransaction(const CWalletTx &wtx);
    static QList<TransactionRecord> decomposeTransaction(const CWalletTx &wtx);

    /* Fixed */
    uint256 hash;
    int64 time;
    Type type;
    std::string address;
    int64 debit;
    int64 credit;

    /* Status: can change with block chain update */
    TransactionStatus status;
};

#endif // TRANSACTIONRECORD_H
