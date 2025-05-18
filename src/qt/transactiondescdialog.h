// Copyright (c) 2011-2020 The Tortoisecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TORTOISECOIN_QT_TRANSACTIONDESCDIALOG_H
#define TORTOISECOIN_QT_TRANSACTIONDESCDIALOG_H

#include <QDialog>

namespace Ui {
    class TransactionDescDialog;
}

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

/** Dialog showing transaction details. */
class TransactionDescDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TransactionDescDialog(const QModelIndex &idx, QWidget *parent = nullptr);
    ~TransactionDescDialog();

private:
    Ui::TransactionDescDialog *ui;
};

#endif // TORTOISECOIN_QT_TRANSACTIONDESCDIALOG_H
