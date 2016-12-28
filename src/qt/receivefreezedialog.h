// Copyright (c) 2015-2016 The Bitcoin Unlimited developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_RECEIVEFREEZEDIALOG_H
#define BITCOIN_QT_RECEIVEFREEZEDIALOG_H

#include "walletmodel.h"

#include <QDialog>
#include <QLabel>

class OptionsModel;

namespace Ui {
    class ReceiveFreezeDialog;
}

class ReceiveFreezeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ReceiveFreezeDialog(QWidget *parent = 0);
    ~ReceiveFreezeDialog();

    void setModel(OptionsModel *model);

public Q_SLOTS:
    void getFreezeLockTime(CScriptNum &nFreezeLockTime);
    void on_ReceiveFreezeDialog_rejected();

private Q_SLOTS:
    void on_freezeDateTime_editingFinished();
    void on_freezeBlock_editingFinished();
    void on_resetButton_clicked();
    void on_okButton_clicked();

private:
    Ui::ReceiveFreezeDialog *ui;
    OptionsModel *model;

};

#endif // BITCOIN_QT_RECEIVEFREEZEDIALOG_H
