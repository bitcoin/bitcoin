// Copyright (c) 2011-2020 The Bitcoin Core developers
// Copyright (c) 2014-2022 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <qt/bitcoin.h>
#include <qt/bitcoingui.h>

#include <chainparams.h>
#include <fs.h>
#include <qt/clientmodel.h>
#include <qt/guiconstants.h>
#include <qt/guiutil.h>
#include <qt/intro.h>
#include <net.h>
#include <qt/networkstyle.h>
#include <qt/optionsmodel.h>
#include <qt/splashscreen.h>
#include <qt/utilitydialog.h>
#include <qt/winshutdownmonitor.h>

#ifdef ENABLE_WALLET
#include <qt/paymentserver.h>
#include <qt/walletcontroller.h>
#include <qt/walletmodel.h>
#endif // ENABLE_WALLET

#include <interfaces/handler.h>
#include <interfaces/node.h>
#include <node/context.h>
#include <noui.h>
#include <stacktraces.h>
#include <ui_interface.h>
#include <uint256.h>
#include <util/system.h>
#include <util/translation.h>

#include <memory>

#include <QApplication>
#include <QDebug>
#include <QLibraryInfo>
#include <QLocale>
#include <QMessageBox>
#include <QProcess>
#include <QSettings>
#include <QThread>
#include <QTimer>
#include <QTranslator>

#if defined(QT_STATICPLUGIN)
#include <QtPlugin>
#if defined(QT_QPA_PLATFORM_XCB)
Q_IMPORT_PLUGIN(QXcbIntegrationPlugin);
#elif defined(QT_QPA_PLATFORM_WINDOWS)
Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin);
#elif defined(QT_QPA_PLATFORM_COCOA)
Q_IMPORT_PLUGIN(QCocoaIntegrationPlugin);
Q_IMPORT_PLUGIN(QMacStylePlugin);
#endif
#endif

// Declare meta types used for QMetaObject::invokeMethod
Q_DECLARE_METATYPE(bool*)
Q_DECLARE_METATYPE(CAmount)
Q_DECLARE_METATYPE(uint256)

static QString GetLangTerritory()
{
    QSettings settings;
    // Get desired locale (e.g. "de_DE")
    // 1) System default language
    QString lang_territory = QLocale::system().name();
    // 2) Language from QSettings
    QString lang_territory_qsettings = settings.value("language", "").toString();
    if(!lang_territory_qsettings.isEmpty())
        lang_territory = lang_territory_qsettings;
    // 3) -lang command line argument
    lang_territory = QString::fromStdString(gArgs.GetArg("-lang", lang_territory.toStdString()));
    return lang_territory;
}

/** Set up translations */
static void initTranslations(QTranslator &qtTranslatorBase, QTranslator &qtTranslator, QTranslator &translatorBase, QTranslator &translator)
{
    // Remove old translators
    QApplication::removeTranslator(&qtTranslatorBase);
    QApplication::removeTranslator(&qtTranslator);
    QApplication::removeTranslator(&translatorBase);
    QApplication::removeTranslator(&translator);

    // Get desired locale (e.g. "de_DE")
    // 1) System default language
    QString lang_territory = GetLangTerritory();

    // Convert to "de" only by truncating "_DE"
    QString lang = lang_territory;
    lang.truncate(lang_territory.lastIndexOf('_'));

    // Load language files for configured locale:
    // - First load the translator for the base language, without territory
    // - Then load the more specific locale translator

    // Load e.g. qt_de.qm
    if (qtTranslatorBase.load("qt_" + lang, QLibraryInfo::location(QLibraryInfo::TranslationsPath)))
        QApplication::installTranslator(&qtTranslatorBase);

    // Load e.g. qt_de_DE.qm
    if (qtTranslator.load("qt_" + lang_territory, QLibraryInfo::location(QLibraryInfo::TranslationsPath)))
        QApplication::installTranslator(&qtTranslator);

    // Load e.g. bitcoin_de.qm (shortcut "de" needs to be defined in dash.qrc)
    if (translatorBase.load(lang, ":/translations/"))
        QApplication::installTranslator(&translatorBase);

    // Load e.g. bitcoin_de_DE.qm (shortcut "de_DE" needs to be defined in dash.qrc)
    if (translator.load(lang_territory, ":/translations/"))
        QApplication::installTranslator(&translator);
}

