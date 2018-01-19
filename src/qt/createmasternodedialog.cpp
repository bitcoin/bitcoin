#include "createmasternodedialog.h"
#include "ui_createmasternodedialog.h"
#include "ui_interface.h"
#include "net.h"
#include "masternodeconfig.h"

CreateMasternodeDialog::CreateMasternodeDialog(QWidget *parent) :
    QDialog(parent),
    editMode(false),
    startAlias(""),
    ui(new Ui::CreateMasternodeDialog)
{
    ui->setupUi(this);
}

CreateMasternodeDialog::~CreateMasternodeDialog()
{
    delete ui;
}

QString CreateMasternodeDialog::getAlias()
{
    return ui->aliasEdit->text();
}

void CreateMasternodeDialog::setAlias(QString alias)
{
    ui->aliasEdit->setText(alias);
    startAlias = alias;
}

QString CreateMasternodeDialog::getIP()
{
    return ui->ipEdit->text();
}

void CreateMasternodeDialog::setIP(QString ip)
{
    ui->ipEdit->setText(ip);
}

QString CreateMasternodeDialog::getLabel()
{
    return ui->labelEdit->text();
}

void CreateMasternodeDialog::setNoteLabel(QString text)
{
    ui->noteLabel->setText(text);
}

void CreateMasternodeDialog::setEditMode()
{
    ui->labelEdit->setVisible(false);
    ui->label->setVisible(false);
    ui->buttonBox->move(ui->buttonBox->pos().x(), 
                        ui->buttonBox->pos().y() - 50);
    resize(size().width(), size().height() - 50);
    editMode = true;
}

void CreateMasternodeDialog::accept()
{
    // Check alias
    if (ui->aliasEdit->text().isEmpty())
    {
        //ui->errorLabel->setText("Alias is Required");
        ui->aliasEdit->setValid(false);
        ui->aliasEdit->setPlaceholderText("Alias is Required");
        return;
    }
    // Check if alias exists
    if (masternodeConfig.aliasExists(ui->aliasEdit->text().toStdString()))
    {
        QString aliasEditText = ui->aliasEdit->text();
        if (!(startAlias != "" && aliasEditText == startAlias))
        {
            //ui->errorLabel->setText("Alias '" + ui->aliasEdit->text() + "' Already Exists");
            ui->aliasEdit->setValid(false);
            return;
        }
    }
    QString ip = ui->ipEdit->text();
    // Check ip
    if (ip.isEmpty())
    {
        ui->ipEdit->setPlaceholderText("IP is Required");
        ui->ipEdit->setValid(false);
        return;
    }
    // Check if port is not entered
    if (ip.contains(QRegExp(":+[0-9]")))
    {
        //ui->errorLabel->setText("Enter IP Without Port");
        ui->ipEdit->setValid(false);
        return;
    }
    // Validate ip address
    // This is only for validation so port doesn't matter
    if (!(CService(ip.toStdString() + ":9340").IsIPv4() && CService(ip.toStdString()).IsRoutable())) {
        //ui->errorLabel->setText("Invalid IP Address. IPV4 ONLY");
        ui->ipEdit->setValid(false);
        return;
    }

    QDialog::accept();
}
