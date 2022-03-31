// Copyright (c) 2011-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/transactiontablemodel.h>

#include <qt/addresstablemodel.h>
#include <qt/clientmodel.h>
#include <qt/guiconstants.h>
#include <qt/guiutil.h>
#include <qt/optionsmodel.h>
#include <qt/platformstyle.h>
#include <qt/transactiondesc.h>
#include <qt/walletmodel.h>

#include <core_io.h>
#include <interfaces/handler.h>
#include <uint256.h>

#include <algorithm>

#include <QColor>
#include <QDateTime>
#include <QDebug>
#include <QIcon>
#include <QList>


// Amount column is right-aligned it contains numbers
static int column_alignments[] = {
        Qt::AlignLeft|Qt::AlignVCenter, /* status */
        Qt::AlignLeft|Qt::AlignVCenter, /* watchonly */
        Qt::AlignLeft|Qt::AlignVCenter, /* date */
        Qt::AlignLeft|Qt::AlignVCenter, /* type */
        Qt::AlignLeft|Qt::AlignVCenter, /* address */
        Qt::AlignRight|Qt::AlignVCenter /* amount */
    };

// Comparison operator for sort/binary search of model tx list
struct TxLessThan
{
    bool operator()(const WalletTxRecord& a, const WalletTxRecord& b) const
    {
        return a.GetTxHash() < b.GetTxHash();
    }
    bool operator()(const WalletTxRecord& a, const uint256& b) const
    {
        return a.GetTxHash() < b;
    }
    bool operator()(const uint256& a, const WalletTxRecord& b) const
    {
        return a < b.GetTxHash();
    }
};

// queue notifications to show a non freezing progress dialog e.g. for rescan
struct TransactionNotification
{
public:
    TransactionNotification() {}
    TransactionNotification(uint256 _hash, ChangeType _status, bool _showTransaction):
        hash(_hash), status(_status), showTransaction(_showTransaction) {}

    void invoke(QObject *ttm)
    {
        QString strHash = QString::fromStdString(hash.GetHex());
        qDebug() << "NotifyTransactionChanged: " + strHash + " status= " + QString::number(status);
        bool invoked = QMetaObject::invokeMethod(ttm, "updateTransaction", Qt::QueuedConnection,
                                  Q_ARG(QString, strHash),
                                  Q_ARG(int, status),
                                  Q_ARG(bool, showTransaction));
        assert(invoked);
    }
private:
    uint256 hash;
    ChangeType status;
    bool showTransaction;
};

// Private implementation
class TransactionTablePriv
{
public:
    explicit TransactionTablePriv(TransactionTableModel *_parent) :
        parent(_parent)
    {
    }

    TransactionTableModel *parent;

    /* Local cache of wallet.
     * As it is in the same order as the CWallet, by definition
     * this is sorted by sha256.
     */
    QList<WalletTxRecord> cachedWallet;

    bool fQueueNotifications = false;
    std::vector< TransactionNotification > vQueueNotifications;

    void NotifyTransactionChanged(const uint256 &hash, ChangeType status);
    void ShowProgress(const std::string &title, int nProgress);

    /* Query entire wallet anew from core.
     */
    void refreshWallet(interfaces::Wallet& wallet)
    {
        qDebug() << "TransactionTablePriv::refreshWallet";
        cachedWallet.clear();
        {
            for (const auto& wtx : wallet.getWalletTxs()) {
                cachedWallet.append(wtx);
            }
        }
    }

