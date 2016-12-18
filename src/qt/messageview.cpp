/*
 * Syscoin Developers 2016
 */
#include "messageview.h"
#include "syscoingui.h"

#include "platformstyle.h"
#include "guiutil.h"
#include "clientmodel.h"
#include "walletmodel.h"
#include "optionsmodel.h"
#include "inmessagelistpage.h"
#include "outmessagelistpage.h"
#include "messagetablemodel.h"
#include "ui_interface.h"

#include <QAction>
#if QT_VERSION < 0x050000
#include <QDesktopServices>
#else
#include <QStandardPaths>
#endif
#include <QPushButton>

MessageView::MessageView(const PlatformStyle *platformStyle, QStackedWidget *parent):
    clientModel(0),
    walletModel(0)
{
	tabWidget = new QTabWidget();
    inMessageListPage = new InMessageListPage(platformStyle);
	outMessageListPage = new OutMessageListPage(platformStyle);
	QString theme = GUIUtil::getThemeName();
	tabWidget->addTab(inMessageListPage, tr("Inbox"));
	tabWidget->setTabIcon(0, platformStyle->SingleColorIcon(":/icons/" + theme + "/inmail"));
	tabWidget->addTab(outMessageListPage, tr("Outbox"));
	tabWidget->setTabIcon(1, platformStyle->SingleColorIcon(":/icons/" + theme + "/outmail"));
	parent->addWidget(tabWidget);

}

MessageView::~MessageView()
{
}

void MessageView::setSyscoinGUI(SyscoinGUI *gui)
{
    this->gui = gui;
}

void MessageView::setClientModel(ClientModel *clientModel)
{
    this->clientModel = clientModel;
    if (clientModel)
    {    
        inMessageListPage->setOptionsModel(clientModel->getOptionsModel());
		outMessageListPage->setOptionsModel(clientModel->getOptionsModel());
    }
}

void MessageView::setWalletModel(WalletModel *walletModel)
{

    this->walletModel = walletModel;
    if (walletModel)
    {
        inMessageListPage->setModel(walletModel, walletModel->getMessageTableModelIn());
		outMessageListPage->setModel(walletModel, walletModel->getMessageTableModelOut());

    }
}


void MessageView::gotoMessageListPage()
{
	tabWidget->setCurrentWidget(inMessageListPage);
}
