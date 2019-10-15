// Copyright (c) 2011-2019 The Talkcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/talkcoingui.h>

#include <qt/talkcoinunits.h>
#include <qt/clientmodel.h>
#include <qt/guiconstants.h>
#include <qt/guiutil.h>
#include <qt/modaloverlay.h>
#include <qt/networkstyle.h>
#include <qt/notificator.h>
#include <qt/openuridialog.h>
#include <qt/optionsdialog.h>
#include <qt/optionsmodel.h>
#include <qt/platformstyle.h>
#include <qt/rpcconsole.h>
#include <qt/utilitydialog.h>
#include <rpc/server.h>

#ifdef ENABLE_WALLET
#include <qt/walletcontroller.h>
#include <qt/walletframe.h>
#include <qt/walletmodel.h>
#include <qt/walletview.h>
#endif // ENABLE_WALLET

#ifdef Q_OS_MAC
#include <qt/macdockiconhandler.h>
#endif

#include <chain.h>
#include <chainparams.h>
#include <interfaces/handler.h>
#include <interfaces/node.h>
#include <miner.h>
#include <ui_interface.h>
#include <util/system.h>

#include <iostream>
#include <memory>

#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDateTime>
#include <QDragEnterEvent>
#include <QFile>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QProgressDialog>
#include <QScreen>
#include <QSettings>
#include <QShortcut>
#include <QStackedWidget>
#include <QStatusBar>
#include <QStyle>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QToolBar>
#include <QUrlQuery>
#include <QVBoxLayout>
#include <QWindow>


const std::string TalkcoinGUI::DEFAULT_UIPLATFORM =
#if defined(Q_OS_MAC)
        "macosx"
#elif defined(Q_OS_WIN)
        "windows"
#elif defined(Q_OS_ANDROID)
        "android"
#else
        "other"
#endif
        ;

TalkcoinGUI::TalkcoinGUI(interfaces::Node& node, const PlatformStyle *_platformStyle, const NetworkStyle *networkStyle, QWidget *parent) :
    QMainWindow(parent),
    m_node(node),
    trayIconMenu{new QMenu()},
    platformStyle(_platformStyle),
    m_network_style(networkStyle)
{

    QSettings settings;
    
#if defined(Q_OS_ANDROID)
    setFixedSize(QGuiApplication::primaryScreen()->availableSize().width(), QGuiApplication::primaryScreen()->availableSize().height());
#else
    if (!restoreGeometry(settings.value("MainWindowGeometry").toByteArray())) {
        // Restore failed (perhaps missing setting), center the window
        move(QGuiApplication::primaryScreen()->availableGeometry().center() - frameGeometry().center());
    }    
#endif

    setWindowFlags(Qt::FramelessWindowHint);
    // load stylesheet

    if (settings.value("theme", "").toString() == "transparent"){
        setAttribute( Qt::WA_TranslucentBackground);
        setAttribute(Qt::WA_NoSystemBackground);
    }

    QFile qss(GUIUtil::defaultTheme());
    qss.open(QFile::ReadOnly);
    this->setStyleSheet(qss.readAll());
    qss.close();

#ifdef ENABLE_WALLET
    enableWallet = WalletModel::isWalletEnabled();
#endif // ENABLE_WALLET
    QApplication::setWindowIcon(m_network_style->getTrayAndWindowIcon());
    setWindowIcon(m_network_style->getTrayAndWindowIcon());
    updateWindowTitle();

    helpMessageDialog = new HelpMessageDialog(node, this, false);
#ifdef ENABLE_WALLET
    if(enableWallet)
    {
        /** Create wallet frame and make it the central widget */
        walletFrame = new WalletFrame(_platformStyle, this);
        setCentralWidget(walletFrame);
    } else
#endif // ENABLE_WALLET
    {
        /* When compiled without wallet or -disablewallet is provided,
         * the central widget is the rpc console.
         */
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
    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        createTrayIcon();
    }
    notificator = new Notificator(QApplication::applicationName(), trayIcon, this);

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
    labelWalletEncryptionIcon = new QLabel();
    labelWalletHDStatusIcon = new QLabel();
    labelWalletSPVStatusIcon = new GUIUtil::ClickableLabel();
    labelProxyIcon = new GUIUtil::ClickableLabel();
    connectionsControl = new GUIUtil::ClickableLabel();
    labelBlocksIcon = new GUIUtil::ClickableLabel();
    if(enableWallet)
    {
        frameBlocksLayout->addWidget(unitDisplayControl);
        frameBlocksLayout->addWidget(labelWalletEncryptionIcon);
        frameBlocksLayout->addWidget(labelWalletHDStatusIcon);
        frameBlocksLayout->addWidget(labelWalletSPVStatusIcon);
        connect(labelWalletSPVStatusIcon, SIGNAL(clicked(QPoint)), this, SLOT(toggleSPVMode()));
    }
    frameBlocksLayout->addWidget(labelProxyIcon);

    labelStakingIcon = new QLabel();
    frameBlocksLayout->addStretch();
    frameBlocksLayout->addWidget(labelStakingIcon);
    if (gArgs.GetBoolArg("-staking", true))
    {
        QTimer *timerStakingIcon = new QTimer(labelStakingIcon);
        connect(timerStakingIcon, SIGNAL(timeout()), this, SLOT(updateStakingIcon()));
        timerStakingIcon->start(1000);

        updateStakingIcon();
    }

    frameBlocksLayout->addWidget(connectionsControl);
    frameBlocksLayout->addWidget(labelBlocksIcon);

    // Progress bar and label for blocks download
    labelWalletEncryptionIcon->setObjectName("labelEncryptionIcon");
    labelBlocksIcon->setObjectName("labelBlocksIcon");

//    QToolBar *toolbar = addToolBar(tr("toolbar"));
//    addToolBar(Qt::BottomToolBarArea, toolbar);

    // Progress bar and label for blocks download
    progressBarLabel = new QLabel();
    progressBarLabel->setVisible(false);
    progressBar = new GUIUtil::ProgressBar();
    progressBar->setAlignment(Qt::AlignCenter);
    progressBar->setVisible(false);

    // Override style sheet for progress bar for styles that have a segmented progress bar,
    // as they make the text unreadable (workaround for issue #1071)
    // See https://doc.qt.io/qt-5/gallery.html
    QString curStyle = QApplication::style()->metaObject()->className();
    if(curStyle == "QWindowsStyle" || curStyle == "QWindowsXPStyle")
    {
        progressBar->setStyleSheet("QProgressBar { background-color: #e8e8e8; border: 1px solid grey; border-radius: 7px; padding: 1px; text-align: center; } QProgressBar::chunk { background: QLinearGradient(x1: 0, y1: 0, x2: 1, y2: 0, stop: 0 #FF8000, stop: 1 orange); border-radius: 7px; margin: 0px; }");
    }

    statusBar()->addWidget(progressBarLabel);
    statusBar()->addWidget(progressBar);

//    QtMaterialIconButton *quitButton = new QtMaterialIconButton(QtMaterialTheme::icon("close", ":/icons/quit"));
//    quitButton->setIconSize(QSize(30, 30));
//    quitButton->setFixedWidth(42);


    QPushButton *quitButton = new QPushButton();
    quitButton->setIcon(platformStyle->SingleColorIcon(":/icons/quit"));
    quitButton->setToolTip("Exit");
    quitButton->setObjectName("quitButton");
    connect(quitButton, SIGNAL(clicked()), qApp, SLOT(quit()));

    statusBar()->addPermanentWidget(frameBlocks);
    //statusBar()->addPermanentWidget(quitButton);

    // Install event filter to be able to catch status tip events (QEvent::StatusTip)
    this->installEventFilter(this);

    // Initially wallet actions should be disabled
    setWalletActionsEnabled(false);

    // Subscribe to notifications from core
    subscribeToCoreSignals();

    connect(connectionsControl, &GUIUtil::ClickableLabel::clicked, [this] {
        m_node.setNetworkActive(!m_node.getNetworkActive());
    });
    connect(labelProxyIcon, &GUIUtil::ClickableLabel::clicked, [this] {
        openOptionsDialogWithTab(OptionsDialog::TAB_NETWORK);
    });

    modalOverlay = new ModalOverlay(this->centralWidget());
#ifdef ENABLE_WALLET
    if(enableWallet) {
        connect(walletFrame, &WalletFrame::requestedSyncWarningInfo, this, &TalkcoinGUI::showModalOverlay);
        connect(labelBlocksIcon, &GUIUtil::ClickableLabel::clicked, this, &TalkcoinGUI::showModalOverlay);
        connect(progressBar, &GUIUtil::ClickableProgressBar::clicked, this, &TalkcoinGUI::showModalOverlay);
    }
#endif

#ifdef Q_OS_MAC
    m_app_nap_inhibitor = new CAppNapInhibitor;
#endif
}

