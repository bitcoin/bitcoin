// Copyright (c) 2011-2021 The Bitcoin Core developers
// Copyright (c) 2014-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/bitcoingui.h>

#include <qt/bitcoinunits.h>
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
#include <qt/rpcconsole.h>
#include <qt/utilitydialog.h>

#ifdef ENABLE_WALLET
#include <qt/walletcontroller.h>
#include <qt/walletframe.h>
#include <qt/walletmodel.h>
#include <qt/walletview.h>
#endif // ENABLE_WALLET

#ifdef Q_OS_MACOS
#include <qt/macdockiconhandler.h>
#endif

#include <functional>
#include <chain.h>
#include <chainparams.h>
#include <interfaces/coinjoin.h>
#include <interfaces/handler.h>
#include <interfaces/node.h>
#include <node/interface_ui.h>
#include <qt/governancelist.h>
#include <qt/masternodelist.h>
#include <util/system.h>
#include <util/translation.h>
#include <validation.h>

#include <QAction>
#include <QApplication>
#include <QButtonGroup>
#include <QComboBox>
#include <QCursor>
#include <QDateTime>
#include <QDragEnterEvent>
#include <QInputDialog>
#include <QKeySequence>
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
#include <QToolButton>
#include <QUrlQuery>
#include <QVBoxLayout>
#include <QWindow>


const std::string BitcoinGUI::DEFAULT_UIPLATFORM =
#if defined(Q_OS_MACOS)
        "macosx"
#elif defined(Q_OS_WIN)
        "windows"
#else
        "other"
#endif
        ;

