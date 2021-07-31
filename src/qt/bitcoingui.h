// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_BITCOINGUI_H
#define BITCOIN_QT_BITCOINGUI_H

#if defined(HAVE_CONFIG_H)
#include <config/dash-config.h>
#endif

#include <amount.h>

#include <QLabel>
#include <QMainWindow>
#include <QMap>
#include <QMenu>
#include <QPoint>
#include <QPushButton>
#include <QSystemTrayIcon>

#include <memory>

#ifdef Q_OS_MAC
#include <qt/macos_appnap.h>
#endif

class ClientModel;
class NetworkStyle;
class Notificator;
class OptionsModel;
class RPCConsole;
class SendCoinsRecipient;
class UnitDisplayStatusBarControl;
class WalletFrame;
class WalletModel;
class HelpMessageDialog;
class ModalOverlay;

namespace interfaces {
class Handler;
class Node;
}

QT_BEGIN_NAMESPACE
class QAction;
class QButtonGroup;
class QComboBox;
class QProgressBar;
class QProgressDialog;
class QToolButton;
QT_END_NAMESPACE

/**
  Bitcoin GUI main class. This class represents the main window of the Bitcoin UI. It communicates with both the client and
  wallet models to give the user an up-to-date view of the current core state.
*/
class BitcoinGUI : public QMainWindow
{
    Q_OBJECT

public:
    static const std::string DEFAULT_UIPLATFORM;

    explicit BitcoinGUI(interfaces::Node& node, const NetworkStyle* networkStyle, QWidget* parent = 0);
    ~BitcoinGUI();

    /** Set the client model.
        The client model represents the part of the core that communicates with the P2P network, and is wallet-agnostic.
    */
    void setClientModel(ClientModel *clientModel);

#ifdef ENABLE_WALLET
    /** Set the wallet model.
        The wallet model represents a bitcoin wallet, and offers access to the list of transactions, address book and sending
        functionality.
    */
    bool addWallet(WalletModel *walletModel);
    bool removeWallet(WalletModel* walletModel);
    void removeAllWallets();
#endif // ENABLE_WALLET
    bool enableWallet = false;

