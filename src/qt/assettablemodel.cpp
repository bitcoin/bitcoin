// Copyright (c) 2011-2016 The Bitcoin Core developers
// Copyright (c) 2017 The Raven Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "assettablemodel.h"
#include "assetrecord.h"

#include "guiconstants.h"
#include "guiutil.h"
#include "walletmodel.h"

#include "core_io.h"

#include "amount.h"
#include "assets/assets.h"
#include "validation.h"

#include <QDebug>
#include <QStringList>


class AssetTablePriv {
public:
    AssetTablePriv(AssetTableModel *_parent) :
            parent(_parent)
    {
    }

    AssetTableModel *parent;

    QList<AssetRecord> cachedBalances;

    // loads all current balances into cache
    void refreshWallet() {
        qDebug() << "AssetTablePriv::refreshWallet";
        cachedBalances.clear();
        if (passets) {
            {
                LOCK(cs_main);
                std::map<std::string, CAmount> balances;
                if (!GetMyAssetBalances(*passets, balances)) {
                    qWarning("AssetTablePriv::refreshWallet: Error retrieving asset balances");
                    return;
                }

                auto bal = balances.begin();
                for (; bal != balances.end(); bal++) {
                    // retrieve units for asset
                    uint8_t units = OWNER_UNITS;
                    if (!IsAssetNameAnOwner(bal->first)) {
                        CNewAsset assetData;
                        if (!passets->GetAssetIfExists(bal->first, assetData)) {
                            qWarning("AssetTablePriv::refreshWallet: Error retrieving asset data");
                            return;
                        }
                        units = assetData.units;
                    }
                    cachedBalances.append(AssetRecord(bal->first, bal->second, units));
                }
            }
        }
    }


    int size() {
        qDebug() << "AssetTablePriv::size";
        return cachedBalances.size();
    }

    AssetRecord *index(int idx) {
        qDebug() << "AssetTablePriv::index(" << idx << ")";
        if (idx >= 0 && idx < cachedBalances.size()) {
            return &cachedBalances[idx];
        }
        qDebug() << "AssetTablePriv::index --> 0";
        return 0;
    }

};

AssetTableModel::AssetTableModel(WalletModel *parent) :
        QAbstractTableModel(parent),
        walletModel(parent),
        priv(new AssetTablePriv(this))
{
    qDebug() << "AssetTableModel::AssetTableModel";
    columns << tr("Name") << tr("Quantity");

    priv->refreshWallet();
};

AssetTableModel::~AssetTableModel()
{
    qDebug() << "AssetTableModel::~AssetTableModel";
    delete priv;
};

void AssetTableModel::checkBalanceChanged() {
    qDebug() << "AssetTableModel::CheckBalanceChanged";
    // TODO: optimize by 1) updating cache incrementally; and 2) emitting more specific dataChanged signals
    Q_EMIT layoutAboutToBeChanged();
    priv->refreshWallet();
    Q_EMIT dataChanged(index(0, 0, QModelIndex()), index(priv->size(), columns.length()-1, QModelIndex()));
    Q_EMIT layoutChanged();
}

int AssetTableModel::rowCount(const QModelIndex &parent) const
{
    qDebug() << "AssetTableModel::rowCount";
    Q_UNUSED(parent);
    return priv->size();
}

int AssetTableModel::columnCount(const QModelIndex &parent) const
{
    qDebug() << "AssetTableModel::columnCount";
    Q_UNUSED(parent);
    return columns.length();
}

QVariant AssetTableModel::data(const QModelIndex &index, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    qDebug() << "AssetTableModel::data(" << index << ", " << role << ")";
    Q_UNUSED(role);
    if(!index.isValid())
        return QVariant();
    AssetRecord *rec = static_cast<AssetRecord*>(index.internalPointer());

    switch (index.column())
    {
        case Name:
            return QString::fromStdString(rec->name);
        case Quantity:
            return QString::fromStdString(rec->formattedQuantity());
        default:
            return QString();
    }
}

QVariant AssetTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    qDebug() << "AssetTableModel::headerData";
    if(role == Qt::DisplayRole)
        {
            if (section < columns.size())
                return columns.at(section);
        }

    return QVariant();
}

QModelIndex AssetTableModel::index(int row, int column, const QModelIndex &parent) const
{
    qDebug() << "AssetTableModel::index(" << row << ", " << column << ", " << parent << ")";
    Q_UNUSED(parent);
    AssetRecord *data = priv->index(row);
    if(data)
    {
        QModelIndex idx = createIndex(row, column, priv->index(row));
        qDebug() << "AssetTableModel::index --> " << idx;
        return idx;
    }
    qDebug() << "AssetTableModel::index --> " << QModelIndex();
    return QModelIndex();
}
