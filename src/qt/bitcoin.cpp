// Copyright (c) 2011-2015 The Bitcoin Core developers
// Copyright (c) 2015-2017 The Bitcoin Unlimited developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include "config/bitcoin-config.h"
#endif

#include "bitcoingui.h"

#include "chainparams.h"
#include "clientmodel.h"
#include "fs.h"
#include "guiconstants.h"
#include "guiutil.h"
#include "intro.h"
#include "networkstyle.h"
#include "optionsmodel.h"
#include "platformstyle.h"
#include "splashscreen.h"
#include "unlimitedmodel.h" // BU
#include "utilitydialog.h"
#include "winshutdownmonitor.h"

#ifdef ENABLE_WALLET
#include "paymentserver.h"
#include "walletmodel.h"
#endif

#include "init.h"
#include "rpc/server.h"
#include "scheduler.h"
#include "ui_interface.h"
#include "util.h"

#ifdef ENABLE_WALLET
#include "wallet/wallet.h"
#endif

#include <stdint.h>

#include <boost/thread.hpp>

#include <QApplication>
#include <QDebug>
#include <QLibraryInfo>
#include <QLocale>
#include <QMessageBox>
#include <QSettings>
#include <QSslConfiguration>
#include <QThread>
#include <QTimer>
#include <QTranslator>

#if defined(QT_STATICPLUGIN)
#include <QtPlugin>
#if QT_VERSION < 0x050000
Q_IMPORT_PLUGIN(qcncodecs)
Q_IMPORT_PLUGIN(qjpcodecs)
Q_IMPORT_PLUGIN(qtwcodecs)
Q_IMPORT_PLUGIN(qkrcodecs)
Q_IMPORT_PLUGIN(qtaccessiblewidgets)
#else
#if QT_VERSION < 0x050400
Q_IMPORT_PLUGIN(AccessibleFactory)
#endif
#if defined(QT_QPA_PLATFORM_XCB)
Q_IMPORT_PLUGIN(QXcbIntegrationPlugin);
#elif defined(QT_QPA_PLATFORM_WINDOWS)
Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin);
#elif defined(QT_QPA_PLATFORM_COCOA)
Q_IMPORT_PLUGIN(QCocoaIntegrationPlugin);
#endif
#endif
#endif

#if QT_VERSION < 0x050000
#include <QTextCodec>
#endif

// Declare meta types used for QMetaObject::invokeMethod
Q_DECLARE_METATYPE(bool *)
Q_DECLARE_METATYPE(CAmount)

static void InitMessage(const std::string &message) { LogPrintf("init message: %s\n", message); }
/*
   Translate string to current locale using Qt.
 */
static std::string Translate(const char *psz)
{
    return QCoreApplication::translate("bitcoin-unlimited", psz).toStdString();
}

static QString GetLangTerritory()
{
    QSettings settings;
    // Get desired locale (e.g. "de_DE")
    // 1) System default language
    QString lang_territory = QLocale::system().name();
    // 2) Language from QSettings
    QString lang_territory_qsettings = settings.value("language", "").toString();
    if (!lang_territory_qsettings.isEmpty())
        lang_territory = lang_territory_qsettings;
    // 3) -lang command line argument
    lang_territory = QString::fromStdString(GetArg("-lang", lang_territory.toStdString()));
    return lang_territory;
}

/** Set up translations */
static void initTranslations(QTranslator &qtTranslatorBase,
    QTranslator &qtTranslator,
    QTranslator &translatorBase,
    QTranslator &translator)
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

    // Load e.g. bitcoin_de.qm (shortcut "de" needs to be defined in bitcoin.qrc)
    if (translatorBase.load(lang, ":/translations/"))
        QApplication::installTranslator(&translatorBase);

    // Load e.g. bitcoin_de_DE.qm (shortcut "de_DE" needs to be defined in bitcoin.qrc)
    if (translator.load(lang_territory, ":/translations/"))
        QApplication::installTranslator(&translator);
}

/* qDebug() message handler --> debug.log */
#if QT_VERSION < 0x050000
void DebugMessageHandler(QtMsgType type, const char *msg)
{
    const char *category = (type == QtDebugMsg) ? "qt" : NULL;
    LogPrint(category, "GUI: %s\n", msg);
}
#else
void DebugMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(context);
    const char *category = (type == QtDebugMsg) ? "qt" : NULL;
    LogPrint(category, "GUI: %s\n", msg.toStdString());
}
#endif