    /* Update our model of the wallet incrementally, to synchronize our model of the wallet
       with that of the core.

       Call with transaction that was added, removed or changed.
     */
    void updateWallet(interfaces::Wallet& wallet, const uint256 &hash, int status, bool showTransaction)
    {
        qDebug() << "TransactionTablePriv::updateWallet: " + QString::fromStdString(hash.ToString()) + " " + QString::number(status);

        // Find bounds of this transaction in model
        QList<WalletTxRecord>::iterator lower = std::lower_bound(
            cachedWallet.begin(), cachedWallet.end(), hash, TxLessThan());
        QList<WalletTxRecord>::iterator upper = std::upper_bound(
            cachedWallet.begin(), cachedWallet.end(), hash, TxLessThan());
        int lowerIndex = (lower - cachedWallet.begin());
        int upperIndex = (upper - cachedWallet.begin());
        bool inModel = (lower != upper);

        if(status == CT_UPDATED)
        {
            if(showTransaction && !inModel)
                status = CT_NEW; /* Not in model, but want to show, treat as new */
            if(!showTransaction && inModel)
                status = CT_DELETED; /* In model, but want to hide, treat as deleted */
        }

        qDebug() << "    inModel=" + QString::number(inModel) +
                    " Index=" + QString::number(lowerIndex) + "-" + QString::number(upperIndex) +
                    " showTransaction=" + QString::number(showTransaction) + " derivedStatus=" + QString::number(status);

        switch(status)
        {
        case CT_NEW:
            if(inModel)
            {
                qWarning() << "TransactionTablePriv::updateWallet: Warning: Got CT_NEW, but transaction is already in model";
                break;
            }
            if(showTransaction)
            {
                // Find transaction in wallet
                std::vector<WalletTxRecord> toInsert = wallet.getWalletTx(hash);
                if (toInsert.empty())
                {
                    qWarning() << "TransactionTablePriv::updateWallet: Warning: Got CT_NEW, but transaction is not in wallet";
                    break;
                }
                // Added -- insert at the right position
                if(!toInsert.empty()) /* only if something to insert */
                {
                    parent->beginInsertRows(QModelIndex(), lowerIndex, lowerIndex+toInsert.size()-1);
                    int insert_idx = lowerIndex;
                    for (const WalletTxRecord& rec : toInsert)
                    {
                        cachedWallet.insert(insert_idx, rec);
                        insert_idx += 1;
                    }
                    parent->endInsertRows();
                }
            }
            break;
        case CT_DELETED:
            if(!inModel)
            {
                qWarning() << "TransactionTablePriv::updateWallet: Warning: Got CT_DELETED, but transaction is not in model";
                break;
            }
            // Removed -- remove entire transaction from table
            parent->beginRemoveRows(QModelIndex(), lowerIndex, upperIndex-1);
            cachedWallet.erase(lower, upper);
            parent->endRemoveRows();
            break;
        case CT_UPDATED:
            // Miscellaneous updates -- nothing to do, status update will take care of this, and is only computed for
            // visible transactions.
            for (int i = lowerIndex; i < upperIndex; i++) {
                WalletTxRecord *rec = &cachedWallet[i];
                rec->status.needsUpdate = true;
            }
            break;
        }
    }

    int size()
    {
        return cachedWallet.size();
    }

    WalletTxRecord* index(interfaces::Wallet& wallet, const uint256& cur_block_hash, const int idx)
    {
        if (idx >= 0 && idx < cachedWallet.size()) {
            WalletTxRecord *rec = &cachedWallet[idx];

            // If a status update is needed (blocks came in since last check),
            // try to update the status of this transaction from the wallet.
            // Otherwise, simply re-use the cached status.
            if (!cur_block_hash.IsNull()) {
                rec->UpdateStatusIfNeeded(cur_block_hash);
            }
            return rec;
        }
        return nullptr;
    }

    QString describe(interfaces::Node& node, interfaces::Wallet& wallet, WalletTxRecord* rec, int unit)
    {
        return TransactionDesc::toHTML(node, wallet, rec, unit);
    }

    QString getTxHex(interfaces::Wallet& wallet, WalletTxRecord *rec)
    {
        auto tx = wallet.getTx(rec->GetTxHash());
        if (tx) {
            std::string strHex = EncodeHexTx(*tx);
            return QString::fromStdString(strHex);
        }
        return QString();
    }
};

TransactionTableModel::TransactionTableModel(const PlatformStyle *_platformStyle, WalletModel *parent):
        QAbstractTableModel(parent),
        walletModel(parent),
        priv(new TransactionTablePriv(this)),
        fProcessingQueuedTransactions(false),
        platformStyle(_platformStyle)
{
    columns << QString() << QString() << tr("Date") << tr("Type") << tr("Label") << BitcoinUnits::getAmountColumnTitle(walletModel->getOptionsModel()->getDisplayUnit());
    priv->refreshWallet(walletModel->wallet());

    connect(walletModel->getOptionsModel(), &OptionsModel::displayUnitChanged, this, &TransactionTableModel::updateDisplayUnit);

    subscribeToCoreSignals();
}