/* qDebug() message handler --> debug.log */
void DebugMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString &msg)
{
    Q_UNUSED(context);
    if (type == QtDebugMsg) {
        LogPrint(BCLog::QT, "GUI: %s\n", msg.toStdString());
    } else {
        LogPrintf("GUI: %s\n", msg.toStdString());
    }
}

BitcoinCore::BitcoinCore(interfaces::Node& node) :
    QObject(), m_node(node)
{
}

void BitcoinCore::handleRunawayException(const std::exception_ptr e)
{
    PrintExceptionContinue(e, "Runaway exception");
    Q_EMIT runawayException(QString::fromStdString(m_node.getWarnings()));
}

void BitcoinCore::initialize()
{
    try
    {
        qDebug() << __func__ << ": Running initialization in thread";
        util::ThreadRename("qt-init");
        interfaces::BlockAndHeaderTipInfo tip_info;
        bool rv = m_node.appInitMain(&tip_info);
        Q_EMIT initializeResult(rv, tip_info);
    } catch (...) {
        handleRunawayException(std::current_exception());
    }
}

void BitcoinCore::restart(QStringList args)
{
    static bool executing_restart{false};

    if(!executing_restart) { // Only restart 1x, no matter how often a user clicks on a restart-button
        executing_restart = true;
        try
        {
            qDebug() << __func__ << ": Running Restart in thread";
            m_node.appPrepareShutdown();
            qDebug() << __func__ << ": Shutdown finished";
            Q_EMIT shutdownResult();
            CExplicitNetCleanup::callCleanup();
            QProcess::startDetached(QApplication::applicationFilePath(), args);
            qDebug() << __func__ << ": Restart initiated...";
            QApplication::quit();
        } catch (...) {
            handleRunawayException(std::current_exception());
        }
    }
}

void BitcoinCore::shutdown()
{
    try
    {
        qDebug() << __func__ << ": Running Shutdown in thread";
        m_node.appShutdown();
        qDebug() << __func__ << ": Shutdown finished";
        Q_EMIT shutdownResult();
    } catch (...) {
        handleRunawayException(std::current_exception());
    }
}

static int qt_argc = 1;
static const char* qt_argv = "dash-qt";

BitcoinApplication::BitcoinApplication(interfaces::Node& node):
    QApplication(qt_argc, const_cast<char **>(&qt_argv)),
    coreThread(nullptr),
    m_node(node),
    optionsModel(nullptr),
    clientModel(nullptr),
    window(nullptr),
    pollShutdownTimer(nullptr),
    returnValue(0)
{
    // Qt runs setlocale(LC_ALL, "") on initialization.
    setQuitOnLastWindowClosed(false);
}

BitcoinApplication::~BitcoinApplication()
{
    if(coreThread)
    {
        qDebug() << __func__ << ": Stopping thread";
        coreThread->quit();
        coreThread->wait();
        qDebug() << __func__ << ": Stopped thread";
    }

    delete window;
    window = nullptr;
    // Delete Qt-settings if user clicked on "Reset Options"
    QSettings settings;
    if(optionsModel && optionsModel->resetSettingsOnShutdown){
        settings.clear();
        settings.sync();
    }
    delete optionsModel;
    optionsModel = nullptr;
}

#ifdef ENABLE_WALLET
void BitcoinApplication::createPaymentServer()
{
    paymentServer = new PaymentServer(this);
}
#endif

void BitcoinApplication::createOptionsModel(bool resetSettings)
{
    optionsModel = new OptionsModel(m_node, nullptr, resetSettings);
}

void BitcoinApplication::createWindow(const NetworkStyle *networkStyle)
{
    window = new BitcoinGUI(m_node, networkStyle, nullptr);
    pollShutdownTimer = new QTimer(window);
    connect(pollShutdownTimer, &QTimer::timeout, window, &BitcoinGUI::detectShutdown);
}

