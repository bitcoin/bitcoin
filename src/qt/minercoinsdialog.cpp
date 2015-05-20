// Copyright (c) 2011-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "minercoinsdialog.h"
#include "ui_minercoinsdialog.h"

#include "addresstablemodel.h"
#include "bitcreditunits.h"
#include "miner_coincontroldialog.h"
#include "guiutil.h"
#include "optionsmodel.h"
#include "uneditablecoinsentry.h"
#include "walletmodel.h"

#include "base58.h"
#include "coincontrol.h"
#include "ui_interface.h"

#include <QMessageBox>
#include <QScrollBar>
#include <QTextDocument>

MinerCoinsDialog::MinerCoinsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MinerCoinsDialog),
    bitcredit_model(0),
    deposit_model(0)
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
    connect(clipboardQuantityAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardQuantity()));
    connect(clipboardAmountAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardAmount()));
    ui->labelCoinControlQuantity->addAction(clipboardQuantityAction);
    ui->labelCoinControlAmount->addAction(clipboardAmountAction);
}

void MinerCoinsDialog::setModel(Bitcredit_WalletModel *bitcredit_model, Bitcredit_WalletModel *deposit_model)
{
    this->bitcredit_model = bitcredit_model;
    this->deposit_model = deposit_model;

    if(bitcredit_model && bitcredit_model->getOptionsModel())
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
        refreshStatistics();
        connect(bitcredit_model, SIGNAL(balanceChanged(qint64, qint64, qint64, qint64, qint64)), this, SLOT(setBalance(qint64, qint64, qint64, qint64, qint64)));
        connect(bitcredit_model, SIGNAL(minerStatisticsChanged(qint64, qint64, qint64, qint64, qint64, qint64, qint64, qint64, qint64, qint64, qint64, int, qint64, int)), this, SLOT(setMinerStatistics(qint64, qint64, qint64, qint64, qint64, qint64, qint64, qint64, qint64, qint64, qint64, int, qint64, int)));
        connect(bitcredit_model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));

        connect(deposit_model, SIGNAL(balanceChanged(qint64, qint64, qint64, qint64, qint64)), this, SLOT(refreshBalance()));

        // Coin Control
        connect(bitcredit_model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(coinControlUpdateLabels()));
        connect(bitcredit_model->getOptionsModel(), SIGNAL(transactionFeeChanged(qint64)), this, SLOT(coinControlUpdateLabels()));
        ui->frameCoinControl->setVisible(true);

        ui->frameMinerStatistics->setVisible(true);
    }
}

MinerCoinsDialog::~MinerCoinsDialog()
{
    delete ui;
}

