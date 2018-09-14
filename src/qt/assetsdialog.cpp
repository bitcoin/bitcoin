// Copyright (c) 2011-2016 The Bitcoin Core developers
// Copyright (c) 2017 The Raven Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "assetsdialog.h"
#include "sendcoinsdialog.h"
#include "ui_assetsdialog.h"

#include "addresstablemodel.h"
#include "ravenunits.h"
#include "clientmodel.h"
#include "assetcontroldialog.h"
#include "guiutil.h"
#include "optionsmodel.h"
#include "platformstyle.h"
#include "sendassetsentry.h"
#include "walletmodel.h"

#include "base58.h"
#include "chainparams.h"
#include "wallet/coincontrol.h"
#include "validation.h" // mempool and minRelayTxFee
#include "ui_interface.h"
#include "txmempool.h"
#include "policy/fees.h"
#include "wallet/fees.h"
#include "createassetdialog.h"
#include "reissueassetdialog.h"

#include <QFontMetrics>
#include <QMessageBox>
#include <QScrollBar>
#include <QSettings>
#include <QTextDocument>
#include <QTimer>
#include <policy/policy.h>
#include <core_io.h>
#include <rpc/mining.h>

AssetsDialog::AssetsDialog(const PlatformStyle *_platformStyle, QWidget *parent) :
        QDialog(parent),
        ui(new Ui::AssetsDialog),
        clientModel(0),
        model(0),
        fNewRecipientAllowed(true),
        fFeeMinimized(true),
        platformStyle(_platformStyle)
{
    ui->setupUi(this);

    if (!_platformStyle->getImagesOnButtons()) {
        ui->addButton->setIcon(QIcon());
        ui->clearButton->setIcon(QIcon());
        ui->sendButton->setIcon(QIcon());
    } else {
        ui->addButton->setIcon(_platformStyle->SingleColorIcon(":/icons/add"));
        ui->clearButton->setIcon(_platformStyle->SingleColorIcon(":/icons/remove"));
        ui->sendButton->setIcon(_platformStyle->SingleColorIcon(":/icons/send"));
    }

    GUIUtil::setupAddressWidget(ui->lineEditAssetControlChange, this);

    addEntry();

    connect(ui->addButton, SIGNAL(clicked()), this, SLOT(addEntry()));
    connect(ui->clearButton, SIGNAL(clicked()), this, SLOT(clear()));

    // Coin Control
    connect(ui->pushButtonAssetControl, SIGNAL(clicked()), this, SLOT(assetControlButtonClicked()));
    connect(ui->checkBoxAssetControlChange, SIGNAL(stateChanged(int)), this, SLOT(assetControlChangeChecked(int)));
    connect(ui->lineEditAssetControlChange, SIGNAL(textEdited(const QString &)), this, SLOT(assetControlChangeEdited(const QString &)));

    // Coin Control: clipboard actions
    QAction *clipboardQuantityAction = new QAction(tr("Copy quantity"), this);
    QAction *clipboardAmountAction = new QAction(tr("Copy amount"), this);
    QAction *clipboardFeeAction = new QAction(tr("Copy fee"), this);
    QAction *clipboardAfterFeeAction = new QAction(tr("Copy after fee"), this);
    QAction *clipboardBytesAction = new QAction(tr("Copy bytes"), this);
    QAction *clipboardLowOutputAction = new QAction(tr("Copy dust"), this);
    QAction *clipboardChangeAction = new QAction(tr("Copy change"), this);
    connect(clipboardQuantityAction, SIGNAL(triggered()), this, SLOT(assetControlClipboardQuantity()));
    connect(clipboardAmountAction, SIGNAL(triggered()), this, SLOT(assetControlClipboardAmount()));
    connect(clipboardFeeAction, SIGNAL(triggered()), this, SLOT(assetControlClipboardFee()));
    connect(clipboardAfterFeeAction, SIGNAL(triggered()), this, SLOT(assetControlClipboardAfterFee()));
    connect(clipboardBytesAction, SIGNAL(triggered()), this, SLOT(assetControlClipboardBytes()));
    connect(clipboardLowOutputAction, SIGNAL(triggered()), this, SLOT(assetControlClipboardLowOutput()));
    connect(clipboardChangeAction, SIGNAL(triggered()), this, SLOT(assetControlClipboardChange()));
    ui->labelAssetControlQuantity->addAction(clipboardQuantityAction);
    ui->labelAssetControlAmount->addAction(clipboardAmountAction);
    ui->labelAssetControlFee->addAction(clipboardFeeAction);
    ui->labelAssetControlAfterFee->addAction(clipboardAfterFeeAction);
    ui->labelAssetControlBytes->addAction(clipboardBytesAction);
    ui->labelAssetControlLowOutput->addAction(clipboardLowOutputAction);
    ui->labelAssetControlChange->addAction(clipboardChangeAction);

    // init transaction fee section
    QSettings settings;
    if (!settings.contains("fFeeSectionMinimized"))
        settings.setValue("fFeeSectionMinimized", true);
    if (!settings.contains("nFeeRadio") && settings.contains("nTransactionFee") && settings.value("nTransactionFee").toLongLong() > 0) // compatibility
        settings.setValue("nFeeRadio", 1); // custom
    if (!settings.contains("nFeeRadio"))
        settings.setValue("nFeeRadio", 0); // recommended
    if (!settings.contains("nSmartFeeSliderPosition"))
        settings.setValue("nSmartFeeSliderPosition", 0);
    if (!settings.contains("nTransactionFee"))
        settings.setValue("nTransactionFee", (qint64)DEFAULT_TRANSACTION_FEE);
    if (!settings.contains("fPayOnlyMinFee"))
        settings.setValue("fPayOnlyMinFee", false);
    ui->groupFee->setId(ui->radioSmartFee, 0);
    ui->groupFee->setId(ui->radioCustomFee, 1);
    ui->groupFee->button((int)std::max(0, std::min(1, settings.value("nFeeRadio").toInt())))->setChecked(true);
    ui->customFee->setValue(settings.value("nTransactionFee").toLongLong());
    ui->checkBoxMinimumFee->setChecked(settings.value("fPayOnlyMinFee").toBool());
    minimizeFeeSection(settings.value("fFeeSectionMinimized").toBool());

    /** RVN START */
    connect(ui->createAssetButton, SIGNAL(clicked()), this, SLOT(createAssetButtonClicked()));
    connect(ui->reissueAssetButton, SIGNAL(clicked()), this, SLOT(ressieAssetButtonClicked()));
    connect(ui->refreshButton, SIGNAL(clicked()), this, SLOT(refreshButtonClicked()));
    ui->refreshButton->setIcon(platformStyle->SingleColorIcon(":/icons/refresh"));
    ui->refreshButton->setToolTip(tr("Refresh the page to display newly received assets"));
    ui->optInRBF->hide();

    // If the network is regtest. Add some helper buttons to the asset GUI
    if (Params().NetworkIDString() != "regtest") {
        ui->mineButton->hide();
        ui->mineBlocksCount->hide();
    } else {
        ui->mineButton->setText(tr("Mine Block(s)"));
        ui->mineButton->setToolTip(tr("Mine some number of blocks"));
        ui->mineBlocksCount->setToolTip(tr("The number of blocks to mine"));
        connect(ui->mineButton, SIGNAL(clicked()), this, SLOT(mineButtonClicked()));
    }
    /** RVN END */
}

