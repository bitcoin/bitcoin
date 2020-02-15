// Copyright (c) 2011-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_RECEIVEREQUESTDIALOG_H
#define BITCOIN_QT_RECEIVEREQUESTDIALOG_H

#include <qt/sendcoinsrecipient.h>

#include <QDialog>

class WalletModel;

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

#endif // BITCOIN_QT_RECEIVEREQUESTDIALOG_H
