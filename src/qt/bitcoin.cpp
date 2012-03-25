/*
 * W.J. van der Laan 2011-2012
 */
#include "bitcoingui.h"
#include "clientmodel.h"
#include "walletmodel.h"
#include "optionsmodel.h"
#include "guiutil.h"

#include "headers.h"
#include "init.h"
#include "qtipcserver.h"

#include <QApplication>
#include <QMessageBox>
#include <QTextCodec>
#include <QLocale>
#include <QTranslator>
#include <QSplashScreen>
#include <QLibraryInfo>

#include <boost/interprocess/ipc/message_queue.hpp>

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

// Need a global reference for the notifications to find the GUI
static BitcoinGUI *guiref;
static QSplashScreen *splashref;
static WalletModel *walletmodel;
static ClientModel *clientmodel;

int MyMessageBox(const std::string& message, const std::string& caption, int style, wxWindow* parent, int x, int y)
{
    // Message from AppInit2(), always in main thread before main window is constructed
    QMessageBox::critical(0, QString::fromStdString(caption),
        QString::fromStdString(message),
        QMessageBox::Ok, QMessageBox::Ok);
    return 4;
}

int ThreadSafeMessageBox(const std::string& message, const std::string& caption, int style, wxWindow* parent, int x, int y)
{
    // Message from network thread
    if(guiref)
    {
        QMetaObject::invokeMethod(guiref, "error", Qt::QueuedConnection,
                                   Q_ARG(QString, QString::fromStdString(caption)),
                                   Q_ARG(QString, QString::fromStdString(message)));
    }
    else
    {
        printf("%s: %s\n", caption.c_str(), message.c_str());
        fprintf(stderr, "%s: %s\n", caption.c_str(), message.c_str());
    }
    return 4;
}

bool ThreadSafeAskFee(int64 nFeeRequired, const std::string& strCaption, wxWindow* parent)
{
    if(!guiref)
        return false;
    if(nFeeRequired < MIN_TX_FEE || nFeeRequired <= nTransactionFee || fDaemon)
        return true;
    bool payFee = false;

    QMetaObject::invokeMethod(guiref, "askFee", GUIUtil::blockingGUIThreadConnection(),
                               Q_ARG(qint64, nFeeRequired),
                               Q_ARG(bool*, &payFee));

    return payFee;
}

void ThreadSafeHandleURL(const std::string& strURL)
{
    if(!guiref)
        return;

    QMetaObject::invokeMethod(guiref, "handleURL", GUIUtil::blockingGUIThreadConnection(),
                               Q_ARG(QString, QString::fromStdString(strURL)));
}

void MainFrameRepaint()
{
    if(clientmodel)
        QMetaObject::invokeMethod(clientmodel, "update", Qt::QueuedConnection);
    if(walletmodel)
        QMetaObject::invokeMethod(walletmodel, "update", Qt::QueuedConnection);
}

void AddressBookRepaint()
{
    if(walletmodel)
        QMetaObject::invokeMethod(walletmodel, "updateAddressList", Qt::QueuedConnection);
}

void InitMessage(const std::string &message)
{
    if(splashref)
    {
        splashref->showMessage(QString::fromStdString(message), Qt::AlignBottom|Qt::AlignHCenter, QColor(255,255,200));
        QApplication::instance()->processEvents();
    }
}

void QueueShutdown()
{
    QMetaObject::invokeMethod(QCoreApplication::instance(), "quit", Qt::QueuedConnection);
}

/*
   Translate string to current locale using Qt.
 */
std::string _(const char* psz)
{
    return QCoreApplication::translate("bitcoin-core", psz).toStdString();
}

