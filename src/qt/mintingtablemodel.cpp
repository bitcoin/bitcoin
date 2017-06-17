#include "mintingtablemodel.h"
#include "mintingfilterproxy.h"

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
#include "ui_interface.h"

#include <QList>
#include <QColor>
#include <QTimer>
#include <QIcon>
#include <QDateTime>

// Amount column is right-aligned it contains numbers
static int column_alignments[] = {
        Qt::AlignLeft|Qt::AlignVCenter,
        Qt::AlignLeft|Qt::AlignVCenter,
        Qt::AlignRight|Qt::AlignVCenter,
        Qt::AlignRight|Qt::AlignVCenter,
        Qt::AlignRight|Qt::AlignVCenter,
        Qt::AlignRight|Qt::AlignVCenter
    };

// Comparison operator for sort/binary search of model tx list
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

// Private implementation
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

    /* Local cache of wallet.
     * As it is in the same order as the CWallet, by definition
     * this is sorted by sha256.
     */
    QList<KernelRecord> cachedWallet;

    /* Query entire wallet anew from core.
     */
    void refreshWallet()
    {
        OutputDebugStringF("refreshWallet\n");
        cachedWallet.clear();
        {
            LOCK(wallet->cs_wallet);
            for(std::map<uint256, CWalletTx>::iterator it = wallet->mapWallet.begin(); it != wallet->mapWallet.end(); ++it)
            {
                std::vector<KernelRecord> txList = KernelRecord::decomposeOutput(wallet, it->second);
                if(KernelRecord::showTransaction(it->second))
                    foreach(const KernelRecord& kr, txList) {
                        if(!kr.spent) {
                            cachedWallet.append(kr);
                        }
                    }
            }
        }
    }

    /* Update our model of the wallet incrementally, to synchronize our model of the wallet
       with that of the core.

       Call with transaction that was added, removed or changed.
     */
    void updateWallet(const uint256 &hash, int status)
    {
        OutputDebugStringF("minting updateWallet %s %i\n", hash.ToString().c_str(), status);
        {
            LOCK(wallet->cs_wallet);

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
            bool inModel = (lower != upper);

            // Determine whether to show transaction or not
            bool showTransaction = (inWallet && KernelRecord::showTransaction(mi->second));

            if(status == CT_UPDATED)
            {
                if(showTransaction && !inModel)
                    status = CT_NEW; /* Not in model, but want to show, treat as new */
                if(!showTransaction && inModel)
                    status = CT_DELETED; /* In model, but want to hide, treat as deleted */
            }

            OutputDebugStringF("   inWallet=%i inModel=%i Index=%i-%i showTransaction=%i derivedStatus=%i\n",
                     inWallet, inModel, lowerIndex, upperIndex, showTransaction, status);

            switch(status)
            {
            case CT_NEW:
                if(inModel)
                {
                    OutputDebugStringF("Warning: updateWallet: Got CT_NEW, but transaction is already in model\n");
                    break;
                }
                if(!inWallet)
                {
                    OutputDebugStringF("Warning: updateWallet: Got CT_NEW, but transaction is not in wallet\n");
                    break;
                }
                if(showTransaction)
                {
                    // Added -- insert at the right position
                    std::vector<KernelRecord> toInsert =
                            KernelRecord::decomposeOutput(wallet, mi->second);
                    if(toInsert.size() != 0) /* only if something to insert */
                    {
                        parent->beginInsertRows(QModelIndex(), lowerIndex, lowerIndex+toInsert.size()-1);
                        int insert_idx = lowerIndex;
                        foreach(const KernelRecord &rec, toInsert)
                        {
                            if(!rec.spent)
                            {
                                cachedWallet.insert(insert_idx, rec);
                                insert_idx += 1;
                            }
                        }
                        parent->endInsertRows();
                    }
                }
                break;
            case CT_DELETED:
                if(!inModel)
                {
                    OutputDebugStringF("Warning: updateWallet: Got CT_DELETED, but transaction is not in model\n");
                    break;
                }
                // Removed -- remove entire transaction from table
                parent->beginRemoveRows(QModelIndex(), lowerIndex, upperIndex-1);
                cachedWallet.erase(lower, upper);
                parent->endRemoveRows();
                break;
            case CT_UPDATED:
                // Updated -- remove spent coins from table
                std::vector<KernelRecord> toCheck = KernelRecord::decomposeOutput(wallet, mi->second);
                if(!toCheck.empty())
                {
                    foreach(const KernelRecord &rec, toCheck)
                    {
                        if(rec.spent)
                        {
                            for(int i = lowerIndex; i < upperIndex; i++)
                            {
                                KernelRecord cachedRec = cachedWallet.at(i);
                                if((rec.address == cachedRec.address)
                                   && (rec.nValue == cachedRec.nValue)
                                   && (rec.idx == cachedRec.idx))
                                {
                                    parent->beginRemoveRows(QModelIndex(), i, i);
                                    cachedWallet.removeAt(i);
                                    parent->endRemoveRows();
                                    break;
                                }
                            }
                        }
                    }
                }
                break;
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

MintingTableModel::MintingTableModel(CWallet* wallet, WalletModel *parent):
        QAbstractTableModel(parent),
        wallet(wallet),
        walletModel(parent),
        mintingInterval(10),
        priv(new MintingTablePriv(wallet, this)),
        cachedNumBlocks(0)
{
    columns << tr("Transaction") <<  tr("Address") << tr("Age") << tr("Balance") << tr("CoinDay") << tr("MintProbability");

    priv->refreshWallet();

    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(updateAge()));
    timer->start(MODEL_UPDATE_DELAY);

    connect(walletModel->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));
}

MintingTableModel::~MintingTableModel()
{
    delete priv;
}

void MintingTableModel::updateTransaction(const QString &hash, int status)
{
    uint256 updated;
    updated.SetHex(hash.toStdString());

    priv->updateWallet(updated, status);
    mintingProxyModel->invalidate(); // Force deletion of empty rows
}

void MintingTableModel::updateAge()
{
    emit dataChanged(index(0, Age), index(priv->size()-1, Age));
    emit dataChanged(index(0, CoinDay), index(priv->size()-1, CoinDay));
    emit dataChanged(index(0, MintProbability), index(priv->size()-1, MintProbability));
}

void MintingTableModel::setMintingProxyModel(MintingFilterProxy *mintingProxy)
{
    mintingProxyModel = mintingProxy;
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
        case TxHash:
            return formatTxHash(rec);
        case Age:
            return formatTxAge(rec);
        case Balance:
            return formatTxBalance(rec);
        case CoinDay:
            return formatTxCoinDay(rec);
        case MintProbability:
            return formatDayToMint(rec);
        }
        break;
      case Qt::TextAlignmentRole:
        return column_alignments[index.column()];
        break;
      case Qt::ToolTipRole:
        switch(index.column())
        {
        case MintProbability:
            int interval = this->mintingInterval;
            QString unit = tr("minutes");

            int hours = interval / 60;
            int days = hours  / 24;

            if(hours > 1) {
                interval = hours;
                unit = tr("hours");
            }
            if(days > 1) {
                interval = days;
                unit = tr("days");
            }

            QString str = QString(tr("You have %1 chance to find a POS block if you mint %2 %3 at current difficulty."));
            return str.arg(index.data().toString().toUtf8().constData()).arg(interval).arg(unit);
        }
        break;
      case Qt::EditRole:
        switch(index.column())
        {
        case Address:
            return formatTxAddress(rec, false);
        case TxHash:
            return formatTxHash(rec);
        case Age:
            return rec->getAge();
        case CoinDay:
            return rec->coinAge;
        case Balance:
            return rec->nValue;
        case MintProbability:
            return getDayToMint(rec);
        }
        break;
      case Qt::BackgroundColorRole:
        int minAge = nStakeMinAge / 60 / 60 / 24;
        int maxAge = STAKE_MAX_AGE / 60 / 60 / 24;
        if(rec->getAge() < minAge)
        {
            return COLOR_MINT_YOUNG;
        }
        else if (rec->getAge() >= minAge && rec->getAge() < maxAge)
        {
            return COLOR_MINT_MATURE;
        }
        else
        {
            return COLOR_MINT_OLD;
        }
        break;

    }
    return QVariant();
}

