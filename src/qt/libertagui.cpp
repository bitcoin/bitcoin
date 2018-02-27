// Copyright (c) 2011-2015 The LibertaCore developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "libertagui.h"

#include "libertaunits.h"
#include "clientmodel.h"
#include "guiconstants.h"
#include "guiutil.h"
#include "miner.h"
#include "networkstyle.h"
#include "notificator.h"
#include "openuridialog.h"
#include "optionsdialog.h"
#include "optionsmodel.h"
#include "platformstyle.h"
#include "rpcconsole.h"
#include "utilitydialog.h"

#include "net.h"
#ifdef ENABLE_WALLET
#include "blockexplorer.h"
#include "tradingdialog.h"
#include "walletframe.h"
#include "walletmodel.h"
#endif // ENABLE_WALLET

#ifdef Q_OS_MAC
#include "macdockiconhandler.h"
#endif

#include "init.h"
#include "masternodelist.h"
#include "ui_interface.h"
#include "util.h"

#include <iostream>

#include <QAction>
#include <QApplication>
#include <QDateTime>
#include <QDesktopWidget>
#include <QDragEnterEvent>
#include <QListWidget>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QProgressBar>
#include <QProgressDialog>
#include <QSettings>
#include <QShortcut>
#include <QStackedWidget>
#include <QStatusBar>
#include <QStyle>
#include <QTimer>
#include <QToolBar>
#include <QVBoxLayout>
#include <QPushButton>
#include <QSpinBox>
#if QT_VERSION < 0x050000
#include <QTextDocument>
#include <QUrl>
#else
#include <QUrlQuery>
#endif

const std::string LibertaGUI::DEFAULT_UIPLATFORM =
#if defined(Q_OS_MAC)
        "macosx"
#elif defined(Q_OS_WIN)
        "windows"
#else
        "other"
#endif
        ;

const QString LibertaGUI::DEFAULT_WALLET = "~Default";

