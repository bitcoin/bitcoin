// Copyright (c) 2011-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/transactiontablemodel.h>

#include <qt/bitcoinunits.h>
#include <qt/clientmodel.h>
#include <qt/guiutil.h>
#include <qt/optionsmodel.h>
#include <qt/transactiondesc.h>
#include <qt/transactionrecord.h>
#include <qt/walletmodel.h>

#include <core_io.h>
#include <interfaces/handler.h>
#include <uint256.h>
#include <util/system.h>

#include <algorithm>
#include <functional>

#include <QColor>
#include <QDateTime>
#include <QDebug>
#include <QIcon>
#include <QLatin1Char>
#include <QLatin1String>
#include <QList>
#include <QMessageBox>


// Amount column is right-aligned it contains numbers
static int column_alignments[] = {
        Qt::AlignLeft|Qt::AlignVCenter, /*status=*/
        Qt::AlignLeft|Qt::AlignVCenter, /*watchonly=*/
        Qt::AlignLeft|Qt::AlignVCenter, /*date=*/
        Qt::AlignLeft|Qt::AlignVCenter, /*type=*/
        Qt::AlignLeft|Qt::AlignVCenter, /*address=*/
        Qt::AlignRight|Qt::AlignVCenter /* amount */
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

// queue notifications to show a non freezing progress dialog e.g. for rescan
struct TransactionNotification
{
public:
    TransactionNotification() = default;
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

    //! Local cache of wallet sorted by transaction hash
    QList<TransactionRecord> cachedWallet;

    /** True when model finishes loading all wallet transactions on start */
    bool m_loaded = false;
    /** True when transactions are being notified, for instance when scanning */
    bool m_loading = false;
    std::vector< TransactionNotification > vQueueNotifications;

    void NotifyTransactionChanged(const uint256 &hash, ChangeType status);
    void NotifyAddressBookChanged(const CTxDestination &address, const std::string &label, bool isMine, const std::string &purpose, ChangeType status);
    void DispatchNotifications();

    /* Query entire wallet anew from core.
     */
    void refreshWallet(interfaces::Wallet& wallet, bool force = false)
    {
        parent->beginResetModel();
        assert(!m_loaded || force);
        cachedWallet.clear();
        try {
            for (const auto& wtx : wallet.getWalletTxs()) {
                if (TransactionRecord::showTransaction()) {
                    cachedWallet.append(TransactionRecord::decomposeTransaction(parent->walletModel->node(), wallet, wtx));
                }
            }
        } catch(const std::exception& e) {
            QMessageBox::critical(nullptr, PACKAGE_NAME, QString("Failed to refresh wallet table: ") + QString::fromStdString(e.what()));
        }
        parent->endResetModel();
        m_loaded = true;
        DispatchNotifications();
    }

    /* Update our model of the wallet incrementally, to synchronize our model of the wallet
       with that of the core.

       Call with transaction that was added, removed or changed.
     */
    void updateWallet(interfaces::Wallet& wallet, const uint256 &hash, int status, bool showTransaction)
    {
        qDebug() << "TransactionTablePriv::updateWallet: " + QString::fromStdString(hash.ToString()) + " " + QString::number(status);

        // Find bounds of this transaction in model
        QList<TransactionRecord>::iterator lower = std::lower_bound(
            cachedWallet.begin(), cachedWallet.end(), hash, TxLessThan());
        QList<TransactionRecord>::iterator upper = std::upper_bound(
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
                interfaces::WalletTx wtx = wallet.getWalletTx(hash);
                if(!wtx.tx)
                {
                    qWarning() << "TransactionTablePriv::updateWallet: Warning: Got CT_NEW, but transaction is not in wallet";
                    break;
                }
                // Added -- insert at the right position
                QList<TransactionRecord> toInsert =
                        TransactionRecord::decomposeTransaction(parent->walletModel->node(), wallet, wtx);
                if(!toInsert.isEmpty()) /* only if something to insert */
                {
                    parent->beginInsertRows(QModelIndex(), lowerIndex, lowerIndex+toInsert.size()-1);
                    int insert_idx = lowerIndex;
                    for (const TransactionRecord &rec : toInsert)
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
                TransactionRecord *rec = &cachedWallet[i];
                rec->status.needsUpdate = true;
            }
            Q_EMIT parent->dataChanged(parent->index(lowerIndex, TransactionTableModel::Status), parent->index(upperIndex, TransactionTableModel::Status));
            break;
        }
    }

    void updateAddressBook(interfaces::Wallet& wallet, const QString& address, const QString& label, bool isMine, const QString& purpose, int status)
    {
        std::string address2 = address.toStdString();
        int index = 0;
        for (auto& rec : cachedWallet) {
            if (rec.strAddress == address2) {
                rec.updateLabel(wallet);
                Q_EMIT parent->dataChanged(parent->index(index, TransactionTableModel::ToAddress), parent->index(index, TransactionTableModel::ToAddress));
            }
            index++;
        }
    }

    int size()
    {
        return cachedWallet.size();
    }

    TransactionRecord* index(interfaces::Wallet& wallet, const uint256& cur_block_hash, const int idx)
    {
        if (idx >= 0 && idx < cachedWallet.size()) {
            TransactionRecord *rec = &cachedWallet[idx];

            // If a status update is needed (blocks came in since last check),
            // try to update the status of this transaction from the wallet.
            // Otherwise, simply re-use the cached status.
            interfaces::WalletTxStatus wtx;
            int numBlocks;
            int64_t block_time;
            if (!cur_block_hash.IsNull() && rec->statusUpdateNeeded(cur_block_hash, parent->getChainLockHeight()) && wallet.tryGetTxStatus(rec->hash, wtx, numBlocks, block_time)) {
                rec->updateStatus(wtx, cur_block_hash, numBlocks, parent->getChainLockHeight(), block_time);
            }
            return rec;
        }
        return nullptr;
    }

    QString describe(interfaces::Node& node, interfaces::Wallet& wallet, TransactionRecord* rec, BitcoinUnit unit)
    {
        return TransactionDesc::toHTML(node, wallet, rec, unit);
    }

    QString getTxHex(interfaces::Wallet& wallet, TransactionRecord *rec)
    {
        auto tx = wallet.getTx(rec->hash);
        if (tx) {
            std::string strHex = EncodeHexTx(*tx);
            return QString::fromStdString(strHex);
        }
        return QString();
    }
};

TransactionTableModel::TransactionTableModel(WalletModel *parent):
        QAbstractTableModel(parent),
        walletModel(parent),
        priv(new TransactionTablePriv(this)),
        fProcessingQueuedTransactions(false),
        cachedChainLockHeight(-1)
{
    subscribeToCoreSignals();

    columns << QString() << QString() << tr("Date") << tr("Type") << tr("Address / Label") << BitcoinUnits::getAmountColumnTitle(walletModel->getOptionsModel()->getDisplayUnit());
    priv->refreshWallet(walletModel->wallet());

    connect(walletModel->getOptionsModel(), &OptionsModel::displayUnitChanged, this, &TransactionTableModel::updateDisplayUnit);
}

TransactionTableModel::~TransactionTableModel()
{
    unsubscribeFromCoreSignals();
    delete priv;
}

void TransactionTableModel::refreshWallet(bool force)
{
    priv->refreshWallet(walletModel->wallet(), force);
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

void TransactionTableModel::updateAddressBook(const QString& address, const QString& label, bool isMine,
                                              const QString& purpose, int status)
{
    priv->updateAddressBook(walletModel->wallet(), address, label, isMine, purpose, status);
}

void TransactionTableModel::updateConfirmations()
{
    // Blocks came in since last poll.
    Q_EMIT dataChanged(QModelIndex(), QModelIndex());
}


void TransactionTableModel::updateChainLockHeight(int chainLockHeight)
{
    cachedChainLockHeight = chainLockHeight;
}

int TransactionTableModel::getChainLockHeight() const
{
    return cachedChainLockHeight;
}

int TransactionTableModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return priv->size();
}

int TransactionTableModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return columns.length();
}

