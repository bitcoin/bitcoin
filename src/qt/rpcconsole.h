// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_RPCCONSOLE_H
#define BITCOIN_QT_RPCCONSOLE_H

#include <qt/guiutil.h>
#include <qt/peertablemodel.h>
#include <qt/trafficgraphdata.h>

#include <net.h>

#include <QWidget>
#include <QCompleter>
#include <QThread>

class ClientModel;
class RPCTimerInterface;
class WalletModel;

namespace interfaces {
    class Node;
}

namespace Ui {
    class RPCConsole;
}

QT_BEGIN_NAMESPACE
class QButtonGroup;
class QMenu;
class QItemSelection;
QT_END_NAMESPACE

/** Local Bitcoin RPC console. */
class RPCConsole: public QWidget
{
    Q_OBJECT

public:
    explicit RPCConsole(interfaces::Node& node, QWidget* parent, Qt::WindowFlags flags);
    ~RPCConsole();

    static bool RPCParseCommandLine(interfaces::Node* node, std::string &strResult, const std::string &strCommand, bool fExecute, std::string * const pstrFilteredOut = nullptr, const std::string *walletID = nullptr);
    static bool RPCExecuteCommandLine(interfaces::Node& node, std::string &strResult, const std::string &strCommand, std::string * const pstrFilteredOut = nullptr, const std::string *walletID = nullptr) {
        return RPCParseCommandLine(&node, strResult, strCommand, true, pstrFilteredOut, walletID);
    }

    void setClientModel(ClientModel *model);
    void addWallet(WalletModel * const walletModel);
    void removeWallet(WalletModel* const walletModel);

    enum MessageClass {
        MC_ERROR,
        MC_DEBUG,
        CMD_REQUEST,
        CMD_REPLY,
        CMD_ERROR
    };

    enum TabTypes {
        TAB_INFO = 0,
        TAB_CONSOLE = 1,
        TAB_GRAPH = 2,
        TAB_PEERS = 3,
        TAB_REPAIR = 4
    };

protected:
    virtual bool eventFilter(QObject* obj, QEvent *event) override;
    void keyPressEvent(QKeyEvent *) override;

private Q_SLOTS:
    /** custom tab buttons clicked */
    void showPage(int index);
    void on_lineEdit_returnPressed();
    void on_stackedWidgetRPC_currentChanged(int index);
    /** open the debug.log from the current datadir */
    void on_openDebugLogfileButton_clicked();
    /** change the time range of the network traffic graph */
    void on_sldGraphRange_valueChanged(int value);
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void changeEvent(QEvent* e) override;
    /** Show custom context menu on Peers tab */
    void showPeersTableContextMenu(const QPoint& point);
    /** Show custom context menu on Bans tab */
    void showBanTableContextMenu(const QPoint& point);
    /** Hides ban table if no bans are present */
    void showOrHideBanTableIfRequired();
    /** clear the selected node */
    void clearSelectedNode();

public Q_SLOTS:
    void clear(bool clearHistory = true);
    void fontBigger();
    void fontSmaller();
    void setFontSize(int newSize);

    /** Wallet repair options */
    void walletSalvage();
    void walletRescan1();
    void walletRescan2();
    void walletZaptxes1();
    void walletZaptxes2();
    void walletUpgrade();
    void walletReindex();

    /** Append the message to the message widget */
    void message(int category, const QString &message, bool html = false);
    /** Set number of connections shown in the UI */
    void setNumConnections(int count);
    /** Set network state shown in the UI */
    void setNetworkActive(bool networkActive);
    /** Update number of masternodes shown in the UI */
    void updateMasternodeCount();
    /** Set number of blocks, last block date and last block hash shown in the UI */
    void setNumBlocks(int count, const QDateTime& blockDate, const QString& blockHash, double nVerificationProgress, bool headers);
    /** Set size (number of transactions and memory usage) of the mempool in the UI */
    void setMempoolSize(long numberOfTxs, size_t dynUsage);
    /** Set number of InstantSend locks */
    void setInstantSendLockCount(size_t count);
    /** Go forward or back in history */
    void browseHistory(int offset);
    /** Scroll console view to end */
    void scrollToEnd();
    /** Handle selection of peer in peers list */
    void peerSelected(const QItemSelection &selected, const QItemSelection &deselected);
    /** Handle selection caching before update */
    void peerLayoutAboutToChange();
    /** Handle updated peer information */
    void peerLayoutChanged();
    /** Disconnect a selected node on the Peers tab */
    void disconnectSelectedNode();
    /** Ban a selected node on the Peers tab */
    void banSelectedNode(int bantime);
    /** Unban a selected node on the Bans tab */
    void unbanSelectedNode();
    /** set which tab has the focus (is visible) */
    void setTabFocus(enum TabTypes tabType);

Q_SIGNALS:
    // For RPC command executor
    void stopExecutor();
    void cmdRequest(const QString &command, const QString &walletID);
    /** Get restart command-line parameters and handle restart */
    void handleRestart(QStringList args);

private:
    void startExecutor();
    void setTrafficGraphRange(TrafficGraphData::GraphRange range);
    /** Build parameter list for restart */
    void buildParameterlist(QString arg);
    /** show detailed information on ui about selected node */
    void updateNodeDetail(const CNodeCombinedStats *stats);
    /** Set required icons for buttons inside the dialog */
    void setButtonIcons();
    /** Reload some themes related widgets */
    void reloadThemedWidgets();

    enum ColumnWidths
    {
        ADDRESS_COLUMN_WIDTH = 170,
        SUBVERSION_COLUMN_WIDTH = 150,
        PING_COLUMN_WIDTH = 80,
        BANSUBNET_COLUMN_WIDTH = 200,
        BANTIME_COLUMN_WIDTH = 250

    };

    interfaces::Node& m_node;
    Ui::RPCConsole* const ui;
    ClientModel *clientModel = nullptr;
    QButtonGroup* pageButtons = nullptr;
    QStringList history;
    int historyPtr = 0;
    QString cmdBeforeBrowsing;
    QList<NodeId> cachedNodeids;
    RPCTimerInterface *rpcTimerInterface = nullptr;
    QMenu *peersTableContextMenu = nullptr;
    QMenu *banTableContextMenu = nullptr;
    int consoleFontSize = 0;
    QCompleter *autoCompleter = nullptr;
    QThread thread;
    QString m_last_wallet_id;

    /** Update UI with latest network info from model. */
    void updateNetworkState();
};

#endif // BITCOIN_QT_RPCCONSOLE_H
