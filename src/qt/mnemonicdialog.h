// Copyright (c) 2017-2018 The Particl Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PARTICL_QT_MNEMONICDIALOG_H
#define PARTICL_QT_MNEMONICDIALOG_H

#include <QDialog>

class WalletModel;

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

protected Q_SLOTS:

Q_SIGNALS:
    // Rescan blockchain for transactions
    void startRescan();

private Q_SLOTS:
    void on_btnCancel_clicked();
    void on_btnImport_clicked();
    void on_btnGenerate_clicked();
    void on_btnImportFromHwd_clicked();

private:
    Ui::MnemonicDialog *ui;
};

#endif // PARTICL_QT_MNEMONICDIALOG_H
