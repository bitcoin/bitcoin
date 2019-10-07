// Copyright (c) 2011-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/sendcoinsdialog.h>
#include <qt/forms/ui_sendcoinsdialog.h>

#include <qt/addresstablemodel.h>
#include <qt/bitcoinunits.h>
#include <qt/clientmodel.h>
#include <qt/coincontroldialog.h>
#include <qt/guiutil.h>
#include <qt/optionsmodel.h>
#include <qt/platformstyle.h>
#include <qt/sendcoinsentry.h>

#include <base58.h>
#include <chainparams.h>
#include <consensus/tx_verify.h>
#include <policy/fees.h>
#include <txmempool.h>
#include <validation.h> // mempool and minRelayTxFee
#include <ui_interface.h>
#include <wallet/coincontrol.h>
#include <wallet/fees.h>

#include <QFontMetrics>
#include <QLineEdit>
#include <QScrollBar>
#include <QSettings>
#include <QStringList>
#include <QTextDocument>

static const std::array<int, 9> confTargets = { {2, 4, 6, 12, 24, 48, 144, 504, 1008} };
int getConfTargetForIndex(int index) {
    if (index+1 > static_cast<int>(confTargets.size())) {
        return confTargets.back();
    }
    if (index < 0) {
        return confTargets[0];
    }
    return confTargets[index];
}
int getIndexForConfTarget(int target) {
    for (unsigned int i = 0; i < confTargets.size(); i++) {
        if (confTargets[i] >= target) {
            return i;
        }
    }
    return confTargets.size() - 1;
}

SendCoinsDialog::SendCoinsDialog(const PlatformStyle *_platformStyle, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SendCoinsDialog),
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
        ui->genBindDataButton->setIcon(QIcon());
    } else {
        ui->addButton->setIcon(_platformStyle->SingleColorIcon(":/icons/add"));
        ui->clearButton->setIcon(_platformStyle->SingleColorIcon(":/icons/remove"));
        ui->sendButton->setIcon(_platformStyle->SingleColorIcon(":/icons/send"));
        ui->genBindDataButton->setIcon(_platformStyle->SingleColorIcon(":/icons/key"));
    }

    // Operate method
    ui->operateMethodComboBox->addItem(tr("Pay to"), (int)PayOperateMethod::Pay);
    ui->operateMethodComboBox->addItem(tr("Loan to"), (int)PayOperateMethod::LoanTo);
    ui->operateMethodComboBox->addItem(tr("Bind to"), (int)PayOperateMethod::BindPlotter);
    addEntry();

    GUIUtil::setupAddressWidget(ui->lineEditCoinControlChange, this);

    connect(ui->addButton, SIGNAL(clicked()), this, SLOT(addEntry()));
    connect(ui->clearButton, SIGNAL(clicked()), this, SLOT(clear()));
    connect(ui->operateMethodComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(onOperateMethodComboBoxChanged(int)));

    // Coin Control
    connect(ui->pushButtonCoinControl, SIGNAL(clicked()), this, SLOT(coinControlButtonClicked()));
    connect(ui->checkBoxCoinControlChange, SIGNAL(stateChanged(int)), this, SLOT(coinControlChangeChecked(int)));
    connect(ui->lineEditCoinControlChange, SIGNAL(textEdited(const QString &)), this, SLOT(coinControlChangeEdited(const QString &)));

    // Coin Control: clipboard actions
    QAction *clipboardQuantityAction = new QAction(tr("Copy quantity"), this);
    QAction *clipboardAmountAction = new QAction(tr("Copy amount"), this);
    QAction *clipboardFeeAction = new QAction(tr("Copy fee"), this);
    QAction *clipboardAfterFeeAction = new QAction(tr("Copy after fee"), this);
    QAction *clipboardBytesAction = new QAction(tr("Copy bytes"), this);
    QAction *clipboardLowOutputAction = new QAction(tr("Copy dust"), this);
    QAction *clipboardChangeAction = new QAction(tr("Copy change"), this);
    connect(clipboardQuantityAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardQuantity()));
    connect(clipboardAmountAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardAmount()));
    connect(clipboardFeeAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardFee()));
    connect(clipboardAfterFeeAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardAfterFee()));
    connect(clipboardBytesAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardBytes()));
    connect(clipboardLowOutputAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardLowOutput()));
    connect(clipboardChangeAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardChange()));
    ui->labelCoinControlQuantity->addAction(clipboardQuantityAction);
    ui->labelCoinControlAmount->addAction(clipboardAmountAction);
    ui->labelCoinControlFee->addAction(clipboardFeeAction);
    ui->labelCoinControlAfterFee->addAction(clipboardAfterFeeAction);
    ui->labelCoinControlBytes->addAction(clipboardBytesAction);
    ui->labelCoinControlLowOutput->addAction(clipboardLowOutputAction);
    ui->labelCoinControlChange->addAction(clipboardChangeAction);

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
}

void SendCoinsDialog::setClientModel(ClientModel *_clientModel)
{
    this->clientModel = _clientModel;

    if (_clientModel) {
        connect(_clientModel, SIGNAL(numBlocksChanged(int,QDateTime,double,bool)), this, SLOT(updateSmartFeeLabel()));

    #ifdef ENABLE_WALLET
        connect(_clientModel, SIGNAL(walletChanged(CWallet*)), this, SLOT(currentWalletPrimaryAddressChanged(CWallet*)));
        connect(_clientModel, SIGNAL(walletPrimaryAddressChanged(CWallet*)), this, SLOT(currentWalletPrimaryAddressChanged(CWallet*)));
    #endif
    }
}

