// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "walletview.h"

#include "addressbookpage.h"
#include "askpassphrasedialog.h"
#include "bitcoingui.h"
#include "clientmodel.h"
#include "guiutil.h"
#include "masternodeconfig.h"
#include "systemnodeconfig.h"
#include "optionsmodel.h"
#include "overviewpage.h"
#include "receivecoinsdialog.h"
#include "sendcoinsdialog.h"
#include "signverifymessagedialog.h"
#include "transactiontablemodel.h"
#include "transactionview.h"
#include "transactionrecord.h"

#include "walletmodel.h"

#include "ui_interface.h"

#include <QAction>
#include <QActionGroup>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressDialog>
#include <QPushButton>
#include <QVBoxLayout>

WalletView::WalletView(QWidget *parent):
    QStackedWidget(parent),
    clientModel(0),
    walletModel(0)
{
    // Create tabs
    overviewPage = new OverviewPage();

    transactionsPage = new QWidget(this);
    QVBoxLayout *vbox = new QVBoxLayout();
    QHBoxLayout *hbox_buttons = new QHBoxLayout();
    transactionView = new TransactionView(this);
    vbox->addWidget(transactionView);
    QPushButton *exportButton = new QPushButton(tr("&Export"), this);
    exportButton->setToolTip(tr("Export the data in the current tab to a file"));
#ifndef Q_OS_MAC // Icons on push buttons are very uncommon on Mac
    exportButton->setIcon(QIcon(":/icons/export"));
#endif
    hbox_buttons->addStretch();

    // Sum of selected transactions
    QLabel *transactionSumLabel = new QLabel(); // Label
    transactionSumLabel->setObjectName("transactionSumLabel"); // Label ID as CSS-reference
    transactionSumLabel->setText(tr("Selected amount:"));
    hbox_buttons->addWidget(transactionSumLabel);

    transactionSum = new QLabel(); // Amount
    transactionSum->setObjectName("transactionSum"); // Label ID as CSS-reference
    transactionSum->setMinimumSize(200, 8);
    transactionSum->setTextInteractionFlags(Qt::TextSelectableByMouse);
    hbox_buttons->addWidget(transactionSum);

    hbox_buttons->addWidget(exportButton);
    vbox->addLayout(hbox_buttons);
    transactionsPage->setLayout(vbox);

    receiveCoinsPage = new ReceiveCoinsDialog();
    sendCoinsPage = new SendCoinsDialog();
    if (masternodeConfig.getCount() >= 0) {
        masternodeListPage = new MasternodeList();
    }
    if (systemnodeConfig.getCount() >= 0) {
        systemnodeListPage = new SystemnodeList();
    }

    addWidget(overviewPage);
    addWidget(transactionsPage);
    addWidget(receiveCoinsPage);
    addWidget(sendCoinsPage);
    if (masternodeConfig.getCount() >= 0) {
        addWidget(masternodeListPage);
    }
    if (systemnodeConfig.getCount() >= 0) {
        addWidget(systemnodeListPage);
    }

    // Clicking on a transaction on the overview pre-selects the transaction on the transaction history page
    connect(overviewPage, SIGNAL(transactionClicked(QModelIndex)), transactionView, SLOT(focusTransaction(QModelIndex)));

    // Double-clicking on a transaction on the transaction history page shows details
    connect(transactionView, SIGNAL(doubleClicked(QModelIndex)), transactionView, SLOT(showDetails()));

    // Update wallet with sum of selected transactions
    connect(transactionView, SIGNAL(trxAmount(QString)), this, SLOT(trxAmount(QString)));

    // Clicking on "Export" allows to export the transaction list
    connect(exportButton, SIGNAL(clicked()), transactionView, SLOT(exportClicked()));

    // Pass through messages from sendCoinsPage
    connect(sendCoinsPage, SIGNAL(message(QString,QString,unsigned int)), this, SIGNAL(message(QString,QString,unsigned int)));
    // Pass through messages from transactionView
    connect(transactionView, SIGNAL(message(QString,QString,unsigned int)), this, SIGNAL(message(QString,QString,unsigned int)));
}

WalletView::~WalletView()
{
}

