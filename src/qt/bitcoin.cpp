// Copyright (c) 2011-2021 The Bitcoin Core developers
// Copyright (c) 2014-2024 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <qt/bitcoin.h>

#include <chainparams.h>
#include <fs.h>
#include <init.h>
#include <interfaces/handler.h>
#include <interfaces/init.h>
#include <interfaces/node.h>
#include <net.h>
#include <node/context.h>
#include <node/interface_ui.h>
#include <noui.h>
#include <qt/bitcoingui.h>
#include <qt/clientmodel.h>
#include <qt/guiconstants.h>
#include <qt/guiutil.h>
#include <qt/initexecutor.h>
#include <qt/intro.h>
#include <qt/networkstyle.h>
#include <qt/optionsmodel.h>
#include <qt/splashscreen.h>
#include <qt/utilitydialog.h>
#include <qt/winshutdownmonitor.h>
#include <stacktraces.h>
#include <uint256.h>
#include <util/string.h>
#include <util/system.h>
#include <util/threadnames.h>
#include <util/translation.h>
#include <validation.h>

#ifdef ENABLE_WALLET
#include <qt/paymentserver.h>
#include <qt/walletcontroller.h>
#include <qt/walletmodel.h>
#endif // ENABLE_WALLET

#include <boost/signals2/connection.hpp>
#include <chrono>
#include <memory>

#include <QApplication>
#include <QDebug>
#include <QLatin1String>
#include <QLibraryInfo>
#include <QLocale>
#include <QMessageBox>
#include <QSettings>
#include <QThread>
#include <QTimer>
#include <QTranslator>
#include <QWindow>

#if defined(QT_STATIC)
#include <QtPlugin>
#if defined(QT_QPA_PLATFORM_XCB)
Q_IMPORT_PLUGIN(QXcbIntegrationPlugin);
#elif defined(QT_QPA_PLATFORM_WINDOWS)
Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin);
Q_IMPORT_PLUGIN(QWindowsVistaStylePlugin);
#elif defined(QT_QPA_PLATFORM_COCOA)
Q_IMPORT_PLUGIN(QCocoaIntegrationPlugin);
Q_IMPORT_PLUGIN(QMacStylePlugin);
#elif defined(QT_QPA_PLATFORM_ANDROID)
Q_IMPORT_PLUGIN(QAndroidPlatformIntegrationPlugin)
#endif
#endif

// Declare meta types used for QMetaObject::invokeMethod
Q_DECLARE_METATYPE(bool*)
Q_DECLARE_METATYPE(CAmount)
Q_DECLARE_METATYPE(SynchronizationState)
Q_DECLARE_METATYPE(uint256)

static void RegisterMetaTypes()
{
    // Register meta types used for QMetaObject::invokeMethod and Qt::QueuedConnection
    qRegisterMetaType<bool*>();
    qRegisterMetaType<SynchronizationState>();
  #ifdef ENABLE_WALLET
    qRegisterMetaType<WalletModel*>();
  #endif
    // Register typedefs (see https://doc.qt.io/qt-5/qmetatype.html#qRegisterMetaType)
    // IMPORTANT: if CAmount is no longer a typedef use the normal variant above (see https://doc.qt.io/qt-5/qmetatype.html#qRegisterMetaType-1)
    qRegisterMetaType<CAmount>("CAmount");
    qRegisterMetaType<size_t>("size_t");

    qRegisterMetaType<std::function<void()>>("std::function<void()>");
    qRegisterMetaType<QMessageBox::Icon>("QMessageBox::Icon");
    qRegisterMetaType<interfaces::BlockAndHeaderTipInfo>("interfaces::BlockAndHeaderTipInfo");
}

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

