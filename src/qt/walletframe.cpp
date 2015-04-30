// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "walletframe.h"

#include "bitcoingui.h"
#include "walletview.h"

#include <cstdio>

#include <QHBoxLayout>
#include <QLabel>

WalletFrame::WalletFrame(BitcreditGUI *_gui) :
    QFrame(_gui),
    gui(_gui)
{
    // Leave HBox hook for adding a list view later
    QHBoxLayout *walletFrameLayout = new QHBoxLayout(this);
    setContentsMargins(0,0,0,0);
    walletStack = new QStackedWidget(this);
    walletFrameLayout->setContentsMargins(0,0,0,0);
    walletFrameLayout->addWidget(walletStack);

    QLabel *noWallet = new QLabel(tr("No wallet has been loaded."));
    noWallet->setAlignment(Qt::AlignCenter);
    walletStack->addWidget(noWallet);
}

WalletFrame::~WalletFrame()
{
}

void WalletFrame::setClientModel(ClientModel *clientModel)
{
    this->clientModel = clientModel;
}

bool WalletFrame::addWallet(const QString& name, Bitcredit_WalletModel *bitcredit_model, Bitcoin_WalletModel *bitcoin_model, Bitcredit_WalletModel *deposit_model)
{
    if (!gui || !clientModel || !bitcredit_model || !bitcoin_model || !deposit_model || mapWalletViews.count(name) > 0)
        return false;

    WalletView *walletView = new WalletView(this);
    walletView->setBitcreditGUI(gui);
    walletView->setClientModel(clientModel);
    walletView->setWalletModel(bitcredit_model, bitcoin_model, deposit_model);
    walletView->credits_showOutOfSyncWarning(credits_bOutOfSync);
    walletView->bitcoin_showOutOfSyncWarning(bitcoin_bOutOfSync);

     /* TODO we should goto the currently selected page once dynamically adding wallets is supported */
    walletView->credits_gotoOverviewPage();
    walletStack->addWidget(walletView);
    mapWalletViews[name] = walletView;

    // Ensure a walletView is able to show the main window
    connect(walletView, SIGNAL(showNormalIfMinimized()), gui, SLOT(showNormalIfMinimized()));

    return true;
}

bool WalletFrame::setCurrentWallet(const QString& name)
{
    if (mapWalletViews.count(name) == 0)
        return false;

    WalletView *walletView = mapWalletViews.value(name);
    walletStack->setCurrentWidget(walletView);
    walletView->updateEncryptionStatus();
    return true;
}

bool WalletFrame::removeWallet(const QString &name)
{
    if (mapWalletViews.count(name) == 0)
        return false;

    WalletView *walletView = mapWalletViews.take(name);
    walletStack->removeWidget(walletView);
    return true;
}

void WalletFrame::removeAllWallets()
{
    QMap<QString, WalletView*>::const_iterator i;
    for (i = mapWalletViews.constBegin(); i != mapWalletViews.constEnd(); ++i)
        walletStack->removeWidget(i.value());
    mapWalletViews.clear();
}

bool WalletFrame::credits_handlePaymentRequest(const Bitcredit_SendCoinsRecipient &recipient)
{
    WalletView *walletView = currentWalletView();
    if (!walletView)
        return false;

    return walletView->credits_handlePaymentRequest(recipient);
}
bool WalletFrame::bitcoin_handlePaymentRequest(const Bitcoin_SendCoinsRecipient &recipient)
{
    WalletView *walletView = currentWalletView();
    if (!walletView)
        return false;

    return walletView->bitcoin_handlePaymentRequest(recipient);
}

void WalletFrame::credits_showOutOfSyncWarning(bool fShow)
{
	credits_bOutOfSync = fShow;
    QMap<QString, WalletView*>::const_iterator i;
    for (i = mapWalletViews.constBegin(); i != mapWalletViews.constEnd(); ++i)
        i.value()->credits_showOutOfSyncWarning(fShow);
}
void WalletFrame::bitcoin_showOutOfSyncWarning(bool fShow)
{
	bitcoin_bOutOfSync = fShow;
    QMap<QString, WalletView*>::const_iterator i;
    for (i = mapWalletViews.constBegin(); i != mapWalletViews.constEnd(); ++i)
        i.value()->bitcoin_showOutOfSyncWarning(fShow);
}

void WalletFrame::credits_gotoOverviewPage()
{
    QMap<QString, WalletView*>::const_iterator i;
    for (i = mapWalletViews.constBegin(); i != mapWalletViews.constEnd(); ++i)
        i.value()->credits_gotoOverviewPage();
}
void WalletFrame::bitcoin_gotoOverviewPage()
{
    QMap<QString, WalletView*>::const_iterator i;
    for (i = mapWalletViews.constBegin(); i != mapWalletViews.constEnd(); ++i)
        i.value()->bitcoin_gotoOverviewPage();
}

void WalletFrame::gotoClaimCoinsPage()
{
    QMap<QString, WalletView*>::const_iterator i;
    for (i = mapWalletViews.constBegin(); i != mapWalletViews.constEnd(); ++i)
        i.value()->gotoClaimCoinsPage();
}

void WalletFrame::credits_gotoHistoryPage()
{
    QMap<QString, WalletView*>::const_iterator i;
    for (i = mapWalletViews.constBegin(); i != mapWalletViews.constEnd(); ++i)
        i.value()->credits_gotoHistoryPage();
}
void WalletFrame::bitcoin_gotoHistoryPage()
{
    QMap<QString, WalletView*>::const_iterator i;
    for (i = mapWalletViews.constBegin(); i != mapWalletViews.constEnd(); ++i)
        i.value()->bitcoin_gotoHistoryPage();
}

