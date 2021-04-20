// Copyright (c) 2011-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/syscoingui.h>

#include <qt/syscoinunits.h>
#include <qt/clientmodel.h>
#include <qt/createwalletdialog.h>
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

#ifdef ENABLE_WALLET
#include <qt/walletcontroller.h>
#include <qt/walletframe.h>
#include <qt/walletmodel.h>
#include <qt/walletview.h>
#endif // ENABLE_WALLET

#ifdef Q_OS_MAC
#include <qt/macdockiconhandler.h>
#endif

#include <functional>
#include <chain.h>
#include <chainparams.h>
#include <interfaces/handler.h>
#include <interfaces/node.h>
#include <node/ui_interface.h>
#include <util/system.h>
#include <util/translation.h>
#include <validation.h>

#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDateTime>
#include <QDragEnterEvent>
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
// SYSCOIN
#include <qt/masternodelist.h>
#include <QStringList>
const std::string SyscoinGUI::DEFAULT_UIPLATFORM =
#if defined(Q_OS_MAC)
        "macosx"
#elif defined(Q_OS_WIN)
        "windows"
#else
        "other"
#endif
        ;

SyscoinGUI::SyscoinGUI(interfaces::Node& node, const PlatformStyle *_platformStyle, const NetworkStyle *networkStyle, QWidget *parent) :
    QMainWindow(parent),
    m_node(node),
    trayIconMenu{new QMenu()},
    platformStyle(_platformStyle),
    m_network_style(networkStyle)
{
    QSettings settings;
    if (!restoreGeometry(settings.value("MainWindowGeometry").toByteArray())) {
        // Restore failed (perhaps missing setting), center the window
        move(QGuiApplication::primaryScreen()->availableGeometry().center() - frameGeometry().center());
    }

    setContextMenuPolicy(Qt::PreventContextMenu);

#ifdef ENABLE_WALLET
    enableWallet = WalletModel::isWalletEnabled();
#endif // ENABLE_WALLET
    QApplication::setWindowIcon(m_network_style->getTrayAndWindowIcon());
    setWindowIcon(m_network_style->getTrayAndWindowIcon());
    updateWindowTitle();

    rpcConsole = new RPCConsole(node, _platformStyle, nullptr);
    helpMessageDialog = new HelpMessageDialog(this, false);
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
        setCentralWidget(rpcConsole);
        Q_EMIT consoleShown(rpcConsole);
    }

    modalOverlay = new ModalOverlay(enableWallet, this->centralWidget());

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
    labelProxyIcon = new GUIUtil::ClickableLabel();
    connectionsControl = new GUIUtil::ClickableLabel();
    labelBlocksIcon = new GUIUtil::ClickableLabel();
    if(enableWallet)
    {
        frameBlocksLayout->addStretch();
        frameBlocksLayout->addWidget(unitDisplayControl);
        frameBlocksLayout->addStretch();
        frameBlocksLayout->addWidget(labelWalletEncryptionIcon);
        frameBlocksLayout->addWidget(labelWalletHDStatusIcon);
    }
    frameBlocksLayout->addWidget(labelProxyIcon);
    frameBlocksLayout->addStretch();
    frameBlocksLayout->addWidget(connectionsControl);
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
    // See https://doc.qt.io/qt-5/gallery.html
    QString curStyle = QApplication::style()->metaObject()->className();
    if(curStyle == "QWindowsStyle" || curStyle == "QWindowsXPStyle")
    {
        progressBar->setStyleSheet("QProgressBar { background-color: #e8e8e8; border: 1px solid grey; border-radius: 7px; padding: 1px; text-align: center; } QProgressBar::chunk { background: QLinearGradient(x1: 0, y1: 0, x2: 1, y2: 0, stop: 0 #FF8000, stop: 1 orange); border-radius: 7px; margin: 0px; }");
    }

    statusBar()->addWidget(progressBarLabel);
    statusBar()->addWidget(progressBar);
    statusBar()->addPermanentWidget(frameBlocks);

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

    connect(labelBlocksIcon, &GUIUtil::ClickableLabel::clicked, this, &SyscoinGUI::showModalOverlay);
    connect(progressBar, &GUIUtil::ClickableProgressBar::clicked, this, &SyscoinGUI::showModalOverlay);
#ifdef ENABLE_WALLET
    if(enableWallet) {
        connect(walletFrame, &WalletFrame::requestedSyncWarningInfo, this, &SyscoinGUI::showModalOverlay);
    }
#endif

#ifdef Q_OS_MAC
    m_app_nap_inhibitor = new CAppNapInhibitor;
#endif

    GUIUtil::handleCloseWindowShortcut(this);
}

SyscoinGUI::~SyscoinGUI()
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

    delete rpcConsole;
}

void SyscoinGUI::createActions()
{
    QActionGroup *tabGroup = new QActionGroup(this);
    connect(modalOverlay, &ModalOverlay::triggered, tabGroup, &QActionGroup::setEnabled);

    overviewAction = new QAction(platformStyle->SingleColorIcon(":/icons/overview"), tr("&Overview"), this);
    overviewAction->setStatusTip(tr("Show general overview of wallet"));
    overviewAction->setToolTip(overviewAction->statusTip());
    overviewAction->setCheckable(true);
    overviewAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_1));
    tabGroup->addAction(overviewAction);

    sendCoinsAction = new QAction(platformStyle->SingleColorIcon(":/icons/send"), tr("&Send"), this);
    sendCoinsAction->setStatusTip(tr("Send coins to a Syscoin address"));
    sendCoinsAction->setToolTip(sendCoinsAction->statusTip());
    sendCoinsAction->setCheckable(true);
    sendCoinsAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_2));
    tabGroup->addAction(sendCoinsAction);

    sendCoinsMenuAction = new QAction(sendCoinsAction->text(), this);
    sendCoinsMenuAction->setStatusTip(sendCoinsAction->statusTip());
    sendCoinsMenuAction->setToolTip(sendCoinsMenuAction->statusTip());

    receiveCoinsAction = new QAction(platformStyle->SingleColorIcon(":/icons/receiving_addresses"), tr("&Receive"), this);
    receiveCoinsAction->setStatusTip(tr("Request payments (generates QR codes and syscoin: URIs)"));
    receiveCoinsAction->setToolTip(receiveCoinsAction->statusTip());
    receiveCoinsAction->setCheckable(true);
    receiveCoinsAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_3));
    tabGroup->addAction(receiveCoinsAction);

    receiveCoinsMenuAction = new QAction(receiveCoinsAction->text(), this);
    receiveCoinsMenuAction->setStatusTip(receiveCoinsAction->statusTip());
    receiveCoinsMenuAction->setToolTip(receiveCoinsMenuAction->statusTip());

    historyAction = new QAction(platformStyle->SingleColorIcon(":/icons/history"), tr("&Transactions"), this);
    historyAction->setStatusTip(tr("Browse transaction history"));
    historyAction->setToolTip(historyAction->statusTip());
    historyAction->setCheckable(true);
    historyAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_4));
    tabGroup->addAction(historyAction);