TalkcoinGUI::~TalkcoinGUI()
{
    // Unsubscribe from notifications from core
    unsubscribeFromCoreSignals();

    QSettings settings;
    settings.setValue("MainWindowGeometry", saveGeometry());
    if(trayIcon) // Hide tray icon, as deleting will let it linger until quit (on Ubuntu)
        trayIcon->hide();
#ifdef Q_OS_MAC
    delete m_app_nap_inhibitor;
    delete appMenuBar;
    MacDockIconHandler::cleanup();
#endif

}

void TalkcoinGUI::createActions()
{
    QActionGroup *tabGroup = new QActionGroup(this);

#if defined(Q_OS_ANDROID)
    overviewAction = new QAction(platformStyle->SingleColorIcon(":/icons/overview"), nullptr, this);
#else
    overviewAction = new QAction(platformStyle->SingleColorIcon(":/icons/overview"), tr("&Overview"), this);
#endif
    overviewAction->setStatusTip(tr("Show general overview of wallet"));
    overviewAction->setToolTip(overviewAction->statusTip());
    overviewAction->setCheckable(true);
    overviewAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_1));
    tabGroup->addAction(overviewAction);

#if defined(Q_OS_ANDROID)
    sendCoinsAction = new QAction(platformStyle->SingleColorIcon(":/icons/send"), nullptr, this);
#else
    sendCoinsAction = new QAction(platformStyle->SingleColorIcon(":/icons/send"), tr("&Send"), this);
#endif
    sendCoinsAction->setStatusTip(tr("Send coins to a Talkcoin address"));
    sendCoinsAction->setToolTip(sendCoinsAction->statusTip());
    sendCoinsAction->setCheckable(true);
    sendCoinsAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_2));
    tabGroup->addAction(sendCoinsAction);

#if defined(Q_OS_ANDROID)
    sendCoinsMenuAction = new QAction(platformStyle->TextColorIcon(":/icons/send"), nullptr, this);
#else
    sendCoinsMenuAction = new QAction(platformStyle->TextColorIcon(":/icons/send"), sendCoinsAction->text(), this);
#endif
    sendCoinsMenuAction->setStatusTip(sendCoinsAction->statusTip());
    sendCoinsMenuAction->setToolTip(sendCoinsMenuAction->statusTip());

#if defined(Q_OS_ANDROID)
    receiveCoinsAction = new QAction(platformStyle->SingleColorIcon(":/icons/receiving_addresses"), nullptr, this);
#else
    receiveCoinsAction = new QAction(platformStyle->SingleColorIcon(":/icons/receiving_addresses"), tr("&Receive"), this);
#endif
    receiveCoinsAction->setStatusTip(tr("Request payments (generates QR codes and talkcoin: URIs)"));
    receiveCoinsAction->setToolTip(receiveCoinsAction->statusTip());
    receiveCoinsAction->setCheckable(true);
    receiveCoinsAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_3));
    tabGroup->addAction(receiveCoinsAction);

#if defined(Q_OS_ANDROID)
    receiveCoinsMenuAction = new QAction(platformStyle->TextColorIcon(":/icons/receiving_addresses"), nullptr, this);
#else
    receiveCoinsMenuAction = new QAction(platformStyle->TextColorIcon(":/icons/receiving_addresses"), receiveCoinsAction->text(), this);
#endif
    receiveCoinsMenuAction->setStatusTip(receiveCoinsAction->statusTip());
    receiveCoinsMenuAction->setToolTip(receiveCoinsMenuAction->statusTip());

#if defined(Q_OS_ANDROID)
    historyAction = new QAction(platformStyle->SingleColorIcon(":/icons/history"), nullptr, this);
#else
    historyAction = new QAction(platformStyle->SingleColorIcon(":/icons/history"), tr("&Transactions"), this);
#endif
    historyAction->setStatusTip(tr("Browse transaction history"));
    historyAction->setToolTip(historyAction->statusTip());
    historyAction->setCheckable(true);
    historyAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_4));
    tabGroup->addAction(historyAction);

#ifdef ENABLE_SECURE_MESSAGING
#if defined(Q_OS_ANDROID)
    sendMessagesAction = new QAction(platformStyle->SingleColorIcon(":/icons/chat"), nullptr, this);
#else
    sendMessagesAction = new QAction(platformStyle->SingleColorIcon(":/icons/chat"), tr("Send &Message"), this);
#endif
    sendMessagesAction->setCheckable(true);
    sendMessagesAction->setStatusTip(tr("Send Messages"));
    sendMessagesAction->setToolTip(sendMessagesAction->statusTip());
    tabGroup->addAction(sendMessagesAction);

#if defined(Q_OS_ANDROID)
    messageAction = new QAction(platformStyle->SingleColorIcon(":/icons/messages"), nullptr, this);
#else
    messageAction = new QAction(platformStyle->SingleColorIcon(":/icons/messages"), tr("Mess&ages"), this);
#endif
    messageAction->setStatusTip(tr("View Messages"));
    messageAction->setToolTip(messageAction->statusTip());
    messageAction->setCheckable(true);
    tabGroup->addAction(messageAction);
#endif

#ifdef ENABLE_WALLET
    // These showNormalIfMinimized are needed because Send Coins and Receive Coins
    // can be triggered from the tray menu, and need to show the GUI to be useful.
    connect(overviewAction, &QAction::triggered, [this]{ showNormalIfMinimized(); });
    connect(overviewAction, &QAction::triggered, this, &TalkcoinGUI::gotoOverviewPage);
    connect(sendCoinsAction, &QAction::triggered, [this]{ showNormalIfMinimized(); });
    connect(sendCoinsAction, &QAction::triggered, [this]{ gotoSendCoinsPage(); });
    connect(sendCoinsMenuAction, &QAction::triggered, [this]{ showNormalIfMinimized(); });
    connect(sendCoinsMenuAction, &QAction::triggered, [this]{ gotoSendCoinsPage(); });
    connect(receiveCoinsAction, &QAction::triggered, [this]{ showNormalIfMinimized(); });
    connect(receiveCoinsAction, &QAction::triggered, this, &TalkcoinGUI::gotoReceiveCoinsPage);
    connect(receiveCoinsMenuAction, &QAction::triggered, [this]{ showNormalIfMinimized(); });
    connect(receiveCoinsMenuAction, &QAction::triggered, this, &TalkcoinGUI::gotoReceiveCoinsPage);
    connect(historyAction, &QAction::triggered, [this]{ showNormalIfMinimized(); });
    connect(historyAction, &QAction::triggered, this, &TalkcoinGUI::gotoHistoryPage);
#ifdef ENABLE_SECURE_MESSAGING
    connect(sendMessagesAction, &QAction::triggered, [this]{ showNormalIfMinimized(); });
    connect(sendMessagesAction, &QAction::triggered, this, &TalkcoinGUI::gotoSendMessagesPage);
    connect(messageAction, &QAction::triggered, [this]{ showNormalIfMinimized(); });
    connect(messageAction, &QAction::triggered, this, &TalkcoinGUI::gotoMessagesPage);
#endif
#endif // ENABLE_WALLET

    quitAction = new QAction(platformStyle->TextColorIcon(":/icons/quit"), tr("E&xit"), this);
    quitAction->setStatusTip(tr("Quit application"));
    quitAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q));
    quitAction->setMenuRole(QAction::QuitRole);
    aboutAction = new QAction(platformStyle->TextColorIcon(":/icons/about"), tr("&About %1").arg(tr("Talkcoin")), this);
    aboutAction->setStatusTip(tr("Show information about %1").arg(tr("Talkcoin")));
    aboutAction->setMenuRole(QAction::AboutRole);
    aboutAction->setEnabled(false);
    aboutQtAction = new QAction(platformStyle->TextColorIcon(":/icons/about_qt"), tr("About &Qt"), this);
    aboutQtAction->setStatusTip(tr("Show information about Qt"));
    aboutQtAction->setMenuRole(QAction::AboutQtRole);
    optionsAction = new QAction(platformStyle->TextColorIcon(":/icons/options"), tr("&Options..."), this);
    optionsAction->setStatusTip(tr("Modify configuration options for %1").arg(tr("Talkcoin")));
    optionsAction->setMenuRole(QAction::PreferencesRole);
    optionsAction->setEnabled(false);
    optionsAction->setCheckable(true);
    toggleHideAction = new QAction(platformStyle->TextColorIcon(":/icons/about"), tr("&Show / Hide"), this);
    toggleHideAction->setStatusTip(tr("Show or hide the main Window"));

    encryptWalletAction = new QAction(platformStyle->TextColorIcon(":/icons/lock_closed"), tr("&Encrypt Wallet..."), this);
    encryptWalletAction->setStatusTip(tr("Encrypt the private keys that belong to your wallet"));
    encryptWalletAction->setCheckable(true);
    backupWalletAction = new QAction(platformStyle->TextColorIcon(":/icons/filesave"), tr("&Backup Wallet..."), this);
    backupWalletAction->setStatusTip(tr("Backup wallet to another location"));
    changePassphraseAction = new QAction(platformStyle->TextColorIcon(":/icons/key"), tr("&Change Passphrase..."), this);
    changePassphraseAction->setStatusTip(tr("Change the passphrase used for wallet encryption"));
    signMessageAction = new QAction(platformStyle->TextColorIcon(":/icons/edit"), tr("Sign &message..."), this);
    signMessageAction->setStatusTip(tr("Sign messages with your Talkcoin addresses to prove you own them"));
    verifyMessageAction = new QAction(platformStyle->TextColorIcon(":/icons/verify"), tr("&Verify message..."), this);
    verifyMessageAction->setStatusTip(tr("Verify messages to ensure they were signed with specified Talkcoin addresses"));

    unlockWalletAction = new QAction(platformStyle->TextColorIcon(":/icons/lock_open"), tr("&Unlock Wallet..."), this);
    unlockWalletAction->setToolTip(tr("Unlock wallet"));
    unlockWalletAction->setObjectName("unlockWalletAction");
    lockWalletAction = new QAction(platformStyle->TextColorIcon(":/icons/lock_closed"), tr("&Lock Wallet"), this);
    lockWalletAction->setToolTip(tr("Lock wallet"));