void SendCoinsDialog::setModel(WalletModel *_model)
{
    this->model = _model;

    if(_model && _model->getOptionsModel())
    {
        for(int i = 0; i < ui->entries->count(); ++i)
        {
            SendCoinsEntry *entry = qobject_cast<SendCoinsEntry*>(ui->entries->itemAt(i)->widget());
            if(entry)
            {
                entry->setModel(_model);
            }
        }

        setBalance(_model->getBalance(), _model->getUnconfirmedBalance(), _model->getImmatureBalance(),
                   _model->getLoanBalance(), _model->getBorrowBalance(), _model->getLockedBalance(), 
                   _model->getWatchBalance(), _model->getWatchUnconfirmedBalance(), _model->getWatchImmatureBalance(),
                   _model->getWatchLoanBalance(), _model->getWatchBorrowBalance(), _model->getWatchLockedBalance());
        connect(_model, SIGNAL(balanceChanged(CAmount,CAmount,CAmount,CAmount,CAmount,CAmount,CAmount,CAmount,CAmount,CAmount,CAmount,CAmount)),
            this, SLOT(setBalance(CAmount,CAmount,CAmount,CAmount,CAmount,CAmount,CAmount,CAmount,CAmount,CAmount,CAmount,CAmount)));
        connect(_model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));
        updateDisplayUnit();

        // Coin Control
        connect(_model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(coinControlUpdateLabels()));
        connect(_model->getOptionsModel(), SIGNAL(coinControlFeaturesChanged(bool)), this, SLOT(coinControlFeatureChanged(bool)));
        ui->frameCoinControl->setVisible(_model->getOptionsModel()->getCoinControlFeatures() && getPayOperateMethod() != PayOperateMethod::Pay);
        coinControlUpdateLabels();

        // fee section
        for (const int n : confTargets) {
            ui->confTargetSelector->addItem(tr("%1 (%2 blocks)").arg(GUIUtil::formatNiceTimeOffset(n*Params().GetConsensus().BHDIP008TargetSpacing)).arg(n));
        }
        connect(ui->confTargetSelector, SIGNAL(currentIndexChanged(int)), this, SLOT(updateSmartFeeLabel()));
        connect(ui->confTargetSelector, SIGNAL(currentIndexChanged(int)), this, SLOT(coinControlUpdateLabels()));
        connect(ui->groupFee, SIGNAL(buttonClicked(int)), this, SLOT(updateFeeSectionControls()));
        connect(ui->groupFee, SIGNAL(buttonClicked(int)), this, SLOT(coinControlUpdateLabels()));
        connect(ui->customFee, SIGNAL(valueChanged()), this, SLOT(coinControlUpdateLabels()));
        connect(ui->checkBoxMinimumFee, SIGNAL(stateChanged(int)), this, SLOT(setMinimumFee()));
        connect(ui->checkBoxMinimumFee, SIGNAL(stateChanged(int)), this, SLOT(updateFeeSectionControls()));
        connect(ui->checkBoxMinimumFee, SIGNAL(stateChanged(int)), this, SLOT(coinControlUpdateLabels()));
        connect(ui->optInRBF, SIGNAL(stateChanged(int)), this, SLOT(updateSmartFeeLabel()));
        connect(ui->optInRBF, SIGNAL(stateChanged(int)), this, SLOT(coinControlUpdateLabels()));
        ui->customFee->setSingleStep(GetRequiredFee(1000));
        updateFeeSectionControls();
        updateMinFeeLabel();
        updateSmartFeeLabel();

        // set default rbf checkbox state
        ui->optInRBF->setCheckState(Qt::Checked);

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

        // Set default pay method and force trigger update status
        ui->operateMethodComboBox->setCurrentIndex(0);
        QMetaObject::invokeMethod(ui->operateMethodComboBox, "currentIndexChanged", Qt::QueuedConnection,
            Q_ARG(int, ui->operateMethodComboBox->currentIndex()));

    }
}

SendCoinsDialog::~SendCoinsDialog()
{
    QSettings settings;
    settings.setValue("fFeeSectionMinimized", fFeeMinimized);
    settings.setValue("nFeeRadio", ui->groupFee->checkedId());
    settings.setValue("nConfTarget", getConfTargetForIndex(ui->confTargetSelector->currentIndex()));
    settings.setValue("nTransactionFee", (qint64)ui->customFee->value());
    settings.setValue("fPayOnlyMinFee", ui->checkBoxMinimumFee->isChecked());

    delete ui;
}

#ifdef ENABLE_WALLET
void SendCoinsDialog::currentWalletPrimaryAddressChanged(CWallet *wallet)
{
    QMetaObject::invokeMethod(ui->operateMethodComboBox, "currentIndexChanged", Qt::QueuedConnection,
        Q_ARG(int, ui->operateMethodComboBox->currentIndex()));
}
#endif

