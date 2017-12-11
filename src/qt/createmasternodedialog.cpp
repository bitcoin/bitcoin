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
    return ui->alias->text();
}

void CreateMasternodeDialog::setAlias(QString alias)
{
    ui->alias->setText(alias);
    startAlias = alias;
}

QString CreateMasternodeDialog::getIP()
{
    return ui->ip->text();
}

void CreateMasternodeDialog::setIP(QString ip)
{
    ui->ip->setText(ip);
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

void CreateMasternodeDialog::done(int r)
{
    ui->errorLabel->setStyleSheet("color: #FF0000");
    if (QDialog::Accepted == r)
    {
        // Check alias
        if (ui->alias->text().isEmpty())
        {
            ui->errorLabel->setText("Alias is Required");
            return;
        }
        // Check if alias exists
        if (masternodeConfig.aliasExists(ui->alias->text().toStdString()))
        {
            if (!(startAlias != "" && ui->alias->text() == startAlias))
            {
                ui->errorLabel->setText("Alias '" + ui->alias->text() + "' Already Exists");
                return;
            }
        }
        QString ip = ui->ip->text();
        // Check ip
        if (ip.isEmpty())
        {
            ui->errorLabel->setText("IP is Required");
            return;
        }
        // Check if port is not entered
        if (ip.contains(QRegExp(":+[0-9]")))
        {
            ui->errorLabel->setText("Enter IP Without Port");
            return;
        }
        // Validate ip address
        // This is only for validation so port doesn't matter
        if (!(CService(ip.toStdString() + ":9340").IsIPv4() && CService(ip.toStdString()).IsRoutable())) {
            ui->errorLabel->setText("Invalid IP Address. IPV4 ONLY");
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
