#include "transactiontablemodel.h"
#include "guiutil.h"
#include "main.h"

#include <QLocale>
#include <QDebug>
#include <QList>

const QString TransactionTableModel::Sent = "s";
const QString TransactionTableModel::Received = "r";
const QString TransactionTableModel::Generated = "g";
const QString TransactionTableModel::Other = "o";

/* TODO: look up address in address book
   when showing.
   Color based on confirmation status.
   (fConfirmed ? wxColour(0,0,0) : wxColour(128,128,128))
 */

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
        MaturesIn,
        MaturesWarning, /* Will probably not mature because no nodes have confirmed */
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
        RecvFromAddress,
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

/* Return positive answer if transaction should be shown in list.
 */
bool showTransaction(const CWalletTx &wtx)
{
    if (wtx.IsCoinBase())
    {
        // Don't show generated coin until confirmed by at least one block after it
        // so we don't get the user's hopes up until it looks like it's probably accepted.
        //
        // It is not an error when generated blocks are not accepted.  By design,
        // some percentage of blocks, like 10% or more, will end up not accepted.
        // This is the normal mechanism by which the network copes with latency.
        //
        // We display regular transactions right away before any confirmation
        // because they can always get into some block eventually.  Generated coins
        // are special because if their block is not accepted, they are not valid.
        //
        if (wtx.GetDepthInMainChain() < 2)
        {
            return false;
        }
    }
    return true;
}

/* Decompose CWallet transaction to model transaction records.
 */