#ifdef ENABLE_WALLET
   // SYSCOIN
    QSettings settings;
    if (settings.value("fShowMasternodesTab").toBool()) {
        masternodeAction = new QAction(QIcon(":/icons/masternodes"), tr("&Masternodes"), this);
        masternodeAction->setStatusTip(tr("Browse masternodes"));
        masternodeAction->setToolTip(masternodeAction->statusTip());
        masternodeAction->setCheckable(true);
#ifdef Q_OS_MAC
        masternodeAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_5));
#else
        masternodeAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_5));
#endif
        tabGroup->addAction(masternodeAction);
        connect(masternodeAction, &QAction::triggered, [this]{ showNormalIfMinimized(); });
        connect(masternodeAction, &QAction::triggered,  [this]{ gotoMasternodePage(); });
    }
    // These showNormalIfMinimized are needed because Send Coins and Receive Coins
    // can be triggered from the tray menu, and need to show the GUI to be useful.
    connect(overviewAction, &QAction::triggered, [this]{ showNormalIfMinimized(); });
    connect(overviewAction, &QAction::triggered, this, &SyscoinGUI::gotoOverviewPage);
    connect(sendCoinsAction, &QAction::triggered, [this]{ showNormalIfMinimized(); });
    connect(sendCoinsAction, &QAction::triggered, [this]{ gotoSendCoinsPage(); });
    connect(sendCoinsMenuAction, &QAction::triggered, [this]{ showNormalIfMinimized(); });
    connect(sendCoinsMenuAction, &QAction::triggered, [this]{ gotoSendCoinsPage(); });
    connect(receiveCoinsAction, &QAction::triggered, [this]{ showNormalIfMinimized(); });
    connect(receiveCoinsAction, &QAction::triggered, this, &SyscoinGUI::gotoReceiveCoinsPage);
    connect(receiveCoinsMenuAction, &QAction::triggered, [this]{ showNormalIfMinimized(); });
    connect(receiveCoinsMenuAction, &QAction::triggered, this, &SyscoinGUI::gotoReceiveCoinsPage);
    connect(historyAction, &QAction::triggered, [this]{ showNormalIfMinimized(); });
    connect(historyAction, &QAction::triggered, this, &SyscoinGUI::gotoHistoryPage);
#endif // ENABLE_WALLET

    quitAction = new QAction(tr("E&xit"), this);
    quitAction->setStatusTip(tr("Quit application"));
    quitAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q));
    quitAction->setMenuRole(QAction::QuitRole);
    aboutAction = new QAction(tr("&About %1").arg(PACKAGE_NAME), this);
    aboutAction->setStatusTip(tr("Show information about %1").arg(PACKAGE_NAME));
    aboutAction->setMenuRole(QAction::AboutRole);
    aboutAction->setEnabled(false);
    aboutQtAction = new QAction(tr("About &Qt"), this);
    aboutQtAction->setStatusTip(tr("Show information about Qt"));
    aboutQtAction->setMenuRole(QAction::AboutQtRole);
    optionsAction = new QAction(tr("&Options..."), this);
    optionsAction->setStatusTip(tr("Modify configuration options for %1").arg(PACKAGE_NAME));
    optionsAction->setMenuRole(QAction::PreferencesRole);
    optionsAction->setEnabled(false);
    toggleHideAction = new QAction(tr("&Show / Hide"), this);
    toggleHideAction->setStatusTip(tr("Show or hide the main Window"));

    encryptWalletAction = new QAction(tr("&Encrypt Wallet..."), this);
    encryptWalletAction->setStatusTip(tr("Encrypt the private keys that belong to your wallet"));
    encryptWalletAction->setCheckable(true);
    backupWalletAction = new QAction(tr("&Backup Wallet..."), this);
    backupWalletAction->setStatusTip(tr("Backup wallet to another location"));
    changePassphraseAction = new QAction(tr("&Change Passphrase..."), this);
    changePassphraseAction->setStatusTip(tr("Change the passphrase used for wallet encryption"));
    signMessageAction = new QAction(tr("Sign &message..."), this);
    signMessageAction->setStatusTip(tr("Sign messages with your Syscoin addresses to prove you own them"));
    verifyMessageAction = new QAction(tr("&Verify message..."), this);
    verifyMessageAction->setStatusTip(tr("Verify messages to ensure they were signed with specified Syscoin addresses"));
    m_load_psbt_action = new QAction(tr("&Load PSBT from file..."), this);
    m_load_psbt_action->setStatusTip(tr("Load Partially Signed Syscoin Transaction"));
    m_load_psbt_clipboard_action = new QAction(tr("Load PSBT from clipboard..."), this);
    m_load_psbt_clipboard_action->setStatusTip(tr("Load Partially Signed Syscoin Transaction from clipboard"));

    openRPCConsoleAction = new QAction(tr("Node window"), this);
    openRPCConsoleAction->setStatusTip(tr("Open node debugging and diagnostic console"));
    // initially disable the debug window menu item
    openRPCConsoleAction->setEnabled(false);
    openRPCConsoleAction->setObjectName("openRPCConsoleAction");

    usedSendingAddressesAction = new QAction(tr("&Sending addresses"), this);
    usedSendingAddressesAction->setStatusTip(tr("Show the list of used sending addresses and labels"));
    usedReceivingAddressesAction = new QAction(tr("&Receiving addresses"), this);
    usedReceivingAddressesAction->setStatusTip(tr("Show the list of used receiving addresses and labels"));

    openAction = new QAction(tr("Open &URI..."), this);
    openAction->setStatusTip(tr("Open a syscoin: URI"));

    m_open_wallet_action = new QAction(tr("Open Wallet"), this);
    m_open_wallet_action->setEnabled(false);
    m_open_wallet_action->setStatusTip(tr("Open a wallet"));
    m_open_wallet_menu = new QMenu(this);

    m_close_wallet_action = new QAction(tr("Close Wallet..."), this);
    m_close_wallet_action->setStatusTip(tr("Close wallet"));

    m_create_wallet_action = new QAction(tr("Create Wallet..."), this);
    m_create_wallet_action->setEnabled(false);
    m_create_wallet_action->setStatusTip(tr("Create a new wallet"));

    m_close_all_wallets_action = new QAction(tr("Close All Wallets..."), this);
    m_close_all_wallets_action->setStatusTip(tr("Close all wallets"));

    showHelpMessageAction = new QAction(tr("&Command-line options"), this);
    showHelpMessageAction->setMenuRole(QAction::NoRole);
    showHelpMessageAction->setStatusTip(tr("Show the %1 help message to get a list with possible Syscoin command-line options").arg(PACKAGE_NAME));

    m_mask_values_action = new QAction(tr("&Mask values"), this);
    m_mask_values_action->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_M));
    m_mask_values_action->setStatusTip(tr("Mask the values in the Overview tab"));
    m_mask_values_action->setCheckable(true);

    // SYSCOIN
    openRepairAction = new QAction(platformStyle->SingleColorIcon(":/icons/options"), tr("Wallet &Repair"), this);
    openRepairAction->setStatusTip(tr("Show wallet repair options"));
    openRepairAction->setEnabled(false);

    connect(quitAction, &QAction::triggered, qApp, QApplication::quit);
    connect(aboutAction, &QAction::triggered, this, &SyscoinGUI::aboutClicked);
    connect(aboutQtAction, &QAction::triggered, qApp, QApplication::aboutQt);
    connect(optionsAction, &QAction::triggered, this, &SyscoinGUI::optionsClicked);
    connect(toggleHideAction, &QAction::triggered, this, &SyscoinGUI::toggleHidden);
    connect(showHelpMessageAction, &QAction::triggered, this, &SyscoinGUI::showHelpMessageClicked);
    connect(openRPCConsoleAction, &QAction::triggered, this, &SyscoinGUI::showDebugWindow);
    // prevents an open debug window from becoming stuck/unusable on client shutdown
    connect(quitAction, &QAction::triggered, rpcConsole, &QWidget::hide);
    // SYSCOIN
    connect(openRepairAction, &QAction::triggered, this, &SyscoinGUI::showRepair);

    // Get restart command-line parameters and handle restart
    connect(rpcConsole, &RPCConsole::handleRestart, this, &SyscoinGUI::handleRestart);