#ifdef WIN32
#define strncasecmp strnicmp
#endif
#ifndef BITCOIN_QT_TEST
int main(int argc, char *argv[])
{
#if !defined(MAC_OSX) && !defined(WIN32)
// TODO: implement qtipcserver.cpp for Mac and Windows

    // Do this early as we don't want to bother initializing if we are just calling IPC
    for (int i = 1; i < argc; i++)
    {
        if (strlen(argv[i]) > 7 && strncasecmp(argv[i], "bitcoin:", 8) == 0)
        {
            const char *strURL = argv[i];
            try {
                boost::interprocess::message_queue mq(boost::interprocess::open_only, "BitcoinURL");
                if(mq.try_send(strURL, strlen(strURL), 0))
                    exit(0);
                else
                    break;
            }
            catch (boost::interprocess::interprocess_exception &ex) {
                break;
            }
        }
    }
#endif

    // Internal string conversion is all UTF-8
    QTextCodec::setCodecForTr(QTextCodec::codecForName("UTF-8"));
    QTextCodec::setCodecForCStrings(QTextCodec::codecForTr());

    Q_INIT_RESOURCE(bitcoin);
    QApplication app(argc, argv);

    // Command-line options take precedence:
    ParseParameters(argc, argv);

    // ... then bitcoin.conf:
    if (!ReadConfigFile(mapArgs, mapMultiArgs))
    {
        fprintf(stderr, "Error: Specified directory does not exist\n");
        return 1;
    }

    // Application identification (must be set before OptionsModel is initialized,
    // as it is used to locate QSettings)
    app.setOrganizationName("Bitcoin");
    app.setOrganizationDomain("bitcoin.org");
    if(GetBoolArg("-testnet")) // Separate UI settings for testnet
        app.setApplicationName("Bitcoin-Qt-testnet");
    else
        app.setApplicationName("Bitcoin-Qt");

    // ... then GUI settings:
    OptionsModel optionsModel;

    // Get desired locale ("en_US") from command line or system locale
    QString lang_territory = QString::fromStdString(GetArg("-lang", QLocale::system().name().toStdString()));
    // Load language files for configured locale:
    // - First load the translator for the base language, without territory
    // - Then load the more specific locale translator
    QString lang = lang_territory;

    lang.truncate(lang_territory.lastIndexOf('_')); // "en"
    QTranslator qtTranslatorBase, qtTranslator, translatorBase, translator;

    qtTranslatorBase.load(QLibraryInfo::location(QLibraryInfo::TranslationsPath) + "/qt_" + lang);
    if (!qtTranslatorBase.isEmpty())
        app.installTranslator(&qtTranslatorBase);

    qtTranslator.load(QLibraryInfo::location(QLibraryInfo::TranslationsPath) + "/qt_" + lang_territory);
    if (!qtTranslator.isEmpty())
        app.installTranslator(&qtTranslator);

    translatorBase.load(":/translations/"+lang);
    if (!translatorBase.isEmpty())
        app.installTranslator(&translatorBase);

    translator.load(":/translations/"+lang_territory);
    if (!translator.isEmpty())
        app.installTranslator(&translator);

    QSplashScreen splash(QPixmap(":/images/splash"), 0);
    if (GetBoolArg("-splash", true) && !GetBoolArg("-min"))
    {
        splash.show();
        splash.setAutoFillBackground(true);
        splashref = &splash;
    }

    app.processEvents();

    app.setQuitOnLastWindowClosed(false);

    try
    {
        if(AppInit2(argc, argv))
        {
            {
                // Put this in a block, so that BitcoinGUI is cleaned up properly before
                // calling Shutdown() in case of exceptions.

                optionsModel.Upgrade(); // Must be done after AppInit2

                BitcoinGUI window;
                if (splashref)
                    splash.finish(&window);

                ClientModel clientModel(&optionsModel);
                clientmodel = &clientModel;
                WalletModel walletModel(pwalletMain, &optionsModel);
                walletmodel = &walletModel;

                guiref = &window;
                window.setClientModel(&clientModel);
                window.setWalletModel(&walletModel);

                // If -min option passed, start window minimized.
                if(GetBoolArg("-min"))
                {
                    window.showMinimized();
                }
                else
                {
                    window.show();
                }

                // Place this here as guiref has to be defined if we dont want to lose URLs
                ipcInit();

#if !defined(MAC_OSX) && !defined(WIN32)
// TODO: implement qtipcserver.cpp for Mac and Windows

                // Check for URL in argv
                for (int i = 1; i < argc; i++)
                {
                    if (strlen(argv[i]) > 7 && strncasecmp(argv[i], "bitcoin:", 8) == 0)
                    {
                        const char *strURL = argv[i];
                        try {
                            boost::interprocess::message_queue mq(boost::interprocess::open_only, "BitcoinURL");
                            mq.try_send(strURL, strlen(strURL), 0);
                        }
                        catch (boost::interprocess::interprocess_exception &ex) {
                        }
                    }
                }
#endif
                app.exec();

                guiref = 0;
                clientmodel = 0;
                walletmodel = 0;
            }
            Shutdown(NULL);
        }
        else
        {
            return 1;
        }
    } catch (std::exception& e) {
        PrintException(&e, "Runaway exception");
    } catch (...) {
        PrintException(NULL, "Runaway exception");
    }
    return 0;
}
#endif // BITCOIN_QT_TEST
