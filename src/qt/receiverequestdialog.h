// Copyright (c) 2011-2018 The Talkcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TALKCOIN_QT_RECEIVEREQUESTDIALOG_H
#define TALKCOIN_QT_RECEIVEREQUESTDIALOG_H

#include <qt/walletmodel.h>

#include <QDialog>

namespace Ui {
    class ReceiveRequestDialog;
}

class ReceiveRequestDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ReceiveRequestDialog(QWidget *parent = nullptr);
    ~ReceiveRequestDialog();

    void setModel(WalletModel *model);
    void setInfo(const SendCoinsRecipient &info);

private Q_SLOTS:
    void on_btnCopyURI_clicked();
    void on_btnCopyAddress_clicked();

    void update();

private:
    Ui::ReceiveRequestDialog *ui;
    WalletModel *model;
    SendCoinsRecipient info;
};

#endif // TALKCOIN_QT_RECEIVEREQUESTDIALOG_H