void AssetsDialog::setClientModel(ClientModel *_clientModel)
{
    this->clientModel = _clientModel;

    if (_clientModel) {
        connect(_clientModel, SIGNAL(numBlocksChanged(int,QDateTime,double,bool)), this, SLOT(updateSmartFeeLabel()));
    }
}

void AssetsDialog::setModel(WalletModel *_model)
{
    this->model = _model;

    if(_model && _model->getOptionsModel())
    {
        for(int i = 0; i < ui->entries->count(); ++i)
        {
            SendAssetsEntry *entry = qobject_cast<SendAssetsEntry*>(ui->entries->itemAt(i)->widget());
            if(entry)
            {
                entry->setModel(_model);
            }
        }

        setBalance(_model->getBalance(), _model->getUnconfirmedBalance(), _model->getImmatureBalance(),
                   _model->getWatchBalance(), _model->getWatchUnconfirmedBalance(), _model->getWatchImmatureBalance());
        connect(_model, SIGNAL(balanceChanged(CAmount,CAmount,CAmount,CAmount,CAmount,CAmount)), this, SLOT(setBalance(CAmount,CAmount,CAmount,CAmount,CAmount,CAmount)));
        connect(_model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));
        updateDisplayUnit();

        // Coin Control
        connect(_model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(assetControlUpdateLabels()));
        connect(_model->getOptionsModel(), SIGNAL(coinControlFeaturesChanged(bool)), this, SLOT(assetControlFeatureChanged(bool)));


        ui->frameAssetControl->setVisible(false);
        //TODO Turn on the coin control features for assets
        ui->frameAssetControl->setVisible(_model->getOptionsModel()->getCoinControlFeatures());
        assetControlUpdateLabels();

        // fee section
        for (const int &n : confTargets) {
            ui->confTargetSelector->addItem(tr("%1 (%2 blocks)").arg(GUIUtil::formatNiceTimeOffset(n*Params().GetConsensus().nPowTargetSpacing)).arg(n));
        }
        connect(ui->confTargetSelector, SIGNAL(currentIndexChanged(int)), this, SLOT(updateSmartFeeLabel()));
        connect(ui->confTargetSelector, SIGNAL(currentIndexChanged(int)), this, SLOT(assetControlUpdateLabels()));
        connect(ui->groupFee, SIGNAL(buttonClicked(int)), this, SLOT(updateFeeSectionControls()));
        connect(ui->groupFee, SIGNAL(buttonClicked(int)), this, SLOT(assetControlUpdateLabels()));
        connect(ui->customFee, SIGNAL(valueChanged()), this, SLOT(assetControlUpdateLabels()));
        connect(ui->checkBoxMinimumFee, SIGNAL(stateChanged(int)), this, SLOT(setMinimumFee()));
        connect(ui->checkBoxMinimumFee, SIGNAL(stateChanged(int)), this, SLOT(updateFeeSectionControls()));
        connect(ui->checkBoxMinimumFee, SIGNAL(stateChanged(int)), this, SLOT(assetControlUpdateLabels()));
        connect(ui->optInRBF, SIGNAL(stateChanged(int)), this, SLOT(updateSmartFeeLabel()));
        connect(ui->optInRBF, SIGNAL(stateChanged(int)), this, SLOT(assetControlUpdateLabels()));
        ui->customFee->setSingleStep(GetRequiredFee(1000));
        updateFeeSectionControls();
        updateMinFeeLabel();
        updateSmartFeeLabel();

        // set default rbf checkbox state
        ui->optInRBF->setCheckState(model->getDefaultWalletRbf() ? Qt::Checked : Qt::Unchecked);

        // set the smartfee-sliders default value (wallets default conf.target or last stored value)
        QSettings settings;
        if (settings.value("nSmartFeeSliderPosition").toInt() != 0) {
            // migrate nSmartFeeSliderPosition to nConfTarget
            // nConfTarget is available since 0.15 (replaced nSmartFeeSliderPosition)
            int nConfirmTarget = 25 - settings.value("nSmartFeeSliderPosition").toInt(); // 25 == old slider range
            settings.setValue("nConfTarget", nConfirmTarget);
            settings.remove("nSmartFeeSliderPosition");
        }
        if (settings.value("nConfTarget").toInt() == 0)
            ui->confTargetSelector->setCurrentIndex(getIndexForConfTarget(model->getDefaultConfirmTarget()));
        else
            ui->confTargetSelector->setCurrentIndex(getIndexForConfTarget(settings.value("nConfTarget").toInt()));
    }
}