#ifdef ENABLE_WALLET
    if(walletFrame)
    {
        connect(encryptWalletAction, &QAction::triggered, walletFrame, &WalletFrame::encryptWallet);
        connect(backupWalletAction, &QAction::triggered, walletFrame, &WalletFrame::backupWallet);
        connect(changePassphraseAction, &QAction::triggered, walletFrame, &WalletFrame::changePassphrase);
        connect(signMessageAction, &QAction::triggered, [this]{ showNormalIfMinimized(); });
        connect(signMessageAction, &QAction::triggered, [this]{ gotoSignMessageTab(); });
        connect(m_load_psbt_action, &QAction::triggered, [this]{ gotoLoadPSBT(); });
        connect(m_load_psbt_clipboard_action, &QAction::triggered, [this]{ gotoLoadPSBT(true); });
        connect(verifyMessageAction, &QAction::triggered, [this]{ showNormalIfMinimized(); });
        connect(verifyMessageAction, &QAction::triggered, [this]{ gotoVerifyMessageTab(); });
        connect(usedSendingAddressesAction, &QAction::triggered, walletFrame, &WalletFrame::usedSendingAddresses);
        connect(usedReceivingAddressesAction, &QAction::triggered, walletFrame, &WalletFrame::usedReceivingAddresses);
        connect(openAction, &QAction::triggered, this, &SyscoinGUI::openClicked);
        // SYSCOIN
        connect(openRepairAction, &QAction::triggered, this, &SyscoinGUI::showRepair);

        connect(m_open_wallet_menu, &QMenu::aboutToShow, [this] {
            m_open_wallet_menu->clear();
            for (const std::pair<const std::string, bool>& i : m_wallet_controller->listWalletDir()) {
                const std::string& path = i.first;
                QString name = path.empty() ? QString("["+tr("default wallet")+"]") : QString::fromStdString(path);
                // Menu items remove single &. Single & are shown when && is in
                // the string, but only the first occurrence. So replace only
                // the first & with &&.
                name.replace(name.indexOf(QChar('&')), 1, QString("&&"));
                QAction* action = m_open_wallet_menu->addAction(name);

                if (i.second) {
                    // This wallet is already loaded
                    action->setEnabled(false);
                    continue;
                }

                connect(action, &QAction::triggered, [this, path] {
                    auto activity = new OpenWalletActivity(m_wallet_controller, this);
                    connect(activity, &OpenWalletActivity::opened, this, &SyscoinGUI::setCurrentWallet);
                    connect(activity, &OpenWalletActivity::finished, activity, &QObject::deleteLater);
                    activity->open(path);
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
        connect(m_create_wallet_action, &QAction::triggered, [this] {
            auto activity = new CreateWalletActivity(m_wallet_controller, this);
            connect(activity, &CreateWalletActivity::created, this, &SyscoinGUI::setCurrentWallet);
            connect(activity, &CreateWalletActivity::finished, activity, &QObject::deleteLater);
            activity->create();
        });
        connect(m_close_all_wallets_action, &QAction::triggered, [this] {
            m_wallet_controller->closeAllWallets(this);
        });
        connect(m_mask_values_action, &QAction::toggled, this, &SyscoinGUI::setPrivacy);
    }
#endif // ENABLE_WALLET

    connect(new QShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_C), this), &QShortcut::activated, this, &SyscoinGUI::showDebugWindowActivateConsole);
    connect(new QShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_D), this), &QShortcut::activated, this, &SyscoinGUI::showDebugWindow);
    // SYSCOIN
    connect(new QShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_R), this), &QShortcut::activated, this, &SyscoinGUI::showRepair);
}