void WalletView::setBitcoinGUI(BitcoinGUI *gui)
{
    if (gui)
    {
        // Clicking on a transaction on the overview page simply sends you to transaction history page
        connect(overviewPage, SIGNAL(transactionClicked(QModelIndex)), gui, SLOT(gotoHistoryPage()));

        // Receive and report messages
        connect(this, SIGNAL(message(QString,QString,unsigned int)), gui, SLOT(message(QString,QString,unsigned int)));

        // Pass through encryption status changed signals
        connect(this, SIGNAL(encryptionStatusChanged(int)), gui, SLOT(setEncryptionStatus(int)));

        // Pass through transaction notifications
        connect(this, SIGNAL(incomingTransaction(QString,int,CAmount,QString,QString)), gui, SLOT(incomingTransaction(QString,int,CAmount,QString,QString)));

        connect(this, SIGNAL(guiEnableSystemnodesChanged(bool)), gui, SLOT(guiEnableSystemnodesChanged(bool)));
        connect(this, SIGNAL(guiEnableMasternodesChanged(bool)), gui, SLOT(guiEnableMasternodesChanged(bool)));
    }
}

void WalletView::setClientModel(ClientModel *clientModel)
{
    this->clientModel = clientModel;

    overviewPage->setClientModel(clientModel);
    sendCoinsPage->setClientModel(clientModel);
    if (masternodeConfig.getCount() >= 0) {
        masternodeListPage->setClientModel(clientModel);
    }
    if (systemnodeConfig.getCount() >= 0) {
        systemnodeListPage->setClientModel(clientModel);
    }
}

void WalletView::setWalletModel(WalletModel *walletModel)
{
    this->walletModel = walletModel;

    // Put transaction list in tabs
    transactionView->setModel(walletModel);
    overviewPage->setWalletModel(walletModel);
    receiveCoinsPage->setModel(walletModel);
    sendCoinsPage->setModel(walletModel);
    if (masternodeConfig.getCount() >= 0) {
        masternodeListPage->setWalletModel(walletModel);
    }
    if (systemnodeConfig.getCount() >= 0) {
        systemnodeListPage->setWalletModel(walletModel);
    }

    if (walletModel)
    {
        // Receive and pass through messages from wallet model
        connect(walletModel, SIGNAL(message(QString,QString,unsigned int)), this, SIGNAL(message(QString,QString,unsigned int)));

        // Handle changes in encryption status
        connect(walletModel, SIGNAL(encryptionStatusChanged(int)), this, SIGNAL(encryptionStatusChanged(int)));
        updateEncryptionStatus();

        // Balloon pop-up for new transaction
        connect(walletModel->getTransactionTableModel(), SIGNAL(rowsInserted(QModelIndex,int,int)),
                this, SLOT(processNewTransaction(QModelIndex,int,int)));

        // Ask for passphrase if needed
        connect(walletModel, SIGNAL(requireUnlock()), this, SLOT(unlockWallet()));

        // Show progress dialog
        connect(walletModel, SIGNAL(showProgress(QString,int)), this, SLOT(showProgress(QString,int)));

        // Enable systemnodes tab 
        connect(walletModel->getOptionsModel(), SIGNAL(enableSystemnodesChanged(bool)), this, SLOT(enableSystemnodesChanged(bool)));
        
        // Enable masternodes tab 
        connect(walletModel->getOptionsModel(), SIGNAL(enableMasternodesChanged(bool)), this, SLOT(enableMasternodesChanged(bool)));
    }
}