static bool InitSettings()
{
    if (!gArgs.GetSettingsPath()) {
        return true; // Do nothing if settings file disabled.
    }

    std::vector<std::string> errors;
    if (!gArgs.ReadSettingsFile(&errors)) {
        std::string error = QT_TRANSLATE_NOOP("dash-core", "Settings file could not be read");
        std::string error_translated = QCoreApplication::translate("dash-core", error.c_str()).toStdString();
        InitError(Untranslated(strprintf("%s:\n%s\n", error, MakeUnorderedList(errors))));

        QMessageBox messagebox(QMessageBox::Critical, PACKAGE_NAME, QString::fromStdString(strprintf("%s.", error_translated)), QMessageBox::Reset | QMessageBox::Abort);
        /*: Explanatory text shown on startup when the settings file cannot be read.
            Prompts user to make a choice between resetting or aborting. */
        messagebox.setInformativeText(QObject::tr("Do you want to reset settings to default values, or to abort without making changes?"));
        messagebox.setDetailedText(QString::fromStdString(MakeUnorderedList(errors)));
        messagebox.setTextFormat(Qt::PlainText);
        messagebox.setDefaultButton(QMessageBox::Reset);
        switch (messagebox.exec()) {
        case QMessageBox::Reset:
            break;
        case QMessageBox::Abort:
            return false;
        default:
            assert(false);
        }
    }

    errors.clear();
    if (!gArgs.WriteSettingsFile(&errors)) {
        std::string error = QT_TRANSLATE_NOOP("dash-core", "Settings file could not be written");
        std::string error_translated = QCoreApplication::translate("dash-core", error.c_str()).toStdString();
        InitError(Untranslated(strprintf("%s:\n%s\n", error, MakeUnorderedList(errors))));

        QMessageBox messagebox(QMessageBox::Critical, PACKAGE_NAME, QString::fromStdString(strprintf("%s.", error_translated)), QMessageBox::Ok);
        /*: Explanatory text shown on startup when the settings file could not be written.
            Prompts user to check that we have the ability to write to the file.
            Explains that the user has the option of running without a settings file.*/
        messagebox.setInformativeText(QObject::tr("A fatal error occurred. Check that settings file is writable, or try running with -nosettings."));
        messagebox.setDetailedText(QString::fromStdString(MakeUnorderedList(errors)));
        messagebox.setTextFormat(Qt::PlainText);
        messagebox.setDefaultButton(QMessageBox::Ok);
        messagebox.exec();
        return false;
    }
    return true;
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

static int qt_argc = 1;
static const char* qt_argv = "dash-qt";

BitcoinApplication::BitcoinApplication():
    QApplication(qt_argc, const_cast<char **>(&qt_argv)),
    optionsModel(nullptr),
    clientModel(nullptr),
    window(nullptr),
    pollShutdownTimer(nullptr),
    returnValue(0)
{
    RegisterMetaTypes();
    // Qt runs setlocale(LC_ALL, "") on initialization.
    setQuitOnLastWindowClosed(false);
}

BitcoinApplication::~BitcoinApplication()
{
    m_executor.reset();

    delete window;
    window = nullptr;
    // Delete Qt-settings if user clicked on "Reset Options"
    QSettings settings;
    if(optionsModel && optionsModel->resetSettingsOnShutdown){
        settings.clear();
        settings.sync();
    }
}

#ifdef ENABLE_WALLET
void BitcoinApplication::createPaymentServer()
{
    paymentServer = new PaymentServer(this);
}
#endif

void BitcoinApplication::createOptionsModel(bool resetSettings)
{
    optionsModel = new OptionsModel(this, resetSettings);
}

void BitcoinApplication::createWindow(const NetworkStyle *networkStyle)
{
    window = new BitcoinGUI(node(), networkStyle, nullptr);
    connect(window, &BitcoinGUI::quitRequested, this, &BitcoinApplication::requestShutdown);

    pollShutdownTimer = new QTimer(window);
    connect(pollShutdownTimer, &QTimer::timeout, [this]{
        if (!QApplication::activeModalWidget()) {
            window->detectShutdown();
        }
    });
}

void BitcoinApplication::createSplashScreen(const NetworkStyle *networkStyle)
{
    m_splash = new SplashScreen(networkStyle);
    m_splash->show();
}

void BitcoinApplication::createNode(interfaces::Init& init)
{
    assert(!m_node);
    m_node = init.makeNode();
    if (optionsModel) optionsModel->setNode(*m_node);
    if (m_splash) m_splash->setNode(*m_node);
}

bool BitcoinApplication::baseInitialize()
{
    return node().baseInitialize();
}

void BitcoinApplication::startThread()
{
    assert(!m_executor);
    m_executor.emplace(node());

    /*  communication to and from thread */
    connect(&m_executor.value(), &InitExecutor::initializeResult, this, &BitcoinApplication::initializeResult);
    connect(&m_executor.value(), &InitExecutor::shutdownResult, this, &QCoreApplication::quit);
    connect(&m_executor.value(), &InitExecutor::runawayException, this, &BitcoinApplication::handleRunawayException);
    connect(this, &BitcoinApplication::requestedInitialize, &m_executor.value(), &InitExecutor::initialize);
    connect(this, &BitcoinApplication::requestedShutdown, &m_executor.value(), &InitExecutor::shutdown);
    connect(window, &BitcoinGUI::requestedRestart, &m_executor.value(), &InitExecutor::restart);
}

void BitcoinApplication::parameterSetup()
{
    // Default printtoconsole to false for the GUI. GUI programs should not
    // print to the console unnecessarily.
    gArgs.SoftSetBoolArg("-printtoconsole", false);

    InitLogging(gArgs);
    InitParameterInteraction(gArgs);
}

void BitcoinApplication::InitPruneSetting(int64_t prune_MiB)
{
    optionsModel->SetPruneTargetGB(PruneMiBtoGB(prune_MiB), true);
}

void BitcoinApplication::requestInitialize()
{
    qDebug() << __func__ << ": Requesting initialize";
    startThread();
    Q_EMIT requestedInitialize();
}

void BitcoinApplication::requestShutdown()
{
    for (const auto w : QGuiApplication::topLevelWindows()) {
        w->hide();
    }

    delete m_splash;
    m_splash = nullptr;

    // Show a simple window indicating shutdown status
    // Do this first as some of the steps may take some time below,
    // for example the RPC console may still be executing a command.
    shutdownWindow.reset(ShutdownWindow::showShutdownWindow(window));

    qDebug() << __func__ << ": Requesting shutdown";

    // Must disconnect node signals otherwise current thread can deadlock since
    // no event loop is running.
    window->unsubscribeFromCoreSignals();
    // Request node shutdown, which can interrupt long operations, like
    // rescanning a wallet.
    node().startShutdown();
    // Unsetting the client model can cause the current thread to wait for node
    // to complete an operation, like wait for a RPC execution to complete.
    window->setClientModel(nullptr);
    pollShutdownTimer->stop();

#ifdef ENABLE_WALLET
    // Delete wallet controller here manually, instead of relying on Qt object
    // tracking (https://doc.qt.io/qt-5/objecttrees.html). This makes sure
    // walletmodel m_handle_* notification handlers are deleted before wallets
    // are unloaded, which can simplify wallet implementations. It also avoids
    // these notifications having to be handled while GUI objects are being
    // destroyed, making GUI code less fragile as well.
    delete m_wallet_controller;
    m_wallet_controller = nullptr;
#endif // ENABLE_WALLET

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
    if(success) {
        delete m_splash;
        m_splash = nullptr;

        // Log this only after AppInitMain finishes, as then logging setup is guaranteed complete
        qInfo() << "Platform customization:" << gArgs.GetArg("-uiplatform", BitcoinGUI::DEFAULT_UIPLATFORM).c_str();
        clientModel = new ClientModel(node(), optionsModel);
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
            QTimer::singleShot(100ms, paymentServer, &PaymentServer::uiReady);
        }
#endif
        pollShutdownTimer->start(SHUTDOWN_POLLING_DELAY);
    } else {
        requestShutdown();
    }
}

