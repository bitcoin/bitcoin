#include "mintingtablemodel.h"
#include "transactiontablemodel.h"
#include "guiutil.h"
#include "kernelrecord.h"
#include "guiconstants.h"
#include "transactiondesc.h"
#include "walletmodel.h"
#include "optionsmodel.h"
#include "addresstablemodel.h"
#include "bitcoinunits.h"
#include "util.h"
#include "kernel.h"

#include "wallet.h"

#include <QLocale>
#include <QList>
#include <QColor>
#include <QTimer>
#include <QIcon>
#include <QDateTime>
#include <QtAlgorithms>


static int column_alignments[] = {
    Qt::AlignLeft|Qt::AlignVCenter,
    Qt::AlignRight|Qt::AlignVCenter,
    Qt::AlignRight|Qt::AlignVCenter,
    Qt::AlignRight|Qt::AlignVCenter,
    Qt::AlignRight|Qt::AlignVCenter
};

struct TxLessThan
{
    bool operator()(const KernelRecord &a, const KernelRecord &b) const
    {
        return a.hash < b.hash;
    }
    bool operator()(const KernelRecord &a, const uint256 &b) const
    {
        return a.hash < b;
    }
    bool operator()(const uint256 &a, const KernelRecord &b) const
    {
        return a < b.hash;
    }
};

class MintingTablePriv
{
public:
    MintingTablePriv(CWallet *wallet, MintingTableModel *parent):
        wallet(wallet),
        parent(parent)
    {
    }
    CWallet *wallet;
    MintingTableModel *parent;

    QList<KernelRecord> cachedWallet;