LibertaGUI::LibertaGUI(const PlatformStyle *platformStyle, const NetworkStyle *networkStyle, QWidget *parent) :
    QMainWindow(parent),
    clientModel(0),
    walletFrame(0),
    unitDisplayControl(0),
    labelStakingIcon(0),
    labelEncryptionIcon(0),
    labelConnectionsIcon(0),
    labelUsdIcon(0),
    labelBtcIcon(0),
    labelBlocksIcon(0),
    progressBarLabel(0),
    progressBar(0),
    progressDialog(0),
    appMenuBar(0),
    overviewAction(0),
    historyAction(0),
    masternodeAction(0),
    quitAction(0),
    sendCoinsAction(0),
    sendCoinsMenuAction(0),
    usedSendingAddressesAction(0),
    usedReceivingAddressesAction(0),
    signMessageAction(0),
    verifyMessageAction(0),
    aboutAction(0),
    receiveCoinsAction(0),
    receiveCoinsMenuAction(0),
    optionsAction(0),
    toggleHideAction(0),
    encryptWalletAction(0),
    backupWalletAction(0),
    changePassphraseAction(0),
    aboutQtAction(0),
    openRPCConsoleAction(0),
    openAction(0),
    browserAction(0),
    multiSendAction(0),
    pbxclientAction(0), 
    showHelpMessageAction(0),
    trayIcon(0),
    trayIconMenu(0),
    notificator(0),
    rpcConsole(0),
    helpMessageDialog(0),
    prevBlocks(0),
    spinnerFrame(0),
    platformStyle(platformStyle)
{
    /* Open CSS when configured */
    //    this->setStyleSheet(GUIUtil::loadStyleSheet());

    //qApp->setStyleSheet("QMenu{color: rgba(36, 224, 130, 0.5); } QMenu::item {background-color: transparent;}QMenu::item:selected {background-color: rgb(163, 3, 47);} QDialog{background: qlineargradient(x1:0, y1:0, x2:0, y2:1,stop: 0 rgba(29, 27, 27, 0.2), stop: 0.1 rgba(29, 27, 27, 0.18), stop: 0.5 rgba(29, 27, 27, 0.17), stop: 0.9 rgba(29, 27, 27, 0.16), stop: 1 rgba(29, 27, 27, 0.15)); }");

    //this->setStyleSheet("background: qlineargradient(x1:0, y1:0, x2:0, y2:1,stop: 0 rgba(255, 255, 255, 1.0), stop: 0.1 rgba(255, 255, 255, 1.0), stop: 0.5 rgba(255, 255, 255, 1.0), stop: 0.9 rgba(255, 255, 255, 1.0), stop: 1 rgba(255, 255, 255, 1.0));");
    this->setStyleSheet("background: white;");


    GUIUtil::restoreWindowGeometry("nWindow", QSize(850, 550), this);
    setWindowOpacity(qreal(100)/100);

    QString windowTitle = tr("Liberta") + " - ";
#ifdef ENABLE_WALLET
    /* if compiled with wallet support, -disablewallet can still disable the wallet */
    enableWallet = !GetBoolArg("-disablewallet", false);
#else
    enableWallet = false;
#endif // ENABLE_WALLET
    if(enableWallet)
    {
        windowTitle += tr("Wallet");
    } else {
        windowTitle += tr("Node");
    }
    windowTitle += " " + networkStyle->getTitleAddText();
#ifndef Q_OS_MAC
    QApplication::setWindowIcon(networkStyle->getTrayAndWindowIcon());
    setWindowIcon(networkStyle->getTrayAndWindowIcon());
#else
    MacDockIconHandler::instance()->setIcon(networkStyle->getAppIcon());
#endif
    setWindowTitle(windowTitle);

#if defined(Q_OS_MAC) && QT_VERSION < 0x050000
    // This property is not implemented in Qt 5. Setting it has no effect.
    // A replacement API (QtMacUnifiedToolBar) is available in QtMacExtras.
    setUnifiedTitleAndToolBarOnMac(true);
#endif

    rpcConsole = new RPCConsole(platformStyle, 0);
    helpMessageDialog = new HelpMessageDialog(this, false);
#ifdef ENABLE_WALLET
    if(enableWallet)
    {
        /** Create wallet frame and make it the central widget */
        walletFrame = new WalletFrame(platformStyle, this);
        setCentralWidget(walletFrame);
    } else
#endif // ENABLE_WALLET
    {
        /* When compiled without wallet or -disablewallet is provided,
         * the central widget is the rpc console.
         */
        setCentralWidget(rpcConsole);
    }

    // Accept D&D of URIs
    setAcceptDrops(true);

    // Create actions for the toolbar, menu bar and tray/dock icon
    // Needs walletFrame to be initialized
    createActions();

    // Create application menu bar
    createMenuBar();

    // Create the toolbars
    createToolBars();

    // Create system tray icon and notification
    createTrayIcon(networkStyle);

    // Create status bar
    statusBar();

    // Disable size grip because it looks ugly and nobody needs it
    statusBar()->setSizeGripEnabled(false);

    // Status bar notification icons
    QFrame *frameBlocks = new QFrame();
    frameBlocks->setContentsMargins(0,0,0,0);
    frameBlocks->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    QHBoxLayout *frameBlocksLayout = new QHBoxLayout(frameBlocks);
    frameBlocksLayout->setContentsMargins(3,0,3,0);
    frameBlocksLayout->setSpacing(3);
    unitDisplayControl = new UnitDisplayStatusBarControl(platformStyle);
    labelEncryptionIcon = new QLabel();
    labelConnectionsIcon = new QLabel();
    labelBlocksIcon = new QLabel();
    QLabel *labelUsdlabel = new QLabel("LBT/USD");
    QLabel *labelBtclabel = new QLabel("BTC/USD");
    labelUsdIcon = new QLabel();
    labelBtcIcon = new QLabel();
    
    QLabel *opacityLabel = new QLabel("Opacity:");
    QSpinBox *opacitySpinBox = new QSpinBox();
    opacitySpinBox->setRange(40, 100);
    
    //QLabel *minerLabel = new QLabel("Miner Threads: 0");
	//QSlider *miner = new QSlider(Qt::Horizontal, this);
	//int cores = GetNumCores();
	//miner->setRange(0, cores);
	//QString minermax = QString::number(cores);
	//QLabel *maxminer = new QLabel(minermax);	
	labelStakingIcon = new QLabel();

    frameBlocksLayout->addWidget(labelBtclabel);
    frameBlocksLayout->addWidget(labelBtcIcon);
    frameBlocksLayout->addWidget(labelUsdlabel);
    frameBlocksLayout->addWidget(labelUsdIcon);

    if(enableWallet)
    {
        frameBlocksLayout->addStretch();
        frameBlocksLayout->addWidget(unitDisplayControl);
        frameBlocksLayout->addStretch();
        frameBlocksLayout->addWidget(labelEncryptionIcon);
    }
    frameBlocksLayout->addStretch();
    frameBlocksLayout->addWidget(labelStakingIcon);
    frameBlocksLayout->addStretch();
    frameBlocksLayout->addWidget(labelConnectionsIcon);
    frameBlocksLayout->addStretch();
    frameBlocksLayout->addWidget(labelBlocksIcon);
    frameBlocksLayout->addStretch();

    // Progress bar and label for blocks download
    progressBarLabel = new QLabel();
    progressBarLabel->setVisible(false);
    progressBar = new GUIUtil::ProgressBar();
    progressBar->setAlignment(Qt::AlignCenter);
    progressBar->setVisible(false);

    // Override style sheet for progress bar for styles that have a segmented progress bar,
    // as they make the text unreadable (workaround for issue #1071)
    // See https://qt-project.org/doc/qt-4.8/gallery.html
    QString curStyle = QApplication::style()->metaObject()->className();
    if(curStyle == "QWindowsStyle" || curStyle == "QWindowsXPStyle")
    {
        progressBar->setStyleSheet("QProgressBar { background-color: #e8e8e8; border: 1px solid grey; border-radius: 7px; padding: 1px; text-align: center; } QProgressBar::chunk { background: QLinearGradient(x1: 0, y1: 0, x2: 1, y2: 0, stop: 0 #FF8000, stop: 1 orange); border-radius: 7px; margin: 0px; }");
    }

    statusBar()->addWidget(progressBarLabel);
    statusBar()->addWidget(progressBar);
    
    //statusBar()->addWidget(minerLabel);
    //statusBar()->addWidget(miner);
    //statusBar()->addWidget(maxminer);
    statusBar()->addWidget(opacityLabel);
    statusBar()->addWidget(opacitySpinBox);
    
    statusBar()->addPermanentWidget(frameBlocks);

    connect(openRepairAction, SIGNAL(triggered()), rpcConsole, SLOT(showRepair()));
    connect(openConfEditorAction, SIGNAL(triggered()), rpcConsole, SLOT(showConfEditor()));
    connect(openMNConfEditorAction, SIGNAL(triggered()), rpcConsole, SLOT(showMNConfEditor()));
    connect(showBackupsAction, SIGNAL(triggered()), rpcConsole, SLOT(showBackups()));

    // Get restart command-line parameters and handle restart
    connect(rpcConsole, SIGNAL(handleRestart(QStringList)), this, SLOT(handleRestart(QStringList)));

    // Install event filter to be able to catch status tip events (QEvent::StatusTip)
    this->installEventFilter(this);

    // Initially wallet actions should be disabled
    setWalletActionsEnabled(false);

    // Subscribe to notifications from core
    subscribeToCoreSignals();
    
	timer = new QTimer();
	connect(this->timer,SIGNAL(timeout()),this,SLOT(setprice()));
	
	timer->start(180000);

    opacitySpinBox->setValue(static_cast<int>(windowOpacity() * 100));
    connect(opacitySpinBox, SIGNAL(valueChanged(int)), this, SLOT(on_opacitySpinBox_valueChanged(int)));
    //connect(miner, SIGNAL(valueChanged(int)), this, SLOT(engageDisengageMining(int)));

    QTimer* timerStakingIcon = new QTimer(labelStakingIcon);
    connect(timerStakingIcon, SIGNAL(timeout()), this, SLOT(setStakingStatus()));
    timerStakingIcon->start(99999);

}

void LibertaGUI::on_opacitySpinBox_valueChanged(int i)
{
    // update the window opacity
    if(i > 40)
        setWindowOpacity(qreal(i)/100);
}

void LibertaGUI::setprice(){
   
   QString xprice = QString::number(usdprice, 'f',2);
   labelBtcIcon->setStyleSheet("font-weight: bold; color: black");
   this->labelBtcIcon->setText(xprice);

   QString price = QString::number(usdrate, 'f',2);
   labelUsdIcon->setStyleSheet("font-weight: bold; color: black");
   this->labelUsdIcon->setText(price);


}

LibertaGUI::~LibertaGUI()
{
    // Unsubscribe from notifications from core
    unsubscribeFromCoreSignals();

    GUIUtil::saveWindowGeometry("nWindow", this);
    if(trayIcon) // Hide tray icon, as deleting will let it linger until quit (on Ubuntu)
        trayIcon->hide();
#ifdef Q_OS_MAC
    delete appMenuBar;
    MacDockIconHandler::cleanup();
#endif

    delete rpcConsole;
}

