// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_CREATEWALLETDIALOG_H
#define BITCOIN_QT_CREATEWALLETDIALOG_H

#include <qt/walletcontroller.h>

#include <QDialog>

class WalletModel;

namespace Ui {
    class CreateWalletDialog;
}

/** Dialog for creating wallets
 */
class CreateWalletDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CreateWalletDialog(QWidget *parent, WalletController *wallet_controller);
    virtual ~CreateWalletDialog();

    void accept();
    void WalletNameChanged(const QString& text);

private:
    Ui::CreateWalletDialog *ui;
    WalletController *m_wallet_controller;
};

#endif // BITCOIN_QT_CREATEWALLETDIALOG_H
