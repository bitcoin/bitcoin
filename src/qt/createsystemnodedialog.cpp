#include "createsystemnodedialog.h"
#include "ui_createsystemnodedialog.h"
#include "ui_interface.h"
#include "net.h"
#include "systemnodeconfig.h"
#include <QMessageBox>

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
    return ui->aliasEdit->text();
}

void CreateSystemnodeDialog::setAlias(QString alias)
{
    ui->aliasEdit->setText(alias);
    startAlias = alias;
}

QString CreateSystemnodeDialog::getIP()
{
    return ui->ipEdit->text();
}

void CreateSystemnodeDialog::setIP(QString ip)
{
    ui->ipEdit->setText(ip);
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

void CreateSystemnodeDialog::accept()
{
    // Check alias
    if (ui->aliasEdit->text().isEmpty())
    {
        ui->aliasEdit->setValid(false);
        QMessageBox::warning(this, windowTitle(), tr("Alias is Required"), QMessageBox::Ok, QMessageBox::Ok);
        return;
    }
    // Check if alias exists
    if (systemnodeConfig.aliasExists(ui->aliasEdit->text().toStdString()))
    {
        QString aliasEditText = ui->aliasEdit->text();
        if (!(startAlias != "" && aliasEditText == startAlias))
        {
            ui->aliasEdit->setValid(false);
            QMessageBox::warning(this, windowTitle(), tr("Alias %1 Already Exists").arg(ui->aliasEdit->text()), 
                    QMessageBox::Ok, QMessageBox::Ok);
            return;
        }
    }
    QString ip = ui->ipEdit->text();
    // Check ip
    if (ip.isEmpty())
    {
        ui->ipEdit->setValid(false);
        QMessageBox::warning(this, windowTitle(), tr("IP is Required"), QMessageBox::Ok, QMessageBox::Ok);
        return;
    }
    // Check if port is not entered
    if (ip.contains(QRegExp(":+[0-9]")))
    {
        ui->ipEdit->setValid(false);
        QMessageBox::warning(this, windowTitle(), tr("Enter IP Without Port"), QMessageBox::Ok, QMessageBox::Ok);
        return;
    }
    // Validate ip address
    // This is only for validation so port doesn't matter
    if (!(CService(ip.toStdString() + ":9340").IsIPv4() && CService(ip.toStdString()).IsRoutable())) {
        ui->ipEdit->setValid(false);
        QMessageBox::warning(this, windowTitle(), tr("Invalid IP Address. IPV4 ONLY"), QMessageBox::Ok, QMessageBox::Ok);
        return;
    }

    QDialog::accept();
}
