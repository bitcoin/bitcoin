// Copyright (c) 2011-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bitcoingui.h"

#include "bitcreditunits.h"
#include "clientmodel.h"
#include "guiconstants.h"
#include "guiutil.h"
#include "notificator.h"
#include "openuridialog.h"
#include "optionsdialog.h"
#include "optionsmodel.h"
#include "rpcconsole.h"
#include "bitcoin_rpcconsole.h"
#include "utilitydialog.h"
#ifdef ENABLE_WALLET
#include "walletframe.h"
#include "walletmodel.h"
#endif

#ifdef Q_OS_MAC
#include "macdockiconhandler.h"
#endif

#include "init.h"
#include "ui_interface.h"

#include <iostream>

#include <QApplication>
#include <QDateTime>
#include <QDesktopWidget>
#include <QDragEnterEvent>
#include <QIcon>
#include <QLabel>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QProgressBar>
#include <QProgressDialog>
#include <QSettings>
#include <QStackedWidget>
#include <QStatusBar>
#include <QStyle>
#include <QTimer>
#include <QToolBar>
#include <QVBoxLayout>
#include <QStyleFactory>

#if QT_VERSION < 0x050000
#include <QUrl>
#include <QTextDocument>
#else
#include <QUrlQuery>
#endif

const QString BitcreditGUI::DEFAULT_WALLET = "~Default";

BitcreditGUI::BitcreditGUI(bool fIsTestnet, QWidget *parent) :
    QMainWindow(parent),
    credits_clientModel(0),
    bitcoin_clientModel(0),
    walletFrame(0),

    bitcredit_encryptWalletAction(0),
    bitcredit_changePassphraseAction(0),

    bitcoin_encryptWalletAction(0),
    bitcoin_changePassphraseAction(0),

    aboutQtAction(0),
    trayIcon(0),
    notificator(0),
    bitcredit_rpcConsole(0),
    bitcoin_rpcConsole(0),
    bitcredit_prevBlocks(0),
    bitcoin_prevBlocks(0),
    bitcredit_spinnerFrame(0),
    bitcoin_spinnerFrame(0)
{
    GUIUtil::restoreWindowGeometry("nWindow", QSize(850, 550), this);

    QString windowTitle = tr("Credits Core") + " - ";
#ifdef ENABLE_WALLET
    /* if compiled with wallet support, -disablewallet can still disable the wallet */
    bool enableWallet = !GetBoolArg("-disablewallet", false);
#else
    bool enableWallet = false;
#endif
    if(enableWallet)
    {
        windowTitle += tr("Wallet");
    } else {
        windowTitle += tr("Node");
    }

    if (!fIsTestnet)
    {
#ifndef Q_OS_MAC
        QApplication::setWindowIcon(QIcon(":icons/bitcoin"));
        setWindowIcon(QIcon(":icons/bitcoin"));
#else
        MacDockIconHandler::instance()->setIcon(QIcon(":icons/bitcoin"));
#endif
    }
    else
    {
        windowTitle += " " + tr("[testnet]");
#ifndef Q_OS_MAC
        QApplication::setWindowIcon(QIcon(":icons/bitcredit_testnet"));
        setWindowIcon(QIcon(":icons/bitcredit_testnet"));
#else
        MacDockIconHandler::instance()->setIcon(QIcon(":icons/bitcredit_testnet"));
#endif
    }
    setWindowTitle(windowTitle);

#if defined(Q_OS_MAC) && QT_VERSION < 0x050000
    // This property is not implemented in Qt 5. Setting it has no effect.
    // A replacement API (QtMacUnifiedToolBar) is available in QtMacExtras.
    setUnifiedTitleAndToolBarOnMac(true);
#endif

    bitcredit_rpcConsole = new RPCConsole(enableWallet ? this : 0);
    bitcoin_rpcConsole = new Bitcoin_RPCConsole(enableWallet ? this : 0);
#ifdef ENABLE_WALLET
    if(enableWallet)
    {
        /** Create wallet frame and make it the central widget */
        walletFrame = new WalletFrame(this);
        setCentralWidget(walletFrame);
    } else
#endif
    {
        /* When compiled without wallet or -disablewallet is provided,
         * the central widget is the rpc console.
         */
        setCentralWidget(bitcredit_rpcConsole);
    }

    // Accept D&D of URIs
    setAcceptDrops(true);

    // Create actions for the toolbar, menu bar and tray/dock icon
    // Needs walletFrame to be initialized
    createActions(fIsTestnet);

    // Create application menu bar
    createMenuBar();

    // Create the toolbars
    createToolBars();

    // Create system tray icon and notification
    createTrayIcon(fIsTestnet);

    // Create status bar
    statusBar();

    // Status bar notification icons
    QFrame *bitcredit_frameBlocks = new QFrame();
    bitcredit_frameBlocks->setContentsMargins(0,0,0,0);
    bitcredit_frameBlocks->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    QHBoxLayout *bitcredit_frameBlocksLayout = new QHBoxLayout(bitcredit_frameBlocks);
    bitcredit_frameBlocksLayout->setContentsMargins(3,0,3,0);
    bitcredit_frameBlocksLayout->setSpacing(3);
    bitcredit_labelEncryptionIcon = new QLabel();
    bitcredit_labelConnectionsIcon = new QLabel();
    bitcredit_labelBlocksIcon = new QLabel();
    bitcredit_frameBlocksLayout->addStretch();
    bitcredit_frameBlocksLayout->addWidget(bitcredit_labelEncryptionIcon);
    bitcredit_frameBlocksLayout->addStretch();
    bitcredit_frameBlocksLayout->addWidget(bitcredit_labelConnectionsIcon);
    bitcredit_frameBlocksLayout->addStretch();
    bitcredit_frameBlocksLayout->addWidget(bitcredit_labelBlocksIcon);
    bitcredit_frameBlocksLayout->addStretch();

    // Progress bar and label for blocks download
    bitcredit_progressBarLabel = new QLabel();
    bitcredit_progressBarLabel->setVisible(false);
    bitcredit_progressBar = new QProgressBar();
    bitcredit_progressBar->setAlignment(Qt::AlignCenter);
    bitcredit_progressBar->setVisible(false);

    // Override style sheet for progress bar for styles that have a segmented progress bar,
    // as they make the text unreadable (workaround for issue #1071)
    // See https://qt-project.org/doc/qt-4.8/gallery.html
    QString bitcredit_curStyle = QApplication::style()->metaObject()->className();
    if(bitcredit_curStyle == "QWindowsStyle" || bitcredit_curStyle == "QWindowsXPStyle")
    {
        bitcredit_progressBar->setStyleSheet("QProgressBar { background-color: #e8e8e8; border: 1px solid grey; border-radius: 7px; padding: 1px; text-align: center; } QProgressBar::chunk { background: QLinearGradient(x1: 0, y1: 0, x2: 1, y2: 0, stop: 0 #FF8000, stop: 1 orange); border-radius: 7px; margin: 0px; }");
    }

    statusBar()->addWidget(bitcredit_progressBarLabel);
    statusBar()->addWidget(bitcredit_progressBar);
    statusBar()->addPermanentWidget(bitcredit_frameBlocks);


    // Status bar notification icons
    QFrame *bitcoin_frameBlocks = new QFrame();
    bitcoin_frameBlocks->setContentsMargins(0,0,0,0);
    bitcoin_frameBlocks->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    QHBoxLayout *bitcoin_frameBlocksLayout = new QHBoxLayout(bitcoin_frameBlocks);
    bitcoin_frameBlocksLayout->setContentsMargins(3,0,3,0);
    bitcoin_frameBlocksLayout->setSpacing(3);
    bitcoin_labelEncryptionIcon = new QLabel();
    bitcoin_labelConnectionsIcon = new QLabel();
    bitcoin_labelBlocksIcon = new QLabel();
    bitcoin_frameBlocksLayout->addStretch();
    bitcoin_frameBlocksLayout->addWidget(bitcoin_labelEncryptionIcon);
    bitcoin_frameBlocksLayout->addStretch();
    bitcoin_frameBlocksLayout->addWidget(bitcoin_labelConnectionsIcon);
    bitcoin_frameBlocksLayout->addStretch();
    bitcoin_frameBlocksLayout->addWidget(bitcoin_labelBlocksIcon);
    bitcoin_frameBlocksLayout->addStretch();

    // Progress bar and label for blocks download
    bitcoin_progressBarLabel = new QLabel();
    bitcoin_progressBarLabel->setVisible(false);
    bitcoin_progressBar = new QProgressBar();
    bitcoin_progressBar->setAlignment(Qt::AlignCenter);
    bitcoin_progressBar->setVisible(false);

    // Override style sheet for progress bar for styles that have a segmented progress bar,
    // as they make the text unreadable (workaround for issue #1071)
    // See https://qt-project.org/doc/qt-4.8/gallery.html
    QString bitcoin_curStyle = QApplication::style()->metaObject()->className();
    if(bitcoin_curStyle == "QWindowsStyle" || bitcoin_curStyle == "QWindowsXPStyle")
    {
        bitcoin_progressBar->setStyleSheet("QProgressBar { background-color: #e8e8e8; border: 1px solid grey; border-radius: 7px; padding: 1px; text-align: center; } QProgressBar::chunk { background: QLinearGradient(x1: 0, y1: 0, x2: 1, y2: 0, stop: 0 #FF8000, stop: 1 orange); border-radius: 7px; margin: 0px; }");
    }

    statusBar()->addWidget(bitcoin_progressBarLabel);
    statusBar()->addWidget(bitcoin_progressBar);
    statusBar()->addPermanentWidget(bitcoin_frameBlocks);


    connect(bitcredit_openRPCConsoleAction, SIGNAL(triggered()), bitcredit_rpcConsole, SLOT(show()));
    // prevents an open debug window from becoming stuck/unusable on client shutdown
    connect(quitAction, SIGNAL(triggered()), bitcredit_rpcConsole, SLOT(hide()));

    connect(bitcoin_openRPCConsoleAction, SIGNAL(triggered()), bitcoin_rpcConsole, SLOT(show()));
    // prevents an open debug window from becoming stuck/unusable on client shutdown
    connect(quitAction, SIGNAL(triggered()), bitcoin_rpcConsole, SLOT(hide()));

    // Install event filter to be able to catch status tip events (QEvent::StatusTip)
    this->installEventFilter(this);

    // Initially wallet actions should be disabled
    setWalletActionsEnabled(false);

    // Subscribe to notifications from core
    subscribeToCoreSignals();
}