void SendCoinsDialog::onOperateMethodComboBoxChanged(int index)
{
    if(!model || !model->getOptionsModel())
        return;

    if (index >= 0) {
        int opMethodValue = ui->operateMethodComboBox->itemData(index).toInt();
        switch ((PayOperateMethod)opMethodValue)
        {
        case PayOperateMethod::LoanTo:
        case PayOperateMethod::BindPlotter:
            ui->clearButton->setVisible(false);
            ui->addButton->setVisible(false);
            ui->frameCoinControl->setVisible(false);
            CoinControlDialog::coinControl()->coinPickPolicy = CoinPickPolicy::IncludeIfSet;
            CoinControlDialog::coinControl()->destPick = CoinControlDialog::coinControl()->destChange = model->getWallet()->GetPrimaryDestination();
            break;
        default: // Normal pay
            ui->clearButton->setVisible(true);
            ui->addButton->setVisible(true);
            CoinControlDialog::coinControl()->destChange = CNoDestination();
            CoinControlDialog::coinControl()->destPick = CNoDestination();
            ui->frameCoinControl->setVisible(model->getOptionsModel()->getCoinControlFeatures());
            coinControlChangeEdited(ui->lineEditCoinControlChange->text());
            break;
        }

        ui->frameFee->setVisible((PayOperateMethod)opMethodValue != PayOperateMethod::BindPlotter);
        ui->genBindDataButton->setVisible((PayOperateMethod)opMethodValue == PayOperateMethod::BindPlotter);

        clear();
        setBalance(model->getBalance(), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    }

}

void SendCoinsDialog::on_sendButton_clicked()
{
    if(!model || !model->getOptionsModel())
        return;

    PayOperateMethod operateMethod = getPayOperateMethod();
    QList<SendCoinsRecipient> recipients;
    bool valid = true;

    for(int i = 0; i < ui->entries->count(); ++i)
    {
        SendCoinsEntry *entry = qobject_cast<SendCoinsEntry*>(ui->entries->itemAt(i)->widget());
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

    if (!valid || recipients.isEmpty())
        return;

    fNewRecipientAllowed = false;
    WalletModel::UnlockContext ctx(model->requestUnlock());
    if (!ctx.isValid())
    {
        // Unlock wallet was cancelled
        fNewRecipientAllowed = true;
        return;
    }

    // prepare transaction for getting txFee earlier
    WalletModelTransaction currentTransaction(recipients);
    WalletModel::SendCoinsReturn prepareStatus;

    const Consensus::Params& params = Params().GetConsensus();
    const int nSpendHeight = GetSpendHeight(*pcoinsTip);

    int bindLimitHeight = 0;

    // Always use a CCoinControl instance, use the CoinControlDialog instance if CoinControl has been enabled
    CCoinControl ctrl;
    switch (operateMethod)
    {
    case PayOperateMethod::LoanTo:
    case PayOperateMethod::BindPlotter:
        if (recipients.size() != 1 || recipients[0].paymentRequest.IsInitialized())
            return;
        ctrl.coinPickPolicy = CoinControlDialog::coinControl()->coinPickPolicy;
        if (operateMethod == PayOperateMethod::LoanTo) {
            ctrl.destPick = ctrl.destChange = CoinControlDialog::coinControl()->destPick;
            updateCoinControlState(ctrl);
        } else {
            ctrl.signalRbf = false;
            ctrl.destPick = ctrl.destChange = DecodeDestination(recipients[0].address.toStdString());

            // Fixed bind plotter fee
            LOCK(cs_main);
            if (nSpendHeight >= params.BHDIP006CheckRelayHeight) {
                ctrl.m_fee_mode = FeeEstimateMode::FIXED;
                ctrl.fixedFee = PROTOCOL_BINDPLOTTER_MINFEE;
            }
            if (nSpendHeight >= params.BHDIP006LimitBindPlotterHeight) {
                uint64_t plotterId;
                if (recipients[0].plotterPassphrase.size() == PROTOCOL_BINDPLOTTER_SCRIPTSIZE * 2 && IsHex(recipients[0].plotterPassphrase.toStdString())) {
                    std::vector<unsigned char> bindData(ParseHex(recipients[0].plotterPassphrase.toStdString()));
                    plotterId = GetBindPlotterIdFromScript(CScript(bindData.cbegin(), bindData.cend()));
                } else {
                    plotterId = PocLegacy::GeneratePlotterId(recipients[0].plotterPassphrase.toStdString());
                }
                if (plotterId == 0) {
                    fNewRecipientAllowed = true;
                    QMessageBox msgBox(QMessageBox::Warning, tr("Bind plotter data"), tr("Invalid bind plotter data!"), QMessageBox::Close, this);
                    msgBox.exec();
                    return;
                }

                const CBindPlotterInfo lastBindInfo = pcoinsTip->GetLastBindPlotterInfo(plotterId);
                if (!lastBindInfo.outpoint.IsNull()) {
                    bindLimitHeight = Consensus::GetBindPlotterLimitHeight(nSpendHeight, lastBindInfo, params);
                    if (nSpendHeight < bindLimitHeight) {
                        CAmount diffReward = (GetBlockSubsidy(nSpendHeight, params) * (params.BHDIP001FundRoyaltyForLowMortgage - params.BHDIP001FundRoyaltyForFullMortgage)) / 1000;
                        if (diffReward > 0) {
                            ctrl.m_fee_mode = FeeEstimateMode::FIXED;
                            ctrl.fixedFee = std::max(ctrl.fixedFee, diffReward + PROTOCOL_BINDPLOTTER_MINFEE);

                            QString information = tr("This binding operation triggers anti-cheating mechanism and therefore requires a large transaction fee %1.")
                                .arg("<span style='color:#aa0000;'>" + BitcoinUnits::formatHtmlWithUnit(model->getOptionsModel()->getDisplayUnit(), ctrl.fixedFee) + "</span>");
                            QMessageBox msgBox(QMessageBox::Warning, tr("Confirm bind plotter"), information, QMessageBox::Ok, this);
                            msgBox.exec();
                        }
                    }
                }
            }
        }
        break;
    default:
        if (model->getOptionsModel()->getCoinControlFeatures())
            ctrl = *CoinControlDialog::coinControl();
        updateCoinControlState(ctrl);
        if (!txCustomText.isEmpty())
            currentTransaction.getTransaction()->mapValue["tx_text"] = txCustomText.toStdString();
        break;
    }


    prepareStatus = model->prepareTransaction(currentTransaction, ctrl, operateMethod);

    // Check special tx
    CDatacarrierPayloadRef payload;
    if (prepareStatus.status == WalletModel::OK) {
        LOCK(cs_main);
        if (operateMethod == PayOperateMethod::BindPlotter) {
            payload = ExtractTransactionDatacarrier(*currentTransaction.getTransaction()->tx, nSpendHeight);
            if (!payload || payload->type != DATACARRIER_TYPE_BINDPLOTTER) {
                fNewRecipientAllowed = true;
                QMessageBox msgBox(QMessageBox::Warning, tr("Bind plotter data"), tr("Invalid bind plotter data!"), QMessageBox::Close, this);
                msgBox.exec();
                return;
            }

            if (pcoinsTip->HaveActiveBindPlotter(ExtractAccountID(ctrl.destPick), BindPlotterPayload::As(payload)->GetId()))
                prepareStatus = WalletModel::SendCoinsReturn(WalletModel::BindPlotterExist,
                    QString::number(BindPlotterPayload::As(payload)->GetId()) + "\n" + QString::fromStdString(EncodeDestination(ctrl.destPick)));
        } else if (operateMethod == PayOperateMethod::LoanTo) {
            payload = ExtractTransactionDatacarrier(*currentTransaction.getTransaction()->tx, nSpendHeight);
            if (!payload || payload->type != DATACARRIER_TYPE_RENTAL) {
                prepareStatus = WalletModel::SendCoinsReturn(WalletModel::TransactionCreationFailed);
            }
        } else {
            payload = ExtractTransactionDatacarrierUnlimit(*currentTransaction.getTransaction()->tx, nSpendHeight);
            if (!txCustomText.isEmpty() && (!payload || payload->type != DATACARRIER_TYPE_TEXT)) {
                prepareStatus = WalletModel::SendCoinsReturn(WalletModel::TransactionCreationFailed);
            }
        }
    }

    // process prepareStatus and on error generate message shown to user
    processSendCoinsReturn(prepareStatus,
        BitcoinUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), currentTransaction.getTransactionFee()));

    if(prepareStatus.status != WalletModel::OK) {
        fNewRecipientAllowed = true;
        return;
    }

    CAmount txFee = currentTransaction.getTransactionFee();

    // Format confirmation message
    QStringList formatted;
    for (const SendCoinsRecipient &rcp : currentTransaction.getRecipients())
    {
        // generate bold amount string
        QString amount = "<b>" + BitcoinUnits::formatHtmlWithUnit(model->getOptionsModel()->getDisplayUnit(), rcp.amount);
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

    QString titleString, questionString;
    switch (operateMethod)
    {
    case PayOperateMethod::LoanTo:
        assert(payload && payload->type == DATACARRIER_TYPE_RENTAL);
        titleString = tr("Confirm loan to");
        questionString = "<span style='color:#aa0000;'><b>" + tr("Are you sure you want loan?") + "</b></span>";
        break;
    case PayOperateMethod::BindPlotter:
        assert(payload && payload->type == DATACARRIER_TYPE_BINDPLOTTER);
        titleString = tr("Confirm bind plotter");
        questionString = "<span style='color:#aa0000;'><b>" + tr("Are you sure you want to bind plotter?") + "</b></span>";
        {
            const SendCoinsRecipient &rcp = recipients[0];
            QString amount = "<b>" + BitcoinUnits::formatHtmlWithUnit(model->getOptionsModel()->getDisplayUnit(), rcp.amount) + "</b>";
            QString plotterId = "<b>" + QString::number(BindPlotterPayload::As(payload)->GetId()) + "</b>";
            QString address;
            if (rcp.label.length() > 0)
                address = GUIUtil::HtmlEscape(rcp.label) + "(<span style='font-family: monospace;'>" + rcp.address + "</span>)";
            else
                address = "<span style='font-family: monospace;'>" + rcp.address + "</span>";

            formatted.clear();
            formatted.append(tr("%1 bind to %2").arg(plotterId, address));
            formatted.append("");
            formatted.append(tr("The operation will lock %1 in %2.").arg(amount, address));
        }
        break;
    default:
        titleString = tr("Confirm send coins");
        questionString = "<span style='color:#aa0000;'><b>" + tr("Are you sure you want to send?") + "<b></span>";
        if (payload && payload->type == DATACARRIER_TYPE_TEXT) {
            formatted.append("");
            formatted.append(QString::fromStdString(TextPayload::As(payload)->GetText()));
        }
        break;
    }
    questionString.append("<br /><br />%1");

    if (txFee > 0)
    {
        questionString.append("<hr />");
        // append fee string if a fee is required
        questionString.append("<span style='color:#aa0000;'>").append(BitcoinUnits::formatHtmlWithUnit(model->getOptionsModel()->getDisplayUnit(), txFee)).append("</span>");
        // append transaction size
        questionString.append(" ").append(tr("added as transaction fee")).append(" (" + QString::number((double)currentTransaction.getTransactionSize() / 1000) + " kB)");
        // append
        if (operateMethod == PayOperateMethod::BindPlotter && nSpendHeight < bindLimitHeight && txFee > PROTOCOL_BINDPLOTTER_MINFEE) {
            questionString.append("<br />").append(tr("%1 of this transaction fee is a package reward for miners, and %2 is directly destroyed.").
                arg("<span style='color:#aa0000;'>" + BitcoinUnits::formatHtmlWithUnit(model->getOptionsModel()->getDisplayUnit(), PROTOCOL_BINDPLOTTER_MINFEE) + "</span>",
                    "<span style='color:#aa0000;'>" + BitcoinUnits::formatHtmlWithUnit(model->getOptionsModel()->getDisplayUnit(), txFee - PROTOCOL_BINDPLOTTER_MINFEE) + "</span>"));
            questionString.append("<br />").
                append("<span style='color:#aa0000;'>").
                append(tr("This binding operation triggers anti-cheating mechanism and therefore requires a large transaction fee %1.").
                    arg(BitcoinUnits::formatHtmlWithUnit(model->getOptionsModel()->getDisplayUnit(), txFee))).
                append("</span>");
            questionString.append("<br />").
                append(tr("%1 small bind plotter fee active on %2 block height (%3 blocks after, about %4 minute).").
                        arg("<b>" + QString::number(BindPlotterPayload::As(payload)->GetId()) + "</b>",
                            QString::number(bindLimitHeight),
                            QString::number(bindLimitHeight - nSpendHeight),
                            QString::number((bindLimitHeight - nSpendHeight) * Consensus::GetTargetSpacing(nSpendHeight, params) / 60)));
        }
    }

    // add total amount in all subdivision units
    questionString.append("<hr />");
    CAmount totalAmount = currentTransaction.getTotalTransactionAmount() + txFee;
    QStringList alternativeUnits;
    for (BitcoinUnits::Unit u : BitcoinUnits::availableUnits())
    {
        if(u != model->getOptionsModel()->getDisplayUnit())
            alternativeUnits.append(BitcoinUnits::formatHtmlWithUnit(u, totalAmount));
    }
    questionString.append(tr("Total Amount %1")
        .arg(BitcoinUnits::formatHtmlWithUnit(model->getOptionsModel()->getDisplayUnit(), totalAmount)));
    questionString.append(QString("<span style='font-size:10pt;font-weight:normal;'><br />(=%1)</span>")
        .arg(alternativeUnits.join(" " + tr("or") + "<br />")));

    if (ui->frameFee->isVisible()) {
        questionString.append("<hr /><span>");
        if (ui->optInRBF->isChecked()) {
            questionString.append(tr("You can increase the fee later (signals Replace-By-Fee, BIP-125)."));
        } else {
            questionString.append(tr("Not signalling Replace-By-Fee, BIP-125."));
        }
        questionString.append("</span>");
    }

    SendConfirmationDialog confirmationDialog(titleString, questionString.arg(formatted.join("<br />")), SEND_CONFIRM_DELAY, this);
    confirmationDialog.exec();
    QMessageBox::StandardButton retval = (QMessageBox::StandardButton)confirmationDialog.result();

    if(retval != QMessageBox::Yes)
    {
        fNewRecipientAllowed = true;
        return;
    }

    // now send the prepared transaction
    WalletModel::SendCoinsReturn sendStatus = model->sendCoins(currentTransaction, operateMethod);
    // process sendStatus and on error generate message shown to user
    processSendCoinsReturn(sendStatus);

    if (sendStatus.status == WalletModel::OK)
    {
        accept();
        CoinControlDialog::coinControl()->UnSelectAll();
        coinControlUpdateLabels();
    }
    fNewRecipientAllowed = true;
}

void SendCoinsDialog::on_genBindDataButton_clicked()
{
    if (!model || !model->getOptionsModel())
        return;
    if (getPayOperateMethod() != PayOperateMethod::BindPlotter || ui->entries->count() != 1)
        return;

    SendCoinsEntry *entry = qobject_cast<SendCoinsEntry*>(ui->entries->itemAt(0)->widget());
    if (!entry->validate())
        return;
    const SendCoinsRecipient recipient = entry->getValue();

    if (recipient.plotterPassphrase.size() == PROTOCOL_BINDPLOTTER_SCRIPTSIZE * 2 && IsHex(recipient.plotterPassphrase.toStdString())) {
        QMessageBox msgBox(QMessageBox::Warning, tr("Bind plotter data"), tr("This string look like signatured bind plotter data!"), QMessageBox::Ignore|QMessageBox::Cancel, this);
        if (msgBox.exec() == QMessageBox::Cancel)
            return;
    }

    LOCK(cs_main);
    const int nSpendHeight = GetSpendHeight(*pcoinsTip);
    CTxDestination bindToDest = DecodeDestination(recipient.address.toStdString());
    int activeHeight = std::max(nSpendHeight - 1, Params().GetConsensus().BHDIP006Height);
    CScript script = GetBindPlotterScriptForDestination(bindToDest, recipient.plotterPassphrase.toStdString(), activeHeight + recipient.plotterDataValidHeight);
    if (script.empty())
        return;
    // Check
    CMutableTransaction dummyTx;
    dummyTx.nVersion = CTransaction::UNIFORM_VERSION;
    dummyTx.vin.push_back(CTxIn());
    dummyTx.vout.push_back(CTxOut(PROTOCOL_BINDPLOTTER_LOCKAMOUNT, GetScriptForDestination(bindToDest)));
    dummyTx.vout.push_back(CTxOut(0, script));
    CDatacarrierPayloadRef payload = ExtractTransactionDatacarrier(CTransaction(dummyTx), activeHeight);
    if (!payload || payload->type != DATACARRIER_TYPE_BINDPLOTTER)
        return;

    // format
    QString address;
    if (recipient.label.length() > 0)
        address = GUIUtil::HtmlEscape(recipient.label) + "(<span style='font-family: monospace;'>" + recipient.address + "</span>)";
    else
        address = "<span style='font-family: monospace;'>" + recipient.address + "</span>";
    address = "<b>" + address + "</b>";
    QString plotterId = "<b>" + QString::number(BindPlotterPayload::As(payload)->GetId()) + "</b>";

    QString information;
    information += tr("%1 bind to %2").arg(plotterId, address) + "<br /><br />";
    information += tr("You can copy and send below signature bind data to %1 owner, and let the bind active:").arg(address);
    information += "<hr />";
    information += "<p>";
    information += QString::fromStdString(HexStr(script.begin(), script.end())).insert(144, "<br />").insert(72, "<br />");
    information += "</p>";

    QMessageBox msgBox(QMessageBox::Information, tr("Bind plotter data"), information, QMessageBox::Ok|QMessageBox::Close, this);
    msgBox.button(QMessageBox::Ok)->setText(tr("Copy bind data"));
    if (msgBox.exec() == QMessageBox::Ok) {
        GUIUtil::setClipboard(QString::fromStdString(HexStr(script.begin(), script.end())));
    }
}

void SendCoinsDialog::clear()
{
    // Remove entries until only one left
    while(ui->entries->count())
    {
        ui->entries->takeAt(0)->widget()->deleteLater();
    }
    addEntry();

    updateTabsAndLabels();

    txCustomText.clear();
}

void SendCoinsDialog::reject()
{
    clear();
}

void SendCoinsDialog::accept()
{
    clear();
}

SendCoinsEntry *SendCoinsDialog::addEntry()
{
    SendCoinsEntry *entry = new SendCoinsEntry(getPayOperateMethod(), platformStyle, this);
    entry->setModel(model);
    ui->entries->addWidget(entry);
    connect(entry, SIGNAL(removeEntry(SendCoinsEntry*)), this, SLOT(removeEntry(SendCoinsEntry*)));
    connect(entry, SIGNAL(useAvailableBalance(SendCoinsEntry*)), this, SLOT(useAvailableBalance(SendCoinsEntry*)));
    connect(entry, SIGNAL(payAmountChanged()), this, SLOT(coinControlUpdateLabels()));
    connect(entry, SIGNAL(subtractFeeFromAmountChanged()), this, SLOT(coinControlUpdateLabels()));

    // Focus the field, so that entry can start immediately
    entry->clear();
    entry->setFocus();
    ui->scrollAreaWidgetContents->resize(ui->scrollAreaWidgetContents->sizeHint());
    qApp->processEvents();
    QScrollBar* bar = ui->scrollArea->verticalScrollBar();
    if(bar)
        bar->setSliderPosition(bar->maximum());

    updateTabsAndLabels();
    return entry;
}

void SendCoinsDialog::updateTabsAndLabels()
{
    setupTabChain(0);
    coinControlUpdateLabels();
}

void SendCoinsDialog::removeEntry(SendCoinsEntry* entry)
{
    entry->hide();

    // If the last entry is about to be removed add an empty one
    if (ui->entries->count() == 1)
        addEntry();

    entry->deleteLater();

    updateTabsAndLabels();
}

QWidget *SendCoinsDialog::setupTabChain(QWidget *prev)
{
    for(int i = 0; i < ui->entries->count(); ++i)
    {
        SendCoinsEntry *entry = qobject_cast<SendCoinsEntry*>(ui->entries->itemAt(i)->widget());
        if(entry)
        {
            prev = entry->setupTabChain(prev);
        }
    }
    QWidget::setTabOrder(prev, ui->sendButton);
    QWidget::setTabOrder(ui->sendButton, ui->genBindDataButton);
    QWidget::setTabOrder(ui->genBindDataButton, ui->clearButton);
    QWidget::setTabOrder(ui->clearButton, ui->addButton);
    return ui->addButton;
}

void SendCoinsDialog::setAddress(const QString &address)
{
    SendCoinsEntry *entry = 0;
    // Replace the first entry if it is still unused
    if(ui->entries->count() == 1)
    {
        SendCoinsEntry *first = qobject_cast<SendCoinsEntry*>(ui->entries->itemAt(0)->widget());
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

void SendCoinsDialog::pasteEntry(const SendCoinsRecipient &rv)
{
    if(!fNewRecipientAllowed)
        return;

    SendCoinsEntry *entry = 0;
    // Replace the first entry if it is still unused
    if(ui->entries->count() == 1)
    {
        SendCoinsEntry *first = qobject_cast<SendCoinsEntry*>(ui->entries->itemAt(0)->widget());
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

bool SendCoinsDialog::handlePaymentRequest(const SendCoinsRecipient &rv)
{
    // Just paste the entry, all pre-checks
    // are done in paymentserver.cpp.
    pasteEntry(rv);
    return true;
}

void SendCoinsDialog::setBalance(const CAmount& balance, const CAmount& unconfirmedBalance, const CAmount& immatureBalance,
                                 const CAmount& loanBalance, const CAmount& borrowBalance, const CAmount& lockedBalance,
                                 const CAmount& watchBalance, const CAmount& watchUnconfBalance, const CAmount& watchImmatureBalance,
                                 const CAmount& watchLoanBalance, const CAmount& watchBorrowBalance, const CAmount& watchLockedBalance)
{
    Q_UNUSED(unconfirmedBalance);
    Q_UNUSED(immatureBalance);
    Q_UNUSED(loanBalance);
    Q_UNUSED(borrowBalance);
    Q_UNUSED(lockedBalance);
    Q_UNUSED(watchBalance);
    Q_UNUSED(watchUnconfBalance);
    Q_UNUSED(watchImmatureBalance);
    Q_UNUSED(watchLoanBalance);
    Q_UNUSED(watchBorrowBalance);
    Q_UNUSED(watchLockedBalance);

    if(model && model->getOptionsModel())
    {
        if (getPayOperateMethod() != PayOperateMethod::Pay) {
            ui->labelBalance->setText(BitcoinUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), model->getBalance(CoinControlDialog::coinControl())));
        } else {
            ui->labelBalance->setText(BitcoinUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), balance));
        }
    }
}

void SendCoinsDialog::updateDisplayUnit()
{
    setBalance(model->getBalance(), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    ui->customFee->setDisplayUnit(model->getOptionsModel()->getDisplayUnit());
    updateMinFeeLabel();
    updateSmartFeeLabel();
}

void SendCoinsDialog::processSendCoinsReturn(const WalletModel::SendCoinsReturn &sendCoinsReturn, const QString &msgArg)
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
        msgParams.first = tr("A fee higher than %1 is considered an absurdly high fee.").arg(BitcoinUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), maxTxFee));
        break;
    case WalletModel::PaymentRequestExpired:
        msgParams.first = tr("Payment request expired.");
        msgParams.second = CClientUIInterface::MSG_ERROR;
        break;
    case WalletModel::InactivedBHDIP006:
        if (getPayOperateMethod() == PayOperateMethod::BindPlotter) {
            msgParams.first = tr("The bind plotter consensus active on %1 after.")
                .arg(QString::number(Params().GetConsensus().BHDIP006Height));
        } else {
            msgParams.first = tr("The rental consensus active on %1 after.")
                .arg(QString::number(Params().GetConsensus().BHDIP006Height));
        }
        break;
    case WalletModel::InvalidBindPlotterAmount:
        msgParams.first = tr("The lock amount to bind plotter must be %1.")
            .arg(BitcoinUnits::formatHtmlWithUnit(model->getOptionsModel()->getDisplayUnit(), PROTOCOL_BINDPLOTTER_LOCKAMOUNT));
        msgParams.second = CClientUIInterface::MSG_ERROR;
        break;
    case WalletModel::BindPlotterExist:
        {
            QStringList args = sendCoinsReturn.reasonCommitFailed.split("\n");
            msgParams.first = tr("The plotter %1 already binded to %2.").arg(args.size() > 0 ? args[0] : "", args.size() > 1 ? args[1] : "");
        }
        break;
    case WalletModel::SmallLoanAmount:
        msgParams.first = tr("Loan to amount must be larger than %1.")
            .arg(BitcoinUnits::formatHtmlWithUnit(model->getOptionsModel()->getDisplayUnit(), PROTOCOL_RENTAL_AMOUNT_MIN));
        break;
    case WalletModel::SmallLoanAmountExcludeFee:
        msgParams.first = tr("Loan to amount must be larger than %1 on exclude fee %2.")
            .arg(BitcoinUnits::formatHtmlWithUnit(model->getOptionsModel()->getDisplayUnit(), PROTOCOL_RENTAL_AMOUNT_MIN), msgArg);
        break;
    // included to prevent a compiler warning.
    case WalletModel::OK:
    default:
        return;
    }

    Q_EMIT message(tr("Send Coins"), msgParams.first, msgParams.second);
}