BitcoinGUI::BitcoinGUI(interfaces::Node& node, const NetworkStyle* networkStyle, QWidget* parent) :
    QMainWindow(parent),
    m_node(node),
    trayIconMenu{new QMenu()},
    m_network_style(networkStyle)
{
    GUIUtil::loadTheme(true);

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

    rpcConsole = new RPCConsole(node, this, enableWallet ? Qt::Window : Qt::Widget);
    helpMessageDialog = new HelpMessageDialog(this, HelpMessageDialog::cmdline);
#ifdef ENABLE_WALLET
    if(enableWallet)
    {
        /** Create wallet frame*/
        walletFrame = new WalletFrame(this);
        connect(walletFrame, &WalletFrame::createWalletButtonClicked, [this] {
            auto activity = new CreateWalletActivity(getWalletController(), this);
            activity->create();
        });
        connect(walletFrame, &WalletFrame::message, [this](const QString& title, const QString& message, unsigned int style) {
            this->message(title, message, style);
        });
        connect(walletFrame, &WalletFrame::currentWalletSet, [this] { updateWalletStatus(); });
    } else
#endif // ENABLE_WALLET
    {
        /* When compiled without wallet or -disablewallet is provided,
         * the central widget is the rpc console.
         */
        setCentralWidget(rpcConsole);
        Q_EMIT consoleShown(rpcConsole);
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
    unitDisplayControl = new UnitDisplayStatusBarControl();
    labelWalletEncryptionIcon = new QLabel();
    labelWalletHDStatusIcon = new QLabel();
    labelConnectionsIcon = new GUIUtil::ClickableLabel();
    labelProxyIcon = new GUIUtil::ClickableLabel();
    labelBlocksIcon = new GUIUtil::ClickableLabel();
    if(enableWallet)
    {
        frameBlocksLayout->addStretch();
        frameBlocksLayout->addWidget(unitDisplayControl);
        frameBlocksLayout->addStretch();
        frameBlocksLayout->addWidget(labelWalletHDStatusIcon);
        labelWalletHDStatusIcon->hide();
        frameBlocksLayout->addWidget(labelWalletEncryptionIcon);
        labelWalletEncryptionIcon->hide();
    }
    frameBlocksLayout->addWidget(labelProxyIcon);
    frameBlocksLayout->addStretch();
    frameBlocksLayout->addWidget(labelConnectionsIcon);
    frameBlocksLayout->addStretch();
    frameBlocksLayout->addWidget(labelBlocksIcon);
    frameBlocksLayout->addStretch();

    // Hide the spinner/synced icon by default to avoid
    // that the spinner starts before we have any connections
    labelBlocksIcon->hide();

    // Progress bar and label for blocks download
    progressBarLabel = new QLabel();
    progressBarLabel->setVisible(true);
    progressBarLabel->setObjectName("lblStatusBarProgress");
    progressBar = new GUIUtil::ProgressBar();
    progressBar->setAlignment(Qt::AlignCenter);
    progressBar->setVisible(true);

    // Override style sheet for progress bar for styles that have a segmented progress bar,
    // as they make the text unreadable (workaround for issue #1071)
    // See https://doc.qt.io/qt-5/gallery.html
    QString curStyle = QApplication::style()->metaObject()->className();
    if(curStyle == "QWindowsStyle" || curStyle == "QWindowsXPStyle")
    {
        progressBar->setStyleSheet("QProgressBar { background-color: #F8F8F8; border: 1px solid grey; border-radius: 7px; padding: 1px; text-align: center; } QProgressBar::chunk { background: QLinearGradient(x1: 0, y1: 0, x2: 1, y2: 0, stop: 0 #00CCFF, stop: 1 #33CCFF); border-radius: 7px; margin: 0px; }");
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

    connect(labelProxyIcon, &GUIUtil::ClickableLabel::clicked, [this] {
        openOptionsDialogWithTab(OptionsDialog::TAB_NETWORK);
    });

    modalOverlay = new ModalOverlay(enableWallet, this->centralWidget());
    connect(labelBlocksIcon, &GUIUtil::ClickableLabel::clicked, this, &BitcoinGUI::showModalOverlay);
    connect(progressBar, &GUIUtil::ClickableProgressBar::clicked, this, &BitcoinGUI::showModalOverlay);

#ifdef Q_OS_MACOS
    m_app_nap_inhibitor = new CAppNapInhibitor;
#endif

    incomingTransactionsTimer = new QTimer(this);
    incomingTransactionsTimer->setSingleShot(true);
#ifdef ENABLE_WALLET
    connect(incomingTransactionsTimer, &QTimer::timeout, this, &BitcoinGUI::showIncomingTransactions);
#endif

    bool fDebugCustomStyleSheets = gArgs.GetBoolArg("-debug-ui", false) && GUIUtil::isStyleSheetDirectoryCustom();
    if (fDebugCustomStyleSheets) {
        timerCustomCss = new QTimer(this);
        QObject::connect(timerCustomCss, &QTimer::timeout, [this]() {
            if (!m_node.shutdownRequested()) {
                GUIUtil::loadStyleSheet();
            }
        });
        timerCustomCss->start(200);
    }

    GUIUtil::handleCloseWindowShortcut(this);
}

BitcoinGUI::~BitcoinGUI()
{
    // Unsubscribe from notifications from core
    unsubscribeFromCoreSignals();

    QSettings settings;
    settings.setValue("MainWindowGeometry", saveGeometry());
    if(trayIcon) // Hide tray icon, as deleting will let it linger until quit (on Ubuntu)
        trayIcon->hide();
#ifdef Q_OS_MACOS
    delete m_app_nap_inhibitor;
    MacDockIconHandler::cleanup();
#endif

    delete rpcConsole;
    delete tabGroup;
}

void BitcoinGUI::startSpinner()
{
    if (labelBlocksIcon == nullptr || labelBlocksIcon->isHidden() || timerSpinner != nullptr) {
        return;
    }
    auto getNextFrame = []() {
        static std::vector<std::unique_ptr<QPixmap>> vecFrames;
        static std::vector<std::unique_ptr<QPixmap>>::iterator itFrame;
        while (vecFrames.size() < SPINNER_FRAMES) {
            QString&& strFrame = QString("spinner-%1").arg(vecFrames.size(), 3, 10, QChar('0'));
            QPixmap&& frame = getIcon(strFrame, GUIUtil::ThemedColor::ORANGE, ":animation/").pixmap(STATUSBAR_ICONSIZE, STATUSBAR_ICONSIZE);
            itFrame = vecFrames.insert(vecFrames.end(), std::make_unique<QPixmap>(frame));
        }
        assert(vecFrames.size() == SPINNER_FRAMES);
        if (itFrame == vecFrames.end()) {
            itFrame = vecFrames.begin();
        }
        return *itFrame++->get();
    };

    timerSpinner = new QTimer(this);
    QObject::connect(timerSpinner, &QTimer::timeout, [this, getNextFrame]() {
        if (timerSpinner == nullptr) {
            return;
        }
        labelBlocksIcon->setPixmap(getNextFrame());
    });
    timerSpinner->start(40);
}

void BitcoinGUI::stopSpinner()
{
    if (timerSpinner == nullptr) {
        return;
    }
    timerSpinner->deleteLater();
    timerSpinner = nullptr;
}

void BitcoinGUI::startConnectingAnimation()
{
    static int nStep{-1};
    const int nAnimationSteps = 10;

    if (timerConnecting != nullptr) {
        return;
    }

    timerConnecting = new QTimer(this);
    QObject::connect(timerConnecting, &QTimer::timeout, [this]() {

        if (timerConnecting == nullptr) {
            return;
        }

        QString strImage;
        GUIUtil::ThemedColor color;

        nStep = (nStep + 1) % (nAnimationSteps + 1);
        if (nStep == 0) {
            strImage = "connect_4";
            color = GUIUtil::ThemedColor::ICON_ALTERNATIVE_COLOR;
        } else if (nStep == nAnimationSteps / 2) {
            strImage = "connect_1";
            color = GUIUtil::ThemedColor::ORANGE;
        } else {
            return;
        }
        labelConnectionsIcon->setPixmap(GUIUtil::getIcon(strImage, color).pixmap(STATUSBAR_ICONSIZE, STATUSBAR_ICONSIZE));
    });
    timerConnecting->start(100);
}

void BitcoinGUI::stopConnectingAnimation()
{
    if (timerConnecting == nullptr) {
        return;
    }
    timerConnecting->deleteLater();
    timerConnecting = nullptr;
}

void BitcoinGUI::createActions()
{
    sendCoinsAction = new QAction(tr("&Send"), this);
    sendCoinsAction->setStatusTip(tr("Send coins to a Dash address"));
    QString strCoinJoinName = QString::fromStdString(gCoinJoinName);
    coinJoinCoinsAction = new QAction(QString("&%1").arg(strCoinJoinName), this);
    coinJoinCoinsAction->setStatusTip(tr("Send %1 funds to a Dash address").arg(strCoinJoinName));
    coinJoinCoinsAction->setToolTip(coinJoinCoinsAction->statusTip());

    receiveCoinsAction = new QAction(tr("&Receive"), this);
    receiveCoinsAction->setStatusTip(tr("Request payments (generates QR codes and dash: URIs)"));
    receiveCoinsAction->setToolTip(receiveCoinsAction->statusTip());

#ifdef ENABLE_WALLET
    // These showNormalIfMinimized are needed because Send Coins and Receive Coins
    // can be triggered from the tray menu, and need to show the GUI to be useful.
    connect(sendCoinsAction, &QAction::triggered, [this]{ showNormalIfMinimized(); });
    connect(sendCoinsAction, &QAction::triggered, [this]{ gotoSendCoinsPage(); });
    connect(coinJoinCoinsAction, &QAction::triggered, [this]{ showNormalIfMinimized(); });
    connect(coinJoinCoinsAction, &QAction::triggered, [this]{ gotoCoinJoinCoinsPage(); });
    connect(receiveCoinsAction, &QAction::triggered, [this]{ showNormalIfMinimized(); });
    connect(receiveCoinsAction, &QAction::triggered, this, &BitcoinGUI::gotoReceiveCoinsPage);
#endif

    quitAction = new QAction(tr("E&xit"), this);
    quitAction->setStatusTip(tr("Quit application"));
    quitAction->setShortcut(QKeySequence(tr("Ctrl+Q")));
    quitAction->setMenuRole(QAction::QuitRole);
    aboutAction = new QAction(tr("&About %1").arg(PACKAGE_NAME), this);
    aboutAction->setStatusTip(tr("Show information about %1").arg(PACKAGE_NAME));
    aboutAction->setMenuRole(QAction::AboutRole);
    aboutAction->setEnabled(false);
    aboutQtAction = new QAction(tr("About &Qt"), this);
    aboutQtAction->setStatusTip(tr("Show information about Qt"));
    aboutQtAction->setMenuRole(QAction::AboutQtRole);
    optionsAction = new QAction(tr("&Options…"), this);
    optionsAction->setStatusTip(tr("Modify configuration options for %1").arg(PACKAGE_NAME));
    optionsAction->setMenuRole(QAction::PreferencesRole);
    optionsAction->setEnabled(false);

    encryptWalletAction = new QAction(tr("&Encrypt Wallet…"), this);
    encryptWalletAction->setStatusTip(tr("Encrypt the private keys that belong to your wallet"));
    backupWalletAction = new QAction(tr("&Backup Wallet…"), this);
    backupWalletAction->setStatusTip(tr("Backup wallet to another location"));
    changePassphraseAction = new QAction(tr("&Change Passphrase…"), this);
    changePassphraseAction->setStatusTip(tr("Change the passphrase used for wallet encryption"));
    unlockWalletAction = new QAction(tr("&Unlock Wallet…"), this);
    unlockWalletAction->setToolTip(tr("Unlock wallet"));
    lockWalletAction = new QAction(tr("&Lock Wallet"), this);
    signMessageAction = new QAction(tr("Sign &message…"), this);
    signMessageAction->setStatusTip(tr("Sign messages with your Dash addresses to prove you own them"));
    verifyMessageAction = new QAction(tr("&Verify message…"), this);
    verifyMessageAction->setStatusTip(tr("Verify messages to ensure they were signed with specified Dash addresses"));
    m_load_psbt_action = new QAction(tr("&Load PSBT from file…"), this);
    m_load_psbt_action->setStatusTip(tr("Load Partially Signed Blockchain Transaction"));
    m_load_psbt_clipboard_action = new QAction(tr("Load PSBT from &clipboard…"), this);
    m_load_psbt_clipboard_action->setStatusTip(tr("Load Partially Signed Blockchain Transaction from clipboard"));

    openInfoAction = new QAction(tr("&Information"), this);
    openInfoAction->setStatusTip(tr("Show diagnostic information"));
    openRPCConsoleAction = new QAction(tr("&Debug console"), this);
    openRPCConsoleAction->setStatusTip(tr("Open debugging and diagnostic console"));
    openGraphAction = new QAction(tr("&Network Monitor"), this);
    openGraphAction->setStatusTip(tr("Show network monitor"));
    openPeersAction = new QAction(tr("&Peers list"), this);
    openPeersAction->setStatusTip(tr("Show peers info"));
    openRepairAction = new QAction(tr("Wallet &Repair"), this);
    openRepairAction->setStatusTip(tr("Show wallet repair options"));
    openConfEditorAction = new QAction(tr("Open &wallet configuration file"), this);
    openConfEditorAction->setStatusTip(tr("Open configuration file"));
    // override TextHeuristicRole set by default which confuses this action with application settings
    openConfEditorAction->setMenuRole(QAction::NoRole);
    showBackupsAction = new QAction(tr("Show Automatic &Backups"), this);
    showBackupsAction->setStatusTip(tr("Show automatically created wallet backups"));
    // initially disable the debug window menu items
    openInfoAction->setEnabled(false);
    openRPCConsoleAction->setEnabled(false);
    openRPCConsoleAction->setObjectName("openRPCConsoleAction");
    openGraphAction->setEnabled(false);
    openPeersAction->setEnabled(false);
    openRepairAction->setEnabled(false);

    usedSendingAddressesAction = new QAction(tr("&Sending addresses"), this);
    usedSendingAddressesAction->setStatusTip(tr("Show the list of used sending addresses and labels"));
    usedReceivingAddressesAction = new QAction(tr("&Receiving addresses"), this);
    usedReceivingAddressesAction->setStatusTip(tr("Show the list of used receiving addresses and labels"));

    openAction = new QAction(tr("Open &URI…"), this);
    openAction->setStatusTip(tr("Open a dash: URI"));

    m_open_wallet_action = new QAction(tr("Open Wallet"), this);
    m_open_wallet_action->setEnabled(false);
    m_open_wallet_action->setStatusTip(tr("Open a wallet"));
    m_open_wallet_menu = new QMenu(this);

    m_close_wallet_action = new QAction(tr("Close Wallet…"), this);
    m_close_wallet_action->setStatusTip(tr("Close wallet"));

    m_create_wallet_action = new QAction(tr("Create Wallet…"), this);
    m_create_wallet_action->setEnabled(false);
    m_create_wallet_action->setStatusTip(tr("Create a new wallet"));

    //: Name of the menu item that restores wallet from a backup file.
    m_restore_wallet_action = new QAction(tr("Restore Wallet…"), this);
    m_restore_wallet_action->setEnabled(false);
    //: Status tip for Restore Wallet menu item
    m_restore_wallet_action->setStatusTip(tr("Restore a wallet from a backup file"));

    m_close_all_wallets_action = new QAction(tr("Close All Wallets…"), this);
    m_close_all_wallets_action->setStatusTip(tr("Close all wallets"));

    showHelpMessageAction = new QAction(tr("&Command-line options"), this);
    showHelpMessageAction->setMenuRole(QAction::NoRole);
    showHelpMessageAction->setStatusTip(tr("Show the %1 help message to get a list with possible Dash command-line options").arg(PACKAGE_NAME));

    showCoinJoinHelpAction = new QAction(tr("%1 &information").arg(strCoinJoinName), this);
    showCoinJoinHelpAction->setMenuRole(QAction::NoRole);
    showCoinJoinHelpAction->setStatusTip(tr("Show the %1 basic information").arg(strCoinJoinName));

    m_mask_values_action = new QAction(tr("&Discreet mode"), this);
    m_mask_values_action->setShortcut(QKeySequence(tr("Ctrl+Shift+D")));
    m_mask_values_action->setStatusTip(tr("Mask the values in the Overview tab"));
    m_mask_values_action->setCheckable(true);

    connect(quitAction, &QAction::triggered, this, &BitcoinGUI::quitRequested);
    connect(aboutAction, &QAction::triggered, this, &BitcoinGUI::aboutClicked);
    connect(aboutQtAction, &QAction::triggered, qApp, QApplication::aboutQt);
    connect(optionsAction, &QAction::triggered, this, &BitcoinGUI::optionsClicked);
    connect(showHelpMessageAction, &QAction::triggered, this, &BitcoinGUI::showHelpMessageClicked);
    connect(showCoinJoinHelpAction, &QAction::triggered, this, &BitcoinGUI::showCoinJoinHelpClicked);

    // Jump directly to tabs in RPC-console
    connect(openInfoAction, &QAction::triggered, this, &BitcoinGUI::showInfo);
    connect(openRPCConsoleAction, &QAction::triggered, this, &BitcoinGUI::showConsole);
    connect(openGraphAction, &QAction::triggered, this, &BitcoinGUI::showGraph);
    connect(openPeersAction, &QAction::triggered, this, &BitcoinGUI::showPeers);
    connect(openRepairAction, &QAction::triggered, this, &BitcoinGUI::showRepair);

    // Open configs and backup folder from menu
    connect(openConfEditorAction, &QAction::triggered, this, &BitcoinGUI::showConfEditor);
    connect(showBackupsAction, &QAction::triggered, this, &BitcoinGUI::showBackups);

    // Get restart command-line parameters and handle restart
    connect(rpcConsole, &RPCConsole::handleRestart, this, &BitcoinGUI::handleRestart);

    // prevents an open debug window from becoming stuck/unusable on client shutdown
    connect(quitAction, &QAction::triggered, rpcConsole, &QWidget::hide);

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
        connect(m_load_psbt_action, &QAction::triggered, [this]{ gotoLoadPSBT(); });
        connect(m_load_psbt_clipboard_action, &QAction::triggered, [this]{ gotoLoadPSBT(true); });
        connect(verifyMessageAction, &QAction::triggered, [this]{ showNormalIfMinimized(); });
        connect(verifyMessageAction, &QAction::triggered, [this]{ gotoVerifyMessageTab(); });
        connect(usedSendingAddressesAction, &QAction::triggered, walletFrame, &WalletFrame::usedSendingAddresses);
        connect(usedReceivingAddressesAction, &QAction::triggered, walletFrame, &WalletFrame::usedReceivingAddresses);
        connect(openAction, &QAction::triggered, this, &BitcoinGUI::openClicked);
        connect(m_open_wallet_menu, &QMenu::aboutToShow, [this] {
            m_open_wallet_menu->clear();
            for (const std::pair<const std::string, bool>& i : m_wallet_controller->listWalletDir()) {
                const std::string& path = i.first;
                QString name = path.empty() ? QString("["+tr("default wallet")+"]") : QString::fromStdString(path);
                // An single ampersand in the menu item's text sets a shortcut for this item.
                // Single & are shown when && is in the string. So replace & with &&.
                name.replace(QChar('&'), QString("&&"));
                QAction* action = m_open_wallet_menu->addAction(name);

                if (i.second) {
                    // This wallet is already loaded
                    action->setEnabled(false);
                    continue;
                }

                connect(action, &QAction::triggered, [this, path] {
                    auto activity = new OpenWalletActivity(m_wallet_controller, this);
                    connect(activity, &OpenWalletActivity::opened, this, &BitcoinGUI::setCurrentWallet, Qt::QueuedConnection);
                    connect(activity, &OpenWalletActivity::opened, rpcConsole, &RPCConsole::setCurrentWallet, Qt::QueuedConnection);
                    activity->open(path);
                });
            }
            if (m_open_wallet_menu->isEmpty()) {
                QAction* action = m_open_wallet_menu->addAction(tr("No wallets available"));
                action->setEnabled(false);
            }
        });
        connect(m_restore_wallet_action, &QAction::triggered, [this] {
            //: Name of the wallet data file format.
            QString name_data_file = tr("Wallet Data");

            //: The title for Restore Wallet File Windows
            QString title_windows = tr("Load Wallet Backup");

            QString backup_file = GUIUtil::getOpenFileName(this, title_windows, QString(), name_data_file + QLatin1String(" (*.dat)"), nullptr);
            if (backup_file.isEmpty()) return;

            bool wallet_name_ok;
            /*: Title of pop-up window shown when the user is attempting to
                restore a wallet. */
            QString title = tr("Restore Wallet");
            //: Label of the input field where the name of the wallet is entered.
            QString label = tr("Wallet Name");
            QString wallet_name = QInputDialog::getText(this, title, label, QLineEdit::Normal, "", &wallet_name_ok);
            if (!wallet_name_ok || wallet_name.isEmpty()) return;

            auto activity = new RestoreWalletActivity(m_wallet_controller, this);
            connect(activity, &RestoreWalletActivity::restored, this, &BitcoinGUI::setCurrentWallet, Qt::QueuedConnection);
            connect(activity, &RestoreWalletActivity::restored, rpcConsole, &RPCConsole::setCurrentWallet, Qt::QueuedConnection);

            auto backup_file_path = fs::PathFromString(backup_file.toStdString());
            activity->restore(backup_file_path, wallet_name.toStdString());
        });
        connect(m_close_wallet_action, &QAction::triggered, [this] {
            m_wallet_controller->closeWallet(walletFrame->currentWalletModel(), this);
        });
        connect(m_create_wallet_action, &QAction::triggered, [this] {
            auto activity = new CreateWalletActivity(m_wallet_controller, this);
            connect(activity, &CreateWalletActivity::created, this, &BitcoinGUI::setCurrentWallet);
            connect(activity, &CreateWalletActivity::created, rpcConsole, &RPCConsole::setCurrentWallet);
            activity->create();
        });
        connect(m_close_all_wallets_action, &QAction::triggered, [this] {
            m_wallet_controller->closeAllWallets(this);
        });

        connect(m_mask_values_action, &QAction::toggled, this, &BitcoinGUI::setPrivacy);
    }
#endif // ENABLE_WALLET
}

void BitcoinGUI::createMenuBar()
{
    appMenuBar = menuBar();

    // Configure the menus
    QMenu *file = appMenuBar->addMenu(tr("&File"));
    if(walletFrame)
    {
        file->addAction(m_create_wallet_action);
        file->addAction(m_open_wallet_action);
        file->addAction(m_close_wallet_action);
        file->addAction(m_close_all_wallets_action);
        file->addSeparator();
        file->addAction(backupWalletAction);
        file->addAction(m_restore_wallet_action);
        file->addSeparator();
        file->addAction(openAction);
        file->addAction(signMessageAction);
        file->addAction(verifyMessageAction);
        file->addAction(m_load_psbt_action);
        file->addAction(m_load_psbt_clipboard_action);
        file->addSeparator();
    }
    file->addAction(openConfEditorAction);
    if(walletFrame) {
        file->addAction(showBackupsAction);
    }
    file->addSeparator();
    file->addAction(quitAction);

    QMenu *settings = appMenuBar->addMenu(tr("&Settings"));
    if(walletFrame)
    {
        settings->addAction(encryptWalletAction);
        settings->addAction(changePassphraseAction);
        settings->addAction(unlockWalletAction);
        settings->addAction(lockWalletAction);
        settings->addSeparator();
        settings->addAction(m_mask_values_action);
        settings->addSeparator();
    }
    settings->addAction(optionsAction);

    QMenu* window_menu = appMenuBar->addMenu(tr("&Window"));

    QAction* minimize_action = window_menu->addAction(tr("&Minimize"));
    minimize_action->setShortcut(QKeySequence(tr("Ctrl+M")));
    connect(minimize_action, &QAction::triggered, [] {
        QApplication::activeWindow()->showMinimized();
    });
    connect(qApp, &QApplication::focusWindowChanged, this, [minimize_action] (QWindow* window) {
        minimize_action->setEnabled(window != nullptr && (window->flags() & Qt::Dialog) != Qt::Dialog && window->windowState() != Qt::WindowMinimized);
    });

#ifdef Q_OS_MACOS
    QAction* zoom_action = window_menu->addAction(tr("Zoom"));
    connect(zoom_action, &QAction::triggered, [] {
        QWindow* window = qApp->focusWindow();
        if (window->windowState() != Qt::WindowMaximized) {
            window->showMaximized();
        } else {
            window->showNormal();
        }
    });

    connect(qApp, &QApplication::focusWindowChanged, this, [zoom_action] (QWindow* window) {
        zoom_action->setEnabled(window != nullptr);
    });
#endif

    if (walletFrame) {
#ifdef Q_OS_MACOS
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
    help->addAction(showCoinJoinHelpAction);
    help->addSeparator();
    help->addAction(aboutAction);
    help->addAction(aboutQtAction);
}

void BitcoinGUI::createToolBars()
{
#ifdef ENABLE_WALLET
    if(walletFrame)
    {
        QToolBar *toolbar = new QToolBar(tr("Tabs toolbar"));
        appToolBar = toolbar;
        toolbar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        toolbar->setToolButtonStyle(Qt::ToolButtonTextOnly);
        toolbar->setMovable(false); // remove unused icon in upper left corner

        tabGroup = new QButtonGroup(this);

        overviewButton = new QToolButton(this);
        overviewButton->setText(tr("&Overview"));
        overviewButton->setStatusTip(tr("Show general overview of wallet"));
        tabGroup->addButton(overviewButton);

        sendCoinsButton = new QToolButton(this);
        sendCoinsButton->setText(sendCoinsAction->text());
        sendCoinsButton->setStatusTip(sendCoinsAction->statusTip());
        tabGroup->addButton(sendCoinsButton);

        receiveCoinsButton = new QToolButton(this);
        receiveCoinsButton->setText(receiveCoinsAction->text());
        receiveCoinsButton->setStatusTip(receiveCoinsAction->statusTip());
        tabGroup->addButton(receiveCoinsButton);

        historyButton = new QToolButton(this);
        historyButton->setText(tr("&Transactions"));
        historyButton->setStatusTip(tr("Browse transaction history"));
        tabGroup->addButton(historyButton);

        coinJoinCoinsButton = new QToolButton(this);
        coinJoinCoinsButton->setText(coinJoinCoinsAction->text());
        coinJoinCoinsButton->setStatusTip(coinJoinCoinsAction->statusTip());
        tabGroup->addButton(coinJoinCoinsButton);

        QSettings settings;
        if (settings.value("fShowMasternodesTab").toBool()) {
            masternodeButton = new QToolButton(this);
            masternodeButton->setText(tr("&Masternodes"));
            masternodeButton->setStatusTip(tr("Browse masternodes"));
            tabGroup->addButton(masternodeButton);
            connect(masternodeButton, &QToolButton::clicked, this, &BitcoinGUI::gotoMasternodePage);
            masternodeButton->setEnabled(true);
        }

        if (settings.value("fShowGovernanceTab").toBool()) {
            governanceButton = new QToolButton(this);
            governanceButton->setText(tr("&Governance"));
            governanceButton->setStatusTip(tr("View Governance Proposals"));
            tabGroup->addButton(governanceButton);
            connect(governanceButton, &QToolButton::clicked, this, &BitcoinGUI::gotoGovernancePage);
            governanceButton->setEnabled(true);
        }

        connect(overviewButton, &QToolButton::clicked, this, &BitcoinGUI::gotoOverviewPage);
        connect(sendCoinsButton, &QToolButton::clicked, [this]{ gotoSendCoinsPage(); });
        connect(coinJoinCoinsButton, &QToolButton::clicked, [this]{ gotoCoinJoinCoinsPage(); });
        connect(receiveCoinsButton, &QToolButton::clicked, this, &BitcoinGUI::gotoReceiveCoinsPage);
        connect(historyButton, &QToolButton::clicked, this, &BitcoinGUI::gotoHistoryPage);

        // Give the selected tab button a bolder font.
        connect(tabGroup, qOverload<QAbstractButton *, bool>(&QButtonGroup::buttonToggled), this, &BitcoinGUI::highlightTabButton);

        for (auto button : tabGroup->buttons()) {
            GUIUtil::setFont({button}, GUIUtil::FontWeight::Normal, 16);
            button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
            button->setToolTip(button->statusTip());
            button->setCheckable(true);
            toolbar->addWidget(button);
        }

        overviewButton->setChecked(true);
        GUIUtil::updateFonts();

#ifdef ENABLE_WALLET
        m_wallet_selector = new QComboBox(this);
        m_wallet_selector->setSizeAdjustPolicy(QComboBox::AdjustToContents);
        connect(m_wallet_selector, qOverload<int>(&QComboBox::currentIndexChanged), this, &BitcoinGUI::setCurrentWalletBySelectorIndex);

        QVBoxLayout* walletSelectorLayout = new QVBoxLayout(this);
        walletSelectorLayout->addWidget(m_wallet_selector);
        walletSelectorLayout->setSpacing(0);
        walletSelectorLayout->setMargin(0);
        walletSelectorLayout->setContentsMargins(5, 0, 5, 0);
        QWidget* walletSelector = new QWidget(this);
        walletSelector->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        walletSelector->setObjectName("walletSelector");
        walletSelector->setLayout(walletSelectorLayout);
        m_wallet_selector_action = appToolBar->insertWidget(appToolBarLogoAction, walletSelector);
        m_wallet_selector_action->setVisible(false);
#endif

        QLabel *logoLabel = new QLabel();
        logoLabel->setObjectName("lblToolbarLogo");
        logoLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

        appToolBarLogoAction = toolbar->addWidget(logoLabel);

        /** Create additional container for toolbar and walletFrame and make it the central widget.
            This is a workaround mostly for toolbar styling on Mac OS but should work fine for every other OSes too.
        */
        QVBoxLayout *layout = new QVBoxLayout;
        layout->addWidget(toolbar);
        layout->addWidget(walletFrame);
        layout->setSpacing(0);
        layout->setContentsMargins(QMargins());
        QWidget *containerWidget = new QWidget();
        containerWidget->setLayout(layout);
        setCentralWidget(containerWidget);
    }
#endif // ENABLE_WALLET
}

void BitcoinGUI::setClientModel(ClientModel *_clientModel, interfaces::BlockAndHeaderTipInfo* tip_info)
{
    this->clientModel = _clientModel;
    if(_clientModel)
    {
        // Create system tray menu (or setup the dock menu) that late to prevent users from calling actions,
        // while the client has not yet fully loaded
        if (trayIcon) {
            // do so only if trayIcon is already set
            trayIcon->setContextMenu(trayIconMenu.get());
            createIconMenu(trayIconMenu.get());

#ifndef Q_OS_MACOS
            // Show main window on tray icon click
            // Note: ignore this on Mac - this is not the way tray should work there
            connect(trayIcon, &QSystemTrayIcon::activated, [this](QSystemTrayIcon::ActivationReason reason) {
                if (reason == QSystemTrayIcon::Trigger) {
                    // Click on system tray icon triggers show/hide of the main window
                    toggleHidden();
                }
            });
#else
            // Note: on macOS, the Dock icon is also used to provide menu functionality
            // similar to one for tray icon
            dockIconMenu = new QMenu(this);
            dockIconMenu->setAsDockMenu();
            createIconMenu(dockIconMenu);

            MacDockIconHandler *dockIconHandler = MacDockIconHandler::instance();
            connect(dockIconHandler, &MacDockIconHandler::dockIconClicked, [this] {
                if (m_node.shutdownRequested()) return; // nothing to show, node is shutting down.
                showNormalIfMinimized();
                activateWindow();
            });
#endif
        }

        // Keep up to date with client
        setNetworkActive(m_node.getNetworkActive());
        setNumConnections(_clientModel->getNumConnections());
        connect(labelConnectionsIcon, &GUIUtil::ClickableLabel::clicked, [this] {
            GUIUtil::PopupMenu(m_network_context_menu, QCursor::pos());
        });
        connect(_clientModel, &ClientModel::numConnectionsChanged, this, &BitcoinGUI::setNumConnections);
        connect(_clientModel, &ClientModel::networkActiveChanged, this, &BitcoinGUI::setNetworkActive);

        modalOverlay->setKnownBestHeight(tip_info->header_height, QDateTime::fromSecsSinceEpoch(tip_info->header_time));
        setNumBlocks(tip_info->block_height, QDateTime::fromSecsSinceEpoch(tip_info->block_time), QString::fromStdString(tip_info->block_hash.ToString()), tip_info->verification_progress, false, SynchronizationState::INIT_DOWNLOAD);
        connect(_clientModel, &ClientModel::numBlocksChanged, this, &BitcoinGUI::setNumBlocks);

        connect(_clientModel, &ClientModel::additionalDataSyncProgressChanged, this, &BitcoinGUI::setAdditionalDataSyncProgress);

        // Receive and report messages from client model
        connect(_clientModel, &ClientModel::message, [this](const QString &title, const QString &message, unsigned int style){
            this->message(title, message, style);
        });

        // Show progress dialog
        connect(_clientModel, &ClientModel::showProgress, this, &BitcoinGUI::showProgress);

        rpcConsole->setClientModel(_clientModel, tip_info->block_height, tip_info->block_time, tip_info->block_hash, tip_info->verification_progress);

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

            connect(optionsModel, &OptionsModel::coinJoinEnabledChanged, this, &BitcoinGUI::updateCoinJoinVisibility);
        }
    } else {
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

#ifdef Q_OS_MACOS
        if(dockIconMenu)
        {
            // Disable context menu on dock icon
            dockIconMenu->clear();
        }
#endif
    }

    updateCoinJoinVisibility();
}

#ifdef ENABLE_WALLET
void BitcoinGUI::setWalletController(WalletController* wallet_controller)
{
    assert(!m_wallet_controller);
    assert(wallet_controller);

    m_wallet_controller = wallet_controller;

    m_create_wallet_action->setEnabled(true);
    m_open_wallet_action->setEnabled(true);
    m_open_wallet_action->setMenu(m_open_wallet_menu);
    m_restore_wallet_action->setEnabled(true);

    GUIUtil::ExceptionSafeConnect(wallet_controller, &WalletController::walletAdded, this, &BitcoinGUI::addWallet);
    connect(wallet_controller, &WalletController::walletRemoved, this, &BitcoinGUI::removeWallet);
    connect(wallet_controller, &WalletController::destroyed, this, [this] {
        // wallet_controller gets destroyed manually, but it leaves our member copy dangling
        m_wallet_controller = nullptr;
    });

    auto activity = new LoadWalletsActivity(m_wallet_controller, this);
    activity->load();
}

WalletController* BitcoinGUI::getWalletController()
{
    return m_wallet_controller;
}

void BitcoinGUI::addWallet(WalletModel* walletModel)
{
    if (!walletFrame || !m_wallet_controller) return;

    WalletView* wallet_view = new WalletView(walletModel, walletFrame);
    if (!walletFrame->addView(wallet_view)) return;

    rpcConsole->addWallet(walletModel);
    if (m_wallet_selector->count() == 0) {
        setWalletActionsEnabled(true);
    } else if (m_wallet_selector->count() == 1) {
        m_wallet_selector_action->setVisible(true);
    }

    connect(wallet_view, &WalletView::outOfSyncWarningClicked, this, &BitcoinGUI::showModalOverlay);
    connect(wallet_view, &WalletView::transactionClicked, this, &BitcoinGUI::gotoHistoryPage);
    connect(wallet_view, &WalletView::coinsSent, this, &BitcoinGUI::gotoHistoryPage);
    connect(wallet_view, &WalletView::message, [this](const QString& title, const QString& message, unsigned int style) {
        this->message(title, message, style);
    });
    connect(wallet_view, &WalletView::encryptionStatusChanged, this, &BitcoinGUI::updateWalletStatus);
    connect(wallet_view, &WalletView::incomingTransaction, this, &BitcoinGUI::incomingTransaction);
    connect(this, &BitcoinGUI::setPrivacy, wallet_view, &WalletView::setPrivacy);
    wallet_view->setPrivacy(isPrivacyModeActivated());
    const QString display_name = walletModel->getDisplayName();
    m_wallet_selector->addItem(display_name, QVariant::fromValue(walletModel));
}

void BitcoinGUI::removeWallet(WalletModel* walletModel)
{
    if (!walletFrame) return;

    labelWalletHDStatusIcon->hide();
    labelWalletEncryptionIcon->hide();

    int index = m_wallet_selector->findData(QVariant::fromValue(walletModel));
    m_wallet_selector->removeItem(index);
    if (m_wallet_selector->count() == 0) {
        setWalletActionsEnabled(false);
        overviewButton->setChecked(true);
    } else if (m_wallet_selector->count() == 1) {
        m_wallet_selector_action->setVisible(false);
    }
    rpcConsole->removeWallet(walletModel);
    walletFrame->removeWallet(walletModel);
    updateWindowTitle();
}

void BitcoinGUI::setCurrentWallet(WalletModel* wallet_model)
{
    if (!walletFrame || !m_wallet_controller) return;
    walletFrame->setCurrentWallet(wallet_model);
    for (int index = 0; index < m_wallet_selector->count(); ++index) {
        if (m_wallet_selector->itemData(index).value<WalletModel*>() == wallet_model) {
            m_wallet_selector->setCurrentIndex(index);
            break;
        }
    }
    updateWindowTitle();
}

void BitcoinGUI::setCurrentWalletBySelectorIndex(int index)
{
    WalletModel* wallet_model = m_wallet_selector->itemData(index).value<WalletModel*>();
    if (wallet_model) setCurrentWallet(wallet_model);
}

void BitcoinGUI::removeAllWallets()
{
    if(!walletFrame)
        return;
    setWalletActionsEnabled(false);
    walletFrame->removeAllWallets();
}
#endif // ENABLE_WALLET

void BitcoinGUI::setWalletActionsEnabled(bool enabled)
{
#ifdef ENABLE_WALLET
    if (walletFrame != nullptr) {
        // NOTE: overviewButton is always enabled
        sendCoinsButton->setEnabled(enabled);
        coinJoinCoinsButton->setEnabled(enabled && clientModel->coinJoinOptions().isEnabled());
        receiveCoinsButton->setEnabled(enabled);
        historyButton->setEnabled(enabled);
    }
#endif // ENABLE_WALLET

    sendCoinsAction->setEnabled(enabled);
#ifdef ENABLE_WALLET
    coinJoinCoinsAction->setEnabled(enabled && clientModel->coinJoinOptions().isEnabled());
#else
    coinJoinCoinsAction->setEnabled(enabled);
#endif // ENABLE_WALLET
    receiveCoinsAction->setEnabled(enabled);

    encryptWalletAction->setEnabled(enabled);
    backupWalletAction->setEnabled(enabled);
    changePassphraseAction->setEnabled(enabled);
    unlockWalletAction->setEnabled(enabled);
    lockWalletAction->setEnabled(enabled);
    signMessageAction->setEnabled(enabled);
    verifyMessageAction->setEnabled(enabled);
    usedSendingAddressesAction->setEnabled(enabled);
    usedReceivingAddressesAction->setEnabled(enabled);
    openAction->setEnabled(enabled);
    m_close_wallet_action->setEnabled(enabled);
    m_close_all_wallets_action->setEnabled(enabled);
}

void BitcoinGUI::createTrayIcon()
{
    assert(QSystemTrayIcon::isSystemTrayAvailable());

    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        trayIcon = new QSystemTrayIcon(m_network_style->getTrayAndWindowIcon(), this);
        QString toolTip = tr("%1 client").arg(PACKAGE_NAME) + " " + m_network_style->getTitleAddText();
        trayIcon->setToolTip(toolTip);
    }
}