void SyscoinGUI::createMenuBar()
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
        file->addAction(m_create_wallet_action);
        file->addAction(m_open_wallet_action);
        file->addAction(m_close_wallet_action);
        file->addAction(m_close_all_wallets_action);
        file->addSeparator();
        file->addAction(openAction);
        file->addAction(backupWalletAction);
        file->addAction(signMessageAction);
        file->addAction(verifyMessageAction);
        file->addAction(openRepairAction);
        file->addAction(m_load_psbt_action);
        file->addAction(m_load_psbt_clipboard_action);
        file->addSeparator();
    }
    file->addAction(quitAction);

    QMenu *settings = appMenuBar->addMenu(tr("&Settings"));
    if(walletFrame)
    {
        settings->addAction(encryptWalletAction);
        settings->addAction(changePassphraseAction);
        settings->addSeparator();
        settings->addAction(m_mask_values_action);
        settings->addSeparator();
    }
    settings->addAction(optionsAction);

    QMenu* window_menu = appMenuBar->addMenu(tr("&Window"));

    QAction* minimize_action = window_menu->addAction(tr("Minimize"));
    minimize_action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_M));
    connect(minimize_action, &QAction::triggered, [] {
        QApplication::activeWindow()->showMinimized();
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
#endif

    if (walletFrame) {
#ifdef Q_OS_MAC
        window_menu->addSeparator();
        QAction* main_window_action = window_menu->addAction(tr("Main Window"));
        connect(main_window_action, &QAction::triggered, [this] {
            GUIUtil::bringToFront(this);
        });
#endif
        window_menu->addSeparator();
        window_menu->addAction(usedSendingAddressesAction);
        window_menu->addAction(usedReceivingAddressesAction);
    }

    window_menu->addSeparator();
    for (RPCConsole::TabTypes tab_type : rpcConsole->tabs()) {
        QAction* tab_action = window_menu->addAction(rpcConsole->tabTitle(tab_type));
        tab_action->setShortcut(rpcConsole->tabShortcut(tab_type));
        connect(tab_action, &QAction::triggered, [this, tab_type] {
            rpcConsole->setTabFocus(tab_type);
            showDebugWindow();
        });
    }

    QMenu *help = appMenuBar->addMenu(tr("&Help"));
    help->addAction(showHelpMessageAction);
    help->addSeparator();
    help->addAction(aboutAction);
    help->addAction(aboutQtAction);
}

void SyscoinGUI::createToolBars()
{
    if(walletFrame)
    {
        QToolBar *toolbar = addToolBar(tr("Tabs toolbar"));
        appToolBar = toolbar;
        toolbar->setMovable(false);
        toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        toolbar->addAction(overviewAction);
        toolbar->addAction(sendCoinsAction);
        toolbar->addAction(receiveCoinsAction);
        toolbar->addAction(historyAction);
        // SYSCOIN
        QSettings settings;
        if (settings.value("fShowMasternodesTab").toBool() && masternodeAction)
        {
            toolbar->addAction(masternodeAction);
        }
        overviewAction->setChecked(true);

#ifdef ENABLE_WALLET
        QWidget *spacer = new QWidget();
        spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        toolbar->addWidget(spacer);

        m_wallet_selector = new QComboBox();
        m_wallet_selector->setSizeAdjustPolicy(QComboBox::AdjustToContents);
        connect(m_wallet_selector, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &SyscoinGUI::setCurrentWalletBySelectorIndex);

        m_wallet_selector_label = new QLabel();
        m_wallet_selector_label->setText(tr("Wallet:") + " ");
        m_wallet_selector_label->setBuddy(m_wallet_selector);

        m_wallet_selector_label_action = appToolBar->addWidget(m_wallet_selector_label);
        m_wallet_selector_action = appToolBar->addWidget(m_wallet_selector);

        m_wallet_selector_label_action->setVisible(false);
        m_wallet_selector_action->setVisible(false);
#endif
    }
}

void SyscoinGUI::setClientModel(ClientModel *_clientModel, interfaces::BlockAndHeaderTipInfo* tip_info)
{
    this->clientModel = _clientModel;
    if(_clientModel)
    {
        // Create system tray menu (or setup the dock menu) that late to prevent users from calling actions,
        // while the client has not yet fully loaded
        createTrayIconMenu();

        // Keep up to date with client
        updateNetworkState();
        connect(_clientModel, &ClientModel::numConnectionsChanged, this, &SyscoinGUI::setNumConnections);
        connect(_clientModel, &ClientModel::networkActiveChanged, this, &SyscoinGUI::setNetworkActive);

        modalOverlay->setKnownBestHeight(tip_info->header_height, QDateTime::fromTime_t(tip_info->header_time));
        setNumBlocks(tip_info->block_height, QDateTime::fromTime_t(tip_info->block_time), tip_info->verification_progress, false, SynchronizationState::INIT_DOWNLOAD);
        connect(_clientModel, &ClientModel::numBlocksChanged, this, &SyscoinGUI::setNumBlocks);
        // SYSCOIN
        connect(_clientModel, &ClientModel::additionalDataSyncProgressChanged, this, &SyscoinGUI::setAdditionalDataSyncProgress);
        // Receive and report messages from client model
        connect(_clientModel, &ClientModel::message, [this](const QString &title, const QString &message, unsigned int style){
            this->message(title, message, style);
        });

        // Show progress dialog
        connect(_clientModel, &ClientModel::showProgress, this, &SyscoinGUI::showProgress);

        rpcConsole->setClientModel(_clientModel, tip_info->block_height, tip_info->block_time, tip_info->verification_progress);

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
            connect(optionsModel, &OptionsModel::showTrayIconChanged, trayIcon, &QSystemTrayIcon::setVisible);

            // initialize the disable state of the tray icon with the current value in the model.
            trayIcon->setVisible(optionsModel->getShowTrayIcon());
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
        rpcConsole->setClientModel(nullptr);
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
void SyscoinGUI::setWalletController(WalletController* wallet_controller)
{
    assert(!m_wallet_controller);
    assert(wallet_controller);

    m_wallet_controller = wallet_controller;

    m_create_wallet_action->setEnabled(true);
    m_open_wallet_action->setEnabled(true);
    m_open_wallet_action->setMenu(m_open_wallet_menu);

    GUIUtil::ExceptionSafeConnect(wallet_controller, &WalletController::walletAdded, this, &SyscoinGUI::addWallet);
    connect(wallet_controller, &WalletController::walletRemoved, this, &SyscoinGUI::removeWallet);

    for (WalletModel* wallet_model : m_wallet_controller->getOpenWallets()) {
        addWallet(wallet_model);
    }
}

WalletController* SyscoinGUI::getWalletController()
{
    return m_wallet_controller;
}

void SyscoinGUI::addWallet(WalletModel* walletModel)
{
    if (!walletFrame) return;
    if (!walletFrame->addWallet(walletModel)) return;
    rpcConsole->addWallet(walletModel);
    if (m_wallet_selector->count() == 0) {
        setWalletActionsEnabled(true);
    } else if (m_wallet_selector->count() == 1) {
        m_wallet_selector_label_action->setVisible(true);
        m_wallet_selector_action->setVisible(true);
    }
    const QString display_name = walletModel->getDisplayName();
    m_wallet_selector->addItem(display_name, QVariant::fromValue(walletModel));
}

void SyscoinGUI::removeWallet(WalletModel* walletModel)
{
    if (!walletFrame) return;

    labelWalletHDStatusIcon->hide();
    labelWalletEncryptionIcon->hide();

    int index = m_wallet_selector->findData(QVariant::fromValue(walletModel));
    m_wallet_selector->removeItem(index);
    if (m_wallet_selector->count() == 0) {
        setWalletActionsEnabled(false);
        overviewAction->setChecked(true);
    } else if (m_wallet_selector->count() == 1) {
        m_wallet_selector_label_action->setVisible(false);
        m_wallet_selector_action->setVisible(false);
    }
    rpcConsole->removeWallet(walletModel);
    walletFrame->removeWallet(walletModel);
    updateWindowTitle();
}

void SyscoinGUI::setCurrentWallet(WalletModel* wallet_model)
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

void SyscoinGUI::setCurrentWalletBySelectorIndex(int index)
{
    WalletModel* wallet_model = m_wallet_selector->itemData(index).value<WalletModel*>();
    if (wallet_model) setCurrentWallet(wallet_model);
}

void SyscoinGUI::removeAllWallets()
{
    if(!walletFrame)
        return;
    setWalletActionsEnabled(false);
    walletFrame->removeAllWallets();
}
#endif // ENABLE_WALLET

void SyscoinGUI::setWalletActionsEnabled(bool enabled)
{
    overviewAction->setEnabled(enabled);
    sendCoinsAction->setEnabled(enabled);
    sendCoinsMenuAction->setEnabled(enabled);
    receiveCoinsAction->setEnabled(enabled);
    receiveCoinsMenuAction->setEnabled(enabled);
    historyAction->setEnabled(enabled);
    // SYSCOIN
    QSettings settings;
    if (settings.value("fShowMasternodesTab").toBool() && masternodeAction) {
        masternodeAction->setEnabled(enabled);
    }
    encryptWalletAction->setEnabled(enabled);
    backupWalletAction->setEnabled(enabled);
    changePassphraseAction->setEnabled(enabled);
    signMessageAction->setEnabled(enabled);
    verifyMessageAction->setEnabled(enabled);
    usedSendingAddressesAction->setEnabled(enabled);
    usedReceivingAddressesAction->setEnabled(enabled);
    openAction->setEnabled(enabled);
    m_close_wallet_action->setEnabled(enabled);
    m_close_all_wallets_action->setEnabled(enabled);
}

void SyscoinGUI::createTrayIcon()
{
    assert(QSystemTrayIcon::isSystemTrayAvailable());

#ifndef Q_OS_MAC
    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        trayIcon = new QSystemTrayIcon(m_network_style->getTrayAndWindowIcon(), this);
        QString toolTip = tr("%1 client").arg(PACKAGE_NAME) + " " + m_network_style->getTitleAddText();
        trayIcon->setToolTip(toolTip);
    }
#endif
}

void SyscoinGUI::createTrayIconMenu()
{
#ifndef Q_OS_MAC
    // return if trayIcon is unset (only on non-macOSes)
    if (!trayIcon)
        return;

    trayIcon->setContextMenu(trayIconMenu.get());
    connect(trayIcon, &QSystemTrayIcon::activated, this, &SyscoinGUI::trayIconActivated);
#else
    // Note: On macOS, the Dock icon is used to provide the tray's functionality.
    MacDockIconHandler *dockIconHandler = MacDockIconHandler::instance();
    connect(dockIconHandler, &MacDockIconHandler::dockIconClicked, this, &SyscoinGUI::macosDockIconActivated);
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
    }
    trayIconMenu->addAction(optionsAction);
    trayIconMenu->addAction(openRPCConsoleAction);
    trayIconMenu->addAction(openRepairAction);
#ifndef Q_OS_MAC // This is built-in on macOS
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(quitAction);
#endif
}

#ifndef Q_OS_MAC
void SyscoinGUI::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if(reason == QSystemTrayIcon::Trigger)
    {
        // Click on system tray icon triggers show/hide of the main window
        toggleHidden();
    }
}
#else
void SyscoinGUI::macosDockIconActivated()
{
    show();
    activateWindow();
}
#endif

