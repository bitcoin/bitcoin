// Copyright (c) 2011-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "claimcoinsdialog.h"
#include "ui_claimcoinsdialog.h"

#include "bitcoin_addresstablemodel.h"
#include "bitcoinunits.h"
#include "bitcoin_coincontroldialog.h"
#include "guiutil.h"
#include "optionsmodel.h"
#include "uneditablecoinsentry.h"
#include "bitcoin_walletmodel.h"
#include "walletmodel.h"

#include "base58.h"
#include "coincontrol.h"
#include "ui_interface.h"

#include "rpcserver.h"

#include <QMessageBox>
#include <QScrollBar>
#include <QTextDocument>

ClaimCoinsDialog::ClaimCoinsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ClaimCoinsDialog),
    bitcredit_model(0),
    bitcoin_model(0)
{
    ui->setupUi(this);

#ifdef Q_OS_MAC // Icons on push buttons are very uncommon on Mac
    ui->sendButton->setIcon(QIcon());
#endif

    // Coin Control
    connect(ui->pushButtonCoinControl, SIGNAL(clicked()), this, SLOT(coinControlButtonClicked()));

    // Coin Control: clipboard actions
    QAction *clipboardQuantityAction = new QAction(tr("Copy quantity"), this);
    QAction *clipboardAmountAction = new QAction(tr("Copy amount"), this);
    QAction *clipboardFeeAction = new QAction(tr("Copy fee"), this);
    QAction *clipboardAfterFeeAction = new QAction(tr("Copy after fee"), this);
    QAction *clipboardBytesAction = new QAction(tr("Copy bytes"), this);
    QAction *clipboardPriorityAction = new QAction(tr("Copy priority"), this);
    QAction *clipboardLowOutputAction = new QAction(tr("Copy low output"), this);
    QAction *clipboardChangeAction = new QAction(tr("Copy change"), this);
    connect(clipboardQuantityAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardQuantity()));
    connect(clipboardAmountAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardAmount()));
    connect(clipboardFeeAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardFee()));
    connect(clipboardAfterFeeAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardAfterFee()));
    connect(clipboardBytesAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardBytes()));
    connect(clipboardPriorityAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardPriority()));
    connect(clipboardLowOutputAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardLowOutput()));
    connect(clipboardChangeAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardChange()));
    ui->labelCoinControlQuantity->addAction(clipboardQuantityAction);
    ui->labelCoinControlAmount->addAction(clipboardAmountAction);
    ui->labelCoinControlFee->addAction(clipboardFeeAction);
    ui->labelCoinControlAfterFee->addAction(clipboardAfterFeeAction);
    ui->labelCoinControlBytes->addAction(clipboardBytesAction);
    ui->labelCoinControlPriority->addAction(clipboardPriorityAction);
    ui->labelCoinControlLowOutput->addAction(clipboardLowOutputAction);
    ui->labelCoinControlChange->addAction(clipboardChangeAction);
}

void ClaimCoinsDialog::setModel(Bitcredit_WalletModel *bitcredit_model, Bitcoin_WalletModel * bitcoin_model)
{
    this->bitcredit_model = bitcredit_model;
    this->bitcoin_model = bitcoin_model;

    if(bitcoin_model && bitcoin_model->getOptionsModel())
    {
        for(int i = 0; i < ui->entries->count(); ++i)
        {
            UneditableCoinsEntry *entry = qobject_cast<UneditableCoinsEntry*>(ui->entries->itemAt(i)->widget());
            if(entry)
            {
                entry->setModel(bitcredit_model);
            }
        }

        refreshBalance();
        connect(bitcoin_model, SIGNAL(balanceChanged(qint64, qint64, qint64)), this, SLOT(refreshBalance()));
        connect(bitcoin_model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));

        connect(bitcredit_model, SIGNAL(balanceChanged(qint64, qint64, qint64, qint64, qint64)), this, SLOT(refreshBalance()));

        // Coin Control
        connect(bitcoin_model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(coinControlUpdateLabels()));
        connect(bitcoin_model->getOptionsModel(), SIGNAL(transactionFeeChanged(qint64)), this, SLOT(coinControlUpdateLabels()));
        ui->frameCoinControl->setVisible(true);
        coinControlUpdateLabels();
    }
}

ClaimCoinsDialog::~ClaimCoinsDialog()
{
    delete ui;
}