/** Class encapsulating Bitcoin startup and shutdown.
 * Allows running startup and shutdown in a different thread from the UI thread.
 */
class BitcoinCore : public QObject
{
    Q_OBJECT
public:
    explicit BitcoinCore();

public Q_SLOTS:
    void initialize();
    void shutdown();

Q_SIGNALS:
    void initializeResult(int retval);
    void shutdownResult(int retval);
    void runawayException(const QString &message);

private:
    boost::thread_group threadGroup;
    CScheduler scheduler;

    /// Pass fatal exception message to UI thread
    void handleRunawayException(const std::exception *e);
};

/** Main Bitcoin application object */
class BitcoinApplication : public QApplication
{
    Q_OBJECT
public:
    explicit BitcoinApplication(int &argc, char **argv);
    ~BitcoinApplication();

#ifdef ENABLE_WALLET
    /// Create payment server
    void createPaymentServer();
#endif
    /// parameter interaction/setup based on rules
    void parameterSetup();
    /// Create platform style
    void createPlatformStyle();
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
    int getReturnValue() { return returnValue; }
    /// Get window identifier of QMainWindow (BitcoinGUI)
    WId getMainWinId() const;

public Q_SLOTS:
    void initializeResult(int retval);
    void shutdownResult(int retval);
    /// Handle runaway exceptions. Shows a message box with the problem and quits the program.
    void handleRunawayException(const QString &message);

Q_SIGNALS:
    void requestedInitialize();
    void requestedShutdown();
    void stopThread();
    void splashFinished(QWidget *window);

private:
    QThread *coreThread;
    OptionsModel *optionsModel;
    UnlimitedModel *unlimitedModel;
    ClientModel *clientModel;
    BitcoinGUI *window;
    QTimer *pollShutdownTimer;
#ifdef ENABLE_WALLET
    PaymentServer *paymentServer;
    WalletModel *walletModel;
#endif
    int returnValue;
    const PlatformStyle *platformStyle;

    void startThread();
};

#include "bitcoin.moc"

BitcoinCore::BitcoinCore() : QObject() {}
void BitcoinCore::handleRunawayException(const std::exception *e)
{
    PrintExceptionContinue(e, "Runaway exception");
    Q_EMIT runawayException(QString::fromStdString(strMiscWarning));
}

void BitcoinCore::initialize()
{
    try
    {
        qDebug() << __func__ << ": Running AppInit2 in thread";
        int rv = AppInit2(threadGroup, scheduler);
        Q_EMIT initializeResult(rv);
    }
    catch (const std::exception &e)
    {
        handleRunawayException(&e);
    }
    catch (...)
    {
        handleRunawayException(NULL);
    }
}

void BitcoinCore::shutdown()
{
    try
    {
        qDebug() << __func__ << ": Running Shutdown in thread";
        StartShutdown();
        Interrupt(threadGroup);
        threadGroup.join_all();
        Shutdown();
        qDebug() << __func__ << ": Shutdown finished";
        Q_EMIT shutdownResult(1);
    }
    catch (const std::exception &e)
    {
        handleRunawayException(&e);
    }
    catch (...)
    {
        handleRunawayException(NULL);
    }
}

BitcoinApplication::BitcoinApplication(int &argc, char **argv)
    : QApplication(argc, argv), coreThread(0), optionsModel(0), unlimitedModel(0), clientModel(0), window(0),
      pollShutdownTimer(0),
#ifdef ENABLE_WALLET
      paymentServer(0), walletModel(0),
#endif
      returnValue(0), platformStyle(0)
{
    setQuitOnLastWindowClosed(false);
}

BitcoinApplication::~BitcoinApplication()
{
    if (coreThread)
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
    delete optionsModel;
    optionsModel = 0;
    delete platformStyle;
    platformStyle = 0;
    delete unlimitedModel;
    unlimitedModel = 0;
}

#ifdef ENABLE_WALLET
void BitcoinApplication::createPaymentServer() { paymentServer = new PaymentServer(this); }
#endif