void SyscoinGUI::optionsClicked()
{
    openOptionsDialogWithTab(OptionsDialog::TAB_MAIN);
}

void SyscoinGUI::aboutClicked()
{
    if(!clientModel)
        return;

    HelpMessageDialog dlg(this, true);
    dlg.exec();
}

void SyscoinGUI::showDebugWindow()
{
    GUIUtil::bringToFront(rpcConsole);
    Q_EMIT consoleShown(rpcConsole);
}
// SYSCOIN
void SyscoinGUI::showRepair()
{
    rpcConsole->setTabFocus(RPCConsole::TabTypes::TAB_REPAIR);
    showDebugWindow();
}
void SyscoinGUI::showDebugWindowActivateConsole()
{
    rpcConsole->setTabFocus(RPCConsole::TabTypes::CONSOLE);
    showDebugWindow();
}

void SyscoinGUI::showHelpMessageClicked()
{
    GUIUtil::bringToFront(helpMessageDialog);
}

#ifdef ENABLE_WALLET
void SyscoinGUI::openClicked()
{
    OpenURIDialog dlg(this);
    if(dlg.exec())
    {
        Q_EMIT receivedURI(dlg.getURI());
    }
}
// SYSCOIN

void SyscoinGUI::gotoMasternodePage()
{
    QSettings settings;
    if (settings.value("fShowMasternodesTab").toBool() && masternodeAction) {
        masternodeAction->setChecked(true);
        if (walletFrame) walletFrame->gotoMasternodePage();
    }
}
void SyscoinGUI::gotoOverviewPage()
{
    overviewAction->setChecked(true);
    if (walletFrame) walletFrame->gotoOverviewPage();
}