TransactionTableModel::~TransactionTableModel()
{
    unsubscribeFromCoreSignals();
    delete priv;
}

/** Updates the column title to "Amount (DisplayUnit)" and emits headerDataChanged() signal for table headers to react. */
void TransactionTableModel::updateAmountColumnTitle()
{
    columns[Amount] = BitcoinUnits::getAmountColumnTitle(walletModel->getOptionsModel()->getDisplayUnit());
    Q_EMIT headerDataChanged(Qt::Horizontal,Amount,Amount);
}

void TransactionTableModel::updateTransaction(const QString &hash, int status, bool showTransaction)
{
    uint256 updated;
    updated.SetHex(hash.toStdString());

    priv->updateWallet(walletModel->wallet(), updated, status, showTransaction);
}

void TransactionTableModel::updateConfirmations()
{
    // Blocks came in since last poll.
    // Invalidate status (number of confirmations) and (possibly) description
    //  for all rows. Qt is smart enough to only actually request the data for the
    //  visible rows.
    Q_EMIT dataChanged(index(0, Status), index(priv->size()-1, Status));
    Q_EMIT dataChanged(index(0, ToAddress), index(priv->size()-1, ToAddress));
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

QString TransactionTableModel::formatTxStatus(const WalletTxRecord* wtx) const
{
    QString status;

    switch(wtx->status.status)
    {
    case WalletTxStatus::OpenUntilBlock:
        status = tr("Open for %n more block(s)","",wtx->status.open_for);
        break;
    case WalletTxStatus::OpenUntilDate:
        status = tr("Open until %1").arg(GUIUtil::dateTimeStr(wtx->status.open_for));
        break;
    case WalletTxStatus::Unconfirmed:
        status = tr("Unconfirmed");
        break;
    case WalletTxStatus::Abandoned:
        status = tr("Abandoned");
        break;
    case WalletTxStatus::Confirming:
        status = tr("Confirming (%1 of %2 recommended confirmations)").arg(wtx->status.depth).arg(WalletTxRecord::RecommendedNumConfirmations);
        break;
    case WalletTxStatus::Confirmed:
        status = tr("Confirmed (%1 confirmations)").arg(wtx->status.depth);
        break;
    case WalletTxStatus::Conflicted:
        status = tr("Conflicted");
        break;
    case WalletTxStatus::Immature:
        status = tr("Immature (%1 confirmations, will be available after %2)").arg(wtx->status.depth).arg(wtx->status.depth + wtx->status.matures_in);
        break;
    case WalletTxStatus::NotAccepted:
        status = tr("Generated but not accepted");
        break;
    }

    return status;
}

QString TransactionTableModel::formatTxDate(const WalletTxRecord *wtx) const
{
    if(wtx->GetTxTime())
    {
        return GUIUtil::dateTimeStr(wtx->GetTxTime());
    }
    return QString();
}

/* Look up address in address book, if found return label (address)
   otherwise just return (address)
 */
QString TransactionTableModel::lookupAddress(const std::string &address, bool tooltip) const
{
    QString label = walletModel->getAddressTableModel()->labelForAddress(QString::fromStdString(address));
    QString description;
    if(!label.isEmpty())
    {
        description += label;
    }
    if(label.isEmpty() || tooltip)
    {
        description += QString(" (") + QString::fromStdString(address) + QString(")");
    }
    return description;
}

QString TransactionTableModel::formatTxType(const WalletTxRecord *wtx) const
{
    switch(wtx->type)
    {
    case WalletTxRecord::RecvWithAddress:
        return tr("Received with");
    case WalletTxRecord::RecvFromOther:
        return tr("Received from");
    case WalletTxRecord::SendToAddress:
    case WalletTxRecord::SendToOther:
        return tr("Sent to");
    case WalletTxRecord::SendToSelf:
        return tr("Payment to yourself");
    case WalletTxRecord::Generated:
        return tr("Mined");
    default:
        return QString();
    }
}

QVariant TransactionTableModel::txAddressDecoration(const WalletTxRecord *wtx) const
{
    switch(wtx->type)
    {
    case WalletTxRecord::Generated:
        return QIcon(":/icons/tx_mined");
    case WalletTxRecord::RecvWithAddress:
    case WalletTxRecord::RecvFromOther:
        return QIcon(":/icons/tx_input");
    case WalletTxRecord::SendToAddress:
    case WalletTxRecord::SendToOther:
        return QIcon(":/icons/tx_output");
    default:
        return QIcon(":/icons/tx_inout");
    }
}

QString TransactionTableModel::formatTxToAddress(const WalletTxRecord *wtx, bool tooltip) const
{
    QString watchAddress;
    if (tooltip) {
        // Mark transactions involving watch-only addresses by adding " (watch-only)"
        watchAddress = wtx->involvesWatchAddress ? QString(" (") + tr("watch-only") + QString(")") : "";
    }

    switch(wtx->type)
    {
    case WalletTxRecord::RecvFromOther:
        return QString::fromStdString(wtx->address) + watchAddress;
    case WalletTxRecord::RecvWithAddress:
    case WalletTxRecord::SendToAddress:
    case WalletTxRecord::Generated:
        return lookupAddress(wtx->address, tooltip) + watchAddress;
    case WalletTxRecord::SendToOther:
        return QString::fromStdString(wtx->address) + watchAddress;
    case WalletTxRecord::SendToSelf:
        return lookupAddress(wtx->address, tooltip) + watchAddress;
    default:
        return tr("(n/a)") + watchAddress;
    }
}

QVariant TransactionTableModel::addressColor(const WalletTxRecord *wtx) const
{
    // Show addresses without label in a less visible color
    switch(wtx->type)
    {
    case WalletTxRecord::RecvWithAddress:
    case WalletTxRecord::SendToAddress:
    case WalletTxRecord::Generated:
        {
        QString label = walletModel->getAddressTableModel()->labelForAddress(QString::fromStdString(wtx->address));
        if(label.isEmpty())
            return COLOR_BAREADDRESS;
        } break;
    case WalletTxRecord::SendToSelf:
        return COLOR_BAREADDRESS;
    default:
        break;
    }
    return QVariant();
}

QString TransactionTableModel::formatTxAmount(const WalletTxRecord *wtx, bool showUnconfirmed, BitcoinUnits::SeparatorStyle separators) const
{
    QString str = BitcoinUnits::format(walletModel->getOptionsModel()->getDisplayUnit(), wtx->GetNet(), false, separators);
    if(showUnconfirmed)
    {
        if(!wtx->status.countsForBalance)
        {
            str = QString("[") + str + QString("]");
        }
    }
    return QString(str);
}

QVariant TransactionTableModel::txStatusDecoration(const WalletTxRecord *wtx) const
{
    switch(wtx->status.status)
    {
    case WalletTxStatus::OpenUntilBlock:
    case WalletTxStatus::OpenUntilDate:
        return COLOR_TX_STATUS_OPENUNTILDATE;
    case WalletTxStatus::Unconfirmed:
        return QIcon(":/icons/transaction_0");
    case WalletTxStatus::Abandoned:
        return QIcon(":/icons/transaction_abandoned");
    case WalletTxStatus::Confirming:
        switch(wtx->status.depth)
        {
        case 1: return QIcon(":/icons/transaction_1");
        case 2: return QIcon(":/icons/transaction_2");
        case 3: return QIcon(":/icons/transaction_3");
        case 4: return QIcon(":/icons/transaction_4");
        default: return QIcon(":/icons/transaction_5");
        };
    case WalletTxStatus::Confirmed:
        return QIcon(":/icons/transaction_confirmed");
    case WalletTxStatus::Conflicted:
        return QIcon(":/icons/transaction_conflicted");
    case WalletTxStatus::Immature: {
        int total = wtx->status.depth + wtx->status.matures_in;
        int part = (wtx->status.depth * 4 / total) + 1;
        return QIcon(QString(":/icons/transaction_%1").arg(part));
        }
    case WalletTxStatus::NotAccepted:
        return QIcon(":/icons/transaction_0");
    default:
        return COLOR_BLACK;
    }
}

QVariant TransactionTableModel::txWatchonlyDecoration(const WalletTxRecord *wtx) const
{
    if (wtx->involvesWatchAddress)
        return QIcon(":/icons/eye");
    else
        return QVariant();
}

QString TransactionTableModel::formatTooltip(const WalletTxRecord *rec) const
{
    QString tooltip = formatTxStatus(rec) + QString("\n") + formatTxType(rec);
    if(rec->type==WalletTxRecord::RecvFromOther || rec->type==WalletTxRecord::SendToOther ||
       rec->type==WalletTxRecord::SendToAddress || rec->type==WalletTxRecord::RecvWithAddress)
    {
        tooltip += QString(" ") + formatTxToAddress(rec, true);
    }
    return tooltip;
}

QVariant TransactionTableModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();
    WalletTxRecord *rec = static_cast<WalletTxRecord*>(index.internalPointer());

    switch(role)
    {
    case RawDecorationRole:
        switch(index.column())
        {
        case Status:
            return txStatusDecoration(rec);
        case Watchonly:
            return txWatchonlyDecoration(rec);
        case ToAddress:
            return txAddressDecoration(rec);
        }
        break;
    case Qt::DecorationRole:
    {
        QIcon icon = qvariant_cast<QIcon>(index.data(RawDecorationRole));
        return platformStyle->TextColorIcon(icon);
    }
    case Qt::DisplayRole:
        switch(index.column())
        {
        case Date:
            return formatTxDate(rec);
        case Type:
            return formatTxType(rec);
        case ToAddress:
            return formatTxToAddress(rec, false);
        case Amount:
            return formatTxAmount(rec, true, BitcoinUnits::SeparatorStyle::ALWAYS);
        }
        break;
    case Qt::EditRole:
        // Edit role is used for sorting, so return the unformatted values
        switch(index.column())
        {
        case Status:
            return QString::fromStdString(rec->status.sortKey);
        case Date:
            return qint64(rec->GetTxTime());
        case Type:
            return formatTxType(rec);
        case Watchonly:
            return (rec->involvesWatchAddress ? 1 : 0);
        case ToAddress:
            return formatTxToAddress(rec, true);
        case Amount:
            return qint64(rec->GetNet());
        }
        break;
    case Qt::ToolTipRole:
        return formatTooltip(rec);
    case Qt::TextAlignmentRole:
        return column_alignments[index.column()];
    case Qt::ForegroundRole:
        // Use the "danger" color for abandoned transactions
        if(rec->status.status == WalletTxStatus::Abandoned)
        {
            return COLOR_TX_STATUS_DANGER;
        }
        // Non-confirmed (but not immature) as transactions are grey
        if(!rec->status.countsForBalance && rec->status.status != WalletTxStatus::Immature)
        {
            return COLOR_UNCONFIRMED;
        }
        if (index.column() == Amount && rec->GetNet() < 0)
        {
            return COLOR_NEGATIVE;
        }
        if(index.column() == ToAddress)
        {
            return addressColor(rec);
        }
        break;
    case TypeRole:
        return rec->type;
    case DateRole:
        return QDateTime::fromTime_t(static_cast<uint>(rec->GetTxTime()));
    case WatchonlyRole:
        return rec->involvesWatchAddress;
    case WatchonlyDecorationRole:
        return txWatchonlyDecoration(rec);
    case LongDescriptionRole:
        return priv->describe(walletModel->node(), walletModel->wallet(), rec, walletModel->getOptionsModel()->getDisplayUnit());
    case AddressRole:
        return QString::fromStdString(rec->address);
    case LabelRole:
        return walletModel->getAddressTableModel()->labelForAddress(QString::fromStdString(rec->address));
    case AmountRole:
        return qint64(rec->GetNet());
    case TxHashRole:
        return QString::fromStdString(rec->GetTxHash().ToString());
    case TxHexRole:
        return priv->getTxHex(walletModel->wallet(), rec);
    case TxPlainTextRole:
        {
            QString details;
            QDateTime date = QDateTime::fromTime_t(static_cast<uint>(rec->GetTxTime()));
            QString txLabel = walletModel->getAddressTableModel()->labelForAddress(QString::fromStdString(rec->address));

            details.append(date.toString("M/d/yy HH:mm"));
            details.append(" ");
            details.append(formatTxStatus(rec));
            details.append(". ");
            if(!formatTxType(rec).isEmpty()) {
                details.append(formatTxType(rec));
                details.append(" ");
            }
            if(!rec->address.empty()) {
                if(txLabel.isEmpty())
                    details.append(tr("(no label)") + " ");
                else {
                    details.append("(");
                    details.append(txLabel);
                    details.append(") ");
                }
                details.append(QString::fromStdString(rec->address));
                details.append(" ");
            }
            details.append(formatTxAmount(rec, false, BitcoinUnits::SeparatorStyle::NEVER));
            return details;
        }
    case ConfirmedRole:
        return rec->status.status == WalletTxStatus::Status::Confirming || rec->status.status == WalletTxStatus::Status::Confirmed;
    case FormattedAmountRole:
        // Used for copy/export, so don't include separators
        return formatTxAmount(rec, false, BitcoinUnits::SeparatorStyle::NEVER);
    case StatusRole:
        return rec->status.status;
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
            case Watchonly:
                return tr("Whether or not a watch-only address is involved in this transaction.");
            case ToAddress:
                return tr("User-defined intent/purpose of the transaction.");
            case Amount:
                return tr("Amount removed from or added to balance.");
            }
        }
    }
    return QVariant();
}

