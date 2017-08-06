// Copyright (c) 2015-2016 The Bitcoin Unlimited developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "receivefreezedialog.h"
#include "ui_receivefreezedialog.h"

#include "guiconstants.h"
#include "guiutil.h"
#include "optionsmodel.h"
#include "main.h"

ReceiveFreezeDialog::ReceiveFreezeDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ReceiveFreezeDialog),
    model(0)
{
    ui->setupUi(this);

    ui->freezeDateTime->setMinimumDateTime(QDateTime::currentDateTime());
    ui->freezeDateTime->setDisplayFormat("yyyy/MM/dd hh:mm");
    ui->freezeDateTime->setDateTime(QDateTime::currentDateTime());
  
    ui->freezeBlock->setMinimum(chainActive.Height());

    on_resetButton_clicked();

    connect(this, SIGNAL(accepted()), parent, SLOT(on_freezeDialog_hide()));
    connect(this, SIGNAL(rejected()), parent, SLOT(on_freezeDialog_hide()));

}

ReceiveFreezeDialog::~ReceiveFreezeDialog()
{
    delete ui;
}

void ReceiveFreezeDialog::setModel(OptionsModel *model)
{
    this->model = model;
}

void ReceiveFreezeDialog::on_freezeDateTime_editingFinished()
{
    if (ui->freezeDateTime->dateTime() > ui->freezeDateTime->minimumDateTime())
        ui->freezeBlock->clear();
}

void ReceiveFreezeDialog::on_freezeBlock_editingFinished()
{
    if (ui->freezeBlock->value() > 0)
      ui->freezeDateTime->setDateTime(ui->freezeDateTime->minimumDateTime());

    /* limit check */
    std::string freezeText = ui->freezeBlock->text().toStdString();
    int64_t nFreezeLockTime = 0;
    if (freezeText != "") nFreezeLockTime = std::strtoul(freezeText.c_str(),0,10);
    if (nFreezeLockTime < 1 || nFreezeLockTime > LOCKTIME_THRESHOLD-1)
            ui->freezeBlock->clear();
}

void ReceiveFreezeDialog::on_resetButton_clicked()
{
    ui->freezeBlock->clear();
    ui->freezeDateTime->setDateTime(ui->freezeDateTime->minimumDateTime());
}

void ReceiveFreezeDialog::on_okButton_clicked()
{
    accept();
}

void ReceiveFreezeDialog::on_ReceiveFreezeDialog_rejected()
{
    // this signal is also called by the ESC key
    on_resetButton_clicked();
}

void ReceiveFreezeDialog::getFreezeLockTime(CScriptNum &nFreezeLockTime)
{
    nFreezeLockTime = CScriptNum(0);

    // try freezeBlock
    std::string freezeText = ui->freezeBlock->text().toStdString();
    if (freezeText != "")
        nFreezeLockTime = CScriptNum(std::strtoul(freezeText.c_str(),0,10));

    else
    {
        // try freezeDateTime
        if (ui->freezeDateTime->dateTime() > ui->freezeDateTime->minimumDateTime())
            nFreezeLockTime = CScriptNum(ui->freezeDateTime->dateTime().toMSecsSinceEpoch() / 1000);
    }

}




