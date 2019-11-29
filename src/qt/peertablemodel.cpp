// Copyright (c) 2011-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/peertablemodel.h>

#include <qt/clientmodel.h>
#include <qt/guiconstants.h>
#include <qt/guiutil.h>

#include <interfaces/node.h>
#include <sync.h>

#include <utility>

#include <QDebug>
#include <QList>
#include <QTimer>

bool NodeLessThan::operator()(const CNodeCombinedStats &left, const CNodeCombinedStats &right) const
{
    const CNodeStats *pLeft = &(left.nodeStats);
    const CNodeStats *pRight = &(right.nodeStats);

    if (order == Qt::DescendingOrder)
        std::swap(pLeft, pRight);

    switch(column)
    {
    case PeerTableModel::NetNodeId:
        return pLeft->nodeid < pRight->nodeid;
    case PeerTableModel::Address:
        return pLeft->addrName.compare(pRight->addrName) < 0;
    case PeerTableModel::Subversion:
        return pLeft->cleanSubVer.compare(pRight->cleanSubVer) < 0;
    case PeerTableModel::Ping:
        return pLeft->dMinPing < pRight->dMinPing;
    case PeerTableModel::Sent:
        return pLeft->nSendBytes < pRight->nSendBytes;
    case PeerTableModel::Received:
        return pLeft->nRecvBytes < pRight->nRecvBytes;
    }

    return false;
}

// private implementation
class PeerTablePriv
{
public:
    /** Local cache of peer information */
    QList<CNodeCombinedStats> cachedNodeStats;
    /** Column to sort nodes by (default to unsorted) */
    int sortColumn{-1};
    /** Order (ascending or descending) to sort nodes by */
    Qt::SortOrder sortOrder;
    /** Index of rows by node ID */
    std::map<NodeId, int> mapNodeRows;

    /** Pull a full list of peers from vNodes into our cache */
    void refreshPeers(interfaces::Node& node)
    {
        {
            cachedNodeStats.clear();

            interfaces::Node::NodesStats nodes_stats;
            node.getNodesStats(nodes_stats);
            cachedNodeStats.reserve(nodes_stats.size());
            for (const auto& node_stats : nodes_stats)
            {
                CNodeCombinedStats stats;
                stats.nodeStats = std::get<0>(node_stats);
                stats.fNodeStateStatsAvailable = std::get<1>(node_stats);
                stats.nodeStateStats = std::get<2>(node_stats);
                cachedNodeStats.append(stats);
            }
        }

        if (sortColumn >= 0)
            // sort cacheNodeStats (use stable sort to prevent rows jumping around unnecessarily)
            std::stable_sort(cachedNodeStats.begin(), cachedNodeStats.end(), NodeLessThan(sortColumn, sortOrder));

        // build index map
        mapNodeRows.clear();
        int row = 0;
        for (const CNodeCombinedStats& stats : cachedNodeStats)
            mapNodeRows.insert(std::pair<NodeId, int>(stats.nodeStats.nodeid, row++));
    }

    int size() const
    {
        return cachedNodeStats.size();
    }

    CNodeCombinedStats *index(int idx)
    {
        if (idx >= 0 && idx < cachedNodeStats.size())
            return &cachedNodeStats[idx];

        return nullptr;
    }
};

PeerTableModel::PeerTableModel(interfaces::Node& node, ClientModel *parent) :
    QAbstractTableModel(parent),
    m_node(node),
    clientModel(parent),
    timer(nullptr)
{
    columns << tr("NodeId") << tr("Node/Service") << tr("Ping") << tr("Sent") << tr("Received") << tr("User Agent");
    priv.reset(new PeerTablePriv());

    // set up timer for auto refresh
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &PeerTableModel::refresh);
    timer->setInterval(MODEL_UPDATE_DELAY);

    // load initial data
    refresh();
}

PeerTableModel::~PeerTableModel()
{
    // Intentionally left empty
}

void PeerTableModel::startAutoRefresh()
{
    timer->start();
}

void PeerTableModel::stopAutoRefresh()
{
    timer->stop();
}

int PeerTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return priv->size();
}

int PeerTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return columns.length();
}

QVariant PeerTableModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();

    CNodeCombinedStats *rec = static_cast<CNodeCombinedStats*>(index.internalPointer());

    if (role == Qt::DisplayRole) {
        switch(index.column())
        {
        case NetNodeId:
            return (qint64)rec->nodeStats.nodeid;
        case Address:
            // prepend to peer address down-arrow symbol for inbound connection and up-arrow for outbound connection
            return QString(rec->nodeStats.fInbound ? "↓ " : "↑ ") + QString::fromStdString(rec->nodeStats.addrName);
        case Subversion:
            return QString::fromStdString(rec->nodeStats.cleanSubVer);
        case Ping:
            return GUIUtil::formatPingTime(rec->nodeStats.dMinPing);
        case Sent:
            return GUIUtil::formatBytes(rec->nodeStats.nSendBytes);
        case Received:
            return GUIUtil::formatBytes(rec->nodeStats.nRecvBytes);
        }
    } else if (role == Qt::TextAlignmentRole) {
        switch (index.column()) {
            case Ping:
            case Sent:
            case Received:
                return QVariant(Qt::AlignRight | Qt::AlignVCenter);
            default:
                return QVariant();
        }
    }

    return QVariant();
}

QVariant PeerTableModel::headerData(int section, Qt::Orientation orientation, int role) const
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

Qt::ItemFlags PeerTableModel::flags(const QModelIndex &index) const
{
    if (!index.isValid()) return Qt::NoItemFlags;

    Qt::ItemFlags retval = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    return retval;
}

QModelIndex PeerTableModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    CNodeCombinedStats *data = priv->index(row);

    if (data)
        return createIndex(row, column, data);
    return QModelIndex();
}

const CNodeCombinedStats *PeerTableModel::getNodeStats(int idx)
{
    return priv->index(idx);
}

void PeerTableModel::refresh()
{
    Q_EMIT layoutAboutToBeChanged();
    priv->refreshPeers(m_node);
    Q_EMIT layoutChanged();
}

int PeerTableModel::getRowByNodeId(NodeId nodeid)
{
    std::map<NodeId, int>::iterator it = priv->mapNodeRows.find(nodeid);
    if (it == priv->mapNodeRows.end())
        return -1;

    return it->second;
}

void PeerTableModel::sort(int column, Qt::SortOrder order)
{
    priv->sortColumn = column;
    priv->sortOrder = order;
    refresh();
}
