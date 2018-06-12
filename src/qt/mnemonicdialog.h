// Copyright (c) 2017-2018 The Particl Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PARTICL_QT_MNEMONICDIALOG_H
#define PARTICL_QT_MNEMONICDIALOG_H

#include <QDialog>
#include <QThread>
#include <univalue.h>

class RPCThread : public QThread
{
    Q_OBJECT
public:
    RPCThread(const QString &command, const QString &walletID, UniValue *rv)
        : QThread(), m_command(command), m_wallet(walletID), m_rv(rv) {};
    void run() override;
    QString m_command;
    QString m_wallet;
    UniValue *m_rv;
Q_SIGNALS:
    void complete(bool passed);
};

class WalletModel;

namespace Ui {
    class MnemonicDialog;
}

class MnemonicDialog : public QDialog
{
    Q_OBJECT
private:
    WalletModel *walletModel;

    RPCThread *m_thread = nullptr;
    UniValue m_rv;

public:
    explicit MnemonicDialog(QWidget *parent, WalletModel *wm);
    ~MnemonicDialog();

public Q_SLOTS:
    void hwImportComplete(bool passed);

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
