// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_SENDDIALOG_H
#define BITCOIN_QT_SENDDIALOG_H

#include <QtWidgets/QTabWidget>

#include <qt/clientmodel.h>
#include <qt/walletmodel.h>

class CCoinControl;
class PlatformStyle;
class SendCompose;
class SendSign;
class SendFinish;

namespace Ui {
class SendDialog;
}

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

/** Dialog showing transaction details. */
class SendDialog : public QTabWidget
{
    Q_OBJECT

public:
    QTabWidget *tabWidget;

    // Correspond 1:1 to tab IDs
    enum WorkflowState {
        Draft = 0,
        Sign = 1,
        Finish = 2,
    };

    explicit SendDialog(const PlatformStyle *platformStyle, QWidget *parent = nullptr);
    ~SendDialog();

    void setWorkflowState(enum WorkflowState);

    void setClientModel(ClientModel *clientModel);
    void setModel(WalletModel *model);
    void setAddress(const QString &address);
    bool handlePaymentRequest(const SendCoinsRecipient &recipient);
    void signCancelled();
    void signConfirmed();
    void rbfCancelled(uint256 txid);
    void rbfConfirmed(uint256 txid, CMutableTransaction mtx);
    void sendFailed();
    void sendCompleted();
    void rbfSendFailed(uint256 txid);

    // Also called when creating a PSBT in a watch-only wallet:
    void gotoSign();

    void goNewTransaction();
    void gotoBumpFee(const uint256& txid);

// public Q_SLOTS:


    SendCompose *draftWidget;
    SendSign *signWidget;
    SendFinish *finishWidget;

private:
    WorkflowState m_workflow_state;
    Ui::SendDialog* ui;
    const PlatformStyle *platformStyle;
    WalletModel* walletModel;
    ClientModel* clientModel;

    // std::unique_ptr<WalletModelTransaction> transaction;
    // std::unique_ptr<CCoinControl> coinControl;

Q_SIGNALS:
    // Fired when a message should be reported to the user
    void message(const QString &title, const QString &message, unsigned int style);

    void showTransaction(const uint256& txid);
};

#endif // BITCOIN_QT_SENDDIALOG_H
