#include "transactiontablemodel.h"
#include "guiutil.h"
#include "transactionrecord.h"
#include "guiconstants.h"
#include "transactiondesc.h"

#include "headers.h"

#include <QLocale>
#include <QDebug>
#include <QList>
#include <QColor>
#include <QTimer>
#include <QIcon>
#include <QDateTime>
#include <QtAlgorithms>

// Credit and Debit columns are right-aligned as they contain numbers
static int column_alignments[] = {
        Qt::AlignLeft|Qt::AlignVCenter,
        Qt::AlignLeft|Qt::AlignVCenter,
        Qt::AlignLeft|Qt::AlignVCenter,
        Qt::AlignLeft|Qt::AlignVCenter,
        Qt::AlignRight|Qt::AlignVCenter
    };

// Comparison operator for sort/binary search of model tx list
struct TxLessThan
{
    bool operator()(const TransactionRecord &a, const TransactionRecord &b) const
    {
        return a.hash < b.hash;
    }
    bool operator()(const TransactionRecord &a, const uint256 &b) const
    {
        return a.hash < b;
    }
    bool operator()(const uint256 &a, const TransactionRecord &b) const
    {
        return a < b.hash;
    }
};

// Private implementation
struct TransactionTablePriv
{
    TransactionTablePriv(CWallet *wallet, TransactionTableModel *parent):
            wallet(wallet),
            parent(parent)
    {
    }
    CWallet *wallet;
    TransactionTableModel *parent;

    /* Local cache of wallet.
     * As it is in the same order as the CWallet, by definition
     * this is sorted by sha256.
     */
    QList<TransactionRecord> cachedWallet;

    /* Query entire wallet anew from core.
     */
    void refreshWallet()
    {
#ifdef WALLET_UPDATE_DEBUG
        qDebug() << "refreshWallet";
#endif
        cachedWallet.clear();
        CRITICAL_BLOCK(wallet->cs_mapWallet)
        {
            for(std::map<uint256, CWalletTx>::iterator it = wallet->mapWallet.begin(); it != wallet->mapWallet.end(); ++it)
            {
                cachedWallet.append(TransactionRecord::decomposeTransaction(wallet, it->second));
            }
        }
    }

    /* Update our model of the wallet incrementally, to synchronize our model of the wallet
       with that of the core.

       Call with list of hashes of transactions that were added, removed or changed.
     */
    void updateWallet(const QList<uint256> &updated)
    {
        // Walk through updated transactions, update model as needed.
#ifdef WALLET_UPDATE_DEBUG
        qDebug() << "updateWallet";
#endif
        // Sort update list, and iterate through it in reverse, so that model updates
        //  can be emitted from end to beginning (so that earlier updates will not influence
        // the indices of latter ones).
        QList<uint256> updated_sorted = updated;
        qSort(updated_sorted);

        CRITICAL_BLOCK(wallet->cs_mapWallet)
        {
            for(int update_idx = updated_sorted.size()-1; update_idx >= 0; --update_idx)
            {
                const uint256 &hash = updated_sorted.at(update_idx);
                /* Find transaction in wallet */
                std::map<uint256, CWalletTx>::iterator mi = wallet->mapWallet.find(hash);
                bool inWallet = mi != wallet->mapWallet.end();
                /* Find bounds of this transaction in model */
                QList<TransactionRecord>::iterator lower = qLowerBound(
                    cachedWallet.begin(), cachedWallet.end(), hash, TxLessThan());
                QList<TransactionRecord>::iterator upper = qUpperBound(
                    cachedWallet.begin(), cachedWallet.end(), hash, TxLessThan());
                int lowerIndex = (lower - cachedWallet.begin());
                int upperIndex = (upper - cachedWallet.begin());

                // Determine if transaction is in model already
                bool inModel = false;
                if(lower != upper)
                {
                    inModel = true;
                }

#ifdef WALLET_UPDATE_DEBUG
                qDebug() << "  " << QString::fromStdString(hash.ToString()) << inWallet << " " << inModel
                        << lowerIndex << "-" << upperIndex;
#endif

                if(inWallet && !inModel)
                {
                    // Added -- insert at the right position
                    QList<TransactionRecord> toInsert =
                            TransactionRecord::decomposeTransaction(wallet, mi->second);
                    if(!toInsert.isEmpty()) /* only if something to insert */
                    {
                        parent->beginInsertRows(QModelIndex(), lowerIndex, lowerIndex+toInsert.size()-1);
                        int insert_idx = lowerIndex;
                        foreach(const TransactionRecord &rec, toInsert)
                        {
                            cachedWallet.insert(insert_idx, rec);
                            insert_idx += 1;
                        }
                        parent->endInsertRows();
                    }
                }
                else if(!inWallet && inModel)
                {
                    // Removed -- remove entire transaction from table
                    parent->beginRemoveRows(QModelIndex(), lowerIndex, upperIndex-1);
                    cachedWallet.erase(lower, upper);
                    parent->endRemoveRows();
                }
                else if(inWallet && inModel)
                {
                    // Updated -- nothing to do, status update will take care of this
                }
            }
        }
    }