void WalletView::processNewTransaction(const QModelIndex& parent, int start, int /*end*/)
{
    // Prevent balloon-spam when initial block download is in progress
    if (!walletModel || !clientModel || clientModel->inInitialBlockDownload())
        return;

    TransactionTableModel *ttm = walletModel->getTransactionTableModel();
    if (!ttm || ttm->processingQueuedTransactions())
        return;

    QString date = ttm->index(start, TransactionTableModel::Date, parent).data().toString();
    qint64 amount = ttm->index(start, TransactionTableModel::Amount, parent).data(Qt::EditRole).toULongLong();
    QString type = ttm->index(start, TransactionTableModel::Type, parent).data().toString();
    QString address = ttm->index(start, TransactionTableModel::ToAddress, parent).data().toString();

    // If payment is 500 or 10000 CRW ask to create a Systemnode/Masternode
    qint64 credit = ttm->index(start, TransactionTableModel::AmountCredit, parent).data(Qt::EditRole).toULongLong();
    int typeEnum = ttm->index(start, 0, parent).data(TransactionTableModel::TypeRole).toInt();

    // Payment to yourself, SN or MN
    //if (typeEnum == TransactionRecord::SendToSelf && (credit == SYSTEMNODE_COLLATERAL * COIN || credit == MASTERNODE_COLLATERAL * COIN))
    //{
    //    AvailableCoinsType coin_type = ONLY_500;
    //    QString title = tr("Payment to yourself - ") + 
    //        BitcoinUnits::formatWithUnit(walletModel->getOptionsModel()->getDisplayUnit(), credit);
    //    QString body = tr("Do you want to create a new ");
    //    if (credit == SYSTEMNODE_COLLATERAL * COIN)
    //    {
    //        body += "Systemnode?";
    //    }
    //    else if (credit == MASTERNODE_COLLATERAL * COIN)
    //    {
    //        body += "Masternode?";
    //        coin_type = ONLY_10000;
    //    }

    //    // Display message box
    //    QMessageBox::StandardButton retval = QMessageBox::question(this, title, body,
    //            QMessageBox::Yes | QMessageBox::Cancel,
    //            QMessageBox::Cancel);

    //    if(retval == QMessageBox::Yes)
    //    {
    //        QString hash = ttm->index(start, 0, parent).data(TransactionTableModel::TxHashRole).toString();
    //        std::vector<COutput> vPossibleCoins;
    //        pwalletMain->AvailableCoins(vPossibleCoins, true, NULL, coin_type);
    //        BOOST_FOREACH(COutput& out, vPossibleCoins) {
    //            if (out.tx->GetHash().ToString() == hash.toStdString())
    //            {
    //                COutPoint outpoint = COutPoint(out.tx->GetHash(), boost::lexical_cast<unsigned int>(out.i));
    //                pwalletMain->LockCoin(outpoint);

    //                // Generate a key
    //                CKey secret;
    //                secret.MakeNewKey(false);
    //                std::string privateKey = CBitcoinSecret(secret).ToString();

    //                systemnodeConfig.add("", "", privateKey, hash.toStdString(), strprintf("%d", out.i));
    //                systemnodeListPage->updateMyNodeList(true);
    //            }
    //        }
    //    }
    //}
    emit incomingTransaction(date, walletModel->getOptionsModel()->getDisplayUnit(), amount, type, address);
}

void WalletView::gotoOverviewPage()
{
    setCurrentWidget(overviewPage);
}

