// Copyright (c) 2011-2015 The Bitcoin Core developers
// Copyright (c) 2015-2016 The Bitcoin Unlimited developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "receivefreezedialog.h"
#include "ui_receivefreezedialog.h"

#include "guiconstants.h"
#include "guiutil.h"
#include "optionsmodel.h"
#include "walletmodel.h"



#include <QClipboard>
#include <QDrag>
#include <QMenu>
#include <QMimeData>
#include <QMouseEvent>
#include <QPixmap>
#if QT_VERSION < 0x050000
#include <QUrl>
#endif


ReceiveFreezeDialog::ReceiveFreezeDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ReceiveFreezeDialog),
    model(0)
{
    ui->setupUi(this);

    //connect(ui->clearButton, SIGNAL(clicked()), this, SLOT(clear()));

}

ReceiveFreezeDialog::~ReceiveFreezeDialog()
{
    delete ui;
}

void ReceiveFreezeDialog::setModel(OptionsModel *model)
{
    this->model = model;

}

void ReceiveFreezeDialog::clear()
{
    ui->freezeBlock->clear();
    ui->freezeDateTime->setDateTime(QDateTime::fromMSecsSinceEpoch(0));
}


void ReceiveFreezeDialog::on_freezeDateTime_editingFinished()
{
    if (ui->freezeDateTime->dateTime() > QDateTime::fromMSecsSinceEpoch(0))
        ui->freezeBlock->clear();
}

void ReceiveFreezeDialog::on_freezeBlock_editingFinished()
{
    if (ui->freezeBlock->text() != "")
        ui->freezeDateTime->setDateTime(QDateTime::fromMSecsSinceEpoch(0));

    /* limit check */
    std::string freezeText = ui->freezeBlock->text().toStdString();
    int64_t nFreezeLockTime = 0;
    if (freezeText != "") nFreezeLockTime = std::strtoul(freezeText.c_str(),0,10);
    if (nFreezeLockTime < 1 || nFreezeLockTime > LOCKTIME_THRESHOLD-1)
            ui->freezeBlock->clear();
}
