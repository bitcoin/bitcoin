#ifndef OFFERVIEW_H
#define OFFERVIEW_H

#include <QStackedWidget>

class SyscoinGUI;
class ClientModel;
class WalletModel;
class MyOfferListPage;
class MyAcceptedOfferListPage;
class AcceptedOfferListPage;
class OfferListPage;
class AcceptandPayOfferListPage;
class SendCoinsRecipient;
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
  OfferView class. This class represents the view to the syscoin offer marketplace
  
*/
class OfferView: public QObject
 {
     Q_OBJECT

public:
    explicit OfferView(const PlatformStyle *platformStyle, QStackedWidget *parent);
    ~OfferView();

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
	
    bool handlePaymentRequest(const SendCoinsRecipient* recipient);


private:
    SyscoinGUI *gui;
    ClientModel *clientModel;
    WalletModel *walletModel;

	QTabWidget *tabWidget;
	AcceptandPayOfferListPage *acceptandPayOfferListPage;
    MyOfferListPage *myOfferListPage;
	MyAcceptedOfferListPage *myAcceptedOfferListPage;
	AcceptedOfferListPage *acceptedOfferListPage;
    OfferListPage *offerListPage;	

public:
    /** Switch to offer page */
    void gotoOfferListPage();

Q_SIGNALS:
    /** Signal that we want to show the main window */
    void showNormalIfMinimized();
};

#endif // OFFERVIEW_H
