// Copyright (c) 2011-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_RPCCONSOLE_H
#define BITCOIN_QT_RPCCONSOLE_H

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <qt/guiutil.h>
#include <qt/peertablemodel.h>
#include <qt/trafficgraphdata.h>

#include <net.h>
#include <uint256.h>

#include <QByteArray>
#include <QCompleter>
#include <QThread>
#include <QWidget>

class ClientModel;
class MasternodeFeed;
class RPCExecutor;
class RPCTimerInterface;
class WalletController;
class WalletModel;

namespace interfaces {
    class Node;
}

namespace Ui {
    class RPCConsole;
}

QT_BEGIN_NAMESPACE
class QButtonGroup;
class QDateTime;
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

    static bool RPCParseCommandLine(interfaces::Node* node, std::string &strResult, const std::string &strCommand, bool fExecute, std::string * const pstrFilteredOut = nullptr, const WalletModel* wallet_model = nullptr);
    static bool RPCExecuteCommandLine(interfaces::Node& node, std::string &strResult, const std::string &strCommand, std::string * const pstrFilteredOut = nullptr, const WalletModel* wallet_model = nullptr) {
        return RPCParseCommandLine(&node, strResult, strCommand, true, pstrFilteredOut, wallet_model);
    }

    void setClientModel(ClientModel *model = nullptr, int bestblock_height = 0, int64_t bestblock_date = 0, uint256 bestblock_hash = uint256(), double verification_progress = 0.0);

#ifdef ENABLE_WALLET
    void setWalletController(WalletController* wallet_controller);
    void addWallet(WalletModel* const walletModel);
    void removeWallet(WalletModel* const walletModel);
#endif // ENABLE_WALLET

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
        PEERS,
        REPAIR
    };

    enum class InfoView : uint8_t {
        General,
        Network,
        Governance,
    };

    std::vector<TabTypes> tabs() const { return {TabTypes::INFO, TabTypes::CONSOLE, TabTypes::GRAPH, TabTypes::PEERS, TabTypes::REPAIR}; }

    QString tabTitle(TabTypes tab_type) const;
    QKeySequence tabShortcut(TabTypes tab_type) const;

protected:
    virtual bool eventFilter(QObject* obj, QEvent *event) override;
    void keyPressEvent(QKeyEvent *) override;

private Q_SLOTS:
    /** custom tab buttons clicked */
    void showPage(int index);
    void showInfoView(int index);
    void on_lineEdit_returnPressed();
    void on_stackedWidgetRPC_currentChanged(int index);
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
    /** show detailed information on ui about selected node */
    void updateDetailWidget();

public Q_SLOTS:
    void clear(bool keep_prompt = false);
    void fontBigger();
    void fontSmaller();
    void setFontSize(int newSize);

    /** Repair options */
    void walletReindex();
#ifdef ENABLE_WALLET
    void walletRescan1();
    void walletRescan2();
#endif // ENABLE_WALLET

    /** Append the message to the message widget */
    void message(int category, const QString &msg) { message(category, msg, false); }
    void message(int category, const QString &message, bool html);
    /** Go forward or back in history */
    void browseHistory(int offset);
    /** Scroll console view to end */
    void scrollToEnd();
    /** Disconnect a selected node on the Peers tab */
    void disconnectSelectedNode();
    /** Ban a selected node on the Peers tab */
    void banSelectedNode(int bantime);
    /** Unban a selected node on the Bans tab */
    void unbanSelectedNode();
    /** set which tab has the focus (is visible) */
    void setTabFocus(enum TabTypes tabType);
    /** switch the info sub-view to the given InfoView */
    void setInfoView(InfoView view);
#ifdef ENABLE_WALLET
    /** Set the current (ie - active) wallet */
    void setCurrentWallet(WalletModel* const wallet_model);
#endif // ENABLE_WALLET

Q_SIGNALS:
    /** Get restart command-line parameters and handle restart */
    void handleRestart(QStringList args);

private:
    struct TranslatedStrings {
        const QString yes{tr("Yes")}, no{tr("No")}, to{tr("To")}, from{tr("From")},
            ban_for{tr("Ban for")}, na{tr("N/A")}, unknown{tr("Unknown")};
    } const ts;

    void startExecutor();
    void setTrafficGraphRange(TrafficGraphData::GraphRange range);
    /** Build parameter list for restart */
    void buildParameterlist(QString arg);
    /** Set required icons for buttons inside the dialog */
    void setButtonIcons();
    /** Reload some themes related widgets */
    void reloadThemedWidgets();
#ifdef ENABLE_WALLET
    /** Initiate a wallet rescan */
    void walletRescan(bool from_genesis);
    /** Update wallet UI when selected wallet changes */
    void onWalletChanged();
#endif // ENABLE_WALLET

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
    WalletController* m_wallet_controller{nullptr};
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
    RPCExecutor* m_executor{nullptr};
    WalletModel* m_last_wallet_model{nullptr};
    bool m_is_executing{false};
    QByteArray m_peer_widget_header_state;
    QByteArray m_banlist_widget_header_state;
    MasternodeFeed* m_feed_masternode{nullptr};

    /** Helper for the output of a time duration field. Inputs are UNIX epoch times. */
    QString TimeDurationField(std::chrono::seconds time_now, std::chrono::seconds time_at_event) const
    {
        return time_at_event.count() ? GUIUtil::formatDurationStr(time_now - time_at_event) : tr("Never");
    }

private Q_SLOTS:
    void updateAlerts(const QString& warnings);
};

#endif // BITCOIN_QT_RPCCONSOLE_H
