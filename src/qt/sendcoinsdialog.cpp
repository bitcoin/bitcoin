// Copyright (c) 2011-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

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

#include <chainparams.h>
#include <consensus/tx_verify.h>
#include <interfaces/node.h>
#include <key_io.h>
#include <policy/fees.h>
#include <txmempool.h>
#include <wallet/coincontrol.h>
#include <wallet/fees.h>
#include <ui_interface.h>
#include <validation.h>

#include <functional>
#include <memory>

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
    clientModel(nullptr),
    model(nullptr),
    fNewRecipientAllowed(true),
    fFeeMinimized(true),
    platformStyle(_platformStyle)
{
    ui->setupUi(this);

    if (!_platformStyle->getImagesOnButtons()) {
        ui->addButton->setIcon(QIcon());
        ui->clearButton->setIcon(QIcon());
        ui->sendButton->setIcon(QIcon());
        ui->checkBindDataButton->setIcon(QIcon());
    } else {
        ui->addButton->setIcon(_platformStyle->SingleColorIcon(":/icons/add"));
        ui->clearButton->setIcon(_platformStyle->SingleColorIcon(":/icons/remove"));
        ui->sendButton->setIcon(_platformStyle->SingleColorIcon(":/icons/send"));
        ui->checkBindDataButton->setIcon(_platformStyle->SingleColorIcon(":/icons/key"));
    }

    GUIUtil::setupAddressWidget(ui->lineEditCoinControlChange, this);

    // Operate method
    ui->operateMethodComboBox->addItem(tr("Pay to"), (int)PayOperateMethod::Pay);
    ui->operateMethodComboBox->addItem(tr("Bind to"), (int)PayOperateMethod::BindPlotter);
    ui->operateMethodComboBox->addItem(tr("Point to"), (int)PayOperateMethod::Point);
    ui->operateMethodComboBox->addItem(tr("Staking to"), (int)PayOperateMethod::Staking);
    addEntry();

    connect(ui->addButton, &QPushButton::clicked, this, &SendCoinsDialog::addEntry);
    connect(ui->clearButton, &QPushButton::clicked, this, &SendCoinsDialog::clear);
    connect(ui->operateMethodComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(onOperateMethodComboBoxChanged(int)));

    // Coin Control
    connect(ui->pushButtonCoinControl, &QPushButton::clicked, this, &SendCoinsDialog::coinControlButtonClicked);
    connect(ui->checkBoxCoinControlChange, &QCheckBox::stateChanged, this, &SendCoinsDialog::coinControlChangeChecked);
    connect(ui->lineEditCoinControlChange, &QValidatedLineEdit::textEdited, this, &SendCoinsDialog::coinControlChangeEdited);

    // Coin Control: clipboard actions
    QAction *clipboardQuantityAction = new QAction(tr("Copy quantity"), this);
    QAction *clipboardAmountAction = new QAction(tr("Copy amount"), this);
    QAction *clipboardFeeAction = new QAction(tr("Copy fee"), this);
    QAction *clipboardAfterFeeAction = new QAction(tr("Copy after fee"), this);
    QAction *clipboardBytesAction = new QAction(tr("Copy bytes"), this);
    QAction *clipboardLowOutputAction = new QAction(tr("Copy dust"), this);
    QAction *clipboardChangeAction = new QAction(tr("Copy change"), this);
    connect(clipboardQuantityAction, &QAction::triggered, this, &SendCoinsDialog::coinControlClipboardQuantity);
    connect(clipboardAmountAction, &QAction::triggered, this, &SendCoinsDialog::coinControlClipboardAmount);
    connect(clipboardFeeAction, &QAction::triggered, this, &SendCoinsDialog::coinControlClipboardFee);
    connect(clipboardAfterFeeAction, &QAction::triggered, this, &SendCoinsDialog::coinControlClipboardAfterFee);
    connect(clipboardBytesAction, &QAction::triggered, this, &SendCoinsDialog::coinControlClipboardBytes);
    connect(clipboardLowOutputAction, &QAction::triggered, this, &SendCoinsDialog::coinControlClipboardLowOutput);
    connect(clipboardChangeAction, &QAction::triggered, this, &SendCoinsDialog::coinControlClipboardChange);
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
        settings.setValue("nTransactionFee", (qint64)DEFAULT_PAY_TX_FEE);
    ui->groupFee->setId(ui->radioSmartFee, 0);
    ui->groupFee->setId(ui->radioCustomFee, 1);
    ui->groupFee->button((int)std::max(0, std::min(1, settings.value("nFeeRadio").toInt())))->setChecked(true);
    ui->customFee->SetAllowEmpty(false);
    ui->customFee->setValue(settings.value("nTransactionFee").toLongLong());
    minimizeFeeSection(settings.value("fFeeSectionMinimized").toBool());
}