void SendCoinsDialog::minimizeFeeSection(bool fMinimize)
{
    ui->labelFeeMinimized->setVisible(fMinimize);
    ui->buttonChooseFee  ->setVisible(fMinimize);
    ui->buttonMinimizeFee->setVisible(!fMinimize);
    ui->frameFeeSelection->setVisible(!fMinimize);
    ui->horizontalLayoutSmartFee->setContentsMargins(0, (fMinimize ? 0 : 6), 0, 0);
    fFeeMinimized = fMinimize;
}

void SendCoinsDialog::on_buttonChooseFee_clicked()
{
    minimizeFeeSection(false);
}

void SendCoinsDialog::on_buttonMinimizeFee_clicked()
{
    updateFeeMinimizedLabel();
    minimizeFeeSection(true);
}

void SendCoinsDialog::useAvailableBalance(SendCoinsEntry* entry)
{
    // Get CCoinControl instance if CoinControl is enabled or create a new one.
    CCoinControl coin_control;
    if (model->getOptionsModel()->getCoinControlFeatures()) {
        coin_control = *CoinControlDialog::coinControl();
    }

    // Calculate available amount to send.
    CAmount amount = model->getBalance(&coin_control);
    for (int i = 0; i < ui->entries->count(); ++i) {
        SendCoinsEntry* e = qobject_cast<SendCoinsEntry*>(ui->entries->itemAt(i)->widget());
        if (e && !e->isHidden() && e != entry) {
            amount -= e->getValue().amount;
        }
    }

    if (amount > 0) {
      entry->checkSubtractFeeFromAmount();
      entry->setAmount(amount);
    } else {
      entry->setAmount(0);
    }
}

