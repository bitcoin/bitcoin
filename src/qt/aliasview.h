/*
 * Syscoin Developers 2016
 */
#ifndef ALIASVIEW_H
#define ALIASVIEW_H

#include <QStackedWidget>

class SyscoinGUI;
class ClientModel;
class WalletModel;
class MyAliasListPage;
class AliasListPage;
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
  AliasView class. This class represents the view to the syscoin aliases
  
*/
class AliasView: public QObject
 {
     Q_OBJECT

public:
    explicit AliasView(const PlatformStyle *platformStyle, QStackedWidget *parent);
    ~AliasView();

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
    MyAliasListPage *myAliasListPage;
    AliasListPage *aliasListPage;

public:
    /** Switch to offer page */
    void gotoAliasListPage();

Q_SIGNALS:
    /** Signal that we want to show the main window */
    void showNormalIfMinimized();
};

#endif // ALIASVIEW_H