void BitcoinGUI::createIconMenu(QMenu *pmenu)
{
    // Configuration of the tray icon (or dock icon) icon menu
    QAction* show_hide_action{nullptr};
#ifndef Q_OS_MACOS
    // Note: On macOS, the Dock icon is used to provide the tray's functionality.
    show_hide_action = pmenu->addAction(QString(), this, &BitcoinGUI::toggleHidden);
    pmenu->addSeparator();
#endif // Q_OS_MACOS

    QAction* send_action{nullptr};
    QAction* cj_send_action{nullptr};
    QAction* receive_action{nullptr};
    QAction* sign_action{nullptr};
    QAction* verify_action{nullptr};
    if (enableWallet) {
        send_action = pmenu->addAction(sendCoinsAction->text(), sendCoinsAction, &QAction::trigger);
        cj_send_action = pmenu->addAction(coinJoinCoinsAction->text(), coinJoinCoinsAction, &QAction::trigger);
        receive_action = pmenu->addAction(receiveCoinsAction->text(), receiveCoinsAction, &QAction::trigger);
        pmenu->addSeparator();
        sign_action = pmenu->addAction(signMessageAction->text(), signMessageAction, &QAction::trigger);
        verify_action = pmenu->addAction(verifyMessageAction->text(), verifyMessageAction, &QAction::trigger);
        pmenu->addSeparator();
    }
    QAction* options_action = pmenu->addAction(optionsAction->text(), optionsAction, &QAction::trigger);
    options_action->setMenuRole(QAction::PreferencesRole);
    QAction* info_action = pmenu->addAction(openInfoAction->text(), openInfoAction, &QAction::trigger);
    QAction* node_window_action = pmenu->addAction(openRPCConsoleAction->text(), openRPCConsoleAction, &QAction::trigger);
    QAction* graph_action = pmenu->addAction(openGraphAction->text(), openGraphAction, &QAction::trigger);
    QAction* peer_action = pmenu->addAction(openPeersAction->text(), openPeersAction, &QAction::trigger);
    QAction* repair_action{nullptr};
    if (enableWallet) {
        repair_action = pmenu->addAction(openRepairAction->text(), openRepairAction, &QAction::trigger);
    }
    pmenu->addSeparator();
    QAction* conf_action = pmenu->addAction(openConfEditorAction->text(), openConfEditorAction, &QAction::trigger);
    QAction* backups_action{nullptr};
    if (enableWallet) {
        backups_action = pmenu->addAction(showBackupsAction->text(), showBackupsAction, &QAction::trigger);
    }
    QAction* quit_action{nullptr};
#ifndef Q_OS_MACOS
    // Note: On macOS, the Dock icon's menu already has Quit action.
    pmenu->addSeparator();
    quit_action = pmenu->addAction(quitAction->text(), quitAction, &QAction::trigger);
#endif // Q_OS_MACOS

    connect(
        // Using QSystemTrayIcon::Context is not reliable.
        // See https://bugreports.qt.io/browse/QTBUG-91697
        pmenu, &QMenu::aboutToShow,
        [this, show_hide_action, send_action, cj_send_action, receive_action, sign_action, verify_action, options_action, node_window_action, quit_action, repair_action, backups_action, info_action, graph_action, peer_action, conf_action] {
            if (m_node.shutdownRequested()) return; // nothing to do, node is shutting down.

            if (show_hide_action) show_hide_action->setText(
                (!isHidden() && !isMinimized() && !GUIUtil::isObscured(this)) ?
                    tr("&Hide") :
                    tr("S&how"));
            if (QApplication::activeModalWidget()) {
                for (QAction* a : trayIconMenu.get()->actions()) {
                    a->setEnabled(false);
                }
            } else {
                if (show_hide_action) show_hide_action->setEnabled(true);
                if (enableWallet) {
                    send_action->setEnabled(sendCoinsAction->isEnabled());
                    cj_send_action->setEnabled(coinJoinCoinsAction->isEnabled());
                    receive_action->setEnabled(receiveCoinsAction->isEnabled());
                    sign_action->setEnabled(signMessageAction->isEnabled());
                    verify_action->setEnabled(verifyMessageAction->isEnabled());
                    repair_action->setEnabled(openRepairAction->isEnabled());
                    backups_action->setEnabled(showBackupsAction->isEnabled());
                }
                options_action->setEnabled(optionsAction->isEnabled());
                info_action->setEnabled(openInfoAction->isEnabled());
                node_window_action->setEnabled(openRPCConsoleAction->isEnabled());
                graph_action->setEnabled(openGraphAction->isEnabled());
                peer_action->setEnabled(openPeersAction->isEnabled());
                conf_action->setEnabled(openConfEditorAction->isEnabled());
                if (quit_action) quit_action->setEnabled(true);
            }
        });
}