BitcreditGUI::~BitcreditGUI()
{
    // Unsubscribe from notifications from core
    unsubscribeFromCoreSignals();

    GUIUtil::saveWindowGeometry("nWindow", this);
    if(trayIcon) // Hide tray icon, as deleting will let it linger until quit (on Ubuntu)
        trayIcon->hide();
#ifdef Q_OS_MAC
    delete appMenuBar;
    MacDockIconHandler::instance()->setMainWindow(NULL);
#endif
}

void BitcreditGUI::createActions(bool fIsTestnet)
{
    QActionGroup *tabGroup = new QActionGroup(this);

    overviewAction = new QAction(QIcon(":/icons/overview"), tr("&Overview"), this);
    overviewAction->setStatusTip(tr("Show general overview of wallet"));
    overviewAction->setToolTip(overviewAction->statusTip());
    overviewAction->setCheckable(true);
    overviewAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_1));
    tabGroup->addAction(overviewAction);

    sendCoinsAction = new QAction(QIcon(":/icons/send"), tr("&Send"), this);
    sendCoinsAction->setStatusTip(tr("Send coins to a Credits address"));
    sendCoinsAction->setToolTip(sendCoinsAction->statusTip());
    sendCoinsAction->setCheckable(true);
    sendCoinsAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_2));
    tabGroup->addAction(sendCoinsAction);

    receiveCoinsAction = new QAction(QIcon(":/icons/receiving_addresses"), tr("&Receive"), this);
    receiveCoinsAction->setStatusTip(tr("Request payments (generates QR codes and bitcredit: URIs)"));
    receiveCoinsAction->setToolTip(receiveCoinsAction->statusTip());
    receiveCoinsAction->setCheckable(true);
    receiveCoinsAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_3));
    tabGroup->addAction(receiveCoinsAction);

    historyAction = new QAction(QIcon(":/icons/history"), tr("&Transactions"), this);
    historyAction->setStatusTip(tr("Browse transaction history"));
    historyAction->setToolTip(historyAction->statusTip());
    historyAction->setCheckable(true);
    historyAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_4));
    tabGroup->addAction(historyAction);

    claimCoinsAction = new QAction(QIcon(":/icons/claim"), tr("&Claim bitcoins"), this);
    claimCoinsAction->setStatusTip(tr("Claim bitcoins"));
    claimCoinsAction->setToolTip(claimCoinsAction->statusTip());
    claimCoinsAction->setCheckable(true);
    claimCoinsAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_6));
    tabGroup->addAction(claimCoinsAction);


    minerAction = new QAction(QIcon(":/icons/mining"), tr("&Prepare deposits"), this);
    minerAction->setStatusTip(tr("Manage mining"));
    minerAction->setToolTip(minerAction->statusTip());
    minerAction->setCheckable(true);
    minerAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_7));
    tabGroup->addAction(minerAction);

    minerDepositsAction = new QAction(QIcon(":/icons/mining_deposits"), tr("&Prepared deposits"), this);
    minerDepositsAction->setStatusTip(tr("Browse prepared but not yet used miner deposits"));
    minerDepositsAction->setToolTip(minerDepositsAction->statusTip());
    minerDepositsAction->setCheckable(true);
    minerDepositsAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_8));
    tabGroup->addAction(minerDepositsAction);

    bitcoin_overviewAction = new QAction(QIcon(":/icons/bitcoin_overview"), tr("&Overview"), this);
    bitcoin_overviewAction->setStatusTip(tr("Show general overview of bitcoin wallet"));
    bitcoin_overviewAction->setToolTip(bitcoin_overviewAction->statusTip());
    bitcoin_overviewAction->setCheckable(true);
    bitcoin_overviewAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_9));
    tabGroup->addAction(bitcoin_overviewAction);

    bitcoin_sendCoinsAction = new QAction(QIcon(":/icons/bitcoin_send"), tr("&Send"), this);
    bitcoin_sendCoinsAction->setStatusTip(tr("Send bitcoins to a Bitcoin address"));
    bitcoin_sendCoinsAction->setToolTip(bitcoin_sendCoinsAction->statusTip());
    bitcoin_sendCoinsAction->setCheckable(true);
    bitcoin_sendCoinsAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_A));
    tabGroup->addAction(bitcoin_sendCoinsAction);

    bitcoin_receiveCoinsAction = new QAction(QIcon(":/icons/bitcoin_receiving_addresses"), tr("&Receive"), this);
    bitcoin_receiveCoinsAction->setStatusTip(tr("Request bitcoin payments (generates QR codes and bitcoin: URIs)"));
    bitcoin_receiveCoinsAction->setToolTip(bitcoin_receiveCoinsAction->statusTip());
    bitcoin_receiveCoinsAction->setCheckable(true);
    bitcoin_receiveCoinsAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_B));
    tabGroup->addAction(bitcoin_receiveCoinsAction);

    bitcoin_historyAction = new QAction(QIcon(":/icons/bitcoin_history"), tr("&Transactions"), this);
    bitcoin_historyAction->setStatusTip(tr("Browse bitcoin transaction history"));
    bitcoin_historyAction->setToolTip(bitcoin_historyAction->statusTip());
    bitcoin_historyAction->setCheckable(true);
    bitcoin_historyAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_C));
    tabGroup->addAction(bitcoin_historyAction);

    // These showNormalIfMinimized are needed because Send Coins and Receive Coins
    // can be triggered from the tray menu, and need to show the GUI to be useful.
    connect(overviewAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(overviewAction, SIGNAL(triggered()), this, SLOT(gotoOverviewPage()));
    connect(sendCoinsAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(sendCoinsAction, SIGNAL(triggered()), this, SLOT(gotoSendCoinsPage()));
    connect(receiveCoinsAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(receiveCoinsAction, SIGNAL(triggered()), this, SLOT(gotoReceiveCoinsPage()));
    connect(historyAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(historyAction, SIGNAL(triggered()), this, SLOT(gotoHistoryPage()));
    connect(minerDepositsAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(minerDepositsAction, SIGNAL(triggered()), this, SLOT(gotoMinerDepositsPage()));
    connect(claimCoinsAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(claimCoinsAction, SIGNAL(triggered()), this, SLOT(gotoClaimCoinsPage()));
    connect(minerAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(minerAction, SIGNAL(triggered()), this, SLOT(gotoMinerCoinsPage()));

    connect(bitcoin_overviewAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(bitcoin_overviewAction, SIGNAL(triggered()), this, SLOT(gotoOverviewPage()));
    connect(bitcoin_sendCoinsAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(bitcoin_sendCoinsAction, SIGNAL(triggered()), this, SLOT(gotoSendCoinsPage()));
    connect(bitcoin_receiveCoinsAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(bitcoin_receiveCoinsAction, SIGNAL(triggered()), this, SLOT(gotoReceiveCoinsPage()));
    connect(bitcoin_historyAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(bitcoin_historyAction, SIGNAL(triggered()), this, SLOT(gotoHistoryPage()));

    quitAction = new QAction(QIcon(":/icons/quit"), tr("E&xit"), this);
    quitAction->setStatusTip(tr("Quit application"));
    quitAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q));
    quitAction->setMenuRole(QAction::QuitRole);
    if (!fIsTestnet)
        aboutAction = new QAction(QIcon(":/icons/bitcoin"), tr("&About Credits Core"), this);
    else
        aboutAction = new QAction(QIcon(":/icons/bitcredit_testnet"), tr("&About Credits Core"), this);
    aboutAction->setStatusTip(tr("Show information about Bitcredit"));
    aboutAction->setMenuRole(QAction::AboutRole);
#if QT_VERSION < 0x050000
    aboutQtAction = new QAction(QIcon(":/trolltech/qmessagebox/images/qtlogo-64.png"), tr("About &Qt"), this);
#else
    aboutQtAction = new QAction(QIcon(":/qt-project.org/qmessagebox/images/qtlogo-64.png"), tr("About &Qt"), this);
#endif
    aboutQtAction->setStatusTip(tr("Show information about Qt"));
    aboutQtAction->setMenuRole(QAction::AboutQtRole);
    optionsAction = new QAction(QIcon(":/icons/options"), tr("&Options..."), this);
    optionsAction->setStatusTip(tr("Modify configuration options for Bitcredit"));
    optionsAction->setMenuRole(QAction::PreferencesRole);
    if (!fIsTestnet)
        toggleHideAction = new QAction(QIcon(":/icons/bitcoin"), tr("&Show / Hide"), this);
    else
        toggleHideAction = new QAction(QIcon(":/icons/bitcredit_testnet"), tr("&Show / Hide"), this);
    toggleHideAction->setStatusTip(tr("Show or hide the main Window"));

    bitcredit_encryptWalletAction = new QAction(QIcon(":/icons/lock_closed"), tr("&Encrypt Credits Wallet..."), this);
    bitcredit_encryptWalletAction->setStatusTip(tr("Encrypt the private keys that belong to your Credits wallet"));
    bitcredit_encryptWalletAction->setCheckable(true);
    bitcredit_backupWalletAction = new QAction(QIcon(":/icons/filesave"), tr("&Backup Credits Wallet..."), this);
    bitcredit_backupWalletAction->setStatusTip(tr("Backup Credits wallet to another location"));
    bitcredit_changePassphraseAction = new QAction(QIcon(":/icons/key"), tr("&Change Credits Passphrase..."), this);
    bitcredit_changePassphraseAction->setStatusTip(tr("Change the passphrase used for Credits wallet encryption"));

    bitcoin_encryptWalletAction = new QAction(QIcon(":/icons/lock_closed"), tr("&Encrypt Bitcoin Wallet..."), this);
    bitcoin_encryptWalletAction->setStatusTip(tr("Encrypt the private keys that belong to your bitcoin wallet"));
    bitcoin_encryptWalletAction->setCheckable(true);
    bitcoin_backupWalletAction = new QAction(QIcon(":/icons/filesave"), tr("&Backup Bitcoin Wallet..."), this);
    bitcoin_backupWalletAction->setStatusTip(tr("Backup Bitcoin wallet to another location"));
    bitcoin_changePassphraseAction = new QAction(QIcon(":/icons/key"), tr("&Change Bitcoin Passphrase..."), this);
    bitcoin_changePassphraseAction->setStatusTip(tr("Change the passphrase used for Bitcoin wallet encryption"));

    signMessageAction = new QAction(QIcon(":/icons/edit"), tr("Sign &message..."), this);
    signMessageAction->setStatusTip(tr("Sign messages with your Credits addresses to prove you own them"));
    verifyMessageAction = new QAction(QIcon(":/icons/transaction_0"), tr("&Verify message..."), this);
    verifyMessageAction->setStatusTip(tr("Verify messages to ensure they were signed with specified Credits addresses"));

    bitcredit_openRPCConsoleAction = new QAction(QIcon(":/icons/debugwindow"), tr("&Credits debug window"), this);
    bitcredit_openRPCConsoleAction->setStatusTip(tr("Open debugging and diagnostic console"));

    bitcoin_openRPCConsoleAction = new QAction(QIcon(":/icons/debugwindow"), tr("&Bitcoin info window"), this);
    bitcoin_openRPCConsoleAction->setStatusTip(tr("Open bitcoin debugging and diagnostic console"));

    usedSendingAddressesAction = new QAction(QIcon(":/icons/address-book"), tr("&Sending addresses..."), this);
    usedSendingAddressesAction->setStatusTip(tr("Show the list of used sending addresses and labels"));
    usedReceivingAddressesAction = new QAction(QIcon(":/icons/address-book"), tr("&Receiving addresses..."), this);
    usedReceivingAddressesAction->setStatusTip(tr("Show the list of used receiving addresses and labels"));

    openAction = new QAction(QApplication::style()->standardIcon(QStyle::SP_FileIcon), tr("Open &URI..."), this);
    openAction->setStatusTip(tr("Open a bitcredit: URI or payment request"));

    showHelpMessageAction = new QAction(QApplication::style()->standardIcon(QStyle::SP_MessageBoxInformation), tr("&Command-line options"), this);
    showHelpMessageAction->setStatusTip(tr("Show the Credits Core help message to get a list with possible Credits command-line options"));

    connect(quitAction, SIGNAL(triggered()), qApp, SLOT(quit()));
    connect(aboutAction, SIGNAL(triggered()), this, SLOT(aboutClicked()));
    connect(aboutQtAction, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
    connect(optionsAction, SIGNAL(triggered()), this, SLOT(optionsClicked()));
    connect(toggleHideAction, SIGNAL(triggered()), this, SLOT(toggleHidden()));
    connect(showHelpMessageAction, SIGNAL(triggered()), this, SLOT(showHelpMessageClicked()));
#ifdef ENABLE_WALLET
    if(walletFrame)
    {
        connect(bitcredit_encryptWalletAction, SIGNAL(triggered(bool)), walletFrame, SLOT(bitcredit_encryptWallet(bool)));
        connect(bitcredit_backupWalletAction, SIGNAL(triggered()), walletFrame, SLOT(bitcredit_backupWallet()));
        connect(bitcredit_changePassphraseAction, SIGNAL(triggered()), walletFrame, SLOT(bitcredit_changePassphrase()));

        connect(bitcoin_encryptWalletAction, SIGNAL(triggered(bool)), walletFrame, SLOT(bitcoin_encryptWallet(bool)));
        connect(bitcoin_backupWalletAction, SIGNAL(triggered()), walletFrame, SLOT(bitcoin_backupWallet()));
        connect(bitcoin_changePassphraseAction, SIGNAL(triggered()), walletFrame, SLOT(bitcoin_changePassphrase()));

        connect(signMessageAction, SIGNAL(triggered()), this, SLOT(gotoSignMessageTab()));
        connect(verifyMessageAction, SIGNAL(triggered()), this, SLOT(gotoVerifyMessageTab()));
        connect(usedSendingAddressesAction, SIGNAL(triggered()), walletFrame, SLOT(usedSendingAddresses()));
        connect(usedReceivingAddressesAction, SIGNAL(triggered()), walletFrame, SLOT(usedReceivingAddresses()));
        connect(openAction, SIGNAL(triggered()), this, SLOT(openClicked()));
    }
#endif
}

void BitcreditGUI::createMenuBar()
{
#ifdef Q_OS_MAC
    // Create a decoupled menu bar on Mac which stays even if the window is closed
    appMenuBar = new QMenuBar();
#else
    // Get the main window's menu bar on other platforms
    appMenuBar = menuBar();
#endif

    // Configure the menus
    QMenu *file = appMenuBar->addMenu(tr("&File"));
    if(walletFrame)
    {
        file->addAction(openAction);
        file->addAction(bitcredit_backupWalletAction);
        file->addAction(bitcoin_backupWalletAction);
        file->addAction(signMessageAction);
        file->addAction(verifyMessageAction);
        file->addSeparator();
        file->addAction(usedSendingAddressesAction);
        file->addAction(usedReceivingAddressesAction);
        file->addSeparator();
    }
    file->addAction(quitAction);

    QMenu *settings = appMenuBar->addMenu(tr("&Settings"));
    if(walletFrame)
    {
        settings->addAction(bitcredit_encryptWalletAction);
        settings->addAction(bitcredit_changePassphraseAction);

        settings->addSeparator();

        settings->addAction(bitcoin_encryptWalletAction);
        settings->addAction(bitcoin_changePassphraseAction);

        settings->addSeparator();
    }
    settings->addAction(optionsAction);

    QMenu *help = appMenuBar->addMenu(tr("&Help"));
    if(walletFrame)
    {
        help->addAction(bitcredit_openRPCConsoleAction);
        help->addAction(bitcoin_openRPCConsoleAction);
    }
    help->addAction(showHelpMessageAction);
    help->addSeparator();
    help->addAction(aboutAction);
    help->addAction(aboutQtAction);
}

void BitcreditGUI::createToolBars()
{
    if(walletFrame)
    {
        QToolBar *toolbar = addToolBar(tr("Tabs toolbar"));
        toolbar->layout()->setContentsMargins(5, 5, 5, 5);
        addToolBar(Qt::LeftToolBarArea, toolbar);
        toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        toolbar->setFloatable(false);
		toolbar->setMovable(false);
        toolbar->setStyle(QStyleFactory::create("windows"));
        toolbar->addAction(overviewAction);
        toolbar->widgetForAction(overviewAction)->setFixedWidth(140);
        toolbar->addAction(sendCoinsAction);
        toolbar->widgetForAction(sendCoinsAction)->setFixedWidth(140);
        toolbar->addAction(receiveCoinsAction);
        toolbar->widgetForAction(receiveCoinsAction)->setFixedWidth(140);
        toolbar->addAction(historyAction);
        toolbar->widgetForAction(historyAction)->setFixedWidth(140);
        toolbar->addAction(claimCoinsAction);
        toolbar->widgetForAction(claimCoinsAction)->setFixedWidth(140);
        toolbar->addAction(minerAction);
        toolbar->widgetForAction(minerAction)->setFixedWidth(140);
        toolbar->addAction(minerDepositsAction);
        toolbar->widgetForAction(minerDepositsAction)->setFixedWidth(140);

        QAction* separator = toolbar->addSeparator();
        toolbar->widgetForAction(separator)->setFixedWidth(140);
        QAction* separator2 = toolbar->addSeparator();
        toolbar->widgetForAction(separator2)->setFixedWidth(140);

        toolbar->addAction(bitcoin_overviewAction);
        toolbar->widgetForAction(bitcoin_overviewAction)->setFixedWidth(140);
        toolbar->addAction(bitcoin_sendCoinsAction);
        toolbar->widgetForAction(bitcoin_sendCoinsAction)->setFixedWidth(140);
        toolbar->addAction(bitcoin_receiveCoinsAction);
        toolbar->widgetForAction(bitcoin_receiveCoinsAction)->setFixedWidth(140);
        toolbar->addAction(bitcoin_historyAction);
        toolbar->widgetForAction(bitcoin_historyAction)->setFixedWidth(140);
        QLayout *layt2 = toolbar->layout();
        for (int i = 0; i < layt2->count(); ++i) {
        	layt2->itemAt(i)->setAlignment(Qt::AlignLeft);
        }

        overviewAction->setChecked(true);
    }
}

void BitcreditGUI::bitcredit_setClientModel(ClientModel *clientModel)
{
    this->credits_clientModel = clientModel;
    if(clientModel)
    {
        // Create system tray menu (or setup the dock menu) that late to prevent users from calling actions,
        // while the client has not yet fully loaded
        createTrayIconMenu();

        // Keep up to date with client
        bitcredit_setNumConnections(clientModel->getNumConnections());
        connect(clientModel, SIGNAL(numConnectionsChanged(int)), this, SLOT(bitcredit_setNumConnections(int)));

        bitcredit_setNumBlocks(clientModel->getNumBlocks());
        connect(clientModel, SIGNAL(numBlocksChanged(int)), this, SLOT(bitcredit_setNumBlocks(int)));

        // Receive and report messages from client model
        connect(clientModel, SIGNAL(message(QString,QString,unsigned int)), this, SLOT(message(QString,QString,unsigned int)));

        // Show progress dialog
        connect(clientModel, SIGNAL(showProgress(QString,int)), this, SLOT(showProgress(QString,int)));

        bitcredit_rpcConsole->setClientModel(clientModel);
#ifdef ENABLE_WALLET
        if(walletFrame)
        {
            walletFrame->setClientModel(clientModel);
        }
#endif
    }
}
void BitcreditGUI::bitcoin_setClientModel(ClientModel *clientModel)
{
    this->bitcoin_clientModel = clientModel;
    if(clientModel)
    {
        // Create system tray menu (or setup the dock menu) that late to prevent users from calling actions,
        // while the client has not yet fully loaded
        //createTrayIconMenu();

        // Keep up to date with client
        bitcoin_setNumConnections(clientModel->getNumConnections());
        connect(clientModel, SIGNAL(numConnectionsChanged(int)), this, SLOT(bitcoin_setNumConnections(int)));

        bitcoin_setNumBlocks(clientModel->getNumBlocks());
        connect(clientModel, SIGNAL(numBlocksChanged(int)), this, SLOT(bitcoin_setNumBlocks(int)));

        // Receive and report messages from client model
        connect(clientModel, SIGNAL(message(QString,QString,unsigned int)), this, SLOT(message(QString,QString,unsigned int)));

        bitcoin_rpcConsole->setClientModel(clientModel);
    }
}

#ifdef ENABLE_WALLET
bool BitcreditGUI::addWallet(const QString& name, Bitcredit_WalletModel *bitcredit_model, Bitcoin_WalletModel *bitcoin_model, Bitcredit_WalletModel *deposit_model)
{
    if(!walletFrame)
        return false;
    setWalletActionsEnabled(true);
    return walletFrame->addWallet(name, bitcredit_model, bitcoin_model, deposit_model);
}

bool BitcreditGUI::setCurrentWallet(const QString& name)
{
    if(!walletFrame)
        return false;
    return walletFrame->setCurrentWallet(name);
}

void BitcreditGUI::removeAllWallets()
{
    if(!walletFrame)
        return;
    setWalletActionsEnabled(false);
    walletFrame->removeAllWallets();
}
#endif

void BitcreditGUI::setWalletActionsEnabled(bool enabled)
{
    overviewAction->setEnabled(enabled);
    sendCoinsAction->setEnabled(enabled);
    receiveCoinsAction->setEnabled(enabled);
    claimCoinsAction->setEnabled(enabled);
    historyAction->setEnabled(enabled);
    minerDepositsAction->setEnabled(enabled);
    minerAction->setEnabled(enabled);

    bitcoin_overviewAction->setEnabled(enabled);
    bitcoin_sendCoinsAction->setEnabled(enabled);
    bitcoin_receiveCoinsAction->setEnabled(enabled);
    bitcoin_historyAction->setEnabled(enabled);

    bitcredit_encryptWalletAction->setEnabled(enabled);
    bitcredit_backupWalletAction->setEnabled(enabled);
    bitcredit_changePassphraseAction->setEnabled(enabled);

    bitcoin_encryptWalletAction->setEnabled(enabled);
    bitcoin_backupWalletAction->setEnabled(enabled);
    bitcoin_changePassphraseAction->setEnabled(enabled);

    signMessageAction->setEnabled(enabled);
    verifyMessageAction->setEnabled(enabled);
    usedSendingAddressesAction->setEnabled(enabled);
    usedReceivingAddressesAction->setEnabled(enabled);
    openAction->setEnabled(enabled);
}

void BitcreditGUI::createTrayIcon(bool fIsTestnet)
{
#ifndef Q_OS_MAC
    trayIcon = new QSystemTrayIcon(this);

    if (!fIsTestnet)
    {
        trayIcon->setToolTip(tr("Credits client"));
        trayIcon->setIcon(QIcon(":/icons/toolbar"));
    }
    else
    {
        trayIcon->setToolTip(tr("Credits client") + " " + tr("[testnet]"));
        trayIcon->setIcon(QIcon(":/icons/toolbar_testnet"));
    }

    trayIcon->show();
#endif

    notificator = new Notificator(QApplication::applicationName(), trayIcon, this);
}

void BitcreditGUI::createTrayIconMenu()
{
    QMenu *trayIconMenu;
#ifndef Q_OS_MAC
    // return if trayIcon is unset (only on non-Mac OSes)
    if (!trayIcon)
        return;

    trayIconMenu = new QMenu(this);
    trayIcon->setContextMenu(trayIconMenu);

    connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
            this, SLOT(trayIconActivated(QSystemTrayIcon::ActivationReason)));
#else
    // Note: On Mac, the dock icon is used to provide the tray's functionality.
    MacDockIconHandler *dockIconHandler = MacDockIconHandler::instance();
    dockIconHandler->setMainWindow((QMainWindow *)this);
    trayIconMenu = dockIconHandler->dockMenu();
#endif

    // Configuration of the tray icon (or dock icon) icon menu
    trayIconMenu->addAction(toggleHideAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(sendCoinsAction);
    trayIconMenu->addAction(receiveCoinsAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(signMessageAction);
    trayIconMenu->addAction(verifyMessageAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(optionsAction);
    trayIconMenu->addAction(bitcredit_openRPCConsoleAction);
    trayIconMenu->addAction(bitcoin_openRPCConsoleAction);

    trayIconMenu->addAction(bitcoin_sendCoinsAction);
    trayIconMenu->addAction(bitcoin_receiveCoinsAction);
#ifndef Q_OS_MAC // This is built-in on Mac
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(quitAction);
#endif
}

#ifndef Q_OS_MAC
void BitcreditGUI::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if(reason == QSystemTrayIcon::Trigger)
    {
        // Click on system tray icon triggers show/hide of the main window
        toggleHideAction->trigger();
    }
}
#endif

void BitcreditGUI::optionsClicked()
{
    if(!credits_clientModel || !credits_clientModel->getOptionsModel())
        return;

    OptionsDialog dlg(this);
    dlg.setModel(credits_clientModel->getOptionsModel());
    dlg.exec();
}

void BitcreditGUI::aboutClicked()
{
    if(!credits_clientModel)
        return;

    AboutDialog dlg(this);
    dlg.setModel(credits_clientModel);
    dlg.exec();
}

void BitcreditGUI::showHelpMessageClicked()
{
    HelpMessageDialog *help = new HelpMessageDialog(this);
    help->setAttribute(Qt::WA_DeleteOnClose);
    help->show();
}

#ifdef ENABLE_WALLET
void BitcreditGUI::openClicked()
{
    OpenURIDialog dlg(this);
    if(dlg.exec())
    {
        emit receivedURI(dlg.getURI());
    }
}

void BitcreditGUI::gotoOverviewPage()
{
    overviewAction->setChecked(true);
    if (walletFrame) walletFrame->gotoOverviewPage();
}

void BitcreditGUI::gotoClaimCoinsPage()
{
	claimCoinsAction->setChecked(true);
    if (walletFrame) walletFrame->gotoClaimCoinsPage();
}

void BitcreditGUI::gotoHistoryPage()
{
    historyAction->setChecked(true);
    if (walletFrame) walletFrame->gotoHistoryPage();
}
void BitcreditGUI::gotoMinerDepositsPage()
{
	minerDepositsAction->setChecked(true);
    if (walletFrame) walletFrame->gotoMinerDepositsPage();
}

void BitcreditGUI::gotoReceiveCoinsPage()
{
    receiveCoinsAction->setChecked(true);
    if (walletFrame) walletFrame->gotoReceiveCoinsPage();
}

void BitcreditGUI::gotoSendCoinsPage(QString addr)
{
    sendCoinsAction->setChecked(true);
    if (walletFrame) walletFrame->gotoSendCoinsPage(addr);
}

void BitcreditGUI::gotoMinerCoinsPage(QString addr)
{
    minerAction->setChecked(true);
    if (walletFrame) walletFrame->gotoMinerCoinsPage(addr);
}

void BitcreditGUI::gotoSignMessageTab(QString addr)
{
    if (walletFrame) walletFrame->gotoSignMessageTab(addr);
}

void BitcreditGUI::gotoVerifyMessageTab(QString addr)
{
    if (walletFrame) walletFrame->gotoVerifyMessageTab(addr);
}
#endif

void BitcreditGUI::bitcredit_setNumConnections(int count)
{
    QString icon;
    switch(count)
    {
    case 0: icon = ":/icons/connect_0"; break;
    case 1: case 2: case 3: icon = ":/icons/connect_1"; break;
    case 4: case 5: case 6: icon = ":/icons/connect_2"; break;
    case 7: case 8: case 9: icon = ":/icons/connect_3"; break;
    default: icon = ":/icons/connect_4"; break;
    }
    bitcredit_labelConnectionsIcon->setPixmap(QIcon(icon).pixmap(STATUSBAR_ICONSIZE,STATUSBAR_ICONSIZE));
    bitcredit_labelConnectionsIcon->setToolTip(tr("Credits: %n active connection(s) to network", "", count));
}
void BitcreditGUI::bitcoin_setNumConnections(int count)
{
    QString icon;
    switch(count)
    {
    case 0: icon = ":/icons/connect_0"; break;
    case 1: case 2: case 3: icon = ":/icons/connect_1"; break;
    case 4: case 5: case 6: icon = ":/icons/connect_2"; break;
    case 7: case 8: case 9: icon = ":/icons/connect_3"; break;
    default: icon = ":/icons/connect_4"; break;
    }
    bitcoin_labelConnectionsIcon->setPixmap(QIcon(icon).pixmap(STATUSBAR_ICONSIZE,STATUSBAR_ICONSIZE));
    bitcoin_labelConnectionsIcon->setToolTip(tr("Bitcoin: %n active connection(s) to network", "", count));
}

void BitcreditGUI::bitcredit_setNumBlocks(int count)
{
    // Prevent orphan statusbar messages (e.g. hover Quit in main menu, wait until chain-sync starts -> garbelled text)
    statusBar()->clearMessage();

    // Acquire current block source
    enum BlockSource blockSource = credits_clientModel->getBlockSource();
    switch (blockSource) {
        case BLOCK_SOURCE_NETWORK:
            bitcredit_progressBarLabel->setText(tr("Credits: Synchronizing with network..."));
            break;
        case BLOCK_SOURCE_DISK:
            bitcredit_progressBarLabel->setText(tr("Credits: Importing blocks from disk..."));
            break;
        case BLOCK_SOURCE_REINDEX:
            bitcredit_progressBarLabel->setText(tr("Credits: Reindexing blocks on disk..."));
            break;
        case BLOCK_SOURCE_NONE:
            // Case: not Importing, not Reindexing and no network connection
            bitcredit_progressBarLabel->setText(tr("Credits: No block source available..."));
            break;
    }

    QString tooltip;

    QDateTime lastBlockDate = credits_clientModel->getLastBlockDate();
    QDateTime currentDate = QDateTime::currentDateTime();
    int secs = lastBlockDate.secsTo(currentDate);

    tooltip = tr("Processed %1 credits blocks of transaction history.").arg(count);

    // Set icon state: spinning if catching up, tick otherwise
    if(secs < 90*60)
    {
        tooltip = tr("Credits: Up to date") + QString(".<br>") + tooltip;
        bitcredit_labelBlocksIcon->setPixmap(QIcon(":/icons/synced").pixmap(STATUSBAR_ICONSIZE, STATUSBAR_ICONSIZE));

#ifdef ENABLE_WALLET
        if(walletFrame)
            walletFrame->showOutOfSyncWarning(false);
#endif

        bitcredit_progressBarLabel->setVisible(false);
        bitcredit_progressBar->setVisible(false);
    }
    else
    {
        // Represent time from last generated block in human readable text
        QString timeBehindText;
        const int HOUR_IN_SECONDS = 60*60;
        const int DAY_IN_SECONDS = 24*60*60;
        const int WEEK_IN_SECONDS = 7*24*60*60;
        const int YEAR_IN_SECONDS = 31556952; // Average length of year in Gregorian calendar
        if(secs < 2*DAY_IN_SECONDS)
        {
            timeBehindText = tr("%n hour(s)","",secs/HOUR_IN_SECONDS);
        }
        else if(secs < 2*WEEK_IN_SECONDS)
        {
            timeBehindText = tr("%n day(s)","",secs/DAY_IN_SECONDS);
        }
        else if(secs < YEAR_IN_SECONDS)
        {
            timeBehindText = tr("%n week(s)","",secs/WEEK_IN_SECONDS);
        }
        else
        {
            int years = secs / YEAR_IN_SECONDS;
            int remainder = secs % YEAR_IN_SECONDS;
            timeBehindText = tr("%1 and %2").arg(tr("%n year(s)", "", years)).arg(tr("%n week(s)","", remainder/WEEK_IN_SECONDS));
        }

        bitcredit_progressBarLabel->setVisible(true);
        bitcredit_progressBar->setFormat(tr("%1 behind").arg(timeBehindText));
        bitcredit_progressBar->setMaximum(1000000000);
        bitcredit_progressBar->setValue(credits_clientModel->getVerificationProgress() * 1000000000.0 + 0.5);
        bitcredit_progressBar->setVisible(true);

        tooltip = tr("Credits: Catching up...") + QString("<br>") + tooltip;
        if(count != bitcredit_prevBlocks)
        {
            bitcredit_labelBlocksIcon->setPixmap(QIcon(QString(
                ":/movies/spinner-%1").arg(bitcredit_spinnerFrame, 3, 10, QChar('0')))
                .pixmap(STATUSBAR_ICONSIZE, STATUSBAR_ICONSIZE));
            bitcredit_spinnerFrame = (bitcredit_spinnerFrame + 1) % SPINNER_FRAMES;
        }
        bitcredit_prevBlocks = count;

#ifdef ENABLE_WALLET
        if(walletFrame)
            walletFrame->showOutOfSyncWarning(true);
#endif

        tooltip += QString("<br>");
        tooltip += tr("Last received credits block was generated %1 ago.").arg(timeBehindText);
        tooltip += QString("<br>");
        tooltip += tr("Credits transactions after this will not yet be visible.");
    }

    // Don't word-wrap this (fixed-width) tooltip
    tooltip = QString("<nobr>") + tooltip + QString("</nobr>");

    bitcredit_labelBlocksIcon->setToolTip(tooltip);
    bitcredit_progressBarLabel->setToolTip(tooltip);
    bitcredit_progressBar->setToolTip(tooltip);

    walletFrame->bitcredit_setNumBlocks(count);
}

void BitcreditGUI::bitcoin_setNumBlocks(int count)
{
    // Prevent orphan statusbar messages (e.g. hover Quit in main menu, wait until chain-sync starts -> garbelled text)
    statusBar()->clearMessage();

    // Acquire current block source
    enum BlockSource blockSource = bitcoin_clientModel->getBlockSource();
    switch (blockSource) {
        case BLOCK_SOURCE_NETWORK:
            bitcoin_progressBarLabel->setText(tr("Bitcoin: Synchronizing with network..."));
            break;
        case BLOCK_SOURCE_DISK:
        	bitcoin_progressBarLabel->setText(tr("Bitcoin: Importing blocks from disk..."));
            break;
        case BLOCK_SOURCE_REINDEX:
        	bitcoin_progressBarLabel->setText(tr("Bitcoin: Reindexing blocks on disk..."));
            break;
        case BLOCK_SOURCE_NONE:
            // Case: not Importing, not Reindexing and no network connection
        	bitcoin_progressBarLabel->setText(tr("Bitcoin: No block source available..."));
            break;
    }

    QString tooltip;

    QDateTime lastBlockDate = bitcoin_clientModel->getLastBlockDate();
    QDateTime currentDate = QDateTime::currentDateTime();
    int secs = lastBlockDate.secsTo(currentDate);

    tooltip = tr("Processed %1 bitcoin blocks of transaction history.").arg(count);

    // Set icon state: spinning if catching up, tick otherwise
    if(secs < 90*60)
    {
        tooltip = tr("Bitcoin: Up to date") + QString(".<br>") + tooltip;
        bitcoin_labelBlocksIcon->setPixmap(QIcon(":/icons/synced").pixmap(STATUSBAR_ICONSIZE, STATUSBAR_ICONSIZE));

        bitcoin_progressBarLabel->setVisible(false);
        bitcoin_progressBar->setVisible(false);
    }
    else
    {
        // Represent time from last generated block in human readable text
        QString timeBehindText;
        const int HOUR_IN_SECONDS = 60*60;
        const int DAY_IN_SECONDS = 24*60*60;
        const int WEEK_IN_SECONDS = 7*24*60*60;
        const int YEAR_IN_SECONDS = 31556952; // Average length of year in Gregorian calendar
        if(secs < 2*DAY_IN_SECONDS)
        {
            timeBehindText = tr("%n hour(s)","",secs/HOUR_IN_SECONDS);
        }
        else if(secs < 2*WEEK_IN_SECONDS)
        {
            timeBehindText = tr("%n day(s)","",secs/DAY_IN_SECONDS);
        }
        else if(secs < YEAR_IN_SECONDS)
        {
            timeBehindText = tr("%n week(s)","",secs/WEEK_IN_SECONDS);
        }
        else
        {
            int years = secs / YEAR_IN_SECONDS;
            int remainder = secs % YEAR_IN_SECONDS;
            timeBehindText = tr("%1 and %2").arg(tr("%n year(s)", "", years)).arg(tr("%n week(s)","", remainder/WEEK_IN_SECONDS));
        }

        bitcoin_progressBarLabel->setVisible(true);
        bitcoin_progressBar->setFormat(tr("%1 behind").arg(timeBehindText));
        bitcoin_progressBar->setMaximum(1000000000);
        bitcoin_progressBar->setValue(bitcoin_clientModel->getVerificationProgress() * 1000000000.0 + 0.5);
        bitcoin_progressBar->setVisible(true);

        tooltip = tr("Bitcoin: Catching up...") + QString("<br>") + tooltip;
        if(count != bitcoin_prevBlocks)
        {
            bitcoin_labelBlocksIcon->setPixmap(QIcon(QString(
                ":/movies/spinner-%1").arg(bitcoin_spinnerFrame, 3, 10, QChar('0')))
                .pixmap(STATUSBAR_ICONSIZE, STATUSBAR_ICONSIZE));
            bitcoin_spinnerFrame = (bitcoin_spinnerFrame + 1) % SPINNER_FRAMES;
        }
        bitcoin_prevBlocks = count;

        tooltip += QString("<br>");
        tooltip += tr("Last received bitcoin block was generated %1 ago.").arg(timeBehindText);
        tooltip += QString("<br>");
        tooltip += tr("Bitcoin transactions after this will not yet be visible.");
    }

    // Don't word-wrap this (fixed-width) tooltip
    tooltip = QString("<nobr>") + tooltip + QString("</nobr>");

    bitcoin_labelBlocksIcon->setToolTip(tooltip);
    bitcoin_progressBarLabel->setToolTip(tooltip);
    bitcoin_progressBar->setToolTip(tooltip);
}

void BitcreditGUI::message(const QString &title, const QString &message, unsigned int style, bool *ret)
{
    QString strTitle = tr("Bitcredit"); // default title
    // Default to information icon
    int nMBoxIcon = QMessageBox::Information;
    int nNotifyIcon = Notificator::Information;

    QString msgType;

    // Prefer supplied title over style based title
    if (!title.isEmpty()) {
        msgType = title;
    }
    else {
        switch (style) {
        case CClientUIInterface::MSG_ERROR:
            msgType = tr("Error");
            break;
        case CClientUIInterface::MSG_WARNING:
            msgType = tr("Warning");
            break;
        case CClientUIInterface::MSG_INFORMATION:
            msgType = tr("Information");
            break;
        default:
            break;
        }
    }
    // Append title to "Credits - "
    if (!msgType.isEmpty())
        strTitle += " - " + msgType;

    // Check for error/warning icon
    if (style & CClientUIInterface::ICON_ERROR) {
        nMBoxIcon = QMessageBox::Critical;
        nNotifyIcon = Notificator::Critical;
    }
    else if (style & CClientUIInterface::ICON_WARNING) {
        nMBoxIcon = QMessageBox::Warning;
        nNotifyIcon = Notificator::Warning;
    }

    // Display message
    if (style & CClientUIInterface::MODAL) {
        // Check for buttons, use OK as default, if none was supplied
        QMessageBox::StandardButton buttons;
        if (!(buttons = (QMessageBox::StandardButton)(style & CClientUIInterface::BTN_MASK)))
            buttons = QMessageBox::Ok;

        // Ensure we get users attention, but only if main window is visible
        // as we don't want to pop up the main window for messages that happen before
        // initialization is finished.
        if(!(style & CClientUIInterface::NOSHOWGUI))
            showNormalIfMinimized();
        QMessageBox mBox((QMessageBox::Icon)nMBoxIcon, strTitle, message, buttons, this);
        int r = mBox.exec();
        if (ret != NULL)
            *ret = r == QMessageBox::Ok;
    }
    else
        notificator->notify((Notificator::Class)nNotifyIcon, strTitle, message);
}

void BitcreditGUI::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
#ifndef Q_OS_MAC // Ignored on Mac
    if(e->type() == QEvent::WindowStateChange)
    {
        if(credits_clientModel && credits_clientModel->getOptionsModel()->getMinimizeToTray())
        {
            QWindowStateChangeEvent *wsevt = static_cast<QWindowStateChangeEvent*>(e);
            if(!(wsevt->oldState() & Qt::WindowMinimized) && isMinimized())
            {
                QTimer::singleShot(0, this, SLOT(hide()));
                e->ignore();
            }
        }
    }
#endif
}

void BitcreditGUI::closeEvent(QCloseEvent *event)
{
    if(credits_clientModel)
    {
#ifndef Q_OS_MAC // Ignored on Mac
        if(!credits_clientModel->getOptionsModel()->getMinimizeToTray() &&
           !credits_clientModel->getOptionsModel()->getMinimizeOnClose())
        {
            QApplication::quit();
        }
#endif
    }
    QMainWindow::closeEvent(event);
}

#ifdef ENABLE_WALLET
void BitcreditGUI::incomingTransaction(const QString& date, int unit, qint64 amount, const QString& type, const QString& address)
{
    // On new transaction, make an info balloon
    message((amount)<0 ? tr("Sent transaction") : tr("Incoming transaction"),
             tr("Date: %1\n"
                "Amount: %2\n"
                "Type: %3\n"
                "Address: %4\n")
                  .arg(date)
                  .arg(BitcreditUnits::formatWithUnit(unit, amount, true))
                  .arg(type)
                  .arg(address), CClientUIInterface::MSG_INFORMATION);
}
#endif

void BitcreditGUI::dragEnterEvent(QDragEnterEvent *event)
{
    // Accept only URIs
    if(event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void BitcreditGUI::dropEvent(QDropEvent *event)
{
    if(event->mimeData()->hasUrls())
    {
        foreach(const QUrl &uri, event->mimeData()->urls())
        {
            emit receivedURI(uri.toString());
        }
    }
    event->acceptProposedAction();
}

bool BitcreditGUI::eventFilter(QObject *object, QEvent *event)
{
    // Catch status tip events
    if (event->type() == QEvent::StatusTip)
    {
        // Prevent adding text from setStatusTip(), if we currently use the status bar for displaying other stuff
        if (bitcredit_progressBarLabel->isVisible() || bitcredit_progressBar->isVisible())
            return true;
    }
    return QMainWindow::eventFilter(object, event);
}

#ifdef ENABLE_WALLET
bool BitcreditGUI::handlePaymentRequest(const Bitcredit_SendCoinsRecipient& recipient)
{
    // URI has to be valid
    if (walletFrame && walletFrame->handlePaymentRequest(recipient))
    {
        showNormalIfMinimized();
        gotoSendCoinsPage();
        return true;
    }
    else
        return false;
}

void BitcreditGUI::bitcredit_setEncryptionStatus(int status)
{
    switch(status)
    {
    case Bitcredit_WalletModel::Unencrypted:
        bitcredit_labelEncryptionIcon->hide();
        bitcredit_encryptWalletAction->setChecked(false);
        bitcredit_changePassphraseAction->setEnabled(false);
        bitcredit_encryptWalletAction->setEnabled(true);
        break;
    case Bitcredit_WalletModel::Unlocked:
        bitcredit_labelEncryptionIcon->show();
        bitcredit_labelEncryptionIcon->setPixmap(QIcon(":/icons/lock_open").pixmap(STATUSBAR_ICONSIZE,STATUSBAR_ICONSIZE));
        bitcredit_labelEncryptionIcon->setToolTip(tr("Credits wallet is <b>encrypted</b> and currently <b>unlocked</b>"));
        bitcredit_encryptWalletAction->setChecked(true);
        bitcredit_changePassphraseAction->setEnabled(true);
        bitcredit_encryptWalletAction->setEnabled(false); // TODO: decrypt currently not supported
        break;
    case Bitcredit_WalletModel::Locked:
        bitcredit_labelEncryptionIcon->show();
        bitcredit_labelEncryptionIcon->setPixmap(QIcon(":/icons/lock_closed").pixmap(STATUSBAR_ICONSIZE,STATUSBAR_ICONSIZE));
        bitcredit_labelEncryptionIcon->setToolTip(tr("Credits wallet is <b>encrypted</b> and currently <b>locked</b>"));
        bitcredit_encryptWalletAction->setChecked(true);
        bitcredit_changePassphraseAction->setEnabled(true);
        bitcredit_encryptWalletAction->setEnabled(false); // TODO: decrypt currently not supported
        break;
    }
}

void BitcreditGUI::bitcoin_setEncryptionStatus(int status)
{
    switch(status)
    {
    case Bitcoin_WalletModel::Unencrypted:
        bitcoin_labelEncryptionIcon->hide();
        bitcoin_encryptWalletAction->setChecked(false);
        bitcoin_changePassphraseAction->setEnabled(false);
        bitcoin_encryptWalletAction->setEnabled(true);
        break;
    case Bitcoin_WalletModel::Unlocked:
        bitcoin_labelEncryptionIcon->show();
        bitcoin_labelEncryptionIcon->setPixmap(QIcon(":/icons/lock_open").pixmap(STATUSBAR_ICONSIZE,STATUSBAR_ICONSIZE));
        bitcoin_labelEncryptionIcon->setToolTip(tr("Bitcoin wallet is <b>encrypted</b> and currently <b>unlocked</b>"));
        bitcoin_encryptWalletAction->setChecked(true);
        bitcoin_changePassphraseAction->setEnabled(true);
        bitcoin_encryptWalletAction->setEnabled(false); // TODO: decrypt currently not supported
        break;
    case Bitcoin_WalletModel::Locked:
        bitcoin_labelEncryptionIcon->show();
        bitcoin_labelEncryptionIcon->setPixmap(QIcon(":/icons/lock_closed").pixmap(STATUSBAR_ICONSIZE,STATUSBAR_ICONSIZE));
        bitcoin_labelEncryptionIcon->setToolTip(tr("Bitcoin wallet is <b>encrypted</b> and currently <b>locked</b>"));
        bitcoin_encryptWalletAction->setChecked(true);
        bitcoin_changePassphraseAction->setEnabled(true);
        bitcoin_encryptWalletAction->setEnabled(false); // TODO: decrypt currently not supported
        break;
    }
}
#endif

void BitcreditGUI::showNormalIfMinimized(bool fToggleHidden)
{
    // activateWindow() (sometimes) helps with keyboard focus on Windows
    if (isHidden())
    {
        show();
        activateWindow();
    }
    else if (isMinimized())
    {
        showNormal();
        activateWindow();
    }
    else if (GUIUtil::isObscured(this))
    {
        raise();
        activateWindow();
    }
    else if(fToggleHidden)
        hide();
}

void BitcreditGUI::toggleHidden()
{
    showNormalIfMinimized(true);
}

void BitcreditGUI::detectShutdown()
{
    if (ShutdownRequested())
    {
        if(bitcredit_rpcConsole)
            bitcredit_rpcConsole->hide();
        if(bitcoin_rpcConsole)
            bitcoin_rpcConsole->hide();
        qApp->quit();
    }
}

void BitcreditGUI::showProgress(const QString &title, int nProgress)
{
    if (nProgress == 0)
    {
        progressDialog = new QProgressDialog(title, "", 0, 100);
        progressDialog->setWindowModality(Qt::ApplicationModal);
        progressDialog->setMinimumDuration(0);
        progressDialog->setCancelButton(0);
        progressDialog->setAutoClose(false);
        progressDialog->setValue(0);
    }
    else if (nProgress == 100)
    {
        if (progressDialog)
        {
            progressDialog->close();
            progressDialog->deleteLater();
        }
    }
    else if (progressDialog)
        progressDialog->setValue(nProgress);
}

static bool ThreadSafeMessageBox(BitcreditGUI *gui, const std::string& message, const std::string& caption, unsigned int style)
{
    bool modal = (style & CClientUIInterface::MODAL);
    bool ret = false;
    // In case of modal message, use blocking connection to wait for user to click a button
    QMetaObject::invokeMethod(gui, "message",
                               modal ? GUIUtil::blockingGUIThreadConnection() : Qt::QueuedConnection,
                               Q_ARG(QString, QString::fromStdString(caption)),
                               Q_ARG(QString, QString::fromStdString(message)),
                               Q_ARG(unsigned int, style),
                               Q_ARG(bool*, &ret));
    return ret;
}

void BitcreditGUI::subscribeToCoreSignals()
{
    // Connect signals to client
    uiInterface.ThreadSafeMessageBox.connect(boost::bind(ThreadSafeMessageBox, this, _1, _2, _3));
}

void BitcreditGUI::unsubscribeFromCoreSignals()
{
    // Disconnect signals from client
    uiInterface.ThreadSafeMessageBox.disconnect(boost::bind(ThreadSafeMessageBox, this, _1, _2, _3));
}
