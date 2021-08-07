// Copyright (c) 2011-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/walletframe.h>

#include <node/ui_interface.h>
#include <psbt.h>
#include <qt/guiutil.h>
#include <qt/overviewpage.h>
#include <qt/psbtoperationsdialog.h>
#include <qt/walletmodel.h>
#include <qt/walletview.h>

#include <cassert>

#include <QApplication>
#include <QClipboard>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

WalletFrame::WalletFrame(const PlatformStyle* _platformStyle, QWidget* parent)
    : QFrame(parent),
      platformStyle(_platformStyle),
      m_size_hint(OverviewPage{platformStyle, nullptr}.sizeHint())
{
    // Leave HBox hook for adding a list view later
    QHBoxLayout *walletFrameLayout = new QHBoxLayout(this);
    setContentsMargins(0,0,0,0);
    walletStack = new QStackedWidget(this);
    walletFrameLayout->setContentsMargins(0,0,0,0);
    walletFrameLayout->addWidget(walletStack);

    // hbox for no wallet
    QGroupBox* no_wallet_group = new QGroupBox(walletStack);
    QVBoxLayout* no_wallet_layout = new QVBoxLayout(no_wallet_group);

    QLabel *noWallet = new QLabel(tr("No wallet has been loaded.\nGo to File > Open Wallet to load a wallet.\n- OR -"));
    noWallet->setAlignment(Qt::AlignCenter);
    no_wallet_layout->addWidget(noWallet, 0, Qt::AlignHCenter | Qt::AlignBottom);

    // A button for create wallet dialog
    QPushButton* create_wallet_button = new QPushButton(tr("Create a new wallet"), walletStack);
    connect(create_wallet_button, &QPushButton::clicked, this, &WalletFrame::createWalletButtonClicked);
    no_wallet_layout->addWidget(create_wallet_button, 0, Qt::AlignHCenter | Qt::AlignTop);
    no_wallet_group->setLayout(no_wallet_layout);

    walletStack->addWidget(no_wallet_group);
}

WalletFrame::~WalletFrame()
{
}

void WalletFrame::setClientModel(ClientModel *_clientModel)
{
    this->clientModel = _clientModel;

    for (auto i = mapWalletViews.constBegin(); i != mapWalletViews.constEnd(); ++i) {
        i.value()->setClientModel(_clientModel);
    }
}

bool WalletFrame::addWallet(WalletModel* walletModel, WalletView* walletView)
{
    if (!clientModel || !walletModel) return false;

    if (mapWalletViews.count(walletModel) > 0) return false;

    walletView->setClientModel(clientModel);
    walletView->showOutOfSyncWarning(bOutOfSync);

    WalletView* current_wallet_view = currentWalletView();
    if (current_wallet_view) {
        walletView->setCurrentIndex(current_wallet_view->currentIndex());
    } else {
        walletView->gotoOverviewPage();
    }

    walletStack->addWidget(walletView);
    mapWalletViews[walletModel] = walletView;

    return true;
}

void WalletFrame::setCurrentWallet(WalletModel* wallet_model)
{
    if (mapWalletViews.count(wallet_model) == 0) return;

    // Stop the effect of hidden widgets on the size hint of the shown one in QStackedWidget.
    WalletView* view_about_to_hide = currentWalletView();
    if (view_about_to_hide) {
        QSizePolicy sp = view_about_to_hide->sizePolicy();
        sp.setHorizontalPolicy(QSizePolicy::Ignored);
        view_about_to_hide->setSizePolicy(sp);
    }

    WalletView *walletView = mapWalletViews.value(wallet_model);
    assert(walletView);

    // Set or restore the default QSizePolicy which could be set to QSizePolicy::Ignored previously.
    QSizePolicy sp = walletView->sizePolicy();
    sp.setHorizontalPolicy(QSizePolicy::Preferred);
    walletView->setSizePolicy(sp);
    walletView->updateGeometry();

    walletStack->setCurrentWidget(walletView);

    Q_EMIT currentWalletSet();
}

void WalletFrame::removeWallet(WalletModel* wallet_model)
{
    if (mapWalletViews.count(wallet_model) == 0) return;

    WalletView *walletView = mapWalletViews.take(wallet_model);
    walletStack->removeWidget(walletView);
    delete walletView;
}

void WalletFrame::removeAllWallets()
{
    QMap<WalletModel*, WalletView*>::const_iterator i;
    for (i = mapWalletViews.constBegin(); i != mapWalletViews.constEnd(); ++i)
        walletStack->removeWidget(i.value());
    mapWalletViews.clear();
}

