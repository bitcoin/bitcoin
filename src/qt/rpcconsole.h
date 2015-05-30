// Copyright (c) 2011-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_RPCCONSOLE_H
#define BITCOIN_QT_RPCCONSOLE_H

#include "guiutil.h"
#include "peertablemodel.h"

#include "net.h"

#include <QDialog>

class ClientModel;

namespace Ui {
    class RPCConsole;
}

QT_BEGIN_NAMESPACE
class QItemSelection;
QT_END_NAMESPACE

/** Local Bitcoin RPC console. */
class RPCConsole: public QDialog
{
    Q_OBJECT

public:
    explicit RPCConsole(QWidget *parent);
    ~RPCConsole();

    void setClientModel(ClientModel *model);

    enum MessageClass {
        MC_ERROR,
        MC_DEBUG,
        CMD_REQUEST,
        CMD_REPLY,
        CMD_ERROR
    };

protected:
    virtual bool eventFilter(QObject* obj, QEvent *event);

private slots:
    void on_lineEdit_returnPressed();
    void on_tabWidget_currentChanged(int index);
    /** open the debug.log from the current datadir */
    void on_openDebugLogfileButton_clicked();
    /** change the time range of the network traffic graph */
    void on_sldGraphRange_valueChanged(int value);
    /** update traffic statistics */
    void updateTrafficStats(quint64 totalBytesIn, quint64 totalBytesOut);
    void resizeEvent(QResizeEvent *event);
    void showEvent(QShowEvent *event);
    void hideEvent(QHideEvent *event);

public slots:
    void clear();
    
    /** Wallet repair options */
    void walletSalvage();
    void walletRescan();
    void walletZaptxes1();
    void walletZaptxes2();
    void walletUpgrade();
    void walletReindex();
    
    void reject();
    void message(int category, const QString &message, bool html = false);
    /** Set number of connections shown in the UI */
    void setNumConnections(int count);
    /** Set number of blocks shown in the UI */
    void setNumBlocks(int count);
    /** Set number of masternodes shown in the UI */
    void setMasternodeCount(const QString &strMasternodes);
    /** Go forward or back in history */
    void browseHistory(int offset);
    /** Scroll console view to end */
    void scrollToEnd();
    /** Switch to info tab and show */
    void showInfo();
    /** Switch to console tab and show */
    void showConsole();
    /** Switch to network tab and show */
    void showNetwork();
    /** Switch to peers tab and show */
    void showPeers();
    /** Switch to wallet-repair tab and show */
    void showRepair();
    /** Open external (default) editor with dash.conf */
    void showConfEditor();	
    /** Handle selection of peer in peers list */
    void peerSelected(const QItemSelection &selected, const QItemSelection &deselected);
    /** Handle updated peer information */
    void peerLayoutChanged();
    /** Show folder with wallet backups in default browser */
    void showBackups();

signals:
    // For RPC command executor
    void stopExecutor();
    void cmdRequest(const QString &command);
    /** Get restart command-line parameters and handle restart */
    void handleRestart(QStringList args);

private:
    static QString FormatBytes(quint64 bytes);
    void startExecutor();
    void setTrafficGraphRange(int mins);
    /** Build parameter list for restart */
    void buildParameterlist(QString arg);
    /** show detailed information on ui about selected node */
    void updateNodeDetail(const CNodeCombinedStats *stats);

    enum ColumnWidths
    {
        ADDRESS_COLUMN_WIDTH = 170,
        SUBVERSION_COLUMN_WIDTH = 140,
        PING_COLUMN_WIDTH = 80
    };

    Ui::RPCConsole *ui;
    ClientModel *clientModel;
    QStringList history;
    int historyPtr;
    NodeId cachedNodeid;
};

#endif // BITCOIN_QT_RPCCONSOLE_H
