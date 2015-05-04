// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bitcoin_transactiondescdialog.h"
#include "ui_bitcoin_transactiondescdialog.h"

#include "bitcoin_transactiontablemodel.h"

#include <QModelIndex>

Bitcoin_TransactionDescDialog::Bitcoin_TransactionDescDialog(const QModelIndex &idx, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Bitcoin_TransactionDescDialog)
{
    ui->setupUi(this);
    QString desc = idx.data(Bitcoin_TransactionTableModel::LongDescriptionRole).toString();
    ui->detailText->setHtml(desc);
}

Bitcoin_TransactionDescDialog::~Bitcoin_TransactionDescDialog()
{
    delete ui;
}
