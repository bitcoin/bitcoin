// Copyright (c) 2011-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_RPCCONSOLE_H
#define BITCOIN_QT_RPCCONSOLE_H

#include <qt/guiutil.h>
#include <qt/peertablemodel.h>

#include <net.h>

#include <QWidget>
#include <QCompleter>
#include <QThread>

class ClientModel;
class PlatformStyle;
class RPCTimerInterface;
class WalletModel;

namespace interfaces {
    class Node;
}

namespace Ui {
    class RPCConsole;
}

QT_BEGIN_NAMESPACE
class QMenu;
class QItemSelection;
QT_END_NAMESPACE

/** Local Bitcoin RPC console. */
class RPCConsole: public QWidget
{
    Q_OBJECT

public:
    explicit RPCConsole(interfaces::Node& node, const PlatformStyle *platformStyle, QWidget *parent);
    ~RPCConsole();

    static bool RPCParseCommandLine(interfaces::Node* node, std::string &strResult, const std::string &strCommand, bool fExecute, std::string * const pstrFilteredOut = nullptr, const WalletModel* wallet_model = nullptr);
    static bool RPCExecuteCommandLine(interfaces::Node& node, std::string &strResult, const std::string &strCommand, std::string * const pstrFilteredOut = nullptr, const WalletModel* wallet_model = nullptr) {
        return RPCParseCommandLine(&node, strResult, strCommand, true, pstrFilteredOut, wallet_model);
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

    enum class TabTypes {
        INFO,
        CONSOLE,
        GRAPH,
        PEERS
    };

    std::vector<TabTypes> tabs() const { return {TabTypes::INFO, TabTypes::CONSOLE, TabTypes::GRAPH, TabTypes::PEERS}; }

    QString tabTitle(TabTypes tab_type) const;
    QKeySequence tabShortcut(TabTypes tab_type) const;

protected:
    virtual bool eventFilter(QObject* obj, QEvent *event);
    void keyPressEvent(QKeyEvent *);

private Q_SLOTS:
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
    /** Append the message to the message widget */
    void message(int category, const QString &msg) { message(category, msg, false); }
    void message(int category, const QString &message, bool html);
    /** Set number of connections shown in the UI */
    void setNumConnections(int count);
    /** Set network state shown in the UI */
    void setNetworkActive(bool networkActive);
    /** Set number of blocks and last block date shown in the UI */
    void setNumBlocks(int count, const QDateTime& blockDate, double nVerificationProgress, bool headers);
    /** Set size (number of transactions and memory usage) of the mempool in the UI */
    void setMempoolSize(long numberOfTxs, size_t dynUsage);
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
    void cmdRequest(const QString &command, const WalletModel* wallet_model);

private:
    void startExecutor();
    void setTrafficGraphRange(int mins);
    /** show detailed information on ui about selected node */
    void updateNodeDetail(const CNodeCombinedStats *stats);

    enum ColumnWidths
    {
        ADDRESS_COLUMN_WIDTH = 200,
        SUBVERSION_COLUMN_WIDTH = 150,
        PING_COLUMN_WIDTH = 80,
        BANSUBNET_COLUMN_WIDTH = 200,
        BANTIME_COLUMN_WIDTH = 250

    };

    interfaces::Node& m_node;
    Ui::RPCConsole* const ui;
    ClientModel *clientModel = nullptr;
    QStringList history;
    int historyPtr = 0;
    QString cmdBeforeBrowsing;
    QList<NodeId> cachedNodeids;
    const PlatformStyle* const platformStyle;
    RPCTimerInterface *rpcTimerInterface = nullptr;
    QMenu *peersTableContextMenu = nullptr;
    QMenu *banTableContextMenu = nullptr;
    int consoleFontSize = 0;
    QCompleter *autoCompleter = nullptr;
    QThread thread;
    WalletModel* m_last_wallet_model{nullptr};

    /** Update UI with latest network info from model. */
    void updateNetworkState();

private Q_SLOTS:
    void updateAlerts(const QString& warnings);
};

#endif // BITCOIN_QT_RPCCONSOLE_H
