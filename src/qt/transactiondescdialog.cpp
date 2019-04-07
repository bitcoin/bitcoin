// Copyright (c) 2011-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/transactiondescdialog.h>
#include <qt/forms/ui_transactiondescdialog.h>

#include <qt/guiutil.h>
#include <qt/transactiontablemodel.h>

#include <QModelIndex>

TransactionDescDialog::TransactionDescDialog(const QModelIndex &idx, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TransactionDescDialog)
{
    ui->setupUi(this);
    setWindowTitle(tr("Details for %1").arg(idx.data(TransactionTableModel::TxHashRole).toString()));
    QString desc = idx.data(TransactionTableModel::LongDescriptionRole).toString();
    ui->detailText->setHtml(desc);

    GUIUtil::handleCloseWindowShortcut(this);
}

TransactionDescDialog::~TransactionDescDialog()
{
    delete ui;
}