void SendCoinsDialog::setClientModel(ClientModel *_clientModel)
{
    this->clientModel = _clientModel;

    if (_clientModel) {
        connect(_clientModel, &ClientModel::numBlocksChanged, this, [this](int count, const QDateTime&, double, bool header) {
            //! Optimize update smart fee. updateSmartFeeLabel() call CTxMemPool::GetMinFee() require txmempool.cs
            if (!header && count + 24 > clientModel->getHeaderTipHeight())
                updateSmartFeeLabel();
        });
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

        interfaces::WalletBalances balances = _model->wallet().getBalances();
        setBalance(balances);
        connect(_model, &WalletModel::balanceChanged, this, &SendCoinsDialog::setBalance);
        connect(_model->getOptionsModel(), &OptionsModel::displayUnitChanged, this, &SendCoinsDialog::updateDisplayUnit);
        updateDisplayUnit();

        // Coin Control
        connect(_model->getOptionsModel(), &OptionsModel::displayUnitChanged, this, &SendCoinsDialog::coinControlUpdateLabels);
        connect(_model->getOptionsModel(), &OptionsModel::coinControlFeaturesChanged, this, &SendCoinsDialog::coinControlFeatureChanged);
        ui->frameCoinControl->setVisible(_model->getOptionsModel()->getCoinControlFeatures() && getPayOperateMethod() != PayOperateMethod::Pay);
        coinControlUpdateLabels();

        // fee section
        for (const int n : confTargets) {
            ui->confTargetSelector->addItem(tr("%1 (%2 blocks)").arg(GUIUtil::formatNiceTimeOffset(n*Params().GetConsensus().nPowTargetSpacing)).arg(n));
        }
        connect(ui->confTargetSelector, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &SendCoinsDialog::updateSmartFeeLabel);
        connect(ui->confTargetSelector, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &SendCoinsDialog::coinControlUpdateLabels);
        connect(ui->groupFee, static_cast<void (QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked), this, &SendCoinsDialog::updateFeeSectionControls);
        connect(ui->groupFee, static_cast<void (QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked), this, &SendCoinsDialog::coinControlUpdateLabels);
        connect(ui->customFee, &BitcoinAmountField::valueChanged, this, &SendCoinsDialog::coinControlUpdateLabels);
        connect(ui->optInRBF, &QCheckBox::stateChanged, this, &SendCoinsDialog::updateSmartFeeLabel);
        connect(ui->optInRBF, &QCheckBox::stateChanged, this, &SendCoinsDialog::coinControlUpdateLabels);
        CAmount requiredFee = model->wallet().getRequiredFee(1000);
        ui->customFee->SetMinValue(requiredFee);
        if (ui->customFee->value() < requiredFee) {
            ui->customFee->setValue(requiredFee);
        }
        ui->customFee->setSingleStep(requiredFee);
        updateFeeSectionControls();
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
            ui->confTargetSelector->setCurrentIndex(getIndexForConfTarget(model->wallet().getConfirmTarget()));
        else
            ui->confTargetSelector->setCurrentIndex(getIndexForConfTarget(settings.value("nConfTarget").toInt()));

        // set default pay method and force trigger update status
        ui->operateMethodComboBox->setCurrentIndex(0);
        QMetaObject::invokeMethod(ui->operateMethodComboBox, "currentIndexChanged", Qt::QueuedConnection,
            Q_ARG(int, ui->operateMethodComboBox->currentIndex()));
        // listen update primary address
        connect(model, &WalletModel::primaryAddressChanged, this, &SendCoinsDialog::primaryAddressChanged);
    }
}

SendCoinsDialog::~SendCoinsDialog()
{
    QSettings settings;
    settings.setValue("fFeeSectionMinimized", fFeeMinimized);
    settings.setValue("nFeeRadio", ui->groupFee->checkedId());
    settings.setValue("nConfTarget", getConfTargetForIndex(ui->confTargetSelector->currentIndex()));
    settings.setValue("nTransactionFee", (qint64)ui->customFee->value());

    delete ui;
}