void BitcoinApplication::handleRunawayException(const QString &message)
{
    QMessageBox::critical(
        nullptr, tr("Runaway exception"),
        tr("A fatal error occurred. %1 can no longer continue safely and will quit.").arg(PACKAGE_NAME) +
        QLatin1String("<br><br>") + GUIUtil::MakeHtmlLink(message, PACKAGE_BUGREPORT));
    ::exit(EXIT_FAILURE);
}

void BitcoinApplication::handleNonFatalException(const QString& message)
{
    assert(QThread::currentThread() == thread());
    QMessageBox::warning(
        nullptr, tr("Internal error"),
        tr("An internal error occurred. %1 will attempt to continue safely. This is "
           "an unexpected bug which can be reported as described below.").arg(PACKAGE_NAME) +
        QLatin1String("<br><br>") + GUIUtil::MakeHtmlLink(message, PACKAGE_BUGREPORT));
}

WId BitcoinApplication::getMainWinId() const
{
    if (!window)
        return 0;

    return window->winId();
}

bool BitcoinApplication::event(QEvent* e)
{
    if (e->type() == QEvent::Quit) {
        requestShutdown();
        return true;
    }

    return QApplication::event(e);
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

    NodeContext node_context;
    int unused_exit_status;
    std::unique_ptr<interfaces::Init> init = interfaces::MakeNodeInit(node_context, argc, argv, unused_exit_status);

    SetupEnvironment();
    util::ThreadSetInternalName("main");

    // Subscribe to global signals from core
    boost::signals2::scoped_connection handler_message_box = ::uiInterface.ThreadSafeMessageBox_connect(noui_ThreadSafeMessageBox);
    boost::signals2::scoped_connection handler_question = ::uiInterface.ThreadSafeQuestion_connect(noui_ThreadSafeQuestion);
    boost::signals2::scoped_connection handler_init_message = ::uiInterface.InitMessage_connect(noui_InitMessage);

    // Do not refer to data directory yet, this can be overridden by Intro::pickDataDirectory

    /// 1. Basic Qt initialization (not dependent on parameters or configuration)
    Q_INIT_RESOURCE(dash);
    Q_INIT_RESOURCE(dash_locale);

    // Generate high-dpi pixmaps
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    BitcoinApplication app;
    GUIUtil::LoadFont(QStringLiteral(":/fonts/monospace"));

    /// 2. Parse command-line options. We do this after qt in order to show an error if there are problems parsing these
    // Command-line options take precedence:
    SetupServerArgs(gArgs);
    SetupUIArgs(gArgs);
    std::string error;
    if (!gArgs.ParseParameters(argc, argv, error)) {
        InitError(strprintf(Untranslated("Error parsing command line arguments: %s\n"), error));
        // Create a message box, because the gui has neither been created nor has subscribed to core signals
        QMessageBox::critical(nullptr, PACKAGE_NAME,
            // message cannot be translated because translations have not been initialized
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
        QMessageBox::critical(nullptr, PACKAGE_NAME, QString::fromStdString(crashInfo));
        return EXIT_SUCCESS;
    }

    // Show help message immediately after parsing command-line options (for "-lang") and setting locale,
    // but before showing splash screen.
    if (HelpRequested(gArgs) || gArgs.IsArgSet("-version")) {
        HelpMessageDialog help(nullptr, gArgs.IsArgSet("-version") ? HelpMessageDialog::about : HelpMessageDialog::cmdline);
        help.showOrPrint();
        return EXIT_SUCCESS;
    }

    // Install global event filter that makes sure that long tooltips can be word-wrapped
    app.installEventFilter(new GUIUtil::ToolTipToRichTextFilter(TOOLTIP_WRAP_THRESHOLD, &app));

    /// 5. Now that settings and translations are available, ask user for data directory
    // User language is set up: pick a data directory
    bool did_show_intro = false;
    int64_t prune_MiB = 0;  // Intro dialog prune configuration
    // Gracefully exit if the user cancels
    if (!Intro::showIfNeeded(did_show_intro, prune_MiB)) return EXIT_SUCCESS;

    /// 6. Determine availability of data directory and parse dash.conf
    /// - Do not call gArgs.GetDataDirNet() before this step finishes
    if (!CheckDataDirOption()) {
        InitError(strprintf(Untranslated("Specified data directory \"%s\" does not exist.\n"), gArgs.GetArg("-datadir", "")));
        QMessageBox::critical(nullptr, PACKAGE_NAME,
            QObject::tr("Error: Specified data directory \"%1\" does not exist.").arg(QString::fromStdString(gArgs.GetArg("-datadir", ""))));
        return EXIT_FAILURE;
    }
    if (!gArgs.ReadConfigFiles(error, true)) {
        InitError(strprintf(Untranslated("Error reading configuration file: %s\n"), error));
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
        SelectParams(gArgs.GetChainName());
    } catch(std::exception &e) {
        InitError(Untranslated(strprintf("%s\n", e.what())));
        QMessageBox::critical(nullptr, PACKAGE_NAME, QObject::tr("Error: %1").arg(e.what()));
        return EXIT_FAILURE;
    }
#ifdef ENABLE_WALLET
    // Parse URIs on command line
    PaymentServer::ipcParseCommandLine(argc, argv);
#endif

    if (!InitSettings()) {
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
        QMessageBox::critical(nullptr, PACKAGE_NAME,
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
            QMessageBox::critical(nullptr, PACKAGE_NAME,
                                  QObject::tr("Error: Specified font-family invalid. Valid values: %1.").arg("SystemDefault, Montserrat"));
            return EXIT_FAILURE;
        }
        GUIUtil::setFontFamily(family);
    }
    // Validate/set normal font weight
    if (gArgs.IsArgSet("-font-weight-normal")) {
        QFont::Weight weight;
        if (!GUIUtil::weightFromArg(gArgs.GetIntArg("-font-weight-normal", GUIUtil::weightToArg(GUIUtil::getFontWeightNormal())), weight)) {
            QMessageBox::critical(nullptr, PACKAGE_NAME,
                                  QObject::tr("Error: Specified font-weight-normal invalid. Valid range %1 to %2.").arg(0).arg(8));
            return EXIT_FAILURE;
        }
        GUIUtil::setFontWeightNormal(weight);
    }
    // Validate/set bold font weight
    if (gArgs.IsArgSet("-font-weight-bold")) {
        QFont::Weight weight;
        if (!GUIUtil::weightFromArg(gArgs.GetIntArg("-font-weight-bold", GUIUtil::weightToArg(GUIUtil::getFontWeightBold())), weight)) {
            QMessageBox::critical(nullptr, PACKAGE_NAME,
                                  QObject::tr("Error: Specified font-weight-bold invalid. Valid range %1 to %2.").arg(0).arg(8));
            return EXIT_FAILURE;
        }
        GUIUtil::setFontWeightBold(weight);
    }
    // Validate/set font scale
    if (gArgs.IsArgSet("-font-scale")) {
        const int nScaleMin = -100, nScaleMax = 100;
        int nScale = gArgs.GetIntArg("-font-scale", GUIUtil::getFontScale());
        if (nScale < nScaleMin || nScale > nScaleMax) {
            QMessageBox::critical(nullptr, PACKAGE_NAME,
                                  QObject::tr("Error: Specified font-scale invalid. Valid range %1 to %2.").arg(nScaleMin).arg(nScaleMax));
            return EXIT_FAILURE;
        }
        GUIUtil::setFontScale(nScale);
    }
    // Validate/set custom css directory
    if (gArgs.IsArgSet("-custom-css-dir")) {
        fs::path customDir = gArgs.GetPathArg("-custom-css-dir");
        QString strCustomDir = GUIUtil::PathToQString(customDir);
        std::vector<QString> vecRequiredFiles = GUIUtil::listStyleSheets();
        QString strFile;

        if (!fs::is_directory(customDir)) {
            QMessageBox::critical(nullptr, PACKAGE_NAME,
                                  QObject::tr("Error: Invalid -custom-css-dir path.") + "\n\n" + strCustomDir);
            return EXIT_FAILURE;
        }

        for (auto itCustomDir = fs::directory_iterator(customDir); itCustomDir != fs::directory_iterator(); ++itCustomDir) {
            if (fs::is_regular_file(*itCustomDir) && itCustomDir->path().extension() == ".css") {
                strFile = GUIUtil::PathToQString(itCustomDir->path().filename());
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
            QMessageBox::critical(nullptr, PACKAGE_NAME,
                                  QObject::tr("Error: %1 CSS file(s) missing in -custom-css-dir path.").arg(vecRequiredFiles.size()) + "\n\n" + strMissingFiles);
            return EXIT_FAILURE;
        }

        GUIUtil::setStyleSheetDirectory(strCustomDir);
    }
    // Validate -debug-ui
    if (gArgs.GetBoolArg("-debug-ui", false)) {
        QMessageBox::warning(nullptr, PACKAGE_NAME,
                                "Warning: UI debug mode (-debug-ui) enabled" + QString(gArgs.IsArgSet("-custom-css-dir") ? "." : " without a custom css directory set with -custom-css-dir."));
    }

    if (did_show_intro) {
        // Store intro dialog settings other than datadir (network specific)
        app.InitPruneSetting(prune_MiB);
    }

    if (gArgs.GetBoolArg("-splash", DEFAULT_SPLASHSCREEN) && !gArgs.GetBoolArg("-min", false))
        app.createSplashScreen(networkStyle.data());

    app.createNode(*init);

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
            WinShutdownMonitor::registerShutdownBlockReason(QObject::tr("%1 didn't yet exit safely…").arg(PACKAGE_NAME), (HWND)app.getMainWinId());
#endif
            app.exec();
            rv = app.getReturnValue();
        } else {
            // A dialog with detailed error will have been shown by InitError()
            rv = EXIT_FAILURE;
        }
    } catch (...) {
        PrintExceptionContinue(std::current_exception(), "Runaway exception");
        app.handleRunawayException(QString::fromStdString(app.node().getWarnings().translated));
    }
    return rv;
}
