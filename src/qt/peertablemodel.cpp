// Copyright (c) 2011-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/peertablemodel.h>

#include <qt/guiconstants.h>
#include <qt/guiutil.h>
#include <qt/platformstyle.h>

#include <interfaces/node.h>

#include <utility>

#include <QBrush>
#include <QFont>
#include <QFontInfo>
#include <QImage>
#include <QPainter>
#include <QPixmap>
#include <QList>
#include <QTimer>

PeerTableModel::PeerTableModel(interfaces::Node& node, const PlatformStyle& platform_style, QObject* parent) :
    QAbstractTableModel(parent),
    m_node(node),
    m_platform_style(platform_style),
    timer(nullptr)
{
    // set up timer for auto refresh
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &PeerTableModel::refresh);
    timer->setInterval(MODEL_UPDATE_DELAY);

    DrawIcons();

    // load initial data
    refresh();
}

PeerTableModel::~PeerTableModel()
{
    // Intentionally left empty
}

void PeerTableModel::DrawIcons()
{
    static constexpr auto SIZE = 32;
    static constexpr auto ARROW_HEIGHT = SIZE * 2 / 3;
    QImage icon_in(SIZE, SIZE, QImage::Format_Alpha8);
    icon_in.fill(Qt::transparent);
    QImage icon_out(icon_in);
    QPainter icon_in_painter(&icon_in);
    QPainter icon_out_painter(&icon_out);

    // Arrow
    auto DrawArrow = [](const int x, QPainter& icon_painter) {
        icon_painter.setBrush(Qt::SolidPattern);
        QPoint shape[] = {
            {x, ARROW_HEIGHT / 2},
            {(SIZE-1) - x,  0},
            {(SIZE-1) - x, ARROW_HEIGHT-1},
        };
        icon_painter.drawConvexPolygon(shape, 3);
    };
    DrawArrow(0, icon_in_painter);
    DrawArrow(SIZE-1, icon_out_painter);

    {
        //: Label on inbound connection icon
        const QString label_in  = tr("IN");
        //: Label on outbound connection icon
        const QString label_out = tr("OUT");
        QImage scratch(SIZE, SIZE, QImage::Format_Alpha8);
        QPainter scratch_painter(&scratch);
        QFont font;  // NOTE: Application default font
        font.setBold(true);
        auto CheckSize = [&](const QImage& icon, const QString& text, const bool align_right) {
            // Make sure it's at least able to fit (width only)
            if (scratch_painter.boundingRect(0, 0, SIZE, SIZE, 0, text).width() > SIZE) {
                return false;
            }

            // Draw text on the scratch image
            // NOTE: QImage::fill doesn't like QPainter being active
            scratch_painter.setCompositionMode(QPainter::CompositionMode_Source);
            scratch_painter.fillRect(0, 0, SIZE, SIZE, Qt::transparent);
            scratch_painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
            scratch_painter.drawText(0, SIZE, text);

            int text_offset_x = 0;
            if (align_right) {
                // Figure out how far right we can shift it
                for (int col = SIZE-1; col >= 0; --col) {
                    bool any_pixels = false;
                    for (int row = SIZE-1; row >= 0; --row) {
                        int opacity = qAlpha(scratch.pixel(col, row));
                        if (opacity > 0) {
                            any_pixels = true;
                            break;
                        }
                    }
                    if (any_pixels) {
                        text_offset_x = (SIZE-1) - col;
                        break;
                    }
                }
            }

            // Check if there's any overlap
            for (int row = 0; row < SIZE; ++row) {
                for (int col = text_offset_x; col < SIZE; ++col) {
                    int opacity = qAlpha(icon.pixel(col, row));
                    if (col >= text_offset_x) {
                        opacity += qAlpha(scratch.pixel(col - text_offset_x, row));
                    }
                    if (opacity > 0xff) {
                        // Overlap found, we're done
                        return false;
                    }
                }
            }
            return true;
        };
        int font_size = SIZE;
        while (font_size > 1) {
            font.setPixelSize(--font_size);
            scratch_painter.setFont(font);
            if (CheckSize(icon_in , label_in , /* align_right= */ false) &&
                CheckSize(icon_out, label_out, /* align_right= */ true)) break;
        }
        icon_in_painter .drawText(0, 0, SIZE, SIZE, Qt::AlignLeft  | Qt::AlignBottom, label_in);
        icon_out_painter.drawText(0, 0, SIZE, SIZE, Qt::AlignRight | Qt::AlignBottom, label_out);
    }
    m_icon_conn_in  = m_platform_style.TextColorIcon(QIcon(QPixmap::fromImage(icon_in)));
    m_icon_conn_out = m_platform_style.TextColorIcon(QIcon(QPixmap::fromImage(icon_out)));
}