void SendCoinsDialog::onOperateMethodComboBoxChanged(int index)
{
    if(!model || !model->getOptionsModel())
        return;

    if (index >= 0) {
        int opMethodValue = ui->operateMethodComboBox->itemData(index).toInt();
        switch ((PayOperateMethod)opMethodValue)
        {
        case PayOperateMethod::BindPlotter:
        case PayOperateMethod::Point:
        case PayOperateMethod::Staking:
            ui->clearButton->setVisible(false);
            ui->addButton->setVisible(false);
            ui->frameCoinControl->setVisible(false);
            ui->optInRBF->setVisible(false);
            break;
        default: // Normal pay
            ui->clearButton->setVisible(true);
            ui->addButton->setVisible(true);
            ui->optInRBF->setVisible(true);
            ui->frameCoinControl->setVisible(model->getOptionsModel()->getCoinControlFeatures());
            coinControlChangeEdited(ui->lineEditCoinControlChange->text());
            break;
        }

        ui->frameFee->setVisible((PayOperateMethod) opMethodValue != PayOperateMethod::BindPlotter);
        ui->checkBindDataButton->setVisible((PayOperateMethod) opMethodValue == PayOperateMethod::BindPlotter);

        clear();
        setBalance(model->wallet().getBalances());
    }
}

