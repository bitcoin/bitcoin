// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TRANSACTIONDESCDIALOG_H
#define TRANSACTIONDESCDIALOG_H

#include <QDialog>

namespace Ui {
    class Credits_TransactionDescDialog;
}

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

/** Dialog showing transaction details. */
class Credits_TransactionDescDialog : public QDialog
{
    Q_OBJECT

public:
    explicit Credits_TransactionDescDialog(const QModelIndex &idx, QWidget *parent = 0);
    ~Credits_TransactionDescDialog();

private:
    Ui::Credits_TransactionDescDialog *ui;
};

#endif // TRANSACTIONDESCDIALOG_H