void BitcoinGUI::optionsClicked()
{
    openOptionsDialogWithTab(OptionsDialog::TAB_MAIN);
}

void BitcoinGUI::aboutClicked()
{
    if(!clientModel)
        return;

    auto dlg = new HelpMessageDialog(this, HelpMessageDialog::about);
    GUIUtil::ShowModalDialogAsynchronously(dlg);
}

void BitcoinGUI::showDebugWindow()
{
    GUIUtil::bringToFront(rpcConsole);
    Q_EMIT consoleShown(rpcConsole);
}

void BitcoinGUI::showInfo()
{
    rpcConsole->setTabFocus(RPCConsole::TabTypes::INFO);
    showDebugWindow();
}

void BitcoinGUI::showConsole()
{
    rpcConsole->setTabFocus(RPCConsole::TabTypes::CONSOLE);
    showDebugWindow();
}

void BitcoinGUI::showGraph()
{
    rpcConsole->setTabFocus(RPCConsole::TabTypes::GRAPH);
    showDebugWindow();
}

void BitcoinGUI::showPeers()
{
    rpcConsole->setTabFocus(RPCConsole::TabTypes::PEERS);
    showDebugWindow();
}

void BitcoinGUI::showRepair()
{
    rpcConsole->setTabFocus(RPCConsole::TabTypes::REPAIR);
    showDebugWindow();
}