AssetsDialog::~AssetsDialog()
{
    QSettings settings;
    settings.setValue("fFeeSectionMinimized", fFeeMinimized);
    settings.setValue("nFeeRadio", ui->groupFee->checkedId());
    settings.setValue("nConfTarget", getConfTargetForIndex(ui->confTargetSelector->currentIndex()));
    settings.setValue("nTransactionFee", (qint64)ui->customFee->value());
    settings.setValue("fPayOnlyMinFee", ui->checkBoxMinimumFee->isChecked());

    delete ui;
}

void AssetsDialog::on_sendButton_clicked()
{
    if(!model || !model->getOptionsModel())
        return;

    QList<SendAssetsRecipient> recipients;
    bool valid = true;

    for(int i = 0; i < ui->entries->count(); ++i)
    {
        SendAssetsEntry *entry = qobject_cast<SendAssetsEntry*>(ui->entries->itemAt(i)->widget());
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

    fNewRecipientAllowed = false;
    WalletModel::UnlockContext ctx(model->requestUnlock());
    if(!ctx.isValid())
    {
        // Unlock wallet was cancelled
        fNewRecipientAllowed = true;
        return;
    }

    std::vector< std::pair<CAssetTransfer, std::string> >vTransfers;

    for (auto recipient : recipients) {
        vTransfers.emplace_back(std::make_pair(CAssetTransfer(recipient.assetName.toStdString(), recipient.amount), recipient.address.toStdString()));
    }

    // Always use a CCoinControl instance, use the AssetControlDialog instance if CoinControl has been enabled
    CCoinControl ctrl;
    if (model->getOptionsModel()->getCoinControlFeatures())
        ctrl = *AssetControlDialog::assetControl;

    updateAssetControlState(ctrl);

    CWalletTx tx;
    CReserveKey reservekey(model->getWallet());
    std::pair<int, std::string> error;
    CAmount nFeeRequired;

    if (!CreateTransferAssetTransaction(model->getWallet(), ctrl, vTransfers, "", error, tx, reservekey, nFeeRequired)) {
        QMessageBox msgBox;
        msgBox.setText(QString::fromStdString(error.second));
        msgBox.exec();
        return;
    }

    // Format confirmation message
    QStringList formatted;
    for (SendAssetsRecipient &rcp : recipients)
    {
        // generate bold amount string
        QString amount = "<b>" + QString::fromStdString(ValueFromAmountString(rcp.amount, 8)) + " " + rcp.assetName;
        amount.append("</b>");
        // generate monospace address string
        QString address = "<span style='font-family: monospace;'>" + rcp.address;
        address.append("</span>");

        QString recipientElement;

        if (!rcp.paymentRequest.IsInitialized()) // normal payment
        {
            if(rcp.label.length() > 0) // label with address
            {
                recipientElement = tr("%1 to %2").arg(amount, GUIUtil::HtmlEscape(rcp.label));
                recipientElement.append(QString(" (%1)").arg(address));
            }
            else // just address
            {
                recipientElement = tr("%1 to %2").arg(amount, address);
            }
        }
        else if(!rcp.authenticatedMerchant.isEmpty()) // authenticated payment request
        {
            recipientElement = tr("%1 to %2").arg(amount, GUIUtil::HtmlEscape(rcp.authenticatedMerchant));
        }
        else // unauthenticated payment request
        {
            recipientElement = tr("%1 to %2").arg(amount, address);
        }

        formatted.append(recipientElement);
    }

    QString questionString = tr("Are you sure you want to send?");
    questionString.append("<br /><br />%1");

    if(nFeeRequired > 0)
    {
        // append fee string if a fee is required
        questionString.append("<hr /><span style='color:#aa0000;'>");
        questionString.append(RavenUnits::formatHtmlWithUnit(model->getOptionsModel()->getDisplayUnit(), nFeeRequired));
        questionString.append("</span> ");
        questionString.append(tr("added as transaction fee"));

        // append transaction size
        questionString.append(" (" + QString::number((double)GetVirtualTransactionSize(tx) / 1000) + " kB)");
    }

    if (ui->optInRBF->isChecked())
    {
        questionString.append("<hr /><span>");
        questionString.append(tr("This transaction signals replaceability (optin-RBF)."));
        questionString.append("</span>");
    }

    SendConfirmationDialog confirmationDialog(tr("Confirm send assets"),
                                              questionString.arg(formatted.join("<br />")), SEND_CONFIRM_DELAY, this);
    confirmationDialog.exec();
    QMessageBox::StandardButton retval = (QMessageBox::StandardButton)confirmationDialog.result();

    if(retval != QMessageBox::Yes)
    {
        fNewRecipientAllowed = true;
        return;
    }

    // now send the prepared transaction
    WalletModel::SendCoinsReturn sendStatus = model->sendAssets(tx, recipients, reservekey);
    // process sendStatus and on error generate message shown to user
    processSendCoinsReturn(sendStatus);

    if (sendStatus.status == WalletModel::OK)
    {
        accept();
        AssetControlDialog::assetControl->UnSelectAll();
        assetControlUpdateLabels();
    }
    fNewRecipientAllowed = true;
}

void AssetsDialog::clear()
{
    // Remove entries until only one left
    while(ui->entries->count())
    {
        ui->entries->takeAt(0)->widget()->deleteLater();
    }
    addEntry();

    updateTabsAndLabels();
}

void AssetsDialog::reject()
{
    clear();
}

void AssetsDialog::accept()
{
    clear();
}

SendAssetsEntry *AssetsDialog::addEntry()
{
    LOCK(cs_main);
    std::vector<std::string> assets;
    if (model)
        GetAllMyAssets(model->getWallet(), assets);
    else // If the model isn't present. Grab the list of assets that the cache thinks you own
        GetAllMyAssetsFromCache(assets);

    QStringList list;
    bool fIsOwner = false;
    bool fIsAssetControl = false;
    if (AssetControlDialog::assetControl->HasAssetSelected()) {
        list << QString::fromStdString(AssetControlDialog::assetControl->strAssetSelected);
        fIsOwner = IsAssetNameAnOwner(AssetControlDialog::assetControl->strAssetSelected);
        fIsAssetControl = true;
    } else {
        for (auto name : assets) {
            if (!IsAssetNameAnOwner(name))
                list << QString::fromStdString(name);
        }
    }

    SendAssetsEntry *entry = new SendAssetsEntry(platformStyle, list, this);
    entry->setModel(model);
    ui->entries->addWidget(entry);
    connect(entry, SIGNAL(removeEntry(SendAssetsEntry*)), this, SLOT(removeEntry(SendAssetsEntry*)));
    connect(entry, SIGNAL(payAmountChanged()), this, SLOT(assetControlUpdateLabels()));
    connect(entry, SIGNAL(subtractFeeFromAmountChanged()), this, SLOT(assetControlUpdateLabels()));

    // Focus the field, so that entry can start immediately
    entry->clear();
    entry->setFocus();
    ui->scrollAreaWidgetContents->resize(ui->scrollAreaWidgetContents->sizeHint());
    qApp->processEvents();
    QScrollBar* bar = ui->scrollArea->verticalScrollBar();
    if(bar)
        bar->setSliderPosition(bar->maximum());

    entry->IsAssetControl(fIsAssetControl, fIsOwner);

    if (list.size() == 1)
        entry->setCurrentIndex(1);

    updateTabsAndLabels();

    return entry;
}

void AssetsDialog::updateTabsAndLabels()
{
    setupTabChain(0);
    assetControlUpdateLabels();
}

void AssetsDialog::removeEntry(SendAssetsEntry* entry)
{
    entry->hide();

    // If the last entry is about to be removed add an empty one
    if (ui->entries->count() == 1)
        addEntry();

    entry->deleteLater();

    updateTabsAndLabels();
}

QWidget *AssetsDialog::setupTabChain(QWidget *prev)
{
    for(int i = 0; i < ui->entries->count(); ++i)
    {
        SendAssetsEntry *entry = qobject_cast<SendAssetsEntry*>(ui->entries->itemAt(i)->widget());
        if(entry)
        {
            prev = entry->setupTabChain(prev);
        }
    }
    QWidget::setTabOrder(prev, ui->sendButton);
    QWidget::setTabOrder(ui->sendButton, ui->clearButton);
    QWidget::setTabOrder(ui->clearButton, ui->addButton);
    return ui->addButton;
}

void AssetsDialog::setAddress(const QString &address)
{
    SendAssetsEntry *entry = 0;
    // Replace the first entry if it is still unused
    if(ui->entries->count() == 1)
    {
        SendAssetsEntry *first = qobject_cast<SendAssetsEntry*>(ui->entries->itemAt(0)->widget());
        if(first->isClear())
        {
            entry = first;
        }
    }
    if(!entry)
    {
        entry = addEntry();
    }

    entry->setAddress(address);
}

void AssetsDialog::pasteEntry(const SendAssetsRecipient &rv)
{
    if(!fNewRecipientAllowed)
        return;

    SendAssetsEntry *entry = 0;
    // Replace the first entry if it is still unused
    if(ui->entries->count() == 1)
    {
        SendAssetsEntry *first = qobject_cast<SendAssetsEntry*>(ui->entries->itemAt(0)->widget());
        if(first->isClear())
        {
            entry = first;
        }
    }
    if(!entry)
    {
        entry = addEntry();
    }

    entry->setValue(rv);
    updateTabsAndLabels();
}

bool AssetsDialog::handlePaymentRequest(const SendAssetsRecipient &rv)
{
    // Just paste the entry, all pre-checks
    // are done in paymentserver.cpp.
    pasteEntry(rv);
    return true;
}

void AssetsDialog::setBalance(const CAmount& balance, const CAmount& unconfirmedBalance, const CAmount& immatureBalance,
                                 const CAmount& watchBalance, const CAmount& watchUnconfirmedBalance, const CAmount& watchImmatureBalance)
{
    Q_UNUSED(unconfirmedBalance);
    Q_UNUSED(immatureBalance);
    Q_UNUSED(watchBalance);
    Q_UNUSED(watchUnconfirmedBalance);
    Q_UNUSED(watchImmatureBalance);

    if(model && model->getOptionsModel())
    {
        ui->labelBalance->setText(RavenUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), balance));
    }
}

