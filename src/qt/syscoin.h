// Copyright (c) 2011-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_QT_SYSCOIN_H
#define SYSCOIN_QT_SYSCOIN_H

#if defined(HAVE_CONFIG_H)
#include <config/syscoin-config.h>
#endif

#include <QApplication>
#include <assert.h>
#include <memory>

#include <interfaces/node.h>

class SyscoinGUI;
class ClientModel;
class NetworkStyle;
class OptionsModel;
class PaymentServer;
class PlatformStyle;
class SplashScreen;
class WalletController;
class WalletModel;

QT_BEGIN_NAMESPACE
// SYSCOIN
class QStringList;
QT_END_NAMESPACE
/** Class encapsulating Syscoin Core startup and shutdown.
 * Allows running startup and shutdown in a different thread from the UI thread.
 */
class SyscoinCore: public QObject
{
    Q_OBJECT
public:
    explicit SyscoinCore(interfaces::Node& node);

public Q_SLOTS:
    void initialize();
    void shutdown();
    // SYSCOIN
    void restart(const QStringList &args);

Q_SIGNALS:
    void initializeResult(bool success, interfaces::BlockAndHeaderTipInfo tip_info);
    void shutdownResult();
    void runawayException(const QString &message);

private:
    /// Pass fatal exception message to UI thread
    void handleRunawayException(const std::exception *e);

    interfaces::Node& m_node;
};

/** Main Syscoin application object */
class SyscoinApplication: public QApplication
{
    Q_OBJECT
public:
    explicit SyscoinApplication();
    ~SyscoinApplication();

#ifdef ENABLE_WALLET
    /// Create payment server
    void createPaymentServer();
#endif
    /// parameter interaction/setup based on rules
    void parameterSetup();
    /// Create options model
    void createOptionsModel(bool resetSettings);
    /// Initialize prune setting
    void InitializePruneSetting(bool prune);
    /// Create main window
    void createWindow(const NetworkStyle *networkStyle);
    /// Create splash screen
    void createSplashScreen(const NetworkStyle *networkStyle);
    /// Basic initialization, before starting initialization/shutdown thread. Return true on success.
    bool baseInitialize();

    /// Request core initialization
    void requestInitialize();
    /// Request core shutdown
    void requestShutdown();

    /// Get process return value
    int getReturnValue() const { return returnValue; }

    /// Get window identifier of QMainWindow (SyscoinGUI)
    WId getMainWinId() const;

    /// Setup platform style
    void setupPlatformStyle();

    interfaces::Node& node() const { assert(m_node); return *m_node; }
    void setNode(interfaces::Node& node);

public Q_SLOTS:
    void initializeResult(bool success, interfaces::BlockAndHeaderTipInfo tip_info);
    void shutdownResult();
    /// Handle runaway exceptions. Shows a message box with the problem and quits the program.
    void handleRunawayException(const QString &message);

    /**
     * A helper function that shows a message box
     * with details about a non-fatal exception.
     */
    void handleNonFatalException(const QString& message);

Q_SIGNALS:
    void requestedInitialize();
    void requestedShutdown();
    void splashFinished();
    void windowShown(SyscoinGUI* window);

private:
    QThread *coreThread;
    OptionsModel *optionsModel;
    ClientModel *clientModel;
    SyscoinGUI *window;
    QTimer *pollShutdownTimer;
#ifdef ENABLE_WALLET
    PaymentServer* paymentServer{nullptr};
    WalletController* m_wallet_controller{nullptr};
#endif
    int returnValue;
    const PlatformStyle *platformStyle;
    std::unique_ptr<QWidget> shutdownWindow;
    SplashScreen* m_splash = nullptr;
    interfaces::Node* m_node = nullptr;

    void startThread();
};

int GuiMain(int argc, char* argv[]);

#endif // SYSCOIN_QT_SYSCOIN_H
