// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "walletview.h"

#include "addressbookpage.h"
#include "askpassphrasedialog.h"
#include "bitcoin_askpassphrasedialog.h"
#include "bitcoingui.h"
#include "claimcoinsdialog.h"
#include "clientmodel.h"
#include "guiutil.h"
#include "optionsmodel.h"
#include "minercoinsdialog.h"
#include "minerdepositsview.h"
#include "overviewpage.h"
#include "receivecoinsdialog.h"
#include "sendcoinsdialog.h"
#include "signverifymessagedialog.h"
#include "transactiontablemodel.h"
#include "transactionview.h"
#include "walletmodel.h"
#include "bitcoin_walletmodel.h"
#include "main.h"

#include "ui_interface.h"

#include <QAction>
#include <QActionGroup>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QProgressDialog>
#include <QPushButton>
#include <QVBoxLayout>

WalletView::WalletView(QWidget *parent):
    QStackedWidget(parent),
    clientModel(0),
    bitcredit_model(0),
    bitcoin_model(0),
    deposit_model(0)
{
    // Create tabs
    overviewPage = new OverviewPage();

    transactionsPage = new QWidget(this);
    transactionView = new TransactionView(this);
    QPushButton * transactionsButton = CreateButton(transactionView, transactionsPage);

    minerDepositsPage = new QWidget(this);
    minerDepositsView = new MinerDepositsView(this);
    CreateLowerLayout(minerDepositsView, minerDepositsPage);

    receiveCoinsPage = new ReceiveCoinsDialog();
    sendCoinsPage = new SendCoinsDialog();
    claimCoinsPage = new ClaimCoinsDialog();
    minerCoinsPage = new MinerCoinsDialog();

    addWidget(overviewPage);
    addWidget(transactionsPage);
    addWidget(minerDepositsPage);
    addWidget(receiveCoinsPage);
    addWidget(sendCoinsPage);
    addWidget(claimCoinsPage);
    addWidget(minerCoinsPage);

    // Clicking on a transaction on the overview pre-selects the transaction on the transaction history page
    connect(overviewPage, SIGNAL(transactionClicked(QModelIndex)), transactionView, SLOT(focusTransaction(QModelIndex)));

    // Double-clicking on a transaction on the transaction history page shows details
    connect(transactionView, SIGNAL(doubleClicked(QModelIndex)), transactionView, SLOT(showDetails()));
    // Clicking on "Export" allows to export the transaction list
    connect(transactionsButton, SIGNAL(clicked()), transactionView, SLOT(exportClicked()));

    // Double-clicking on a transaction on the transaction history page shows details
    connect(minerDepositsView, SIGNAL(doubleClicked(QModelIndex)), minerDepositsView, SLOT(showDetails()));

    // Pass through messages from sendCoinsPage
    connect(sendCoinsPage, SIGNAL(message(QString,QString,unsigned int)), this, SIGNAL(message(QString,QString,unsigned int)));
    // Pass through messages from claimCoinsPage
    connect(claimCoinsPage, SIGNAL(message(QString,QString,unsigned int)), this, SIGNAL(message(QString,QString,unsigned int)));
    // Pass through messages from minerCoinsPage
    connect(minerCoinsPage, SIGNAL(message(QString,QString,unsigned int)), this, SIGNAL(message(QString,QString,unsigned int)));
    // Pass through messages from transactionView
    connect(transactionView, SIGNAL(message(QString,QString,unsigned int)), this, SIGNAL(message(QString,QString,unsigned int)));
    // Pass through messages from minerDepositsView
    connect(minerDepositsView, SIGNAL(message(QString,QString,unsigned int)), this, SIGNAL(message(QString,QString,unsigned int)));
}

WalletView::~WalletView()
{
}