QModelIndex TransactionTableModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    WalletTxRecord* data = priv->index(walletModel->wallet(), walletModel->getLastBlockProcessed(), row);
    if(data)
    {
        return createIndex(row, column, data);
    }
    return QModelIndex();
}

void TransactionTableModel::updateDisplayUnit()
{
    // emit dataChanged to update Amount column with the current unit
    updateAmountColumnTitle();
    Q_EMIT dataChanged(index(0, Amount), index(priv->size()-1, Amount));
}

void TransactionTablePriv::NotifyTransactionChanged(const uint256 &hash, ChangeType status)
{
    // Find transaction in wallet
    // Determine whether to show transaction or not (determine this here so that no relocking is needed in GUI thread)
    TransactionNotification notification(hash, status, true);

    if (fQueueNotifications)
    {
        vQueueNotifications.push_back(notification);
        return;
    }
    notification.invoke(parent);
}

void TransactionTablePriv::ShowProgress(const std::string &title, int nProgress)
{
    if (nProgress == 0)
        fQueueNotifications = true;

    if (nProgress == 100)
    {
        fQueueNotifications = false;
        if (vQueueNotifications.size() > 10) { // prevent balloon spam, show maximum 10 balloons
            bool invoked = QMetaObject::invokeMethod(parent, "setProcessingQueuedTransactions", Qt::QueuedConnection, Q_ARG(bool, true));
            assert(invoked);
        }
        for (unsigned int i = 0; i < vQueueNotifications.size(); ++i)
        {
            if (vQueueNotifications.size() - i <= 10) {
                bool invoked = QMetaObject::invokeMethod(parent, "setProcessingQueuedTransactions", Qt::QueuedConnection, Q_ARG(bool, false));
                assert(invoked);
            }

            vQueueNotifications[i].invoke(parent);
        }
        vQueueNotifications.clear();
    }
}

void TransactionTableModel::subscribeToCoreSignals()
{
    // Connect signals to wallet
    m_handler_transaction_changed = walletModel->wallet().handleTransactionChanged(std::bind(&TransactionTablePriv::NotifyTransactionChanged, priv, std::placeholders::_1, std::placeholders::_2));
    m_handler_show_progress = walletModel->wallet().handleShowProgress(std::bind(&TransactionTablePriv::ShowProgress, priv, std::placeholders::_1, std::placeholders::_2));
}

void TransactionTableModel::unsubscribeFromCoreSignals()
{
    // Disconnect signals from wallet
    m_handler_transaction_changed->disconnect();
    m_handler_show_progress->disconnect();
}
