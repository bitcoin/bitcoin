// Copyright (c) 2011-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <qt/sendsign.h>
#include <qt/forms/ui_sendsign.h>

#include <qt/bitcoinunits.h>
#include <qt/guiutil.h>
#include <qt/optionsmodel.h>
#include <qt/platformstyle.h>

#include <wallet/coincontrol.h>

SendSign::SendSign(const PlatformStyle *platformStyle, QWidget* parent)
    : QWidget(parent),
    ui(new Ui::SendSign)
{
    ui->setupUi(this);
    connect(&countDownTimer, &QTimer::timeout, this, &SendSign::countDown);
    connect(ui->editButton, &QPushButton::clicked, this, &SendSign::cancel);
    connect(ui->cancelButton, &QPushButton::clicked, this, &SendSign::cancel);
    connect(ui->sendButton, &QPushButton::clicked, this, &SendSign::confirm);
}

SendSign::~SendSign()
{
    delete ui;
}

bool SendSign::isActiveWidget()
{
    return m_is_active_widget;
}

void SendSign::setActiveWidget(bool active)
{
    m_is_active_widget = active;
}

void SendSign::setModel(WalletModel *_walletModel)
{
    this->walletModel = _walletModel;
}

void SendSign::processSendCoinsReturn(const WalletModel::SendCoinsReturn &sendCoinsReturn, const QString &msgArg)
{
    QPair<QString, CClientUIInterface::MessageBoxFlags> msgParams;
    // Default to a warning message, override if error message is needed
    msgParams.second = CClientUIInterface::MSG_WARNING;

    // This comment is specific to SendCompose usage of WalletModel::SendCoinsReturn.
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
    case WalletModel::AbsurdFee:
        msgParams.first = tr("A fee higher than %1 is considered an absurdly high fee.").arg(BitcoinUnits::formatWithUnit(walletModel->getOptionsModel()->getDisplayUnit(), walletModel->wallet().getDefaultMaxTxFee()));
        break;
    case WalletModel::PaymentRequestExpired:
        msgParams.first = tr("Payment request expired.");
        msgParams.second = CClientUIInterface::MSG_ERROR;
        break;
    // included to prevent a compiler warning.
    case WalletModel::TransactionCommitFailed:
    case WalletModel::OK:
    default:
        return;
    }

    Q_EMIT message(tr("Send Coins"), msgParams.first, msgParams.second);
}

void SendSign::renderForm() {
    QString areYouSure = tr("Are you sure you want to send?");
    QString send = tr("Send");
    QString sendTooltip = tr("Confirm the send action");
    switch (mode) {
    case Mode::SignTransaction:
        ui->editButton->setVisible(true);
        ui->cancelButton->setVisible(false);
        break;
    case Mode::CreatePsbt:
        ui->editButton->setVisible(true);
        ui->cancelButton->setVisible(false);
        ui->explanation->setVisible(true);
        areYouSure = tr("Do you want to draft this transaction? This will produce a Partially Signed Bitcoin Transaction (PSBT) which you can copy and then sign with e.g. an offline Bitcoin Core wallet, or a PSBT-compatible hardware wallet.");
        send = tr("Copy PSBT to clipboard");
        break;
    case Mode::SignRbf:
        ui->editButton->setVisible(false);
        ui->cancelButton->setVisible(true);
        areYouSure = tr("Do you want to increase the fee?");
        send = tr("Bump fee");
        sendTooltip = tr("Confirm the fee increase");
        break;
    }
    ui->labelAreYouSure->setText(areYouSure);
    ui->labelAreYouSure->setWordWrap(true);
    if(secDelay > 0)
    {
        ui->sendButton->setEnabled(false);
        ui->sendButton->setText(send + " (" + QString::number(secDelay) + ")");
    } else {
        ui->sendButton->setEnabled(true);
        ui->sendButton->setText(send);
    }
    ui->sendButton->setToolTip(sendTooltip);
}