void BitcoinApplication::createPlatformStyle()
{
    std::string platformName;
    platformName = GetArg("-uiplatform", DEFAULT_UIPLATFORM);
    platformStyle = PlatformStyle::instantiate(QString::fromStdString(platformName));
    if (!platformStyle) // Fall back to "other" if specified name not found
        platformStyle = PlatformStyle::instantiate("other");
    assert(platformStyle);
}

void BitcoinApplication::createOptionsModel(bool resetSettings)
{
    optionsModel = new OptionsModel(NULL, resetSettings);
    unlimitedModel = new UnlimitedModel(); // BU
}

void BitcoinApplication::createWindow(const NetworkStyle *networkStyle)
{
    window = new BitcoinGUI(platformStyle, networkStyle, 0);

    pollShutdownTimer = new QTimer(window);
    connect(pollShutdownTimer, SIGNAL(timeout()), window, SLOT(detectShutdown()));
    pollShutdownTimer->start(200);
}

void BitcoinApplication::createSplashScreen(const NetworkStyle *networkStyle)
{
    SplashScreen *splash = new SplashScreen(0, networkStyle);
    // We don't hold a direct pointer to the splash screen after creation, so use
    // Qt::WA_DeleteOnClose to make sure that the window will be deleted eventually.
    splash->setAttribute(Qt::WA_DeleteOnClose);
    splash->show();
    connect(this, SIGNAL(splashFinished(QWidget *)), splash, SLOT(slotFinish(QWidget *)));
}

void BitcoinApplication::startThread()
{
    if (coreThread)
        return;
    coreThread = new QThread(this);
    BitcoinCore *executor = new BitcoinCore();
    executor->moveToThread(coreThread);

    /*  communication to and from thread */
    connect(executor, SIGNAL(initializeResult(int)), this, SLOT(initializeResult(int)));
    connect(executor, SIGNAL(shutdownResult(int)), this, SLOT(shutdownResult(int)));
    connect(executor, SIGNAL(runawayException(QString)), this, SLOT(handleRunawayException(QString)));
    connect(this, SIGNAL(requestedInitialize()), executor, SLOT(initialize()));
    connect(this, SIGNAL(requestedShutdown()), executor, SLOT(shutdown()));
    /*  make sure executor object is deleted in its own thread */
    connect(this, SIGNAL(stopThread()), executor, SLOT(deleteLater()));
    connect(this, SIGNAL(stopThread()), coreThread, SLOT(quit()));

    coreThread->start();
}

void BitcoinApplication::parameterSetup()
{
    InitLogging();
    InitParameterInteraction();
}

void BitcoinApplication::requestInitialize()
{
    qDebug() << __func__ << ": Requesting initialize";
    startThread();
    Q_EMIT requestedInitialize();
}

void BitcoinApplication::requestShutdown()
{
    qDebug() << __func__ << ": Requesting shutdown";
    startThread();
    window->hide();
    window->setClientModel(0);
    pollShutdownTimer->stop();

#ifdef ENABLE_WALLET
    window->removeAllWallets();
    delete walletModel;
    walletModel = 0;
#endif
    delete clientModel;
    clientModel = 0;

    // Show a simple window indicating shutdown status
    ShutdownWindow::showShutdownWindow(window);

    // Request shutdown from core thread
    Q_EMIT requestedShutdown();
}

