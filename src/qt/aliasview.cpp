/*
 * Syscoin Developers 2016
 */
#include "aliasview.h"
#include "syscoingui.h"

#include "platformstyle.h"
#include "guiutil.h"
#include "clientmodel.h"
#include "walletmodel.h"
#include "optionsmodel.h"
#include "myaliaslistpage.h"
#include "aliaslistpage.h"
#include "aliastablemodel.h"
#include "ui_interface.h"

#include <QAction>
#if QT_VERSION < 0x050000
#include <QDesktopServices>
#else
#include <QStandardPaths>
#endif
#include <QPushButton>

AliasView::AliasView(const PlatformStyle *platformStyle, QStackedWidget *parent):
    clientModel(0),
    walletModel(0)
{
	tabWidget = new QTabWidget();
    aliasListPage = new AliasListPage(platformStyle);
    myAliasListPage = new MyAliasListPage(platformStyle);
	QString theme = GUIUtil::getThemeName();
	tabWidget->addTab(myAliasListPage, tr("&My Aliases"));
	tabWidget->addTab(aliasListPage, tr("&Search"));
	tabWidget->setTabIcon(0, platformStyle->SingleColorIcon(":/icons/" + theme + "/alias"));
	tabWidget->setTabIcon(1, platformStyle->SingleColorIcon(":/icons/" + theme + "/search"));
	parent->addWidget(tabWidget);

}

AliasView::~AliasView()
{
}

void AliasView::setSyscoinGUI(SyscoinGUI *gui)
{
    this->gui = gui;
}

void AliasView::setClientModel(ClientModel *clientModel)
{
    this->clientModel = clientModel;
    if (clientModel)
    {
       
        aliasListPage->setOptionsModel(clientModel->getOptionsModel());
		myAliasListPage->setOptionsModel(clientModel,clientModel->getOptionsModel());

    }
}

void AliasView::setWalletModel(WalletModel *walletModel)
{

    this->walletModel = walletModel;
    if (walletModel)
    {

        aliasListPage->setModel(walletModel, walletModel->getAliasTableModelAll());
		myAliasListPage->setModel(walletModel, walletModel->getAliasTableModelMine());

    }
}


void AliasView::gotoAliasListPage()
{
	tabWidget->setCurrentWidget(aliasListPage);
}