#if defined(Q_OS_ANDROID)
    openRPCConsoleAction = new QAction(platformStyle->SingleColorIcon(":/icons/debugwindow"), nullptr, this);
#else
    openRPCConsoleAction = new QAction(platformStyle->SingleColorIcon(":/icons/debugwindow"), tr("&Debug window"), this);
#endif
    openRPCConsoleAction->setStatusTip(tr("Open debugging and diagnostic console"));
    openRPCConsoleAction->setCheckable(true);
    // initially disable the debug window menu item
    openRPCConsoleAction->setObjectName("openRPCConsoleAction");

    usedSendingAddressesAction = new QAction(platformStyle->TextColorIcon(":/icons/address-book"), tr("&Sending addresses"), this);
    usedSendingAddressesAction->setStatusTip(tr("Show the list of used sending addresses and labels"));
    usedReceivingAddressesAction = new QAction(platformStyle->TextColorIcon(":/icons/address-book"), tr("&Receiving addresses"), this);
    usedReceivingAddressesAction->setStatusTip(tr("Show the list of used receiving addresses and labels"));

    openAction = new QAction(platformStyle->TextColorIcon(":/icons/open"), tr("Open &URI..."), this);
    openAction->setStatusTip(tr("Open a talkcoin: URI or payment request"));

    m_open_wallet_action = new QAction(tr("Open Wallet"), this);
    m_open_wallet_action->setEnabled(false);
    m_open_wallet_action->setStatusTip(tr("Open a wallet"));
    m_open_wallet_menu = new QMenu(this);

    m_close_wallet_action = new QAction(tr("Close Wallet..."), this);
    m_close_wallet_action->setStatusTip(tr("Close wallet"));

    showHelpMessageAction = new QAction(platformStyle->TextColorIcon(":/icons/info"), tr("&Command-line options"), this);
    showHelpMessageAction->setMenuRole(QAction::NoRole);
    showHelpMessageAction->setStatusTip(tr("Show the %1 help message to get a list with possible Talkcoin command-line options").arg(tr("Talkcoin")));

    connect(quitAction, &QAction::triggered, qApp, QApplication::quit);
    connect(aboutAction, &QAction::triggered, this, &TalkcoinGUI::aboutClicked);
    connect(aboutQtAction, &QAction::triggered, qApp, QApplication::aboutQt);
    connect(optionsAction, &QAction::triggered, this, &TalkcoinGUI::optionsClicked);
    connect(toggleHideAction, &QAction::triggered, this, &TalkcoinGUI::toggleHidden);
    connect(showHelpMessageAction, &QAction::triggered, this, &TalkcoinGUI::showHelpMessageClicked);
    connect(openRPCConsoleAction, &QAction::triggered, this, &TalkcoinGUI::showDebugWindow);
    // prevents an open debug window from becoming stuck/unusable on client shutdown

#ifdef ENABLE_WALLET
    if(walletFrame)
    {
        connect(encryptWalletAction, &QAction::triggered, walletFrame, &WalletFrame::encryptWallet);
        connect(backupWalletAction, &QAction::triggered, walletFrame, &WalletFrame::backupWallet);
        connect(changePassphraseAction, &QAction::triggered, walletFrame, &WalletFrame::changePassphrase);
        connect(unlockWalletAction, &QAction::triggered, walletFrame, &WalletFrame::unlockWallet);
        connect(lockWalletAction, &QAction::triggered, walletFrame, &WalletFrame::lockWallet);
        connect(signMessageAction, &QAction::triggered, [this]{ showNormalIfMinimized(); });
        connect(signMessageAction, &QAction::triggered, [this]{ gotoSignMessageTab(); });
        connect(verifyMessageAction, &QAction::triggered, [this]{ showNormalIfMinimized(); });
        connect(verifyMessageAction, &QAction::triggered, [this]{ gotoVerifyMessageTab(); });
        connect(usedSendingAddressesAction, &QAction::triggered, walletFrame, &WalletFrame::usedSendingAddresses);
        connect(usedReceivingAddressesAction, &QAction::triggered, walletFrame, &WalletFrame::usedReceivingAddresses);
        connect(openAction, &QAction::triggered, this, &TalkcoinGUI::openClicked);
        connect(m_open_wallet_menu, &QMenu::aboutToShow, [this] {
            m_open_wallet_menu->clear();
            for (const std::pair<const std::string, bool>& i : m_wallet_controller->listWalletDir()) {
                const std::string& path = i.first;
                QString name = path.empty() ? QString("["+tr("default wallet")+"]") : QString::fromStdString(path);
                QAction* action = m_open_wallet_menu->addAction(name);

                if (i.second) {
                    // This wallet is already loaded
                    action->setEnabled(false);
                    continue;
                }

                connect(action, &QAction::triggered, [this, name, path] {
                    OpenWalletActivity* activity = m_wallet_controller->openWallet(path);

                    QProgressDialog* dialog = new QProgressDialog(this);
                    dialog->setLabelText(tr("Opening Wallet <b>%1</b>...").arg(name.toHtmlEscaped()));
                    dialog->setRange(0, 0);
                    dialog->setCancelButton(nullptr);
                    dialog->setWindowModality(Qt::ApplicationModal);
                    dialog->show();

                    connect(activity, &OpenWalletActivity::message, this, [this] (QMessageBox::Icon icon, QString text) {
                        QMessageBox box;
                        box.setIcon(icon);
                        box.setText(tr("Open Wallet Failed"));
                        box.setInformativeText(text);
                        box.setStandardButtons(QMessageBox::Ok);
                        box.setDefaultButton(QMessageBox::Ok);
                        connect(this, &QObject::destroyed, &box, &QDialog::accept);
                        box.exec();
                    });
                    connect(activity, &OpenWalletActivity::opened, this, &TalkcoinGUI::setCurrentWallet);
                    connect(activity, &OpenWalletActivity::finished, activity, &QObject::deleteLater);
                    connect(activity, &OpenWalletActivity::finished, dialog, &QObject::deleteLater);
                    bool invoked = QMetaObject::invokeMethod(activity, "open");
                    assert(invoked);
                });
            }
            if (m_open_wallet_menu->isEmpty()) {
                QAction* action = m_open_wallet_menu->addAction(tr("No wallets available"));
                action->setEnabled(false);
            }
        });
        connect(m_close_wallet_action, &QAction::triggered, [this] {
            m_wallet_controller->closeWallet(walletFrame->currentWalletModel(), this);
        });
    }
#endif // ENABLE_WALLET

    connect(new QShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_C), this), &QShortcut::activated, this, &TalkcoinGUI::showDebugWindowActivateConsole);
    connect(new QShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_D), this), &QShortcut::activated, this, &TalkcoinGUI::showDebugWindow);
}