    void refreshWallet()
    {
#ifdef WALLET_UPDATE_DEBUG
        qDebug() << "refreshWallet";
#endif
        cachedWallet.clear();
        {
            LOCK(wallet->cs_wallet);
            for(std::map<uint256, CWalletTx>::iterator it = wallet->mapWallet.begin(); it != wallet->mapWallet.end(); ++it)
            {
                QList<KernelRecord> txList = KernelRecord::decomposeOutput(wallet, it->second);
                for(QList<KernelRecord>::iterator i = txList.begin(); i != txList.end(); ++i) {
                    if(!(*i).spent) {
                        cachedWallet.append(*i);
                    }
                }

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

        {
            LOCK(wallet->cs_wallet);
            for(int update_idx = updated_sorted.size()-1; update_idx >= 0; --update_idx)
            {
                const uint256 &hash = updated_sorted.at(update_idx);
                // Find transaction in wallet
                std::map<uint256, CWalletTx>::iterator mi = wallet->mapWallet.find(hash);
                bool inWallet = mi != wallet->mapWallet.end();
                // Find bounds of this transaction in model
                QList<KernelRecord>::iterator lower = qLowerBound(
                    cachedWallet.begin(), cachedWallet.end(), hash, TxLessThan());
                QList<KernelRecord>::iterator upper = qUpperBound(
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
                    QList<KernelRecord> toInsert =
                            KernelRecord::decomposeOutput(wallet, mi->second);
                    if(!toInsert.isEmpty()) /* only if something to insert */
                    {
                        parent->beginInsertRows(QModelIndex(), lowerIndex, lowerIndex+toInsert.size()-1);
                        int insert_idx = lowerIndex;
                        foreach(const KernelRecord &rec, toInsert)
                        {
                            if(!rec.spent) {
                                cachedWallet.insert(insert_idx, rec);
                                insert_idx += 1;
                            }
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

    KernelRecord *index(int idx)
    {
        if(idx >= 0 && idx < cachedWallet.size())
        {
            KernelRecord *rec = &cachedWallet[idx];
            // If a status update is needed (blocks came in since last check),
            //  update the status of this transaction from the wallet. Otherwise,
            // simply re-use the cached status.

// TODO
//            if(rec->statusUpdateNeeded())
//            {
//                {
//                    LOCK(wallet->cs_wallet);
//                    std::map<uint256, CWalletTx>::iterator mi = wallet->mapWallet.find(rec->hash);

//                    if(mi != wallet->mapWallet.end())
//                    {
//                        rec->updateStatus(mi->second);
//                    }
//                }
//            }
            return rec;
        }
        else
        {
            return 0;
        }
    }

    QString describe(KernelRecord *rec)
    {
        {
            LOCK(wallet->cs_wallet);
            std::map<uint256, CWalletTx>::iterator mi = wallet->mapWallet.find(rec->hash);
            if(mi != wallet->mapWallet.end())
            {
                return TransactionDesc::toHTML(wallet, mi->second);
            }
        }
        return QString("");
    }

};


MintingTableModel::MintingTableModel(CWallet *wallet, WalletModel *parent):
        QAbstractTableModel(parent),
        wallet(wallet),
        walletModel(parent),
        priv(new MintingTablePriv(wallet, this))
{
    columns << tr("Address") << tr("Age") << tr("Balance") << tr("CoinDay") << tr("MintProbability");
    priv->refreshWallet();

    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(update()));
    timer->start(MODEL_UPDATE_DELAY);
}

MintingTableModel::~MintingTableModel()
{
    delete priv;
}

void MintingTableModel::update()
{
    QList<uint256> updated;

    {
        TRY_LOCK(wallet->cs_wallet, lockWallet);
        if (lockWallet && !wallet->vWalletUpdated.empty())
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
    }
}

int MintingTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return priv->size();
}

int MintingTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return columns.length();
}

QVariant MintingTableModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();
    KernelRecord *rec = static_cast<KernelRecord*>(index.internalPointer());
    switch(role)
    {
      case Qt::DisplayRole:
        switch(index.column())
        {
        case Address:
            return formatTxAddress(rec, false);
        case Age:
            return formatTxAge(rec);
        case Balance:
            return formatTxBalance(rec);
        case CoinDay:
            return formatTxCoinDay(rec);
        }
        break;
      case Qt::TextAlignmentRole:
        return column_alignments[index.column()];
        break;

//    case Qt::DecorationRole:
//        switch(index.column())
//        {
//        case Status:
//            return txStatusDecoration(rec);
//        case ToAddress:
//            return txAddressDecoration(rec);
//        }
//        break;
//    case Qt::DisplayRole:
//        switch(index.column())
//        {
//        case Date:
//            return formatTxDate(rec);
//        case Type:
//            return formatTxType(rec);
//        case ToAddress:
//            return formatTxToAddress(rec, false);
//        case Amount:
//            return formatTxAmount(rec);
//        }
//        break;
//    case Qt::EditRole:
//        // Edit role is used for sorting, so return the unformatted values
//        switch(index.column())
//        {
//        case Status:
//            return QString::fromStdString(rec->status.sortKey);
//        case Date:
//            return rec->time;
//        case Type:
//            return formatTxType(rec);
//        case ToAddress:
//            return formatTxToAddress(rec, true);
//        case Amount:
//            return rec->credit + rec->debit;
//        }
//        break;
//    case Qt::ToolTipRole:
//        return formatTooltip(rec);

//    case Qt::ForegroundRole:
//        // Non-confirmed transactions are grey
//        if(!rec->status.confirmed)
//        {
//            return COLOR_UNCONFIRMED;
//        }
//        if(index.column() == Amount && (rec->credit+rec->debit) < 0)
//        {
//            return COLOR_NEGATIVE;
//        }
//        if(index.column() == ToAddress)
//        {
//            return addressColor(rec);
//        }
//        break;
//    case TypeRole:
//        return rec->type;
//    case DateRole:
//        return QDateTime::fromTime_t(static_cast<uint>(rec->time));
//    case LongDescriptionRole:
//        return priv->describe(rec);
//    case AddressRole:
//        return QString::fromStdString(rec->address);
//    case LabelRole:
//        return walletModel->getAddressTableModel()->labelForAddress(QString::fromStdString(rec->address));
//    case AmountRole:
//        return rec->credit + rec->debit;
//    case TxIDRole:
//        return QString::fromStdString(rec->getTxID());
//    case ConfirmedRole:
//        // Return True if transaction counts for balance
//        return rec->status.confirmed && !(rec->type == TransactionRecord::Generated &&
//                                          rec->status.maturity != TransactionStatus::Mature);
//    case FormattedAmountRole:
//        return formatTxAmount(rec, false);
    }
    return QVariant();
}


QString MintingTableModel::lookupAddress(const std::string &address, bool tooltip) const
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


QString MintingTableModel::formatTxAddress(const KernelRecord *wtx, bool tooltip) const
{
    return QString::fromStdString(wtx->address);
}

QString MintingTableModel::formatTxCoinDay(const KernelRecord *wtx) const
{
    CWalletTx walletTx;
    this->wallet->GetTransaction(wtx->hash, walletTx);
    CTxDB txdb("r");
    uint64 nCoinAge;
    walletTx.GetCoinAge(txdb, nCoinAge);
    return QString::number(nCoinAge);
}

QString MintingTableModel::formatTxAge(const KernelRecord *wtx) const
{
    return QString::number((time(NULL) - wtx->time) / 86400);
}

QString MintingTableModel::formatTxBalance(const KernelRecord *wtx) const
{
    return BitcoinUnits::format(walletModel->getOptionsModel()->getDisplayUnit(), wtx->nValue);
}

QVariant MintingTableModel::headerData(int section, Qt::Orientation orientation, int role) const
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
            case Address:
                return tr("TODO");
            case Age:
                return tr("TODO");
            case Balance:
                return tr("TODO");
            case CoinDay:
                return tr("TODO");
            case MintProbabilityPerDay:
                return tr("TODO");
            }
        }
    }
    return QVariant();
}

QModelIndex MintingTableModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    KernelRecord *data = priv->index(row);
    if(data)
    {
        return createIndex(row, column, priv->index(row));
    }
    else
    {
        return QModelIndex();
    }
}