void ClaimCoinsDialog::on_sendButton_clicked()
{
    if(!bitcoin_model || !bitcoin_model->getOptionsModel())
        return;

    if(!bitcredit_model || !bitcredit_model->getOptionsModel())
        return;

    QList<Bitcredit_SendCoinsRecipient> recipients;
    bool valid = true;

    for(int i = 0; i < ui->entries->count(); ++i)
    {
        UneditableCoinsEntry *entry = qobject_cast<UneditableCoinsEntry*>(ui->entries->itemAt(i)->widget());
        if(entry)
        {
            if(entry->validate())
            {
                recipients.append(entry->getValue());
            }
            else
            {
                valid = false;
            }
        }
    }

    if(!valid || recipients.isEmpty())
    {
        return;
    }

    const int displayUnit = bitcoin_model->getOptionsModel()->getDisplayUnit();

    // Format confirmation message
    QStringList formatted;
    foreach(const Bitcredit_SendCoinsRecipient &rcp, recipients)
    {
        // generate bold amount string
        QString amount = "<b>" + BitcoinUnits::formatWithUnit(displayUnit, rcp.amount);
        amount.append("</b>");

        QString recipientElement = tr("%1 to this wallet").arg(amount);

        formatted.append(recipientElement);
    }


    Bitcoin_WalletModel::UnlockContext bitcoin_ctx(bitcoin_model->requestUnlock());
    if(!bitcoin_ctx.isValid())
    {
        // Unlock wallet was cancelled
        return;
    }

    Bitcredit_WalletModel::UnlockContext bitcredit_ctx(bitcredit_model->requestUnlock());
    if(!bitcredit_ctx.isValid())
    {
        // Unlock wallet was cancelled
        return;
    }

    // prepare transaction for getting txFee earlier
    Bitcredit_WalletModelTransaction currentTransaction(recipients);
    Bitcredit_WalletModel::SendCoinsReturn prepareStatus = bitcredit_model->prepareClaimTransaction(bitcoin_model, credits_pcoinsTip, currentTransaction, Bitcoin_CoinControlDialog::coinControl);

    // process prepareStatus and on error generate message shown to user
    processSendCoinsReturn(prepareStatus,
        BitcoinUnits::formatWithUnit(displayUnit, currentTransaction.getTransactionFee()));

    if(prepareStatus.status != Bitcredit_WalletModel::OK) {
        return;
    }

    qint64 txFee = currentTransaction.getTransactionFee();
    QString questionString = tr("Are you sure you want to claim bitcoins?");
    questionString.append("<br /><br />%1");

    if(txFee > 0)
    {
        // append fee string if a fee is required
        questionString.append("<hr /><span style='color:#aa0000;'>");
        questionString.append(BitcoinUnits::formatWithUnit(displayUnit, txFee));
        questionString.append("</span> ");
        questionString.append(tr("added as transaction fee"));
    }

    // add total amount in all subdivision units
    questionString.append("<hr />");
    qint64 totalAmount = currentTransaction.getTotalTransactionAmount() + txFee;
    QStringList alternativeUnits;
    foreach(BitcoinUnits::Unit u, BitcoinUnits::availableUnits())
    {
        if(u != displayUnit)
            alternativeUnits.append(BitcoinUnits::formatWithUnit(u, totalAmount));
    }
    questionString.append(tr("Total Amount %1 (= %2)")
        .arg(BitcoinUnits::formatWithUnit(displayUnit, totalAmount))
        .arg(alternativeUnits.join(" " + tr("or") + " ")));

    QMessageBox::StandardButton retval = QMessageBox::question(this, tr("Confirm claim bitcoins"),
        questionString.arg(formatted.join("<br />")),
        QMessageBox::Yes | QMessageBox::Cancel,
        QMessageBox::Cancel);

    if(retval != QMessageBox::Yes)
    {
        return;
    }

    // now send the prepared transaction
    Bitcredit_WalletModel::SendCoinsReturn sendStatus = bitcredit_model->sendCoins(currentTransaction);
    // process sendStatus and on error generate message shown to user
    processSendCoinsReturn(sendStatus);

    if (sendStatus.status == Bitcredit_WalletModel::OK)
    {
        accept();
        Bitcoin_CoinControlDialog::coinControl->UnSelectAll();
        coinControlUpdateLabels();
    }
}