void AssetsDialog::updateDisplayUnit()
{
    setBalance(model->getBalance(), 0, 0, 0, 0, 0);
    ui->customFee->setDisplayUnit(model->getOptionsModel()->getDisplayUnit());
    updateMinFeeLabel();
    updateSmartFeeLabel();
}

void AssetsDialog::processSendCoinsReturn(const WalletModel::SendCoinsReturn &sendCoinsReturn, const QString &msgArg)
{
    QPair<QString, CClientUIInterface::MessageBoxFlags> msgParams;
    // Default to a warning message, override if error message is needed
    msgParams.second = CClientUIInterface::MSG_WARNING;

    // This comment is specific to SendCoinsDialog usage of WalletModel::SendCoinsReturn.
    // WalletModel::TransactionCommitFailed is used only in WalletModel::sendCoins()
    // all others are used only in WalletModel::prepareTransaction()
    switch(sendCoinsReturn.status)
    {
        case WalletModel::InvalidAddress:
            msgParams.first = tr("The recipient address is not valid. Please recheck.");
            break;
        case WalletModel::InvalidAmount:
            msgParams.first = tr("The amount to pay must be larger than 0.");
            break;
        case WalletModel::AmountExceedsBalance:
            msgParams.first = tr("The amount exceeds your balance.");
            break;
        case WalletModel::AmountWithFeeExceedsBalance:
            msgParams.first = tr("The total exceeds your balance when the %1 transaction fee is included.").arg(msgArg);
            break;
        case WalletModel::DuplicateAddress:
            msgParams.first = tr("Duplicate address found: addresses should only be used once each.");
            break;
        case WalletModel::TransactionCreationFailed:
            msgParams.first = tr("Transaction creation failed!");
            msgParams.second = CClientUIInterface::MSG_ERROR;
            break;
        case WalletModel::TransactionCommitFailed:
            msgParams.first = tr("The transaction was rejected with the following reason: %1").arg(sendCoinsReturn.reasonCommitFailed);
            msgParams.second = CClientUIInterface::MSG_ERROR;
            break;
        case WalletModel::AbsurdFee:
            msgParams.first = tr("A fee higher than %1 is considered an absurdly high fee.").arg(RavenUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), maxTxFee));
            break;
        case WalletModel::PaymentRequestExpired:
            msgParams.first = tr("Payment request expired.");
            msgParams.second = CClientUIInterface::MSG_ERROR;
            break;
            // included to prevent a compiler warning.
        case WalletModel::OK:
        default:
            return;
    }

    Q_EMIT message(tr("Send Coins"), msgParams.first, msgParams.second);
}