void SendCoinsDialog::setMinimumFee()
{
    ui->customFee->setValue(GetRequiredFee(1000));
}

void SendCoinsDialog::updateFeeSectionControls()
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

void SendCoinsDialog::updateFeeMinimizedLabel()
{
    if(!model || !model->getOptionsModel())
        return;

    if (ui->radioSmartFee->isChecked())
        ui->labelFeeMinimized->setText(ui->labelSmartFee->text());
    else {
        ui->labelFeeMinimized->setText(BitcoinUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), ui->customFee->value()) + "/kB");
    }
}

void SendCoinsDialog::updateMinFeeLabel()
{
    if (model && model->getOptionsModel())
        ui->checkBoxMinimumFee->setText(tr("Pay only the required fee of %1").arg(
            BitcoinUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), GetRequiredFee(1000)) + "/kB")
        );
}

void SendCoinsDialog::updateCoinControlState(CCoinControl& ctrl)
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

void SendCoinsDialog::updateSmartFeeLabel()
{
    if(!model || !model->getOptionsModel())
        return;
    CCoinControl coin_control;
    updateCoinControlState(coin_control);
    coin_control.m_feerate.reset(); // Explicitly use only fee estimation rate for smart fee labels
    FeeCalculation feeCalc;
    CFeeRate feeRate = CFeeRate(GetMinimumFee(1000, coin_control, ::mempool, ::feeEstimator, &feeCalc));

    ui->labelSmartFee->setText(BitcoinUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), feeRate.GetFeePerK()) + "/kB");

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