void ClaimCoinsDialog::clear()
{
    // Remove entries until only one left
    while(ui->entries->count())
    {
        ui->entries->takeAt(0)->widget()->deleteLater();
    }

    updateTabsAndLabels();
}

void ClaimCoinsDialog::reject()
{
    clear();
}

void ClaimCoinsDialog::accept()
{
    clear();
}

UneditableCoinsEntry *ClaimCoinsDialog::addEntry(qint64 value)
{
    UneditableCoinsEntry *entry = new UneditableCoinsEntry(this);
    entry->setModel(bitcredit_model);
    if(value != -1) {
    	entry->setValue(value);
    }

    ui->entries->addWidget(entry);
    connect(entry, SIGNAL(removeEntry(UneditableCoinsEntry*)), this, SLOT(removeEntry(UneditableCoinsEntry*)));
    connect(entry, SIGNAL(payAmountChanged()), this, SLOT(coinControlUpdateLabels()));

    updateTabsAndLabels();

    // Focus the field, so that entry can start immediately
    entry->setFocus();
    ui->scrollAreaWidgetContents->resize(ui->scrollAreaWidgetContents->sizeHint());
    qApp->processEvents();
    QScrollBar* bar = ui->scrollArea->verticalScrollBar();
    if(bar)
        bar->setSliderPosition(bar->maximum());
    return entry;
}

void ClaimCoinsDialog::updateTabsAndLabels()
{
    setupTabChain(0);
    coinControlUpdateLabels();
}

void ClaimCoinsDialog::removeEntry(UneditableCoinsEntry* entry)
{
    entry->hide();
    entry->deleteLater();

    updateTabsAndLabels();
}

QWidget *ClaimCoinsDialog::setupTabChain(QWidget *prev)
{
    for(int i = 0; i < ui->entries->count(); ++i)
    {
        UneditableCoinsEntry *entry = qobject_cast<UneditableCoinsEntry*>(ui->entries->itemAt(i)->widget());
        if(entry)
        {
            prev = entry->setupTabChain(prev);
        }
    }
    QWidget::setTabOrder(prev, ui->sendButton);
    return ui->sendButton;
}

void ClaimCoinsDialog::setBalance(qint64 balance, qint64 unconfirmedBalance, qint64 immatureBalance)
{
    Q_UNUSED(unconfirmedBalance);
    Q_UNUSED(immatureBalance);

    if(bitcoin_model && bitcoin_model->getOptionsModel())
    {
        ui->labelBalance->setText(BitcoinUnits::formatWithUnit(bitcoin_model->getOptionsModel()->getDisplayUnit(), balance));
    }
}

void ClaimCoinsDialog::refreshBalance()
{
    //Find all claimed  transactions
    map<uint256, set<int> > mapClaimTxInPoints;
    bitcredit_model->wallet->ClaimTxInPoints(mapClaimTxInPoints);

    setBalance(bitcoin_model->getBalance(credits_pcoinsTip, mapClaimTxInPoints), 0, 0);
}

void ClaimCoinsDialog::updateDisplayUnit()
{
    refreshBalance();
}

void ClaimCoinsDialog::processSendCoinsReturn(const Bitcredit_WalletModel::SendCoinsReturn &sendCoinsReturn, const QString &msgArg)
{
    QPair<QString, CClientUIInterface::MessageBoxFlags> msgParams;
    // Default to a warning message, override if error message is needed
    msgParams.second = CClientUIInterface::MSG_WARNING;

    // This comment is specific to ClaimCoinsDialog usage of WalletModel::SendCoinsReturn.
    // WalletModel::TransactionCommitFailed is used only in WalletModel::sendCoins()
    // all others are used only in WalletModel::prepareTransaction()
    switch(sendCoinsReturn.status)
    {
    case Bitcredit_WalletModel::InvalidAddress:
        msgParams.first = tr("The recipient address is not valid, please recheck.");
        break;
    case Bitcredit_WalletModel::InvalidAmount:
        msgParams.first = tr("The amount to pay must be larger than 0.");
        break;
    case Bitcredit_WalletModel::AmountExceedsBalance:
        msgParams.first = tr("The amount exceeds your balance.");
        break;
    case Bitcredit_WalletModel::AmountWithFeeExceedsBalance:
        msgParams.first = tr("The total exceeds your balance when the %1 transaction fee is included.").arg(msgArg);
        break;
    case Bitcredit_WalletModel::DuplicateAddress:
        msgParams.first = tr("Duplicate address found, can only send to each address once per send operation.");
        break;
    case Bitcredit_WalletModel::TransactionCreationFailed:
        msgParams.first = tr("Claim creation failed!");
        msgParams.second = CClientUIInterface::MSG_ERROR;
        break;
    case Bitcredit_WalletModel::TransactionCommitFailed:
        msgParams.first = tr("The claim was rejected! This might happen if some of the coins in your wallet were already spent, such as if you used a copy of wallet.dat and coins were spent in the copy but not marked as spent here.");
        msgParams.second = CClientUIInterface::MSG_ERROR;
        break;
    // included to prevent a compiler warning.
    case Bitcredit_WalletModel::OK:
    default:
        return;
    }

    emit message(tr("Claim Coins"), msgParams.first, msgParams.second);
}

