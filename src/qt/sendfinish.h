// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_SENDFINISH_H
#define BITCOIN_QT_SENDFINISH_H

#include <qt/walletmodel.h>

#include <QWidget>

class WalletModelTransaction;

namespace Ui {
    class SendFinish;
}

class SendFinish : public QWidget
{
    Q_OBJECT

public:
    explicit SendFinish(const PlatformStyle *platformStyle, QWidget* parent = nullptr);
    ~SendFinish();
    bool isActiveWidget();
    void setActiveWidget(bool active);

    std::unique_ptr<WalletModelTransaction> transaction;

    void setModel(WalletModel *walletModel);
    void broadcastTransaction();
    void createPsbt();
    void broadcastRbf(uint256 txid, CMutableTransaction mtx);

private Q_SLOTS:
    void showTransactionClicked();
    void newTransactionClicked();

Q_SIGNALS:
    // Fired when a message should be reported to the user
    void message(const QString &title, const QString &message, unsigned int style);

    // Fired when send fails
    void sendFailed();

    // Fired when send succeeds
    void sendCompleted();

    // Fired when RBF send fails
    void rbfSendFailed(uint256 txid);

    // Fired when RBF send succeeds
    void rbfCompleted();

    // Fired when user asks to see transaction
    void showTransaction(uint256 txid);

    // Fired when user wants to create a new transaction
    void goNewTransaction();

private:
    bool m_is_active_widget{false};
    enum Mode {
        FinishTransaction = 0,
        FinishPsbt = 1,
        FinishRbf = 2
    };

    Ui::SendFinish *ui;
    Mode mode;
    WalletModel *walletModel;

    // RBF:
    std::shared_ptr<CMutableTransaction> mtx;
    uint256 txid;
    uint256 new_hash;

    void prepareForm();

    // Process WalletModel::SendCoinsReturn and generate a pair consisting
    // of a message and message flags for use in Q_EMIT message().
    // Additional parameter msgArg can be used via .arg(msgArg).
    void processSendCoinsReturn(const WalletModel::SendCoinsReturn &sendCoinsReturn, const QString &msgArg = QString());
};

#endif // BITCOIN_QT_SENDFINISH_H