PayOperateMethod SendCoinsDialog::getPayOperateMethod()
{
    return (PayOperateMethod) ui->operateMethodComboBox->itemData(ui->operateMethodComboBox->currentIndex()).toInt();
}

// Coin Control: copy label "Quantity" to clipboard
void SendCoinsDialog::coinControlClipboardQuantity()
{
    GUIUtil::setClipboard(ui->labelCoinControlQuantity->text());
}

// Coin Control: copy label "Amount" to clipboard
void SendCoinsDialog::coinControlClipboardAmount()
{
    GUIUtil::setClipboard(ui->labelCoinControlAmount->text().left(ui->labelCoinControlAmount->text().indexOf(" ")));
}

// Coin Control: copy label "Fee" to clipboard
void SendCoinsDialog::coinControlClipboardFee()
{
    GUIUtil::setClipboard(ui->labelCoinControlFee->text().left(ui->labelCoinControlFee->text().indexOf(" ")).replace(ASYMP_UTF8, ""));
}

// Coin Control: copy label "After fee" to clipboard
void SendCoinsDialog::coinControlClipboardAfterFee()
{
    GUIUtil::setClipboard(ui->labelCoinControlAfterFee->text().left(ui->labelCoinControlAfterFee->text().indexOf(" ")).replace(ASYMP_UTF8, ""));
}