void BitcoinApplication::createSplashScreen(const NetworkStyle *networkStyle)
{
    SplashScreen *splash = new SplashScreen(m_node, nullptr, networkStyle);
    // We don't hold a direct pointer to the splash screen after creation, but the splash
    // screen will take care of deleting itself when finish() happens.
    splash->show();
    connect(this, &BitcoinApplication::requestedInitialize, splash, &SplashScreen::handleLoadWallet);
    connect(this, &BitcoinApplication::splashFinished, splash, &SplashScreen::finish);
    connect(this, &BitcoinApplication::requestedShutdown, splash, &QWidget::close);
}

bool BitcoinApplication::baseInitialize()
{
    return m_node.baseInitialize();
}

void BitcoinApplication::startThread()
{
    if(coreThread)
        return;
    coreThread = new QThread(this);
    BitcoinCore *executor = new BitcoinCore(m_node);
    executor->moveToThread(coreThread);

    /*  communication to and from thread */
    connect(executor, &BitcoinCore::initializeResult, this, &BitcoinApplication::initializeResult);
    connect(executor, &BitcoinCore::shutdownResult, this, &BitcoinApplication::shutdownResult);
    connect(executor, &BitcoinCore::runawayException, this, &BitcoinApplication::handleRunawayException);
    connect(this, &BitcoinApplication::requestedInitialize, executor, &BitcoinCore::initialize);
    connect(this, &BitcoinApplication::requestedShutdown, executor, &BitcoinCore::shutdown);
    connect(window, &BitcoinGUI::requestedRestart, executor, &BitcoinCore::restart);
    /*  make sure executor object is deleted in its own thread */
    connect(coreThread, &QThread::finished, executor, &QObject::deleteLater);

    coreThread->start();
}

void BitcoinApplication::parameterSetup()
{
    // Default printtoconsole to false for the GUI. GUI programs should not
    // print to the console unnecessarily.
    gArgs.SoftSetBoolArg("-printtoconsole", false);

    m_node.initLogging();
    m_node.initParameterInteraction();
}

void BitcoinApplication::InitializePruneSetting(bool prune)
{
    // If prune is set, intentionally override existing prune size with
    // the default size since this is called when choosing a new datadir.
    optionsModel->SetPruneTargetGB(prune ? DEFAULT_PRUNE_TARGET_GB : 0, true);
}

void BitcoinApplication::requestInitialize()
{
    qDebug() << __func__ << ": Requesting initialize";
    startThread();
    Q_EMIT requestedInitialize();
}

void BitcoinApplication::requestShutdown()
{
    // Show a simple window indicating shutdown status
    // Do this first as some of the steps may take some time below,
    // for example the RPC console may still be executing a command.
    shutdownWindow.reset(ShutdownWindow::showShutdownWindow(m_node, window));

    qDebug() << __func__ << ": Requesting shutdown";
    startThread();
    window->hide();
    // Must disconnect node signals otherwise current thread can deadlock since
    // no event loop is running.
    window->unsubscribeFromCoreSignals();
    // Request node shutdown, which can interrupt long operations, like
    // rescanning a wallet.
    m_node.startShutdown();
    // Unsetting the client model can cause the current thread to wait for node
    // to complete an operation, like wait for a RPC execution to complete.
    window->setClientModel(nullptr);
    pollShutdownTimer->stop();

    delete clientModel;
    clientModel = nullptr;

    // Request shutdown from core thread
    Q_EMIT requestedShutdown();
}