// Coin Control: copy label "Quantity" to clipboard
void ClaimCoinsDialog::coinControlClipboardQuantity()
{
    GUIUtil::setClipboard(ui->labelCoinControlQuantity->text());
}

// Coin Control: copy label "Amount" to clipboard
void ClaimCoinsDialog::coinControlClipboardAmount()
{
    GUIUtil::setClipboard(ui->labelCoinControlAmount->text().left(ui->labelCoinControlAmount->text().indexOf(" ")));
}

// Coin Control: copy label "Fee" to clipboard
void ClaimCoinsDialog::coinControlClipboardFee()
{
    GUIUtil::setClipboard(ui->labelCoinControlFee->text().left(ui->labelCoinControlFee->text().indexOf(" ")));
}

// Coin Control: copy label "After fee" to clipboard
void ClaimCoinsDialog::coinControlClipboardAfterFee()
{
    GUIUtil::setClipboard(ui->labelCoinControlAfterFee->text().left(ui->labelCoinControlAfterFee->text().indexOf(" ")));
}

// Coin Control: copy label "Bytes" to clipboard
void ClaimCoinsDialog::coinControlClipboardBytes()
{
    GUIUtil::setClipboard(ui->labelCoinControlBytes->text());
}

// Coin Control: copy label "Priority" to clipboard
void ClaimCoinsDialog::coinControlClipboardPriority()
{
    GUIUtil::setClipboard(ui->labelCoinControlPriority->text());
}

// Coin Control: copy label "Low output" to clipboard
void ClaimCoinsDialog::coinControlClipboardLowOutput()
{
    GUIUtil::setClipboard(ui->labelCoinControlLowOutput->text());
}

// Coin Control: copy label "Change" to clipboard
void ClaimCoinsDialog::coinControlClipboardChange()
{
    GUIUtil::setClipboard(ui->labelCoinControlChange->text().left(ui->labelCoinControlChange->text().indexOf(" ")));
}

// Coin Control: button inputs -> show actual coin control dialog
void ClaimCoinsDialog::coinControlButtonClicked()
{
    Bitcoin_CoinControlDialog dlg;
    dlg.setModel(bitcoin_model, bitcredit_model);
    dlg.exec();
    
    // Remove entries and add new
    while(ui->entries->count()) {
        ui->entries->takeAt(0)->widget()->deleteLater();
    }

    if(Bitcoin_CoinControlDialog::amountAfterFee > 0) {
    	addEntry(Bitcoin_CoinControlDialog::amountAfterFee);
    }

    coinControlUpdateLabels();
}

// Coin Control: update labels
void ClaimCoinsDialog::coinControlUpdateLabels()
{
    if (!bitcoin_model || !bitcoin_model->getOptionsModel())
        return;

    // set pay amounts
    Bitcoin_CoinControlDialog::payAmounts.clear();
    for(int i = 0; i < ui->entries->count(); ++i)
    {
        UneditableCoinsEntry *entry = qobject_cast<UneditableCoinsEntry*>(ui->entries->itemAt(i)->widget());
        if(entry)
            Bitcoin_CoinControlDialog::payAmounts.append(entry->getValue().amount);
    }

    if (Bitcoin_CoinControlDialog::coinControl->HasSelected())
    {
        // actual coin control calculation
    	Bitcoin_CoinControlDialog::updateLabels(bitcoin_model, bitcredit_model, this);

        // show coin control stats
        ui->widgetCoinControl->show();
    }
    else
    {
        // hide coin control stats
        ui->widgetCoinControl->hide();
        ui->labelCoinControlInsuffFunds->hide();
    }
}
