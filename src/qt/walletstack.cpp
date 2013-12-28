/*
 * Qt4 bitcoin GUI.
 *
 * W.J. van der Laan 2011-2012
 * The Bitcoin Developers 2011-2013
 */
#include "walletstack.h"
#include "walletview.h"
#include "bitcoingui.h"

#include <QMap>
#include <QMessageBox>

WalletStack::WalletStack(QWidget *parent) :
    QStackedWidget(parent),
    gui(0),
    clientModel(0),
    bOutOfSync(true)
{
    setContentsMargins(0,0,0,0);
}

WalletStack::~WalletStack()
{
}

bool WalletStack::addWallet(const QString& name, WalletModel *walletModel)
{
    if (!gui || !clientModel || mapWalletViews.count(name) > 0)
        return false;

    WalletView *walletView = new WalletView(this, gui);
    walletView->setBitcoinGUI(gui);
    walletView->setClientModel(clientModel);
    walletView->setWalletModel(walletModel);
    walletView->showOutOfSyncWarning(bOutOfSync);
    addWidget(walletView);
    mapWalletViews[name] = walletView;

    // Ensure a walletView is able to show the main window
	connect(walletView, SIGNAL(showNormalIfMinimized()), gui, SLOT(showNormalIfMinimized()));

    return true;
}

bool WalletStack::removeWallet(const QString& name)
{
    if (mapWalletViews.count(name) == 0) return false;
    WalletView *walletView = mapWalletViews.take(name);
    removeWidget(walletView);
    return true;
}

void WalletStack::removeAllWallets()
{
    QMap<QString, WalletView*>::const_iterator i;
    for (i = mapWalletViews.constBegin(); i != mapWalletViews.constEnd(); ++i)
        removeWidget(i.value());
    mapWalletViews.clear();
}

bool WalletStack::handleURI(const QString &uri)
{
    WalletView *walletView = (WalletView*)currentWidget();
    if (!walletView) return false;

    return walletView->handleURI(uri);
}

void WalletStack::showOutOfSyncWarning(bool fShow)
{
    bOutOfSync = fShow;
    QMap<QString, WalletView*>::const_iterator i;
    for (i = mapWalletViews.constBegin(); i != mapWalletViews.constEnd(); ++i)
        i.value()->showOutOfSyncWarning(fShow);
}

void WalletStack::gotoOverviewPage()
{
    QMap<QString, WalletView*>::const_iterator i;
    for (i = mapWalletViews.constBegin(); i != mapWalletViews.constEnd(); ++i)
        i.value()->gotoOverviewPage();
}

void WalletStack::gotoHistoryPage()
{
    QMap<QString, WalletView*>::const_iterator i;
    for (i = mapWalletViews.constBegin(); i != mapWalletViews.constEnd(); ++i)
        i.value()->gotoHistoryPage();
}

void WalletStack::gotoAddressBookPage()
{
    QMap<QString, WalletView*>::const_iterator i;
    for (i = mapWalletViews.constBegin(); i != mapWalletViews.constEnd(); ++i)
        i.value()->gotoAddressBookPage();
}

void WalletStack::gotoReceiveCoinsPage()
{
    QMap<QString, WalletView*>::const_iterator i;
    for (i = mapWalletViews.constBegin(); i != mapWalletViews.constEnd(); ++i)
        i.value()->gotoReceiveCoinsPage();
}

void WalletStack::gotoSendCoinsPage(QString addr)
{
    QMap<QString, WalletView*>::const_iterator i;
    for (i = mapWalletViews.constBegin(); i != mapWalletViews.constEnd(); ++i)
        i.value()->gotoSendCoinsPage(addr);
}

void WalletStack::gotoSignMessageTab(QString addr)
{
    WalletView *walletView = (WalletView*)currentWidget();
    if (walletView) walletView->gotoSignMessageTab(addr);
}

void WalletStack::gotoVerifyMessageTab(QString addr)
{
    WalletView *walletView = (WalletView*)currentWidget();
    if (walletView) walletView->gotoVerifyMessageTab(addr);
}

void WalletStack::encryptWallet(bool status)
{
    WalletView *walletView = (WalletView*)currentWidget();
    if (walletView) walletView->encryptWallet(status);
}

void WalletStack::backupWallet()
{
    WalletView *walletView = (WalletView*)currentWidget();
    if (walletView) walletView->backupWallet();
}

void WalletStack::changePassphrase()
{
    WalletView *walletView = (WalletView*)currentWidget();
    if (walletView) walletView->changePassphrase();
}

void WalletStack::unlockWallet()
{
    WalletView *walletView = (WalletView*)currentWidget();
    if (walletView) walletView->unlockWallet();
}

void WalletStack::setEncryptionStatus()
{
    WalletView *walletView = (WalletView*)currentWidget();
    if (walletView) walletView->setEncryptionStatus();
}

void WalletStack::setCurrentWallet(const QString& name)
{
    if (mapWalletViews.count(name) == 0) return;
    WalletView *walletView = mapWalletViews.value(name);
    setCurrentWidget(walletView);
    walletView->setEncryptionStatus();
}