void LibertaGUI::createActions()
{
    QActionGroup *tabGroup = new QActionGroup(this);

    overviewAction = new QAction(platformStyle->SingleColorIcon(":/icons/overview"), tr("&Overview"), this);
    overviewAction->setStatusTip(tr("Show general overview of wallet"));
    overviewAction->setToolTip(overviewAction->statusTip());
    overviewAction->setCheckable(true);
    overviewAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_1));
    tabGroup->addAction(overviewAction);

    sendCoinsAction = new QAction(platformStyle->SingleColorIcon(":/icons/send"), tr("&Send"), this);
    sendCoinsAction->setStatusTip(tr("Send coins to a Liberta address"));
    sendCoinsAction->setToolTip(sendCoinsAction->statusTip());
    sendCoinsAction->setCheckable(true);
    sendCoinsAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_2));
    tabGroup->addAction(sendCoinsAction);

    sendCoinsMenuAction = new QAction(platformStyle->TextColorIcon(":/icons/send"), sendCoinsAction->text(), this);
    sendCoinsMenuAction->setStatusTip(sendCoinsAction->statusTip());
    sendCoinsMenuAction->setToolTip(sendCoinsMenuAction->statusTip());

    receiveCoinsAction = new QAction(platformStyle->SingleColorIcon(":/icons/receiving_addresses"), tr("&Receive"), this);
    receiveCoinsAction->setStatusTip(tr("Request payments (generates QR codes and liberta: URIs)"));
    receiveCoinsAction->setToolTip(receiveCoinsAction->statusTip());
    receiveCoinsAction->setCheckable(true);
    receiveCoinsAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_3));
    tabGroup->addAction(receiveCoinsAction);

    receiveCoinsMenuAction = new QAction(platformStyle->TextColorIcon(":/icons/receiving_addresses"), receiveCoinsAction->text(), this);
    receiveCoinsMenuAction->setStatusTip(receiveCoinsAction->statusTip());
    receiveCoinsMenuAction->setToolTip(receiveCoinsMenuAction->statusTip());

    historyAction = new QAction(platformStyle->SingleColorIcon(":/icons/history"), tr("&Transactions"), this);
    historyAction->setStatusTip(tr("Browse transaction history"));
    historyAction->setToolTip(historyAction->statusTip());
    historyAction->setCheckable(true);
    historyAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_4));
    tabGroup->addAction(historyAction);

#ifdef ENABLE_WALLET


    masternodeAction = new QAction(platformStyle->SingleColorIcon(":/icons/masternode"), tr("&Masternodes"), this);
    masternodeAction->setStatusTip(tr("Browse masternodes"));
    masternodeAction->setToolTip(masternodeAction->statusTip());
    masternodeAction->setCheckable(true);
#ifdef Q_OS_MAC
    masternodeAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_5));
#else
    masternodeAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_5));