QList<TransactionRecord> decomposeTransaction(const CWalletTx &wtx)
{
    QList<TransactionRecord> parts;
    int64 nTime = wtx.nTimeDisplayed = wtx.GetTxTime();
    int64 nCredit = wtx.GetCredit(true);
    int64 nDebit = wtx.GetDebit();
    int64 nNet = nCredit - nDebit;
    uint256 hash = wtx.GetHash();
    std::map<std::string, std::string> mapValue = wtx.mapValue;

    // Find the block the tx is in
    CBlockIndex* pindex = NULL;
    std::map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.find(wtx.hashBlock);
    if (mi != mapBlockIndex.end())
        pindex = (*mi).second;

    // Determine transaction status
    TransactionStatus status;
    // Sort order, unrecorded transactions sort to the top
    status.sortKey = strprintf("%010d-%01d-%010u",
        (pindex ? pindex->nHeight : INT_MAX),
        (wtx.IsCoinBase() ? 1 : 0),
        wtx.nTimeReceived);
    status.confirmed = wtx.IsConfirmed();
    status.depth = wtx.GetDepthInMainChain();

    if (!wtx.IsFinal())
    {
        if (wtx.nLockTime < 500000000)
        {
            status.status = TransactionStatus::OpenUntilBlock;
            status.open_for = nBestHeight - wtx.nLockTime;
        } else {
            status.status = TransactionStatus::OpenUntilDate;
            status.open_for = wtx.nLockTime;
        }
    }
    else
    {
        if (GetAdjustedTime() - wtx.nTimeReceived > 2 * 60 && wtx.GetRequestCount() == 0)
        {
            status.status = TransactionStatus::Offline;
        } else if (status.depth < 6)
        {
            status.status = TransactionStatus::Unconfirmed;
        } else
        {
            status.status = TransactionStatus::HaveConfirmations;
        }
    }

    if (showTransaction(wtx))
    {

        if (nNet > 0 || wtx.IsCoinBase())
        {
            //
            // Credit
            //
            TransactionRecord sub(hash, nTime, status);

            sub.credit = nNet;

            if (wtx.IsCoinBase())
            {
                // Generated
                sub.type = TransactionRecord::Generated;

                if (nCredit == 0)
                {
                    sub.status.maturity = TransactionStatus::Immature;

                    int64 nUnmatured = 0;
                    BOOST_FOREACH(const CTxOut& txout, wtx.vout)
                        nUnmatured += txout.GetCredit();
                    sub.credit = nUnmatured;

                    if (wtx.IsInMainChain())
                    {
                        sub.status.maturity = TransactionStatus::MaturesIn;
                        sub.status.matures_in = wtx.GetBlocksToMaturity();

                        // Check if the block was requested by anyone
                        if (GetAdjustedTime() - wtx.nTimeReceived > 2 * 60 && wtx.GetRequestCount() == 0)
                            sub.status.maturity = TransactionStatus::MaturesWarning;
                    }
                    else
                    {
                        sub.status.maturity = TransactionStatus::NotAccepted;
                    }
                }
            }
            else if (!mapValue["from"].empty() || !mapValue["message"].empty())
            {
                // Received by IP connection
                sub.type = TransactionRecord::RecvFromIP;
                if (!mapValue["from"].empty())
                    sub.address = mapValue["from"];
            }
            else
            {
                // Received by Bitcoin Address
                sub.type = TransactionRecord::RecvFromAddress;
                BOOST_FOREACH(const CTxOut& txout, wtx.vout)
                {
                    if (txout.IsMine())
                    {
                        std::vector<unsigned char> vchPubKey;
                        if (ExtractPubKey(txout.scriptPubKey, true, vchPubKey))
                        {
                            sub.address = PubKeyToAddress(vchPubKey);
                        }
                        break;
                    }
                }
            }
            parts.append(sub);
        }
        else
        {
            bool fAllFromMe = true;
            BOOST_FOREACH(const CTxIn& txin, wtx.vin)
                fAllFromMe = fAllFromMe && txin.IsMine();

            bool fAllToMe = true;
            BOOST_FOREACH(const CTxOut& txout, wtx.vout)
                fAllToMe = fAllToMe && txout.IsMine();

            if (fAllFromMe && fAllToMe)
            {
                // Payment to self
                int64 nChange = wtx.GetChange();

                parts.append(TransactionRecord(hash, nTime, status, TransactionRecord::SendToSelf, "",
                                -(nDebit - nChange), nCredit - nChange));
            }
            else if (fAllFromMe)
            {
                //
                // Debit
                //
                int64 nTxFee = nDebit - wtx.GetValueOut();

                for (int nOut = 0; nOut < wtx.vout.size(); nOut++)
                {
                    const CTxOut& txout = wtx.vout[nOut];
                    TransactionRecord sub(hash, nTime, status);

                    if (txout.IsMine())
                    {
                        // Sent to self
                        sub.type = TransactionRecord::SendToSelf;
                        sub.credit = txout.nValue;
                    } else if (!mapValue["to"].empty())
                    {
                        // Sent to IP
                        sub.type = TransactionRecord::SendToIP;
                        sub.address = mapValue["to"];
                    } else {
                        // Sent to Bitcoin Address
                        sub.type = TransactionRecord::SendToAddress;
                        uint160 hash160;
                        if (ExtractHash160(txout.scriptPubKey, hash160))
                            sub.address = Hash160ToAddress(hash160);
                    }

                    int64 nValue = txout.nValue;
                    /* Add fee to first output */
                    if (nTxFee > 0)
                    {
                        nValue += nTxFee;
                        nTxFee = 0;
                    }
                    sub.debit = nValue;
                    sub.status.sortKey += strprintf("-%d", nOut);

                    parts.append(sub);
                }
            } else {
                //
                // Mixed debit transaction, can't break down payees
                //
                bool fAllMine = true;
                BOOST_FOREACH(const CTxOut& txout, wtx.vout)
                    fAllMine = fAllMine && txout.IsMine();
                BOOST_FOREACH(const CTxIn& txin, wtx.vin)
                    fAllMine = fAllMine && txin.IsMine();

                parts.append(TransactionRecord(hash, nTime, status, TransactionRecord::Other, "", nNet, 0));
            }
        }
    }

    return parts;
}

/* Internal implementation */
class TransactionTableImpl
{
public:
    QList<TransactionRecord> cachedWallet;

    /* Update our model of the wallet */
    void updateWallet()
    {
        QList<int> insertedIndices;
        QList<int> removedIndices;

        cachedWallet.clear();

        /* Query wallet from core, and compare with our own
           representation.
         */
        CRITICAL_BLOCK(cs_mapWallet)
        {
            for(std::map<uint256, CWalletTx>::iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
            {
                /* TODO: Make note of new and removed transactions */
                /* insertedIndices */
                /* removedIndices */
                cachedWallet.append(decomposeTransaction(it->second));
            }
        }
        /* beginInsertRows(QModelIndex(), first, last) */
        /* endInsertRows */
        /* beginRemoveRows(QModelIndex(), first, last) */
        /* beginEndRows */
    }

    int size()
    {
        return cachedWallet.size();
    }

    TransactionRecord *index(int idx)
    {
        if(idx >= 0 && idx < cachedWallet.size())
        {
            return &cachedWallet[idx];
        } else {
            return 0;
        }
    }
};

/* Credit and Debit columns are right-aligned as they contain numbers */
static int column_alignments[] = {
        Qt::AlignLeft|Qt::AlignVCenter,
        Qt::AlignLeft|Qt::AlignVCenter,
        Qt::AlignLeft|Qt::AlignVCenter,
        Qt::AlignRight|Qt::AlignVCenter,
        Qt::AlignRight|Qt::AlignVCenter,
        Qt::AlignLeft|Qt::AlignVCenter
    };

TransactionTableModel::TransactionTableModel(QObject *parent):
        QAbstractTableModel(parent),
        impl(new TransactionTableImpl())
{
    columns << tr("Status") << tr("Date") << tr("Description") << tr("Debit") << tr("Credit");