void MinerCoinsDialog::on_sendButton_clicked()
{
    Credits_CCoinsViewCache &credits_view = *credits_pcoinsTip;

    if(!bitcredit_model || !bitcredit_model->getOptionsModel())
        return;
    if(!deposit_model)
        return;

	Bitcredit_WalletModel::UnlockContext ctx(bitcredit_model->requestUnlock());
	if(!ctx.isValid())
	{
		// Unlock wallet was cancelled
		return;
	}

    vector<COutPoint> vCoinControl;
    vector<Credits_COutput>   vOutputs;
    Miner_CoinControlDialog::coinControl->ListSelected(vCoinControl);
    bitcredit_model->getOutputs(vCoinControl, vOutputs);

    const int displayUnit = bitcredit_model->getOptionsModel()->getDisplayUnit();
    qint64 totalAmount = 0;

    // Test run for deposits and format confirmation message
	QString questionString = tr("Are you sure you want to create deposits?");
	questionString.append("<br /><br />%1");
    QStringList formatted;

    for(unsigned int i = 0; i < vOutputs.size(); ++i)
    {
    	const Credits_COutput& output = vOutputs[i];
    	const int64_t nValue = output.tx->vout[output.i].nValue;

    	Bitcredit_SendCoinsRecipient recipient;
    	recipient.amount = nValue;
        QList<Bitcredit_SendCoinsRecipient> recipients;
        recipients.append(recipient);

        // generate bold amount string
        QString amount = "<b>" + BitcreditUnits::formatWithUnit(displayUnit, nValue);
        amount.append("</b>");

        QString recipientElement = tr("%1 prepared as one deposit").arg(amount);
        formatted.append(recipientElement);

		// prepare transaction
		Bitcredit_WalletModelTransaction transaction(recipients);
		Bitcredit_WalletModel::SendCoinsReturn prepareStatus = bitcredit_model->prepareDepositTransaction(deposit_model, transaction, output, credits_view);

		// process prepareStatus and on error generate message shown to user
		processSendCoinsReturn(prepareStatus,
			BitcreditUnits::formatWithUnit(displayUnit, transaction.getTransactionFee()));

		if(prepareStatus.status != Bitcredit_WalletModel::OK) {
			return;
		}

		if(!transaction.getTransaction()->IsValidDeposit()) {
			QMessageBox::critical(this, tr("Deposit Creation Failed"),
				tr("The deposit you tried to create did not validate as a correct deposit."));
			return;
		}

		totalAmount += transaction.getTotalTransactionAmount();
    }

	// add total amount in all subdivision units
	questionString.append("<hr />");
	QStringList alternativeUnits;
	foreach(BitcreditUnits::Unit u, BitcreditUnits::availableUnits())
	{
		if(u != displayUnit)
			alternativeUnits.append(BitcreditUnits::formatWithUnit(u, totalAmount));
	}
	questionString.append(tr("Total Amount %1 (= %2)")
		.arg(BitcreditUnits::formatWithUnit(displayUnit, totalAmount))
		.arg(alternativeUnits.join(" " + tr("or") + " ")));

    QMessageBox::StandardButton retval = QMessageBox::question(this, tr("Confirm create deposits"),
        questionString.arg(formatted.join("<br />")),
        QMessageBox::Yes | QMessageBox::Cancel,
        QMessageBox::Cancel);

    if(retval != QMessageBox::Yes)
    {
        return;
    }

    // now create and store the transactions for real
    for(unsigned int i = 0; i < vOutputs.size(); ++i)
    {
    	const Credits_COutput& output = vOutputs[i];
    	const int64_t nValue = output.tx->vout[output.i].nValue;

    	Bitcredit_SendCoinsRecipient recipient;
    	recipient.amount = nValue;
        QList<Bitcredit_SendCoinsRecipient> recipients;
        recipients.append(recipient);

		// prepare transaction
		Bitcredit_WalletModelTransaction transaction(recipients);
		bitcredit_model->prepareDepositTransaction(deposit_model, transaction, output, credits_view);

		//Finalize it
		Bitcredit_WalletModel::SendCoinsReturn sendStatus = deposit_model->storeDepositTransaction(bitcredit_model, transaction);
		// process sendStatus and on error generate message shown to user
		processSendCoinsReturn(sendStatus);

		if (sendStatus.status == Bitcredit_WalletModel::OK)
		{
			accept();
		}
    }

	Miner_CoinControlDialog::coinControl->UnSelectAll();
	coinControlUpdateLabels();
}

void MinerCoinsDialog::clear()
{
    // Remove entries until only one left
    while(ui->entries->count())
    {
        ui->entries->takeAt(0)->widget()->deleteLater();
    }

    updateTabsAndLabels();
}

void MinerCoinsDialog::reject()
{
    clear();
}

void MinerCoinsDialog::accept()
{
    clear();
}

UneditableCoinsEntry *MinerCoinsDialog::addEntry(qint64 value)
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

void MinerCoinsDialog::updateTabsAndLabels()
{
    setupTabChain(0);
    coinControlUpdateLabels();
}

void MinerCoinsDialog::removeEntry(UneditableCoinsEntry* entry)
{
    entry->hide();
    entry->deleteLater();

    updateTabsAndLabels();
}

QWidget *MinerCoinsDialog::setupTabChain(QWidget *prev)
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