void TalkcoinGUI::createMenuBar()
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
        file->addAction(m_open_wallet_action);
        file->addAction(m_close_wallet_action);
        file->addSeparator();
        file->addAction(openAction);
        file->addAction(backupWalletAction);
        file->addAction(signMessageAction);
        file->addAction(verifyMessageAction);
        file->addSeparator();
        file->addAction(usedSendingAddressesAction);
        file->addAction(usedReceivingAddressesAction);
    }
    file->addAction(quitAction);

    QMenu *settings = appMenuBar->addMenu(tr("&Settings"));
    if(walletFrame)
    {
        settings->addAction(encryptWalletAction);
        settings->addAction(changePassphraseAction);
        settings->addAction(unlockWalletAction);
        settings->addAction(lockWalletAction);
        settings->addSeparator();
    }
    settings->addAction(optionsAction);

    QMenu* window_menu = appMenuBar->addMenu(tr("&Window"));

    QAction* minimize_action = window_menu->addAction(tr("Minimize"));
    minimize_action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_M));
    connect(minimize_action, &QAction::triggered, [] {
        qApp->focusWindow()->showMinimized();
    });
    connect(qApp, &QApplication::focusWindowChanged, [minimize_action] (QWindow* window) {
        minimize_action->setEnabled(window != nullptr && (window->flags() & Qt::Dialog) != Qt::Dialog && window->windowState() != Qt::WindowMinimized);
    });

#ifdef Q_OS_MAC
    QAction* zoom_action = window_menu->addAction(tr("Zoom"));
    connect(zoom_action, &QAction::triggered, [] {
        QWindow* window = qApp->focusWindow();
        if (window->windowState() != Qt::WindowMaximized) {
            window->showMaximized();
        } else {
            window->showNormal();
        }
    });

    connect(qApp, &QApplication::focusWindowChanged, [zoom_action] (QWindow* window) {
        zoom_action->setEnabled(window != nullptr);
    });
#else
    QAction* restore_action = window_menu->addAction(tr("Restore"));
    connect(restore_action, &QAction::triggered, [] {
        qApp->focusWindow()->showNormal();
    });

    connect(qApp, &QApplication::focusWindowChanged, [restore_action] (QWindow* window) {
        restore_action->setEnabled(window != nullptr);
    });
#endif

    if (walletFrame) {
        window_menu->addSeparator();
        QAction* main_window_action = window_menu->addAction(tr("Main Window"));
        connect(main_window_action, &QAction::triggered, [this] {
            GUIUtil::bringToFront(this);
        });

        window_menu->addSeparator();
        window_menu->addAction(usedSendingAddressesAction);
        window_menu->addAction(usedReceivingAddressesAction);
    }

    window_menu->addSeparator();

    QMenu *help = appMenuBar->addMenu(tr("&Help"));
    help->addAction(showHelpMessageAction);
    help->addSeparator();
    help->addAction(aboutAction);
    help->addAction(aboutQtAction);
}

void TalkcoinGUI::createToolBars()
{
    QWidget *spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QToolBar *toolbar = addToolBar(tr("Tabs toolbar"));
    if(walletFrame)
    {
        Logo = new QPushButton(QIcon(":icons/talkcoin"),"",nullptr);
        Logo->setObjectName("Logo");
        connect(Logo, SIGNAL(clicked()), this, SLOT(gotoOverviewPage()));
        appToolBar = toolbar;
        toolbar->setContextMenuPolicy(Qt::PreventContextMenu);
        toolbar->setMovable(false);
        toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        //toolbar->addWidget(Logo);
        toolbar->addAction(overviewAction);
        toolbar->addAction(sendCoinsAction);
        toolbar->addAction(receiveCoinsAction);
        toolbar->addAction(historyAction);
#ifdef ENABLE_SECURE_MESSAGING
        toolbar->addAction(messageAction);
//        toolbar->addAction(sendMessagesAction);
#endif
        overviewAction->setChecked(true);

#ifdef ENABLE_WALLET
        m_wallet_selector = new QComboBox();
        m_wallet_selector->setSizeAdjustPolicy(QComboBox::AdjustToContents);
        connect(m_wallet_selector, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &TalkcoinGUI::setCurrentWalletBySelectorIndex);

        m_wallet_selector_label = new QLabel();
        m_wallet_selector_label->setText(tr("Wallet:") + " ");
        m_wallet_selector_label->setBuddy(m_wallet_selector);

        m_wallet_selector_label_action = appToolBar->addWidget(m_wallet_selector_label);
        m_wallet_selector_action = appToolBar->addWidget(m_wallet_selector);

        m_wallet_selector_label_action->setVisible(false);
        m_wallet_selector_action->setVisible(false);
#endif
    }
//#if defined(Q_OS_ANDROID)
        toolbar->addAction(openRPCConsoleAction);
//#endif
    toolbar->setIconSize(QSize(30,30));
}

void TalkcoinGUI::setClientModel(ClientModel *_clientModel)
{
    this->clientModel = _clientModel;
    if(_clientModel)
    {
        // Create system tray menu (or setup the dock menu) that late to prevent users from calling actions,
        // while the client has not yet fully loaded
        createTrayIconMenu();

        // Keep up to date with client
        updateNetworkState();
        connect(_clientModel, &ClientModel::numConnectionsChanged, this, &TalkcoinGUI::setNumConnections);
        connect(_clientModel, &ClientModel::networkActiveChanged, this, &TalkcoinGUI::setNetworkActive);

        modalOverlay->setKnownBestHeight(_clientModel->getHeaderTipHeight(), QDateTime::fromTime_t(_clientModel->getHeaderTipTime()));
        setNumBlocks(m_node.getNumBlocks(), QDateTime::fromTime_t(m_node.getLastBlockTime()), m_node.getVerificationProgress(), false);
        connect(_clientModel, &ClientModel::numBlocksChanged, this, &TalkcoinGUI::setNumBlocks);

        // Receive and report messages from client model
        connect(_clientModel, &ClientModel::message, [this](const QString &title, const QString &message, unsigned int style){
            this->message(title, message, style);
        });

        // Show progress dialog
        connect(_clientModel, &ClientModel::showProgress, this, &TalkcoinGUI::showProgress);

        // Show auxiliary block request (SPV) progress
        connect(_clientModel, SIGNAL(auxiliaryBlockRequestProgressChanged(QDateTime,int,int,int)), this, SLOT(setAuxiliaryBlockRequestProgress(QDateTime,int,int,int)));

        // If we already have a auxiliary block request, update the progress immediately
        int64_t created;
        size_t requestedBlocks, loadedBlocks, processedBlocks;
        if (_clientModel->hasAuxiliaryBlockRequest(&created, &requestedBlocks, &loadedBlocks, &processedBlocks))
        {
            setAuxiliaryBlockRequestProgress(QDateTime::fromTime_t(created), requestedBlocks, loadedBlocks, processedBlocks);
        }

        updateProxyIcon();

#ifdef ENABLE_WALLET
        if(walletFrame)
        {
            walletFrame->setClientModel(_clientModel);
        }
#endif // ENABLE_WALLET
        unitDisplayControl->setOptionsModel(_clientModel->getOptionsModel());

        OptionsModel* optionsModel = _clientModel->getOptionsModel();
        if (optionsModel && trayIcon) {
            // be aware of the tray icon disable state change reported by the OptionsModel object.
            connect(optionsModel, &OptionsModel::hideTrayIconChanged, this, &TalkcoinGUI::setTrayIconVisible);

            // initialize the disable state of the tray icon with the current value in the model.
            setTrayIconVisible(optionsModel->getHideTrayIcon());
        }
    } else {
        // Disable possibility to show main window via action
        toggleHideAction->setEnabled(false);
        if(trayIconMenu)
        {
            // Disable context menu on tray icon
            trayIconMenu->clear();
        }
        // Propagate cleared model to child objects
#ifdef ENABLE_WALLET
        if (walletFrame)
        {
            walletFrame->setClientModel(nullptr);
        }
#endif // ENABLE_WALLET
        unitDisplayControl->setOptionsModel(nullptr);
    }
}

#ifdef ENABLE_WALLET
void TalkcoinGUI::setWalletController(WalletController* wallet_controller)
{
    assert(!m_wallet_controller);
    assert(wallet_controller);

    m_wallet_controller = wallet_controller;

    m_open_wallet_action->setEnabled(true);
    m_open_wallet_action->setMenu(m_open_wallet_menu);

    connect(wallet_controller, &WalletController::walletAdded, this, &TalkcoinGUI::addWallet);
    connect(wallet_controller, &WalletController::walletRemoved, this, &TalkcoinGUI::removeWallet);

    for (WalletModel* wallet_model : m_wallet_controller->getOpenWallets()) {
        addWallet(wallet_model);
    }
}

void TalkcoinGUI::addWallet(WalletModel* walletModel)
{
    if (!walletFrame) return;
    const QString display_name = walletModel->getDisplayName();
    setWalletActionsEnabled(true);

    walletFrame->addWallet(walletModel);
    m_wallet_selector->addItem(display_name, QVariant::fromValue(walletModel));
    if (m_wallet_selector->count() == 2) {
        m_wallet_selector_label_action->setVisible(true);
        m_wallet_selector_action->setVisible(true);
    }
}

