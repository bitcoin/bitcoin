/*
 * W.J. van der Laan 2011-2012
 */

#include "bitcoingui.h"
#include "clientmodel.h"
#include "walletmodel.h"
#include "optionsmodel.h"
#include "guiutil.h"
#include "guiconstants.h"
#include "init.h"
#include "util.h"
#include "ui_interface.h"
#include "paymentserver.h"
#include "splashscreen.h"
#include "intro.h"

#include <QApplication>
#include <QMessageBox>
#if QT_VERSION < 0x050000
#include <QTextCodec>
#endif
#include <QLocale>
#include <QTimer>
#include <QTranslator>
#include <QLibraryInfo>
#include <QSettings>

#if defined(BITCOIN_NEED_QT_PLUGINS) && !defined(_BITCOIN_QT_PLUGINS_INCLUDED)
#define _BITCOIN_QT_PLUGINS_INCLUDED
#define __INSURE__
#include <QtPlugin>
Q_IMPORT_PLUGIN(qcncodecs)
Q_IMPORT_PLUGIN(qjpcodecs)
Q_IMPORT_PLUGIN(qtwcodecs)
Q_IMPORT_PLUGIN(qkrcodecs)
Q_IMPORT_PLUGIN(qtaccessiblewidgets)
#endif

// Declare meta types used for QMetaObject::invokeMethod
Q_DECLARE_METATYPE(bool*)

// Need a global reference for the notifications to find the GUI
static BitcoinGUI *guiref;
static SplashScreen *splashref;

static bool ThreadSafeMessageBox(const std::string& message, const std::string& caption, unsigned int style)
{
    // Message from network thread
    if(guiref)
    {
        bool modal = (style & CClientUIInterface::MODAL);
        bool ret = false;
        // In case of modal message, use blocking connection to wait for user to click a button
        QMetaObject::invokeMethod(guiref, "message",
                                   modal ? GUIUtil::blockingGUIThreadConnection() : Qt::QueuedConnection,
                                   Q_ARG(QString, QString::fromStdString(caption)),
                                   Q_ARG(QString, QString::fromStdString(message)),
                                   Q_ARG(unsigned int, style),
                                   Q_ARG(bool*, &ret));
        return ret;
    }
    else
    {
        printf("%s: %s\n", caption.c_str(), message.c_str());
        fprintf(stderr, "%s: %s\n", caption.c_str(), message.c_str());
        return false;
    }
}

static bool ThreadSafeAskFee(int64 nFeeRequired)
{
    if(!guiref)
        return false;
    if(nFeeRequired < CTransaction::nMinTxFee || nFeeRequired <= nTransactionFee || fDaemon)
        return true;

    bool payFee = false;

    QMetaObject::invokeMethod(guiref, "askFee", GUIUtil::blockingGUIThreadConnection(),
                               Q_ARG(qint64, nFeeRequired),
                               Q_ARG(bool*, &payFee));

    return payFee;
}

static void InitMessage(const std::string &message)
{
    if(splashref)
    {
        splashref->showMessage(QString::fromStdString(message), Qt::AlignBottom|Qt::AlignHCenter, QColor(55,55,55));
        qApp->processEvents();
    }
    printf("init message: %s\n", message.c_str());
}

/*
   Translate string to current locale using Qt.
 */
static std::string Translate(const char* psz)
{
    return QCoreApplication::translate("bitcoin-core", psz).toStdString();
}

/* Handle runaway exceptions. Shows a message box with the problem and quits the program.
 */
static void handleRunawayException(std::exception *e)
{
    PrintExceptionContinue(e, "Runaway exception");
    QMessageBox::critical(0, "Runaway exception", BitcoinGUI::tr("A fatal error occurred. Bitcoin can no longer continue safely and will quit.") + QString("\n\n") + QString::fromStdString(strMiscWarning));
    exit(1);
}

/** Set up translations */
static void initTranslations(QTranslator &qtTranslatorBase, QTranslator &qtTranslator, QTranslator &translatorBase, QTranslator &translator)
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
    lang_territory = QString::fromStdString(GetArg("-lang", lang_territory.toStdString()));

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