void SyscoinGUI::gotoHistoryPage()
{
    historyAction->setChecked(true);
    if (walletFrame) walletFrame->gotoHistoryPage();
}

void SyscoinGUI::gotoReceiveCoinsPage()
{
    receiveCoinsAction->setChecked(true);
    if (walletFrame) walletFrame->gotoReceiveCoinsPage();
}

void SyscoinGUI::gotoSendCoinsPage(QString addr)
{
    sendCoinsAction->setChecked(true);
    if (walletFrame) walletFrame->gotoSendCoinsPage(addr);
}

void SyscoinGUI::gotoSignMessageTab(QString addr)
{
    if (walletFrame) walletFrame->gotoSignMessageTab(addr);
}

void SyscoinGUI::gotoVerifyMessageTab(QString addr)
{
    if (walletFrame) walletFrame->gotoVerifyMessageTab(addr);
}
void SyscoinGUI::gotoLoadPSBT(bool from_clipboard)
{
    if (walletFrame) walletFrame->gotoLoadPSBT(from_clipboard);
}
#endif // ENABLE_WALLET

void SyscoinGUI::updateNetworkState()
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
        tooltip = tr("%n active connection(s) to Syscoin network", "", count) + QString(".<br>") + tr("Click to disable network activity.");
    } else {
        tooltip = tr("Network activity disabled.") + QString("<br>") + tr("Click to enable network activity again.");
        icon = ":/icons/network_disabled";
    }

    // Don't word-wrap this (fixed-width) tooltip
    tooltip = QString("<nobr>") + tooltip + QString("</nobr>");
    connectionsControl->setToolTip(tooltip);

    connectionsControl->setPixmap(platformStyle->SingleColorIcon(icon).pixmap(STATUSBAR_ICONSIZE,STATUSBAR_ICONSIZE));
}

void SyscoinGUI::setNumConnections(int count)
{
    updateNetworkState();
}

void SyscoinGUI::setNetworkActive(bool networkActive)
{
    updateNetworkState();
}

void SyscoinGUI::updateHeadersSyncProgressLabel()
{
    int64_t headersTipTime = clientModel->getHeaderTipTime();
    int headersTipHeight = clientModel->getHeaderTipHeight();
    int estHeadersLeft = (GetTime() - headersTipTime) / Params().GetConsensus().nPowTargetSpacing;
    if (estHeadersLeft > HEADER_HEIGHT_DELTA_SYNC)
        progressBarLabel->setText(tr("Syncing Headers (%1%)...").arg(QString::number(100.0 / (headersTipHeight+estHeadersLeft)*headersTipHeight, 'f', 1)));
}

void SyscoinGUI::openOptionsDialogWithTab(OptionsDialog::Tab tab)
{
    if (!clientModel || !clientModel->getOptionsModel())
        return;

    OptionsDialog dlg(this, enableWallet);
    dlg.setCurrentTab(tab);
    dlg.setModel(clientModel->getOptionsModel());
    dlg.exec();
}

