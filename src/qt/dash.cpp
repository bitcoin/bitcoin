// Copyright (c) 2011-2015 The Bitcoin Core developers
// Copyright (c) 2014-2019 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/dash-config.h>
#endif

#include <qt/bitcoingui.h>

#include <chainparams.h>
#include <qt/clientmodel.h>
#include <fs.h>
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
#include <qt/walletmodel.h>
#endif

#include <init.h>
#include <interfaces/handler.h>
#include <interfaces/node.h>
#include <rpc/server.h>
#include <stacktraces.h>
#include <ui_interface.h>
#include <uint256.h>
#include <util.h>
#include <warnings.h>

#include <walletinitinterface.h>

#include <memory>
#include <stdint.h>

#include <boost/thread.hpp>

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
#endif
#endif

// Declare meta types used for QMetaObject::invokeMethod
Q_DECLARE_METATYPE(bool*)
Q_DECLARE_METATYPE(CAmount)
Q_DECLARE_METATYPE(uint256)

static void InitMessage(const std::string &message)
{
    LogPrintf("init message: %s\n", message);
}

/*
   Translate string to current locale using Qt.
 */
static std::string Translate(const char* psz)
{
    return QCoreApplication::translate("dash-core", psz).toStdString();
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

/** Class encapsulating Dash Core startup and shutdown.
 * Allows running startup and shutdown in a different thread from the UI thread.
 */
class BitcoinCore: public QObject
{
    Q_OBJECT
public:
    explicit BitcoinCore(interfaces::Node& node);

public Q_SLOTS:
    void initialize();
    void shutdown();
    void restart(QStringList args);

Q_SIGNALS:
    void initializeResult(bool success);
    void shutdownResult();
    void runawayException(const QString &message);

private:
    /// Pass fatal exception message to UI thread
    void handleRunawayException(const std::exception_ptr e);

    interfaces::Node& m_node;
};

/** Main Dash application object */
class BitcoinApplication: public QApplication
{
    Q_OBJECT
public:
    explicit BitcoinApplication(interfaces::Node& node, int &argc, char **argv);
    ~BitcoinApplication();

#ifdef ENABLE_WALLET
    /// Create payment server
    void createPaymentServer();
#endif
    /// parameter interaction/setup based on rules
    void parameterSetup();
    /// Create options model
    void createOptionsModel(bool resetSettings);
    /// Create main window
    void createWindow(const NetworkStyle *networkStyle);
    /// Create splash screen
    void createSplashScreen(const NetworkStyle *networkStyle);

    /// Request core initialization
    void requestInitialize();
    /// Request core shutdown
    void requestShutdown();

    /// Get process return value
    int getReturnValue() const { return returnValue; }

    /// Get window identifier of QMainWindow (BitcoinGUI)
    WId getMainWinId() const;

public Q_SLOTS:
    void initializeResult(bool success);
    void shutdownResult();
    /// Handle runaway exceptions. Shows a message box with the problem and quits the program.
    void handleRunawayException(const QString &message);
    void addWallet(WalletModel* walletModel);
    void removeWallet();

Q_SIGNALS:
    void requestedInitialize();
    void requestedRestart(QStringList args);
    void requestedShutdown();
    void stopThread();
    void splashFinished(QWidget *window);

private:
    QThread *coreThread;
    interfaces::Node& m_node;
    OptionsModel *optionsModel;
    ClientModel *clientModel;
    BitcoinGUI *window;
    QTimer *pollShutdownTimer;
#ifdef ENABLE_WALLET
    PaymentServer* paymentServer;
    std::vector<WalletModel*> m_wallet_models;
    std::unique_ptr<interfaces::Handler> m_handler_load_wallet;
#endif
    int returnValue;
    std::unique_ptr<QWidget> shutdownWindow;

    void startThread();
};

#include <qt/dash.moc>

BitcoinCore::BitcoinCore(interfaces::Node& node) :
    QObject(), m_node(node)
{
}

void BitcoinCore::handleRunawayException(const std::exception_ptr e)
{
    PrintExceptionContinue(e, "Runaway exception");
    Q_EMIT runawayException(QString::fromStdString(m_node.getWarnings("gui")));
}

void BitcoinCore::initialize()
{
    try
    {
        qDebug() << __func__ << ": Running initialization in thread";
        bool rv = m_node.appInitMain();
        Q_EMIT initializeResult(rv);
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
            Interrupt();
            StartRestart();
            PrepareShutdown();
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

BitcoinApplication::BitcoinApplication(interfaces::Node& node, int &argc, char **argv):
    QApplication(argc, argv),
    coreThread(0),
    m_node(node),
    optionsModel(0),
    clientModel(0),
    window(0),
    pollShutdownTimer(0),
#ifdef ENABLE_WALLET
    paymentServer(0),
    m_wallet_models(),
#endif
    returnValue(0)
{
    setQuitOnLastWindowClosed(false);
}

BitcoinApplication::~BitcoinApplication()
{
    if(coreThread)
    {
        qDebug() << __func__ << ": Stopping thread";
        Q_EMIT stopThread();
        coreThread->wait();
        qDebug() << __func__ << ": Stopped thread";
    }

    delete window;
    window = 0;
#ifdef ENABLE_WALLET
    delete paymentServer;
    paymentServer = 0;
#endif
    // Delete Qt-settings if user clicked on "Reset Options"
    QSettings settings;
    if(optionsModel && optionsModel->resetSettingsOnShutdown){
        settings.clear();
        settings.sync();
    }
    delete optionsModel;
    optionsModel = 0;
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
    window = new BitcoinGUI(m_node, networkStyle, 0);
    pollShutdownTimer = new QTimer(window);
    connect(pollShutdownTimer, SIGNAL(timeout()), window, SLOT(detectShutdown()));
}

void BitcoinApplication::createSplashScreen(const NetworkStyle *networkStyle)
{
    SplashScreen *splash = new SplashScreen(m_node, 0, networkStyle);
    // We don't hold a direct pointer to the splash screen after creation, but the splash
    // screen will take care of deleting itself when slotFinish happens.
    splash->show();
    connect(this, SIGNAL(splashFinished(QWidget*)), splash, SLOT(slotFinish(QWidget*)));
    connect(this, SIGNAL(requestedShutdown()), splash, SLOT(close()));
}

void BitcoinApplication::startThread()
{
    if(coreThread)
        return;
    coreThread = new QThread(this);
    BitcoinCore *executor = new BitcoinCore(m_node);
    executor->moveToThread(coreThread);

    /*  communication to and from thread */
    connect(executor, SIGNAL(initializeResult(bool)), this, SLOT(initializeResult(bool)));
    connect(executor, SIGNAL(shutdownResult()), this, SLOT(shutdownResult()));
    connect(executor, SIGNAL(runawayException(QString)), this, SLOT(handleRunawayException(QString)));
    connect(this, SIGNAL(requestedInitialize()), executor, SLOT(initialize()));
    connect(this, SIGNAL(requestedShutdown()), executor, SLOT(shutdown()));
    connect(window, SIGNAL(requestedRestart(QStringList)), executor, SLOT(restart(QStringList)));
    /*  make sure executor object is deleted in its own thread */
    connect(this, SIGNAL(stopThread()), executor, SLOT(deleteLater()));
    connect(this, SIGNAL(stopThread()), coreThread, SLOT(quit()));

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
    window->setClientModel(0);
    pollShutdownTimer->stop();

#ifdef ENABLE_WALLET
    window->removeAllWallets();
    for (WalletModel *walletModel : m_wallet_models) {
        delete walletModel;
    }
    m_wallet_models.clear();
#endif
    delete clientModel;
    clientModel = 0;

    m_node.startShutdown();

    // Request shutdown from core thread
    Q_EMIT requestedShutdown();
}

void BitcoinApplication::addWallet(WalletModel* walletModel)
{
#ifdef ENABLE_WALLET
    window->addWallet(walletModel);

    if (m_wallet_models.empty()) {
        window->setCurrentWallet(walletModel->getWalletName());
    }

    connect(walletModel, SIGNAL(coinsSent(WalletModel*, SendCoinsRecipient, QByteArray)),
        paymentServer, SLOT(fetchPaymentACK(WalletModel*, const SendCoinsRecipient&, QByteArray)));
    connect(walletModel, SIGNAL(unload()), this, SLOT(removeWallet()));

    m_wallet_models.push_back(walletModel);
#endif
}

void BitcoinApplication::removeWallet()
{
#ifdef ENABLE_WALLET
    WalletModel* walletModel = static_cast<WalletModel*>(sender());
    m_wallet_models.erase(std::find(m_wallet_models.begin(), m_wallet_models.end(), walletModel));
    window->removeWallet(walletModel);
    walletModel->deleteLater();
#endif
}

void BitcoinApplication::initializeResult(bool success)
{
    qDebug() << __func__ << ": Initialization result: " << success;
    // Set exit result.
    returnValue = success ? EXIT_SUCCESS : EXIT_FAILURE;
    if(success)
    {
        // Log this only after AppInitMain finishes, as then logging setup is guaranteed complete
        qWarning() << "Platform customization:" << gArgs.GetArg("-uiplatform", BitcoinGUI::DEFAULT_UIPLATFORM).c_str();
#ifdef ENABLE_WALLET
        PaymentServer::LoadRootCAs();
        paymentServer->setOptionsModel(optionsModel);
#endif

        clientModel = new ClientModel(m_node, optionsModel);
        window->setClientModel(clientModel);

#ifdef ENABLE_WALLET
        m_handler_load_wallet = m_node.handleLoadWallet([this](std::unique_ptr<interfaces::Wallet> wallet) {
            WalletModel* wallet_model = new WalletModel(std::move(wallet), m_node, optionsModel, nullptr);
            // Fix wallet model thread affinity.
            wallet_model->moveToThread(thread());
            QMetaObject::invokeMethod(this, "addWallet", Qt::QueuedConnection, Q_ARG(WalletModel*, wallet_model));
        });

        for (auto& wallet : m_node.getWallets()) {
            addWallet(new WalletModel(std::move(wallet), m_node, optionsModel));
        }
#endif

        // If -min option passed, start window minimized.
        if(gArgs.GetBoolArg("-min", false))
        {
            window->showMinimized();
        }
        else
        {
            window->show();
        }
        Q_EMIT splashFinished(window);

        // Let the users setup their prefered appearance if there are no settings for it defined yet.
        GUIUtil::setupAppearance(window, clientModel->getOptionsModel());

#ifdef ENABLE_WALLET
        // Now that initialization/startup is done, process any command-line
        // dash: URIs or payment requests:
        connect(paymentServer, SIGNAL(receivedPaymentRequest(SendCoinsRecipient)),
                         window, SLOT(handlePaymentRequest(SendCoinsRecipient)));
        connect(window, SIGNAL(receivedURI(QString)),
                         paymentServer, SLOT(handleURIOrFile(QString)));
        connect(paymentServer, SIGNAL(message(QString,QString,unsigned int)),
                         window, SLOT(message(QString,QString,unsigned int)));
        QTimer::singleShot(100, paymentServer, SLOT(uiReady()));
#endif
        pollShutdownTimer->start(200);
    } else {
        Q_EMIT splashFinished(window); // Make sure splash screen doesn't stick around during shutdown
        quit(); // Exit first main loop invocation
    }
}

void BitcoinApplication::shutdownResult()
{
    quit(); // Exit second main loop invocation after shutdown finished
}

void BitcoinApplication::handleRunawayException(const QString &message)
{
    QMessageBox::critical(0, "Runaway exception", BitcoinGUI::tr("A fatal error occurred. Dash Core can no longer continue safely and will quit.") + QString("\n\n") + message);
    ::exit(EXIT_FAILURE);
}

WId BitcoinApplication::getMainWinId() const
{
    if (!window)
        return 0;

    return window->winId();
}

static void SetupUIArgs()
{
#ifdef ENABLE_WALLET
    gArgs.AddArg("-allowselfsignedrootcertificates", strprintf("Allow self signed root certificates (default: %u)", DEFAULT_SELFSIGNED_ROOTCERTS), true, OptionsCategory::GUI);
#endif
    gArgs.AddArg("-choosedatadir", strprintf(QObject::tr("Choose data directory on startup (default: %u)").toStdString(), DEFAULT_CHOOSE_DATADIR), false, OptionsCategory::GUI);
    gArgs.AddArg("-custom-css-dir", "Set a directory which contains custom css files. Those will be used as stylesheets for the UI.", false, OptionsCategory::GUI);
    gArgs.AddArg("-font-family", QObject::tr("Set the font family. Possible values: %1. (default: %2)").arg("SystemDefault, Montserrat").arg(GUIUtil::fontFamilyToString(GUIUtil::getFontFamilyDefault())).toStdString(), false, OptionsCategory::GUI);
    gArgs.AddArg("-font-scale", QObject::tr("Set a scale factor which gets applied to the base font size. Possible range %1 (smallest fonts) to %2 (largest fonts). (default: %3)").arg(-100).arg(100).arg(GUIUtil::getFontScaleDefault()).toStdString(), false, OptionsCategory::GUI);
    gArgs.AddArg("-font-weight-bold", QObject::tr("Set the font weight for bold texts. Possible range %1 to %2 (default: %3)").arg(0).arg(8).arg(GUIUtil::weightToArg(GUIUtil::getFontWeightBoldDefault())).toStdString(), false, OptionsCategory::GUI);
    gArgs.AddArg("-font-weight-normal", QObject::tr("Set the font weight for normal texts. Possible range %1 to %2 (default: %3)").arg(0).arg(8).arg(GUIUtil::weightToArg(GUIUtil::getFontWeightNormalDefault())).toStdString(), false, OptionsCategory::GUI);
    gArgs.AddArg("-lang=<lang>", QObject::tr("Set language, for example \"de_DE\" (default: system locale)").toStdString(), false, OptionsCategory::GUI);
    gArgs.AddArg("-min", QObject::tr("Start minimized").toStdString(), false, OptionsCategory::GUI);
    gArgs.AddArg("-resetguisettings", QObject::tr("Reset all settings changed in the GUI").toStdString(), false, OptionsCategory::GUI);
    gArgs.AddArg("-rootcertificates=<file>", QObject::tr("Set SSL root certificates for payment request (default: -system-)").toStdString(), false, OptionsCategory::GUI);
    gArgs.AddArg("-splash", strprintf(QObject::tr("Show splash screen on startup (default: %u)").toStdString(), DEFAULT_SPLASHSCREEN), false, OptionsCategory::GUI);
    gArgs.AddArg("-uiplatform", strprintf("Select platform to customize UI for (one of windows, macosx, other; default: %s)", BitcoinGUI::DEFAULT_UIPLATFORM), true, OptionsCategory::GUI);
    gArgs.AddArg("-debug-ui", "Updates the UI's stylesheets in realtime with changes made to the css files in -custom-css-dir and forces some widgets to show up which are usually only visible under certain circumstances. (default: 0)", true, OptionsCategory::GUI);
    gArgs.AddArg("-windowtitle=<name>", _("Sets a window title which is appended to \"Dash Core - \""), false, OptionsCategory::GUI);
}

#ifndef BITCOIN_QT_TEST
int main(int argc, char *argv[])
{
    RegisterPrettyTerminateHander();
    RegisterPrettySignalHandlers();

    SetupEnvironment();

    std::unique_ptr<interfaces::Node> node = interfaces::MakeNode();

    /// 1. Parse command-line options. These take precedence over anything else.
    // Command-line options take precedence:
    node->setupServerArgs();
    SetupUIArgs();
    node->parseParameters(argc, argv);

    // Do not refer to data directory yet, this can be overridden by Intro::pickDataDirectory

    /// 2. Basic Qt initialization (not dependent on parameters or configuration)
    Q_INIT_RESOURCE(dash);
    Q_INIT_RESOURCE(dash_locale);

    // Generate high-dpi pixmaps
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#if QT_VERSION >= 0x050600
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
#ifdef Q_OS_MAC
    QApplication::setAttribute(Qt::AA_DontShowIconsInMenus);
#endif

    BitcoinApplication app(*node, argc, argv);

    // Register meta types used for QMetaObject::invokeMethod
    qRegisterMetaType< bool* >();
    //   Need to pass name here as CAmount is a typedef (see http://qt-project.org/doc/qt-5/qmetatype.html#qRegisterMetaType)
    //   IMPORTANT if it is no longer a typedef use the normal variant above
    qRegisterMetaType< CAmount >("CAmount");
    qRegisterMetaType< std::function<void(void)> >("std::function<void(void)>");
#ifdef ENABLE_WALLET
    qRegisterMetaType<WalletModel*>("WalletModel*");
#endif

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
    translationInterface.Translate.connect(Translate);

    if (gArgs.IsArgSet("-printcrashinfo")) {
        auto crashInfo = GetCrashInfoStrFromSerializedStr(gArgs.GetArg("-printcrashinfo", ""));
        std::cout << crashInfo << std::endl;
        QMessageBox::critical(0, QObject::tr(PACKAGE_NAME), QString::fromStdString(crashInfo));
        return EXIT_SUCCESS;
    }

    // Show help message immediately after parsing command-line options (for "-lang") and setting locale,
    // but before showing splash screen.
    if (gArgs.IsArgSet("-?") || gArgs.IsArgSet("-h") || gArgs.IsArgSet("-help") || gArgs.IsArgSet("-version"))
    {
        HelpMessageDialog help(*node, nullptr, gArgs.IsArgSet("-version") ? HelpMessageDialog::about : HelpMessageDialog::cmdline);
        help.showOrPrint();
        return EXIT_SUCCESS;
    }

    /// 5. Now that settings and translations are available, ask user for data directory
    // User language is set up: pick a data directory
    if (!Intro::pickDataDirectory(*node))
        return EXIT_SUCCESS;

    /// 6. Determine availability of data and blocks directory and parse dash.conf
    /// - Do not call GetDataDir(true) before this step finishes
    if (!fs::is_directory(GetDataDir(false)))
    {
        QMessageBox::critical(0, QObject::tr(PACKAGE_NAME),
                              QObject::tr("Error: Specified data directory \"%1\" does not exist.").arg(QString::fromStdString(gArgs.GetArg("-datadir", ""))));
        return EXIT_FAILURE;
    }
    try {
        node->readConfigFile(gArgs.GetArg("-conf", BITCOIN_CONF_FILENAME));
    } catch (const std::exception& e) {
        QMessageBox::critical(0, QObject::tr(PACKAGE_NAME),
                              QObject::tr("Error: Cannot parse configuration file: %1. Only use key=value syntax.").arg(e.what()));
        return EXIT_FAILURE;
    }

    /// 7. Determine network (and switch to network specific options)
    // - Do not call Params() before this step
    // - Do this after parsing the configuration file, as the network can be switched there
    // - QSettings() will use the new application name after this, resulting in network-specific settings
    // - Needs to be done before createOptionsModel

    // Check for -testnet or -regtest parameter (Params() calls are only valid after this clause)
    try {
        node->selectParams(gArgs.GetChainName());
    } catch(std::exception &e) {
        QMessageBox::critical(0, QObject::tr(PACKAGE_NAME), QObject::tr("Error: %1").arg(e.what()));
        return EXIT_FAILURE;
    }
#ifdef ENABLE_WALLET
    // Parse URIs on command line -- this can affect Params()
    PaymentServer::ipcParseCommandLine(*node, argc, argv);
#endif

    QScopedPointer<const NetworkStyle> networkStyle(NetworkStyle::instantiate(QString::fromStdString(Params().NetworkIDString())));
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
    app.createPaymentServer();
#endif

    /// 9. Main GUI initialization
    // Install global event filter that makes sure that long tooltips can be word-wrapped
    app.installEventFilter(new GUIUtil::ToolTipToRichTextFilter(TOOLTIP_WRAP_THRESHOLD, &app));
#if defined(Q_OS_WIN)
    // Install global event filter for processing Windows session related Windows messages (WM_QUERYENDSESSION and WM_ENDSESSION)
    qApp->installNativeEventFilter(new WinShutdownMonitor());
#endif
    // Install qDebug() message handler to route to debug.log
    qInstallMessageHandler(DebugMessageHandler);
    // Allow parameter interaction before we create the options model
    app.parameterSetup();
    // Load GUI settings from QSettings
    app.createOptionsModel(gArgs.GetBoolArg("-resetguisettings", false));
    // Load custom application fonts and setup font management
    if (!GUIUtil::loadFonts()) {
        QMessageBox::critical(0, QObject::tr(PACKAGE_NAME),
                              QObject::tr("Error: Failed to load application fonts."));
        return EXIT_FAILURE;
    }
    // Validate/set font family
    if (gArgs.IsArgSet("-font-family")) {
        GUIUtil::FontFamily family;
        QString strFamily = gArgs.GetArg("-font-family", GUIUtil::fontFamilyToString(GUIUtil::getFontFamilyDefault()).toStdString()).c_str();
        try {
            family = GUIUtil::fontFamilyFromString(strFamily);
        } catch (const std::exception& e) {
            QMessageBox::critical(0, QObject::tr(PACKAGE_NAME),
                                  QObject::tr("Error: Specified font-family invalid. Valid values: %1.").arg("SystemDefault, Montserrat"));
            return EXIT_FAILURE;
        }
        GUIUtil::setFontFamily(family);
    }
    // Validate/set normal font weight
    if (gArgs.IsArgSet("-font-weight-normal")) {
        QFont::Weight weight;
        if (!GUIUtil::weightFromArg(gArgs.GetArg("-font-weight-normal", GUIUtil::weightToArg(GUIUtil::getFontWeightNormal())), weight)) {
            QMessageBox::critical(0, QObject::tr(PACKAGE_NAME),
                                  QObject::tr("Error: Specified font-weight-normal invalid. Valid range %1 to %2.").arg(0).arg(8));
            return EXIT_FAILURE;
        }
        GUIUtil::setFontWeightNormal(weight);
    }
    // Validate/set bold font weight
    if (gArgs.IsArgSet("-font-weight-bold")) {
        QFont::Weight weight;
        if (!GUIUtil::weightFromArg(gArgs.GetArg("-font-weight-bold", GUIUtil::weightToArg(GUIUtil::getFontWeightBold())), weight)) {
            QMessageBox::critical(0, QObject::tr(PACKAGE_NAME),
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
            QMessageBox::critical(0, QObject::tr(PACKAGE_NAME),
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
            QMessageBox::critical(0, QObject::tr(PACKAGE_NAME),
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
            QMessageBox::critical(0, QObject::tr(PACKAGE_NAME),
                                  QObject::tr("Error: %1 CSS file(s) missing in -custom-css-dir path.").arg(vecRequiredFiles.size()) + "\n\n" + strMissingFiles);
            return EXIT_FAILURE;
        }

        GUIUtil::setStyleSheetDirectory(strCustomDir);
    }
    // Validate -debug-ui
    if (gArgs.GetBoolArg("-debug-ui", false)) {
        QMessageBox::warning(0, QObject::tr(PACKAGE_NAME),
                                "Warning: UI debug mode (-debug-ui) enabled" + QString(gArgs.IsArgSet("-custom-css-dir") ? "." : " without a custom css directory set with -custom-css-dir."));
    }

    // Subscribe to global signals from core
    std::unique_ptr<interfaces::Handler> handler = node->handleInitMessage(InitMessage);

    if (gArgs.GetBoolArg("-splash", DEFAULT_SPLASHSCREEN) && !gArgs.GetBoolArg("-min", false))
        app.createSplashScreen(networkStyle.data());

    int rv = EXIT_SUCCESS;
    try
    {
        app.createWindow(networkStyle.data());
        // Perform base initialization before spinning up initialization/shutdown thread
        // This is acceptable because this function only contains steps that are quick to execute,
        // so the GUI thread won't be held up.
        if (node->baseInitialize()) {
            app.requestInitialize();
#if defined(Q_OS_WIN)
            WinShutdownMonitor::registerShutdownBlockReason(QObject::tr("%1 didn't yet exit safely...").arg(QObject::tr(PACKAGE_NAME)), (HWND)app.getMainWinId());
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
        app.handleRunawayException(QString::fromStdString(node->getWarnings("gui")));
    }
    return rv;
}
#endif // BITCOIN_QT_TEST