    impl->updateWallet();
}

TransactionTableModel::~TransactionTableModel()
{
    delete impl;
}


int TransactionTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return impl->size();
}

int TransactionTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return columns.length();
}

QVariant TransactionTableModel::formatTxStatus(const TransactionRecord *wtx) const
{
    QString status;
    switch(wtx->status.status)
    {
    case TransactionStatus::OpenUntilBlock:
        status = tr("Open for %n block(s)","",wtx->status.open_for);
        break;
    case TransactionStatus::OpenUntilDate:
        status = tr("Open until ") + DateTimeStr(wtx->status.open_for);
        break;
    case TransactionStatus::Offline:
        status = tr("%1/offline").arg(wtx->status.depth);
        break;
    case TransactionStatus::Unconfirmed:
        status = tr("%1/unconfirmed").arg(wtx->status.depth);
        break;
    case TransactionStatus::HaveConfirmations:
        status = tr("%1 confirmations").arg(wtx->status.depth);
        break;
    }

    return QVariant(status);
}

QVariant TransactionTableModel::formatTxDate(const TransactionRecord *wtx) const
{
    if(wtx->time)
    {
        return QVariant(DateTimeStr(wtx->time));
    } else {
        return QVariant();
    }
}

QVariant TransactionTableModel::formatTxDescription(const TransactionRecord *wtx) const
{
    return QVariant();
}

QVariant TransactionTableModel::formatTxDebit(const TransactionRecord *wtx) const
{
    if(wtx->debit)
    {
        QString str = QString::fromStdString(FormatMoney(wtx->debit));
        if(!wtx->status.confirmed)
        {
            str = QString("[") + str + QString("]");
        }
        return QVariant(str);
    } else {
        return QVariant();
    }
}

QVariant TransactionTableModel::formatTxCredit(const TransactionRecord *wtx) const
{
    if(wtx->credit)
    {
        QString str = QString::fromStdString(FormatMoney(wtx->credit));
        if(!wtx->status.confirmed)
        {
            str = QString("[") + str + QString("]");
        }
        return QVariant(str);
    } else {
        return QVariant();
    }
}

QVariant TransactionTableModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();
    TransactionRecord *rec = static_cast<TransactionRecord*>(index.internalPointer());

    if(role == Qt::DisplayRole)
    {
        switch(index.column())
        {
        case Status:
            return formatTxStatus(rec);
        case Date:
            return formatTxDate(rec);
        case Description:
            return formatTxDescription(rec);
        case Debit:
            return formatTxDebit(rec);
        case Credit:
            return formatTxCredit(rec);
        }
    } else if (role == Qt::TextAlignmentRole)
    {
        return column_alignments[index.column()];
    } else if (role == TypeRole)
    {
        /* user role: transaction type for filtering
           "s" (sent)
           "r" (received)
           "g" (generated)
        */
        switch(index.row() % 3)
        {
        case 0: return QString("s");
        case 1: return QString("r");
        case 2: return QString("o");
        }
    }
    return QVariant();
}

QVariant TransactionTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(role == Qt::DisplayRole)
    {
        if(orientation == Qt::Horizontal)
        {
            return columns[section];
        }
    } else if (role == Qt::TextAlignmentRole)
    {
        return column_alignments[section];
    }
    return QVariant();
}

Qt::ItemFlags TransactionTableModel::flags(const QModelIndex &index) const
{
    return QAbstractTableModel::flags(index);
}


QModelIndex TransactionTableModel::index ( int row, int column, const QModelIndex & parent ) const
{
    Q_UNUSED(parent);
    TransactionRecord *data = impl->index(row);
    if(data)
    {
        return createIndex(row, column, impl->index(row));
    } else {
        return QModelIndex();
    }
}

