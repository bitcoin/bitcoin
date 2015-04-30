// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "transactiondescdialog.h"
#include "ui_transactiondescdialog.h"

#include "transactiontablemodel.h"

#include <QModelIndex>

Credits_TransactionDescDialog::Credits_TransactionDescDialog(const QModelIndex &idx, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Credits_TransactionDescDialog)
{
    ui->setupUi(this);
    QString desc = idx.data(Bitcredit_TransactionTableModel::LongDescriptionRole).toString();
    ui->detailText->setHtml(desc);
}

Credits_TransactionDescDialog::~Credits_TransactionDescDialog()
{
    delete ui;
}
