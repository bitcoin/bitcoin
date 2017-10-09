// Copyright (c) 2017 The Particl Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PARTICL_QT_MNEMONICDIALOG_H
#define PARTICL_QT_MNEMONICDIALOG_H

#include <QDialog>

class WalletModel;
class UniValue;

namespace Ui {
    class MnemonicDialog;
}

class MnemonicDialog : public QDialog
{
    Q_OBJECT
private:
    WalletModel *walletModel;

public:
    explicit MnemonicDialog(QWidget *parent, WalletModel *wm);
    ~MnemonicDialog();

protected Q_SLOTS:

private Q_SLOTS:
    void on_btnCancel_clicked();
    void on_btnCancel2_clicked();
    void on_btnImport_clicked();
    void on_btnGenerate_clicked();

private:
    Ui::MnemonicDialog *ui;
};

#endif // PARTICL_QT_MNEMONICDIALOG_H
