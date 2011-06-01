#include "transactiontablemodel.h"
#include "guiutil.h"
#include "transactionrecord.h"
#include "guiconstants.h"
#include "main.h"

#include <QLocale>
#include <QDebug>
#include <QList>
#include <QColor>
#include <QTimer>

const QString TransactionTableModel::Sent = "s";
const QString TransactionTableModel::Received = "r";
const QString TransactionTableModel::Other = "o";

/* Private implementation */
struct TransactionTablePriv
{
    /* Local cache of wallet.
     * As it is in the same order as the CWallet, by definition
     * this is sorted by sha256.
     */
    QList<TransactionRecord> cachedWallet;

    void refreshWallet()
    {
        qDebug() << "refreshWallet";

        /* Query entire wallet from core.
         */
        cachedWallet.clear();
        CRITICAL_BLOCK(cs_mapWallet)
        {
            for(std::map<uint256, CWalletTx>::iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
            {
                cachedWallet.append(TransactionRecord::decomposeTransaction(it->second));
            }
        }
    }

    /* Update our model of the wallet.
       Call with list of hashes of transactions that were added, removed or changed.
     */
    void updateWallet(const QList<uint256> &updated)
    {
        /* TODO: update only transactions in updated, and only if
           the transactions are really part of the visible wallet.

           Update status of the other transactions in the cache just in case,
           because this call means that a new block came in.
         */
        qDebug() << "updateWallet";
        foreach(uint256 hash, updated)
        {
            qDebug() << "  " << QString::fromStdString(hash.ToString());
        }
        /* beginInsertRows(QModelIndex(), first, last) */
        /* endInsertRows */
        /* beginRemoveRows(QModelIndex(), first, last) */
        /* beginEndRows */

        refreshWallet();
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
        priv(new TransactionTablePriv())
{
    columns << tr("Status") << tr("Date") << tr("Description") << tr("Debit") << tr("Credit");

    priv->refreshWallet();

    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(update()));
    timer->start(MODEL_UPDATE_DELAY);
}

TransactionTableModel::~TransactionTableModel()
{
    delete priv;
}

void TransactionTableModel::update()
{
    QList<uint256> updated;

    /* Check if there are changes to wallet map */
    TRY_CRITICAL_BLOCK(cs_mapWallet)
    {
        if(!vWalletUpdated.empty())
        {
            BOOST_FOREACH(uint256 hash, vWalletUpdated)
            {
                updated.append(hash);
            }
            vWalletUpdated.clear();
        }
    }

    if(!updated.empty())
    {
        /* TODO: improve this, way too brute-force at the moment,
           only update transactions that actually changed, and add/remove
           transactions that were added/removed.
         */
        beginResetModel();
        priv->updateWallet(updated);
        endResetModel();
    }
}

int TransactionTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return priv->size();
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
    case TransactionRecord::RecvWithAddress:
        description = tr("Received with: ") + QString::fromStdString(lookupAddress(wtx->address));
        break;
    case TransactionRecord::RecvFromIP:
        description = tr("Received from IP: ") + QString::fromStdString(wtx->address);
        break;
    case TransactionRecord::SendToAddress:
        description = tr("Sent to: ") + QString::fromStdString(lookupAddress(wtx->address));
        break;
    case TransactionRecord::SendToIP:
        description = tr("Sent to IP: ") + QString::fromStdString(wtx->address);
        break;
    case TransactionRecord::SendToSelf:
        description = tr("Payment to yourself");
        break;
    case TransactionRecord::Generated:
        switch(wtx->status.maturity)
        {
        case TransactionStatus::Immature:
            description = tr("Generated (matures in %n more blocks)", "",
                           wtx->status.matures_in);
            break;
        case TransactionStatus::Mature:
            description = tr("Generated");
            break;
        case TransactionStatus::MaturesWarning:
            description = tr("Generated - Warning: This block was not received by any other nodes and will probably not be accepted!");
            break;
        case TransactionStatus::NotAccepted:
            description = tr("Generated (not accepted)");
            break;
        }
        break;
    }
    return QVariant(description);
}

QVariant TransactionTableModel::formatTxDebit(const TransactionRecord *wtx) const
{
    if(wtx->debit)
    {
        QString str = QString::fromStdString(FormatMoney(wtx->debit));
        if(!wtx->status.confirmed || wtx->status.maturity != TransactionStatus::Mature)
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
        if(!wtx->status.confirmed || wtx->status.maturity != TransactionStatus::Mature)
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
        /* Delegate to specific column handlers */
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
    } else if(role == Qt::EditRole)
    {
        /* Edit role is used for sorting so return the real values */
        switch(index.column())
        {
        case Status:
            return QString::fromStdString(rec->status.sortKey);
        case Date:
            return rec->time;
        case Description:
            return formatTxDescription(rec);
        case Debit:
            return rec->debit;
        case Credit:
            return rec->credit;
        }
    } else if (role == Qt::TextAlignmentRole)
    {
        return column_alignments[index.column()];
    } else if (role == Qt::ForegroundRole)
    {
        /* Non-confirmed transactions are grey */
        if(rec->status.confirmed)
        {
            return QColor(0, 0, 0);
        } else {
            return QColor(128, 128, 128);
        }
    } else if (role == TypeRole)
    {
        /* Role for filtering tabs by type */
        switch(rec->type)
        {
        case TransactionRecord::RecvWithAddress:
        case TransactionRecord::RecvFromIP:
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
    if(orientation == Qt::Horizontal)
    {
        if(role == Qt::DisplayRole)
        {
            return columns[section];
        } else if (role == Qt::TextAlignmentRole)
        {
            return column_alignments[section];
        }
    }
    return QVariant();
}

Qt::ItemFlags TransactionTableModel::flags(const QModelIndex &index) const
{
    return QAbstractTableModel::flags(index);
}

QModelIndex TransactionTableModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    TransactionRecord *data = priv->index(row);
    if(data)
    {
        return createIndex(row, column, priv->index(row));
    } else {
        return QModelIndex();
    }
}