void TalkcoinGUI::removeWallet(WalletModel* walletModel)
{
    if (!walletFrame) return;
    int index = m_wallet_selector->findData(QVariant::fromValue(walletModel));
    m_wallet_selector->removeItem(index);
    if (m_wallet_selector->count() == 0) {
        setWalletActionsEnabled(false);
    } else if (m_wallet_selector->count() == 1) {
        m_wallet_selector_label_action->setVisible(false);
        m_wallet_selector_action->setVisible(false);
    }

    walletFrame->removeWallet(walletModel);
    updateWindowTitle();
}

void TalkcoinGUI::setCurrentWallet(WalletModel* wallet_model)
{
    if (!walletFrame) return;
    walletFrame->setCurrentWallet(wallet_model);
    for (int index = 0; index < m_wallet_selector->count(); ++index) {
        if (m_wallet_selector->itemData(index).value<WalletModel*>() == wallet_model) {
            m_wallet_selector->setCurrentIndex(index);
            break;
        }
    }
    updateWindowTitle();
}

void TalkcoinGUI::setCurrentWalletBySelectorIndex(int index)
{
    WalletModel* wallet_model = m_wallet_selector->itemData(index).value<WalletModel*>();
    if (wallet_model) setCurrentWallet(wallet_model);
}

void TalkcoinGUI::removeAllWallets()
{
    if(!walletFrame)
        return;
    setWalletActionsEnabled(false);
    walletFrame->removeAllWallets();
}
#endif // ENABLE_WALLET

void TalkcoinGUI::setWalletActionsEnabled(bool enabled)
{
    overviewAction->setEnabled(enabled);
    sendCoinsAction->setEnabled(enabled);
    sendCoinsMenuAction->setEnabled(enabled);
    receiveCoinsAction->setEnabled(enabled);
    receiveCoinsMenuAction->setEnabled(enabled);
    historyAction->setEnabled(enabled);
    encryptWalletAction->setEnabled(enabled);
    backupWalletAction->setEnabled(enabled);
    changePassphraseAction->setEnabled(enabled);
    signMessageAction->setEnabled(enabled);
    verifyMessageAction->setEnabled(enabled);
    usedSendingAddressesAction->setEnabled(enabled);
    usedReceivingAddressesAction->setEnabled(enabled);
    openAction->setEnabled(enabled);
#ifdef ENABLE_SECURE_MESSAGING
    sendMessagesAction->setEnabled(enabled);
    messageAction->setEnabled(enabled);
#endif
    unlockWalletAction->setEnabled(enabled);
    lockWalletAction->setEnabled(enabled);
    m_close_wallet_action->setEnabled(enabled);
    openRPCConsoleAction->setEnabled(enabled);

}

void TalkcoinGUI::createTrayIcon()
{
    assert(QSystemTrayIcon::isSystemTrayAvailable());

#ifndef Q_OS_MAC
    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        trayIcon = new QSystemTrayIcon(m_network_style->getTrayAndWindowIcon(), this);
        QString toolTip = tr("%1 client").arg("Talkcoin") + " " + m_network_style->getTitleAddText();
        trayIcon->setToolTip(toolTip);
    }
#endif
}

void TalkcoinGUI::createTrayIconMenu()
{
#ifndef Q_OS_MAC
    // return if trayIcon is unset (only on non-macOSes)
    if (!trayIcon)
        return;

    trayIcon->setContextMenu(trayIconMenu.get());
    connect(trayIcon, &QSystemTrayIcon::activated, this, &TalkcoinGUI::trayIconActivated);
#else
    // Note: On macOS, the Dock icon is used to provide the tray's functionality.
    MacDockIconHandler *dockIconHandler = MacDockIconHandler::instance();
    connect(dockIconHandler, &MacDockIconHandler::dockIconClicked, this, &TalkcoinGUI::macosDockIconActivated);
    trayIconMenu->setAsDockMenu();
#endif

    // Configuration of the tray icon (or Dock icon) menu
#ifndef Q_OS_MAC
    // Note: On macOS, the Dock icon's menu already has Show / Hide action.
    trayIconMenu->addAction(toggleHideAction);
    trayIconMenu->addSeparator();
#endif
    if (enableWallet) {
        trayIconMenu->addAction(sendCoinsMenuAction);
        trayIconMenu->addAction(receiveCoinsMenuAction);
        trayIconMenu->addSeparator();
        trayIconMenu->addAction(signMessageAction);
        trayIconMenu->addAction(verifyMessageAction);
        trayIconMenu->addSeparator();
        trayIconMenu->addAction(openRPCConsoleAction);
    }
    trayIconMenu->addAction(optionsAction);
#ifndef Q_OS_MAC // This is built-in on macOS
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(quitAction);
#endif
}

#ifndef Q_OS_MAC
void TalkcoinGUI::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if(reason == QSystemTrayIcon::Trigger)
    {
        // Click on system tray icon triggers show/hide of the main window
        toggleHidden();
    }
}
#else
void TalkcoinGUI::macosDockIconActivated()
{
    show();
    activateWindow();
}
#endif

void TalkcoinGUI::optionsClicked()
{
    openOptionsDialogWithTab(OptionsDialog::TAB_MAIN);
}

void TalkcoinGUI::aboutClicked()
{
    if(!clientModel)
        return;

    HelpMessageDialog dlg(m_node, this, true);
    dlg.exec();
}

void TalkcoinGUI::showDebugWindow()
{
    openRPCConsoleAction->setChecked(true);
    if (walletFrame) walletFrame->gotoRpcPage();
}

void TalkcoinGUI::showDebugWindowActivateConsole()
{
    showDebugWindow();
}

void TalkcoinGUI::showHelpMessageClicked()
{
    helpMessageDialog->show();
}

#ifdef ENABLE_WALLET
void TalkcoinGUI::openClicked()
{
    OpenURIDialog dlg(this);
    if(dlg.exec())
    {
        Q_EMIT receivedURI(dlg.getURI());
    }
}
#ifdef ENABLE_SECURE_MESSAGING
void TalkcoinGUI::gotoSendMessagesPage()
{
    sendMessagesAction->setChecked(true);
    if (walletFrame) walletFrame->gotoSendMessagesPage();
}

void TalkcoinGUI::gotoMessagesPage()
{
    messageAction->setChecked(true);
    if (walletFrame) walletFrame->gotoMessagesPage();
}
#endif

void TalkcoinGUI::gotoOverviewPage()
{
    overviewAction->setChecked(true);
    if (walletFrame) walletFrame->gotoOverviewPage();
}

void TalkcoinGUI::gotoHistoryPage()
{
    historyAction->setChecked(true);
    if (walletFrame) walletFrame->gotoHistoryPage();
}

void TalkcoinGUI::gotoReceiveCoinsPage()
{
    receiveCoinsAction->setChecked(true);
    if (walletFrame) walletFrame->gotoReceiveCoinsPage();
}

void TalkcoinGUI::gotoSendCoinsPage(QString addr)
{
    sendCoinsAction->setChecked(true);
    if (walletFrame) walletFrame->gotoSendCoinsPage(addr);
}

void TalkcoinGUI::gotoSignMessageTab(QString addr)
{
    if (walletFrame) walletFrame->gotoSignMessageTab(addr);
}

void TalkcoinGUI::gotoVerifyMessageTab(QString addr)
{
    if (walletFrame) walletFrame->gotoVerifyMessageTab(addr);
}
#endif // ENABLE_WALLET

void TalkcoinGUI::updateNetworkState()
{
    int count = clientModel->getNumConnections();
    QString icon;
    switch(count)
    {
    case 0: icon = ":/icons/connect_0"; break;
    case 1: case 2: case 3: icon = ":/icons/connect_1"; break;
    case 4: case 5: case 6: icon = ":/icons/connect_2"; break;
    case 7: case 8: case 9: icon = ":/icons/connect_3"; break;
    default: icon = ":/icons/connect_4"; break;
    }

    QString tooltip;

    if (m_node.getNetworkActive()) {
        tooltip = tr("%n active connection(s) to Talkcoin network", "", count) + QString(".<br>") + tr("Click to disable network activity.");
    } else {
        tooltip = tr("Network activity disabled.") + QString("<br>") + tr("Click to enable network activity again.");
        icon = ":/icons/network_disabled";
    }

    // Don't word-wrap this (fixed-width) tooltip
    tooltip = QString("<nobr>") + tooltip + QString("</nobr>");
    connectionsControl->setToolTip(tooltip);

    connectionsControl->setPixmap(platformStyle->SingleColorIcon(icon).pixmap(STATUSBAR_ICONSIZE,STATUSBAR_ICONSIZE));
}

void TalkcoinGUI::setNumConnections(int count)
{
    updateNetworkState();
}

void TalkcoinGUI::setNetworkActive(bool networkActive)
{
    updateNetworkState();
}