QPushButton * WalletView::CreateButton(QWidget * view, QWidget * page) {
    QVBoxLayout *vbox = new QVBoxLayout();
    QHBoxLayout *hbox_buttons = new QHBoxLayout();
    vbox->addWidget(view);
    QPushButton *exportButton = new QPushButton(tr("&Export"), this);
    exportButton->setToolTip(tr("Export the data in the current tab to a file"));
#ifndef Q_OS_MAC // Icons on push buttons are very uncommon on Mac
    exportButton->setIcon(QIcon(":/icons/export"));
#endif
    hbox_buttons->addStretch();
    hbox_buttons->addWidget(exportButton);
    vbox->addLayout(hbox_buttons);
    page->setLayout(vbox);
    return exportButton;
}
void WalletView::CreateLowerLayout(QWidget * view, QWidget * page) {
    QVBoxLayout *vbox = new QVBoxLayout();
    QHBoxLayout *hbox_buttons = new QHBoxLayout();
    vbox->addWidget(view);
    hbox_buttons->addStretch();
    vbox->addLayout(hbox_buttons);
    page->setLayout(vbox);
}

void WalletView::setBitcreditGUI(BitcreditGUI *gui)
{
    if (gui)
    {
        // Clicking on a transaction on the overview page simply sends you to transaction history page
        connect(overviewPage, SIGNAL(transactionClicked(QModelIndex)), gui, SLOT(gotoHistoryPage()));

        // Receive and report messages
        connect(this, SIGNAL(message(QString,QString,unsigned int)), gui, SLOT(message(QString,QString,unsigned int)));

        // Pass through encryption status changed signals
        connect(this, SIGNAL(bitcredit_encryptionStatusChanged(int)), gui, SLOT(bitcredit_setEncryptionStatus(int)));
        connect(this, SIGNAL(bitcoin_encryptionStatusChanged(int)), gui, SLOT(bitcoin_setEncryptionStatus(int)));
        connect(this, SIGNAL(deposit_encryptionStatusChanged(int)), gui, SLOT(deposit_setEncryptionStatus(int)));

        // Pass through transaction notifications
        connect(this, SIGNAL(incomingTransaction(QString,int,qint64,QString,QString)), gui, SLOT(incomingTransaction(QString,int,qint64,QString,QString)));
    }
}

void WalletView::setClientModel(ClientModel *clientModel)
{
    this->clientModel = clientModel;

    overviewPage->setClientModel(clientModel);
}

void WalletView::setWalletModel(Bitcredit_WalletModel *bitcredit_model, Bitcoin_WalletModel *bitcoin_model, Bitcredit_WalletModel *deposit_model)
{
    this->bitcredit_model = bitcredit_model;
    this->bitcoin_model = bitcoin_model;
    this->deposit_model = deposit_model;

    // Put transaction list in tabs
    transactionView->setModel(bitcredit_model);
    overviewPage->setWalletModel(bitcredit_model, deposit_model);
    receiveCoinsPage->setModel(bitcredit_model);
    sendCoinsPage->setModel(bitcredit_model ,deposit_model);
    claimCoinsPage->setModel(bitcredit_model, bitcoin_model);
    minerCoinsPage->setModel(bitcredit_model, deposit_model);
    minerDepositsView->setModel(bitcredit_model, deposit_model);

    if (bitcredit_model)
    {
        // Receive and pass through messages from wallet model
        connect(bitcredit_model, SIGNAL(message(QString,QString,unsigned int)), this, SIGNAL(message(QString,QString,unsigned int)));

        // Handle changes in encryption status
        connect(bitcredit_model, SIGNAL(encryptionStatusChanged(int)), this, SIGNAL(bitcredit_encryptionStatusChanged(int)));
        updateEncryptionStatus();

        // Balloon pop-up for new transaction
        connect(bitcredit_model->getTransactionTableModel(), SIGNAL(rowsInserted(QModelIndex,int,int)),
                this, SLOT(processNewTransaction(QModelIndex,int,int)));

        // Ask for passphrase if needed
        connect(bitcredit_model, SIGNAL(requireUnlock()), this, SLOT(bitcredit_unlockWallet()));

        // Show progress dialog
        connect(bitcredit_model, SIGNAL(showProgress(QString,int)), this, SLOT(showProgress(QString,int)));
    }

    if (bitcoin_model)
    {
        // Receive and pass through messages from wallet model
        connect(bitcoin_model, SIGNAL(message(QString,QString,unsigned int)), this, SIGNAL(message(QString,QString,unsigned int)));

        // Handle changes in encryption status
        connect(bitcoin_model, SIGNAL(encryptionStatusChanged(int)), this, SIGNAL(bitcoin_encryptionStatusChanged(int)));
        updateEncryptionStatus();

        // Balloon pop-up for new transaction
        //connect(bitcoin_model->getTransactionTableModel(), SIGNAL(rowsInserted(QModelIndex,int,int)),
        //        this, SLOT(processNewTransaction(QModelIndex,int,int)));

        // Ask for passphrase if needed
        connect(bitcoin_model, SIGNAL(requireUnlock()), this, SLOT(bitcoin_unlockWallet()));

        // Show progress dialog
        connect(bitcoin_model, SIGNAL(showProgress(QString,int)), this, SLOT(showProgress(QString,int)));
    }

    if (deposit_model)
    {
        // Receive and pass through messages from wallet model
        connect(deposit_model, SIGNAL(message(QString,QString,unsigned int)), this, SIGNAL(message(QString,QString,unsigned int)));

        // Handle changes in encryption status
        connect(deposit_model, SIGNAL(encryptionStatusChanged(int)), this, SIGNAL(deposit_encryptionStatusChanged(int)));
        updateEncryptionStatus();

        // Balloon pop-up for new transaction
        //connect(deposit_model->getTransactionTableModel(), SIGNAL(rowsInserted(QModelIndex,int,int)),
        //        this, SLOT(processNewTransaction(QModelIndex,int,int)));

        // Ask for passphrase if needed
        connect(deposit_model, SIGNAL(requireUnlock()), this, SLOT(deposit_unlockWallet()));

        // Show progress dialog
        connect(deposit_model, SIGNAL(showProgress(QString,int)), this, SLOT(showProgress(QString,int)));
    }
}

