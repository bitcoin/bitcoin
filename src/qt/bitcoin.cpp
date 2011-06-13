/*
 * W.J. van der Laan 2011
 */
#include "bitcoingui.h"
#include "clientmodel.h"
#include "util.h"
#include "init.h"
#include "main.h"
#include "externui.h"

#include <QApplication>
#include <QMessageBox>
#include <QThread>

// Need a global reference for the notifications to find the GUI
BitcoinGUI *guiref;

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

    /* Call slot on GUI thread.
       If called from another thread, use a blocking QueuedConnection.
     */
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
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);

    try
    {
        if(AppInit2(argc, argv))
        {
            BitcoinGUI window;
            ClientModel model;
            guiref = &window;
            window.setModel(&model);

            window.show();

            int retval = app.exec();

            guiref = 0;
            Shutdown(NULL);

            return retval;
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
}
