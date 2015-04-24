// Copyright (c) 2011-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOINGUI_H
#define BITCOINGUI_H

#if defined(HAVE_CONFIG_H)
#include "bitcredit-config.h"
#endif

#include <QMainWindow>
#include <QMap>
#include <QSystemTrayIcon>

class ClientModel;
class Notificator;
class RPCConsole;
class Bitcoin_RPCConsole;
class Bitcredit_SendCoinsRecipient;
class WalletFrame;
class Bitcredit_WalletModel;
class Bitcoin_WalletModel;

class Bitcredit_CWallet;
class Bitcoin_CWallet;

QT_BEGIN_NAMESPACE
class QAction;
class QLabel;
class QProgressBar;
class QProgressDialog;
QT_END_NAMESPACE

/**
  Bitcredit GUI main class. This class represents the main window of the Bitcredit UI. It communicates with both the client and
  wallet models to give the user an up-to-date view of the current core state.
*/
class BitcreditGUI : public QMainWindow
{
    Q_OBJECT

public:
    static const QString DEFAULT_WALLET;

    explicit BitcreditGUI(bool fIsTestnet = false, QWidget *parent = 0);
    ~BitcreditGUI();

    /** Set the client model.
        The client model represents the part of the core that communicates with the P2P network, and is wallet-agnostic.
    */
    void bitcredit_setClientModel(ClientModel *clientModel);
    void bitcoin_setClientModel(ClientModel *clientModel);

#ifdef ENABLE_WALLET
    /** Set the wallet model.
        The wallet model represents a bitcoin wallet, and offers access to the list of transactions, address book and sending
        functionality.
    */
    bool addWallet(const QString& name, Bitcredit_WalletModel *bitcredit_model, Bitcoin_WalletModel *bitcoin_model, Bitcredit_WalletModel *deposit_model);
    bool setCurrentWallet(const QString& name);
    void removeAllWallets();
#endif

protected:
    void changeEvent(QEvent *e);
    void closeEvent(QCloseEvent *event);
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);
    bool eventFilter(QObject *object, QEvent *event);

private:
    ClientModel *credits_clientModel;
    ClientModel *bitcoin_clientModel;
    WalletFrame *walletFrame;

    QLabel *bitcredit_labelEncryptionIcon;
    QLabel *bitcredit_labelConnectionsIcon;
    QLabel *bitcredit_labelBlocksIcon;
    QLabel *bitcredit_progressBarLabel;
    QProgressBar *bitcredit_progressBar;

    QLabel *bitcoin_labelEncryptionIcon;
    QLabel *bitcoin_labelConnectionsIcon;
    QLabel *bitcoin_labelBlocksIcon;
    QLabel *bitcoin_progressBarLabel;
    QProgressBar *bitcoin_progressBar;

    QProgressDialog *progressDialog;

    QMenuBar *appMenuBar;
    QAction *overviewAction;
    QAction *claimCoinsAction;
    QAction *historyAction;
    QAction *minerDepositsAction;
    QAction *minerAction;
    QAction *quitAction;
    QAction *sendCoinsAction;
    QAction *usedSendingAddressesAction;
    QAction *usedReceivingAddressesAction;
    QAction *signMessageAction;
    QAction *verifyMessageAction;
    QAction *aboutAction;
    QAction *receiveCoinsAction;
    QAction *optionsAction;
    QAction *toggleHideAction;

    QAction *bitcredit_encryptWalletAction;
    QAction *bitcredit_backupWalletAction;
    QAction *bitcredit_changePassphraseAction;

    QAction *bitcoin_encryptWalletAction;
    QAction *bitcoin_backupWalletAction;
    QAction *bitcoin_changePassphraseAction;

    QAction *aboutQtAction;
    QAction *bitcredit_openRPCConsoleAction;
    QAction *bitcoin_openRPCConsoleAction;
    QAction *openAction;
    QAction *showHelpMessageAction;

    QSystemTrayIcon *trayIcon;
    Notificator *notificator;
    RPCConsole *bitcredit_rpcConsole;
    Bitcoin_RPCConsole *bitcoin_rpcConsole;

    /** Keep track of previous number of blocks, to detect progress */
    int bitcredit_prevBlocks;
    int bitcoin_prevBlocks;
    int bitcredit_spinnerFrame;
    int bitcoin_spinnerFrame;

    /** Create the main UI actions. */
    void createActions(bool fIsTestnet);
    /** Create the menu bar and sub-menus. */
    void createMenuBar();
    /** Create the toolbars */
    void createToolBars();
    /** Create system tray icon and notification */
    void createTrayIcon(bool fIsTestnet);
    /** Create system tray menu (or setup the dock menu) */
    void createTrayIconMenu();

    /** Enable or disable all wallet-related actions */
    void setWalletActionsEnabled(bool enabled);

    /** Connect core signals to GUI client */
    void subscribeToCoreSignals();
    /** Disconnect core signals from GUI client */
    void unsubscribeFromCoreSignals();

