// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/bantablemodel.h>

#include <qt/clientmodel.h>
#include <qt/guiconstants.h>
#include <qt/guiutil.h>

#include <interfaces/node.h>
#include <sync.h>
#include <util/time.h>

#include <algorithm>

#include <QDebug>
#include <QList>

bool BannedNodeLessThan::operator()(const CCombinedBan& left, const CCombinedBan& right) const
{
    const CCombinedBan* pLeft = &left;
    const CCombinedBan* pRight = &right;

    if (order == Qt::DescendingOrder)
        std::swap(pLeft, pRight);

    switch(column)
    {
    case BanTableModel::Address:
        return pLeft->subnet.ToString().compare(pRight->subnet.ToString()) < 0;
    case BanTableModel::Bantime:
        return pLeft->banEntry.nBanUntil < pRight->banEntry.nBanUntil;
    }

    return false;
}

// private implementation
class BanTablePriv
{
public:
    /** Local cache of peer information */
    QList<CCombinedBan> cachedBanlist;
    /** Column to sort nodes by */
    int sortColumn;
    /** Order (ascending or descending) to sort nodes by */
    Qt::SortOrder sortOrder;

    /** Pull a full list of banned nodes from CNode into our cache */
    void refreshBanlist(interfaces::Node& node)
    {
        banmap_t banMap;
        node.getBanned(banMap);

        cachedBanlist.clear();
        cachedBanlist.reserve(banMap.size());
        for (const auto& entry : banMap)
        {
            CCombinedBan banEntry;
            banEntry.subnet = entry.first;
            banEntry.banEntry = entry.second;
            cachedBanlist.append(banEntry);
        }

        if (sortColumn >= 0)
            // sort cachedBanlist (use stable sort to prevent rows jumping around unnecessarily)
            std::stable_sort(cachedBanlist.begin(), cachedBanlist.end(), BannedNodeLessThan(sortColumn, sortOrder));
    }

    int size() const
    {
        return cachedBanlist.size();
    }

    CCombinedBan *index(int idx)
    {
        if (idx >= 0 && idx < cachedBanlist.size())
            return &cachedBanlist[idx];

        return 0;
    }
};

BanTableModel::BanTableModel(interfaces::Node& node, ClientModel *parent) :
    QAbstractTableModel(parent),
    m_node(node),
    clientModel(parent)
{
    columns << tr("IP/Netmask") << tr("Banned Until");
    priv.reset(new BanTablePriv());
    // default to unsorted
    priv->sortColumn = -1;

    // load initial data
    refresh();
}

BanTableModel::~BanTableModel()
{
    // Intentionally left empty
}

int BanTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return priv->size();
}

int BanTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return columns.length();
}

QVariant BanTableModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();

    CCombinedBan *rec = static_cast<CCombinedBan*>(index.internalPointer());

    if (role == Qt::DisplayRole) {
        switch(index.column())
        {
        case Address:
            return QString::fromStdString(rec->subnet.ToString());
        case Bantime:
            QDateTime date = QDateTime::fromMSecsSinceEpoch(0);
            date = date.addSecs(rec->banEntry.nBanUntil);
            return date.toString(Qt::SystemLocaleLongDate);
        }
    }

    return QVariant();
}

QVariant BanTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation == Qt::Horizontal)
    {
        if(role == Qt::DisplayRole && section < columns.size())
        {
            return columns[section];
        }
    }
    return QVariant();
}

Qt::ItemFlags BanTableModel::flags(const QModelIndex &index) const
{
    if(!index.isValid())
        return 0;

    Qt::ItemFlags retval = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    return retval;
}

QModelIndex BanTableModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    CCombinedBan *data = priv->index(row);

    if (data)
        return createIndex(row, column, data);
    return QModelIndex();
}

void BanTableModel::refresh()
{
    Q_EMIT layoutAboutToBeChanged();
    priv->refreshBanlist(m_node);
    Q_EMIT layoutChanged();
}

void BanTableModel::sort(int column, Qt::SortOrder order)
{
    priv->sortColumn = column;
    priv->sortOrder = order;
    refresh();
}

bool BanTableModel::shouldShow()
{
    return priv->size() > 0;
}