void SendCoinsDialog::on_sendButton_clicked()
{
    if(!model || !model->getOptionsModel())
        return;

    fNewRecipientAllowed = false;
    std::unique_ptr< bool,std::function<void(bool*)> > resetNewRecipientAllowed(&fNewRecipientAllowed, [](bool *b) { *b = true; });

    PayOperateMethod operateMethod = getPayOperateMethod();

    QList<SendCoinsRecipient> recipients;
    bool valid = true;

    for(int i = 0; i < ui->entries->count(); ++i)
    {
        SendCoinsEntry *entry = qobject_cast<SendCoinsEntry*>(ui->entries->itemAt(i)->widget());
        if(entry)
        {
            if(entry->validate(model->node()))
            {
                recipients.append(entry->getValue());
            }
            else if (valid)
            {
                ui->scrollArea->ensureWidgetVisible(entry);
                valid = false;
            }
        }
    }

    if(!valid || recipients.isEmpty())
    {
        return;
    }

    WalletModel::UnlockContext ctx(model->requestUnlock());
    if(!ctx.isValid())
    {
        // Unlock wallet was cancelled
        return;
    }

    // prepare transaction for getting txFee earlier
    WalletModelTransaction currentTransaction(recipients);
    WalletModel::SendCoinsReturn prepareStatus;

    const Consensus::Params& params = Params().GetConsensus();
    auto& chain = model->wallet().chain();
    int nSpendHeight = chain.lock()->getHeight().get_value_or(0) + 1;
    int nBindLimitHeight = 0;

    // Check tx payload
    CTxOutPayloadRef payload;
    if (operateMethod == PayOperateMethod::BindPlotter) {
        CTxOut txout(recipients[0].amount, GetScriptForDestination(DecodeDestination(recipients[0].address.toStdString())), recipients[0].payload);
        payload = ExtractTxoutPayload(txout, nSpendHeight, {TXOUT_TYPE_BINDPLOTTER}, true);
        if (!payload || payload->type != TXOUT_TYPE_BINDPLOTTER) {
            QMessageBox msgBox(QMessageBox::Warning,
                tr("Bind plotter"),
                tr("Invalid bind plotter recipients!"),
                QMessageBox::Close, this);
            msgBox.exec();
            return;
        }

        if (chain.haveActiveBindPlotter(ExtractAccountID(DecodeDestination(recipients[0].address.toStdString())), BindPlotterPayload::As(payload)->GetId())) {
            QMessageBox msgBox(QMessageBox::Warning,
                tr("Bind plotter"),
                tr("The plotter %1 already binded to %2.").arg(QString::number(BindPlotterPayload::As(payload)->GetId()), recipients[0].address), 
                QMessageBox::Close, this);
            msgBox.exec();
            return;
        }
    } else if (operateMethod == PayOperateMethod::Point) {
        CTxOut txout(recipients[0].amount, GetScriptForDestination(DecodeDestination(recipients[0].address.toStdString())), recipients[0].payload);
        payload = ExtractTxoutPayload(txout, nSpendHeight, {TXOUT_TYPE_POINT});
        if (!payload || payload->type != TXOUT_TYPE_POINT) {
            prepareStatus = WalletModel::SendCoinsReturn(WalletModel::TransactionCreationFailed);
            QMessageBox msgBox(QMessageBox::Warning, tr("Point"), tr("Invalid point recipients!"), QMessageBox::Close, this);
            msgBox.exec();
            return;
        }
    } else if (operateMethod == PayOperateMethod::Staking) {
        CTxOut txout(recipients[0].amount, GetScriptForDestination(DecodeDestination(recipients[0].address.toStdString())), recipients[0].payload);
        payload = ExtractTxoutPayload(txout, nSpendHeight, {TXOUT_TYPE_STAKING});
        if (!payload || payload->type != TXOUT_TYPE_STAKING) {
            prepareStatus = WalletModel::SendCoinsReturn(WalletModel::TransactionCreationFailed);
            QMessageBox msgBox(QMessageBox::Warning, tr("Staking"), tr("Invalid staking recipients!"), QMessageBox::Close, this);
            msgBox.exec();
            return;
        }
    }

    // Always use a CCoinControl instance, use the CoinControlDialog instance if CoinControl has been enabled
    CCoinControl ctrl;
    if (operateMethod == PayOperateMethod::BindPlotter)
    {
        if (model->node().isInitialBlockDownload()){
            QMessageBox msgBox(QMessageBox::Warning, tr("Bind plotter"), tr("Please wait block sync!"), QMessageBox::Ok, this);
            msgBox.exec();
            return;
        }

        // Update bind fee
        auto bindPunishment = chain.getBindPlotterPunishment(nSpendHeight, BindPlotterPayload::As(payload)->GetId());
        if (bindPunishment.first > 0) {
            ctrl.m_min_txfee = std::max(ctrl.m_min_txfee, bindPunishment.first);
            QString information = tr("This binding operation triggers anti-cheating mechanism and therefore requires a large transaction fee %1.")
                .arg("<span style='color:#aa0000;'>" + BitcoinUnits::formatHtmlWithUnit(model->getOptionsModel()->getDisplayUnit(), ctrl.m_min_txfee) + "</span>");
            QMessageBox msgBox(QMessageBox::Warning, tr("Bind plotter"), information, QMessageBox::Ok|QMessageBox::Cancel, this);
            if (msgBox.exec() == QMessageBox::Cancel)
                return;

            nBindLimitHeight = bindPunishment.second;
        }
    }
    else
    {
        if (model->getOptionsModel()->getCoinControlFeatures())
            ctrl = *CoinControlDialog::coinControl();
        updateCoinControlState(ctrl);
    }

    prepareStatus = model->prepareTransaction(currentTransaction, ctrl);

    // process prepareStatus and on error generate message shown to user
    processSendCoinsReturn(prepareStatus,
        BitcoinUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), currentTransaction.getTransactionFee()));

    if(prepareStatus.status != WalletModel::OK) {
        return;
    }

    CAmount txFee = currentTransaction.getTransactionFee();

    // Format confirmation message
    QStringList formatted;
    for (const SendCoinsRecipient &rcp : currentTransaction.getRecipients())
    {
        // generate amount string with wallet name in case of multiwallet
        QString amount = BitcoinUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), rcp.amount);
        if (model->isMultiwallet()) {
            amount.append(tr(" from wallet '%1'").arg(GUIUtil::HtmlEscape(model->getWalletName())));
        }

        // generate address string
        QString address = rcp.address;

        QString recipientElement;

#ifdef ENABLE_BIP70
        if (!rcp.paymentRequest.IsInitialized()) // normal payment
#endif
        {
            if(rcp.label.length() > 0) // label with address
            {
                recipientElement.append(tr("%1 to '%2'").arg(amount, GUIUtil::HtmlEscape(rcp.label)));
                recipientElement.append(QString(" (%1)").arg(address));
            }
            else // just address
            {
                recipientElement.append(tr("%1 to %2").arg(amount, address));
            }
        }
