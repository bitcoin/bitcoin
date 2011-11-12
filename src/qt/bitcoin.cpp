/*
 * W.J. van der Laan 2011
 */
#include "bitcoingui.h"
#include "clientmodel.h"
#include "walletmodel.h"
#include "optionsmodel.h"

#include "headers.h"
#include "init.h"

#include <QApplication>
#include <QMessageBox>
#include <QThread>
#include <QTextCodec>
#include <QLocale>
#include <QTranslator>
#include <QSplashScreen>
#include <QLibraryInfo>

// Need a global reference for the notifications to find the GUI
BitcoinGUI *guiref;
QSplashScreen *splashref;

int MyMessageBox(const std::string& message, const std::string& caption, int style, wxWindow* parent, int x, int y)
{
    // Message from main thread
    if(guiref)
    {
        guiref->error(QString::fromStdString(caption),
                      QString::fromStdString(message));
    }
    else
    {
        QMessageBox::critical(0, QString::fromStdString(caption),
            QString::fromStdString(message),
            QMessageBox::Ok, QMessageBox::Ok);
    }
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

    // Call slot on GUI thread.
    // If called from another thread, use a blocking QueuedConnection.
    Qt::ConnectionType connectionType = Qt::DirectConnection;
    if(QThread::currentThread() != QCoreApplication::instance()->thread())
    {
        connectionType = Qt::BlockingQueuedConnection;
    }

    QMetaObject::invokeMethod(guiref, "askFee", connectionType,
                               Q_ARG(qint64, nFeeRequired),
                               Q_ARG(bool*, &payFee));

    return payFee;
}

void CalledSetStatusBar(const std::string& strText, int nField)
{
    // Only used for built-in mining, which is disabled, simple ignore
}

void UIThreadCall(boost::function0<void> fn)
{
    // Only used for built-in mining, which is disabled, simple ignore
}

void MainFrameRepaint()
{
    if(guiref)
        QMetaObject::invokeMethod(guiref, "refreshStatusBar", Qt::QueuedConnection);
}

void InitMessage(const std::string &message)
{
    if(splashref)
    {
        splashref->showMessage(QString::fromStdString(message), Qt::AlignBottom|Qt::AlignHCenter, QColor(255,255,200));
        QApplication::instance()->processEvents();
    }
}

/*
   Translate string to current locale using Qt.
 */
std::string _(const char* psz)
{
    return QCoreApplication::translate("bitcoin-core", psz).toStdString();
}

int main(int argc, char *argv[])
{
    // Internal string conversion is all UTF-8
    QTextCodec::setCodecForTr(QTextCodec::codecForName("UTF-8"));
    QTextCodec::setCodecForCStrings(QTextCodec::codecForTr());

    Q_INIT_RESOURCE(bitcoin);
    QApplication app(argc, argv);

    ParseParameters(argc, argv);

    // Load language files for system locale:
    // - First load the translator for the base language, without territory
    // - Then load the more specific locale translator
    QString lang_territory = QLocale::system().name(); // "en_US"
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

    app.setApplicationName(QApplication::translate("main", "Bitcoin-Qt"));

    QSplashScreen splash(QPixmap(":/images/splash"), 0);
    splash.show();
    splash.setAutoFillBackground(true);
    splashref = &splash;

    app.processEvents();

    app.setQuitOnLastWindowClosed(false);

    try
    {
        if(AppInit2(argc, argv))
        {
            {
                // Put this in a block, so that BitcoinGUI is cleaned up properly before
                // calling Shutdown() in case of exceptions.
                BitcoinGUI window;
                splash.finish(&window);
                OptionsModel optionsModel(pwalletMain);
                ClientModel clientModel(&optionsModel);
                WalletModel walletModel(pwalletMain, &optionsModel);

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

                app.exec();

                guiref = 0;
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