void TalkcoinGUI::updateHeadersSyncProgressLabel()
{
    int64_t headersTipTime = clientModel->getHeaderTipTime();
    int headersTipHeight = clientModel->getHeaderTipHeight();
    int estHeadersLeft = (GetTime() - headersTipTime) / Params().GetConsensus().nPowTargetSpacing;
    if (estHeadersLeft > HEADER_HEIGHT_DELTA_SYNC)
        progressBarLabel->setText(tr("Syncing Headers (%1%)...").arg(QString::number(100.0 / (headersTipHeight+estHeadersLeft)*headersTipHeight, 'f', 1)));
}

void TalkcoinGUI::openOptionsDialogWithTab(OptionsDialog::Tab tab)
{
    if (!clientModel || !clientModel->getOptionsModel())
        return;

    OptionsDialog dlg(this, enableWallet);
    dlg.setCurrentTab(tab);
    dlg.setModel(clientModel->getOptionsModel());
    dlg.exec();
}

void TalkcoinGUI::setNumBlocks(int count, const QDateTime& blockDate, double nVerificationProgress, bool header)
{
// Disabling macOS App Nap on initial sync, disk and reindex operations.
#ifdef Q_OS_MAC
    (m_node.isInitialBlockDownload() || m_node.getReindex() || m_node.getImporting()) ? m_app_nap_inhibitor->disableAppNap() : m_app_nap_inhibitor->enableAppNap();
#endif

    if (modalOverlay)
    {
        if (header)
            modalOverlay->setKnownBestHeight(count, blockDate);
        else
            modalOverlay->tipUpdate(count, blockDate, nVerificationProgress);
    }
    if (!clientModel)
        return;
    if (clientModel->hasAuxiliaryBlockRequest())
        return;

    // Prevent orphan statusbar messages (e.g. hover Quit in main menu, wait until chain-sync starts -> garbled text)

    // Acquire current block source
//    bool headerSyncInProgress = false;
    enum BlockSource blockSource = clientModel->getBlockSource();
    switch (blockSource) {
        case BlockSource::NETWORK:
            if (header) {
                updateHeadersSyncProgressLabel();
                return;
            }
            progressBarLabel->setText(tr("Synchronizing with network..."));
            updateHeadersSyncProgressLabel();
            break;
        case BlockSource::DISK:
            if (header) {
                progressBarLabel->setText(tr("Indexing blocks on disk..."));
            } else {
                progressBarLabel->setText(tr("Processing blocks on disk..."));
            }
            break;
        case BlockSource::REINDEX:
            progressBarLabel->setText(tr("Reindexing blocks on disk..."));
            break;
        case BlockSource::NONE:
            if (header) {
                return;
            }
            progressBarLabel->setText(tr("Connecting ..."));
            break;
    }

    QString tooltip;

    QDateTime currentDate = QDateTime::currentDateTime();
    qint64 secs = blockDate.secsTo(currentDate);

    tooltip = tr("Processed %n block(s) of transaction history.", "", count);

    // Set icon state: spinning if catching up, tick otherwise
    if (secs < MAX_BLOCK_TIME_GAP) {
        tooltip = tr("Up to date") + QString(".<br>") + tooltip;
        labelBlocksIcon->setPixmap(platformStyle->SingleColorIcon(":/icons/synced").pixmap(STATUSBAR_ICONSIZE, STATUSBAR_ICONSIZE));

#ifdef ENABLE_WALLET
        if(walletFrame)
        {
            walletFrame->showOutOfSyncWarning(false);
            modalOverlay->showHide(true, true);
        }
#endif // ENABLE_WALLET

        progressBarLabel->setVisible(false);
        progressBar->setVisible(false);
    }
    else
    {
        QString timeBehindText = GUIUtil::formatNiceTimeOffset(secs);

        progressBarLabel->setVisible(true);
        progressBar->setFormat(tr("%1 ").arg(timeBehindText));
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
        {
            walletFrame->showOutOfSyncWarning(true);
            if (!walletFrame->getSPVMode())
                modalOverlay->showHide();
        }
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

void TalkcoinGUI::setAuxiliaryBlockRequestProgress(const QDateTime& blockDate, int requestesBlocks, int loadedBlocks, int processedBlocks)
{
    // at this stage, always display the progress bar and it's label
    progressBar->setVisible(true);
    progressBarLabel->setVisible(true);

    // if we are not yet connected to some peers, show different text
    if (clientModel->getNumConnections() == 0)
    {
        progressBarLabel->setText(tr("Connecting to peers..."));
        return;
    }

    QString tooltip = tr("Scanning %1 blocks...").arg(QString::number(requestesBlocks));
    tooltip += QString("<br>");
    tooltip += tr("%1 blocks loaded.").arg(QString::number(loadedBlocks));
    tooltip += QString("<br>");
    tooltip += tr("%1 blocks processed.").arg(QString::number(processedBlocks));
    tooltip = QString("<nobr>") + tooltip + QString("</nobr>");

    double nABRprogress = 1.0/requestesBlocks*loadedBlocks;
    progressBar->setFormat(tr("behind"));
    progressBar->setMaximum(1000000000);
    progressBar->setValue(nABRprogress  * 1000000000.0 + 0.5);
    progressBarLabel->setText("Download & scanning blocks (SPV)...");
    progressBar->setToolTip(tooltip);
    progressBarLabel->setToolTip(tooltip);
    if (nABRprogress == 1)
    {
        // don't show progress if request has been completed
        progressBar->setVisible(false);
        progressBarLabel->setVisible(false);
    }
}

void TalkcoinGUI::message(const QString& title, QString message, unsigned int style, bool* ret)
{
    // Default title. On macOS, the window title is ignored (as required by the macOS Guidelines).
    QString strTitle{"Talkcoin"};
    // Default to information icon
    int nMBoxIcon = QMessageBox::Information;
    int nNotifyIcon = Notificator::Information;

    bool prefix = !(style & CClientUIInterface::MSG_NOPREFIX);
    style &= ~CClientUIInterface::MSG_NOPREFIX;

    QString msgType;
    if (!title.isEmpty()) {
        msgType = title;
    } else {
        switch (style) {
        case CClientUIInterface::MSG_ERROR:
            msgType = tr("Error");
            if (prefix) message = tr("Error: %1").arg(message);
            break;
        case CClientUIInterface::MSG_WARNING:
            msgType = tr("Warning");
            if (prefix) message = tr("Warning: %1").arg(message);
            break;
        case CClientUIInterface::MSG_INFORMATION:
            msgType = tr("Information");
            // No need to prepend the prefix here.
            break;
        default:
            break;
        }
    }

    if (!msgType.isEmpty()) {
        strTitle += " - " + msgType;
    }

    if (style & CClientUIInterface::ICON_ERROR) {
        nMBoxIcon = QMessageBox::Critical;
        nNotifyIcon = Notificator::Critical;
    } else if (style & CClientUIInterface::ICON_WARNING) {
        nMBoxIcon = QMessageBox::Warning;
        nNotifyIcon = Notificator::Warning;
    }

    if (style & CClientUIInterface::MODAL) {
        // Check for buttons, use OK as default, if none was supplied
        QMessageBox::StandardButton buttons;
        if (!(buttons = (QMessageBox::StandardButton)(style & CClientUIInterface::BTN_MASK)))
            buttons = QMessageBox::Ok;

        showNormalIfMinimized();
        QMessageBox mBox(static_cast<QMessageBox::Icon>(nMBoxIcon), strTitle, message, buttons, this);
        mBox.setTextFormat(Qt::PlainText);
        int r = mBox.exec();
        if (ret != nullptr)
            *ret = r == QMessageBox::Ok;
    } else {
        notificator->notify(static_cast<Notificator::Class>(nNotifyIcon), strTitle, message);
    }
}

void TalkcoinGUI::changeEvent(QEvent *e)
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
                QTimer::singleShot(0, this, &TalkcoinGUI::hide);
                e->ignore();
            }
            else if((wsevt->oldState() & Qt::WindowMinimized) && !isMinimized())
            {
                QTimer::singleShot(0, this, &TalkcoinGUI::show);
                e->ignore();
            }
        }
    }
#endif
}

void TalkcoinGUI::closeEvent(QCloseEvent *event)
{
    if (isMiningEngaged)
        engageDisengageMining(0);
#ifndef Q_OS_MAC // Ignored on Mac
    if(clientModel && clientModel->getOptionsModel())
    {
        if(!clientModel->getOptionsModel()->getMinimizeOnClose())
        {
            // close rpcConsole in case it was open to make some space for the shutdown window

            QApplication::quit();
        }
        else
        {
            QMainWindow::showMinimized();
            event->ignore();
        }
    }
#else
    QMainWindow::closeEvent(event);
#endif
}