void WalletView::processNewTransaction(const QModelIndex& parent, int start, int /*end*/)
{
    // Prevent balloon-spam when initial block download is in progress
    if (!bitcredit_model || !clientModel || Bitcredit_IsInitialBlockDownload() ||  Bitcoin_IsInitialBlockDownload())
        return;

    Bitcredit_TransactionTableModel *ttm = bitcredit_model->getTransactionTableModel();

    QString date = ttm->index(start, Bitcredit_TransactionTableModel::Date, parent).data().toString();
    qint64 amount = ttm->index(start, Bitcredit_TransactionTableModel::Amount, parent).data(Qt::EditRole).toULongLong();
    QString type = ttm->index(start, Bitcredit_TransactionTableModel::Type, parent).data().toString();
    QString address = ttm->index(start, Bitcredit_TransactionTableModel::ToAddress, parent).data().toString();

    emit incomingTransaction(date, bitcredit_model->getOptionsModel()->getDisplayUnit(), amount, type, address);
}

void WalletView::gotoOverviewPage()
{
    setCurrentWidget(overviewPage);
}

void WalletView::gotoClaimCoinsPage()
{
    setCurrentWidget(claimCoinsPage);
}

void WalletView::gotoHistoryPage()
{
    setCurrentWidget(transactionsPage);
}
void WalletView::gotoMinerDepositsPage()
{
    setCurrentWidget(minerDepositsPage);
}

void WalletView::gotoReceiveCoinsPage()
{
    setCurrentWidget(receiveCoinsPage);
}

void WalletView::gotoSendCoinsPage(QString addr)
{
    setCurrentWidget(sendCoinsPage);

    if (!addr.isEmpty())
        sendCoinsPage->setAddress(addr);
}

void WalletView::gotoMinerCoinsPage(QString addr)
{
    setCurrentWidget(minerCoinsPage);
}

void WalletView::gotoSignMessageTab(QString addr)
{
    // calls show() in showTab_SM()
    SignVerifyMessageDialog *signVerifyMessageDialog = new SignVerifyMessageDialog(this);
    signVerifyMessageDialog->setAttribute(Qt::WA_DeleteOnClose);
    signVerifyMessageDialog->setModel(bitcredit_model);
    signVerifyMessageDialog->showTab_SM(true);

    if (!addr.isEmpty())
        signVerifyMessageDialog->setAddress_SM(addr);
}

