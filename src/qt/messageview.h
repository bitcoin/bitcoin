/*
 * Syscoin Developers 2016
 */
#ifndef MESSAGEVIEW_H
#define MESSAGEVIEW_H

#include <QStackedWidget>

class SyscoinGUI;
class ClientModel;
class WalletModel;
class InMessageListPage;
class OutMessageListPage;
class PlatformStyle;

QT_BEGIN_NAMESPACE
class QObject;
class QWidget;
class QLabel;
class QModelIndex;
class QTabWidget;
class QStackedWidget;
class QAction;
QT_END_NAMESPACE

/*
  MessageView class. This class represents the view to the syscoin messagees
  
*/
class MessageView: public QObject
 {
     Q_OBJECT

public:
    explicit MessageView(const PlatformStyle *platformStyle, QStackedWidget *parent);
    ~MessageView();

    void setSyscoinGUI(SyscoinGUI *gui);
    /** Set the client model.
        The client model represents the part of the core that communicates with the P2P network, and is wallet-agnostic.
    */
    void setClientModel(ClientModel *clientModel);
    /** Set the wallet model.
        The wallet model represents a syscoin wallet, and offers access to the list of transactions, address book and sending
        functionality.
    */
    void setWalletModel(WalletModel *walletModel);

private:
    SyscoinGUI *gui;
    ClientModel *clientModel;
    WalletModel *walletModel;

    QTabWidget *tabWidget;
    InMessageListPage *inMessageListPage;
	OutMessageListPage *outMessageListPage;

public:
    /** Switch to offer page */
    void gotoMessageListPage();

Q_SIGNALS:
    /** Signal that we want to show the main window */
    void showNormalIfMinimized();
};

#endif // MESSAGEVIEW_H
