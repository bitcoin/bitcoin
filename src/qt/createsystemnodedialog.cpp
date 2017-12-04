#include "createsystemnodedialog.h"
#include "ui_createsystemnodedialog.h"
#include "ui_interface.h"

CreateSystemnodeDialog::CreateSystemnodeDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CreateSystemnodeDialog)
{
    ui->setupUi(this);
}

CreateSystemnodeDialog::~CreateSystemnodeDialog()
{
    delete ui;
}

QString CreateSystemnodeDialog::getAlias()
{
    return ui->alias->text();
}

QString CreateSystemnodeDialog::getIP()
{
    return ui->ip->text();
}

QString CreateSystemnodeDialog::getLabel()
{
    return ui->labelEdit->text();
}

#include <iostream>
void CreateSystemnodeDialog::done(int r)
{
    ui->errorLabel->setStyleSheet("color: #FF0000");
    if (QDialog::Accepted == r)
    {
        // 1. Check alias
        if (ui->alias->text().isEmpty())
        {
            ui->errorLabel->setText("Alias is required");
            return;
        }
        // 2. Check ip
        if (ui->ip->text().isEmpty())
        {
            ui->errorLabel->setText("IP is required");
            return;
        }
        QDialog::done(r);
        return;
    }
    else
    {
        QDialog::done(r);
        return;
    }
}