void BitcoinApplication::initializeResult(bool success, interfaces::BlockAndHeaderTipInfo tip_info)
{
    qDebug() << __func__ << ": Initialization result: " << success;
    // Set exit result.
    returnValue = success ? EXIT_SUCCESS : EXIT_FAILURE;
    if(success)
    {
        // Log this only after AppInitMain finishes, as then logging setup is guaranteed complete
        qInfo() << "Platform customization:" << gArgs.GetArg("-uiplatform", BitcoinGUI::DEFAULT_UIPLATFORM).c_str();
        clientModel = new ClientModel(m_node, optionsModel);
        window->setClientModel(clientModel, &tip_info);
#ifdef ENABLE_WALLET
        if (WalletModel::isWalletEnabled()) {
            m_wallet_controller = new WalletController(*clientModel, this);
            window->setWalletController(m_wallet_controller);
            if (paymentServer) {
                paymentServer->setOptionsModel(optionsModel);
            }
        }
#endif // ENABLE_WALLET

        // If -min option passed, start window minimized (iconified) or minimized to tray
        if (!gArgs.GetBoolArg("-min", false)) {
            window->show();
        } else if (clientModel->getOptionsModel()->getMinimizeToTray() && window->hasTrayIcon()) {
            // do nothing as the window is managed by the tray icon
        } else {
            window->showMinimized();
        }
        Q_EMIT splashFinished();
        Q_EMIT windowShown(window);

        // Let the users setup their preferred appearance if there are no settings for it defined yet.
        GUIUtil::setupAppearance(window, clientModel->getOptionsModel());

#ifdef ENABLE_WALLET
        // Now that initialization/startup is done, process any command-line
        // dash: URIs or payment requests:
        if (paymentServer) {
            connect(paymentServer, &PaymentServer::receivedPaymentRequest, window, &BitcoinGUI::handlePaymentRequest);
            connect(window, &BitcoinGUI::receivedURI, paymentServer, &PaymentServer::handleURIOrFile);
            connect(paymentServer, &PaymentServer::message, [this](const QString& title, const QString& message, unsigned int style) {
                window->message(title, message, style);
            });
            QTimer::singleShot(100, paymentServer, &PaymentServer::uiReady);
        }
#endif
        pollShutdownTimer->start(200);
    } else {
        Q_EMIT splashFinished(); // Make sure splash screen doesn't stick around during shutdown
        quit(); // Exit first main loop invocation
    }
}

void BitcoinApplication::shutdownResult()
{
    quit(); // Exit second main loop invocation after shutdown finished
}

void BitcoinApplication::handleRunawayException(const QString &message)
{
    QMessageBox::critical(nullptr, "Runaway exception", BitcoinGUI::tr("A fatal error occurred. %1 can no longer continue safely and will quit.").arg(PACKAGE_NAME) + QString("\n\n") + message);
    ::exit(EXIT_FAILURE);
}

WId BitcoinApplication::getMainWinId() const
{
    if (!window)
        return 0;

    return window->winId();
}

static void SetupUIArgs(ArgsManager& argsman)
{
    argsman.AddArg("-choosedatadir", strprintf(QObject::tr("Choose data directory on startup (default: %u)").toStdString(), DEFAULT_CHOOSE_DATADIR), ArgsManager::ALLOW_ANY, OptionsCategory::GUI);
    argsman.AddArg("-custom-css-dir", "Set a directory which contains custom css files. Those will be used as stylesheets for the UI.", ArgsManager::ALLOW_ANY, OptionsCategory::GUI);
    argsman.AddArg("-font-family", QObject::tr("Set the font family. Possible values: %1. (default: %2)").arg("SystemDefault, Montserrat").arg(GUIUtil::fontFamilyToString(GUIUtil::getFontFamilyDefault())).toStdString(), ArgsManager::ALLOW_ANY, OptionsCategory::GUI);
    argsman.AddArg("-font-scale", QObject::tr("Set a scale factor which gets applied to the base font size. Possible range %1 (smallest fonts) to %2 (largest fonts). (default: %3)").arg(-100).arg(100).arg(GUIUtil::getFontScaleDefault()).toStdString(), ArgsManager::ALLOW_ANY, OptionsCategory::GUI);
    argsman.AddArg("-font-weight-bold", QObject::tr("Set the font weight for bold texts. Possible range %1 to %2 (default: %3)").arg(0).arg(8).arg(GUIUtil::weightToArg(GUIUtil::getFontWeightBoldDefault())).toStdString(), ArgsManager::ALLOW_ANY, OptionsCategory::GUI);
    argsman.AddArg("-font-weight-normal", QObject::tr("Set the font weight for normal texts. Possible range %1 to %2 (default: %3)").arg(0).arg(8).arg(GUIUtil::weightToArg(GUIUtil::getFontWeightNormalDefault())).toStdString(), ArgsManager::ALLOW_ANY, OptionsCategory::GUI);
    argsman.AddArg("-lang=<lang>", QObject::tr("Set language, for example \"de_DE\" (default: system locale)").toStdString(), ArgsManager::ALLOW_ANY, OptionsCategory::GUI);
    argsman.AddArg("-min", QObject::tr("Start minimized").toStdString(), ArgsManager::ALLOW_ANY, OptionsCategory::GUI);
    argsman.AddArg("-resetguisettings", QObject::tr("Reset all settings changed in the GUI").toStdString(), ArgsManager::ALLOW_ANY, OptionsCategory::GUI);
    argsman.AddArg("-splash", strprintf(QObject::tr("Show splash screen on startup (default: %u)").toStdString(), DEFAULT_SPLASHSCREEN), ArgsManager::ALLOW_ANY, OptionsCategory::GUI);
    argsman.AddArg("-uiplatform", strprintf("Select platform to customize UI for (one of windows, macosx, other; default: %s)", BitcoinGUI::DEFAULT_UIPLATFORM), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::GUI);
    argsman.AddArg("-debug-ui", "Updates the UI's stylesheets in realtime with changes made to the css files in -custom-css-dir and forces some widgets to show up which are usually only visible under certain circumstances. (default: 0)", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::GUI);
    argsman.AddArg("-windowtitle=<name>", _("Sets a window title which is appended to \"Dash Core - \"").translated, ArgsManager::ALLOW_ANY, OptionsCategory::GUI);
}