void AssetsDialog::minimizeFeeSection(bool fMinimize)
{
    ui->labelFeeMinimized->setVisible(fMinimize);
    ui->buttonChooseFee  ->setVisible(fMinimize);
    ui->buttonMinimizeFee->setVisible(!fMinimize);
    ui->frameFeeSelection->setVisible(!fMinimize);
    ui->horizontalLayoutSmartFee->setContentsMargins(0, (fMinimize ? 0 : 6), 0, 0);
    fFeeMinimized = fMinimize;
}

void AssetsDialog::on_buttonChooseFee_clicked()
{
    minimizeFeeSection(false);
}

void AssetsDialog::on_buttonMinimizeFee_clicked()
{
    updateFeeMinimizedLabel();
    minimizeFeeSection(true);
}

void AssetsDialog::setMinimumFee()
{
    ui->customFee->setValue(GetRequiredFee(1000));
}

void AssetsDialog::updateFeeSectionControls()
{
    ui->confTargetSelector      ->setEnabled(ui->radioSmartFee->isChecked());
    ui->labelSmartFee           ->setEnabled(ui->radioSmartFee->isChecked());
    ui->labelSmartFee2          ->setEnabled(ui->radioSmartFee->isChecked());
    ui->labelSmartFee3          ->setEnabled(ui->radioSmartFee->isChecked());
    ui->labelFeeEstimation      ->setEnabled(ui->radioSmartFee->isChecked());
    ui->checkBoxMinimumFee      ->setEnabled(ui->radioCustomFee->isChecked());
    ui->labelMinFeeWarning      ->setEnabled(ui->radioCustomFee->isChecked());
    ui->labelCustomPerKilobyte  ->setEnabled(ui->radioCustomFee->isChecked() && !ui->checkBoxMinimumFee->isChecked());
    ui->customFee               ->setEnabled(ui->radioCustomFee->isChecked() && !ui->checkBoxMinimumFee->isChecked());
}