void BitcoinApplication::initializeResult(int retval)
{
    qDebug() << __func__ << ": Initialization result: " << retval;
    // Set exit result: 0 if successful, 1 if failure
    returnValue = retval ? 0 : 1;
    if (retval)
    {
        // Log this only after AppInit2 finishes, as then logging setup is guaranteed complete
        qWarning() << "Platform customization:" << platformStyle->getName();
#ifdef ENABLE_WALLET
        PaymentServer::LoadRootCAs();
        paymentServer->setOptionsModel(optionsModel);
#endif

        clientModel = new ClientModel(optionsModel, unlimitedModel);
        window->setClientModel(clientModel);

#ifdef ENABLE_WALLET
        if (pwalletMain)
        {
            walletModel = new WalletModel(platformStyle, pwalletMain, optionsModel);

            window->addWallet(BitcoinGUI::DEFAULT_WALLET, walletModel);
            window->setCurrentWallet(BitcoinGUI::DEFAULT_WALLET);

            connect(walletModel, SIGNAL(coinsSent(CWallet *, SendCoinsRecipient, QByteArray)), paymentServer,
                SLOT(fetchPaymentACK(CWallet *, const SendCoinsRecipient &, QByteArray)));
        }
#endif

        // If -min option passed, start window minimized.
        if (GetBoolArg("-min", false))
        {
            window->showMinimized();
        }
        else
        {
            window->show();
        }
        Q_EMIT splashFinished(window);

#ifdef ENABLE_WALLET
        // Now that initialization/startup is done, process any command-line
        // bitcoin: URIs or payment requests:
        connect(paymentServer, SIGNAL(receivedPaymentRequest(SendCoinsRecipient)), window,
            SLOT(handlePaymentRequest(SendCoinsRecipient)));
        connect(window, SIGNAL(receivedURI(QString)), paymentServer, SLOT(handleURIOrFile(QString)));
        connect(paymentServer, SIGNAL(message(QString, QString, unsigned int)), window,
            SLOT(message(QString, QString, unsigned int)));
        QTimer::singleShot(100, paymentServer, SLOT(uiReady()));
#endif
    }
    else
    {
        quit(); // Exit main loop
    }
}

void BitcoinApplication::shutdownResult(int retval)
{
    qDebug() << __func__ << ": Shutdown result: " << retval;
    quit(); // Exit main loop after shutdown finished
}

void BitcoinApplication::handleRunawayException(const QString &message)
{
    QMessageBox::critical(0, "Runaway exception",
        BitcoinGUI::tr("A fatal error occurred. Bitcoin can no longer continue safely and will quit.") +
            QString("\n\n") + message);
    ::exit(EXIT_FAILURE);
}

WId BitcoinApplication::getMainWinId() const
{
    if (!window)
        return 0;

    return window->winId();
}

const char *APP_SETTINGS_MIGRATED_FLAG = "fMigrated";
/**
* Checks to see if Qt App Settings have already had migration performed, based
* on the presence of setting `fMigrated` with a value of true.
* @param[in] to      App settings to check for prior migration and writability
* @param[in] from    App settings to check for keys to migrate
* @return true if migration is possible and not yet performed, otherwise false
*/
bool CanMigrateQtAppSettings(const QSettings &to, const QSettings &from)
{
    // first check to see if the desired settings are already marked as migrated
    if (to.value(APP_SETTINGS_MIGRATED_FLAG, false).toBool())
        return false;

    // next verify that the source settings actually exist/have values
    if (from.allKeys().size() <= 0)
        return false;

    // last verify that the desired settings are writable
    if (!to.isWritable())
        return error("%s: App Settings are not writable, migration skipped.", __func__);

    return true;
}

/**
* Create a backup of Qt App Settings
* Backup will only be performed if there are settings to backup and the backup
* location is writable.
* @param[in] source      App settings to be backed up
* @param[in] backupName  Backup location name
* @return true if backup was successful or there were no settings to backup, otherwise false
*/
bool BackupQtAppSettings(const QSettings &source, const QString &backupName)
{
    // parameter saftey check
    if (backupName.trimmed().size() <= 0)
        return error("%s: Parameter backupName must contain a non-whitespace value.", __func__);

    // verify there are settings to actually backup (if not just return true)
    if (source.allKeys().size() <= 0)
        return true;

    // The backup settings location
    QSettings backup(backupName, source.applicationName());

    // verify that the backup location is writable
    if (!backup.isWritable())
        return error("%s: Unable to backup existing App Settings, backup location is not writable.", __func__);

    // Get the list of all keys in the source location
    QStringList keys = source.allKeys();

    // Loop through all keys in the source location
    // NOTE: Loop may not handle sub-keys correctly w/o modification (though currently there aren't any sub-keys)
    Q_FOREACH (const QString &key, keys)
    {
        // copy every setting in source to backukp
        backup.setValue(key, source.value(key));
    }

    LogPrintf("APP SETTINGS: Settings successfully backed up to '%s'\n", backupName.toStdString());

    // NOTE: backup will go out of scope upon return so we don't need to manually call sync()

    return true;
}