void BitcoinGUI::showConfEditor()
{
    GUIUtil::openConfigfile();
}

void BitcoinGUI::showBackups()
{
    GUIUtil::showBackups();
}

void BitcoinGUI::showHelpMessageClicked()
{
    GUIUtil::bringToFront(helpMessageDialog);
}

void BitcoinGUI::showCoinJoinHelpClicked()
{
    if(!clientModel)
        return;

    HelpMessageDialog dlg(this, HelpMessageDialog::pshelp);
    dlg.exec();
}

#ifdef ENABLE_WALLET
void BitcoinGUI::openClicked()
{
    OpenURIDialog dlg(this);
    if(dlg.exec())
    {
        Q_EMIT receivedURI(dlg.getURI());
    }
}

void BitcoinGUI::highlightTabButton(QAbstractButton *button, bool checked)
{
    GUIUtil::setFont({button}, checked ? GUIUtil::FontWeight::Bold : GUIUtil::FontWeight::Normal, 16);
    GUIUtil::updateFonts();
}

void BitcoinGUI::gotoGovernancePage()
{
    QSettings settings;
    if (settings.value("fShowGovernanceTab").toBool() && governanceButton) {
        governanceButton->setChecked(true);
        if (walletFrame) walletFrame->gotoGovernancePage();
    }
}

void BitcoinGUI::gotoOverviewPage()
{
    overviewButton->setChecked(true);
    if (walletFrame) walletFrame->gotoOverviewPage();
}

void BitcoinGUI::gotoHistoryPage()
{
    historyButton->setChecked(true);
    if (walletFrame) walletFrame->gotoHistoryPage();
}

void BitcoinGUI::gotoMasternodePage()
{
    QSettings settings;
    if (settings.value("fShowMasternodesTab").toBool() && masternodeButton) {
        masternodeButton->setChecked(true);
        if (walletFrame) walletFrame->gotoMasternodePage();
    }
}

