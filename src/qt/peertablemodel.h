// Copyright (c) 2011-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_PEERTABLEMODEL_H
#define BITCOIN_QT_PEERTABLEMODEL_H

#include <net_processing.h> // For CNodeStateStats
#include <net.h>

#include <QAbstractTableModel>
#include <QIcon>
#include <QList>
#include <QModelIndex>
#include <QStringList>
#include <QVariant>

class PeerTablePriv;
class PlatformStyle;

namespace interfaces {
class Node;
}

QT_BEGIN_NAMESPACE
class QTimer;
QT_END_NAMESPACE

struct CNodeCombinedStats {
    CNodeStats nodeStats;
    CNodeStateStats nodeStateStats;
    bool fNodeStateStatsAvailable;
};
Q_DECLARE_METATYPE(CNodeCombinedStats*)

/**
   Qt model providing information about connected peers, similar to the
   "getpeerinfo" RPC call. Used by the rpc console UI.
 */
class PeerTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit PeerTableModel(interfaces::Node& node, const PlatformStyle&, QObject* parent);
    ~PeerTableModel();
    void startAutoRefresh();
    void stopAutoRefresh();

    // See also RPCConsole::ColumnWidths in rpcconsole.h
    enum ColumnIndex {
        NetNodeId = 0,
        Age,
        Direction,
        Address,
        ConnectionType,
        Ping,
        Sent,
        Received,
        Subversion,
        Network,  // Not used, just kept at the end to avoid excessive code removal
    };

    enum {
        StatsRole = Qt::UserRole,
    };

    /** @name Methods overridden from QAbstractTableModel
        @{*/
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    /*@}*/

public Q_SLOTS:
    void refresh();
    void updatePalette();

private:
    //! Internal peer data structure.
    QList<CNodeCombinedStats> m_peers_data{};
    interfaces::Node& m_node;
    const PlatformStyle& m_platform_style;
    void DrawIcons();
    QIcon m_icon_conn_in, m_icon_conn_out;
    const QStringList columns{
        /*: Title of Peers Table column which contains a
            unique number used to identify a connection. */
        tr("id"),
        /*: Title of Peers Table column which indicates the duration (length of time)
            since the peer connection started. */
        tr("Age"),
        "",  // Direction column has no title
        /*: Title of Peers Table column which contains the
            IP/Onion/I2P address of the connected peer. */
        tr("Address"),
        /*: Title of Peers Table column which describes the type of
            peer connection. The "type" describes why the connection exists. */
        tr("Type"),
        /*: Title of Peers Table column which indicates the current latency
            of the connection with the peer. */
        tr("Ping"),
        /*: Title of Peers Table column which indicates the total amount of
            network information we have sent to the peer. */
        tr("Sent"),
        /*: Title of Peers Table column which indicates the total amount of
            network information we have received from the peer. */
        tr("Recv'd"),
        /*: Title of Peers Table column which contains the peer's
            User Agent string. */
        tr("User Agent")};
    QTimer* timer{nullptr};
};

#endif // BITCOIN_QT_PEERTABLEMODEL_H
