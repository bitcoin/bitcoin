// Copyright (c) 2011-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "transactiontablemodel.h"

#include "addresstablemodel.h"
#include "bitcreditunits.h"
#include "guiconstants.h"
#include "guiutil.h"
#include "optionsmodel.h"
#include "transactiondesc.h"
#include "transactionrecord.h"
#include "walletmodel.h"

#include "main.h"
#include "sync.h"
#include "uint256.h"
#include "util.h"
#include "wallet.h"

#include <QColor>
#include <QDateTime>
#include <QDebug>
#include <QIcon>
#include <QList>

// Amount column is right-aligned it contains numbers
static int column_alignments[] = {
        Qt::AlignLeft|Qt::AlignVCenter, /* status */
        Qt::AlignLeft|Qt::AlignVCenter, /* date */
        Qt::AlignLeft|Qt::AlignVCenter, /* type */
        Qt::AlignLeft|Qt::AlignVCenter, /* address */
        Qt::AlignRight|Qt::AlignVCenter /* amount */
    };

// Comparison operator for sort/binary search of model tx list
struct Bitcredit_TxLessThan
{
    bool operator()(const Bitcredit_TransactionRecord &a, const Bitcredit_TransactionRecord &b) const
    {
        return a.hash < b.hash;
    }
    bool operator()(const Bitcredit_TransactionRecord &a, const uint256 &b) const
    {
        return a.hash < b;
    }
    bool operator()(const uint256 &a, const Bitcredit_TransactionRecord &b) const
    {
        return a < b.hash;
    }
};

// Private implementation
class Bitcredit_TransactionTablePriv
{
public:
    Bitcredit_TransactionTablePriv(Bitcredit_CWallet *bitcredit_wallet, Bitcredit_CWallet *keyholder_wallet, Bitcredit_TransactionTableModel *parent) :
        bitcredit_wallet(bitcredit_wallet),
		keyholder_wallet(keyholder_wallet),
        parent(parent)
    {
    }

    Bitcredit_CWallet *bitcredit_wallet;
    Bitcredit_CWallet *keyholder_wallet;
    Bitcredit_TransactionTableModel *parent;

    /* Local cache of wallet.
     * As it is in the same order as the CWallet, by definition
     * this is sorted by sha256.
     */
    QList<Bitcredit_TransactionRecord> cachedWallet;

    /* Query entire wallet anew from core.
     */
    void refreshWallet()
    {
        qDebug() << "TransactionTablePriv::refreshWallet";
        cachedWallet.clear();
        {
            LOCK2(bitcredit_mainState.cs_main, bitcredit_wallet->cs_wallet);
            for(std::map<uint256, Bitcredit_CWalletTx>::iterator it = bitcredit_wallet->mapWallet.begin(); it != bitcredit_wallet->mapWallet.end(); ++it)
            {
                if(Bitcredit_TransactionRecord::showTransaction(it->second))
                    cachedWallet.append(Bitcredit_TransactionRecord::decomposeTransaction(keyholder_wallet, it->second));
            }
        }
    }