void AssetsDialog::updateFeeMinimizedLabel()
{
    if(!model || !model->getOptionsModel())
        return;

    if (ui->radioSmartFee->isChecked())
        ui->labelFeeMinimized->setText(ui->labelSmartFee->text());
    else {
        ui->labelFeeMinimized->setText(RavenUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), ui->customFee->value()) + "/kB");
    }
}

void AssetsDialog::updateMinFeeLabel()
{
    if (model && model->getOptionsModel())
        ui->checkBoxMinimumFee->setText(tr("Pay only the required fee of %1").arg(
                RavenUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), GetRequiredFee(1000)) + "/kB")
        );
}

void AssetsDialog::updateAssetControlState(CCoinControl& ctrl)
{
    if (ui->radioCustomFee->isChecked()) {
        ctrl.m_feerate = CFeeRate(ui->customFee->value());
    } else {
        ctrl.m_feerate.reset();
    }
    // Avoid using global defaults when sending money from the GUI
    // Either custom fee will be used or if not selected, the confirmation target from dropdown box
    ctrl.m_confirm_target = getConfTargetForIndex(ui->confTargetSelector->currentIndex());
    ctrl.signalRbf = ui->optInRBF->isChecked();
}

void AssetsDialog::updateSmartFeeLabel()
{
    if(!model || !model->getOptionsModel())
        return;
    CCoinControl coin_control;
    updateAssetControlState(coin_control);
    coin_control.m_feerate.reset(); // Explicitly use only fee estimation rate for smart fee labels
    FeeCalculation feeCalc;
    CFeeRate feeRate = CFeeRate(GetMinimumFee(1000, coin_control, ::mempool, ::feeEstimator, &feeCalc));

    ui->labelSmartFee->setText(RavenUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), feeRate.GetFeePerK()) + "/kB");

    if (feeCalc.reason == FeeReason::FALLBACK) {
        ui->labelSmartFee2->show(); // (Smart fee not initialized yet. This usually takes a few blocks...)
        ui->labelFeeEstimation->setText("");
        ui->fallbackFeeWarningLabel->setVisible(true);
        int lightness = ui->fallbackFeeWarningLabel->palette().color(QPalette::WindowText).lightness();
        QColor warning_colour(255 - (lightness / 5), 176 - (lightness / 3), 48 - (lightness / 14));
        ui->fallbackFeeWarningLabel->setStyleSheet("QLabel { color: " + warning_colour.name() + "; }");
        ui->fallbackFeeWarningLabel->setIndent(QFontMetrics(ui->fallbackFeeWarningLabel->font()).width("x"));
    }
    else
    {
        ui->labelSmartFee2->hide();
        ui->labelFeeEstimation->setText(tr("Estimated to begin confirmation within %n block(s).", "", feeCalc.returnedTarget));
        ui->fallbackFeeWarningLabel->setVisible(false);
    }

    updateFeeMinimizedLabel();
}