void BitcoinGUI::gotoReceiveCoinsPage()
{
    receiveCoinsButton->setChecked(true);
    if (walletFrame) walletFrame->gotoReceiveCoinsPage();
}

void BitcoinGUI::gotoSendCoinsPage(QString addr)
{
    sendCoinsButton->setChecked(true);
    if (walletFrame) walletFrame->gotoSendCoinsPage(addr);
}

void BitcoinGUI::gotoCoinJoinCoinsPage(QString addr)
{
    coinJoinCoinsButton->setChecked(true);
    if (walletFrame) walletFrame->gotoCoinJoinCoinsPage(addr);
}

void BitcoinGUI::gotoSignMessageTab(QString addr)
{
    if (walletFrame) walletFrame->gotoSignMessageTab(addr);
}

void BitcoinGUI::gotoVerifyMessageTab(QString addr)
{
    if (walletFrame) walletFrame->gotoVerifyMessageTab(addr);
}
void BitcoinGUI::gotoLoadPSBT(bool from_clipboard)
{
    if (walletFrame) walletFrame->gotoLoadPSBT(from_clipboard);
}
#endif // ENABLE_WALLET

void BitcoinGUI::updateNetworkState()
{
    if (!clientModel) return;
    static int nCountPrev{0};
    static bool fNetworkActivePrev{false};
    int count = clientModel->getNumConnections();
    bool fNetworkActive = m_node.getNetworkActive();
    QString icon;
    GUIUtil::ThemedColor color = GUIUtil::ThemedColor::ORANGE;
    switch(count)
    {
    case 0: icon = "connect_4"; color = GUIUtil::ThemedColor::ICON_ALTERNATIVE_COLOR; break;
    case 1: case 2: icon = "connect_1"; break;
    case 3: case 4: case 5: icon = "connect_2"; break;
    case 6: case 7: icon = "connect_3"; break;
    default: icon = "connect_4"; color = GUIUtil::ThemedColor::GREEN; break;
    }

    labelBlocksIcon->setVisible(count > 0);
    updateProgressBarVisibility();

    bool fNetworkBecameActive = (!fNetworkActivePrev && fNetworkActive) || (nCountPrev == 0 && count > 0);
    bool fNetworkBecameInactive = (fNetworkActivePrev && !fNetworkActive) || (nCountPrev > 0 && count == 0);

    if (fNetworkBecameActive) {
        // If the sync process still signals synced after five seconds represent it in the UI.
        if (m_node.masternodeSync().isSynced()) {
            QTimer::singleShot(5000, this, [&]() {
                if (clientModel->getNumConnections() > 0 && m_node.masternodeSync().isSynced()) {
                    setAdditionalDataSyncProgress(1);
                }
            });
        }
        startSpinner();
    } else if (fNetworkBecameInactive) {
        labelBlocksIcon->hide();
        stopSpinner();
    }

    if (fNetworkBecameActive || fNetworkBecameInactive) {
        setNumBlocks(m_node.getNumBlocks(), QDateTime::fromSecsSinceEpoch(m_node.getLastBlockTime()), QString::fromStdString(m_node.getLastBlockHash()), m_node.getVerificationProgress(), false, SynchronizationState::INIT_DOWNLOAD);
    }

    nCountPrev = count;
    fNetworkActivePrev = fNetworkActive;

    QString tooltip;
    if (fNetworkActive) {
        //: A substring of the tooltip.
        tooltip = tr("%n active connection(s) to Dash network", "", count);
    } else {
        tooltip = tr("Network activity disabled");
        icon = "connect_4";
        color = GUIUtil::ThemedColor::RED;
    }

    // Don't word-wrap this (fixed-width) tooltip
    tooltip = QLatin1String("<nobr>") + tooltip + QLatin1String("<br>") +
              //: A substring of the tooltip. "More actions" are available via the context menu.
              tr("Click for more actions.") + QLatin1String("</nobr>");

    if (fNetworkActive && count == 0) {
        startConnectingAnimation();
    }
    if (!fNetworkActive || count > 0) {
        stopConnectingAnimation();
        labelConnectionsIcon->setPixmap(GUIUtil::getIcon(icon, color).pixmap(STATUSBAR_ICONSIZE, STATUSBAR_ICONSIZE));
    }
    labelConnectionsIcon->setToolTip(tooltip);
}

void BitcoinGUI::setNumConnections(int count)
{
    updateNetworkState();
}

void BitcoinGUI::setNetworkActive(bool network_active)
{
    updateNetworkState();
    m_network_context_menu->clear();
    m_network_context_menu->addAction(
        //: A context menu item. The "Peers tab" is an element of the "Node window".
        tr("Show Peers tab"),
        [this] {
            rpcConsole->setTabFocus(RPCConsole::TabTypes::PEERS);
            showDebugWindow();
        });
    m_network_context_menu->addAction(
        network_active ?
            //: A context menu item.
            tr("Disable network activity") :
            //: A context menu item. The network activity was disabled previously.
            tr("Enable network activity"),
        [this, new_state = !network_active] { m_node.setNetworkActive(new_state); });
}

void BitcoinGUI::updateHeadersSyncProgressLabel()
{
    int64_t headersTipTime = clientModel->getHeaderTipTime();
    int headersTipHeight = clientModel->getHeaderTipHeight();
    int estHeadersLeft = (GetTime() - headersTipTime) / Params().GetConsensus().nPowTargetSpacing;
    if (estHeadersLeft > HEADER_HEIGHT_DELTA_SYNC)
        progressBarLabel->setText(tr("Syncing Headers (%1%)…").arg(QString::number(100.0 / (headersTipHeight+estHeadersLeft)*headersTipHeight, 'f', 1)));
}

void BitcoinGUI::openOptionsDialogWithTab(OptionsDialog::Tab tab)
{
    if (!clientModel || !clientModel->getOptionsModel())
        return;

    auto dlg = new OptionsDialog(this, enableWallet);
    connect(dlg, &OptionsDialog::quitOnReset, this, &BitcoinGUI::quitRequested);
    dlg->setCurrentTab(tab);
    dlg->setModel(clientModel->getOptionsModel());
    connect(dlg, &OptionsDialog::appearanceChanged, [this]() {
        updateWidth();
    });
    GUIUtil::ShowModalDialogAsynchronously(dlg);

    updateCoinJoinVisibility();
}

void BitcoinGUI::updateProgressBarVisibility()
{
    if (clientModel == nullptr) {
        return;
    }
    // Show the progress bar label if the network is active + we are out of sync or we have no connections.
    bool fShowProgressBarLabel = m_node.getNetworkActive() && (!m_node.masternodeSync().isSynced() || clientModel->getNumConnections() == 0);
    // Show the progress bar only if the the network active + we are not synced + we have any connection. Unlike with the label
    // which gives an info text about the connecting phase there is no reason to show the progress bar if we don't have connections
    // since it will not get any updates in this case.
    bool fShowProgressBar = m_node.getNetworkActive() && !m_node.masternodeSync().isSynced() && clientModel->getNumConnections() > 0;
    progressBarLabel->setVisible(fShowProgressBarLabel);
    progressBar->setVisible(fShowProgressBar);
}

void BitcoinGUI::updateCoinJoinVisibility()
{
#ifdef ENABLE_WALLET
    bool fEnabled = m_node.coinJoinOptions().isEnabled();
#else
    bool fEnabled = false;
#endif
    // CoinJoin button is the third QToolButton, show/hide the underlying QAction
    // Hiding the QToolButton itself doesn't work for the GUI part
    // but is still needed for shortcuts to work properly.
    if (appToolBar != nullptr) {
        appToolBar->actions()[4]->setVisible(fEnabled);
        coinJoinCoinsButton->setVisible(fEnabled);
        GUIUtil::updateButtonGroupShortcuts(tabGroup);
    }
    coinJoinCoinsAction->setVisible(fEnabled);
    showCoinJoinHelpAction->setVisible(fEnabled);
    updateWidth();
}

void BitcoinGUI::updateWidth()
{
    if (walletFrame == nullptr) {
        return;
    }
    if (windowState() & (Qt::WindowMaximized | Qt::WindowFullScreen)) {
        return;
    }
    int nWidthWidestButton{0};
    int nButtonsVisible{0};
    for (QAbstractButton* button : tabGroup->buttons()) {
        if (!button->isEnabled()) {
            continue;
        }
        QFontMetrics fm(button->font());
        nWidthWidestButton = std::max<int>(nWidthWidestButton, GUIUtil::TextWidth(fm, button->text()));
        ++nButtonsVisible;
    }
    // Add 30 per button as padding and use minimum 980 which is the minimum required to show all tab's contents
    // Use nButtonsVisible + 1 <- for the dash logo
    int nWidth = std::max<int>(980, (nWidthWidestButton + 30) * (nButtonsVisible + 1));
    setMinimumWidth(nWidth);

    // Resize to new minimum width but don't shrink window
    resize(std::max(width(), nWidth), height());
}

