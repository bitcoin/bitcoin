// Copyright (c) 2011-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_SENDSIGN_H
#define BITCOIN_QT_SENDSIGN_H

#include <qt/walletmodel.h>

#include <QDialog>
#include <QMessageBox>
#include <QAbstractButton>
#include <QTimer>

class CCoinControl;
struct CMutableTransaction;
class WalletModelTransaction;

namespace Ui {
    class SendSign;
}

#define SEND_CONFIRM_DELAY   3

enum Mode {
    SignTransaction = 0,
    CreatePsbt = 1,
    SignRbf = 2
};

class SendSign : public QWidget
{
    Q_OBJECT

public:
    explicit SendSign(const PlatformStyle *platformStyle, QWidget* parent = nullptr);
    ~SendSign();
    bool isActiveWidget();
    void setActiveWidget(bool active);
    void setModel(WalletModel *walletModel);

    std::unique_ptr<WalletModelTransaction> transaction;
    std::unique_ptr<CCoinControl> coinControl;

    // Prepares an usigned transaction, to be signed or turned into a PSBT
    void prepareTransaction();

    void prepareSignRbf(uint256 txid, CMutableTransaction mtx, CAmount old_fee, CAmount new_fee);

public Q_SLOTS:
    void cancel();
    void confirm();

private Q_SLOTS:
    void countDown();

Q_SIGNALS:
    // Fired when a message should be reported to the user
    void message(const QString &title, const QString &message, unsigned int style);

    // Fired when user cancels sign
    void signCancelled();

    // Fired when user confirms sign
    void signConfirmed();

    // Fired when user cancels RBF
    void rbfCancelled(uint256 txid);

    // Fired when user confirms RBF
    void rbfConfirmed(uint256 txid, CMutableTransaction mtx);

private:
    Ui::SendSign *ui;
    bool m_is_active_widget{false};
    Mode mode;
    QTimer countDownTimer;
    int secDelay;
    WalletModel *walletModel;

    // RBF:
    uint256 txid; // Original transaction
    CMutableTransaction mtx;

    void renderForm();
    void processSendCoinsReturn(const WalletModel::SendCoinsReturn &sendCoinsReturn, const QString &msgArg = QString());
};

#endif // BITCOIN_QT_SENDSIGN_H
