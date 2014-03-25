// Copyright (c) 2011-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_RPCCONSOLE_H
#define BITCOIN_RPCCONSOLE_H

#include "guiutil.h"
#include "peertablemodel.h"

#include "net.h"

#include <QDialog>

class ClientModel;
struct CNodeCombinedStats;

QT_BEGIN_NAMESPACE
class QItemSelection;
QT_END_NAMESPACE

namespace Ui {
    class Bitcoin_RPCConsole;
}

/** Local Bitcredit RPC console. */
class Bitcoin_RPCConsole: public QDialog
{
    Q_OBJECT

public:
    explicit Bitcoin_RPCConsole(QWidget *parent);
    ~Bitcoin_RPCConsole();

    void setClientModel(ClientModel *model);

    enum MessageClass {
        MC_ERROR,
        MC_DEBUG,
        CMD_REQUEST,
        CMD_REPLY,
        CMD_ERROR
    };

private:
    /** show detailed information on ui about selected node */
    void updateNodeDetail(const CNodeCombinedStats *combinedStats);

    enum ColumnWidths
    {
        ADDRESS_COLUMN_WIDTH = 250,
        MINIMUM_COLUMN_WIDTH = 120
    };

    /** track the node that we are currently viewing detail on in the peers tab */
    CNodeCombinedStats detailNodeStats;

private slots:
    /** change the time range of the network traffic graph */
    void on_sldGraphRange_valueChanged(int value);
    /** update traffic statistics */
    void updateTrafficStats(quint64 totalBytesIn, quint64 totalBytesOut);
    void resizeEvent(QResizeEvent *event);
    void showEvent(QShowEvent *event);
    void hideEvent(QHideEvent *event);

public slots:
    void reject();
    /** Set number of connections shown in the UI */
    void setNumConnections(int count);
    /** Set number of blocks shown in the UI */
    void setNumBlocks(int count);
    /** Handle selection of peer in peers list */
    void peerSelected(const QItemSelection &selected, const QItemSelection &deselected);
    /** Handle updated peer information */
    void peerLayoutChanged();

signals:
    // For RPC command executor
    void stopExecutor();
    void cmdRequest(const QString &command);

private:
    static QString FormatBytes(quint64 bytes);
    void setTrafficGraphRange(int mins);

    Ui::Bitcoin_RPCConsole *ui;
    ClientModel *clientModel;
    GUIUtil::TableViewLastColumnResizingFixer *columnResizingFixer;

    void startExecutor();
};

#endif // BITCOIN_RPCCONSOLE_H
