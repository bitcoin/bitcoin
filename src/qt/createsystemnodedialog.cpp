#include "createsystemnodedialog.h"
#include "ui_createsystemnodedialog.h"
#include "ui_interface.h"
#include "net.h"
#include "systemnodeconfig.h"

CreateSystemnodeDialog::CreateSystemnodeDialog(QWidget *parent) :
    QDialog(parent),
    editMode(false),
    startAlias(""),
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

void CreateSystemnodeDialog::setAlias(QString alias)
{
    ui->alias->setText(alias);
    startAlias = alias;
}

QString CreateSystemnodeDialog::getIP()
{
    return ui->ip->text();
}

void CreateSystemnodeDialog::setIP(QString ip)
{
    ui->ip->setText(ip);
}

QString CreateSystemnodeDialog::getLabel()
{
    return ui->labelEdit->text();
}

void CreateSystemnodeDialog::setNoteLabel(QString text)
{
    ui->noteLabel->setText(text);
}

void CreateSystemnodeDialog::setEditMode()
{
    ui->labelEdit->setVisible(false);
    ui->label->setVisible(false);
    ui->buttonBox->move(ui->buttonBox->pos().x(), 
                        ui->buttonBox->pos().y() - 50);
    resize(size().width(), size().height() - 50);
    editMode = true;
}

void CreateSystemnodeDialog::done(int r)
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
        if (systemnodeConfig.aliasExists(ui->alias->text().toStdString()))
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