bool WalletFrame::handlePaymentRequest(const SendCoinsRecipient &recipient)
{
    WalletView *walletView = currentWalletView();
    if (!walletView)
        return false;

    return walletView->handlePaymentRequest(recipient);
}

void WalletFrame::showOutOfSyncWarning(bool fShow)
{
    bOutOfSync = fShow;
    QMap<WalletModel*, WalletView*>::const_iterator i;
    for (i = mapWalletViews.constBegin(); i != mapWalletViews.constEnd(); ++i)
        i.value()->showOutOfSyncWarning(fShow);
}

void WalletFrame::gotoOverviewPage()
{
    QMap<WalletModel*, WalletView*>::const_iterator i;
    for (i = mapWalletViews.constBegin(); i != mapWalletViews.constEnd(); ++i)
        i.value()->gotoOverviewPage();
}

void WalletFrame::gotoHistoryPage()
{
    QMap<WalletModel*, WalletView*>::const_iterator i;
    for (i = mapWalletViews.constBegin(); i != mapWalletViews.constEnd(); ++i)
        i.value()->gotoHistoryPage();
}

void WalletFrame::gotoReceiveCoinsPage()
{
    QMap<WalletModel*, WalletView*>::const_iterator i;
    for (i = mapWalletViews.constBegin(); i != mapWalletViews.constEnd(); ++i)
        i.value()->gotoReceiveCoinsPage();
}

void WalletFrame::gotoSendCoinsPage(QString addr)
{
    QMap<WalletModel*, WalletView*>::const_iterator i;
    for (i = mapWalletViews.constBegin(); i != mapWalletViews.constEnd(); ++i)
        i.value()->gotoSendCoinsPage(addr);
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

void WalletFrame::gotoLoadPSBT(bool from_clipboard)
{
    std::string data;

    if (from_clipboard) {
        std::string raw = QApplication::clipboard()->text().toStdString();
        bool invalid;
        data = DecodeBase64(raw, &invalid);
        if (invalid) {
            Q_EMIT message(tr("Error"), tr("Unable to decode PSBT from clipboard (invalid base64)"), CClientUIInterface::MSG_ERROR);
            return;
        }
    } else {
        QString filename = GUIUtil::getOpenFileName(this,
            tr("Load Transaction Data"), QString(),
            tr("Partially Signed Transaction (*.psbt)"), nullptr);
        if (filename.isEmpty()) return;
        if (GetFileSize(filename.toLocal8Bit().data(), MAX_FILE_SIZE_PSBT) == MAX_FILE_SIZE_PSBT) {
            Q_EMIT message(tr("Error"), tr("PSBT file must be smaller than 100 MiB"), CClientUIInterface::MSG_ERROR);
            return;
        }
        std::ifstream in(filename.toLocal8Bit().data(), std::ios::binary);
        data = std::string(std::istreambuf_iterator<char>{in}, {});
    }

    std::string error;
    PartiallySignedTransaction psbtx;
    if (!DecodeRawPSBT(psbtx, data, error)) {
        Q_EMIT message(tr("Error"), tr("Unable to decode PSBT") + "\n" + QString::fromStdString(error), CClientUIInterface::MSG_ERROR);
        return;
    }

    PSBTOperationsDialog* dlg = new PSBTOperationsDialog(this, currentWalletModel(), clientModel);
    dlg->openWithPSBT(psbtx);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->exec();
}

void WalletFrame::encryptWallet()
{
    WalletView *walletView = currentWalletView();
    if (walletView)
        walletView->encryptWallet();
}

void WalletFrame::backupWallet()
{
    WalletView *walletView = currentWalletView();
    if (walletView)
        walletView->backupWallet();
}

void WalletFrame::changePassphrase()
{
    WalletView *walletView = currentWalletView();
    if (walletView)
        walletView->changePassphrase();
}

void WalletFrame::unlockWallet()
{
    WalletView *walletView = currentWalletView();
    if (walletView)
        walletView->unlockWallet();
}

void WalletFrame::usedSendingAddresses()
{
    WalletView *walletView = currentWalletView();
    if (walletView)
        walletView->usedSendingAddresses();
}

void WalletFrame::usedReceivingAddresses()
{
    WalletView *walletView = currentWalletView();
    if (walletView)
        walletView->usedReceivingAddresses();
}

WalletView* WalletFrame::currentWalletView() const
{
    return qobject_cast<WalletView*>(walletStack->currentWidget());
}

WalletModel* WalletFrame::currentWalletModel() const
{
    WalletView* wallet_view = currentWalletView();
    return wallet_view ? wallet_view->getWalletModel() : nullptr;
}