/**
* Migrates Qt App Settings from a previously installed alternate client
* implementation (Core, XT, Classic, pre-1.0.1 BU).
* Migration will only be performed if there are alternate settings and a prior
* migration has not been performed.
* @param[in] oldOrg    Org name to migrate settings from
* @param[in] oldApp    App name to migrate settings from
* @param[in] newOrg    Org name to migrate settings to
* @param[in] newApp    App name to migrate settings to
* @return true if migration was performed, otherwise false
* @see CanMigrateQtAppSettings()
* @see BackupQtAppSettings()
*/
bool TryMigrateQtAppSettings(const QString &oldOrg, const QString &oldApp, const QString &newOrg, const QString &newApp)
{
    // parameter saftey checks
    if (oldOrg.trimmed().size() <= 0)
        return error("%s: Parameter oldOrg must contain a non-whitespace value.", __func__);
    if (oldApp.trimmed().size() <= 0)
        return error("%s: Parameter oldApp must contain a non-whitespace value.", __func__);
    if (newOrg.trimmed().size() <= 0)
        return error("%s: Parameter newOrg must contain a non-whitespace value.", __func__);
    if (newApp.trimmed().size() <= 0)
        return error("%s: Parameter newApp must contain a non-whitespace value.", __func__);

    // The desired settings location
    QSettings sink(newOrg, newApp);
    // The previous settings location
    QSettings source(oldOrg, oldApp);

    // Check to see if we actually can/need to migrate
    if (!CanMigrateQtAppSettings(sink, source))
        return false;

    // as a saftey precaution save a backup copy of the current settings prior to overwriting
    if (!BackupQtAppSettings(sink, newOrg + ".bak"))
        return false;

    // Get the list of all keys in the source location
    QStringList keys = source.allKeys();

    // Loop through all keys in the source location
    // NOTE: Loop may not handle sub-keys correctly w/o modification (though currently there aren't any sub-keys)
    Q_FOREACH (const QString &key, keys)
    {
        // copy every setting in source to sink, even if the key is empty
        sink.setValue(key, source.value(key));
    }

    // lastly we need to add the flag which indicates we have performed a migration
    sink.setValue(APP_SETTINGS_MIGRATED_FLAG, true);

    LogPrintf("APP SETTINGS: Settings successfully migrated from '%s/%s' to '%s/%s'\n", oldOrg.toStdString(),
        oldApp.toStdString(), newOrg.toStdString(), newApp.toStdString());

    // NOTE: sink will go out of scope upon return so we don't need to manually call sync()

    return true;
}