void SyscoinGUI::setNumBlocks(int count, const QDateTime& blockDate, double nVerificationProgress, bool header, SynchronizationState sync_state)
{
// Disabling macOS App Nap on initial sync, disk and reindex operations.
#ifdef Q_OS_MAC
    if (sync_state == SynchronizationState::POST_INIT) {
        m_app_nap_inhibitor->enableAppNap();
    } else {
        m_app_nap_inhibitor->disableAppNap();
    }
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

    // Prevent orphan statusbar messages (e.g. hover Quit in main menu, wait until chain-sync starts -> garbled text)
    statusBar()->clearMessage();

    // Acquire current block source
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
            progressBarLabel->setText(tr("Connecting to peers..."));
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
    if(!m_node.masternodeSync().isBlockchainSynced())
    {
        QString timeBehindText = GUIUtil::formatNiceTimeOffset(secs);

        progressBarLabel->setVisible(true);
        progressBar->setFormat(tr("%1 behind").arg(timeBehindText));
        progressBar->setMaximum(1000000000);
        progressBar->setValue(nVerificationProgress * 1000000000.0 + 0.5);
        progressBar->setVisible(true);

        tooltip = tr("Catching up...") + QString("<br>") + tooltip;
        if(count != prevBlocks)
        {
            labelBlocksIcon->setPixmap(platformStyle->SingleColorIcon(QString(
                ":/animation/spinner-%1").arg(spinnerFrame, 3, 10, QChar('0')))
                .pixmap(STATUSBAR_ICONSIZE, STATUSBAR_ICONSIZE));
            spinnerFrame = (spinnerFrame + 1) % SPINNER_FRAMES;
        }
        prevBlocks = count;

#ifdef ENABLE_WALLET
        if(walletFrame)
        {
            walletFrame->showOutOfSyncWarning(true);
            modalOverlay->showHide();
        }
#endif // ENABLE_WALLET

        tooltip += QString("<br>");
        tooltip += tr("Last received block was generated %1 ago.").arg(timeBehindText);
        tooltip += QString("<br>");
        tooltip += tr("Transactions after this will not yet be visible.");
    } else if (fDisableGovernance) {
        setAdditionalDataSyncProgress(1);
    }
    // Don't word-wrap this (fixed-width) tooltip
    tooltip = QString("<nobr>") + tooltip + QString("</nobr>");

    labelBlocksIcon->setToolTip(tooltip);
    progressBarLabel->setToolTip(tooltip);
    progressBar->setToolTip(tooltip);
}
// SYSCOIN
void SyscoinGUI::setAdditionalDataSyncProgress(double nSyncProgress)
{
    if(!clientModel)
        return;

    // No additional data sync should be happening while blockchain is not synced, nothing to update
    if(!m_node.masternodeSync().isBlockchainSynced())
        return;
   // Prevent orphan statusbar messages (e.g. hover Quit in main menu, wait until chain-sync starts -> garbelled text)
    statusBar()->clearMessage();

    QString tooltip;
    QString strSyncStatus;
    tooltip = tr("Up to date") + QString(".<br>") + tooltip;

#ifdef ENABLE_WALLET
    if(walletFrame)
        walletFrame->showOutOfSyncWarning(false);
#endif // ENABLE_WALLET

     if (m_node.masternodeSync().isSynced()) {
        progressBarLabel->setVisible(false);
        progressBar->setVisible(false);
        labelBlocksIcon->setPixmap(platformStyle->SingleColorIcon(":/icons/synced").pixmap(STATUSBAR_ICONSIZE, STATUSBAR_ICONSIZE));
    } else {

        labelBlocksIcon->setPixmap(platformStyle->SingleColorIcon(QString(
            ":/movies/spinner-%1").arg(spinnerFrame, 3, 10, QChar('0')))
            .pixmap(STATUSBAR_ICONSIZE, STATUSBAR_ICONSIZE));
        spinnerFrame = (spinnerFrame + 1) % SPINNER_FRAMES;

        progressBar->setFormat(tr("Synchronizing additional data: %p%"));
        progressBar->setMaximum(1000000000);
        progressBar->setValue(nSyncProgress * 1000000000.0 + 0.5);
    }

    strSyncStatus = QString::fromStdString(m_node.masternodeSync().getSyncStatus());
    progressBarLabel->setText(strSyncStatus);
    tooltip = strSyncStatus + QString("<br>") + tooltip;

    // Don't word-wrap this (fixed-width) tooltip
    tooltip = QString("<nobr>") + tooltip + QString("</nobr>");

    labelBlocksIcon->setToolTip(tooltip);
    progressBarLabel->setToolTip(tooltip);
    progressBar->setToolTip(tooltip);
}
void SyscoinGUI::message(const QString& title, QString message, unsigned int style, bool* ret, const QString& detailed_message)
{
    // Default title. On macOS, the window title is ignored (as required by the macOS Guidelines).
    QString strTitle{PACKAGE_NAME};
    // Default to information icon
    int nMBoxIcon = QMessageBox::Information;
    int nNotifyIcon = Notificator::Information;

    QString msgType;
    if (!title.isEmpty()) {
        msgType = title;
    } else {
        switch (style) {
        case CClientUIInterface::MSG_ERROR:
            msgType = tr("Error");
            message = tr("Error: %1").arg(message);
            break;
        case CClientUIInterface::MSG_WARNING:
            msgType = tr("Warning");
            message = tr("Warning: %1").arg(message);
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
        mBox.setDetailedText(detailed_message);
        int r = mBox.exec();
        if (ret != nullptr)
            *ret = r == QMessageBox::Ok;
    } else {
        notificator->notify(static_cast<Notificator::Class>(nNotifyIcon), strTitle, message);
    }
}

void SyscoinGUI::changeEvent(QEvent *e)
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
                QTimer::singleShot(0, this, &SyscoinGUI::hide);
                e->ignore();
            }
            else if((wsevt->oldState() & Qt::WindowMinimized) && !isMinimized())
            {
                QTimer::singleShot(0, this, &SyscoinGUI::show);
                e->ignore();
            }
        }
    }
#endif
}

