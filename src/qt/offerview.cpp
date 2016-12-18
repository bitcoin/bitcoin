/*
 * Syscoin Developers 2015
 */
#include "offerview.h"
#include "syscoingui.h"
#include "guiutil.h"
#include "platformstyle.h"
#include "clientmodel.h"
#include "walletmodel.h"
#include "optionsmodel.h"
#include "myofferlistpage.h"
#include "myacceptedofferlistpage.h"
#include "acceptedofferlistpage.h"
#include "acceptandpayofferlistpage.h"
#include "offerlistpage.h"
#include "offertablemodel.h"
#include "offeraccepttablemodel.h"
#include "ui_interface.h"

#include <QAction>
#if QT_VERSION < 0x050000
#include <QDesktopServices>
#else
#include <QStandardPaths>
#endif
#include <QPushButton>

OfferView::OfferView(const PlatformStyle *platformStyle, QStackedWidget *parent):
    clientModel(0),
    walletModel(0)
{

	tabWidget = new QTabWidget();
    offerListPage = new OfferListPage(platformStyle, this);
    myOfferListPage = new MyOfferListPage(platformStyle);
	acceptandPayOfferListPage = new AcceptandPayOfferListPage(platformStyle);
	myAcceptedOfferListPage = new MyAcceptedOfferListPage(platformStyle);
	acceptedOfferListPage = new AcceptedOfferListPage(platformStyle);
	QString theme = GUIUtil::getThemeName();
	tabWidget->addTab(myOfferListPage, tr("Selling"));
	tabWidget->addTab(myAcceptedOfferListPage, tr("Sold"));
	tabWidget->addTab(acceptedOfferListPage, tr("My Purchases"));
	tabWidget->addTab(offerListPage, tr("Search"));
	tabWidget->addTab(acceptandPayOfferListPage, tr("Buy"));
	tabWidget->setTabIcon(0, QIcon(":/icons/" + theme + "/cart"));
	tabWidget->setTabIcon(1, QIcon(":/icons/" + theme + "/cart"));
	tabWidget->setTabIcon(2, QIcon(":/icons/" + theme + "/cart"));
	tabWidget->setTabIcon(3, QIcon(":/icons/" + theme + "/search"));
	tabWidget->setTabIcon(4, QIcon(":/icons/" + theme + "/send"));
	parent->addWidget(tabWidget);
}

OfferView::~OfferView()
{

}

void OfferView::setSyscoinGUI(SyscoinGUI *gui)
{
    this->gui = gui;
}
void OfferView::setClientModel(ClientModel *clientModel)
{
    this->clientModel = clientModel;
    if (clientModel)
    {
       
        offerListPage->setOptionsModel(clientModel->getOptionsModel());
		myOfferListPage->setOptionsModel(clientModel,clientModel->getOptionsModel());
		myAcceptedOfferListPage->setOptionsModel(clientModel,clientModel->getOptionsModel());
		acceptedOfferListPage->setOptionsModel(clientModel,clientModel->getOptionsModel());
    }
}

void OfferView::setWalletModel(WalletModel *walletModel)
{

    this->walletModel = walletModel;
    if (walletModel)
    {

        offerListPage->setModel(walletModel, walletModel->getOfferTableModelAll());
		myOfferListPage->setModel(walletModel, walletModel->getOfferTableModelMine());
		myAcceptedOfferListPage->setModel(walletModel, walletModel->getOfferTableModelMyAccept());
		acceptedOfferListPage->setModel(walletModel, walletModel->getOfferTableModelAccept());
		acceptandPayOfferListPage->setModel(walletModel);

    }
}


void OfferView::gotoOfferListPage()
{
	tabWidget->setCurrentWidget(offerListPage);
}


bool OfferView::handlePaymentRequest(const SendCoinsRecipient* recipient)
{
    if (acceptandPayOfferListPage->handlePaymentRequest(recipient))
    {
        tabWidget->setCurrentWidget(acceptandPayOfferListPage);
        Q_EMIT showNormalIfMinimized();
        return true;
    }
    return false;
}

