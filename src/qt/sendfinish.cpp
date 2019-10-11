// Copyright (c) 2011-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <qt/sendfinish.h>
#include <qt/forms/ui_sendfinish.h>

#include <qt/guiutil.h>
#include <qt/platformstyle.h>

#include <wallet/psbtwallet.h>

SendFinish::SendFinish(const PlatformStyle *platformStyle, QWidget* parent)
    : QWidget(parent),
    ui(new Ui::SendFinish)
{
    ui->setupUi(this);
    ui->labelTransaction->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    connect(ui->showTransaction, &QPushButton::clicked, this, &SendFinish::showTransactionClicked);
    connect(ui->newTransactionButton, &QPushButton::clicked, this, &SendFinish::newTransactionClicked);
}

SendFinish::~SendFinish()
{
    delete ui;
}

bool SendFinish::isActiveWidget()
{
    return m_is_active_widget;
}

void SendFinish::setActiveWidget(bool active)
{
    m_is_active_widget = active;
}

void SendFinish::setModel(WalletModel *_walletModel)
{
    this->walletModel = _walletModel;
}

void SendFinish::prepareForm() {
    ui->labelTransactionFinish->setText("Transaction sent");
    switch (mode) {
    case Mode::FinishTransaction:
        ui->showTransaction->setVisible(true);
        break;
    case Mode::FinishPsbt:
        ui->showTransaction->setVisible(false);
        ui->labelTransactionFinish->setText("PSBT created");
        break;
    }
}

void SendFinish::processSendCoinsReturn(const WalletModel::SendCoinsReturn &sendCoinsReturn, const QString &msgArg)
{
    assert(mode == Mode::FinishTransaction);
    QPair<QString, CClientUIInterface::MessageBoxFlags> msgParams;
    // Default to a warning message, override if error message is needed
    msgParams.second = CClientUIInterface::MSG_WARNING;

    // This comment is specific to SendCompose usage of WalletModel::SendCoinsReturn.
    // WalletModel::TransactionCommitFailed is used only in WalletModel::sendCoins()
    // all others are used only in WalletModel::prepareTransaction()
    switch(sendCoinsReturn.status)
    {
    case WalletModel::TransactionCommitFailed:
        msgParams.first = tr("The transaction was rejected with the following reason: %1").arg(sendCoinsReturn.reasonCommitFailed);
        msgParams.second = CClientUIInterface::MSG_ERROR;
        break;
    // included to prevent a compiler warning.
    case WalletModel::InvalidAddress:
    case WalletModel::InvalidAmount:
    case WalletModel::AmountExceedsBalance:
    case WalletModel::AmountWithFeeExceedsBalance:
    case WalletModel::DuplicateAddress:
    case WalletModel::TransactionCreationFailed:
    case WalletModel::AbsurdFee:
    case WalletModel::PaymentRequestExpired:
    case WalletModel::OK:
    default:
        return;
    }

    Q_EMIT message(tr("Send Coins"), msgParams.first, msgParams.second);
}

void SendFinish::createPsbt() {
    mode = Mode::FinishPsbt;
    prepareForm();

    assert(walletModel->privateKeysDisabled());

    CMutableTransaction mtx = CMutableTransaction{*(transaction->getWtx())};
    PartiallySignedTransaction psbtx(mtx);
    bool complete = false;
    const TransactionError err = walletModel->wallet().fillPSBT(psbtx, complete, SIGHASH_ALL, false /* sign */, true /* bip32derivs */);
    assert(!complete);
    assert(err == TransactionError::OK);
    // Serialize the PSBT
    CDataStream ssTx(SER_NETWORK, PROTOCOL_VERSION);
    ssTx << psbtx;
    GUIUtil::setClipboard(EncodeBase64(ssTx.str()).c_str());
    ui->labelTransaction->setText("The Partially Signed Bitcoin Transaction (PSBT) has been copied to the clipboard.");
    Q_EMIT message(tr("PSBT copied"), "Copied to clipboard", CClientUIInterface::MSG_INFORMATION);
}

void SendFinish::broadcastTransaction() {
    mode = Mode::FinishTransaction;
    prepareForm();

    WalletModel::SendCoinsReturn sendStatus = walletModel->sendCoins(*transaction);
    // process sendStatus and on error generate message shown to user
    processSendCoinsReturn(sendStatus);

    if (sendStatus.status == WalletModel::OK)
    {
        ui->labelTransaction->setText(tr("Transaction identifier: %1").arg(QString::fromStdString(transaction->getWtx()->GetHash().GetHex())));
        sendCompleted();
    } else {
        sendFailed();
    }
}

void SendFinish::showTransactionClicked() {
    switch (mode) {
    case Mode::FinishTransaction:
        assert(transaction != nullptr);
        Q_EMIT showTransaction(transaction->getWtx()->GetHash());
        break;
    case Mode::FinishPsbt:
        assert(false);
        break;
    }
}

void SendFinish::newTransactionClicked() {
    Q_EMIT goNewTransaction();
}