void TalkcoinGUI::showEvent(QShowEvent *event)
{
    // enable the debug window when the main window shows up
    openRPCConsoleAction->setEnabled(true);
    aboutAction->setEnabled(true);
    optionsAction->setEnabled(true);
}

#ifdef ENABLE_WALLET
void TalkcoinGUI::incomingTransaction(const QString& date, int unit, const CAmount& amount, const QString& type, const QString& address, const QString& label, const QString& walletName)
{
    // On new transaction, make an info balloon
    QString msg = tr("Date: %1\n").arg(date) +
                  tr("Amount: %1\n").arg(TalkcoinUnits::formatWithUnit(unit, amount, true));
    if (m_node.getWallets().size() > 1 && !walletName.isEmpty()) {
        msg += tr("Wallet: %1\n").arg(walletName);
    }
    msg += tr("Type: %1\n").arg(type);
    if (!label.isEmpty())
        msg += tr("Label: %1\n").arg(label);
    else if (!address.isEmpty())
        msg += tr("Address: %1\n").arg(address);
    message((amount)<0 ? tr("Sent transaction") : tr("Incoming transaction"),
             msg, CClientUIInterface::MSG_INFORMATION);
}
#ifdef ENABLE_SECURE_MESSAGING
void TalkcoinGUI::incomingMessage(const QString& sent_datetime, QString from_address, QString to_address, QString message, int type)
{
    // Prevent balloon-spam when initial block download is in progress

    if (type == MessageTableEntry::Received)
    {
        notificator->notify(Notificator::Information,
                            tr("Incoming Message"),
                            tr("Date: %1\n"
                               "From Address: %2\n"
                               "To Address: %3\n"
                               "Message: %4\n")
                              .arg(sent_datetime)
                              .arg(from_address)
                              .arg(to_address)
                              .arg(message));
    };
}
#endif
#endif // ENABLE_WALLET