    int size()
    {
        return cachedWallet.size();
    }

    TransactionRecord *index(int idx)
    {
        if(idx >= 0 && idx < cachedWallet.size())
        {
            TransactionRecord *rec = &cachedWallet[idx];

            // If a status update is needed (blocks came in since last check),
            //  update the status of this transaction from the wallet. Otherwise,
            // simply re-use the cached status.
            if(rec->statusUpdateNeeded())
            {
                CRITICAL_BLOCK(wallet->cs_mapWallet)
                {
                    std::map<uint256, CWalletTx>::iterator mi = wallet->mapWallet.find(rec->hash);

                    if(mi != wallet->mapWallet.end())
                    {
                        rec->updateStatus(mi->second);
                    }
                }
            }
            return rec;
        }
        else
        {
            return 0;
        }
    }

    QString describe(TransactionRecord *rec)
    {
        CRITICAL_BLOCK(wallet->cs_mapWallet)
        {
            std::map<uint256, CWalletTx>::iterator mi = wallet->mapWallet.find(rec->hash);
            if(mi != wallet->mapWallet.end())
            {
                return QString::fromStdString(TransactionDesc::toHTML(wallet, mi->second));
            }
        }
        return QString("");
    }

};

TransactionTableModel::TransactionTableModel(CWallet* wallet, QObject *parent):
        QAbstractTableModel(parent),
        wallet(wallet),
        priv(new TransactionTablePriv(wallet, this))
{
    columns << tr("Status") << tr("Date") << tr("Type") << tr("Address") << tr("Amount");

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

    // Check if there are changes to wallet map
    TRY_CRITICAL_BLOCK(wallet->cs_mapWallet)
    {
        if(!wallet->vWalletUpdated.empty())
        {
            BOOST_FOREACH(uint256 hash, wallet->vWalletUpdated)
            {
                updated.append(hash);
            }
            wallet->vWalletUpdated.clear();
        }
    }

    if(!updated.empty())
    {
        priv->updateWallet(updated);

        // Status (number of confirmations) and (possibly) description
        //  columns changed for all rows.
        emit dataChanged(index(0, Status), index(priv->size()-1, Status));
        emit dataChanged(index(0, ToAddress), index(priv->size()-1, ToAddress));
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
        status = tr("Open until %1").arg(GUIUtil::DateTimeStr(wtx->status.open_for));
        break;
    case TransactionStatus::Offline:
        status = tr("Offline (%1)").arg(wtx->status.depth);
        break;
    case TransactionStatus::Unconfirmed:
        status = tr("Unconfirmed (%1/%2)").arg(wtx->status.depth).arg(TransactionRecord::NumConfirmations);
        break;
    case TransactionStatus::HaveConfirmations:
        status = tr("Confirmed (%1)").arg(wtx->status.depth);
        break;
    }

    return QVariant(status);
}

QVariant TransactionTableModel::formatTxDate(const TransactionRecord *wtx) const
{
    if(wtx->time)
    {
        return QVariant(GUIUtil::DateTimeStr(wtx->time));
    }
    else
    {
        return QVariant();
    }
}

/* Look up label for address in address book, if not found return empty string.
   This should really move to the wallet class.
 */
QString TransactionTableModel::labelForAddress(const std::string &address) const
{
    CRITICAL_BLOCK(wallet->cs_mapAddressBook)
    {
        std::map<std::string, std::string>::iterator mi = wallet->mapAddressBook.find(address);
        if (mi != wallet->mapAddressBook.end())
        {
            return QString::fromStdString(mi->second);
        }
    }
    return QString();
}

/* Look up address in address book, if found return
     address[0:12]... (label)
   otherwise just return address
 */
QString TransactionTableModel::lookupAddress(const std::string &address) const
{
    QString label = labelForAddress(address);
    QString description;
    if(label.isEmpty())
    {
        description = QString::fromStdString(address);
    }
    else
    {
        description = label + QString(" (") + QString::fromStdString(address.substr(0,12)) + QString("...)");
    }
    return description;
}

QVariant TransactionTableModel::formatTxType(const TransactionRecord *wtx) const
{
    QString description;

    switch(wtx->type)
    {
    case TransactionRecord::RecvWithAddress:
        description = tr("Received with");
        break;
    case TransactionRecord::RecvFromIP:
        description = tr("Received from IP");
        break;
    case TransactionRecord::SendToAddress:
        description = tr("Sent to");
        break;
    case TransactionRecord::SendToIP:
        description = tr("Sent to IP");
        break;
    case TransactionRecord::SendToSelf:
        description = tr("Payment to yourself");
        break;
    case TransactionRecord::Generated:
        description = tr("Generated");
        break;
    }
    return QVariant(description);
}