#ifdef ENABLE_BIP70
        else if(!rcp.authenticatedMerchant.isEmpty()) // authenticated payment request
        {
            recipientElement.append(tr("%1 to '%2'").arg(amount, rcp.authenticatedMerchant));
        }
        else // unauthenticated payment request
        {
            recipientElement.append(tr("%1 to %2").arg(amount, address));
        }
#endif

        formatted.append(recipientElement);
    }

    QString titleString, questionString;
    if (operateMethod == PayOperateMethod::BindPlotter)
    {
        assert(payload && payload->type == TXOUT_TYPE_BINDPLOTTER);

        titleString = tr("Confirm bind plotter");
        questionString = "<span style='color:#aa0000;'><b>" + tr("Are you sure you want to send?") + "</b></span>";

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
        formatted.append(tr("The operation will lock %1.").arg(amount));
        formatted.append(tr("You can unbind later, and the locked amount will be returned to this address."));
    }
    else if (operateMethod == PayOperateMethod::Point)
    {
        assert(payload && payload->type == TXOUT_TYPE_POINT);

        titleString = tr("Confirm point coins");
        questionString = "<span style='color:#aa0000;'><b>" + tr("Are you sure you want send?") + "</b></span>";

        const SendCoinsRecipient &rcp = recipients[0];
        QString lockAmount = "<b>" + BitcoinUnits::formatHtmlWithUnit(model->getOptionsModel()->getDisplayUnit(), rcp.amount) + "</b>";
        QString lockInAddress = "<span style='font-family: monospace;'>" + rcp.address + "</span>";
        QString recvAmount = "<b>" + BitcoinUnits::formatHtmlWithUnit(model->getOptionsModel()->getDisplayUnit(), PointPayload::As(payload)->GetAmount()) + "</b>";
        QString recvAddress = "<span style='font-family: monospace;'>" + QString::fromStdString(EncodeDestination(ExtractDestination(PointPayload::As(payload)->GetReceiverID()))) + "</span>";

        formatted.clear();
        formatted.append(tr("Point %1 to %2, and lock %3 blocks.").arg(recvAmount, recvAddress, QString::number(PointPayload::As(payload)->GetLockBlocks())));
        formatted.append(tr("The operation will lock %1 in %2.").arg(lockAmount, lockInAddress));
        formatted.append(tr("You can withdraw this point at lock timeout, and the amount will be returned to this address."));
    }
    else if (operateMethod == PayOperateMethod::Staking)
    {
        assert(payload && payload->type == TXOUT_TYPE_STAKING);

        titleString = tr("Confirm staking coins");
        questionString = "<span style='color:#aa0000;'><b>" + tr("Are you sure you want send?") + "</b></span>";
        
        const SendCoinsRecipient &rcp = recipients[0];
        QString lockAmount = "<b>" + BitcoinUnits::formatHtmlWithUnit(model->getOptionsModel()->getDisplayUnit(), rcp.amount) + "</b>";
        QString lockInAddress = "<span style='font-family: monospace;'>" + rcp.address + "</span>";
        QString recvAmount = "<b>" + BitcoinUnits::formatHtmlWithUnit(model->getOptionsModel()->getDisplayUnit(), StakingPayload::As(payload)->GetAmount()) + "</b>";
        QString recvAddress = "<span style='font-family: monospace;'>" + QString::fromStdString(EncodeDestination(ExtractDestination(StakingPayload::As(payload)->GetReceiverID()))) + "</span>";

        formatted.clear();
        formatted.append(tr("Staking %1 to %2, and lock %3 blocks.").arg(recvAmount, recvAddress, QString::number(StakingPayload::As(payload)->GetLockBlocks())));
        formatted.append(tr("The operation will lock %1 in %2.").arg(lockAmount, lockInAddress));
        formatted.append(tr("You can withdraw this staking at lock timeout, and the amount will be returned to this address."));
    }
    else
    {
        titleString = tr("Confirm send coins");
        questionString = "<span style='color:#aa0000;'><b>" + tr("Are you sure you want to send?") + "<b></span>";
    }
    questionString.append("</span>%1");

    if(txFee > 0)
    {
        // append fee string if a fee is required
        questionString.append("<hr /><b>");
        questionString.append(tr("Transaction fee"));
        questionString.append("</b>");

        // append transaction size
        questionString.append(" (" + QString::number((double)currentTransaction.getTransactionSize() / 1000) + " kB): ");

        // append transaction fee value
        questionString.append("<span style='color:#aa0000; font-weight:bold;'>");
        questionString.append(BitcoinUnits::formatHtmlWithUnit(model->getOptionsModel()->getDisplayUnit(), txFee));
        questionString.append("</span><br />");

        // append bindplotter limit notice
        if (operateMethod == PayOperateMethod::BindPlotter && nBindLimitHeight > 0) {
            questionString.append("<span style='color:#aa0000;'>").
                append(tr("This binding operation triggers anti-cheating mechanism and therefore requires a large transaction fee %1.").
                    arg(BitcoinUnits::formatHtmlWithUnit(model->getOptionsModel()->getDisplayUnit(), txFee))).
                append("</span>");
            questionString.append("<br />").
                append(tr("%1 small bind plotter fee active on %2 block height (%3 blocks after, about %4 minute).").
                        arg("<b>" + QString::number(BindPlotterPayload::As(payload)->GetId()) + "</b>",
                            QString::number(nBindLimitHeight),
                            QString::number(nBindLimitHeight - nSpendHeight),
                            QString::number((nBindLimitHeight - nSpendHeight) * params.nPowTargetSpacing / 60)));
            questionString.append("<br />");
        }

        // append RBF message according to transaction's signalling
        if (ui->frameFee->isVisible()) {
            questionString.append("<span style='font-size:10pt; font-weight:normal;'>");
            if (ui->optInRBF->isChecked() && ui->optInRBF->isVisible()) {
                questionString.append(tr("You can increase the fee later (signals Replace-By-Fee, BIP-125)."));
            } else {
                questionString.append(tr("Not signalling Replace-By-Fee, BIP-125."));
            }
            questionString.append("</span>");
        }
    }

    // add total amount in all subdivision units
    questionString.append("<hr />");
    CAmount totalAmount = currentTransaction.getTotalTransactionAmount() + txFee;
    QStringList alternativeUnits;
    for (const BitcoinUnits::Unit u : BitcoinUnits::availableUnits())
    {
        if(u != model->getOptionsModel()->getDisplayUnit())
            alternativeUnits.append(BitcoinUnits::formatHtmlWithUnit(u, totalAmount));
    }
    questionString.append(QString("<b>%1</b>: <b>%2</b>").arg(tr("Total Amount"))
        .arg(BitcoinUnits::formatHtmlWithUnit(model->getOptionsModel()->getDisplayUnit(), totalAmount)));
    questionString.append(QString("<br /><span style='font-size:10pt; font-weight:normal;'>(=%1)</span>")
        .arg(alternativeUnits.join(" " + tr("or") + " ")));

    QString informative_text;
    QString detailed_text;
    if (operateMethod == PayOperateMethod::Pay && formatted.size() > 1) {
        questionString = questionString.arg("");
        informative_text = tr("To review recipient list click \"Show Details...\"");
        detailed_text = formatted.join("\n\n");
    } else {
        questionString = questionString.arg("<br /><br />" + formatted.join("\n\n"));
    }

    SendConfirmationDialog confirmationDialog(tr("Confirm send coins"), questionString, informative_text, detailed_text, SEND_CONFIRM_DELAY, this);
    confirmationDialog.exec();
    QMessageBox::StandardButton retval = static_cast<QMessageBox::StandardButton>(confirmationDialog.result());

    if(retval != QMessageBox::Yes)
    {
        return;
    }

    // now send the prepared transaction
    WalletModel::SendCoinsReturn sendStatus = model->sendCoins(currentTransaction);
    // process sendStatus and on error generate message shown to user
    processSendCoinsReturn(sendStatus);

    if (sendStatus.status == WalletModel::OK)
    {
        accept();
        CoinControlDialog::coinControl()->UnSelectAll();
        coinControlUpdateLabels();
        Q_EMIT coinsSent(currentTransaction.getWtx()->GetHash());
    }
}