QString TransactionTableModel::formatTxStatus(const TransactionRecord *wtx) const
{
    QString status;

    switch(wtx->status.status)
    {
    case TransactionStatus::Unconfirmed:
        status = tr("Unconfirmed");
        break;
    case TransactionStatus::Abandoned:
        status = tr("Abandoned");
        break;
    case TransactionStatus::Confirming:
        status = tr("Confirming (%1 of %2 recommended confirmations)").arg(wtx->status.depth).arg(TransactionRecord::RecommendedNumConfirmations);
        break;
    case TransactionStatus::Confirmed:
        status = tr("Confirmed (%1 confirmations)").arg(wtx->status.depth);
        break;
    case TransactionStatus::Conflicted:
        status = tr("Conflicted");
        break;
    case TransactionStatus::Immature:
        status = tr("Immature (%1 confirmations, will be available after %2)").arg(wtx->status.depth).arg(wtx->status.depth + wtx->status.matures_in);
        break;
    case TransactionStatus::NotAccepted:
        status = tr("Generated but not accepted");
        break;
    }

    if (wtx->status.lockedByInstantSend) {
        status += ", " + tr("verified via InstantSend");
    }
    if (wtx->status.lockedByChainLocks) {
        status += ", " + tr("locked via ChainLocks");
    }

    return status;
}