#endif
    tabGroup->addAction(masternodeAction);
    connect(masternodeAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(masternodeAction, SIGNAL(triggered()), this, SLOT(gotoMasternodePage()));
    

    // These showNormalIfMinimized are needed because Send Coins and Receive Coins
    // can be triggered from the tray menu, and need to show the GUI to be useful.
    connect(overviewAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(overviewAction, SIGNAL(triggered()), this, SLOT(gotoOverviewPage()));
    connect(sendCoinsAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(sendCoinsAction, SIGNAL(triggered()), this, SLOT(gotoSendCoinsPage()));
    connect(sendCoinsMenuAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(sendCoinsMenuAction, SIGNAL(triggered()), this, SLOT(gotoSendCoinsPage()));
    connect(receiveCoinsAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(receiveCoinsAction, SIGNAL(triggered()), this, SLOT(gotoReceiveCoinsPage()));
    connect(receiveCoinsMenuAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(receiveCoinsMenuAction, SIGNAL(triggered()), this, SLOT(gotoReceiveCoinsPage()));
    connect(historyAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(historyAction, SIGNAL(triggered()), this, SLOT(gotoHistoryPage()));
#endif // ENABLE_WALLET

    quitAction = new QAction(platformStyle->TextColorIcon(":/icons/quit"), tr("E&xit"), this);
    quitAction->setStatusTip(tr("Quit application"));
    quitAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q));
    quitAction->setMenuRole(QAction::QuitRole);
    aboutAction = new QAction(platformStyle->TextColorIcon(":/icons/about"), tr("&About Liberta"), this);
    aboutAction->setStatusTip(tr("Show information about Liberta"));
    aboutAction->setMenuRole(QAction::AboutRole);
    aboutAction->setEnabled(false);
    aboutQtAction = new QAction(platformStyle->TextColorIcon(":/icons/about_qt"), tr("About &Qt"), this);
    aboutQtAction->setStatusTip(tr("Show information about Qt"));
    aboutQtAction->setMenuRole(QAction::AboutQtRole);
    optionsAction = new QAction(platformStyle->TextColorIcon(":/icons/options"), tr("&Options..."), this);
    optionsAction->setStatusTip(tr("Modify configuration options for Liberta"));
    optionsAction->setMenuRole(QAction::PreferencesRole);
    optionsAction->setEnabled(false);
    toggleHideAction = new QAction(platformStyle->TextColorIcon(":/icons/about"), tr("&Show / Hide"), this);
    toggleHideAction->setStatusTip(tr("Show or hide the main Window"));

    encryptWalletAction = new QAction(platformStyle->TextColorIcon(":/icons/lock_closed"), tr("&Encrypt Wallet..."), this);
    encryptWalletAction->setStatusTip(tr("Encrypt the private keys that belong to your wallet"));
    encryptWalletAction->setCheckable(true);
    backupWalletAction = new QAction(platformStyle->TextColorIcon(":/icons/filesave"), tr("&Backup Wallet..."), this);
    backupWalletAction->setStatusTip(tr("Backup wallet to another location"));
    changePassphraseAction = new QAction(platformStyle->TextColorIcon(":/icons/key"), tr("&Change Passphrase..."), this);
    changePassphraseAction->setStatusTip(tr("Change the passphrase used for wallet encryption"));
    unlockWalletAction = new QAction(tr("&Unlock Wallet..."), this);
    unlockWalletAction->setToolTip(tr("Unlock wallet"));
    lockWalletAction = new QAction(tr("&Lock Wallet"), this);
    signMessageAction = new QAction(platformStyle->TextColorIcon(":/icons/edit"), tr("Sign &message..."), this);
    signMessageAction->setStatusTip(tr("Sign messages with your Liberta addresses to prove you own them"));
    verifyMessageAction = new QAction(platformStyle->TextColorIcon(":/icons/verify"), tr("&Verify message..."), this);
    verifyMessageAction->setStatusTip(tr("Verify messages to ensure they were signed with specified Liberta addresses"));
    multiSendAction = new QAction(QIcon(":/icons/edit"), tr("&MultiSend"), this);
    multiSendAction->setToolTip(tr("MultiSend Settings"));
    multiSendAction->setCheckable(true);

    openRepairAction = new QAction(QIcon(":/icons/options"), tr("Wallet &Repair"), this);
    openRepairAction->setStatusTip(tr("Show wallet repair options"));
    openConfEditorAction = new QAction(QIcon(":/icons/edit"), tr("Open Wallet &Configuration File"), this);
    openConfEditorAction->setStatusTip(tr("Open configuration file"));
    openMNConfEditorAction = new QAction(QIcon(":/icons/edit"), tr("Open &Masternode Configuration File"), this);
    openMNConfEditorAction->setStatusTip(tr("Open Masternode configuration file"));
    showBackupsAction = new QAction(QIcon(":/icons/browse"), tr("Show Automatic &Backups"), this);
    showBackupsAction->setStatusTip(tr("Show automatically created wallet backups"));

    openRPCConsoleAction = new QAction(platformStyle->TextColorIcon(":/icons/debugwindow"), tr("&Debug window"), this);
    openRPCConsoleAction->setStatusTip(tr("Open debugging and diagnostic console"));
    // initially disable the debug window menu item
    openRPCConsoleAction->setEnabled(false);

    usedSendingAddressesAction = new QAction(platformStyle->TextColorIcon(":/icons/address-book"), tr("&Sending addresses..."), this);
    usedSendingAddressesAction->setStatusTip(tr("Show the list of used sending addresses and labels"));
    usedReceivingAddressesAction = new QAction(platformStyle->TextColorIcon(":/icons/address-book"), tr("&Receiving addresses..."), this);
    usedReceivingAddressesAction->setStatusTip(tr("Show the list of used receiving addresses and labels"));

    openAction = new QAction(platformStyle->TextColorIcon(":/icons/open"), tr("Open &URI..."), this);
    openAction->setStatusTip(tr("Open a Liberta: URI or payment request"));

    browserAction = new QAction(platformStyle->SingleColorIcon(":/icons/browser"), tr("&Web Browser..."), this);
    browserAction->setStatusTip(tr("Surf's up!"));

    pbxclientAction = new QAction(platformStyle->SingleColorIcon(":/icons/phone"), tr("&PBX Phone"), this);
    pbxclientAction->setStatusTip(tr("Phone"));

    openTradingwindowAction = new QAction(platformStyle->SingleColorIcon(":/icons/trade"), tr("&Trading window"), this);
    openTradingwindowAction->setStatusTip(tr("Bittrex trading window"));
    openBlockExplorerAction = new QAction(platformStyle->SingleColorIcon(":/icons/explorer"), tr("&Blockchain explorer"), this);
    openBlockExplorerAction->setStatusTip(tr("Block explorer window"));

    showHelpMessageAction = new QAction(platformStyle->TextColorIcon(":/icons/info"), tr("&Command-line options"), this);
    showHelpMessageAction->setMenuRole(QAction::NoRole);
    showHelpMessageAction->setStatusTip(tr("Show the Liberta help message to get a list with possible Liberta command-line options"));

    connect(quitAction, SIGNAL(triggered()), qApp, SLOT(quit()));
    connect(aboutAction, SIGNAL(triggered()), this, SLOT(aboutClicked()));
    connect(aboutQtAction, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
    connect(optionsAction, SIGNAL(triggered()), this, SLOT(optionsClicked()));
    connect(toggleHideAction, SIGNAL(triggered()), this, SLOT(toggleHidden()));
    connect(showHelpMessageAction, SIGNAL(triggered()), this, SLOT(showHelpMessageClicked()));
    connect(browserAction, SIGNAL(triggered()), this, SLOT(browserClicked()));

    connect(openRPCConsoleAction, SIGNAL(triggered()), this, SLOT(showDebugWindow()));
    // prevents an open debug window from becoming stuck/unusable on client shutdown
    connect(quitAction, SIGNAL(triggered()), rpcConsole, SLOT(hide()));

#ifdef ENABLE_WALLET
    if(walletFrame)
    {
        connect(encryptWalletAction, SIGNAL(triggered(bool)), walletFrame, SLOT(encryptWallet(bool)));
        connect(backupWalletAction, SIGNAL(triggered()), walletFrame, SLOT(backupWallet()));
        connect(changePassphraseAction, SIGNAL(triggered()), walletFrame, SLOT(changePassphrase()));
        connect(unlockWalletAction, SIGNAL(triggered()), walletFrame, SLOT(unlockWallet()));
        connect(lockWalletAction, SIGNAL(triggered()), walletFrame, SLOT(lockWallet()));
        connect(signMessageAction, SIGNAL(triggered()), this, SLOT(gotoSignMessageTab()));
        connect(verifyMessageAction, SIGNAL(triggered()), this, SLOT(gotoVerifyMessageTab()));
        connect(usedSendingAddressesAction, SIGNAL(triggered()), walletFrame, SLOT(usedSendingAddresses()));
        connect(usedReceivingAddressesAction, SIGNAL(triggered()), walletFrame, SLOT(usedReceivingAddresses()));
        connect(openAction, SIGNAL(triggered()), walletFrame, SLOT(openClicked()));
        connect(multiSendAction, SIGNAL(triggered()), walletFrame, SLOT(gotoMultiSendDialog()));
        connect(pbxclientAction, SIGNAL(triggered()), walletFrame, SLOT(gotoPbxClient()));
        connect(openBlockExplorerAction, SIGNAL(triggered()), walletFrame, SLOT(gotoBlockExplorerPage()));
        connect(openTradingwindowAction, SIGNAL(triggered()), walletFrame, SLOT(gotoTradingPage()));

    }
#endif // ENABLE_WALLET

    new QShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_C), this, SLOT(showDebugWindowActivateConsole()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_D), this, SLOT(showDebugWindow()));
}

void LibertaGUI::createMenuBar()
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
#if defined(Q_OS_WIN)
        file->setStyleSheet("font-weight: bold; color: black, background-color: transparent");
#endif

    if(walletFrame)
    {
        file->addAction(openAction);
        file->addAction(backupWalletAction);
        file->addAction(signMessageAction);
        file->addAction(verifyMessageAction);
        file->addSeparator();
        file->addAction(usedSendingAddressesAction);
        file->addAction(usedReceivingAddressesAction);
        file->addSeparator();
    }
    file->addAction(quitAction);

    QMenu *settings = appMenuBar->addMenu(tr("&Settings"));
#if defined(Q_OS_WIN)
        settings->setStyleSheet("font-weight: bold; color: black, background-color: transparent");
#endif
    if(walletFrame)
    {
        settings->addAction(encryptWalletAction);
        settings->addAction(changePassphraseAction);
        settings->addAction(unlockWalletAction);
        settings->addAction(lockWalletAction);
        settings->addAction(multiSendAction);
        settings->addSeparator();
    }
    settings->addAction(optionsAction);

    if (walletFrame) {

        QMenu* tools = appMenuBar->addMenu(tr("&Tools"));
#if defined(Q_OS_WIN)
        tools->setStyleSheet("font-weight: bold; color: black, background-color: transparent");
#endif
        tools->addAction(openRepairAction);
        tools->addAction(openConfEditorAction);
        tools->addAction(openMNConfEditorAction);
        tools->addAction(showBackupsAction);
    }

    QMenu *help = appMenuBar->addMenu(tr("&Help"));
#if defined(Q_OS_WIN)
        help->setStyleSheet("font-weight: bold; color: black, background-color: transparent");
#endif

    if(walletFrame)
    {
        help->addAction(openRPCConsoleAction);
    }
    help->addAction(showHelpMessageAction);
    help->addSeparator();
    help->addAction(aboutAction);
    help->addAction(aboutQtAction);
}

void LibertaGUI::engageDisengageMining(int cores)
{
    isMiningEngaged = false;
    if (cores > 0)
        isMiningEngaged = true;

    GenerateLibertas(isMiningEngaged, cores, Params());

}

void LibertaGUI::createToolBars()
{
    labelLogo = new QLabel();
    labelLogo->show();
    labelLogo->setPixmap(QPixmap(":icons/banner"));
    
    QToolBar *toolbar = addToolBar(tr("Tabs toolbar"));
    addToolBar(Qt::LeftToolBarArea, toolbar);
    toolbar->setOrientation(Qt::Vertical);
    toolbar->setFixedWidth(228);
    toolbar->setFixedHeight(450);
    //toolbar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    toolbar->setMovable(false);
    toolbar->addWidget(labelLogo);
    toolbar->setIconSize(QSize(50,20));
    toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    QWidget* space = new QWidget();
    space->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    toolbar->addWidget(space);
    toolbar->setStyleSheet("#toolbar {border-radius: 20px; font-weight:600; background:rgba(255, 255, 255, 0); text-align: left; min-width:180px;} QToolBar QToolButton { font-weight:600;font-size:12px; padding-left:20px; padding-right:180px; padding-top:4px;padding-bottom:4px; width:100%; text-align: left; text-transform:uppercase; }");

    if(walletFrame)
    {
        toolbar->addAction(overviewAction);
        overviewAction->setChecked(true);
        toolbar->addAction(sendCoinsAction);
        toolbar->addAction(receiveCoinsAction);
        toolbar->addAction(historyAction);
        toolbar->addAction(masternodeAction);
        toolbar->addAction(openBlockExplorerAction);
        toolbar->addAction(openTradingwindowAction);
    }

    //browserButton = new QPushButton(this);
    //browserButton->setText("Web Browser");
    //connect(browserButton, SIGNAL(released()), this, SLOT(browserClicked()));
    //toolbar->addWidget(browserButton);

    toolbar->addWidget(space);

}

void LibertaGUI::setClientModel(ClientModel *clientModel)
{
    this->clientModel = clientModel;
    if(clientModel)
    {
        // Create system tray menu (or setup the dock menu) that late to prevent users from calling actions,
        // while the client has not yet fully loaded
        createTrayIconMenu();

        // Keep up to date with client
        setNumConnections(clientModel->getNumConnections());
        connect(clientModel, SIGNAL(numConnectionsChanged(int)), this, SLOT(setNumConnections(int)));

        setNumBlocks(clientModel->getNumBlocks(), clientModel->getLastBlockDate(), clientModel->getVerificationProgress(NULL));
        connect(clientModel, SIGNAL(numBlocksChanged(int,QDateTime,double)), this, SLOT(setNumBlocks(int,QDateTime,double)));

        // Receive and report messages from client model
        connect(clientModel, SIGNAL(message(QString,QString,unsigned int)), this, SLOT(message(QString,QString,unsigned int)));

        // Show progress dialog
        connect(clientModel, SIGNAL(showProgress(QString,int)), this, SLOT(showProgress(QString,int)));

        rpcConsole->setClientModel(clientModel);
#ifdef ENABLE_WALLET
        if(walletFrame)
        {
            walletFrame->setClientModel(clientModel);
        }
#endif // ENABLE_WALLET
        unitDisplayControl->setOptionsModel(clientModel->getOptionsModel());
    } else {
        // Disable possibility to show main window via action
        toggleHideAction->setEnabled(false);
        if(trayIconMenu)
        {
            // Disable context menu on tray icon
            trayIconMenu->clear();
        }
    }
}

#ifdef ENABLE_WALLET
bool LibertaGUI::addWallet(const QString& name, WalletModel *walletModel)
{
    if(!walletFrame)
        return false;
    setWalletActionsEnabled(true);
    return walletFrame->addWallet(name, walletModel);
}

bool LibertaGUI::setCurrentWallet(const QString& name)
{
    if(!walletFrame)
        return false;
    return walletFrame->setCurrentWallet(name);
}

void LibertaGUI::removeAllWallets()
{
    if(!walletFrame)
        return;
    setWalletActionsEnabled(false);
    walletFrame->removeAllWallets();
}
#endif // ENABLE_WALLET

void LibertaGUI::setWalletActionsEnabled(bool enabled)
{
    overviewAction->setEnabled(enabled);
    sendCoinsAction->setEnabled(enabled);
    sendCoinsMenuAction->setEnabled(enabled);
    receiveCoinsAction->setEnabled(enabled);
    receiveCoinsMenuAction->setEnabled(enabled);
    historyAction->setEnabled(enabled);
    masternodeAction->setEnabled(enabled);
    encryptWalletAction->setEnabled(enabled);
    backupWalletAction->setEnabled(enabled);
    changePassphraseAction->setEnabled(enabled);
    signMessageAction->setEnabled(enabled);
    verifyMessageAction->setEnabled(enabled);
    usedSendingAddressesAction->setEnabled(enabled);
    usedReceivingAddressesAction->setEnabled(enabled);
    openAction->setEnabled(enabled);
}

void LibertaGUI::createTrayIcon(const NetworkStyle *networkStyle)
{
#ifndef Q_OS_MAC
    trayIcon = new QSystemTrayIcon(this);
    QString toolTip = tr("Liberta client") + " " + networkStyle->getTitleAddText();
    trayIcon->setToolTip(toolTip);
    trayIcon->setIcon(networkStyle->getTrayAndWindowIcon());
    trayIcon->show();
#endif

    notificator = new Notificator(QApplication::applicationName(), trayIcon, this);
}

void LibertaGUI::createTrayIconMenu()
{
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
    trayIconMenu->addAction(browserAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(sendCoinsMenuAction);
    trayIconMenu->addAction(receiveCoinsMenuAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(signMessageAction);
    trayIconMenu->addAction(verifyMessageAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(optionsAction);
    trayIconMenu->addAction(openTradingwindowAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(openRPCConsoleAction);
    trayIconMenu->addAction(openRepairAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(openConfEditorAction);
    trayIconMenu->addAction(openMNConfEditorAction);
    trayIconMenu->addAction(showBackupsAction);
    trayIconMenu->addAction(openBlockExplorerAction);
#ifndef Q_OS_MAC // This is built-in on Mac
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(quitAction);
#endif
}

#ifndef Q_OS_MAC
void LibertaGUI::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if(reason == QSystemTrayIcon::Trigger)
    {
        // Click on system tray icon triggers show/hide of the main window
        toggleHidden();
    }
}
#endif

void LibertaGUI::optionsClicked()
{
    if(!clientModel || !clientModel->getOptionsModel())
        return;

    OptionsDialog dlg(this, enableWallet);
    dlg.setModel(clientModel->getOptionsModel());
    dlg.exec();
}

void LibertaGUI::aboutClicked()
{
    if(!clientModel)
        return;

    HelpMessageDialog dlg(this, true);
    dlg.exec();
}

void LibertaGUI::showDebugWindow()
{
    rpcConsole->showNormal();
    rpcConsole->show();
    rpcConsole->raise();
    rpcConsole->activateWindow();
}

void LibertaGUI::showDebugWindowActivateConsole()
{
    rpcConsole->setTabFocus(RPCConsole::TAB_CONSOLE);
    showDebugWindow();
}

void LibertaGUI::showHelpMessageClicked()
{
    helpMessageDialog->show();
}

void LibertaGUI::browserClicked()
{
   // BrowserApplication::instance()->newMainWindow();
}

void LibertaGUI::gotoPbxClient()
{
    pbxclientAction->setChecked(true);
    if (walletFrame) walletFrame->gotoPbxClient();
}

#ifdef ENABLE_WALLET
void LibertaGUI::openClicked()
{
    OpenURIDialog dlg(this);
    if(dlg.exec())
    {
        Q_EMIT receivedURI(dlg.getURI());
    }
}

void LibertaGUI::gotoOverviewPage()
{
    overviewAction->setChecked(true);
    if (walletFrame) walletFrame->gotoOverviewPage();
}

void LibertaGUI::gotoHistoryPage()
{
    historyAction->setChecked(true);
    if (walletFrame) walletFrame->gotoHistoryPage();
}

void LibertaGUI::gotoMasternodePage()
{
    QSettings settings;
    if (settings.value("fShowMasternodesTab").toBool()) {
        masternodeAction->setChecked(true);
        if (walletFrame) walletFrame->gotoMasternodePage();
    }
}

void LibertaGUI::gotoReceiveCoinsPage()
{
    receiveCoinsAction->setChecked(true);
    if (walletFrame) walletFrame->gotoReceiveCoinsPage();
}

void LibertaGUI::gotoSendCoinsPage(QString addr)
{
    sendCoinsAction->setChecked(true);
    if (walletFrame) walletFrame->gotoSendCoinsPage(addr);
}

void LibertaGUI::gotoSignMessageTab(QString addr)
{
    if (walletFrame) walletFrame->gotoSignMessageTab(addr);
}

void LibertaGUI::gotoVerifyMessageTab(QString addr)
{
    if (walletFrame) walletFrame->gotoVerifyMessageTab(addr);
}

void LibertaGUI::gotoTradingPage()
{
    overviewAction->setChecked(true);
    if (walletFrame) walletFrame->gotoTradingPage();
}

void LibertaGUI::gotoMultiSendDialog()
{
    multiSendAction->setChecked(true);
    if (walletFrame)
        walletFrame->gotoMultiSendDialog();
}
void LibertaGUI::gotoBlockExplorerPage()
{
    if (walletFrame) walletFrame->gotoBlockExplorerPage();
}

#endif // ENABLE_WALLET

void LibertaGUI::setNumConnections(int count)
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
    labelConnectionsIcon->setPixmap(platformStyle->SingleColorIcon(icon).pixmap(STATUSBAR_ICONSIZE,STATUSBAR_ICONSIZE));
    labelConnectionsIcon->setToolTip(tr("%n active connection(s) to Liberta network", "", count));
}

void LibertaGUI::setNumBlocks(int count, const QDateTime& blockDate, double nVerificationProgress)
{
    if(!clientModel)
        return;

    // Prevent orphan statusbar messages (e.g. hover Quit in main menu, wait until chain-sync starts -> garbelled text)
    statusBar()->clearMessage();

    // Acquire current block source
    enum BlockSource blockSource = clientModel->getBlockSource();
    switch (blockSource) {
        case BLOCK_SOURCE_NETWORK:
            progressBarLabel->setText(tr("Syncing with network..."));
            break;
        case BLOCK_SOURCE_DISK:
            progressBarLabel->setText(tr("Importing blocks from disk..."));
            break;
        case BLOCK_SOURCE_REINDEX:
            progressBarLabel->setText(tr("Reindexing blocks on disk..."));
            break;
        case BLOCK_SOURCE_NONE:
            // Case: not Importing, not Reindexing and no network connection
            progressBarLabel->setText(tr("No block source available..."));
            break;
    }

    QString tooltip;

    QDateTime lastBlockDate = clientModel->getLastBlockDate();
    QDateTime currentDate = QDateTime::currentDateTime();
    qint64 secs = blockDate.secsTo(currentDate);

    tooltip = tr("Processed %n block(s) of transaction history.", "", count);

    // Set icon state: spinning if catching up, tick otherwise
    //if(secs < 25*60)
    if (masternodeSync.IsBlockchainSynced()) {
        QString strSyncStatus;
        tooltip = tr("Up to date") + QString(".<br>") + tooltip;

        if (masternodeSync.IsSynced()) {
            progressBarLabel->setVisible(false);
            progressBar->setVisible(false);
            labelBlocksIcon->setPixmap(QIcon(":/icons/synced").pixmap(STATUSBAR_ICONSIZE, STATUSBAR_ICONSIZE));
        } else {
            int nAttempt;
            int progress = 0;

            labelBlocksIcon->setPixmap(QIcon(QString(
                                                 ":/movies/spinner-%1")
                                                 .arg(spinnerFrame, 3, 10, QChar('0')))
                                           .pixmap(STATUSBAR_ICONSIZE, STATUSBAR_ICONSIZE));
            spinnerFrame = (spinnerFrame + 1) % SPINNER_FRAMES;

#ifdef ENABLE_WALLET
            if (walletFrame)
                walletFrame->showOutOfSyncWarning(false);
#endif // ENABLE_WALLET

            nAttempt = masternodeSync.RequestedMasternodeAttempt < MASTERNODE_SYNC_THRESHOLD ?
                           masternodeSync.RequestedMasternodeAttempt + 1 :
                           MASTERNODE_SYNC_THRESHOLD;
            progress = nAttempt + (masternodeSync.RequestedMasternodeAssets - 1) * MASTERNODE_SYNC_THRESHOLD;
            progressBar->setMaximum(4 * MASTERNODE_SYNC_THRESHOLD);
            progressBar->setFormat(tr("Syncing additional data: %p%"));
            progressBar->setValue(progress);
        }

        strSyncStatus = QString(masternodeSync.GetSyncStatus().c_str());
        progressBarLabel->setText(strSyncStatus);
        tooltip = strSyncStatus + QString("<br>") + tooltip;
    } else
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
            qint64 years = secs / YEAR_IN_SECONDS;
            qint64 remainder = secs % YEAR_IN_SECONDS;
            timeBehindText = tr("%1 and %2").arg(tr("%n year(s)", "", years)).arg(tr("%n week(s)","", remainder/WEEK_IN_SECONDS));
        }

        progressBarLabel->setVisible(true);
        progressBar->setFormat(tr("%1 behind").arg(timeBehindText));
        progressBar->setMaximum(1000000000);
        progressBar->setValue(nVerificationProgress * 1000000000.0 + 0.5);
        progressBar->setVisible(true);

        tooltip = tr("Catching up...") + QString("<br>") + tooltip;
        if(count != prevBlocks)
        {
            labelBlocksIcon->setPixmap(platformStyle->SingleColorIcon(QString(
                ":/movies/spinner-%1").arg(spinnerFrame, 3, 10, QChar('0')))
                .pixmap(STATUSBAR_ICONSIZE, STATUSBAR_ICONSIZE));
            spinnerFrame = (spinnerFrame + 1) % SPINNER_FRAMES;
        }
        prevBlocks = count;

#ifdef ENABLE_WALLET
        if(walletFrame)
            walletFrame->showOutOfSyncWarning(true);
#endif // ENABLE_WALLET

        tooltip += QString("<br>");
        tooltip += tr("Last received block was generated %1 ago.").arg(timeBehindText);
        tooltip += QString("<br>");
        tooltip += tr("Transactions after this will not yet be visible.");
    }

    // Don't word-wrap this (fixed-width) tooltip
    tooltip = QString("<nobr>") + tooltip + QString("</nobr>");

    labelBlocksIcon->setToolTip(tooltip);
    progressBarLabel->setToolTip(tooltip);
    progressBar->setToolTip(tooltip);
}

void LibertaGUI::message(const QString &title, const QString &message, unsigned int style, bool *ret)
{
    QString strTitle = tr("Liberta"); // default title
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
    // Append title to "Liberta- "
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

        showNormalIfMinimized();
        QMessageBox mBox((QMessageBox::Icon)nMBoxIcon, strTitle, message, buttons, this);
        int r = mBox.exec();
        if (ret != NULL)
            *ret = r == QMessageBox::Ok;
    }
    else
        notificator->notify((Notificator::Class)nNotifyIcon, strTitle, message);
}

void LibertaGUI::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
#ifndef Q_OS_MAC // Ignored on Mac
    if(e->type() == QEvent::WindowStateChange)
    {
        if(clientModel && clientModel->getOptionsModel() && clientModel->getOptionsModel()->getMinimizeToTray())
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

void LibertaGUI::closeEvent(QCloseEvent *event)
{
    if (isMiningEngaged)
        engageDisengageMining(0);
#ifndef Q_OS_MAC // Ignored on Mac
    if(clientModel && clientModel->getOptionsModel())
    {
        if(!clientModel->getOptionsModel()->getMinimizeToTray() &&
           !clientModel->getOptionsModel()->getMinimizeOnClose())
        {
            // close rpcConsole in case it was open to make some space for the shutdown window
            rpcConsole->close();

            QApplication::quit();
        }
    }
#endif
    QMainWindow::closeEvent(event);
}

void LibertaGUI::showEvent(QShowEvent *event)
{
    // enable the debug window when the main window shows up
    openRPCConsoleAction->setEnabled(true);
    aboutAction->setEnabled(true);
    optionsAction->setEnabled(true);
}

#ifdef ENABLE_WALLET
void LibertaGUI::incomingTransaction(const QString& date, int unit, const CAmount& amount, const QString& type, const QString& address, const QString& label)
{
    // On new transaction, make an info balloon
    QString msg = tr("Date: %1\n").arg(date) +
                  tr("Amount: %1\n").arg(LibertaUnits::formatWithUnit(unit, amount, true)) +
                  tr("Type: %1\n").arg(type);
    if (!label.isEmpty())
        msg += tr("Label: %1\n").arg(label);
    else if (!address.isEmpty())
        msg += tr("Address: %1\n").arg(address);
    message((amount)<0 ? tr("Sent transaction") : tr("Incoming transaction"),
             msg, CClientUIInterface::MSG_INFORMATION);
}
#endif // ENABLE_WALLET

void LibertaGUI::dragEnterEvent(QDragEnterEvent *event)
{
    // Accept only URIs
    if(event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void LibertaGUI::dropEvent(QDropEvent *event)
{
    if(event->mimeData()->hasUrls())
    {
        Q_FOREACH(const QUrl &uri, event->mimeData()->urls())
        {
            Q_EMIT receivedURI(uri.toString());
        }
    }
    event->acceptProposedAction();
}

bool LibertaGUI::eventFilter(QObject *object, QEvent *event)
{
    // Catch status tip events
    if (event->type() == QEvent::StatusTip)
    {
        // Prevent adding text from setStatusTip(), if we currently use the status bar for displaying other stuff
        if (progressBarLabel->isVisible() || progressBar->isVisible())
            return true;
    }
    return QMainWindow::eventFilter(object, event);
}

void LibertaGUI::setStakingStatus()
{
    if (pwalletMain)
        fMultiSend = pwalletMain->isMultiSendEnabled();

    if (nLastCoinStakeSearchInterval) {
        labelStakingIcon->show();
        labelStakingIcon->setPixmap(QIcon(":/icons/staking_active").pixmap(STATUSBAR_ICONSIZE, STATUSBAR_ICONSIZE));
        labelStakingIcon->setToolTip(tr("Staking is active\n MultiSend: %1").arg(fMultiSend ? tr("Active") : tr("Not Active")));
    } else {
        labelStakingIcon->show();
        labelStakingIcon->setPixmap(QIcon(":/icons/staking_inactive").pixmap(STATUSBAR_ICONSIZE, STATUSBAR_ICONSIZE));
        labelStakingIcon->setToolTip(tr("Staking is not active\n MultiSend: %1").arg(fMultiSend ? tr("Active") : tr("Not Active")));
    }
}

#ifdef ENABLE_WALLET
bool LibertaGUI::handlePaymentRequest(const SendCoinsRecipient& recipient)
{
    // URI has to be valid
    if (walletFrame && walletFrame->handlePaymentRequest(recipient))
    {
        showNormalIfMinimized();
        gotoSendCoinsPage();
        return true;
    }
    return false;
}

void LibertaGUI::setEncryptionStatus(int status)
{
    switch(status)
    {
    case WalletModel::Unencrypted:
        labelEncryptionIcon->hide();
        encryptWalletAction->setChecked(false);
        changePassphraseAction->setEnabled(false);
        unlockWalletAction->setVisible(false);
        lockWalletAction->setVisible(false);
        encryptWalletAction->setEnabled(true);
        break;
    case WalletModel::Unlocked:
        nLastCoinStakeSearchInterval = 1;
        labelEncryptionIcon->show();
        labelEncryptionIcon->setPixmap(platformStyle->SingleColorIcon(":/icons/lock_open").pixmap(STATUSBAR_ICONSIZE,STATUSBAR_ICONSIZE));
        labelEncryptionIcon->setToolTip(tr("Wallet is <b>encrypted</b> and currently <b>unlocked</b>"));
        encryptWalletAction->setChecked(true);
        changePassphraseAction->setEnabled(true);
        unlockWalletAction->setVisible(false);
        lockWalletAction->setVisible(true);
        encryptWalletAction->setEnabled(false); // TODO: decrypt currently not supported
        break;
    case WalletModel::UnlockedForAnonymizationOnly:
        nLastCoinStakeSearchInterval = 1;
        labelEncryptionIcon->show();
        labelEncryptionIcon->setPixmap(QIcon(":/icons/lock_open").pixmap(STATUSBAR_ICONSIZE, STATUSBAR_ICONSIZE));
        labelEncryptionIcon->setToolTip(tr("Wallet is <b>encrypted</b> and currently <b>unlocked</b> for anonimization and staking only"));
        encryptWalletAction->setChecked(true);
        changePassphraseAction->setEnabled(true);
        unlockWalletAction->setVisible(true);
        lockWalletAction->setVisible(true);
        encryptWalletAction->setEnabled(false); // TODO: decrypt currently not supported
        break;
    case WalletModel::Locked:
        nLastCoinStakeSearchInterval = 0;
        //printf("nLastCoinStakeSearchInterval ===== %d\n", nLastCoinStakeSearchInterval);
        labelEncryptionIcon->show();
        labelEncryptionIcon->setPixmap(platformStyle->SingleColorIcon(":/icons/lock_closed").pixmap(STATUSBAR_ICONSIZE,STATUSBAR_ICONSIZE));
        labelEncryptionIcon->setToolTip(tr("Wallet is <b>encrypted</b> and currently <b>locked</b>"));
        encryptWalletAction->setChecked(true);
        changePassphraseAction->setEnabled(true);
        unlockWalletAction->setVisible(true);
        lockWalletAction->setVisible(false);
        encryptWalletAction->setEnabled(false); // TODO: decrypt currently not supported
        break;
    }

    
}
#endif // ENABLE_WALLET

void LibertaGUI::showNormalIfMinimized(bool fToggleHidden)
{
    if(!clientModel)
        return;

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

void LibertaGUI::toggleHidden()
{
    showNormalIfMinimized(true);
}

void LibertaGUI::detectShutdown()
{
    if (ShutdownRequested())
    {
        if (isMiningEngaged)
            engageDisengageMining(0);
        if(rpcConsole)
            rpcConsole->hide();
        qApp->quit();
    }
}

void LibertaGUI::showProgress(const QString &title, int nProgress)
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

static bool ThreadSafeMessageBox(LibertaGUI *gui, const std::string& message, const std::string& caption, unsigned int style)
{
    bool modal = (style & CClientUIInterface::MODAL);
    // The SECURE flag has no effect in the Qt GUI.
    // bool secure = (style & CClientUIInterface::SECURE);
    style &= ~CClientUIInterface::SECURE;
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

void LibertaGUI::subscribeToCoreSignals()
{
    // Connect signals to client
    uiInterface.ThreadSafeMessageBox.connect(boost::bind(ThreadSafeMessageBox, this, _1, _2, _3));
}

void LibertaGUI::unsubscribeFromCoreSignals()
{
    // Disconnect signals from client
    uiInterface.ThreadSafeMessageBox.disconnect(boost::bind(ThreadSafeMessageBox, this, _1, _2, _3));
}

/** Get restart command-line parameters and request restart */
void LibertaGUI::handleRestart(QStringList args)
{
    if (!ShutdownRequested())
        Q_EMIT requestedRestart(args);
}

UnitDisplayStatusBarControl::UnitDisplayStatusBarControl(const PlatformStyle *platformStyle) :
    optionsModel(0),
    menu(0)
{
    createContextMenu();
    setToolTip(tr("Unit to show amounts in. Click to select another unit."));
    QList<LibertaUnits::Unit> units = LibertaUnits::availableUnits();
    int max_width = 0;
    const QFontMetrics fm(font());
    Q_FOREACH (const LibertaUnits::Unit unit, units)
    {
        max_width = qMax(max_width, fm.width(LibertaUnits::name(unit)));
    }
    setMinimumSize(max_width, 0);
    setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    setStyleSheet(QString("QLabel { color : %1 }").arg(platformStyle->SingleColor().name()));
}

/** So that it responds to button clicks */
void UnitDisplayStatusBarControl::mousePressEvent(QMouseEvent *event)
{
    onDisplayUnitsClicked(event->pos());
}

/** Creates context menu, its actions, and wires up all the relevant signals for mouse events. */
void UnitDisplayStatusBarControl::createContextMenu()
{
    menu = new QMenu();
    Q_FOREACH(LibertaUnits::Unit u, LibertaUnits::availableUnits())
    {
        QAction *menuAction = new QAction(QString(LibertaUnits::name(u)), this);
        menuAction->setData(QVariant(u));
        menu->addAction(menuAction);
    }
    connect(menu,SIGNAL(triggered(QAction*)),this,SLOT(onMenuSelection(QAction*)));
}

/** Lets the control know about the Options Model (and its signals) */
void UnitDisplayStatusBarControl::setOptionsModel(OptionsModel *optionsModel)
{
    if (optionsModel)
    {
        this->optionsModel = optionsModel;

        // be aware of a display unit change reported by the OptionsModel object.
        connect(optionsModel,SIGNAL(displayUnitChanged(int)),this,SLOT(updateDisplayUnit(int)));

        // initialize the display units label with the current value in the model.
        updateDisplayUnit(optionsModel->getDisplayUnit());
    }
}

/** When Display Units are changed on OptionsModel it will refresh the display text of the control on the status bar */
void UnitDisplayStatusBarControl::updateDisplayUnit(int newUnits)
{
    setText(LibertaUnits::name(newUnits));
}

/** Shows context menu with Display Unit options by the mouse coordinates */
void UnitDisplayStatusBarControl::onDisplayUnitsClicked(const QPoint& point)
{
    QPoint globalPos = mapToGlobal(point);
    menu->exec(globalPos);
}

/** Tells underlying optionsModel to update its current display unit. */
void UnitDisplayStatusBarControl::onMenuSelection(QAction* action)
{
    if (action)
    {
        optionsModel->setDisplayUnit(action->data());
    }
}