void SendCoinsDialog::on_checkBindDataButton_clicked()
{
    if (!model || !model->getOptionsModel() || ui->entries->count() != 1)
        return;
    if (clientModel->getHeaderTipTime() < (GetTime() - nMaxTipAge)) {
        QMessageBox msgBox(QMessageBox::Warning, tr("Bind plotter"), tr("Please wait block header sync!"), QMessageBox::Ok, this);
        msgBox.exec();
        return;
    }
    int nTipHeight = clientModel->getHeaderTipHeight(); // Use header tip height generate and check bind data

    SendCoinsEntry *entry = qobject_cast<SendCoinsEntry*>(ui->entries->itemAt(0)->widget());
    if (!entry->validate(model->node()))
        return;
    const SendCoinsRecipient recipient = entry->getValue();
    CTxDestination bindToDest = DecodeDestination(recipient.address.toStdString());

    // Verify data
    CTxOut txout(PROTOCOL_BINDPLOTTER_LOCKAMOUNT, GetScriptForDestination(bindToDest), recipient.payload);
    auto payload = ExtractTxoutPayload(txout, nTipHeight, {}, true);
    if (!payload || payload->type != TXOUT_TYPE_BINDPLOTTER) {
        QMessageBox msgBox(QMessageBox::Critical, tr("Bind plotter"), tr("Bad bind plotter data!"), QMessageBox::Ok, this);
        msgBox.exec();
        return;
    }

    // format
    QString address;
    if (recipient.label.length() > 0)
        address = GUIUtil::HtmlEscape(recipient.label) + "(<span style='font-family: monospace;'>" + recipient.address + "</span>)";
    else
        address = "<span style='font-family: monospace;'>" + recipient.address + "</span>";
    address = "<b>" + address + "</b>";
    QString plotterId = "<b>" + QString::number(BindPlotterPayload::As(payload)->GetId()) + "</b>";

    QString information;
    information += tr("Bind %1 to %2").arg(plotterId, address) + "<br /><br />";
    information += tr("You can copy and send below signature bind data to %1 owner:").arg(address);
    information += "<hr />";
    information += "<p>";
    information += QString::fromStdString(HexStr(recipient.payload.begin(), recipient.payload.end())).insert(144, "<br />").insert(72, "<br />");
    information += "</p>";

    QMessageBox msgBox(QMessageBox::Information, tr("Bind plotter"), information, QMessageBox::Ok|QMessageBox::Close, this);
    msgBox.button(QMessageBox::Ok)->setText(tr("Copy bind data"));
    if (msgBox.exec() == QMessageBox::Ok) {
        GUIUtil::setClipboard(QString::fromStdString(HexStr(recipient.payload.begin(), recipient.payload.end())));
    }
}