void BitcoinGUI::setNumBlocks(int count, const QDateTime& blockDate, const QString& blockHash, double nVerificationProgress, bool header, SynchronizationState sync_state)
{
#ifdef Q_OS_MACOS
    // Disabling macOS App Nap on initial sync, disk, reindex operations and mixing.
    bool disableAppNap = !m_node.masternodeSync().isSynced() || sync_state != SynchronizationState::POST_INIT;
#ifdef ENABLE_WALLET
    if (enableWallet) {
        for (const auto& wallet : m_node.walletLoader().getWallets()) {
            disableAppNap |= m_node.coinJoinLoader()->GetClient(wallet->getWalletName())->isMixing();
        }
    }
#endif // ENABLE_WALLET
    if (disableAppNap) {
        m_app_nap_inhibitor->disableAppNap();
    } else {
        m_app_nap_inhibitor->enableAppNap();
    }
#endif // Q_OS_MACOS

    if (modalOverlay)
    {
        if (header)
            modalOverlay->setKnownBestHeight(count, blockDate);
        else
            modalOverlay->tipUpdate(count, blockDate, nVerificationProgress);
    }
    if (!clientModel)
        return;

    updateProgressBarVisibility();

    // Prevent orphan statusbar messages (e.g. hover Quit in main menu, wait until chain-sync starts -> garbled text)
    statusBar()->clearMessage();

    // Acquire current block source
    BlockSource blockSource{clientModel->getBlockSource()};
    switch (blockSource) {
        case BlockSource::NETWORK:
            if (header) {
                updateHeadersSyncProgressLabel();
                return;
            }
            progressBarLabel->setText(tr("Synchronizing with network…"));
            updateHeadersSyncProgressLabel();
            break;
        case BlockSource::DISK:
            if (header) {
                progressBarLabel->setText(tr("Indexing blocks on disk…"));
            } else {
                progressBarLabel->setText(tr("Processing blocks on disk…"));
            }
            break;
        case BlockSource::NONE:
            if (header) {
                return;
            }
            progressBarLabel->setText(tr("Connecting to peers…"));
            break;
    }

    QString tooltip;

    QDateTime currentDate = QDateTime::currentDateTime();
    qint64 secs = blockDate.secsTo(currentDate);

    tooltip = tr("Processed %n block(s) of transaction history.", "", count);

    // Set icon state: spinning if catching up, tick otherwise
#ifdef ENABLE_WALLET
    if (walletFrame) {
        if(secs < MAX_BLOCK_TIME_GAP) {
            modalOverlay->showHide(true, true);
            // TODO instead of hiding it forever, we should add meaningful information about MN sync to the overlay
            modalOverlay->hideForever();
        } else {
            modalOverlay->showHide();
        }
    }
#endif // ENABLE_WALLET

    if(!m_node.masternodeSync().isBlockchainSynced())
    {
        QString timeBehindText = GUIUtil::formatNiceTimeOffset(secs);

        progressBar->setFormat(tr("%1 behind").arg(timeBehindText));
        progressBar->setMaximum(1000000000);
        progressBar->setValue(nVerificationProgress * 1000000000.0 + 0.5);

        tooltip = tr("Catching up…") + QString("<br>") + tooltip;

#ifdef ENABLE_WALLET
        if(walletFrame)
        {
            walletFrame->showOutOfSyncWarning(true);
        }
#endif // ENABLE_WALLET

        tooltip += QString("<br>");
        tooltip += tr("Last received block was generated %1 ago.").arg(timeBehindText);
        tooltip += QString("<br>");
        tooltip += tr("Transactions after this will not yet be visible.");
    } else if (!m_node.gov().isEnabled()) {
        setAdditionalDataSyncProgress(1);
    }

    // Don't word-wrap this (fixed-width) tooltip
    tooltip = QString("<nobr>") + tooltip + QString("</nobr>");

    labelBlocksIcon->setToolTip(tooltip);
    progressBarLabel->setToolTip(tooltip);
    progressBar->setToolTip(tooltip);
}

void BitcoinGUI::setAdditionalDataSyncProgress(double nSyncProgress)
{
    if(!clientModel)
        return;

    // If masternodeSync->Reset() has been called make sure status bar shows the correct information.
    if (nSyncProgress == -1) {
        setNumBlocks(m_node.getNumBlocks(), QDateTime::fromSecsSinceEpoch(m_node.getLastBlockTime()), QString::fromStdString(m_node.getLastBlockHash()), m_node.getVerificationProgress(), false, SynchronizationState::INIT_DOWNLOAD);
        if (clientModel->getNumConnections()) {
            labelBlocksIcon->show();
            startSpinner();
        }
        return;
    }

    // No additional data sync should be happening while blockchain is not synced, nothing to update
    if(!m_node.masternodeSync().isBlockchainSynced())
        return;

    // Prevent orphan statusbar messages (e.g. hover Quit in main menu, wait until chain-sync starts -> garbelled text)
    statusBar()->clearMessage();

    QString tooltip;

    // Set icon state: spinning if catching up, tick otherwise
    QString strSyncStatus;
    tooltip = tr("Up to date") + QString(".<br>") + tooltip;

#ifdef ENABLE_WALLET
    if(walletFrame)
        walletFrame->showOutOfSyncWarning(false);
#endif // ENABLE_WALLET

    updateProgressBarVisibility();

    if(m_node.masternodeSync().isSynced()) {
        stopSpinner();
        labelBlocksIcon->setPixmap(GUIUtil::getIcon("synced", GUIUtil::ThemedColor::GREEN).pixmap(STATUSBAR_ICONSIZE, STATUSBAR_ICONSIZE));
    } else {
        progressBar->setFormat(tr("Synchronizing additional data: %p%"));
        progressBar->setMaximum(1000000000);
        progressBar->setValue(nSyncProgress * 1000000000.0 + 0.5);
    }

    strSyncStatus = QString(m_node.masternodeSync().getSyncStatus().c_str());
    progressBarLabel->setText(strSyncStatus);
    tooltip = strSyncStatus + QString("<br>") + tooltip;

    // Don't word-wrap this (fixed-width) tooltip
    tooltip = QString("<nobr>") + tooltip + QString("</nobr>");

    labelBlocksIcon->setToolTip(tooltip);
    progressBarLabel->setToolTip(tooltip);
    progressBar->setToolTip(tooltip);
}

void BitcoinGUI::message(const QString& title, QString message, unsigned int style, bool* ret, const QString& detailed_message)
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

void BitcoinGUI::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
#ifndef Q_OS_MACOS // Ignored on macOS
    if(e->type() == QEvent::WindowStateChange)
    {
        if(clientModel && clientModel->getOptionsModel() && clientModel->getOptionsModel()->getMinimizeToTray())
        {
            QWindowStateChangeEvent *wsevt = static_cast<QWindowStateChangeEvent*>(e);
            if(!(wsevt->oldState() & Qt::WindowMinimized) && isMinimized())
            {
                QTimer::singleShot(0, this, &BitcoinGUI::hide);
                e->ignore();
            }
            else if((wsevt->oldState() & Qt::WindowMinimized) && !isMinimized())
            {
                QTimer::singleShot(0, this, &BitcoinGUI::show);
                e->ignore();
            }
        }
    }
#endif
    if (e->type() == QEvent::StyleChange) {
        updateNetworkState();
#ifdef ENABLE_WALLET
        if (walletFrame) {
            updateWalletStatus();
        }
#endif
        if (m_node.masternodeSync().isSynced()) {
            labelBlocksIcon->setPixmap(GUIUtil::getIcon("synced", GUIUtil::ThemedColor::GREEN).pixmap(STATUSBAR_ICONSIZE, STATUSBAR_ICONSIZE));
        }
    }
}