void SyscoinGUI::closeEvent(QCloseEvent *event)
{
#ifndef Q_OS_MAC // Ignored on Mac
    if(clientModel && clientModel->getOptionsModel())
    {
        if(!clientModel->getOptionsModel()->getMinimizeOnClose())
        {
            // close rpcConsole in case it was open to make some space for the shutdown window
            rpcConsole->close();

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

void SyscoinGUI::showEvent(QShowEvent *event)
{
    // enable the debug window when the main window shows up
    openRPCConsoleAction->setEnabled(true);
    aboutAction->setEnabled(true);
    optionsAction->setEnabled(true);
    openRepairAction->setEnabled(true);
}

#ifdef ENABLE_WALLET
void SyscoinGUI::incomingTransaction(const QString& date, int unit, const CAmount& amount, const QString& type, const QString& address, const QString& label, const QString& walletName)
{
    // On new transaction, make an info balloon
    QString msg = tr("Date: %1\n").arg(date) +
                  tr("Amount: %1\n").arg(SyscoinUnits::formatWithUnit(unit, amount, true));
    if (m_node.walletClient().getWallets().size() > 1 && !walletName.isEmpty()) {
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
#endif // ENABLE_WALLET

void SyscoinGUI::dragEnterEvent(QDragEnterEvent *event)
{
    // Accept only URIs
    if(event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void SyscoinGUI::dropEvent(QDropEvent *event)
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

bool SyscoinGUI::eventFilter(QObject *object, QEvent *event)
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
bool SyscoinGUI::handlePaymentRequest(const SendCoinsRecipient& recipient)
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

void SyscoinGUI::setHDStatus(bool privkeyDisabled, int hdEnabled)
{
    labelWalletHDStatusIcon->setPixmap(platformStyle->SingleColorIcon(privkeyDisabled ? ":/icons/eye" : hdEnabled ? ":/icons/hd_enabled" : ":/icons/hd_disabled").pixmap(STATUSBAR_ICONSIZE,STATUSBAR_ICONSIZE));
    labelWalletHDStatusIcon->setToolTip(privkeyDisabled ? tr("Private key <b>disabled</b>") : hdEnabled ? tr("HD key generation is <b>enabled</b>") : tr("HD key generation is <b>disabled</b>"));
    labelWalletHDStatusIcon->show();
    // eventually disable the QLabel to set its opacity to 50%
    labelWalletHDStatusIcon->setEnabled(hdEnabled);
}

void SyscoinGUI::setEncryptionStatus(int status)
{
    switch(status)
    {
    case WalletModel::Unencrypted:
        labelWalletEncryptionIcon->hide();
        encryptWalletAction->setChecked(false);
        changePassphraseAction->setEnabled(false);
        encryptWalletAction->setEnabled(true);
        break;
    case WalletModel::Unlocked:
        labelWalletEncryptionIcon->show();
        labelWalletEncryptionIcon->setPixmap(platformStyle->SingleColorIcon(":/icons/lock_open").pixmap(STATUSBAR_ICONSIZE,STATUSBAR_ICONSIZE));
        labelWalletEncryptionIcon->setToolTip(tr("Wallet is <b>encrypted</b> and currently <b>unlocked</b>"));
        encryptWalletAction->setChecked(true);
        changePassphraseAction->setEnabled(true);
        encryptWalletAction->setEnabled(false);
        break;
    case WalletModel::Locked:
        labelWalletEncryptionIcon->show();
        labelWalletEncryptionIcon->setPixmap(platformStyle->SingleColorIcon(":/icons/lock_closed").pixmap(STATUSBAR_ICONSIZE,STATUSBAR_ICONSIZE));
        labelWalletEncryptionIcon->setToolTip(tr("Wallet is <b>encrypted</b> and currently <b>locked</b>"));
        encryptWalletAction->setChecked(true);
        changePassphraseAction->setEnabled(true);
        encryptWalletAction->setEnabled(false);
        break;
    }
}

void SyscoinGUI::updateWalletStatus()
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
    setHDStatus(walletModel->wallet().privateKeysDisabled(), walletModel->wallet().hdEnabled());
}
#endif // ENABLE_WALLET

void SyscoinGUI::updateProxyIcon()
{
    std::string ip_port;
    bool proxy_enabled = clientModel->getProxyInfo(ip_port);

    if (proxy_enabled) {
        if (!GUIUtil::HasPixmap(labelProxyIcon)) {
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

void SyscoinGUI::updateWindowTitle()
{
    QString window_title = PACKAGE_NAME;
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

void SyscoinGUI::showNormalIfMinimized(bool fToggleHidden)
{
    if(!clientModel)
        return;

    if (!isHidden() && !isMinimized() && !GUIUtil::isObscured(this) && fToggleHidden) {
        hide();
    } else {
        GUIUtil::bringToFront(this);
    }
}

void SyscoinGUI::toggleHidden()
{
    showNormalIfMinimized(true);
}

void SyscoinGUI::detectShutdown()
{
    if (m_node.shutdownRequested())
    {
        if(rpcConsole)
            rpcConsole->hide();
        qApp->quit();
    }
}

void SyscoinGUI::showProgress(const QString &title, int nProgress)
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

void SyscoinGUI::showModalOverlay()
{
    if (modalOverlay && (progressBar->isVisible() || modalOverlay->isLayerVisible()))
        modalOverlay->toggleVisibility();
}

static bool ThreadSafeMessageBox(SyscoinGUI* gui, const bilingual_str& message, const std::string& caption, unsigned int style)
{
    bool modal = (style & CClientUIInterface::MODAL);
    // The SECURE flag has no effect in the Qt GUI.
    // bool secure = (style & CClientUIInterface::SECURE);
    style &= ~CClientUIInterface::SECURE;
    bool ret = false;

    QString detailed_message; // This is original message, in English, for googling and referencing.
    if (message.original != message.translated) {
        detailed_message = SyscoinGUI::tr("Original message:") + "\n" + QString::fromStdString(message.original);
    }

    // In case of modal message, use blocking connection to wait for user to click a button
    bool invoked = QMetaObject::invokeMethod(gui, "message",
                               modal ? GUIUtil::blockingGUIThreadConnection() : Qt::QueuedConnection,
                               Q_ARG(QString, QString::fromStdString(caption)),
                               Q_ARG(QString, QString::fromStdString(message.translated)),
                               Q_ARG(unsigned int, style),
                               Q_ARG(bool*, &ret),
                               Q_ARG(QString, detailed_message));
    assert(invoked);
    return ret;
}

/** Get restart command-line parameters and request restart */
void SyscoinGUI::handleRestart(const QStringList &args)
{
    if (!m_node.shutdownRequested())
        Q_EMIT requestedRestart(args);
}

void SyscoinGUI::subscribeToCoreSignals()
{
    // Connect signals to client
    m_handler_message_box = m_node.handleMessageBox(std::bind(ThreadSafeMessageBox, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    m_handler_question = m_node.handleQuestion(std::bind(ThreadSafeMessageBox, this, std::placeholders::_1, std::placeholders::_3, std::placeholders::_4));
}

void SyscoinGUI::unsubscribeFromCoreSignals()
{
    // Disconnect signals from client
    m_handler_message_box->disconnect();
    m_handler_question->disconnect();
}

bool SyscoinGUI::isPrivacyModeActivated() const
{
    assert(m_mask_values_action);
    return m_mask_values_action->isChecked();
}

UnitDisplayStatusBarControl::UnitDisplayStatusBarControl(const PlatformStyle *platformStyle) :
    optionsModel(nullptr),
    menu(nullptr)
{
    createContextMenu();
    setToolTip(tr("Unit to show amounts in. Click to select another unit."));
    QList<SyscoinUnits::Unit> units = SyscoinUnits::availableUnits();
    int max_width = 0;
    const QFontMetrics fm(font());
    for (const SyscoinUnits::Unit unit : units)
    {
        max_width = qMax(max_width, GUIUtil::TextWidth(fm, SyscoinUnits::longName(unit)));
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
    menu = new QMenu(this);
    for (const SyscoinUnits::Unit u : SyscoinUnits::availableUnits()) {
        menu->addAction(SyscoinUnits::longName(u))->setData(QVariant(u));
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
    setText(SyscoinUnits::longName(newUnits));
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
