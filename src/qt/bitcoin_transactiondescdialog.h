// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TRANSACTIONDESCDIALOG_H
#define BITCOIN_TRANSACTIONDESCDIALOG_H

#include <QDialog>

namespace Ui {
    class Bitcoin_TransactionDescDialog;
}

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

/** Dialog showing transaction details. */
class Bitcoin_TransactionDescDialog : public QDialog
{
    Q_OBJECT

public:
    explicit Bitcoin_TransactionDescDialog(const QModelIndex &idx, QWidget *parent = 0);
    ~Bitcoin_TransactionDescDialog();

private:
    Ui::Bitcoin_TransactionDescDialog *ui;
};

#endif // BITCOIN_TRANSACTIONDESCDIALOG_H