    /* Update our model of the wallet incrementally, to synchronize our model of the wallet
       with that of the core.

       Call with transaction that was added, removed or changed.
     */
    void updateWallet(const uint256 &hash, int status)
    {
        qDebug() << "TransactionTablePriv::updateWallet : " + QString::fromStdString(hash.ToString()) + " " + QString::number(status);
        {
            LOCK2(bitcredit_mainState.cs_main, bitcredit_wallet->cs_wallet);

            // Find transaction in wallet
            std::map<uint256, Bitcredit_CWalletTx>::iterator mi = bitcredit_wallet->mapWallet.find(hash);
            bool inWallet = mi != bitcredit_wallet->mapWallet.end();

            // Find bounds of this transaction in model
            QList<Bitcredit_TransactionRecord>::iterator lower = qLowerBound(
                cachedWallet.begin(), cachedWallet.end(), hash, Bitcredit_TxLessThan());
            QList<Bitcredit_TransactionRecord>::iterator upper = qUpperBound(
                cachedWallet.begin(), cachedWallet.end(), hash, Bitcredit_TxLessThan());
            int lowerIndex = (lower - cachedWallet.begin());
            int upperIndex = (upper - cachedWallet.begin());
            bool inModel = (lower != upper);

            // Determine whether to show transaction or not
            bool showTransaction = (inWallet && Bitcredit_TransactionRecord::showTransaction(mi->second));

            if(status == CT_UPDATED)
            {
                if(showTransaction && !inModel)
                    status = CT_NEW; /* Not in model, but want to show, treat as new */
                if(!showTransaction && inModel)
                    status = CT_DELETED; /* In model, but want to hide, treat as deleted */
            }

            qDebug() << "   inWallet=" + QString::number(inWallet) + " inModel=" + QString::number(inModel) +
                        " Index=" + QString::number(lowerIndex) + "-" + QString::number(upperIndex) +
                        " showTransaction=" + QString::number(showTransaction) + " derivedStatus=" + QString::number(status);

            switch(status)
            {
            case CT_NEW:
                if(inModel)
                {
                    qDebug() << "TransactionTablePriv::updateWallet : Warning: Got CT_NEW, but transaction is already in model";
                    break;
                }
                if(!inWallet)
                {
                    qDebug() << "TransactionTablePriv::updateWallet : Warning: Got CT_NEW, but transaction is not in wallet";
                    break;
                }
                if(showTransaction)
                {
                    // Added -- insert at the right position
                    QList<Bitcredit_TransactionRecord> toInsert =
                            Bitcredit_TransactionRecord::decomposeTransaction(keyholder_wallet, mi->second);
                    if(!toInsert.isEmpty()) /* only if something to insert */
                    {
                        parent->beginInsertRows(QModelIndex(), lowerIndex, lowerIndex+toInsert.size()-1);
                        int insert_idx = lowerIndex;
                        foreach(const Bitcredit_TransactionRecord &rec, toInsert)
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
                    qDebug() << "TransactionTablePriv::updateWallet : Warning: Got CT_DELETED, but transaction is not in model";
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
                break;
            }
        }
    }

    int size()
    {
        return cachedWallet.size();
    }

    Bitcredit_TransactionRecord *index(int idx)
    {
        if(idx >= 0 && idx < cachedWallet.size())
        {
            Bitcredit_TransactionRecord *rec = &cachedWallet[idx];

            // Get required locks upfront. This avoids the GUI from getting
            // stuck if the core is holding the locks for a longer time - for
            // example, during a wallet rescan.
            //
            // If a status update is needed (blocks came in since last check),
            //  update the status of this transaction from the wallet. Otherwise,
            // simply re-use the cached status.
            TRY_LOCK(bitcredit_mainState.cs_main, lockMain);
            if(lockMain)
            {
                TRY_LOCK(bitcredit_wallet->cs_wallet, lockWallet);
                if(lockWallet && rec->statusUpdateNeeded())
                {
                    std::map<uint256, Bitcredit_CWalletTx>::iterator mi = bitcredit_wallet->mapWallet.find(rec->hash);

                    if(mi != bitcredit_wallet->mapWallet.end())
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

    QString describe(Bitcredit_TransactionRecord *rec, int unit)
    {
        {
            LOCK2(bitcredit_mainState.cs_main, bitcredit_wallet->cs_wallet);
            std::map<uint256, Bitcredit_CWalletTx>::iterator mi = bitcredit_wallet->mapWallet.find(rec->hash);
            if(mi != bitcredit_wallet->mapWallet.end())
            {
                return Bitcredit_TransactionDesc::toHTML(keyholder_wallet, mi->second, rec, unit);
            }
        }
        return QString("");
    }
};

Bitcredit_TransactionTableModel::Bitcredit_TransactionTableModel(Bitcredit_CWallet* bitcredit_wallet, Bitcredit_CWallet *keyholder_wallet, bool isForDepositWalletIn, Bitcredit_WalletModel *parent):
        QAbstractTableModel(parent),
        wallet(bitcredit_wallet),
        keyholder_wallet(keyholder_wallet),
        walletModel(parent),
        priv(new Bitcredit_TransactionTablePriv(bitcredit_wallet, keyholder_wallet, this))
{
	//This means we are in the normal bitcredit_wallet, otherwise in the deposit_wallet
	isForDepositWallet = isForDepositWalletIn;
	if(isForDepositWallet) {
		columns << QString() << tr("Date") << tr("Type") << tr("Address") << tr("Deposit");
	} else {
		columns << QString() << tr("Date") << tr("Type") << tr("Address") << tr("Amount");
	}
    priv->refreshWallet();

    connect(walletModel->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));
}

Bitcredit_TransactionTableModel::~Bitcredit_TransactionTableModel()
{
    delete priv;
}

void Bitcredit_TransactionTableModel::updateTransaction(const QString &hash, int status)
{
    uint256 updated;
    updated.SetHex(hash.toStdString());

    priv->updateWallet(updated, status);
}

void Bitcredit_TransactionTableModel::updateConfirmations()
{
    // Blocks came in since last poll.
    // Invalidate status (number of confirmations) and (possibly) description
    //  for all rows. Qt is smart enough to only actually request the data for the
    //  visible rows.
    emit dataChanged(index(0, Status), index(priv->size()-1, Status));
    emit dataChanged(index(0, ToAddress), index(priv->size()-1, ToAddress));
}

int Bitcredit_TransactionTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return priv->size();
}

int Bitcredit_TransactionTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return columns.length();
}

QString Bitcredit_TransactionTableModel::formatTxStatus(const Bitcredit_TransactionRecord *wtx) const
{
    QString status;

    switch(wtx->status.status)
    {
    case Bitcredit_TransactionStatus::OpenUntilBlock:
        status = tr("Open for %n more block(s)","",wtx->status.open_for);
        break;
    case Bitcredit_TransactionStatus::OpenUntilDate:
        status = tr("Open until %1").arg(GUIUtil::dateTimeStr(wtx->status.open_for));
        break;
    case Bitcredit_TransactionStatus::Offline:
        status = tr("Offline");
        break;
    case Bitcredit_TransactionStatus::Unconfirmed:
        status = tr("Unconfirmed");
        break;
    case Bitcredit_TransactionStatus::Confirming:
        status = tr("Confirming (%1 of %2 recommended confirmations)").arg(wtx->status.depth).arg(Bitcredit_TransactionRecord::RecommendedNumConfirmations);
        break;
    case Bitcredit_TransactionStatus::Confirmed:
        status = tr("Confirmed (%1 confirmations)").arg(wtx->status.depth);
        break;
    case Bitcredit_TransactionStatus::Conflicted:
        status = tr("Conflicted");
        break;
    case Bitcredit_TransactionStatus::Immature:
        status = tr("Immature (%1 confirmations, will be available after %2)").arg(wtx->status.depth).arg(wtx->status.depth + wtx->status.matures_in);
        break;
    case Bitcredit_TransactionStatus::ImmatureDeposit:
        status = tr("In deposit (%1 confirmations, will be available after %2)").arg(wtx->status.depth).arg(wtx->status.depth + wtx->status.matures_in);
        break;
    case Bitcredit_TransactionStatus::ImmatureDepositChange:
        status = tr("Deposit change (%1 confirmations, will be available after %2)").arg(wtx->status.depth).arg(wtx->status.depth + wtx->status.matures_in);
        break;
    case Bitcredit_TransactionStatus::MaturesWarning:
        status = tr("This block was not received by any other nodes and will probably not be accepted!");
        break;
    case Bitcredit_TransactionStatus::NotAccepted:
        status = tr("Generated but not accepted");
        break;
    }

    return status;
}

QString Bitcredit_TransactionTableModel::formatTxDate(const Bitcredit_TransactionRecord *wtx) const
{
    if(wtx->time)
    {
        return GUIUtil::dateTimeStr(wtx->time);
    }
    else
    {
        return QString();
    }
}

/* Look up address in address book, if found return label (address)
   otherwise just return (address)
 */
QString Bitcredit_TransactionTableModel::lookupAddress(const std::string &address, bool tooltip) const
{
    QString label = walletModel->getAddressTableModel()->labelForAddress(QString::fromStdString(address));
    QString description;
    if(!label.isEmpty())
    {
        description += label + QString(" ");
    }
    if(label.isEmpty() || walletModel->getOptionsModel()->getDisplayAddresses() || tooltip)
    {
        description += QString("(") + QString::fromStdString(address) + QString(")");
    }
    return description;
}

QString Bitcredit_TransactionTableModel::formatTxType(const Bitcredit_TransactionRecord *wtx) const
{
    switch(wtx->type)
    {
    case Bitcredit_TransactionRecord::RecvWithAddress:
        return tr("Received with");
    case Bitcredit_TransactionRecord::RecvFromOther:
        return tr("Received from");
    case Bitcredit_TransactionRecord::SendToAddress:
    case Bitcredit_TransactionRecord::SendToOther:
        return tr("Sent to");
    case Bitcredit_TransactionRecord::SendToSelf:
        return tr("Payment to yourself");
    case Bitcredit_TransactionRecord::Generated:
        return tr("Mined");
    case Bitcredit_TransactionRecord::Deposit:
        return tr("Deposit");
    case Bitcredit_TransactionRecord::DepositChange:
        return tr("Deposit change");
    default:
        return QString();
    }
}

QVariant Bitcredit_TransactionTableModel::txAddressDecoration(const Bitcredit_TransactionRecord *wtx) const
{
    switch(wtx->type)
    {
    case Bitcredit_TransactionRecord::Generated:
        return QIcon(":/icons/tx_mined");
    case Bitcredit_TransactionRecord::Deposit:
        return QIcon(":/icons/tx_deposit");
    case Bitcredit_TransactionRecord::DepositChange:
        return QIcon(":/icons/tx_deposit");
    case Bitcredit_TransactionRecord::RecvWithAddress:
    case Bitcredit_TransactionRecord::RecvFromOther:
        return QIcon(":/icons/tx_input");
    case Bitcredit_TransactionRecord::SendToAddress:
    case Bitcredit_TransactionRecord::SendToOther:
        return QIcon(":/icons/tx_output");
    default:
        return QIcon(":/icons/tx_inout");
    }
    return QVariant();
}

QString Bitcredit_TransactionTableModel::formatTxToAddress(const Bitcredit_TransactionRecord *wtx, bool tooltip) const
{
    switch(wtx->type)
    {
    case Bitcredit_TransactionRecord::RecvFromOther:
        return QString::fromStdString(wtx->address);
    case Bitcredit_TransactionRecord::RecvWithAddress:
    case Bitcredit_TransactionRecord::SendToAddress:
    case Bitcredit_TransactionRecord::Generated:
        return lookupAddress(wtx->address, tooltip);
    case Bitcredit_TransactionRecord::Deposit:
        return lookupAddress(wtx->address, tooltip);
    case Bitcredit_TransactionRecord::DepositChange:
        return lookupAddress(wtx->address, tooltip);
    case Bitcredit_TransactionRecord::SendToOther:
        return QString::fromStdString(wtx->address);
    case Bitcredit_TransactionRecord::SendToSelf:
    default:
        return tr("(n/a)");
    }
}

QVariant Bitcredit_TransactionTableModel::addressColor(const Bitcredit_TransactionRecord *wtx) const
{
    // Show addresses without label in a less visible color
    switch(wtx->type)
    {
    case Bitcredit_TransactionRecord::RecvWithAddress:
    case Bitcredit_TransactionRecord::SendToAddress:
    case Bitcredit_TransactionRecord::Generated:
        {
        QString label = walletModel->getAddressTableModel()->labelForAddress(QString::fromStdString(wtx->address));
        if(label.isEmpty())
            return COLOR_BAREADDRESS;
        } break;
    case Bitcredit_TransactionRecord::Deposit:
        {
        QString label = walletModel->getAddressTableModel()->labelForAddress(QString::fromStdString(wtx->address));
        if(label.isEmpty())
            return COLOR_BAREADDRESS;
        } break;
    case Bitcredit_TransactionRecord::DepositChange:
        {
        QString label = walletModel->getAddressTableModel()->labelForAddress(QString::fromStdString(wtx->address));
        if(label.isEmpty())
            return COLOR_BAREADDRESS;
        } break;
    case Bitcredit_TransactionRecord::SendToSelf:
        return COLOR_BAREADDRESS;
    default:
        break;
    }
    return QVariant();
}

QString Bitcredit_TransactionTableModel::formatTxAmount(const Bitcredit_TransactionRecord *wtx, bool showUnconfirmed) const
{
    QString str = BitcreditUnits::format(walletModel->getOptionsModel()->getDisplayUnit(), wtx->credit + wtx->debit);
    if(showUnconfirmed)
    {
        if(!wtx->status.countsForBalance)
        {
            str = QString("[") + str + QString("]");
        }
    }
    return QString(str);
}
QString Bitcredit_TransactionTableModel::formatTxAmountWithDeposit(const Bitcredit_TransactionRecord *wtx, bool showUnconfirmed) const
{
    QString str = BitcreditUnits::format(walletModel->getOptionsModel()->getDisplayUnit(), wtx->credit + wtx->debit);
    QString strCredit = BitcreditUnits::format(walletModel->getOptionsModel()->getDisplayUnit(), wtx->credit);
    if(showUnconfirmed)
    {
        if(!wtx->status.countsForBalance)
        {
            str = QString("[") + str + QString("]");
        }
    }
    return QString(QString("(") + strCredit + QString(") ") + str);
}
QString Bitcredit_TransactionTableModel::formatTxDepositAmount(const Bitcredit_TransactionRecord *wtx) const
{
    QString str = BitcreditUnits::format(walletModel->getOptionsModel()->getDisplayUnit(), wtx->credit);
	str = QString("[") + str + QString("]");
    return QString(str);
}

QVariant Bitcredit_TransactionTableModel::txStatusDecoration(const Bitcredit_TransactionRecord *wtx) const
{
    switch(wtx->status.status)
    {
    case Bitcredit_TransactionStatus::OpenUntilBlock:
    case Bitcredit_TransactionStatus::OpenUntilDate:
        return QColor(64,64,255);
    case Bitcredit_TransactionStatus::Offline:
        return QColor(192,192,192);
    case Bitcredit_TransactionStatus::Unconfirmed:
        return QIcon(":/icons/transaction_0");
    case Bitcredit_TransactionStatus::Confirming:
        switch(wtx->status.depth)
        {
        case 1: return QIcon(":/icons/transaction_1");
        case 2: return QIcon(":/icons/transaction_2");
        case 3: return QIcon(":/icons/transaction_3");
        case 4: return QIcon(":/icons/transaction_4");
        default: return QIcon(":/icons/transaction_5");
        };
    case Bitcredit_TransactionStatus::Confirmed:
        return QIcon(":/icons/transaction_confirmed");
    case Bitcredit_TransactionStatus::Conflicted:
        return QIcon(":/icons/transaction_conflicted");
    case Bitcredit_TransactionStatus::Immature: {
        int total = wtx->status.depth + wtx->status.matures_in;
        int part = (wtx->status.depth * 4 / total) + 1;
        return QIcon(QString(":/icons/transaction_%1").arg(part));
        }
    case Bitcredit_TransactionStatus::ImmatureDeposit: {
        int total = wtx->status.depth + wtx->status.matures_in;
        int part = (wtx->status.depth * 4 / total) + 1;
        return QIcon(QString(":/icons/transaction_%1").arg(part));
        }
    case Bitcredit_TransactionStatus::ImmatureDepositChange: {
        int total = wtx->status.depth + wtx->status.matures_in;
        int part = (wtx->status.depth * 4 / total) + 1;
        return QIcon(QString(":/icons/transaction_%1").arg(part));
        }
    case Bitcredit_TransactionStatus::MaturesWarning:
    case Bitcredit_TransactionStatus::NotAccepted:
        return QIcon(":/icons/transaction_0");
    }
    return QColor(0,0,0);
}

QString Bitcredit_TransactionTableModel::formatTooltip(const Bitcredit_TransactionRecord *rec) const
{
    QString tooltip = formatTxStatus(rec) + QString("\n") + formatTxType(rec);
    if(rec->type==Bitcredit_TransactionRecord::RecvFromOther || rec->type==Bitcredit_TransactionRecord::SendToOther ||
       rec->type==Bitcredit_TransactionRecord::SendToAddress || rec->type==Bitcredit_TransactionRecord::RecvWithAddress)
    {
        tooltip += QString(" ") + formatTxToAddress(rec, true);
    }
    return tooltip;
}

QVariant Bitcredit_TransactionTableModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();
    Bitcredit_TransactionRecord *rec = static_cast<Bitcredit_TransactionRecord*>(index.internalPointer());

    switch(role)
    {
    case Qt::DecorationRole:
        switch(index.column())
        {
        case Status:
            return txStatusDecoration(rec);
        case ToAddress:
            return txAddressDecoration(rec);
        }
        break;
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
        	if(isForDepositWallet) {
        		return formatTxDepositAmount(rec);
        	} else {
        		if(rec->type == Bitcredit_TransactionRecord::Deposit || rec->type == Bitcredit_TransactionRecord::DepositChange) {
					return formatTxAmountWithDeposit(rec);
        		} else {
					return formatTxAmount(rec);
        		}
        	}
        }
        break;
    case Qt::EditRole:
        // Edit role is used for sorting, so return the unformatted values
        switch(index.column())
        {
        case Status:
            return QString::fromStdString(rec->status.sortKey);
        case Date:
            return rec->time;
        case Type:
            return formatTxType(rec);
        case ToAddress:
            return formatTxToAddress(rec, true);
        case Amount:
        	if(isForDepositWallet) {
				return rec->credit;
        	} else {
				return rec->credit + rec->debit;
        	}
        }
        break;
    case Qt::ToolTipRole:
        return formatTooltip(rec);
    case Qt::TextAlignmentRole:
        return column_alignments[index.column()];
    case Qt::ForegroundRole:
        // Non-confirmed (but not immature) as transactions are grey
        if(!rec->status.countsForBalance && rec->status.status != Bitcredit_TransactionStatus::Immature)
        {
            return COLOR_UNCONFIRMED;
        }
        if(index.column() == Amount && (rec->credit+rec->debit) < 0)
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
        return QDateTime::fromTime_t(static_cast<uint>(rec->time));
    case LongDescriptionRole:
        return priv->describe(rec, walletModel->getOptionsModel()->getDisplayUnit());
    case AddressRole:
        return QString::fromStdString(rec->address);
    case LabelRole:
        return walletModel->getAddressTableModel()->labelForAddress(QString::fromStdString(rec->address));
    case AmountRole:
        return rec->credit + rec->debit;
    case TxIDRole:
        return rec->getTxID();
    case TxHashRole:
        return QString::fromStdString(rec->hash.ToString());
    case ConfirmedRole:
        return rec->status.countsForBalance;
    case FormattedAmountRole:
        return formatTxAmount(rec, false);
    case FormattedDepositAmountRole:
        return formatTxDepositAmount(rec);
    case StatusRole:
        return rec->status.status;
    }
    return QVariant();
}

QVariant Bitcredit_TransactionTableModel::headerData(int section, Qt::Orientation orientation, int role) const
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
            	if(isForDepositWallet) {
					return tr("Amount added as deposit.");
            	} else {
            		return tr("Amount removed from or added to balance.");
            	}
            }
        }
    }
    return QVariant();
}

QModelIndex Bitcredit_TransactionTableModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    Bitcredit_TransactionRecord *data = priv->index(row);
    if(data)
    {
        return createIndex(row, column, priv->index(row));
    }
    else
    {
        return QModelIndex();
    }
}

void Bitcredit_TransactionTableModel::updateDisplayUnit()
{
    // emit dataChanged to update Amount column with the current unit
    emit dataChanged(index(0, Amount), index(priv->size()-1, Amount));
}