void SendSign::prepareTransaction()
{
    assert(m_is_active_widget);
    mode = !walletModel->privateKeysDisabled() ? Mode::SignTransaction : Mode::CreatePsbt;
    if (mode == Mode::SignTransaction) {
        secDelay = SEND_CONFIRM_DELAY;
        countDownTimer.start(1000);
    }
    renderForm();

    WalletModel::UnlockContext ctx(walletModel->requestUnlock());
    if(!ctx.isValid())
    {
        // Unlock wallet was cancelled
        Q_EMIT signCancelled();
        return;
    }

    WalletModel::SendCoinsReturn prepareStatus;
    prepareStatus = walletModel->prepareTransaction(*transaction, *coinControl);

    // process prepareStatus and on error generate message shown to user
    processSendCoinsReturn(prepareStatus,
        BitcoinUnits::formatWithUnit(walletModel->getOptionsModel()->getDisplayUnit(), transaction->getTransactionFee()));

    if(prepareStatus.status != WalletModel::OK) {
        Q_EMIT signCancelled();
        return;
    }

    CAmount txFee = transaction->getTransactionFee();

    // Format confirmation message
    QStringList formatted;
    for (const SendCoinsRecipient &rcp : transaction->getRecipients())
    {
        // generate amount string with wallet name in case of multiwallet
        QString amount = BitcoinUnits::formatWithUnit(walletModel->getOptionsModel()->getDisplayUnit(), rcp.amount);
        if (walletModel->isMultiwallet()) {
            amount.append(tr(" from wallet '%1'").arg(GUIUtil::HtmlEscape(walletModel->getWalletName())));
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

    QString questionString = QString("<b>%1</b>").arg(tr("Recipients"));
    questionString = questionString.append("<br />");
    questionString = questionString.append(formatted.join("<br />"));

    if(txFee > 0)
    {
        // append fee string if a fee is required
        questionString.append("<hr /><b>");
        questionString.append(tr("Transaction fee"));
        questionString.append("</b>");

        // append transaction size
        questionString.append(" (" + QString::number((double)transaction->getTransactionSize() / 1000) + " kB): ");

        // append transaction fee value
        questionString.append("<span style='color:#aa0000; font-weight:bold;'>");
        questionString.append(BitcoinUnits::formatHtmlWithUnit(walletModel->getOptionsModel()->getDisplayUnit(), txFee));
        questionString.append("</span><br />");

        // append RBF message according to transaction's signalling
        questionString.append("<span style='font-size:10pt; font-weight:normal;'>");
        assert(coinControl->m_signal_bip125_rbf); // SendCompose always sets this boost::optional
        if (*coinControl->m_signal_bip125_rbf) {
            questionString.append(tr("You can increase the fee later (signals Replace-By-Fee, BIP-125)."));
        } else {
            questionString.append(tr("Not signalling Replace-By-Fee, BIP-125."));
        }
        questionString.append("</span>");
    }

    // add total amount in all subdivision units
    questionString.append("<hr />");
    CAmount totalAmount = transaction->getTotalTransactionAmount() + txFee;
    QStringList alternativeUnits;
    for (const BitcoinUnits::Unit u : BitcoinUnits::availableUnits())
    {
        if(u != walletModel->getOptionsModel()->getDisplayUnit())
            alternativeUnits.append(BitcoinUnits::formatHtmlWithUnit(u, totalAmount));
    }
    questionString.append(QString("<b>%1</b>: <b>%2</b>").arg(tr("Total Amount"))
        .arg(BitcoinUnits::formatHtmlWithUnit(walletModel->getOptionsModel()->getDisplayUnit(), totalAmount)));
    questionString.append(QString("<br /><span style='font-size:10pt; font-weight:normal;'>(=%1)</span>")
        .arg(alternativeUnits.join(" " + tr("or") + " ")));

    ui->questionLabel->setText(questionString);
}

void SendSign::prepareSignRbf(uint256 _txid, CMutableTransaction _mtx, CAmount old_fee, CAmount new_fee) {
    assert(m_is_active_widget);
    mode = Mode::SignRbf;
    secDelay = SEND_CONFIRM_DELAY;
    countDownTimer.start(1000);
    renderForm();

    txid = _txid;
    mtx = _mtx;

    WalletModel::UnlockContext ctx(walletModel->requestUnlock());
    if(!ctx.isValid())
    {
        // Unlock wallet was cancelled
        Q_EMIT rbfCancelled(txid);
        return;
    }

    QString questionString;
    questionString.append("<br />");
    questionString.append("<table style=\"text-align: left;\">");
    questionString.append("<tr><td>");
    questionString.append(tr("Current fee:"));
    questionString.append("</td><td>");
    questionString.append(BitcoinUnits::formatHtmlWithUnit(walletModel->getOptionsModel()->getDisplayUnit(), old_fee));
    questionString.append("</td></tr><tr><td>");
    questionString.append(tr("Increase:"));
    questionString.append("</td><td>");
    questionString.append(BitcoinUnits::formatHtmlWithUnit(walletModel->getOptionsModel()->getDisplayUnit(), new_fee - old_fee));
    questionString.append("</td></tr><tr><td>");
    questionString.append(tr("New fee:"));
    questionString.append("</td><td>");
    questionString.append(BitcoinUnits::formatHtmlWithUnit(walletModel->getOptionsModel()->getDisplayUnit(), new_fee));
    questionString.append("</td></tr></table>");

    ui->questionLabel->setText(questionString);
}

void SendSign::countDown()
{
    secDelay--;
    renderForm();

    if(secDelay <= 0)
    {
        countDownTimer.stop();
    }
}

void SendSign::cancel()
{
    assert(m_is_active_widget);
    switch (mode) {
    case Mode::SignTransaction:
    case Mode::CreatePsbt:
        Q_EMIT signCancelled();
        break;
    case Mode::SignRbf:
        Q_EMIT rbfCancelled(txid);
        break;
    }

}

void SendSign::confirm()
{
    assert(m_is_active_widget);
    switch (mode) {
    case Mode::SignTransaction:
    case Mode::CreatePsbt:
        Q_EMIT signConfirmed();
        break;
    case Mode::SignRbf:
        Q_EMIT rbfConfirmed(txid, mtx);
        break;
    }
}
