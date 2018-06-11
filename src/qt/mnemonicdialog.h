// Copyright (c) 2017-2018 The Particl Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PARTICL_QT_MNEMONICDIALOG_H
#define PARTICL_QT_MNEMONICDIALOG_H

#include <QDialog>
#include <QThread>

class RPCExecutor2 : public QObject
{
    Q_OBJECT
public Q_SLOTS:
    void request(const QString &command, const QString &walletID);
Q_SIGNALS:
    void reply(bool category, const QString &reply);
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

    QThread *m_thread = nullptr;

public:
    explicit MnemonicDialog(QWidget *parent, WalletModel *wm);
    ~MnemonicDialog();

public Q_SLOTS:
    void hwImportComplete(bool passed, QString reply);

Q_SIGNALS:
    // Rescan blockchain for transactions
    void startRescan();

    // For RPC command executor
    void stopExecutor();
    void cmdRequest(const QString &command, const QString &walletID);

private Q_SLOTS:
    void on_btnCancel_clicked();
    void on_btnImport_clicked();
    void on_btnGenerate_clicked();
    void on_btnImportFromHwd_clicked();

private:
    Ui::MnemonicDialog *ui;
};

#endif // PARTICL_QT_MNEMONICDIALOG_H