void MinerCoinsDialog::setBalance(qint64 balance, qint64 unconfirmedBalance, qint64 immatureBalance, qint64 preparedDepositBalance, qint64 inDepositBalance)
{
    Q_UNUSED(unconfirmedBalance);
    Q_UNUSED(immatureBalance);
    Q_UNUSED(preparedDepositBalance);
    Q_UNUSED(inDepositBalance);

    if(bitcredit_model && bitcredit_model->getOptionsModel())
    {
        ui->labelBalance->setText(BitcreditUnits::formatWithUnit(bitcredit_model->getOptionsModel()->getDisplayUnit(), balance));
    }
}

void MinerCoinsDialog::refreshBalance() {
//Find all prepared deposit transactions
map<uint256, set<int> > mapPreparedDepositTxInPoints;
deposit_model->wallet->PreparedDepositTxInPoints(mapPreparedDepositTxInPoints);

setBalance(bitcredit_model->getBalance(mapPreparedDepositTxInPoints),
					bitcredit_model->getUnconfirmedBalance(mapPreparedDepositTxInPoints),
					bitcredit_model->getImmatureBalance(mapPreparedDepositTxInPoints),
					bitcredit_model->getPreparedDepositBalance(),
					bitcredit_model->getInDepositBalance());
}

void MinerCoinsDialog::setMinerStatistics(qint64 reqDeposit1, qint64 reqDeposit2, qint64 reqDeposit3, qint64 reqDeposit4, qint64 reqDeposit5, qint64 maxSubsidy1, qint64 maxSubsidy2, qint64 maxSubsidy3, qint64 maxSubsidy4, qint64 maxSubsidy5, unsigned int blockHeight, qint64 totalMonetaryBase, qint64 totalDepositBase, int nextSubsidyUpdateHeight)
{
	const int unit = bitcredit_model->getOptionsModel()->getDisplayUnit();
    ui->labelMinerStatisticsRequiredDeposit1->setText(BitcreditUnits::formatWithUnit(unit, reqDeposit1));
    ui->labelMinerStatisticsRequiredDeposit2->setText(BitcreditUnits::formatWithUnit(unit, reqDeposit2));
    ui->labelMinerStatisticsRequiredDeposit3->setText(BitcreditUnits::formatWithUnit(unit, reqDeposit3));
    ui->labelMinerStatisticsRequiredDeposit4->setText(BitcreditUnits::formatWithUnit(unit, reqDeposit4));
    ui->labelMinerStatisticsRequiredDeposit5->setText(BitcreditUnits::formatWithUnit(unit, reqDeposit5));
    ui->labelMinerStatisticsReward1->setText(BitcreditUnits::formatWithUnit(unit, maxSubsidy1));
    ui->labelMinerStatisticsReward2->setText(BitcreditUnits::formatWithUnit(unit, maxSubsidy2));
    ui->labelMinerStatisticsReward3->setText(BitcreditUnits::formatWithUnit(unit, maxSubsidy3));
    ui->labelMinerStatisticsReward4->setText(BitcreditUnits::formatWithUnit(unit, maxSubsidy4));
    ui->labelMinerStatisticsReward5->setText(BitcreditUnits::formatWithUnit(unit, maxSubsidy5));

    ui->labelMinerStatisticsBlockHeight->setText(QString::number(blockHeight));
    ui->labelMinerStatisticsTotalMonetaryBase->setText(BitcreditUnits::formatWithUnit(unit, totalMonetaryBase));
    ui->labelMinerStatisticsTotalDepositBase->setText(BitcreditUnits::formatWithUnit(unit, totalDepositBase));
    ui->labelMinerStatisticsRewardChangeAtBlock->setText(QString::number(nextSubsidyUpdateHeight));
}

void MinerCoinsDialog::refreshStatistics() {
    setMinerStatistics(bitcredit_model->getRequiredDepositLevel(0),
    								bitcredit_model->getRequiredDepositLevel(1),
    								bitcredit_model->getRequiredDepositLevel(100),
    								bitcredit_model->getRequiredDepositLevel(1000),
    								bitcredit_model->getRequiredDepositLevel(10000),
    								bitcredit_model->getMaxBlockSubsidy(0),
    								bitcredit_model->getMaxBlockSubsidy(1),
    								bitcredit_model->getMaxBlockSubsidy(100),
    								bitcredit_model->getMaxBlockSubsidy(1000),
    								bitcredit_model->getMaxBlockSubsidy(10000),
    								bitcredit_model->getBlockHeight(),
    								bitcredit_model->getTotalMonetaryBase(),
    								bitcredit_model->getTotalDepositBase(),
    								bitcredit_model->getNextSubsidyUpdateHeight());
}

