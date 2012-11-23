#include "importprivatekeydialog.h"
#include "ui_importprivatekeydialog.h"
#include "walletmodel.h"

#include "version.h"

ImportPrivateKeyDialog::ImportPrivateKeyDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ImportPrivateKeyDialog)
{
    ui->setupUi(this);
}

void ImportPrivateKeyDialog::setModel(WalletModel *model)
{
    if(model)
    {
        //ui->versionLabel->setText(model->formatFullVersion());
    }
}

ImportPrivateKeyDialog::~ImportPrivateKeyDialog()
{
    delete ui;
}

void ImportPrivateKeyDialog::on_buttonBox_accepted()
{
    close();
}