QString TransactionTableModel::formatTxDate(const TransactionRecord *wtx) const
{
    if(wtx->time)
    {
        return GUIUtil::dateTimeStr(wtx->time);
    }
    return QString();
}

/* If label is non-empty, return label (address)
   otherwise just return (address)
 */
QString TransactionTableModel::formatAddressLabel(const std::string &address, const QString& label, bool tooltip) const
{
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

QString TransactionTableModel::formatTxType(const TransactionRecord *wtx) const
{
    switch(wtx->type)
    {
    case TransactionRecord::RecvWithAddress:
        return tr("Received with");
    case TransactionRecord::RecvFromOther:
        return tr("Received from");
    case TransactionRecord::RecvWithCoinJoin:
        return tr("Received via %1").arg(QString::fromStdString(gCoinJoinName));
    case TransactionRecord::SendToAddress:
    case TransactionRecord::SendToOther:
        return tr("Sent to");
    case TransactionRecord::SendToSelf:
        return tr("Payment to yourself");
    case TransactionRecord::Generated:
        return tr("Mined");
    case TransactionRecord::PlatformTransfer:
        return tr("Platform Transfer");

    case TransactionRecord::CoinJoinMixing:
        return tr("%1 Mixing").arg(QString::fromStdString(gCoinJoinName));
    case TransactionRecord::CoinJoinCollateralPayment:
        return tr("%1 Collateral Payment").arg(QString::fromStdString(gCoinJoinName));
    case TransactionRecord::CoinJoinMakeCollaterals:
        return tr("%1 Make Collateral Inputs").arg(QString::fromStdString(gCoinJoinName));
    case TransactionRecord::CoinJoinCreateDenominations:
        return tr("%1 Create Denominations").arg(QString::fromStdString(gCoinJoinName));
    case TransactionRecord::CoinJoinSend:
        return tr("%1 Send").arg(QString::fromStdString(gCoinJoinName));

    case TransactionRecord::Other:
        break; // use fail-over here
    } // no default case, so the compiler can warn about missing cases
    return QString();
}

QVariant TransactionTableModel::txAddressDecoration(const TransactionRecord *wtx) const
{
    if (wtx->status.lockedByInstantSend) {
        return GUIUtil::getIcon("transaction_locked", GUIUtil::ThemedColor::BLUE);
    }
    return QVariant();
}

QString TransactionTableModel::formatTxToAddress(const TransactionRecord *wtx, bool tooltip) const
{
    QString watchAddress;
    if (tooltip && wtx->involvesWatchAddress) {
        // Mark transactions involving watch-only addresses by adding " (watch-only)"
        watchAddress = QLatin1String(" (") + tr("watch-only") + QLatin1Char(')');
    }

    switch(wtx->type)
    {
    case TransactionRecord::RecvFromOther:
        return QString::fromStdString(wtx->strAddress) + watchAddress;
    case TransactionRecord::RecvWithAddress:
    case TransactionRecord::RecvWithCoinJoin:
    case TransactionRecord::SendToAddress:
    case TransactionRecord::Generated:
    case TransactionRecord::CoinJoinSend:
    case TransactionRecord::PlatformTransfer:
        return formatAddressLabel(wtx->strAddress, wtx->label, tooltip) + watchAddress;
    case TransactionRecord::SendToOther:
        return QString::fromStdString(wtx->strAddress) + watchAddress;
    case TransactionRecord::SendToSelf:
        return formatAddressLabel(wtx->strAddress, wtx->label, tooltip) + watchAddress;
    case TransactionRecord::CoinJoinMixing:
    case TransactionRecord::CoinJoinCollateralPayment:
    case TransactionRecord::CoinJoinMakeCollaterals:
    case TransactionRecord::CoinJoinCreateDenominations:
    case TransactionRecord::Other:
        break; // use fail-over here
    } // no default case, so the compiler can warn about missing cases
    return tr("(n/a)") + watchAddress;
}

QVariant TransactionTableModel::addressColor(const TransactionRecord *wtx) const
{
    // Show addresses without label in a less visible color
    switch(wtx->type)
    {
    case TransactionRecord::RecvWithAddress:
    case TransactionRecord::SendToAddress:
    case TransactionRecord::Generated:
    case TransactionRecord::PlatformTransfer:
    case TransactionRecord::CoinJoinSend:
    case TransactionRecord::RecvWithCoinJoin:
        {
        if (wtx->label.isEmpty()) {
            return GUIUtil::getThemedQColor(GUIUtil::ThemedColor::BAREADDRESS);
        }
        } break;
    case TransactionRecord::SendToSelf:
    case TransactionRecord::CoinJoinCreateDenominations:
    case TransactionRecord::CoinJoinMixing:
    case TransactionRecord::CoinJoinMakeCollaterals:
    case TransactionRecord::CoinJoinCollateralPayment:
        return GUIUtil::getThemedQColor(GUIUtil::ThemedColor::BAREADDRESS);
    case TransactionRecord::SendToOther:
    case TransactionRecord::RecvFromOther:
    case TransactionRecord::Other:
        break;
    } // no default case, so the compiler can warn about missing cases
    return GUIUtil::getThemedQColor(GUIUtil::ThemedColor::DEFAULT);
}

QString TransactionTableModel::formatTxAmount(const TransactionRecord *wtx, bool showUnconfirmed, BitcoinUnits::SeparatorStyle separators) const
{
    QString str = BitcoinUnits::format(walletModel->getOptionsModel()->getDisplayUnit(), wtx->credit + wtx->debit, false, separators);
    if(showUnconfirmed)
    {
        if(!wtx->status.countsForBalance)
        {
            str = QString("[") + str + QString("]");
        }
    }
    return QString(str);
}

QVariant TransactionTableModel::amountColor(const TransactionRecord *rec) const
{
    switch (rec->type) {
    case TransactionRecord::Generated:
    case TransactionRecord::RecvWithCoinJoin:
    case TransactionRecord::RecvWithAddress:
    case TransactionRecord::RecvFromOther:
    case TransactionRecord::PlatformTransfer:
        return GUIUtil::getThemedQColor(GUIUtil::ThemedColor::GREEN);
    case TransactionRecord::CoinJoinSend:
    case TransactionRecord::SendToAddress:
    case TransactionRecord::SendToOther:
    case TransactionRecord::Other:
        return GUIUtil::getThemedQColor(GUIUtil::ThemedColor::RED);
    case TransactionRecord::SendToSelf:
    case TransactionRecord::CoinJoinMixing:
    case TransactionRecord::CoinJoinCollateralPayment:
    case TransactionRecord::CoinJoinMakeCollaterals:
    case TransactionRecord::CoinJoinCreateDenominations:
        return GUIUtil::getThemedQColor(GUIUtil::ThemedColor::ORANGE);
    }
    return GUIUtil::getThemedQColor(GUIUtil::ThemedColor::DEFAULT);
}

QVariant TransactionTableModel::txStatusDecoration(const TransactionRecord *wtx) const
{
    switch(wtx->status.status)
    {
    case TransactionStatus::Unconfirmed:
        return GUIUtil::getIcon("transaction_0");
    case TransactionStatus::Abandoned:
        return GUIUtil::getIcon("transaction_abandoned", GUIUtil::ThemedColor::RED);
    case TransactionStatus::Confirming:
        switch(wtx->status.depth)
        {
        case 1: return GUIUtil::getIcon("transaction_1", GUIUtil::ThemedColor::ORANGE);
        case 2: return GUIUtil::getIcon("transaction_2", GUIUtil::ThemedColor::ORANGE);
        case 3: return GUIUtil::getIcon("transaction_3", GUIUtil::ThemedColor::ORANGE);
        case 4: return GUIUtil::getIcon("transaction_4", GUIUtil::ThemedColor::ORANGE);
        default: return GUIUtil::getIcon("transaction_5", GUIUtil::ThemedColor::ORANGE);
        };
    case TransactionStatus::Confirmed:
        return GUIUtil::getIcon("synced", GUIUtil::ThemedColor::GREEN);
    case TransactionStatus::Conflicted:
        return GUIUtil::getIcon("transaction_0", GUIUtil::ThemedColor::RED, GUIUtil::ThemedColor::RED);
    case TransactionStatus::Immature: {
        int total = wtx->status.depth + wtx->status.matures_in;
        int part = (wtx->status.depth * 5 / total) + 1;
        return GUIUtil::getIcon(QString("transaction_%1").arg(part), GUIUtil::ThemedColor::ORANGE);
        }
    case TransactionStatus::NotAccepted:
        return GUIUtil::getIcon("transaction_0", GUIUtil::ThemedColor::RED);
    default:
        return GUIUtil::getThemedQColor(GUIUtil::ThemedColor::DEFAULT);
    }
}

QVariant TransactionTableModel::txWatchonlyDecoration(const TransactionRecord *wtx) const
{
    if (wtx->involvesWatchAddress)
        return GUIUtil::getIcon("eye");
    else
        return QVariant();
}

QString TransactionTableModel::formatTooltip(const TransactionRecord *rec) const
{
    QString tooltip = formatTxStatus(rec) + QString("\n") + formatTxType(rec);
    if(rec->type==TransactionRecord::RecvFromOther || rec->type==TransactionRecord::SendToOther ||
       rec->type==TransactionRecord::SendToAddress || rec->type==TransactionRecord::RecvWithAddress)
    {
        tooltip += QString(" ") + formatTxToAddress(rec, true);
    }
    return tooltip;
}

QVariant TransactionTableModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();
    TransactionRecord *rec = static_cast<TransactionRecord*>(index.internalPointer());

    const auto column = static_cast<ColumnIndex>(index.column());
    switch (role) {
    case RawDecorationRole:
        switch (column) {
        case Status:
            return txStatusDecoration(rec);
        case Watchonly:
            return txWatchonlyDecoration(rec);
        case Date: return {};
        case Type: return {};
        case ToAddress:
            return txAddressDecoration(rec);
        case Amount: return {};
        } // no default case, so the compiler can warn about missing cases
        assert(false);
    case Qt::DecorationRole:
    {
        return qvariant_cast<QIcon>(index.data(RawDecorationRole));
    }
    case Qt::DisplayRole:
        switch (column) {
        case Status: return {};
        case Watchonly: return {};
        case Date:
            return formatTxDate(rec);
        case Type:
            return formatTxType(rec);
        case ToAddress:
            return formatTxToAddress(rec, false);
        case Amount:
            return formatTxAmount(rec, true, BitcoinUnits::SeparatorStyle::ALWAYS);
        } // no default case, so the compiler can warn about missing cases
        assert(false);
    case Qt::EditRole:
        // Edit role is used for sorting, so return the unformatted values
        switch (column) {
        case Status:
            return QString::fromStdString(rec->status.sortKey);
        case Date:
            return rec->time;
        case Type:
            return formatTxType(rec);
        case Watchonly:
            return (rec->involvesWatchAddress ? 1 : 0);
        case ToAddress:
            return formatTxToAddress(rec, true);
        case Amount:
            return qint64(rec->credit + rec->debit);
        } // no default case, so the compiler can warn about missing cases
        assert(false);
    case Qt::ToolTipRole:
        return formatTooltip(rec);
    case Qt::TextAlignmentRole:
        return column_alignments[index.column()];
    case Qt::ForegroundRole:
        // Non-confirmed (but not immature) as transactions are grey
        if(!rec->status.countsForBalance && rec->status.status != TransactionStatus::Immature)
        {
            return GUIUtil::getThemedQColor(GUIUtil::ThemedColor::UNCONFIRMED);
        }
        if (index.column() == Amount) {
            return amountColor(rec);
        }
        if(index.column() == ToAddress)
        {
            return addressColor(rec);
        }
        return GUIUtil::getThemedQColor(GUIUtil::ThemedColor::DEFAULT);
    case TypeRole:
        return rec->type;
    case DateRole:
        return QDateTime::fromSecsSinceEpoch(rec->time);
    case DateRoleInt:
        return qint64(rec->time);
    case WatchonlyRole:
        return rec->involvesWatchAddress;
    case WatchonlyDecorationRole:
        return txWatchonlyDecoration(rec);
    case LongDescriptionRole:
        return priv->describe(walletModel->node(), walletModel->wallet(), rec, walletModel->getOptionsModel()->getDisplayUnit());
    case AddressRole:
        return QString::fromStdString(rec->strAddress);
    case LabelRole:
        return rec->label;
    case AmountRole:
        return qint64(rec->credit + rec->debit);
    case TxHashRole:
        return rec->getTxHash();
    case TxHexRole:
        return priv->getTxHex(walletModel->wallet(), rec);
    case TxPlainTextRole:
        {
            QString details;
            QString txLabel = rec->label;

            details.append(formatTxDate(rec));
            details.append(" ");
            details.append(formatTxStatus(rec));
            details.append(". ");
            if(!formatTxType(rec).isEmpty()) {
                details.append(formatTxType(rec));
                details.append(" ");
            }
            if(!rec->strAddress.empty()) {
                if(txLabel.isEmpty())
                    details.append(tr("(no label)") + " ");
                else {
                    details.append("(");
                    details.append(txLabel);
                    details.append(") ");
                }
                details.append(QString::fromStdString(rec->strAddress));
                details.append(" ");
            }
            details.append(formatTxAmount(rec, false, BitcoinUnits::SeparatorStyle::NEVER));
            return details;
        }
    case ConfirmedRole:
        return rec->status.status == TransactionStatus::Status::Confirming || rec->status.status == TransactionStatus::Status::Confirmed;
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
    TransactionRecord *data = priv->index(walletModel->wallet(), walletModel->getLastBlockProcessed(), row);
    if (data) {
        return createIndex(row, column, data);
    }
    return QModelIndex();
}

void TransactionTableModel::updateDisplayUnit()
{
    // emit dataChanged to update Amount column with the current unit
    updateAmountColumnTitle();
    Q_EMIT dataChanged(QModelIndex(), QModelIndex());
}

void TransactionTablePriv::NotifyTransactionChanged(const uint256 &hash, ChangeType status)
{
    // Find transaction in wallet
    // Determine whether to show transaction or not (determine this here so that no relocking is needed in GUI thread)
    bool showTransaction = TransactionRecord::showTransaction();

    TransactionNotification notification(hash, status, showTransaction);

    if (!m_loaded || m_loading)
    {
        vQueueNotifications.push_back(notification);
        return;
    }
    notification.invoke(parent);
}

void TransactionTablePriv::NotifyAddressBookChanged(const CTxDestination &address, const std::string &label, bool isMine, const std::string &purpose, ChangeType status)
{
    bool invoked = QMetaObject::invokeMethod(parent, "updateAddressBook", Qt::QueuedConnection,
                              Q_ARG(QString, QString::fromStdString(EncodeDestination(address))),
                              Q_ARG(QString, QString::fromStdString(label)),
                              Q_ARG(bool, isMine),
                              Q_ARG(QString, QString::fromStdString(purpose)),
                              Q_ARG(int, (int)status));
    assert(invoked);
}

void TransactionTablePriv::DispatchNotifications()
{
    if (!m_loaded || m_loading) return;

    if (vQueueNotifications.size() < 10000) {
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
    } else {
        // it's much faster to just drop all the queued notifications and refresh the whole thing instead
        vQueueNotifications.clear();
        bool invoked = QMetaObject::invokeMethod(parent, "refreshWallet", Qt::QueuedConnection, Q_ARG(bool, true));
        assert(invoked);
    }
}

void TransactionTableModel::subscribeToCoreSignals()
{
    // Connect signals to wallet
    m_handler_transaction_changed = walletModel->wallet().handleTransactionChanged(std::bind(&TransactionTablePriv::NotifyTransactionChanged, priv, std::placeholders::_1, std::placeholders::_2));
    m_handler_address_book_changed = walletModel->wallet().handleAddressBookChanged(std::bind(&TransactionTablePriv::NotifyAddressBookChanged, priv, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
    m_handler_show_progress = walletModel->wallet().handleShowProgress([this](const std::string&, int progress) {
        priv->m_loading = progress < 100;
        priv->DispatchNotifications();
    });
}

void TransactionTableModel::unsubscribeFromCoreSignals()
{
    // Disconnect signals from wallet
    m_handler_transaction_changed->disconnect();
    m_handler_address_book_changed->disconnect();
    m_handler_show_progress->disconnect();
}