// Coin Control: copy label "Bytes" to clipboard
void SendCoinsDialog::coinControlClipboardBytes()
{
    GUIUtil::setClipboard(ui->labelCoinControlBytes->text().replace(ASYMP_UTF8, ""));
}

// Coin Control: copy label "Dust" to clipboard
void SendCoinsDialog::coinControlClipboardLowOutput()
{
    GUIUtil::setClipboard(ui->labelCoinControlLowOutput->text());
}

// Coin Control: copy label "Change" to clipboard
void SendCoinsDialog::coinControlClipboardChange()
{
    GUIUtil::setClipboard(ui->labelCoinControlChange->text().left(ui->labelCoinControlChange->text().indexOf(" ")).replace(ASYMP_UTF8, ""));
}

// Coin Control: settings menu - coin control enabled/disabled by user
void SendCoinsDialog::coinControlFeatureChanged(bool checked)
{
    ui->frameCoinControl->setVisible(checked && getPayOperateMethod() == PayOperateMethod::Pay);

    if (!checked && model) // coin control features disabled
        CoinControlDialog::coinControl()->SetNull();

    coinControlUpdateLabels();
}

// Coin Control: button inputs -> show actual coin control dialog
void SendCoinsDialog::coinControlButtonClicked()
{
    CoinControlDialog dlg(platformStyle);
    dlg.setModel(model);
    dlg.exec();
    coinControlUpdateLabels();
}

