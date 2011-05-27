#include "transactiontablemodel.h"
#include "main.h"

#include <QLocale>
#include <QDebug>
#include <QList>

const QString TransactionTableModel::Sent = "s";
const QString TransactionTableModel::Received = "r";
const QString TransactionTableModel::Generated = "g";

/* Separate transaction record format from core.
 * When the GUI is going to communicate with the core through the network,
 * we'll need our own internal formats anyway.
 */
class TransactionRecord
{
public:
    /* Information that never changes for the life of the transaction
     */
    uint256 hash;
    int64 time;
    int64 credit;
    int64 debit;
    int64 change;
    int64 lockTime;
    int64 timeReceived;
    bool isCoinBase;
    int blockIndex;

    /* Properties that change based on changes in block chain that come in
       over the network.
     */
    bool confirmed;
    int depthInMainChain;
    bool final;
    int requestCount;

    TransactionRecord(const CWalletTx &tx)
    {
        /* Copy immutable properties.
         */
        hash = tx.GetHash();
        time = tx.GetTxTime();
        credit = tx.GetCredit(true);
        debit = tx.GetDebit();
        change = tx.GetChange();
        isCoinBase = tx.IsCoinBase();
        lockTime = tx.nLockTime;
        timeReceived = tx.nTimeReceived;

        /* Find the block the tx is in, store the index
         */
        CBlockIndex* pindex = NULL;
        std::map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.find(tx.hashBlock);
        if (mi != mapBlockIndex.end())
            pindex = (*mi).second;
        blockIndex = (pindex ? pindex->nHeight : INT_MAX);

        update(tx);
    }

    void update(const CWalletTx &tx)
    {
        confirmed = tx.IsConfirmed();
        depthInMainChain = tx.GetDepthInMainChain();
        final = tx.IsFinal();
        requestCount = tx.GetRequestCount();
    }
};

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
                cachedWallet.append(TransactionRecord(it->second));
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
    /*
    int retval = 0;
    CRITICAL_BLOCK(cs_mapWallet)
    {
        retval = mapWallet.size();
    }
    return retval;
    */
    return impl->size();
}

int TransactionTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return columns.length();
}

QVariant TransactionTableModel::formatTxStatus(const TransactionRecord *wtx) const
{
    return QVariant(QString("Test"));
#if 0
    // Status
    if (!wtx.IsFinal())
    {
        if (wtx.nLockTime < 500000000)
            return strprintf(_("Open for %d blocks"), nBestHeight - wtx.nLockTime);
        else
            return strprintf(_("Open until %s"), DateTimeStr(wtx.nLockTime).c_str());
    }
    else
    {
        int nDepth = wtx.GetDepthInMainChain();
        if (GetAdjustedTime() - wtx.nTimeReceived > 2 * 60 && wtx.GetRequestCount() == 0)
            return strprintf(_("%d/offline?"), nDepth);
        else if (nDepth < 6)
            return strprintf(_("%d/unconfirmed"), nDepth);
        else
            return strprintf(_("%d confirmations"), nDepth);
    }
#endif
}

QVariant TransactionTableModel::formatTxDate(const TransactionRecord *wtx) const
{
    return QVariant();
}

QVariant TransactionTableModel::formatTxDescription(const TransactionRecord *wtx) const
{
    return QVariant();
}

QVariant TransactionTableModel::formatTxDebit(const TransactionRecord *wtx) const
{
    return QVariant();
}

QVariant TransactionTableModel::formatTxCredit(const TransactionRecord *wtx) const
{
    return QVariant();
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