void WalletView::gotoHistoryPage()
{
    setCurrentWidget(transactionsPage);
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

void WalletView::gotoMasternodePage()
{
    if (masternodeConfig.getCount() >= 0 || walletModel->getOptionsModel()->getMasternodesEnabled()) {
        setCurrentWidget(masternodeListPage);
    }
}

void WalletView::gotoSystemnodePage()
{
    if (systemnodeConfig.getCount() >= 0 || walletModel->getOptionsModel()->getSystemnodesEnabled()) {
        setCurrentWidget(systemnodeListPage);
    }
}

void WalletView::gotoMultisigTab()
{
    // calls show() in showTab_SM()
    MultisigDialog *multisigDialog = new MultisigDialog(this);
    multisigDialog->setAttribute(Qt::WA_DeleteOnClose);
    multisigDialog->setModel(walletModel);
    multisigDialog->showTab(true);
}

void WalletView::gotoSignMessageTab(QString addr)
{
    // calls show() in showTab_SM()
    SignVerifyMessageDialog *signVerifyMessageDialog = new SignVerifyMessageDialog(this);
    signVerifyMessageDialog->setAttribute(Qt::WA_DeleteOnClose);
    signVerifyMessageDialog->setModel(walletModel);
    signVerifyMessageDialog->showTab_SM(true);

    if (!addr.isEmpty())
        signVerifyMessageDialog->setAddress_SM(addr);
}

void WalletView::gotoVerifyMessageTab(QString addr)
{
    // calls show() in showTab_VM()
    SignVerifyMessageDialog *signVerifyMessageDialog = new SignVerifyMessageDialog(this);
    signVerifyMessageDialog->setAttribute(Qt::WA_DeleteOnClose);
    signVerifyMessageDialog->setModel(walletModel);
    signVerifyMessageDialog->showTab_VM(true);

    if (!addr.isEmpty())
        signVerifyMessageDialog->setAddress_VM(addr);
}

bool WalletView::handlePaymentRequest(const SendCoinsRecipient& recipient)
{
    return sendCoinsPage->handlePaymentRequest(recipient);
}

void WalletView::showOutOfSyncWarning(bool fShow)
{
    overviewPage->showOutOfSyncWarning(fShow);
}

void WalletView::updateEncryptionStatus()
{
    emit encryptionStatusChanged(walletModel->getEncryptionStatus());
}

void WalletView::encryptWallet(bool status)
{
    if(!walletModel)
        return;
    AskPassphraseDialog dlg(status ? AskPassphraseDialog::Encrypt : AskPassphraseDialog::Decrypt, this);
    dlg.setModel(walletModel);
    dlg.exec();

    updateEncryptionStatus();
}

void WalletView::backupWallet()
{
    QString filename = GUIUtil::getSaveFileName(this,
        tr("Backup Wallet"), QString(),
        tr("Wallet Data (*.dat)"), NULL);

    if (filename.isEmpty())
        return;

    if (!walletModel->backupWallet(filename)) {
        emit message(tr("Backup Failed"), tr("There was an error trying to save the wallet data to %1.").arg(filename),
            CClientUIInterface::MSG_ERROR);
        }
    else {
        emit message(tr("Backup Successful"), tr("The wallet data was successfully saved to %1.").arg(filename),
            CClientUIInterface::MSG_INFORMATION);
    }
}

void WalletView::changePassphrase()
{
    AskPassphraseDialog dlg(AskPassphraseDialog::ChangePass, this);
    dlg.setModel(walletModel);
    dlg.exec();
}

void WalletView::unlockWallet()
{
    if(!walletModel)
        return;
    // Unlock wallet when requested by wallet model

    if (walletModel->getEncryptionStatus() == WalletModel::Locked)
    {
        AskPassphraseDialog dlg(AskPassphraseDialog::Unlock, this);
        dlg.setModel(walletModel);
        dlg.exec();
    }
}

void WalletView::lockWallet()
{
    if(!walletModel)
        return;

    walletModel->setWalletLocked(true);
}

void WalletView::usedSendingAddresses()
{
    if(!walletModel)
        return;
    AddressBookPage *dlg = new AddressBookPage(AddressBookPage::ForEditing, AddressBookPage::SendingTab, this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setModel(walletModel->getAddressTableModel());
    dlg->show();
}

void WalletView::usedReceivingAddresses()
{
    if(!walletModel)
        return;
    AddressBookPage *dlg = new AddressBookPage(AddressBookPage::ForEditing, AddressBookPage::ReceivingTab, this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setModel(walletModel->getAddressTableModel());
    dlg->show();
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

/** Update wallet with the sum of the selected transactions */
void WalletView::trxAmount(QString amount)
{
    transactionSum->setText(amount);
}

/** Enable systemnodes tab */
void WalletView::enableSystemnodesChanged(bool enabled)
{
    if (enabled && systemnodeListPage == NULL)
    {
        systemnodeListPage = new SystemnodeList();
        addWidget(systemnodeListPage);
        systemnodeListPage->setClientModel(clientModel);
        systemnodeListPage->setWalletModel(walletModel);
    }
    emit guiEnableSystemnodesChanged(enabled);
}

/** Enabled masternodes tab */
void WalletView::enableMasternodesChanged(bool enabled)
{
    if (enabled && masternodeListPage == NULL)
    {
        masternodeListPage = new MasternodeList();
        addWidget(masternodeListPage);
        masternodeListPage->setClientModel(clientModel);
        masternodeListPage->setWalletModel(walletModel);
    }
    emit guiEnableMasternodesChanged(enabled);
}