// Coin Control: checkbox custom change address
void SendCoinsDialog::coinControlChangeChecked(int state)
{
    if (state == Qt::Unchecked)
    {
        CoinControlDialog::coinControl()->destChange = CNoDestination();
        ui->labelCoinControlChangeLabel->clear();
    }
    else
        // use this to re-validate an already entered address
        coinControlChangeEdited(ui->lineEditCoinControlChange->text());

    ui->lineEditCoinControlChange->setEnabled((state == Qt::Checked));
}

// Coin Control: custom change address changed
void SendCoinsDialog::coinControlChangeEdited(const QString& text)
{
    if (model && model->getAddressTableModel())
    {
        // Default to no change address until verified
        CoinControlDialog::coinControl()->destChange = CNoDestination();
        ui->labelCoinControlChangeLabel->setStyleSheet("QLabel{color:red;}");

        const CTxDestination dest = DecodeDestination(text.toStdString());

        if (text.isEmpty()) // Nothing entered
        {
            ui->labelCoinControlChangeLabel->setText("");
        }
        else if (!IsValidDestination(dest)) // Invalid address
        {
            ui->labelCoinControlChangeLabel->setText(tr("Warning: Invalid BitcoinHD address"));
        }
        else // Valid address
        {
            if (!model->IsSpendable(dest)) {
                ui->labelCoinControlChangeLabel->setText(tr("Warning: Unknown change address"));

                // confirmation dialog
                QMessageBox::StandardButton btnRetVal = QMessageBox::question(this, tr("Confirm custom change address"), tr("The address you selected for change is not part of this wallet. Any or all funds in your wallet may be sent to this address. Are you sure?"),
                    QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);

                if(btnRetVal == QMessageBox::Yes)
                    CoinControlDialog::coinControl()->destChange = dest;
                else
                {
                    ui->lineEditCoinControlChange->setText("");
                    ui->labelCoinControlChangeLabel->setStyleSheet("QLabel{color:black;}");
                    ui->labelCoinControlChangeLabel->setText("");
                }
            }
            else // Known change address
            {
                ui->labelCoinControlChangeLabel->setStyleSheet("QLabel{color:black;}");

                // Query label
                QString associatedLabel = model->getAddressTableModel()->labelForAddress(text);
                if (!associatedLabel.isEmpty())
                    ui->labelCoinControlChangeLabel->setText(associatedLabel);
                else
                    ui->labelCoinControlChangeLabel->setText(tr("(no label)"));

                CoinControlDialog::coinControl()->destChange = dest;
            }
        }
    }
}

// Coin Control: update labels
void SendCoinsDialog::coinControlUpdateLabels()
{
    if (!model || !model->getOptionsModel())
        return;

    updateCoinControlState(*CoinControlDialog::coinControl());

    // set pay amounts
    CoinControlDialog::payAmounts.clear();
    CoinControlDialog::fSubtractFeeFromAmount = false;

    for(int i = 0; i < ui->entries->count(); ++i)
    {
        SendCoinsEntry *entry = qobject_cast<SendCoinsEntry*>(ui->entries->itemAt(i)->widget());
        if(entry && !entry->isHidden())
        {
            SendCoinsRecipient rcp = entry->getValue();
            CoinControlDialog::payAmounts.append(rcp.amount);
            if (rcp.fSubtractFeeFromAmount)
                CoinControlDialog::fSubtractFeeFromAmount = true;
        }
    }

    if (CoinControlDialog::coinControl()->HasSelected())
    {
        // actual coin control calculation
        CoinControlDialog::updateLabels(model, this);

        // show coin control stats
        ui->labelCoinControlAutomaticallySelected->hide();
        ui->widgetCoinControl->show();
    }
    else
    {
        // hide coin control stats
        ui->labelCoinControlAutomaticallySelected->show();
        ui->widgetCoinControl->hide();
        ui->labelCoinControlInsuffFunds->hide();
    }
}

SendConfirmationDialog::SendConfirmationDialog(const QString &title, const QString &text, int _secDelay,
    QWidget *parent) :
    QMessageBox(QMessageBox::Question, title, text, QMessageBox::Yes | QMessageBox::Cancel, parent), secDelay(_secDelay)
{
    setDefaultButton(QMessageBox::Cancel);
    yesButton = button(QMessageBox::Yes);
    updateYesButton();
    connect(&countDownTimer, SIGNAL(timeout()), this, SLOT(countDown()));
}

int SendConfirmationDialog::exec()
{
    updateYesButton();
    countDownTimer.start(1000);
    return QMessageBox::exec();
}

void SendConfirmationDialog::countDown()
{
    secDelay--;
    updateYesButton();

    if(secDelay <= 0)
    {
        countDownTimer.stop();
    }
}

void SendConfirmationDialog::updateYesButton()
{
    if(secDelay > 0)
    {
        yesButton->setEnabled(false);
        yesButton->setText(tr("Yes") + " (" + QString::number(secDelay) + ")");
    }
    else
    {
        yesButton->setEnabled(true);
        yesButton->setText(tr("Yes"));
    }
}