QVariant TransactionTableModel::formatTxToAddress(const TransactionRecord *wtx) const
{
    QString description;

    switch(wtx->type)
    {
    case TransactionRecord::RecvWithAddress:
        description = lookupAddress(wtx->address);
        break;
    case TransactionRecord::RecvFromIP:
        description = QString::fromStdString(wtx->address);
        break;
    case TransactionRecord::SendToAddress:
        description = lookupAddress(wtx->address);
        break;
    case TransactionRecord::SendToIP:
        description = QString::fromStdString(wtx->address);
        break;
    case TransactionRecord::SendToSelf:
        description = QString();
        break;
    case TransactionRecord::Generated:
        switch(wtx->status.maturity)
        {
        case TransactionStatus::Immature:
            description = tr("(matures in %n more blocks)", "",
                           wtx->status.matures_in);
            break;
        case TransactionStatus::Mature:
            description = QString();
            break;
        case TransactionStatus::MaturesWarning:
            description = tr("(Warning: This block was not received by any other nodes and will probably not be accepted!)");
            break;
        case TransactionStatus::NotAccepted:
            description = tr("(not accepted)");
            break;
        }
        break;
    }
    return QVariant(description);
}

QVariant TransactionTableModel::formatTxAmount(const TransactionRecord *wtx) const
{
    QString str = QString::fromStdString(FormatMoney(wtx->credit + wtx->debit));
    if(!wtx->status.confirmed || wtx->status.maturity != TransactionStatus::Mature)
    {
        str = QString("[") + str + QString("]");
    }
    return QVariant(str);
}

QVariant TransactionTableModel::formatTxDecoration(const TransactionRecord *wtx) const
{
    switch(wtx->status.status)
    {
    case TransactionStatus::OpenUntilBlock:
    case TransactionStatus::OpenUntilDate:
        return QColor(64,64,255);
        break;
    case TransactionStatus::Offline:
        return QColor(192,192,192);
    case TransactionStatus::Unconfirmed:
        switch(wtx->status.depth)
        {
        case 0: return QIcon(":/icons/transaction_0");
        case 1: return QIcon(":/icons/transaction_1");
        case 2: return QIcon(":/icons/transaction_2");
        case 3: return QIcon(":/icons/transaction_3");
        case 4: return QIcon(":/icons/transaction_4");
        default: return QIcon(":/icons/transaction_5");
        };
    case TransactionStatus::HaveConfirmations:
        return QIcon(":/icons/transaction_confirmed");
    }
    return QColor(0,0,0);
}

QVariant TransactionTableModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();
    TransactionRecord *rec = static_cast<TransactionRecord*>(index.internalPointer());

    if(role == Qt::DecorationRole)
    {
        if(index.column() == Status)
        {
            return formatTxDecoration(rec);
        }
    }
    else if(role == Qt::DisplayRole)
    {
        // Delegate to specific column handlers
        switch(index.column())
        {
        case Date:
            return formatTxDate(rec);
        case Type:
            return formatTxType(rec);
        case ToAddress:
            return formatTxToAddress(rec);
        case Amount:
            return formatTxAmount(rec);
        }
    }
    else if(role == Qt::EditRole)
    {
        // Edit role is used for sorting so return the real values
        switch(index.column())
        {
        case Status:
            return QString::fromStdString(rec->status.sortKey);
        case Date:
            return rec->time;
        case Type:
            return formatTxType(rec);
        case ToAddress:
            return formatTxToAddress(rec);
        case Amount:
            return rec->credit + rec->debit;
        }
    }
    else if (role == Qt::ToolTipRole)
    {
        if(index.column() == Status)
        {
            return formatTxStatus(rec);
        }
    }
    else if (role == Qt::TextAlignmentRole)
    {
        return column_alignments[index.column()];
    }
    else if (role == Qt::ForegroundRole)
    {
        /* Non-confirmed transactions are grey */
        if(!rec->status.confirmed)
        {
            return QColor(128, 128, 128);
        }
        if(index.column() == Amount && (rec->credit+rec->debit) < 0)
        {
            return QColor(255, 0, 0);
        }
    }
    else if (role == TypeRole)
    {
        return rec->type;
    }
    else if (role == DateRole)
    {
        return QDateTime::fromTime_t(static_cast<uint>(rec->time));
    }
    else if (role == LongDescriptionRole)
    {
        return priv->describe(rec);
    }
    else if (role == AddressRole)
    {
        return QString::fromStdString(rec->address);
    }
    else if (role == LabelRole)
    {
        return labelForAddress(rec->address);
    }
    else if (role == AbsoluteAmountRole)
    {
        return llabs(rec->credit + rec->debit);
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
        }
        else if (role == Qt::TextAlignmentRole)
        {
            return column_alignments[section];
        } else if (role == Qt::ToolTipRole)
        {
            switch(section)
            {
            case Status:
                return tr("Transaction status. Hover over this field to show number of confirmations.");
            case Date:
                return tr("Date and time that the transaction was received.");
            case Type:
                return tr("Type of transaction.");
            case ToAddress:
                return tr("Destination address of transaction.");
            case Amount:
                return tr("Amount removed from or added to balance.");
            }
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
    }
    else
    {
        return QModelIndex();
    }
}