void SendCoinsDialog::clear()
{
    // Clear coin control settings
    CoinControlDialog::coinControl()->UnSelectAll();
    ui->checkBoxCoinControlChange->setChecked(false);
    ui->lineEditCoinControlChange->clear();
    coinControlUpdateLabels();

    // Remove entries until only one left
    while(ui->entries->count())
    {
        ui->entries->takeAt(0)->widget()->deleteLater();
    }
    addEntry();

    updateTabsAndLabels();
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
    connect(entry, &SendCoinsEntry::removeEntry, this, &SendCoinsDialog::removeEntry);
    connect(entry, &SendCoinsEntry::useAvailableBalance, this, &SendCoinsDialog::useAvailableBalance);
    connect(entry, &SendCoinsEntry::payAmountChanged, this, &SendCoinsDialog::coinControlUpdateLabels);
    connect(entry, &SendCoinsEntry::subtractFeeFromAmountChanged, this, &SendCoinsDialog::coinControlUpdateLabels);

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
    setupTabChain(nullptr);
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
    QWidget::setTabOrder(ui->sendButton, ui->checkBindDataButton);
    QWidget::setTabOrder(ui->checkBindDataButton, ui->clearButton);
    QWidget::setTabOrder(ui->clearButton, ui->addButton);
    return ui->addButton;
}

void SendCoinsDialog::setAddress(const QString &address)
{
    SendCoinsEntry *entry = nullptr;
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
    if(getPayOperateMethod() != PayOperateMethod::Pay)
        return;

    SendCoinsEntry *entry = nullptr;
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

void SendCoinsDialog::setBalance(const interfaces::WalletBalances& balances)
{
    if(model && model->getOptionsModel())
    {
        CAmount balance = balances.balance;
        if (getPayOperateMethod() != PayOperateMethod::Pay) {
            balance = model->wallet().getAvailableBalance(*CoinControlDialog::coinControl());
        }
        ui->labelBalance->setText(BitcoinUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), balance));
    }
}

