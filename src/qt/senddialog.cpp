// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <qt/senddialog.h>
#include <qt/sendcompose.h>
#include <qt/sendsign.h>
#include <qt/sendfinish.h>

#include <qt/forms/ui_senddialog.h>
#include <qt/optionsmodel.h>
#include <qt/platformstyle.h>

#include <util/memory.h>
#include <wallet/coincontrol.h>

SendDialog::SendDialog(const PlatformStyle *_platformStyle, QWidget *parent) :
    QTabWidget(parent),
    ui(new Ui::SendDialog),
    platformStyle(_platformStyle),
    walletModel(nullptr),
    clientModel(nullptr)
{
    ui->setupUi(this);
    draftWidget = new SendCompose(platformStyle);
    signWidget = new SendSign(platformStyle);
    finishWidget = new SendFinish(platformStyle);

    addTab(draftWidget, "(1) Draft");
    addTab(signWidget, "(2) Sign");
    addTab(finishWidget, "(3) Finish");

    setWorkflowState(WorkflowState::Draft);

    // Provide draft widget with coin control
    draftWidget->coinControl = MakeUnique<CCoinControl>();

    // Pass through messages from tabs
    connect(draftWidget, &SendCompose::message, this, &SendDialog::message);
    connect(signWidget, &SendSign::message, this, &SendDialog::message);
    connect(finishWidget, &SendFinish::message, this, &SendDialog::message);

    // Sign button in Draft tab hands over to gotoSign()
    connect(draftWidget, &SendCompose::gotoSign, this, &SendDialog::gotoSign);

    // Handle cancelled signing
    connect(signWidget, &SendSign::signCancelled, this, &SendDialog::signCancelled);

    // Handle confirmed signing
    connect(signWidget, &SendSign::signConfirmed, this, &SendDialog::signConfirmed);

    // Handle cancelled RBF
    connect(signWidget, &SendSign::rbfCancelled, this, &SendDialog::rbfCancelled);

    // Handle confirmed RBF
    connect(signWidget, &SendSign::rbfConfirmed, this, &SendDialog::rbfConfirmed);

    // Handle failed RBF
    connect(finishWidget, &SendFinish::rbfSendFailed, this, &SendDialog::rbfCancelled);

    // Handle go to transactions page
    connect(finishWidget, &SendFinish::showTransaction, this, &SendDialog::showTransaction);

    // Handle start new transaction
    connect(finishWidget, &SendFinish::goNewTransaction, this, &SendDialog::goNewTransaction);

}

SendDialog::~SendDialog()
{
    delete ui;
}

void SendDialog::setWorkflowState(enum SendDialog::WorkflowState state)
{
    m_workflow_state = state;
    setCurrentIndex(state);
    for (auto i = 0; i < count(); ++i) {
        setTabEnabled(i, i == currentIndex());
    }
    this->draftWidget->setActiveWidget(state == WorkflowState::Draft);
    this->signWidget->setActiveWidget(state == WorkflowState::Sign);
    this->finishWidget->setActiveWidget(state == WorkflowState::Finish);
}

void SendDialog::setClientModel(ClientModel *_clientModel)
{
    this->clientModel = _clientModel;
    this->draftWidget->setClientModel(_clientModel);
}

void SendDialog::setModel(WalletModel *_model)
{
    this->walletModel = _model;
    this->draftWidget->setModel(_model);
    this->signWidget->setModel(_model);
    this->finishWidget->setModel(_model);
}

void SendDialog::setAddress(const QString &address) {
    setWorkflowState(WorkflowState::Draft);
    this->draftWidget->setAddress(address);
}

bool SendDialog::handlePaymentRequest(const SendCoinsRecipient &rv)
{
    setWorkflowState(WorkflowState::Draft);
    return this->draftWidget->handlePaymentRequest(rv);
}

void SendDialog::signCancelled() {
    assert(m_workflow_state == WorkflowState::Sign);
    // Clear transaction and coin control; draft widget will provide new ones:
    signWidget->transaction.reset();
    signWidget->coinControl.reset();

    setWorkflowState(WorkflowState::Draft);
    draftWidget->signCancelled();
}

void SendDialog::signConfirmed() {
    // Pass transaction to finish widget:
    finishWidget->transaction = std::move(signWidget->transaction);

    // Discard coin cointrol:
    signWidget->coinControl.reset();

    if (!walletModel->privateKeysDisabled()) {
        finishWidget->broadcastTransaction();
    } else {
        finishWidget->createPsbt();
    }
    setWorkflowState(WorkflowState::Finish);
}

void SendDialog::rbfCancelled(uint256 txid) {
    setWorkflowState(WorkflowState::Draft);
    Q_EMIT showTransaction(txid);
}

void SendDialog::rbfConfirmed(uint256 txid, CMutableTransaction mtx) {
    finishWidget->broadcastRbf(txid, mtx);
    setWorkflowState(WorkflowState::Finish);
}

void SendDialog::sendFailed() {
    assert(m_workflow_state == WorkflowState::Finish);
    // Clear transaction; draft widget will provide new one:
    finishWidget->transaction.reset();
    setWorkflowState(WorkflowState::Draft);
    draftWidget->signCancelled();
}

void SendDialog::sendCompleted() {
    draftWidget->signCompleted();
}

void SendDialog::gotoSign()
{
    signWidget->transaction = std::move(draftWidget->transaction);
    signWidget->coinControl = std::move(draftWidget->coinControl);
    setWorkflowState(WorkflowState::Sign);
    signWidget->prepareTransaction();
}

void SendDialog::goNewTransaction() {
    assert(m_workflow_state == WorkflowState::Finish);
    // Clear transaction; draft widget will provide new one:
    finishWidget->transaction.reset();
    setWorkflowState(WorkflowState::Draft);
}

void SendDialog::gotoBumpFee(const uint256& txid) {
    CCoinControl coin_control;
    coin_control.m_signal_bip125_rbf = true;
    std::vector<std::string> errors;
    CAmount old_fee;
    CAmount new_fee;
    CMutableTransaction mtx;
    if (!walletModel->wallet().createBumpTransaction(txid, coin_control, 0 /* totalFee */, errors, old_fee, new_fee, mtx)) {
        QMessageBox::critical(nullptr, tr("Fee bump error"), tr("Increasing transaction fee failed") + "<br />(" +
        (errors.size() ? QString::fromStdString(errors[0]) : "") +")");
        return;
    }
    setWorkflowState(WorkflowState::Sign);
    signWidget->prepareSignRbf(txid, mtx, old_fee, new_fee);
}
