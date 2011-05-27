#include "transactiontablemodel.h"
#include "guiutil.h"
#include "transactionrecord.h"
#include "main.h"

#include <QLocale>
#include <QDebug>
#include <QList>
#include <QColor>

const QString TransactionTableModel::Sent = "s";
const QString TransactionTableModel::Received = "r";
const QString TransactionTableModel::Other = "o";

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
                cachedWallet.append(TransactionRecord::decomposeTransaction(it->second));
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

/* Look up address in address book, if found return
     address[0:12]... (label)
   otherwise just return address
 */
std::string lookupAddress(const std::string &address)
{
    std::string description;
    CRITICAL_BLOCK(cs_mapAddressBook)
    {
        std::map<std::string, std::string>::iterator mi = mapAddressBook.find(address);
        if (mi != mapAddressBook.end() && !(*mi).second.empty())
        {
            std::string label = (*mi).second;
            description += address.substr(0,12) + "... ";
            description += "(" + label + ")";
        }
        else
            description += address;
    }
    return description;
}

QVariant TransactionTableModel::formatTxDescription(const TransactionRecord *wtx) const
{
    QString description;

    switch(wtx->type)
    {
    case TransactionRecord::RecvFromAddress:
        description = tr("From: ") + QString::fromStdString(lookupAddress(wtx->address));
        break;
    case TransactionRecord::RecvFromIP:
        description = tr("From IP: ") + QString::fromStdString(wtx->address);
        break;
    case TransactionRecord::SendToAddress:
        description = tr("To: ") + QString::fromStdString(lookupAddress(wtx->address));
        break;
    case TransactionRecord::SendToIP:
        description = tr("To IP: ") + QString::fromStdString(wtx->address);
        break;
    case TransactionRecord::SendToSelf:
        description = tr("Payment to yourself");
        break;
    case TransactionRecord::Generated:
        /* TODO: more extensive description */
        description = tr("Generated");
        break;
    }
    return QVariant(description);
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
    } else if (role == Qt::ForegroundRole)
    {
        if(rec->status.confirmed)
        {
            return QColor(0, 0, 0);
        } else {
            return QColor(128, 128, 128);
        }
    } else if (role == TypeRole)
    {
        switch(rec->type)
        {
        case TransactionRecord::RecvFromAddress:
        case TransactionRecord::RecvFromIP:
        case TransactionRecord::Generated:
            return TransactionTableModel::Received;
        case TransactionRecord::SendToAddress:
        case TransactionRecord::SendToIP:
        case TransactionRecord::SendToSelf:
            return TransactionTableModel::Sent;
        default:
            return TransactionTableModel::Other;
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