void TalkcoinGUI::dragEnterEvent(QDragEnterEvent *event)
{
    // Accept only URIs
    if(event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void TalkcoinGUI::dropEvent(QDropEvent *event)
{
    if(event->mimeData()->hasUrls())
    {
        for (const QUrl &uri : event->mimeData()->urls())
        {
            Q_EMIT receivedURI(uri.toString());
        }
    }
    event->acceptProposedAction();
}

bool TalkcoinGUI::eventFilter(QObject *object, QEvent *event)
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

#ifdef ENABLE_WALLET
bool TalkcoinGUI::handlePaymentRequest(const SendCoinsRecipient& recipient)
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

void TalkcoinGUI::setHDStatus(bool privkeyDisabled, int hdEnabled)
{
    labelWalletHDStatusIcon->setPixmap(platformStyle->SingleColorIcon(privkeyDisabled ? ":/icons/eye" : hdEnabled ? ":/icons/hd_enabled" : ":/icons/hd_disabled").pixmap(STATUSBAR_ICONSIZE,STATUSBAR_ICONSIZE));
    labelWalletHDStatusIcon->setToolTip(privkeyDisabled ? tr("Private key <b>disabled</b>") : hdEnabled ? tr("HD key generation is <b>enabled</b>") : tr("HD key generation is <b>disabled</b>"));

    // eventually disable the QLabel to set its opacity to 50%
    labelWalletHDStatusIcon->setEnabled(hdEnabled);
}

void TalkcoinGUI::setSPVStatus(int spvEnabled)
{
    labelWalletSPVStatusIcon->setPixmap(platformStyle->SingleColorIcon(spvEnabled ? ":/icons/spv_enabled" : ":/icons/spv_disabled").pixmap(STATUSBAR_ICONSIZE,STATUSBAR_ICONSIZE));
    labelWalletSPVStatusIcon->setToolTip(spvEnabled ? tr("Simple Payment Verification is <b>enabled</b>") : tr("Simple Payment Verification is <b>disabled</b>"));
    if(spvEnabled)
        modalOverlay->showHide(true, false);
}

void TalkcoinGUI::setEncryptionStatus(int status)
{
    WalletView * const walletView = walletFrame->currentWalletView();
    WalletModel * const walletModel = walletView->getWalletModel();
    switch(status)
    {
    case WalletModel::Unencrypted:
        labelWalletEncryptionIcon->hide();
        encryptWalletAction->setChecked(false);
        changePassphraseAction->setEnabled(false);
#ifdef ENABLE_PROOF_OF_STAKE
        unlockWalletAction->setVisible(false);
        lockWalletAction->setVisible(false);
#endif
        encryptWalletAction->setEnabled(true);
        break;
    case WalletModel::Unlocked:
        labelWalletEncryptionIcon->show();
#ifdef ENABLE_PROOF_OF_STAKE
        if(walletModel->getWalletUnlockStakingOnly())
        {
            labelWalletEncryptionIcon->setPixmap(platformStyle->SingleColorIcon(":/icons/lock_staking").pixmap(STATUSBAR_ICONSIZE,STATUSBAR_ICONSIZE));
            labelWalletEncryptionIcon->setToolTip(tr("Wallet is <b>encrypted</b> and currently <b>unlocked for staking only</b>"));
        }
        else
        {
            labelWalletEncryptionIcon->setPixmap(platformStyle->SingleColorIcon(":/icons/lock_open").pixmap(STATUSBAR_ICONSIZE,STATUSBAR_ICONSIZE));
            labelWalletEncryptionIcon->setToolTip(tr("Wallet is <b>encrypted</b> and currently <b>unlocked</b>"));
        }
        unlockWalletAction->setVisible(false);
        lockWalletAction->setVisible(true);
#else
        labelWalletEncryptionIcon->setPixmap(platformStyle->SingleColorIcon(":/icons/lock_open").pixmap(STATUSBAR_ICONSIZE,STATUSBAR_ICONSIZE));
        labelWalletEncryptionIcon->setToolTip(tr("Wallet is <b>encrypted</b> and currently <b>unlocked</b>"));
#endif
        encryptWalletAction->setChecked(true);
        changePassphraseAction->setEnabled(true);
        encryptWalletAction->setEnabled(false); // TODO: decrypt currently not supported
        break;
    case WalletModel::Locked:
        labelWalletEncryptionIcon->show();
        labelWalletEncryptionIcon->setPixmap(platformStyle->SingleColorIcon(":/icons/lock_closed").pixmap(STATUSBAR_ICONSIZE,STATUSBAR_ICONSIZE));
        labelWalletEncryptionIcon->setToolTip(tr("Wallet is <b>encrypted</b> and currently <b>locked</b>"));
        encryptWalletAction->setChecked(true);
        changePassphraseAction->setEnabled(true);
#ifdef ENABLE_PROOF_OF_STAKE
        unlockWalletAction->setVisible(true);
        lockWalletAction->setVisible(false);
#endif
        encryptWalletAction->setEnabled(false); // TODO: decrypt currently not supported
        break;
    }
}

void TalkcoinGUI::updateWalletStatus()
{
    if (!walletFrame) {
        return;
    }
    WalletView * const walletView = walletFrame->currentWalletView();
    if (!walletView) {
        return;
    }
    WalletModel * const walletModel = walletView->getWalletModel();
    setEncryptionStatus(walletModel->getEncryptionStatus());
    setHDStatus(walletModel->privateKeysDisabled(), walletModel->wallet().hdEnabled());
    setSPVStatus(walletModel->spvEnabled());
}

void TalkcoinGUI::toggleSPVMode()
{
    if (walletFrame)
        walletFrame->setSPVMode(!walletFrame->getSPVMode());
}

#endif // ENABLE_WALLET

void TalkcoinGUI::updateProxyIcon()
{
    std::string ip_port;
    bool proxy_enabled = clientModel->getProxyInfo(ip_port);

    if (proxy_enabled) {
        if (labelProxyIcon->pixmap() == nullptr) {
            QString ip_port_q = QString::fromStdString(ip_port);
            labelProxyIcon->setPixmap(platformStyle->SingleColorIcon(":/icons/proxy").pixmap(STATUSBAR_ICONSIZE, STATUSBAR_ICONSIZE));
            labelProxyIcon->setToolTip(tr("Proxy is <b>enabled</b>: %1").arg(ip_port_q));
        } else {
            labelProxyIcon->show();
        }
    } else {
        labelProxyIcon->hide();
    }
}

void TalkcoinGUI::updateWindowTitle()
{
    QString window_title = "Talkcoin";
#ifdef ENABLE_WALLET
    if (walletFrame) {
        WalletModel* const wallet_model = walletFrame->currentWalletModel();
        if (wallet_model && !wallet_model->getWalletName().isEmpty()) {
            window_title += " - " + wallet_model->getDisplayName();
        }
    }
#endif
    if (!m_network_style->getTitleAddText().isEmpty()) {
        window_title += " - " + m_network_style->getTitleAddText();
    }
    setWindowTitle(window_title);
}

void TalkcoinGUI::showNormalIfMinimized(bool fToggleHidden)
{
    if(!clientModel)
        return;

    if (!isHidden() && !isMinimized() && !GUIUtil::isObscured(this) && fToggleHidden) {
        hide();
    } else {
        GUIUtil::bringToFront(this);
    }
}

void TalkcoinGUI::toggleHidden()
{
    showNormalIfMinimized(true);
}

void TalkcoinGUI::updateStakingIcon()
{
    if(m_node.shutdownRequested())
        return;

    if (!walletFrame) {
        return;
    }
    WalletView * const walletView = walletFrame->currentWalletView();
    if (!walletView) {
        return;
    }
    WalletModel * const walletModel = walletView->getWalletModel();

    uint64_t nWeight= walletModel->getStakeWeight();
    if (walletModel->wallet().getLastCoinStakeSearchInterval() && nWeight)
    {
        uint64_t nNetworkWeight = GetPoSKernelPS();
        const Consensus::Params& consensusParams = Params().GetConsensus();
        int64_t nTargetSpacing = consensusParams.nPowTargetSpacing;

        unsigned nEstimateTime = nTargetSpacing * nNetworkWeight / nWeight;

        QString text;
        if (nEstimateTime < 60)
        {
            text = tr("%n second(s)", "", nEstimateTime);
        }
        else if (nEstimateTime < 60*60)
        {
            text = tr("%n minute(s)", "", nEstimateTime/60);
        }
        else if (nEstimateTime < 24*60*60)
        {
            text = tr("%n hour(s)", "", nEstimateTime/(60*60));
        }
        else
        {
            text = tr("%n day(s)", "", nEstimateTime/(60*60*24));
        }

        nWeight /= COIN;
        nNetworkWeight /= COIN;

        labelStakingIcon->setPixmap(QIcon(":/icons/staking_on").pixmap(STATUSBAR_ICONSIZE, STATUSBAR_ICONSIZE));
        labelStakingIcon->setToolTip(tr("Staking.<br>Your weight is %1<br>Network weight is %2<br>Expected time to earn reward is %3").arg(nWeight).arg(nNetworkWeight).arg(text));
    }
    else
    {
        labelStakingIcon->setPixmap(QIcon(":/icons/staking_off").pixmap(STATUSBAR_ICONSIZE, STATUSBAR_ICONSIZE));

        if (m_node.getNodeCount(CConnman::CONNECTIONS_ALL) == 0)
            labelStakingIcon->setToolTip(tr("Not staking because wallet is offline"));
        else if (m_node.isInitialBlockDownload())
            labelStakingIcon->setToolTip(tr("Not staking because wallet is syncing"));
        else if (!nWeight)
            labelStakingIcon->setToolTip(tr("Not staking because you don't have mature coins"));
        else if (walletModel->wallet().isLocked())
            labelStakingIcon->setToolTip(tr("Not staking because wallet is locked"));
        else
            labelStakingIcon->setToolTip(tr("Not staking"));
    }
}

void TalkcoinGUI::detectShutdown()
{
    if (m_node.shutdownRequested())
    {
        if (isMiningEngaged)
            engageDisengageMining(0);
        

        qApp->quit();
    }
}

void TalkcoinGUI::showProgress(const QString &title, int nProgress)
{
    if (nProgress == 0) {
        progressDialog = new QProgressDialog(title, QString(), 0, 100);
        GUIUtil::PolishProgressDialog(progressDialog);
        progressDialog->setWindowModality(Qt::ApplicationModal);
        progressDialog->setMinimumDuration(0);
        progressDialog->setAutoClose(false);
        progressDialog->setValue(0);
    } else if (nProgress == 100) {
        if (progressDialog) {
            progressDialog->close();
            progressDialog->deleteLater();
            progressDialog = nullptr;
        }
    } else if (progressDialog) {
        progressDialog->setValue(nProgress);
    }
}

void TalkcoinGUI::setTrayIconVisible(bool fHideTrayIcon)
{
    if (trayIcon)
    {
        trayIcon->setVisible(!fHideTrayIcon);
    }
}

void TalkcoinGUI::showModalOverlay()
{
    if (modalOverlay && (progressBar->isVisible() || modalOverlay->isLayerVisible()))
        modalOverlay->toggleVisibility();
}

static bool ThreadSafeMessageBox(TalkcoinGUI* gui, const std::string& message, const std::string& caption, unsigned int style)
{
    bool modal = (style & CClientUIInterface::MODAL);
    // The SECURE flag has no effect in the Qt GUI.
    // bool secure = (style & CClientUIInterface::SECURE);
    style &= ~CClientUIInterface::SECURE;
    bool ret = false;
    // In case of modal message, use blocking connection to wait for user to click a button
    bool invoked = QMetaObject::invokeMethod(gui, "message",
                               modal ? GUIUtil::blockingGUIThreadConnection() : Qt::QueuedConnection,
                               Q_ARG(QString, QString::fromStdString(caption)),
                               Q_ARG(QString, QString::fromStdString(message)),
                               Q_ARG(unsigned int, style),
                               Q_ARG(bool*, &ret));
    assert(invoked);
    return ret;
}

void TalkcoinGUI::subscribeToCoreSignals()
{
    // Connect signals to client
    m_handler_message_box = m_node.handleMessageBox(std::bind(ThreadSafeMessageBox, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    m_handler_question = m_node.handleQuestion(std::bind(ThreadSafeMessageBox, this, std::placeholders::_1, std::placeholders::_3, std::placeholders::_4));
}

void TalkcoinGUI::unsubscribeFromCoreSignals()
{
    // Disconnect signals from client
    m_handler_message_box->disconnect();
    m_handler_question->disconnect();
}
#ifndef OS_ANDROID
void TalkcoinGUI::mousePressEvent(QMouseEvent *event)
{
    m_nMouseClick_X_Coordinate = event->x();
    m_nMouseClick_Y_Coordinate = event->y();
}

void TalkcoinGUI::mouseMoveEvent(QMouseEvent *event)
{
    move(event->globalX() - m_nMouseClick_X_Coordinate, event->globalY() - m_nMouseClick_Y_Coordinate);
}
#endif
void TalkcoinGUI::engageDisengageMining(int cores)
{
    WalletView * const walletView = walletFrame->currentWalletView();
    WalletModel * const walletModel = walletView->getWalletModel();
    std::shared_ptr<CTxDestination> coinbase_script;
    walletModel->getScriptForMining(coinbase_script);

    isMiningEngaged = false;
    if (cores > 0)
        isMiningEngaged = true;

    //pwallet->GetScriptForMining(coinbase_script);
    GenerateTalkcoins(isMiningEngaged, cores, coinbase_script);

}

UnitDisplayStatusBarControl::UnitDisplayStatusBarControl(const PlatformStyle *platformStyle) :
    optionsModel(nullptr),
    menu(nullptr)
{
    createContextMenu();
    setToolTip(tr("Unit to show amounts in. Click to select another unit."));
    QList<TalkcoinUnits::Unit> units = TalkcoinUnits::availableUnits();
//    QList<TalkcoinUnits::Unit> units = TalkcoinUnits::availableUnits();
//    int max_width = 0;
    //for (const TalkcoinUnits::Unit unit : units)
    //{
    //    max_width = qMax(max_width, fm.width(TalkcoinUnits::longName(unit)));
    //}
    //setMinimumSize(max_width, 0);
    setAlignment(Qt::AlignVCenter);
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
    menu = new QMenu(this);
    for (const TalkcoinUnits::Unit u : TalkcoinUnits::availableUnits())
    {
        QAction *menuAction = new QAction(QString(TalkcoinUnits::longName(u)), this);
        menuAction->setData(QVariant(u));
        menu->addAction(menuAction);
    }
    connect(menu, &QMenu::triggered, this, &UnitDisplayStatusBarControl::onMenuSelection);
}

/** Lets the control know about the Options Model (and its signals) */
void UnitDisplayStatusBarControl::setOptionsModel(OptionsModel *_optionsModel)
{
    if (_optionsModel)
    {
        this->optionsModel = _optionsModel;

        // be aware of a display unit change reported by the OptionsModel object.
        connect(_optionsModel, &OptionsModel::displayUnitChanged, this, &UnitDisplayStatusBarControl::updateDisplayUnit);

        // initialize the display units label with the current value in the model.
        updateDisplayUnit(_optionsModel->getDisplayUnit());
    }
}

/** When Display Units are changed on OptionsModel it will refresh the display text of the control on the status bar */
void UnitDisplayStatusBarControl::updateDisplayUnit(int newUnits)
{
    setText(TalkcoinUnits::longName(newUnits));
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