void WalletView::gotoVerifyMessageTab(QString addr)
{
    // calls show() in showTab_VM()
    SignVerifyMessageDialog *signVerifyMessageDialog = new SignVerifyMessageDialog(this);
    signVerifyMessageDialog->setAttribute(Qt::WA_DeleteOnClose);
    signVerifyMessageDialog->setModel(bitcredit_model);
    signVerifyMessageDialog->showTab_VM(true);

    if (!addr.isEmpty())
        signVerifyMessageDialog->setAddress_VM(addr);
}

bool WalletView::handlePaymentRequest(const Bitcredit_SendCoinsRecipient& recipient)
{
    return sendCoinsPage->handlePaymentRequest(recipient);
}

void WalletView::showOutOfSyncWarning(bool fShow)
{
    overviewPage->showOutOfSyncWarning(fShow);
}

void WalletView::updateEncryptionStatus()
{
    emit bitcredit_encryptionStatusChanged(bitcredit_model->getEncryptionStatus());
    emit bitcoin_encryptionStatusChanged(bitcoin_model->getEncryptionStatus());
    emit deposit_encryptionStatusChanged(deposit_model->getEncryptionStatus());
}

void WalletView::bitcredit_encryptWallet(bool status)
{
    if(!bitcredit_model)
        return;
    Bitcredit_AskPassphraseDialog dlg(status ? Bitcredit_AskPassphraseDialog::Encrypt : Bitcredit_AskPassphraseDialog::Decrypt, this);
    dlg.setModel(bitcredit_model);
    dlg.exec();

    updateEncryptionStatus();
}
void WalletView::bitcoin_encryptWallet(bool status)
{
    if(!bitcoin_model)
        return;
    Bitcoin_AskPassphraseDialog dlg(status ? Bitcoin_AskPassphraseDialog::Encrypt : Bitcoin_AskPassphraseDialog::Decrypt, this);
    dlg.setModel(bitcoin_model);
    dlg.exec();

    updateEncryptionStatus();
}
void WalletView::deposit_encryptWallet(bool status)
{
    if(!deposit_model)
        return;
    Bitcredit_AskPassphraseDialog dlg(status ? Bitcredit_AskPassphraseDialog::Encrypt : Bitcredit_AskPassphraseDialog::Decrypt, this);
    dlg.setModel(deposit_model);
    dlg.exec();

    updateEncryptionStatus();
}

void WalletView::bitcredit_backupWallet()
{
    QString filename = GUIUtil::getSaveFileName(this,
        tr("Backup Bitcredit Wallet"), QString(),
        tr("Bitcredit Wallet Data (*.dat)"), NULL);

    if (filename.isEmpty())
        return;

    if (!bitcredit_model->backupWallet(filename)) {
        emit message(tr("Backup Failed"), tr("There was an error trying to save the wallet data to %1.").arg(filename),
            CClientUIInterface::MSG_ERROR);
        }
    else {
        emit message(tr("Backup Successful"), tr("The wallet data was successfully saved to %1.").arg(filename),
            CClientUIInterface::MSG_INFORMATION);
    }
}
void WalletView::bitcoin_backupWallet()
{
    QString filename = GUIUtil::getSaveFileName(this,
        tr("Backup Bitcoin Wallet"), QString(),
        tr("Bitcoin Wallet Data (*.dat)"), NULL);

    if (filename.isEmpty())
        return;

    if (!bitcoin_model->backupWallet(filename)) {
        emit message(tr("Backup Failed"), tr("There was an error trying to save the wallet data to %1.").arg(filename),
            CClientUIInterface::MSG_ERROR);
        }
    else {
        emit message(tr("Backup Successful"), tr("The wallet data was successfully saved to %1.").arg(filename),
            CClientUIInterface::MSG_INFORMATION);
    }
}
void WalletView::deposit_backupWallet()
{
    QString filename = GUIUtil::getSaveFileName(this,
        tr("Backup Deposit Wallet"), QString(),
        tr("Bitcredit Deposit Data (*.dat)"), NULL);

    if (filename.isEmpty())
        return;

    if (!deposit_model->backupWallet(filename)) {
        emit message(tr("Backup Failed"), tr("There was an error trying to save the wallet data to %1.").arg(filename),
            CClientUIInterface::MSG_ERROR);
        }
    else {
        emit message(tr("Backup Successful"), tr("The wallet data was successfully saved to %1.").arg(filename),
            CClientUIInterface::MSG_INFORMATION);
    }
}