    /** Get the tray icon status.
        Some systems have not "system tray" or "notification area" available.
    */
    bool hasTrayIcon() const { return trayIcon; }

protected:
    void changeEvent(QEvent *e) override;
    void closeEvent(QCloseEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    bool eventFilter(QObject *object, QEvent *event) override;

private:
    interfaces::Node& m_node;
    std::unique_ptr<interfaces::Handler> m_handler_message_box;
    std::unique_ptr<interfaces::Handler> m_handler_question;
    ClientModel* clientModel = nullptr;
    WalletFrame* walletFrame = nullptr;

    UnitDisplayStatusBarControl* unitDisplayControl = nullptr;
    QLabel* labelWalletEncryptionIcon = nullptr;
    QLabel* labelWalletHDStatusIcon = nullptr;
    QLabel* labelProxyIcon = nullptr;
    QLabel* labelConnectionsIcon = nullptr;
    QLabel* labelBlocksIcon = nullptr;
    QLabel* progressBarLabel = nullptr;
    QProgressBar* progressBar = nullptr;
    QProgressDialog* progressDialog = nullptr;

    QMenuBar* appMenuBar = nullptr;
    QToolBar* appToolBar = nullptr;
    QToolButton* overviewButton = nullptr;
    QToolButton* sendCoinsButton = nullptr;
    QToolButton* coinJoinCoinsButton = nullptr;
    QToolButton* receiveCoinsButton = nullptr;
    QToolButton* historyButton = nullptr;
    QToolButton* masternodeButton = nullptr;
    QAction* appToolBarLogoAction = nullptr;
    QAction* quitAction = nullptr;
    QAction* sendCoinsMenuAction = nullptr;
    QAction* coinJoinCoinsMenuAction = nullptr;
    QAction* usedSendingAddressesAction = nullptr;
    QAction* usedReceivingAddressesAction = nullptr;
    QAction* signMessageAction = nullptr;
    QAction* verifyMessageAction = nullptr;
    QAction* aboutAction = nullptr;
    QAction* receiveCoinsMenuAction = nullptr;
    QAction* optionsAction = nullptr;
    QAction* toggleHideAction = nullptr;
    QAction* encryptWalletAction = nullptr;
    QAction* backupWalletAction = nullptr;
    QAction* changePassphraseAction = nullptr;
    QAction* unlockWalletAction = nullptr;
    QAction* lockWalletAction = nullptr;
    QAction* aboutQtAction = nullptr;
    QAction* openInfoAction = nullptr;
    QAction* openRPCConsoleAction = nullptr;
    QAction* openGraphAction = nullptr;
    QAction* openPeersAction = nullptr;
    QAction* openRepairAction = nullptr;
    QAction* openConfEditorAction = nullptr;
    QAction* showBackupsAction = nullptr;
    QAction* openAction = nullptr;
    QAction* showHelpMessageAction = nullptr;
    QAction* showCoinJoinHelpAction = nullptr;
    QAction* m_wallet_selector_action = nullptr;

    QComboBox* m_wallet_selector = nullptr;

    QSystemTrayIcon* trayIcon = nullptr;
    QMenu* trayIconMenu = nullptr;
    QMenu* dockIconMenu = nullptr;
    Notificator* notificator = nullptr;
    RPCConsole* rpcConsole = nullptr;
    HelpMessageDialog* helpMessageDialog = nullptr;
    ModalOverlay* modalOverlay = nullptr;
    QButtonGroup* tabGroup = nullptr;

#ifdef Q_OS_MAC
    CAppNapInhibitor* m_app_nap_inhibitor = nullptr;
#endif

    /** Timer to update the spinner animation in the status bar periodically */
    QTimer* timerSpinner = nullptr;
    /** Start the spinner animation in the status bar if it's not running and if labelBlocksIcon is visible. */
    void startSpinner();
    /** Stop the spinner animation in the status bar */
    void stopSpinner();

    /** Timer to update the connection icon during connecting phase */
    QTimer* timerConnecting = nullptr;
    /** Start the connecting animation */
    void startConnectingAnimation();
    /** Stop the connecting animation */
    void stopConnectingAnimation();

    struct IncomingTransactionMessage {
        QString date;
        int unit;
        CAmount amount;
        QString type;
        QString address;
        QString label;
        QString walletName;
    };
    std::list<IncomingTransactionMessage> incomingTransactions;
    QTimer* incomingTransactionsTimer = nullptr;

    /** Timer to update custom css styling in -debug-ui mode periodically */
    QTimer* timerCustomCss = nullptr;

    /** Create the main UI actions. */
    void createActions();
    /** Create the menu bar and sub-menus. */
    void createMenuBar();
    /** Create the toolbars */
    void createToolBars();
    /** Create system tray icon and notification */
    void createTrayIcon(const NetworkStyle *networkStyle);
    /** Create system tray menu (or setup the dock menu) */
    void createIconMenu(QMenu *pmenu);

    /** Enable or disable all wallet-related actions */
    void setWalletActionsEnabled(bool enabled);

    /** Connect core signals to GUI client */
    void subscribeToCoreSignals();
    /** Disconnect core signals from GUI client */
    void unsubscribeFromCoreSignals();

    /** Update UI with latest network info from model. */
    void updateNetworkState();

    void updateHeadersSyncProgressLabel();

    void updateProgressBarVisibility();

Q_SIGNALS:
    /** Signal raised when a URI was entered or dragged to the GUI */
    void receivedURI(const QString &uri);
    /** Restart handling */
    void requestedRestart(QStringList args);

public Q_SLOTS:
    /** Set number of connections shown in the UI */
    void setNumConnections(int count);
    /** Set network state shown in the UI */
    void setNetworkActive(bool networkActive);
    /** Get restart command-line parameters and request restart */
    void handleRestart(QStringList args);
    /** Set number of blocks and last block date shown in the UI */
    void setNumBlocks(int count, const QDateTime& blockDate, const QString& blockHash, double nVerificationProgress, bool headers);
    /** Set additional data sync status shown in the UI */
    void setAdditionalDataSyncProgress(double nSyncProgress);

    /** Notify the user of an event from the core network or transaction handling code.
       @param[in] title     the message box / notification title
       @param[in] message   the displayed text
       @param[in] style     modality and style definitions (icon and used buttons - buttons only for message boxes)
                            @see CClientUIInterface::MessageBoxFlags
       @param[in] ret       pointer to a bool that will be modified to whether Ok was clicked (modal only)
    */
    void message(const QString &title, const QString &message, unsigned int style, bool *ret = nullptr);

#ifdef ENABLE_WALLET
    bool setCurrentWallet(WalletModel* wallet_model);
    bool setCurrentWalletBySelectorIndex(int index);
    /** Set the UI status indicators based on the currently selected wallet.
    */
    void updateWalletStatus();

private:
    /** Set the encryption status as shown in the UI.
       @param[in] status            current encryption status
       @see WalletModel::EncryptionStatus
    */
    void setEncryptionStatus(int status);

    /** Set the hd-enabled status as shown in the UI.
     @param[in] hdEnabled         current hd enabled status
     @see WalletModel::EncryptionStatus
     */
    void setHDStatus(int hdEnabled);

public Q_SLOTS:
    bool handlePaymentRequest(const SendCoinsRecipient& recipient);

    /** Show incoming transaction notification for new transactions. */
    void incomingTransaction(const QString& date, int unit, const CAmount& amount, const QString& type, const QString& address, const QString& label, const QString& walletName);
    void showIncomingTransactions();
#endif // ENABLE_WALLET

private:
    /** Set the proxy-enabled icon as shown in the UI. */
    void updateProxyIcon();

private Q_SLOTS:
#ifdef ENABLE_WALLET
    /** Switch to overview (home) page */
    void gotoOverviewPage();
    /** Switch to history (transactions) page */
    void gotoHistoryPage();
    /** Switch to masternode page */
    void gotoMasternodePage();
    /** Switch to receive coins page */
    void gotoReceiveCoinsPage();
    /** Switch to send coins page */
    void gotoSendCoinsPage(QString addr = "");
    /** Switch to CoinJoin coins page */
    void gotoCoinJoinCoinsPage(QString addr = "");

    /** Show Sign/Verify Message dialog and switch to sign message tab */
    void gotoSignMessageTab(QString addr = "");
    /** Show Sign/Verify Message dialog and switch to verify message tab */
    void gotoVerifyMessageTab(QString addr = "");

    /** Show open dialog */
    void openClicked();

    /** Highlight checked tab button */
    void highlightTabButton(QAbstractButton *button, bool checked);
#endif // ENABLE_WALLET
    /** Show configuration dialog */
    void optionsClicked();
    /** Show about dialog */
    void aboutClicked();
    /** Show debug window */
    void showDebugWindow();

    /** Show debug window and set focus to the appropriate tab */
    void showInfo();
    void showConsole();
    void showGraph();
    void showPeers();
    void showRepair();

    /** Open external (default) editor with dash.conf */
    void showConfEditor();
    /** Show folder with wallet backups in default file browser */
    void showBackups();

    /** Show help message dialog */
    void showHelpMessageClicked();
    /** Show CoinJoin help message dialog */
    void showCoinJoinHelpClicked();
#ifndef Q_OS_MAC
    /** Handle tray icon clicked */
    void trayIconActivated(QSystemTrayIcon::ActivationReason reason);
#else
    /** Handle macOS Dock icon clicked */
    void macosDockIconActivated();
#endif

    /** Show window if hidden, unminimize when minimized, rise when obscured or show if hidden and fToggleHidden is true */
    void showNormalIfMinimized(bool fToggleHidden = false);
    /** Simply calls showNormalIfMinimized(true) for use in SLOT() macro */
    void toggleHidden();

    /** called by a timer to check if ShutdownRequested() has been set **/
    void detectShutdown();

    /** Show progress dialog e.g. for verifychain */
    void showProgress(const QString &title, int nProgress);

    /** When hideTrayIcon setting is changed in OptionsModel hide or show the icon accordingly. */
    void setTrayIconVisible(bool);

    /** Toggle networking */
    void toggleNetworkActive();

    void showModalOverlay();

    void updateCoinJoinVisibility();

    void updateWidth();
};

class UnitDisplayStatusBarControl : public QLabel
{
    Q_OBJECT

public:
    explicit UnitDisplayStatusBarControl();
    /** Lets the control know about the Options Model (and its signals) */
    void setOptionsModel(OptionsModel *optionsModel);

protected:
    /** So that it responds to left-button clicks */
    void mousePressEvent(QMouseEvent *event) override;

private:
    OptionsModel *optionsModel;
    QMenu* menu;

    /** Shows context menu with Display Unit options by the mouse coordinates */
    void onDisplayUnitsClicked(const QPoint& point);
    /** Creates context menu, its actions, and wires up all the relevant signals for mouse events. */
    void createContextMenu();

private Q_SLOTS:
    /** When Display Units are changed on OptionsModel it will refresh the display text of the control on the status bar */
    void updateDisplayUnit(int newUnits);
    /** Tells underlying optionsModel to update its current display unit. */
    void onMenuSelection(QAction* action);
};

#endif // BITCOIN_QT_BITCOINGUI_H