void PeerTableModel::startAutoRefresh()
{
    timer->start();
}

void PeerTableModel::stopAutoRefresh()
{
    timer->stop();
}

int PeerTableModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_peers_data.size();
}

int PeerTableModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return columns.length();
}

QVariant PeerTableModel::data(const QModelIndex& index, int role) const
{
    if(!index.isValid())
        return QVariant();

    CNodeCombinedStats *rec = static_cast<CNodeCombinedStats*>(index.internalPointer());

    const auto column = static_cast<ColumnIndex>(index.column());
    if (role == Qt::DisplayRole) {
        switch (column) {
        case NetNodeId:
            return (qint64)rec->nodeStats.nodeid;
        case Direction:
            return {};
        case Address:
            return QString::fromStdString(rec->nodeStats.addrName);
        case ConnectionType:
            return GUIUtil::ConnectionTypeToQString(rec->nodeStats.m_conn_type, /* prepend_direction */ false);
        case Network:
            return GUIUtil::NetworkToQString(rec->nodeStats.m_network);
        case Ping:
            return GUIUtil::formatPingTime(rec->nodeStats.m_min_ping_time);
        case Sent:
            return GUIUtil::formatBytes(rec->nodeStats.nSendBytes);
        case Received:
            return GUIUtil::formatBytes(rec->nodeStats.nRecvBytes);
        case Subversion:
            return QString::fromStdString(rec->nodeStats.cleanSubVer);
        } // no default case, so the compiler can warn about missing cases
        assert(false);
    } else if (role == Qt::TextAlignmentRole) {
        switch (column) {
        case NetNodeId:
            return QVariant(Qt::AlignRight | Qt::AlignVCenter);
        case Direction:
        case Address:
            return {};
        case ConnectionType:
        case Network:
            return QVariant(Qt::AlignCenter);
        case Ping:
        case Sent:
        case Received:
            return QVariant(Qt::AlignRight | Qt::AlignVCenter);
        case Subversion:
            return {};
        } // no default case, so the compiler can warn about missing cases
        assert(false);
    } else if (role == StatsRole) {
        return QVariant::fromValue(rec);
    } else if (index.column() == Direction && role == Qt::DecorationRole) {
        return rec->nodeStats.fInbound ? m_icon_conn_in : m_icon_conn_out;
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

QModelIndex PeerTableModel::index(int row, int column, const QModelIndex& parent) const
{
    Q_UNUSED(parent);

    if (0 <= row && row < rowCount() && 0 <= column && column < columnCount()) {
        return createIndex(row, column, const_cast<CNodeCombinedStats*>(&m_peers_data[row]));
    }

    return QModelIndex();
}

void PeerTableModel::refresh()
{
    interfaces::Node::NodesStats nodes_stats;
    m_node.getNodesStats(nodes_stats);
    decltype(m_peers_data) new_peers_data;
    new_peers_data.reserve(nodes_stats.size());
    for (const auto& node_stats : nodes_stats) {
        const CNodeCombinedStats stats{std::get<0>(node_stats), std::get<2>(node_stats), std::get<1>(node_stats)};
        new_peers_data.append(stats);
    }

    // Handle peer addition or removal as suggested in Qt Docs. See:
    // - https://doc.qt.io/qt-5/model-view-programming.html#inserting-and-removing-rows
    // - https://doc.qt.io/qt-5/model-view-programming.html#resizable-models
    // We take advantage of the fact that the std::vector returned
    // by interfaces::Node::getNodesStats is sorted by nodeid.
    for (int i = 0; i < m_peers_data.size();) {
        if (i < new_peers_data.size() && m_peers_data.at(i).nodeStats.nodeid == new_peers_data.at(i).nodeStats.nodeid) {
            ++i;
            continue;
        }
        // A peer has been removed from the table.
        beginRemoveRows(QModelIndex(), i, i);
        m_peers_data.erase(m_peers_data.begin() + i);
        endRemoveRows();
    }

    if (m_peers_data.size() < new_peers_data.size()) {
        // Some peers have been added to the end of the table.
        beginInsertRows(QModelIndex(), m_peers_data.size(), new_peers_data.size() - 1);
        m_peers_data.swap(new_peers_data);
        endInsertRows();
    } else {
        m_peers_data.swap(new_peers_data);
    }

    Q_EMIT changed();
}
