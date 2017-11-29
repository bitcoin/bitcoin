#include "createsystemnodedialog.h"
#include "ui_createsystemnodedialog.h"

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