void SendCoinsDialog::updateDisplayUnit()
{
    setBalance(model->wallet().getBalances());
    ui->customFee->setDisplayUnit(model->getOptionsModel()->getDisplayUnit());
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
        msgParams.first = tr("A fee higher than %1 is considered an absurdly high fee.").arg(BitcoinUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), model->wallet().getDefaultMaxTxFee()));
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
    CAmount amount = model->wallet().getAvailableBalance(coin_control);
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

void SendCoinsDialog::updateFeeSectionControls()
{
    ui->confTargetSelector      ->setEnabled(ui->radioSmartFee->isChecked());
    ui->labelSmartFee           ->setEnabled(ui->radioSmartFee->isChecked());
    ui->labelSmartFee2          ->setEnabled(ui->radioSmartFee->isChecked());
    ui->labelSmartFee3          ->setEnabled(ui->radioSmartFee->isChecked());
    ui->labelFeeEstimation      ->setEnabled(ui->radioSmartFee->isChecked());
    ui->labelCustomFeeWarning   ->setEnabled(ui->radioCustomFee->isChecked());
    ui->labelCustomPerKilobyte  ->setEnabled(ui->radioCustomFee->isChecked());
    ui->customFee               ->setEnabled(ui->radioCustomFee->isChecked());
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
    ctrl.m_signal_bip125_rbf = ui->optInRBF->isChecked();
}

void SendCoinsDialog::updateSmartFeeLabel()
{
    if(!model || !model->getOptionsModel())
        return;
    CCoinControl coin_control;
    updateCoinControlState(coin_control);
    coin_control.m_feerate.reset(); // Explicitly use only fee estimation rate for smart fee labels
    int returned_target;
    FeeReason reason;
    CFeeRate feeRate = CFeeRate(model->wallet().getMinimumFee(1000, coin_control, &returned_target, &reason));

    ui->labelSmartFee->setText(BitcoinUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), feeRate.GetFeePerK()) + "/kB");

    if (reason == FeeReason::FALLBACK) {
        ui->labelSmartFee2->show(); // (Smart fee not initialized yet. This usually takes a few blocks...)
        ui->labelFeeEstimation->setText("");
        ui->fallbackFeeWarningLabel->setVisible(true);
        int lightness = ui->fallbackFeeWarningLabel->palette().color(QPalette::WindowText).lightness();
        QColor warning_colour(255 - (lightness / 5), 176 - (lightness / 3), 48 - (lightness / 14));
        ui->fallbackFeeWarningLabel->setStyleSheet("QLabel { color: " + warning_colour.name() + "; }");
        ui->fallbackFeeWarningLabel->setIndent(GUIUtil::TextWidth(QFontMetrics(ui->fallbackFeeWarningLabel->font()), "x"));
    }
    else
    {
        ui->labelSmartFee2->hide();
        ui->labelFeeEstimation->setText(tr("Estimated to begin confirmation within %n block(s).", "", returned_target));
        ui->fallbackFeeWarningLabel->setVisible(false);
    }

    updateFeeMinimizedLabel();
}

void SendCoinsDialog::primaryAddressChanged()
{
    if(!model || !model->getOptionsModel())
        return;

    if (getPayOperateMethod() == PayOperateMethod::Pay)
        return;

    CoinControlDialog::coinControl()->destChange = model->wallet().getPrimaryAddress();
    setBalance(model->wallet().getBalances());
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
            ui->labelCoinControlChangeLabel->setText(tr("Warning: Invalid Qitcoin address"));
        }
        else // Valid address
        {
            if (!model->wallet().isSpendable(dest)) {
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

SendConfirmationDialog::SendConfirmationDialog(const QString& title, const QString& text, const QString& informative_text, const QString& detailed_text, int _secDelay, QWidget* parent)
    : QMessageBox(parent), secDelay(_secDelay)
{
    setIcon(QMessageBox::Question);
    setWindowTitle(title); // On macOS, the window title is ignored (as required by the macOS Guidelines).
    setText(text);
    setInformativeText(informative_text);
    setDetailedText(detailed_text);
    setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
    setDefaultButton(QMessageBox::Cancel);
    yesButton = button(QMessageBox::Yes);
    updateYesButton();
    connect(&countDownTimer, &QTimer::timeout, this, &SendConfirmationDialog::countDown);
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