// Coin Control: copy label "Quantity" to clipboard
void AssetsDialog::assetControlClipboardQuantity()
{
    GUIUtil::setClipboard(ui->labelAssetControlQuantity->text());
}

// Coin Control: copy label "Amount" to clipboard
void AssetsDialog::assetControlClipboardAmount()
{
    GUIUtil::setClipboard(ui->labelAssetControlAmount->text().left(ui->labelAssetControlAmount->text().indexOf(" ")));
}

// Coin Control: copy label "Fee" to clipboard
void AssetsDialog::assetControlClipboardFee()
{
    GUIUtil::setClipboard(ui->labelAssetControlFee->text().left(ui->labelAssetControlFee->text().indexOf(" ")).replace(ASYMP_UTF8, ""));
}

// Coin Control: copy label "After fee" to clipboard
void AssetsDialog::assetControlClipboardAfterFee()
{
    GUIUtil::setClipboard(ui->labelAssetControlAfterFee->text().left(ui->labelAssetControlAfterFee->text().indexOf(" ")).replace(ASYMP_UTF8, ""));
}

// Coin Control: copy label "Bytes" to clipboard
void AssetsDialog::assetControlClipboardBytes()
{
    GUIUtil::setClipboard(ui->labelAssetControlBytes->text().replace(ASYMP_UTF8, ""));
}

// Coin Control: copy label "Dust" to clipboard
void AssetsDialog::assetControlClipboardLowOutput()
{
    GUIUtil::setClipboard(ui->labelAssetControlLowOutput->text());
}

// Coin Control: copy label "Change" to clipboard
void AssetsDialog::assetControlClipboardChange()
{
    GUIUtil::setClipboard(ui->labelAssetControlChange->text().left(ui->labelAssetControlChange->text().indexOf(" ")).replace(ASYMP_UTF8, ""));
}

// Coin Control: settings menu - coin control enabled/disabled by user
void AssetsDialog::assetControlFeatureChanged(bool checked)
{
    ui->frameAssetControl->setVisible(checked);

    if (!checked && model) // coin control features disabled
        AssetControlDialog::assetControl->SetNull();

    assetControlUpdateLabels();
}

// Coin Control: button inputs -> show actual coin control dialog
void AssetsDialog::assetControlButtonClicked()
{
    AssetControlDialog dlg(platformStyle);
    dlg.setModel(model);
    dlg.exec();
    assetControlUpdateLabels();
    assetControlUpdateSendCoinsDialog();
}

// Coin Control: checkbox custom change address
void AssetsDialog::assetControlChangeChecked(int state)
{
    if (state == Qt::Unchecked)
    {
        AssetControlDialog::assetControl->destChange = CNoDestination();
        ui->labelAssetControlChangeLabel->clear();
    }
    else
        // use this to re-validate an already entered address
        assetControlChangeEdited(ui->lineEditAssetControlChange->text());

    ui->lineEditAssetControlChange->setEnabled((state == Qt::Checked));
}