#ifndef BITCOIN_QT_TEST
int main(int argc, char *argv[])
{
    fHaveGUI = true;

    // Command-line options take precedence:
    ParseParameters(argc, argv);

#if QT_VERSION < 0x050000
    // Internal string conversion is all UTF-8
    QTextCodec::setCodecForTr(QTextCodec::codecForName("UTF-8"));
    QTextCodec::setCodecForCStrings(QTextCodec::codecForTr());
#endif

    Q_INIT_RESOURCE(bitcoin);
    QApplication app(argc, argv);

    // Register meta types used for QMetaObject::invokeMethod
    qRegisterMetaType< bool* >();

    // Application identification (must be set before OptionsModel is initialized,
    // as it is used to locate QSettings)
    QApplication::setOrganizationName("Bitcoin");
    QApplication::setOrganizationDomain("bitcoin.org");
    if (GetBoolArg("-testnet", false)) // Separate UI settings for testnet
        QApplication::setApplicationName("Bitcoin-Qt-testnet");
    else
        QApplication::setApplicationName("Bitcoin-Qt");

    // Now that QSettings are accessible, initialize translations
    QTranslator qtTranslatorBase, qtTranslator, translatorBase, translator;
    initTranslations(qtTranslatorBase, qtTranslator, translatorBase, translator);

    // User language is set up: pick a data directory
    Intro::pickDataDirectory();

    // Do this early as we don't want to bother initializing if we are just calling IPC
    // ... but do it after creating app, so QCoreApplication::arguments is initialized:
    if (PaymentServer::ipcSendCommandLine())
        exit(0);
    PaymentServer* paymentServer = new PaymentServer(&app);

    // Install global event filter that makes sure that long tooltips can be word-wrapped
    app.installEventFilter(new GUIUtil::ToolTipToRichTextFilter(TOOLTIP_WRAP_THRESHOLD, &app));

    // ... then bitcoin.conf:
    if (!boost::filesystem::is_directory(GetDataDir(false)))
    {
        QMessageBox::critical(0, QObject::tr("Bitcoin"),
                              QObject::tr("Error: Specified data directory \"%1\" does not exist.").arg(QString::fromStdString(mapArgs["-datadir"])));
        return 1;
    }
    ReadConfigFile(mapArgs, mapMultiArgs);

    // ... then GUI settings:
    OptionsModel optionsModel;

    // Subscribe to global signals from core
    uiInterface.ThreadSafeMessageBox.connect(ThreadSafeMessageBox);
    uiInterface.ThreadSafeAskFee.connect(ThreadSafeAskFee);
    uiInterface.InitMessage.connect(InitMessage);
    uiInterface.Translate.connect(Translate);

    // Show help message immediately after parsing command-line options (for "-lang") and setting locale,
    // but before showing splash screen.
    if (mapArgs.count("-?") || mapArgs.count("--help"))
    {
        GUIUtil::HelpMessageBox help;
        help.showOrPrint();
        return 1;
    }

    SplashScreen splash(QPixmap(), 0);
    if (GetBoolArg("-splash", true) && !GetBoolArg("-min", false))
    {
        splash.show();
        splash.setAutoFillBackground(true);
        splashref = &splash;
    }

    app.processEvents();
    app.setQuitOnLastWindowClosed(false);

    try
    {
#ifndef Q_OS_MAC
        // Regenerate startup link, to fix links to old versions
        // OSX: makes no sense on mac and might also scan/mount external (and sleeping) volumes (can take up some secs)
        if (GUIUtil::GetStartOnSystemStartup())
            GUIUtil::SetStartOnSystemStartup(true);
#endif

        boost::thread_group threadGroup;

        BitcoinGUI window(GetBoolArg("-testnet", false), 0);
        guiref = &window;

        QTimer* pollShutdownTimer = new QTimer(guiref);
        QObject::connect(pollShutdownTimer, SIGNAL(timeout()), guiref, SLOT(detectShutdown()));
        pollShutdownTimer->start(200);

        if(AppInit2(threadGroup))
        {
            {
                // Put this in a block, so that the Model objects are cleaned up before
                // calling Shutdown().

                optionsModel.Upgrade(); // Must be done after AppInit2

                if (splashref)
                    splash.finish(&window);

                ClientModel clientModel(&optionsModel);
                WalletModel walletModel(pwalletMain, &optionsModel);

                window.setClientModel(&clientModel);
                window.addWallet("~Default", &walletModel);
                window.setCurrentWallet("~Default");

                // If -min option passed, start window minimized.
                if(GetBoolArg("-min", false))
                {
                    window.showMinimized();
                }
                else
                {
                    window.show();
                }

                // Now that initialization/startup is done, process any command-line
                // bitcoin: URIs
                QObject::connect(paymentServer, SIGNAL(receivedURI(QString)), &window, SLOT(handleURI(QString)));
                QTimer::singleShot(100, paymentServer, SLOT(uiReady()));

                app.exec();

                window.hide();
                window.setClientModel(0);
                window.removeAllWallets();
                guiref = 0;
            }
            // Shutdown the core and its threads, but don't exit Bitcoin-Qt here
            threadGroup.interrupt_all();
            threadGroup.join_all();
            Shutdown();
        }
        else
        {
            threadGroup.interrupt_all();
            threadGroup.join_all();
            Shutdown();
            return 1;
        }
    } catch (std::exception& e) {
        handleRunawayException(&e);
    } catch (...) {
        handleRunawayException(NULL);
    }
    return 0;
}
#endif // BITCOIN_QT_TEST