void MintingTableModel::setMintingInterval(int interval)
{
    mintingInterval = interval;
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

double MintingTableModel::getDayToMint(KernelRecord *wtx) const
{
    const CBlockIndex *p = GetLastBlockIndex(pindexBest, true);
    double difficulty = p->GetBlockDifficulty();

    double prob = wtx->getProbToMintWithinNMinutes(difficulty, mintingInterval);
    prob = prob * 100;
    return prob;
}

QString MintingTableModel::formatDayToMint(KernelRecord *wtx) const
{
    double prob = getDayToMint(wtx);
    return QString::number(prob, 'f', 6) + "%";
}

QString MintingTableModel::formatTxAddress(const KernelRecord *wtx, bool tooltip) const
{
    return QString::fromStdString(wtx->address);
}

QString MintingTableModel::formatTxHash(const KernelRecord *wtx) const
{
    return QString::fromStdString(wtx->hash.ToString());
}

QString MintingTableModel::formatTxCoinDay(const KernelRecord *wtx) const
{
    return QString::number(wtx->coinAge);
}

QString MintingTableModel::formatTxAge(const KernelRecord *wtx) const
{
    int64 nAge = wtx->getAge();
    return QString::number(nAge);
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
                return tr("Destination address of the output.");
            case TxHash:
                return tr("Original transaction id.");
            case Age:
                return tr("Age of the transaction in days.");
            case Balance:
                return tr("Balance of the output.");
            case CoinDay:
                return tr("Coin age in the output.");
            case MintProbability:
                return tr("Chance to mint a block within given time interval.");
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

void MintingTableModel::updateDisplayUnit()
{
    // emit dataChanged to update Balance column with the current unit
    emit dataChanged(index(0, Balance), index(priv->size()-1, Balance));
}