// Coin Control: custom change address changed
void AssetsDialog::assetControlChangeEdited(const QString& text)
{
    if (model && model->getAddressTableModel())
    {
        // Default to no change address until verified
        AssetControlDialog::assetControl->destChange = CNoDestination();
        ui->labelAssetControlChangeLabel->setStyleSheet("QLabel{color:red;}");

        const CTxDestination dest = DecodeDestination(text.toStdString());

        if (text.isEmpty()) // Nothing entered
        {
            ui->labelAssetControlChangeLabel->setText("");
        }
        else if (!IsValidDestination(dest)) // Invalid address
        {
            ui->labelAssetControlChangeLabel->setText(tr("Warning: Invalid Raven address"));
        }
        else // Valid address
        {
            if (!model->IsSpendable(dest)) {
                ui->labelAssetControlChangeLabel->setText(tr("Warning: Unknown change address"));

                // confirmation dialog
                QMessageBox::StandardButton btnRetVal = QMessageBox::question(this, tr("Confirm custom change address"), tr("The address you selected for change is not part of this wallet. Any or all funds in your wallet may be sent to this address. Are you sure?"),
                                                                              QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);

                if(btnRetVal == QMessageBox::Yes)
                    AssetControlDialog::assetControl->destChange = dest;
                else
                {
                    ui->lineEditAssetControlChange->setText("");
                    ui->labelAssetControlChangeLabel->setStyleSheet("QLabel{color:black;}");
                    ui->labelAssetControlChangeLabel->setText("");
                }
            }
            else // Known change address
            {
                ui->labelAssetControlChangeLabel->setStyleSheet("QLabel{color:black;}");

                // Query label
                QString associatedLabel = model->getAddressTableModel()->labelForAddress(text);
                if (!associatedLabel.isEmpty())
                    ui->labelAssetControlChangeLabel->setText(associatedLabel);
                else
                    ui->labelAssetControlChangeLabel->setText(tr("(no label)"));

                AssetControlDialog::assetControl->destChange = dest;
            }
        }
    }
}

// Coin Control: update labels
void AssetsDialog::assetControlUpdateLabels()
{
    if (!model || !model->getOptionsModel())
        return;

    updateAssetControlState(*AssetControlDialog::assetControl);

    // set pay amounts
    AssetControlDialog::payAmounts.clear();
    AssetControlDialog::fSubtractFeeFromAmount = false;

    for(int i = 0; i < ui->entries->count(); ++i)
    {
        SendAssetsEntry *entry = qobject_cast<SendAssetsEntry*>(ui->entries->itemAt(i)->widget());
        if(entry && !entry->isHidden())
        {
            SendAssetsRecipient rcp = entry->getValue();
            AssetControlDialog::payAmounts.append(rcp.amount);
//            if (rcp.fSubtractFeeFromAmount)
//                AssetControlDialog::fSubtractFeeFromAmount = true;
        }
    }

    if (AssetControlDialog::assetControl->HasAssetSelected())
    {
        // actual coin control calculation
        AssetControlDialog::updateLabels(model, this);

        // show coin control stats
        ui->labelAssetControlAutomaticallySelected->hide();
        ui->widgetAssetControl->show();
    }
    else
    {
        // hide coin control stats
        ui->labelAssetControlAutomaticallySelected->show();
        ui->widgetAssetControl->hide();
        ui->labelAssetControlInsuffFunds->hide();
    }
}

/** RVN START */
void AssetsDialog::createAssetButtonClicked()
{
    WalletModel::UnlockContext ctx(model->requestUnlock());
    if(!ctx.isValid())
    {
        // Unlock wallet was cancelled
        return;
    }

    CreateAssetDialog dlg(platformStyle, 0, model, clientModel);
    dlg.setModel(model);
    dlg.setClientModel(clientModel);
    dlg.exec();
}

void AssetsDialog::ressieAssetButtonClicked()
{
    WalletModel::UnlockContext ctx(model->requestUnlock());
    if(!ctx.isValid())
    {
        // Unlock wallet was cancelled
        return;
    }

    ReissueAssetDialog dlg(platformStyle, 0, model, clientModel);
    dlg.setModel(model);
    dlg.setClientModel(clientModel);
    dlg.exec();
}

void AssetsDialog::refreshButtonClicked()
{
    for(int i = 0; i < ui->entries->count(); ++i)
    {
        SendAssetsEntry *entry = qobject_cast<SendAssetsEntry*>(ui->entries->itemAt(i)->widget());
        if(entry)
        {
            removeEntry(entry);
        }
    }

    addEntry();
}

void AssetsDialog::mineButtonClicked()
{

    int num_generate = ui->mineBlocksCount->value();
    uint64_t max_tries = 1000000;

    std::shared_ptr<CReserveScript> coinbase_script;
    model->getWallet()->GetScriptForMining(coinbase_script);

    // If the keypool is exhausted, no script is returned at all.  Catch this.
    if (!coinbase_script) {
        return;
    }

    //throw an error if no script was provided
    if (coinbase_script->reserveScript.empty()) {
        return;
    }

    generateBlocks(coinbase_script, num_generate, max_tries, true);
}

void AssetsDialog::assetControlUpdateSendCoinsDialog()
{
    AssetControlDialog::assetControl->HasAssetSelected();
    for(int i = 0; i < ui->entries->count(); ++i)
    {
        SendAssetsEntry *entry = qobject_cast<SendAssetsEntry*>(ui->entries->itemAt(i)->widget());
        if(entry)
        {
            removeEntry(entry);
        }
    }

    addEntry();

}
/** RVN END */
