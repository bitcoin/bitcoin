#ifndef TALKCOINMOBILEGUI_H
#define TALKCOINMOBILEGUI_H

#include <QtQuick/QQuickView>
#include <memory>
#include <interfaces/handler.h>
#include <interfaces/wallet.h>
#include <qt/transactionfilterproxy.h>

class PlatformStyle;
class NetworkStyle;
class ClientModel;
class WalletController;
class WalletModel;

namespace interfaces {
class Node;
}

class TalkcoinMobileGUI : public QQuickView
{
    Q_OBJECT

public:
    TalkcoinMobileGUI(interfaces::Node& node, const PlatformStyle *m_platformStyle, const NetworkStyle *networkStyle, QWindow *parent = nullptr);
    void setClientModel(ClientModel *m_clientModel);
    void setWalletController(WalletController *wallet_controller);
    void unsubscribeFromCoreSignals();

public Q_SLOTS:
    void showInitMessage(const QString &message, int alignment, const QColor &color);
    void message(const QString &title, QString message, unsigned int style, bool *ret);
    void splashFinished();

private Q_SLOTS:
    void requestTalkcoin();
    void sendTalkcoin(QString address, int amount, int confirmations);
    void setClipboard(QString text);
    void setDisplayUnit(int index);
    void executeRpcCommand(QString command);

private:
    void addWallet(WalletModel* m_walletModel);
    void subscribeToCoreSignals();
    void setWalletModel(WalletModel *model);
    void updateDisplayUnit();
    void setBalance(const interfaces::WalletBalances &balances);

private:
    interfaces::Node& m_node;
    const PlatformStyle *m_platformStyle;
    const NetworkStyle* const m_networkStyle;
    ClientModel* m_clientModel = nullptr;
    WalletController* m_walletController{nullptr};
    WalletModel* m_walletModel;

    QObject* m_walletPane;
    QObject* m_sendPane;
    QObject* m_consolePane;

    std::unique_ptr<interfaces::Handler> m_handler_init_message;
    std::unique_ptr<interfaces::Handler> m_handler_show_progress;
    std::unique_ptr<interfaces::Handler> m_handler_load_wallet;

    std::unique_ptr<interfaces::Handler> m_handler_message_box;
    std::unique_ptr<interfaces::Handler> m_handler_question;

    std::unique_ptr<TransactionFilterProxy> filter;

    interfaces::WalletBalances m_balances;
    int m_displayUnit;
};

#endif // TALKCOINMOBILEGUI_H