int GuiMain(int argc, char* argv[])
{
    RegisterPrettyTerminateHander();
    RegisterPrettySignalHandlers();

#ifdef WIN32
    util::WinCmdLineArgs winArgs;
    std::tie(argc, argv) = winArgs.get();
#endif
    SetupEnvironment();
    util::ThreadSetInternalName("main");

    NodeContext node_context;
    std::unique_ptr<interfaces::Node> node = interfaces::MakeNode(&node_context);

    // Subscribe to global signals from core
    std::unique_ptr<interfaces::Handler> handler_message_box = node->handleMessageBox(noui_ThreadSafeMessageBox);
    std::unique_ptr<interfaces::Handler> handler_question = node->handleQuestion(noui_ThreadSafeQuestion);
    std::unique_ptr<interfaces::Handler> handler_init_message = node->handleInitMessage(noui_InitMessage);

    // Do not refer to data directory yet, this can be overridden by Intro::pickDataDirectory

    /// 1. Basic Qt initialization (not dependent on parameters or configuration)
    Q_INIT_RESOURCE(dash);
    Q_INIT_RESOURCE(dash_locale);

    // Generate high-dpi pixmaps
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    BitcoinApplication app(*node);

    // Register meta types used for QMetaObject::invokeMethod and Qt::QueuedConnection
    qRegisterMetaType<bool*>();
#ifdef ENABLE_WALLET
    qRegisterMetaType<WalletModel*>();
#endif
    // Register typedefs (see http://qt-project.org/doc/qt-5/qmetatype.html#qRegisterMetaType)
    // IMPORTANT: if CAmount is no longer a typedef use the normal variant above (see https://doc.qt.io/qt-5/qmetatype.html#qRegisterMetaType-1)
    qRegisterMetaType<CAmount>("CAmount");
    qRegisterMetaType<size_t>("size_t");

    qRegisterMetaType<std::function<void()>>("std::function<void()>");
    qRegisterMetaType<QMessageBox::Icon>("QMessageBox::Icon");
    qRegisterMetaType<interfaces::BlockAndHeaderTipInfo>("interfaces::BlockAndHeaderTipInfo");

    /// 2. Parse command-line options. We do this after qt in order to show an error if there are problems parsing these
    // Command-line options take precedence:
    node->setupServerArgs();
    SetupUIArgs(gArgs);
    std::string error;
    if (!node->parseParameters(argc, argv, error)) {
        node->initError(strprintf(Untranslated("Error parsing command line arguments: %s\n"), error));
        // Create a message box, because the gui has neither been created nor has subscribed to core signals
        QMessageBox::critical(nullptr, PACKAGE_NAME,
            // message can not be translated because translations have not been initialized
            QString::fromStdString("Error parsing command line arguments: %1.").arg(QString::fromStdString(error)));
        return EXIT_FAILURE;
    }

    /// 3. Application identification
    // must be set before OptionsModel is initialized or translations are loaded,
    // as it is used to locate QSettings
    QApplication::setOrganizationName(QAPP_ORG_NAME);
    QApplication::setOrganizationDomain(QAPP_ORG_DOMAIN);
    QApplication::setApplicationName(QAPP_APP_NAME_DEFAULT);

    /// 4. Initialization of translations, so that intro dialog is in user's language
    // Now that QSettings are accessible, initialize translations
    QTranslator qtTranslatorBase, qtTranslator, translatorBase, translator;
    initTranslations(qtTranslatorBase, qtTranslator, translatorBase, translator);

    if (gArgs.IsArgSet("-printcrashinfo")) {
        auto crashInfo = GetCrashInfoStrFromSerializedStr(gArgs.GetArg("-printcrashinfo", ""));
        std::cout << crashInfo << std::endl;
        QMessageBox::critical(0, PACKAGE_NAME, QString::fromStdString(crashInfo));
        return EXIT_SUCCESS;
    }

    // Show help message immediately after parsing command-line options (for "-lang") and setting locale,
    // but before showing splash screen.
    if (HelpRequested(gArgs) || gArgs.IsArgSet("-version")) {
        HelpMessageDialog help(*node, nullptr, gArgs.IsArgSet("-version") ? HelpMessageDialog::about : HelpMessageDialog::cmdline);
        help.showOrPrint();
        return EXIT_SUCCESS;
    }

    // Install global event filter that makes sure that long tooltips can be word-wrapped
    app.installEventFilter(new GUIUtil::ToolTipToRichTextFilter(TOOLTIP_WRAP_THRESHOLD, &app));

    /// 5. Now that settings and translations are available, ask user for data directory
    // User language is set up: pick a data directory
    bool did_show_intro = false;
    bool prune = false; // Intro dialog prune check box
    // Gracefully exit if the user cancels
    if (!Intro::showIfNeeded(*node, did_show_intro, prune)) return EXIT_SUCCESS;

    /// 6. Determine availability of data directory and parse dash.conf
    /// - Do not call GetDataDir(true) before this step finishes
    if (!CheckDataDirOption()) {
        node->initError(strprintf(Untranslated("Specified data directory \"%s\" does not exist.\n"), gArgs.GetArg("-datadir", "")));
        QMessageBox::critical(nullptr, PACKAGE_NAME,
            QObject::tr("Error: Specified data directory \"%1\" does not exist.").arg(QString::fromStdString(gArgs.GetArg("-datadir", ""))));
        return EXIT_FAILURE;
    }
    if (!node->readConfigFiles(error)) {
        node->initError(strprintf(Untranslated("Error reading configuration file: %s\n"), error));
        QMessageBox::critical(nullptr, PACKAGE_NAME,
            QObject::tr("Error: Cannot parse configuration file: %1.").arg(QString::fromStdString(error)));
        return EXIT_FAILURE;
    }

    /// 7. Determine network (and switch to network specific options)
    // - Do not call Params() before this step
    // - Do this after parsing the configuration file, as the network can be switched there
    // - QSettings() will use the new application name after this, resulting in network-specific settings
    // - Needs to be done before createOptionsModel

    // Check for -chain, -testnet or -regtest parameter (Params() calls are only valid after this clause)
    try {
        node->selectParams(gArgs.GetChainName());
    } catch(std::exception &e) {
        node->initError(Untranslated(strprintf("%s\n", e.what())));
        QMessageBox::critical(nullptr, PACKAGE_NAME, QObject::tr("Error: %1").arg(e.what()));
        return EXIT_FAILURE;
    }
#ifdef ENABLE_WALLET
    // Parse URIs on command line -- this can affect Params()
    PaymentServer::ipcParseCommandLine(*node, argc, argv);
#endif
    if (!node->initSettings(error)) {
        node->initError(Untranslated(error));
        QMessageBox::critical(nullptr, PACKAGE_NAME, QObject::tr("Error initializing settings: %1").arg(QString::fromStdString(error)));
        return EXIT_FAILURE;
    }

    QScopedPointer<const NetworkStyle> networkStyle(NetworkStyle::instantiate(Params().NetworkIDString()));
    assert(!networkStyle.isNull());
    // Allow for separate UI settings for testnets
    // QApplication::setApplicationName(networkStyle->getAppName()); // moved to NetworkStyle::NetworkStyle
    // Re-initialize translations after changing application name (language in network-specific settings can be different)
    initTranslations(qtTranslatorBase, qtTranslator, translatorBase, translator);

#ifdef ENABLE_WALLET
    /// 8. URI IPC sending
    // - Do this early as we don't want to bother initializing if we are just calling IPC
    // - Do this *after* setting up the data directory, as the data directory hash is used in the name
    // of the server.
    // - Do this after creating app and setting up translations, so errors are
    // translated properly.
    if (PaymentServer::ipcSendCommandLine())
        exit(EXIT_SUCCESS);

    // Start up the payment server early, too, so impatient users that click on
    // dash: links repeatedly have their payment requests routed to this process:
    if (WalletModel::isWalletEnabled()) {
        app.createPaymentServer();
    }
#endif // ENABLE_WALLET

    /// 9. Main GUI initialization
    // Install global event filter that makes sure that out-of-focus labels do not contain text cursor.
    app.installEventFilter(new GUIUtil::LabelOutOfFocusEventFilter(&app));
#if defined(Q_OS_WIN)
    // Install global event filter for processing Windows session related Windows messages (WM_QUERYENDSESSION and WM_ENDSESSION)
    qApp->installNativeEventFilter(new WinShutdownMonitor());
#endif
    // Install qDebug() message handler to route to debug.log
    qInstallMessageHandler(DebugMessageHandler);
    // Allow parameter interaction before we create the options model
    app.parameterSetup();
    GUIUtil::LogQtInfo();
    // Load custom application fonts and setup font management
    if (!GUIUtil::loadFonts()) {
        QMessageBox::critical(0, PACKAGE_NAME,
                              QObject::tr("Error: Failed to load application fonts."));
        return EXIT_FAILURE;
    }
    // Load GUI settings from QSettings
    app.createOptionsModel(gArgs.GetBoolArg("-resetguisettings", false));
    // Validate/set font family
    if (gArgs.IsArgSet("-font-family")) {
        GUIUtil::FontFamily family;
        QString strFamily = gArgs.GetArg("-font-family", GUIUtil::fontFamilyToString(GUIUtil::getFontFamilyDefault()).toStdString()).c_str();
        try {
            family = GUIUtil::fontFamilyFromString(strFamily);
        } catch (const std::exception& e) {
            QMessageBox::critical(0, PACKAGE_NAME,
                                  QObject::tr("Error: Specified font-family invalid. Valid values: %1.").arg("SystemDefault, Montserrat"));
            return EXIT_FAILURE;
        }
        GUIUtil::setFontFamily(family);
    }
    // Validate/set normal font weight
    if (gArgs.IsArgSet("-font-weight-normal")) {
        QFont::Weight weight;
        if (!GUIUtil::weightFromArg(gArgs.GetArg("-font-weight-normal", GUIUtil::weightToArg(GUIUtil::getFontWeightNormal())), weight)) {
            QMessageBox::critical(0, PACKAGE_NAME,
                                  QObject::tr("Error: Specified font-weight-normal invalid. Valid range %1 to %2.").arg(0).arg(8));
            return EXIT_FAILURE;
        }
        GUIUtil::setFontWeightNormal(weight);
    }
    // Validate/set bold font weight
    if (gArgs.IsArgSet("-font-weight-bold")) {
        QFont::Weight weight;
        if (!GUIUtil::weightFromArg(gArgs.GetArg("-font-weight-bold", GUIUtil::weightToArg(GUIUtil::getFontWeightBold())), weight)) {
            QMessageBox::critical(0, PACKAGE_NAME,
                                  QObject::tr("Error: Specified font-weight-bold invalid. Valid range %1 to %2.").arg(0).arg(8));
            return EXIT_FAILURE;
        }
        GUIUtil::setFontWeightBold(weight);
    }
    // Validate/set font scale
    if (gArgs.IsArgSet("-font-scale")) {
        const int nScaleMin = -100, nScaleMax = 100;
        int nScale = gArgs.GetArg("-font-scale", GUIUtil::getFontScale());
        if (nScale < nScaleMin || nScale > nScaleMax) {
            QMessageBox::critical(0, PACKAGE_NAME,
                                  QObject::tr("Error: Specified font-scale invalid. Valid range %1 to %2.").arg(nScaleMin).arg(nScaleMax));
            return EXIT_FAILURE;
        }
        GUIUtil::setFontScale(nScale);
    }
    // Validate/set custom css directory
    if (gArgs.IsArgSet("-custom-css-dir")) {
        fs::path customDir = fs::path(gArgs.GetArg("-custom-css-dir", ""));
        QString strCustomDir = QString::fromStdString(customDir.string());
        std::vector<QString> vecRequiredFiles = GUIUtil::listStyleSheets();
        QString strFile;

        if (!fs::is_directory(customDir)) {
            QMessageBox::critical(0, PACKAGE_NAME,
                                  QObject::tr("Error: Invalid -custom-css-dir path.") + "\n\n" + strCustomDir);
            return EXIT_FAILURE;
        }

        for (auto itCustomDir = fs::directory_iterator(customDir); itCustomDir != fs::directory_iterator(); ++itCustomDir) {
            if (fs::is_regular_file(*itCustomDir) && itCustomDir->path().extension() == ".css") {
                strFile = QString::fromStdString(itCustomDir->path().filename().string());
                auto itFile = std::find(vecRequiredFiles.begin(), vecRequiredFiles.end(), strFile);
                if (itFile != vecRequiredFiles.end()) {
                    vecRequiredFiles.erase(itFile);
                }
            }
        }

        if (vecRequiredFiles.size()) {
            QString strMissingFiles;
            for (const auto& strMissingFile : vecRequiredFiles) {
                strMissingFiles += strMissingFile + "\n";
            }
            QMessageBox::critical(0, PACKAGE_NAME,
                                  QObject::tr("Error: %1 CSS file(s) missing in -custom-css-dir path.").arg(vecRequiredFiles.size()) + "\n\n" + strMissingFiles);
            return EXIT_FAILURE;
        }

        GUIUtil::setStyleSheetDirectory(strCustomDir);
    }
    // Validate -debug-ui
    if (gArgs.GetBoolArg("-debug-ui", false)) {
        QMessageBox::warning(0, PACKAGE_NAME,
                                "Warning: UI debug mode (-debug-ui) enabled" + QString(gArgs.IsArgSet("-custom-css-dir") ? "." : " without a custom css directory set with -custom-css-dir."));
    }

    if (did_show_intro) {
        // Store intro dialog settings other than datadir (network specific)
        app.InitializePruneSetting(prune);
    }

    if (gArgs.GetBoolArg("-splash", DEFAULT_SPLASHSCREEN) && !gArgs.GetBoolArg("-min", false))
        app.createSplashScreen(networkStyle.data());

    int rv = EXIT_SUCCESS;
    try
    {
        app.createWindow(networkStyle.data());
        // Perform base initialization before spinning up initialization/shutdown thread
        // This is acceptable because this function only contains steps that are quick to execute,
        // so the GUI thread won't be held up.
        if (app.baseInitialize()) {
            app.requestInitialize();
#if defined(Q_OS_WIN)
            WinShutdownMonitor::registerShutdownBlockReason(QObject::tr("%1 didn't yet exit safely...").arg(PACKAGE_NAME), (HWND)app.getMainWinId());
#endif
            app.exec();
            app.requestShutdown();
            app.exec();
            rv = app.getReturnValue();
        } else {
            // A dialog with detailed error will have been shown by InitError()
            rv = EXIT_FAILURE;
        }
    } catch (...) {
        PrintExceptionContinue(std::current_exception(), "Runaway exception");
        app.handleRunawayException(QString::fromStdString(node->getWarnings()));
    }
    return rv;
}