#ifndef BITCOIN_QT_TEST
int main(int argc, char *argv[])
{
    SetupEnvironment();

// Do not refer to data directory yet, this can be overridden by Intro::pickDataDirectory

/// 1. Basic Qt initialization (not dependent on parameters or configuration)
#if QT_VERSION < 0x050000
    // Internal string conversion is all UTF-8
    QTextCodec::setCodecForTr(QTextCodec::codecForName("UTF-8"));
    QTextCodec::setCodecForCStrings(QTextCodec::codecForTr());
#endif

    Q_INIT_RESOURCE(bitcoin);
    Q_INIT_RESOURCE(bitcoin_locale);

    BitcoinApplication app(argc, argv);
#if QT_VERSION > 0x050100
    // Generate high-dpi pixmaps
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif
#if QT_VERSION >= 0x050600
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
#ifdef Q_OS_MAC
    QApplication::setAttribute(Qt::AA_DontShowIconsInMenus);
#endif
#if QT_VERSION >= 0x050500
    // Because of the POODLE attack it is recommended to disable SSLv3 (https://disablessl3.com/),
    // so set SSL protocols to TLS1.0+.
    QSslConfiguration sslconf = QSslConfiguration::defaultConfiguration();
    sslconf.setProtocol(QSsl::TlsV1_0OrLater);
    QSslConfiguration::setDefaultConfiguration(sslconf);
#endif

    // Register meta types used for QMetaObject::invokeMethod
    qRegisterMetaType<bool *>();
    // qt-project.org/doc/qt-5/qmetatype.html#qRegisterMetaType)
    //   Need to pass name here as CAmount is a typedef (see http:
    //   IMPORTANT if it is no longer a typedef use the normal variant above
    qRegisterMetaType<CAmount>("CAmount");

    /// 2. Parse command-line options. Command-line options take precedence:
    AllowedArgs::BitcoinQt allowedArgs(&tweaks);
    try
    {
        ParseParameters(argc, argv, allowedArgs);
    }
    catch (const std::exception &e)
    {
        QMessageBox::critical(
            0, QObject::tr("Bitcoin"), QObject::tr("Error: Cannot parse program options: %1.").arg(e.what()));
        return EXIT_FAILURE;
    }

/// 3. Migrate application settings, if necessary
// BU changed the QAPP_ORG_NAME and since this is used for reading the app settings
// from the registry (Windows) or a configuration file (Linux/OSX)
// we need to check to see if we need to migrate old settings to the new location
#ifdef BITCOIN_CASH
    bool fMigrated = false;
    // For BUCash, first try to migrate from BTC BU settings
    fMigrated = TryMigrateQtAppSettings(QAPP_ORG_NAME, QAPP_APP_NAME_DEFAULT, QAPP_ORG_NAME, QAPP_APP_NAME_BUCASH);
    // Then try to migrate from non-BU client settings (if we didn't just migrate from BU settings)
    fMigrated = fMigrated || TryMigrateQtAppSettings(
                                 QAPP_ORG_NAME_LEGACY, QAPP_APP_NAME_DEFAULT, QAPP_ORG_NAME, QAPP_APP_NAME_BUCASH);

    // If we just migrated and this is a BUcash node, have the user reconfirm the data directory.
    // This is necessary in case the user wants to run side-by-side BTC chain and BCC chain nodes
    // in which case each instance requires a different data directory.
    if (fMigrated)
        SoftSetBoolArg("-choosedatadir", true);
#else
    // Try to migrate from non-BU client settings
    TryMigrateQtAppSettings(QAPP_ORG_NAME_LEGACY, QAPP_APP_NAME_DEFAULT, QAPP_ORG_NAME, QAPP_APP_NAME_DEFAULT);
#endif

    /// 4. Application identification
    // must be set before OptionsModel is initialized or translations are loaded,
    // as it is used to locate QSettings
    QApplication::setOrganizationName(QAPP_ORG_NAME);
    QApplication::setOrganizationDomain(QAPP_ORG_DOMAIN);
#ifdef BITCOIN_CASH
    // Use a different app name for BUCash to enable side-by-side installations which won't
    // interfere with each other
    QApplication::setApplicationName(QAPP_APP_NAME_BUCASH);
#else
    QApplication::setApplicationName(QAPP_APP_NAME_DEFAULT);
#endif
    GUIUtil::SubstituteFonts(GetLangTerritory());

    /// 5. Initialization of translations, so that intro dialog is in user's language
    // Now that QSettings are accessible, initialize translations
    QTranslator qtTranslatorBase, qtTranslator, translatorBase, translator;
    initTranslations(qtTranslatorBase, qtTranslator, translatorBase, translator);
    translationInterface.Translate.connect(Translate);

    // Show help message immediately after parsing command-line options (for "-lang") and setting locale,
    // but before showing splash screen.
    if (mapArgs.count("-?") || mapArgs.count("-h") || mapArgs.count("-help") || mapArgs.count("-version"))
    {
        HelpMessageDialog help(NULL, mapArgs.count("-version"));
        help.showOrPrint();
        return EXIT_SUCCESS;
    }

    /// 6. Now that settings and translations are available, ask user for data directory
    // User language is set up: pick a data directory
    if (!Intro::pickDataDirectory())
        return EXIT_FAILURE;

    /// 7. Determine availability of data directory and parse bitcoin.conf
    /// - Do not call GetDataDir(true) before this step finishes
    if (!fs::is_directory(GetDataDir(false)))
    {
        QMessageBox::critical(
            0, QObject::tr(PACKAGE_NAME), QObject::tr("Error: Specified data directory \"%1\" does not exist.")
                                              .arg(QString::fromStdString(mapArgs["-datadir"])));
        return EXIT_FAILURE;
    }
    try
    {
        ReadConfigFile(mapArgs, mapMultiArgs, allowedArgs);
    }
    catch (const std::exception &e)
    {
        QMessageBox::critical(0, QObject::tr(PACKAGE_NAME),
            QObject::tr("Error: Cannot parse configuration file: %1. Only use key=value syntax.").arg(e.what()));
        return EXIT_FAILURE;
    }

    // UI per-platform customization
    app.createPlatformStyle();

    /// 8. Determine network (and switch to network specific options)
    // - Do not call Params() before this step
    // - Do this after parsing the configuration file, as the network can be switched there
    // - QSettings() will use the new application name after this, resulting in network-specific settings
    // - Needs to be done before createOptionsModel

    // Check for -testnet or -regtest parameter (Params() calls are only valid after this clause)
    try
    {
        SelectParams(ChainNameFromCommandLine());
    }
    catch (std::exception &e)
    {
        QMessageBox::critical(0, QObject::tr(PACKAGE_NAME), QObject::tr("Error: %1").arg(e.what()));
        return EXIT_FAILURE;
    }
#ifdef ENABLE_WALLET
    // Parse URIs on command line -- this can affect Params()
    PaymentServer::ipcParseCommandLine(argc, argv);
#endif

    QScopedPointer<const NetworkStyle> networkStyle(
        NetworkStyle::instantiate(QString::fromStdString(Params().NetworkIDString())));
    assert(!networkStyle.isNull());
    // Allow for separate UI settings for testnets
    QApplication::setApplicationName(networkStyle->getAppName());
    // Re-initialize translations after changing application name (language in network-specific settings can be
    // different)
    initTranslations(qtTranslatorBase, qtTranslator, translatorBase, translator);

#ifdef ENABLE_WALLET
    /// 9. URI IPC sending
    // - Do this early as we don't want to bother initializing if we are just calling IPC
    // - Do this *after* setting up the data directory, as the data directory hash is used in the name
    // of the server.
    // - Do this after creating app and setting up translations, so errors are
    // translated properly.
    if (PaymentServer::ipcSendCommandLine())
        exit(EXIT_SUCCESS);

    // Start up the payment server early, too, so impatient users that click on
    // bitcoin: links repeatedly have their payment requests routed to this process:
    app.createPaymentServer();
#endif

    /// 10. Main GUI initialization
    // Install global event filter that makes sure that long tooltips can be word-wrapped
    app.installEventFilter(new GUIUtil::ToolTipToRichTextFilter(TOOLTIP_WRAP_THRESHOLD, &app));
#if QT_VERSION < 0x050000
    // Install qDebug() message handler to route to debug.log
    qInstallMsgHandler(DebugMessageHandler);
#else
#if defined(Q_OS_WIN)
    // Install global event filter for processing Windows session related Windows messages (WM_QUERYENDSESSION and
    // WM_ENDSESSION)
    qApp->installNativeEventFilter(new WinShutdownMonitor());
#endif
    // Install qDebug() message handler to route to debug.log
    qInstallMessageHandler(DebugMessageHandler);
#endif
    // Allow parameter interaction before we create the options model
    app.parameterSetup();
    // Load GUI settings from QSettings
    app.createOptionsModel(GetBoolArg("-resetguisettings", false));

    // Subscribe to global signals from core
    uiInterface.InitMessage.connect(InitMessage);

    if (GetBoolArg("-splash", DEFAULT_SPLASHSCREEN) && !GetBoolArg("-min", false))
        app.createSplashScreen(networkStyle.data());

    UnlimitedSetup();

    try
    {
        app.createWindow(networkStyle.data());
        app.requestInitialize();
#if defined(Q_OS_WIN) && QT_VERSION >= 0x050000
        WinShutdownMonitor::registerShutdownBlockReason(
            QObject::tr("%1 didn't yet exit safely...").arg(QObject::tr(PACKAGE_NAME)), (HWND)app.getMainWinId());
#endif
        app.exec();
        app.requestShutdown();
        app.exec();
    }
    catch (const std::exception &e)
    {
        PrintExceptionContinue(&e, "Runaway exception");
        app.handleRunawayException(QString::fromStdString(strMiscWarning));
    }
    catch (...)
    {
        PrintExceptionContinue(NULL, "Runaway exception");
        app.handleRunawayException(QString::fromStdString(strMiscWarning));
    }

    return app.getReturnValue();
}
#endif // BITCOIN_QT_TEST