void WalletView::bitcredit_changePassphrase()
{
    Bitcredit_AskPassphraseDialog dlg(Bitcredit_AskPassphraseDialog::ChangePass, this);
    dlg.setModel(bitcredit_model);
    dlg.exec();
}
void WalletView::bitcoin_changePassphrase()
{
    Bitcoin_AskPassphraseDialog dlg(Bitcoin_AskPassphraseDialog::ChangePass, this);
    dlg.setModel(bitcoin_model);
    dlg.exec();
}
void WalletView::deposit_changePassphrase()
{
    Bitcredit_AskPassphraseDialog dlg(Bitcredit_AskPassphraseDialog::ChangePass, this);
    dlg.setModel(deposit_model);
    dlg.exec();
}

void WalletView::bitcredit_unlockWallet()
{
    if(!bitcredit_model)
        return;
    // Unlock wallet when requested by wallet model
    if (bitcredit_model->getEncryptionStatus() == Bitcredit_WalletModel::Locked)
    {
        Bitcredit_AskPassphraseDialog dlg(Bitcredit_AskPassphraseDialog::Unlock, this);
        dlg.setModel(bitcredit_model);
        dlg.exec();
    }
}
void WalletView::bitcoin_unlockWallet()
{
    if(!bitcoin_model)
        return;
    // Unlock wallet when requested by wallet model
    if (bitcoin_model->getEncryptionStatus() == Bitcoin_WalletModel::Locked)
    {
        Bitcoin_AskPassphraseDialog dlg(Bitcoin_AskPassphraseDialog::Unlock, this);
        dlg.setModel(bitcoin_model);
        dlg.exec();
    }
}
void WalletView::deposit_unlockWallet()
{
    if(!deposit_model)
        return;
    // Unlock wallet when requested by wallet model
    if (deposit_model->getEncryptionStatus() == Bitcredit_WalletModel::Locked)
    {
        Bitcredit_AskPassphraseDialog dlg(Bitcredit_AskPassphraseDialog::Unlock, this);
        dlg.setModel(deposit_model);
        dlg.exec();
    }
}

void WalletView::usedSendingAddresses()
{
    if(!bitcredit_model)
        return;
    AddressBookPage *dlg = new AddressBookPage(AddressBookPage::ForEditing, AddressBookPage::SendingTab, this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setModel(bitcredit_model->getAddressTableModel());
    dlg->show();
}

void WalletView::usedReceivingAddresses()
{
    if(!bitcredit_model)
        return;
    AddressBookPage *dlg = new AddressBookPage(AddressBookPage::ForEditing, AddressBookPage::ReceivingTab, this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setModel(bitcredit_model->getAddressTableModel());
    dlg->show();
}

void WalletView::bitcredit_setNumBlocks(int count) {
	if(!bitcredit_model)
		return;
	minerCoinsPage->refreshStatistics();
}

void WalletView::showProgress(const QString &title, int nProgress)
{
    if (nProgress == 0)
    {
        progressDialog = new QProgressDialog(title, "", 0, 100);
        progressDialog->setWindowModality(Qt::ApplicationModal);
        progressDialog->setMinimumDuration(0);
        progressDialog->setCancelButton(0);
        progressDialog->setAutoClose(false);
        progressDialog->setValue(0);
    }
    else if (nProgress == 100)
    {
        if (progressDialog)
        {
            progressDialog->close();
            progressDialog->deleteLater();
        }
    }
    else if (progressDialog)
        progressDialog->setValue(nProgress);
}