signals:
    /** Signal raised when a URI was entered or dragged to the GUI */
    void receivedURI(const QString &uri);

public slots:
    /** Set number of connections shown in the UI */
    void bitcredit_setNumConnections(int count);
    void bitcoin_setNumConnections(int count);
    /** Set number of blocks shown in the UI */
    void bitcredit_setNumBlocks(int count);
    void bitcoin_setNumBlocks(int count);

    /** Notify the user of an event from the core network or transaction handling code.
       @param[in] title     the message box / notification title
       @param[in] message   the displayed text
       @param[in] style     modality and style definitions (icon and used buttons - buttons only for message boxes)
                            @see CClientUIInterface::MessageBoxFlags
       @param[in] ret       pointer to a bool that will be modified to whether Ok was clicked (modal only)
    */
    void message(const QString &title, const QString &message, unsigned int style, bool *ret = NULL);

#ifdef ENABLE_WALLET
    /** Set the encryption status as shown in the UI.
       @param[in] status            current encryption status
       @see WalletModel::EncryptionStatus
    */
    void bitcredit_setEncryptionStatus(int status);
    void bitcoin_setEncryptionStatus(int status);

    bool handlePaymentRequest(const Bitcredit_SendCoinsRecipient& recipient);

    /** Show incoming transaction notification for new transactions. */
    void incomingTransaction(const QString& date, int unit, qint64 amount, const QString& type, const QString& address);
#endif

private slots:
#ifdef ENABLE_WALLET
    /** Switch to overview (home) page */
    void gotoOverviewPage();
    /** Switch to miner management page */
    void gotoClaimCoinsPage();
    /** Switch to history (transactions) page */
    void gotoHistoryPage();
    /** Switch to miner deposits page */
    void gotoMinerDepositsPage();
    /** Switch to receive coins page */
    void gotoReceiveCoinsPage();
    /** Switch to send coins page */
    void gotoSendCoinsPage(QString addr = "");
    /** Switch to send coins page */
    void gotoMinerCoinsPage(QString addr = "");

    /** Show Sign/Verify Message dialog and switch to sign message tab */
    void gotoSignMessageTab(QString addr = "");
    /** Show Sign/Verify Message dialog and switch to verify message tab */
    void gotoVerifyMessageTab(QString addr = "");

    /** Show open dialog */
    void openClicked();
#endif
    /** Show configuration dialog */
    void optionsClicked();
    /** Show about dialog */
    void aboutClicked();
    /** Show help message dialog */
    void showHelpMessageClicked();
#ifndef Q_OS_MAC
    /** Handle tray icon clicked */
    void trayIconActivated(QSystemTrayIcon::ActivationReason reason);
#endif

    /** Show window if hidden, unminimize when minimized, rise when obscured or show if hidden and fToggleHidden is true */
    void showNormalIfMinimized(bool fToggleHidden = false);
    /** Simply calls showNormalIfMinimized(true) for use in SLOT() macro */
    void toggleHidden();

    /** called by a timer to check if fRequestShutdown has been set **/
    void detectShutdown();

    /** Show progress dialog e.g. for verifychain */
    void showProgress(const QString &title, int nProgress);
};

#endif // BITCOINGUI_H