void BitcoinGUI::closeEvent(QCloseEvent *event)
{
#ifndef Q_OS_MACOS // Ignored on macOS
    if(clientModel && clientModel->getOptionsModel())
    {
        if(!clientModel->getOptionsModel()->getMinimizeOnClose())
        {
            // close rpcConsole in case it was open to make some space for the shutdown window
            rpcConsole->close();

            Q_EMIT quitRequested();
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

void BitcoinGUI::showEvent(QShowEvent *event)
{
    // enable the debug window when the main window shows up
    openInfoAction->setEnabled(true);
    openRPCConsoleAction->setEnabled(true);
    openGraphAction->setEnabled(true);
    openPeersAction->setEnabled(true);
    openRepairAction->setEnabled(true);
    aboutAction->setEnabled(true);
    optionsAction->setEnabled(true);

    if (!event->spontaneous()) {
        updateCoinJoinVisibility();
    }
}

#ifdef ENABLE_WALLET
void BitcoinGUI::incomingTransaction(const QString& date, BitcoinUnit unit, const CAmount& amount, const QString& type, const QString& address, const QString& label, const QString& walletName)
{
    IncomingTransactionMessage itx = {
            date, unit, amount, type, address, label, walletName
    };
    incomingTransactions.emplace_back(itx);

    if (incomingTransactions.size() == 1) {
        // first TX since we last showed pending messages, let's wait 100ms and then show each individual message
        incomingTransactionsTimer->start(100);
    } else if (incomingTransactions.size() == 10) {
        // we seem to have received 10 TXs in 100ms and we can expect even more, so let's pause for 1 sec and
        // show a "Multiple TXs sent/received!" message instead of individual messages
        incomingTransactionsTimer->start(1000);
    }
}
void BitcoinGUI::showIncomingTransactions()
{
    auto txs = std::move(this->incomingTransactions);

    if (txs.empty()) {
        return;
    }

    if (txs.size() >= 100) {
        // Show one balloon for all transactions instead of showing one for each individual one
        // (which would kill some systems)

        CAmount sentAmount = 0;
        CAmount receivedAmount = 0;
        int sentCount = 0;
        int receivedCount = 0;
        for (auto& itx : txs) {
            if (itx.amount < 0) {
                sentAmount += itx.amount;
                sentCount++;
            } else {
                receivedAmount += itx.amount;
                receivedCount++;
            }
        }

        QString title;
        if (sentCount > 0 && receivedCount > 0) {
            title = tr("Received and sent multiple transactions");
        } else if (sentCount > 0) {
            title = tr("Sent multiple transactions");
        } else if (receivedCount > 0) {
            title = tr("Received multiple transactions");
        } else {
            return;
        }

        // Use display unit of last entry
        BitcoinUnit unit = txs.back().unit;

        QString msg;
        if (sentCount > 0) {
            msg += tr("Sent Amount: %1\n").arg(BitcoinUnits::formatWithUnit(unit, sentAmount, true));
        }
        if (receivedCount > 0) {
            msg += tr("Received Amount: %1\n").arg(BitcoinUnits::formatWithUnit(unit, receivedAmount, true));
        }

        message(title, msg, CClientUIInterface::MSG_INFORMATION);
    } else {
        for (auto& itx : txs) {
            // On new transaction, make an info balloon
            QString msg = tr("Date: %1\n").arg(itx.date) +
                          tr("Amount: %1\n").arg(BitcoinUnits::formatWithUnit(itx.unit, itx.amount, true));
            if (m_node.walletLoader().getWallets().size() > 1 && !itx.walletName.isEmpty()) {
                msg += tr("Wallet: %1\n").arg(itx.walletName);
            }
            msg += tr("Type: %1\n").arg(itx.type);
            if (!itx.label.isEmpty())
                msg += tr("Label: %1\n").arg(itx.label);
            else if (!itx.address.isEmpty())
                msg += tr("Address: %1\n").arg(itx.address);
            message((itx.amount)<0 ? tr("Sent transaction") : tr("Incoming transaction"),
                    msg, CClientUIInterface::MSG_INFORMATION);
        }
    }
}
#endif // ENABLE_WALLET

void BitcoinGUI::dragEnterEvent(QDragEnterEvent *event)
{
    // Accept only URIs
    if(event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void BitcoinGUI::dropEvent(QDropEvent *event)
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

bool BitcoinGUI::eventFilter(QObject *object, QEvent *event)
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
bool BitcoinGUI::handlePaymentRequest(const SendCoinsRecipient& recipient)
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

void BitcoinGUI::setHDStatus(int hdEnabled)
{
    if (hdEnabled) {
        labelWalletHDStatusIcon->setPixmap(GUIUtil::getIcon("hd_enabled", GUIUtil::ThemedColor::GREEN).pixmap(STATUSBAR_ICONSIZE, STATUSBAR_ICONSIZE));
        labelWalletHDStatusIcon->setToolTip(tr("HD key generation is <b>enabled</b>"));
        labelWalletHDStatusIcon->show();
    }
    labelWalletHDStatusIcon->setVisible(hdEnabled);
}

void BitcoinGUI::setEncryptionStatus(int status)
{
    switch(status)
    {
    case WalletModel::NoKeys:
        labelWalletEncryptionIcon->hide();
        encryptWalletAction->setChecked(false);
        changePassphraseAction->setEnabled(false);
        encryptWalletAction->setEnabled(false);
        break;
    case WalletModel::Unencrypted:
        labelWalletEncryptionIcon->show();
        labelWalletEncryptionIcon->setPixmap(GUIUtil::getIcon("lock_open", GUIUtil::ThemedColor::RED).pixmap(STATUSBAR_ICONSIZE, STATUSBAR_ICONSIZE));
        labelWalletEncryptionIcon->setToolTip(tr("Wallet is <b>unencrypted</b>"));
        changePassphraseAction->setEnabled(false);
        unlockWalletAction->setVisible(false);
        lockWalletAction->setVisible(false);
        encryptWalletAction->setEnabled(true);
        break;
    case WalletModel::Unlocked:
        labelWalletEncryptionIcon->show();
        labelWalletEncryptionIcon->setPixmap(GUIUtil::getIcon("lock_open", GUIUtil::ThemedColor::RED).pixmap(STATUSBAR_ICONSIZE, STATUSBAR_ICONSIZE));
        labelWalletEncryptionIcon->setToolTip(tr("Wallet is <b>encrypted</b> and currently <b>unlocked</b>"));
        changePassphraseAction->setEnabled(true);
        unlockWalletAction->setVisible(false);
        lockWalletAction->setVisible(true);
        encryptWalletAction->setEnabled(false);
        break;
    case WalletModel::UnlockedForMixingOnly:
        labelWalletEncryptionIcon->show();
        labelWalletEncryptionIcon->setPixmap(GUIUtil::getIcon("lock_open", GUIUtil::ThemedColor::ORANGE).pixmap(STATUSBAR_ICONSIZE, STATUSBAR_ICONSIZE));
        labelWalletEncryptionIcon->setToolTip(tr("Wallet is <b>encrypted</b> and currently <b>unlocked</b> for mixing only"));
        changePassphraseAction->setEnabled(true);
        unlockWalletAction->setVisible(true);
        lockWalletAction->setVisible(true);
        encryptWalletAction->setEnabled(false);
        break;
    case WalletModel::Locked:
        labelWalletEncryptionIcon->show();
        labelWalletEncryptionIcon->setPixmap(GUIUtil::getIcon("lock_closed", GUIUtil::ThemedColor::GREEN).pixmap(STATUSBAR_ICONSIZE, STATUSBAR_ICONSIZE));
        labelWalletEncryptionIcon->setToolTip(tr("Wallet is <b>encrypted</b> and currently <b>locked</b>"));
        changePassphraseAction->setEnabled(true);
        unlockWalletAction->setVisible(true);
        lockWalletAction->setVisible(false);
        encryptWalletAction->setEnabled(false);
        break;
    }
}

void BitcoinGUI::updateWalletStatus()
{
    assert(walletFrame);

    WalletView * const walletView = walletFrame->currentWalletView();
    if (!walletView) {
        return;
    }
    WalletModel * const walletModel = walletView->getWalletModel();
    setEncryptionStatus(walletModel->getEncryptionStatus());
    setHDStatus(walletModel->wallet().hdEnabled());
}
#endif // ENABLE_WALLET

void BitcoinGUI::updateProxyIcon()
{
    std::string ip_port;
    bool proxy_enabled = clientModel->getProxyInfo(ip_port);

    if (proxy_enabled) {
        if (!GUIUtil::HasPixmap(labelProxyIcon)) {
            QString ip_port_q = QString::fromStdString(ip_port);
            labelProxyIcon->setPixmap(GUIUtil::getIcon("proxy", GUIUtil::ThemedColor::GREEN).pixmap(STATUSBAR_ICONSIZE, STATUSBAR_ICONSIZE));
            labelProxyIcon->setToolTip(tr("Proxy is <b>enabled</b>: %1").arg(ip_port_q));
        } else {
            labelProxyIcon->show();
        }
    } else {
        labelProxyIcon->hide();
    }
}

void BitcoinGUI::updateWindowTitle()
{
    QString window_title = PACKAGE_NAME;
#ifdef ENABLE_WALLET
    if (walletFrame) {
        WalletModel* const wallet_model = walletFrame->currentWalletModel();
        QString userWindowTitle = QString::fromStdString(gArgs.GetArg("-windowtitle", ""));
        if (!userWindowTitle.isEmpty()) {
            window_title += " - " + userWindowTitle;
        }
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

void BitcoinGUI::showNormalIfMinimized(bool fToggleHidden)
{
    if(!clientModel)
        return;

    if (!isHidden() && !isMinimized() && !GUIUtil::isObscured(this) && fToggleHidden) {
        hide();
    } else {
        GUIUtil::bringToFront(this);
    }
}

void BitcoinGUI::toggleHidden()
{
    showNormalIfMinimized(true);
}

void BitcoinGUI::detectShutdown()
{
    if (m_node.shutdownRequested())
    {
        if(rpcConsole)
            rpcConsole->hide();
        Q_EMIT quitRequested();
    }
}

void BitcoinGUI::showProgress(const QString &title, int nProgress)
{
    if (nProgress == 0) {
        progressDialog = new QProgressDialog(title, QString(), 0, 100, this);
        GUIUtil::PolishProgressDialog(progressDialog);
        progressDialog->setWindowModality(Qt::ApplicationModal);
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

void BitcoinGUI::showModalOverlay()
{
    if (modalOverlay && (progressBar->isVisible() || modalOverlay->isLayerVisible()))
        modalOverlay->toggleVisibility();
}

static bool ThreadSafeMessageBox(BitcoinGUI* gui, const bilingual_str& message, const std::string& caption, unsigned int style)
{
    bool modal = (style & CClientUIInterface::MODAL);
    // The SECURE flag has no effect in the Qt GUI.
    // bool secure = (style & CClientUIInterface::SECURE);
    style &= ~CClientUIInterface::SECURE;
    bool ret = false;

    QString detailed_message; // This is original message, in English, for googling and referencing.
    if (message.original != message.translated) {
        detailed_message = BitcoinGUI::tr("Original message:") + "\n" + QString::fromStdString(message.original);
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

void BitcoinGUI::subscribeToCoreSignals()
{
    // Connect signals to client
    m_handler_message_box = m_node.handleMessageBox(std::bind(ThreadSafeMessageBox, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    m_handler_question = m_node.handleQuestion(std::bind(ThreadSafeMessageBox, this, std::placeholders::_1, std::placeholders::_3, std::placeholders::_4));
}

void BitcoinGUI::unsubscribeFromCoreSignals()
{
    // Disconnect signals from client
    m_handler_message_box->disconnect();
    m_handler_question->disconnect();
}

/** Get restart command-line parameters and request restart */
void BitcoinGUI::handleRestart(QStringList args)
{
    if (!m_node.shutdownRequested())
        Q_EMIT requestedRestart(args);
}

bool BitcoinGUI::isPrivacyModeActivated() const
{
    assert(m_mask_values_action);
    return m_mask_values_action->isChecked();
}

UnitDisplayStatusBarControl::UnitDisplayStatusBarControl() :
    optionsModel(nullptr),
    menu(nullptr)
{
    createContextMenu();
    setToolTip(tr("Unit to show amounts in. Click to select another unit."));
    QList<BitcoinUnit> units = BitcoinUnits::availableUnits();
    int max_width = 0;
    const QFontMetrics fm(GUIUtil::getFontNormal());
    for (const BitcoinUnit unit : units) {
        max_width = qMax(max_width, GUIUtil::TextWidth(fm, BitcoinUnits::name(unit)));
    }
    setMinimumSize(max_width, 0);
    setAlignment(Qt::AlignRight | Qt::AlignVCenter);
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
    for (const BitcoinUnit u : BitcoinUnits::availableUnits()) {
        menu->addAction(BitcoinUnits::name(u))->setData(QVariant::fromValue(u));
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
void UnitDisplayStatusBarControl::updateDisplayUnit(BitcoinUnit newUnits)
{
    setText(BitcoinUnits::name(newUnits));
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