void WalletFrame::gotoMinerDepositsPage()
{
    QMap<QString, WalletView*>::const_iterator i;
    for (i = mapWalletViews.constBegin(); i != mapWalletViews.constEnd(); ++i)
        i.value()->gotoMinerDepositsPage();
}

void WalletFrame::credits_gotoReceiveCoinsPage()
{
    QMap<QString, WalletView*>::const_iterator i;
    for (i = mapWalletViews.constBegin(); i != mapWalletViews.constEnd(); ++i)
        i.value()->credits_gotoReceiveCoinsPage();
}
void WalletFrame::bitcoin_gotoReceiveCoinsPage()
{
    QMap<QString, WalletView*>::const_iterator i;
    for (i = mapWalletViews.constBegin(); i != mapWalletViews.constEnd(); ++i)
        i.value()->bitcoin_gotoReceiveCoinsPage();
}

void WalletFrame::credits_gotoSendCoinsPage(QString addr)
{
    QMap<QString, WalletView*>::const_iterator i;
    for (i = mapWalletViews.constBegin(); i != mapWalletViews.constEnd(); ++i)
        i.value()->credits_gotoSendCoinsPage(addr);
}
void WalletFrame::bitcoin_gotoSendCoinsPage(QString addr)
{
    QMap<QString, WalletView*>::const_iterator i;
    for (i = mapWalletViews.constBegin(); i != mapWalletViews.constEnd(); ++i)
        i.value()->bitcoin_gotoSendCoinsPage(addr);
}

void WalletFrame::gotoMinerCoinsPage(QString addr)
{
    QMap<QString, WalletView*>::const_iterator i;
    for (i = mapWalletViews.constBegin(); i != mapWalletViews.constEnd(); ++i)
        i.value()->gotoMinerCoinsPage(addr);
}

void WalletFrame::gotoSignMessageTab(QString addr)
{
    WalletView *walletView = currentWalletView();
    if (walletView)
        walletView->gotoSignMessageTab(addr);
}

void WalletFrame::gotoVerifyMessageTab(QString addr)
{
    WalletView *walletView = currentWalletView();
    if (walletView)
        walletView->gotoVerifyMessageTab(addr);
}

void WalletFrame::bitcredit_encryptWallet(bool status)
{
    WalletView *walletView = currentWalletView();
    if (walletView)
        walletView->bitcredit_encryptWallet(status);
}
void WalletFrame::bitcoin_encryptWallet(bool status)
{
    WalletView *walletView = currentWalletView();
    if (walletView)
        walletView->bitcoin_encryptWallet(status);
}
void WalletFrame::deposit_encryptWallet(bool status)
{
    WalletView *walletView = currentWalletView();
    if (walletView)
        walletView->deposit_encryptWallet(status);
}

void WalletFrame::bitcredit_backupWallet()
{
    WalletView *walletView = currentWalletView();
    if (walletView)
        walletView->bitcredit_backupWallet();
}
void WalletFrame::bitcoin_backupWallet()
{
    WalletView *walletView = currentWalletView();
    if (walletView)
        walletView->bitcoin_backupWallet();
}
void WalletFrame::deposit_backupWallet()
{
    WalletView *walletView = currentWalletView();
    if (walletView)
        walletView->deposit_backupWallet();
}

void WalletFrame::bitcredit_changePassphrase()
{
    WalletView *walletView = currentWalletView();
    if (walletView)
        walletView->bitcredit_changePassphrase();
}
void WalletFrame::bitcoin_changePassphrase()
{
    WalletView *walletView = currentWalletView();
    if (walletView)
        walletView->bitcoin_changePassphrase();
}
void WalletFrame::deposit_changePassphrase()
{
    WalletView *walletView = currentWalletView();
    if (walletView)
        walletView->deposit_changePassphrase();
}

void WalletFrame::bitcredit_unlockWallet()
{
    WalletView *walletView = currentWalletView();
    if (walletView)
        walletView->bitcredit_unlockWallet();
}
void WalletFrame::bitcoin_unlockWallet()
{
    WalletView *walletView = currentWalletView();
    if (walletView)
        walletView->bitcoin_unlockWallet();
}
void WalletFrame::deposit_unlockWallet()
{
    WalletView *walletView = currentWalletView();
    if (walletView)
        walletView->deposit_unlockWallet();
}

void WalletFrame::credits_usedSendingAddresses()
{
    WalletView *walletView = currentWalletView();
    if (walletView)
        walletView->credits_usedSendingAddresses();
}
void WalletFrame::bitcoin_usedSendingAddresses()
{
    WalletView *walletView = currentWalletView();
    if (walletView)
        walletView->bitcoin_usedSendingAddresses();
}

void WalletFrame::credits_usedReceivingAddresses()
{
    WalletView *walletView = currentWalletView();
    if (walletView)
        walletView->credits_usedReceivingAddresses();
}
void WalletFrame::bitcoin_usedReceivingAddresses()
{
    WalletView *walletView = currentWalletView();
    if (walletView)
        walletView->bitcoin_usedReceivingAddresses();
}

void WalletFrame::bitcredit_setNumBlocks(int count) {
    WalletView *walletView = currentWalletView();
    if (walletView)
        walletView->bitcredit_setNumBlocks(count);
}

WalletView *WalletFrame::currentWalletView()
{
    return qobject_cast<WalletView*>(walletStack->currentWidget());
}