void MinerCoinsDialog::updateDisplayUnit()
{
	refreshBalance();
}

void MinerCoinsDialog::processSendCoinsReturn(const Bitcredit_WalletModel::SendCoinsReturn &sendCoinsReturn, const QString &msgArg)
{
    QPair<QString, CClientUIInterface::MessageBoxFlags> msgParams;
    // Default to a warning message, override if error message is needed
    msgParams.second = CClientUIInterface::MSG_WARNING;

    // This comment is specific to MinerCoinsDialog usage of WalletModel::SendCoinsReturn.
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
        msgParams.first = tr("Deposit creation failed!");
        msgParams.second = CClientUIInterface::MSG_ERROR;
        break;
    case Bitcredit_WalletModel::TransactionCommitFailed:
        msgParams.first = tr("The deposit was rejected! This might happen if some of the coins in your wallet were already spent, such as if you used a copy of wallet.dat and coins were spent in the copy but not marked as spent here.");
        msgParams.second = CClientUIInterface::MSG_ERROR;
        break;
    case Bitcredit_WalletModel::TransactionDoubleSpending:
        msgParams.first = tr("The deposit was rejected! This might happen if some of the deposits that you already have prepared tries to spend the same input as for the deposit you just are trying to create.");
        msgParams.second = CClientUIInterface::MSG_ERROR;
        break;
    // included to prevent a compiler warning.
    case Bitcredit_WalletModel::OK:
    default:
        return;
    }

    emit message(tr("Deposit Coins"), msgParams.first, msgParams.second);
}

// Coin Control: copy label "Quantity" to clipboard
void MinerCoinsDialog::coinControlClipboardQuantity()
{
    GUIUtil::setClipboard(ui->labelCoinControlQuantity->text());
}

// Coin Control: copy label "Amount" to clipboard
void MinerCoinsDialog::coinControlClipboardAmount()
{
    GUIUtil::setClipboard(ui->labelCoinControlAmount->text().left(ui->labelCoinControlAmount->text().indexOf(" ")));
}

// Coin Control: button inputs -> show actual coin control dialog
void MinerCoinsDialog::coinControlButtonClicked()
{
    Miner_CoinControlDialog dlg;
    dlg.setModel(bitcredit_model, deposit_model);
    dlg.exec();

    // Remove entries and add new
    while(ui->entries->count()) {
        ui->entries->takeAt(0)->widget()->deleteLater();
    }
    vector<COutPoint> vCoinControl;
    vector<Credits_COutput>   vOutputs;
    Miner_CoinControlDialog::coinControl->ListSelected(vCoinControl);
    bitcredit_model->getOutputs(vCoinControl, vOutputs);

    foreach(const Credits_COutput &output, vOutputs) {
        addEntry( output.tx->vout[output.i].nValue);
    }

    coinControlUpdateLabels();
}

// Coin Control: update labels
void MinerCoinsDialog::coinControlUpdateLabels()
{
    if (!bitcredit_model || !bitcredit_model->getOptionsModel())
        return;

    // set pay amounts
    Miner_CoinControlDialog::payAmounts.clear();
    for(int i = 0; i < ui->entries->count(); ++i)
    {
        UneditableCoinsEntry *entry = qobject_cast<UneditableCoinsEntry*>(ui->entries->itemAt(i)->widget());
        if(entry)
            Miner_CoinControlDialog::payAmounts.append(entry->getValue().amount);
    }

    if (Miner_CoinControlDialog::coinControl->HasSelected())
    {
        // actual coin control calculation
        Miner_CoinControlDialog::updateLabels(bitcredit_model, this);

        // show coin control stats
        ui->widgetCoinControl->show();
    }
    else
    {
        // hide coin control stats
        ui->widgetCoinControl->hide();
    }
}
