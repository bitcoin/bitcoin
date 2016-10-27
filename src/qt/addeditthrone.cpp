#include "addeditthrone.h"
#include "ui_addeditthrone.h"
#include "masternodeconfig.h"
#include "thronemanager.h"
#include "ui_thronemanager.h"

#include "util.h"

#include <QMessageBox>
#include <QClipboard>

AddEditThrone::AddEditThrone(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddEditThrone)
{
    ui->setupUi(this);

}

AddEditThrone::~AddEditThrone()
{
    delete ui;
}


void AddEditThrone::on_okButton_clicked()
{
    if(ui->aliasLineEdit->text() == "")
    {
        QMessageBox msg;
        msg.setText("Please enter an alias.");
        msg.exec();
        return;
    }
    else if(ui->addressLineEdit->text() == "")
    {
        QMessageBox msg;
        msg.setText("Please enter an ip address and port. (123.45.67.89:9340)");
        msg.exec();
        return;
    }
    else if(ui->privkeyLineEdit->text() == "")
    {
        QMessageBox msg;
        msg.setText("Please enter a throne private key. This can be found using the \"throne genkey\" command in the console.");
        msg.exec();
        return;
    }
    else if(ui->txhashLineEdit->text() == "")
    {
        QMessageBox msg;
        msg.setText("Please enter the transaction hash for the transaction that has 10 000 coins");
        msg.exec();
        return;
    }
    else if(ui->outputindexLineEdit->text() == "")
    {
        QMessageBox msg;
        msg.setText("Please enter a transaction output index. This can be found using the \"throne outputs\" command in the console.");
        msg.exec();
        return;
    }
    else
    {
        std::string sAlias = ui->aliasLineEdit->text().toStdString();
        std::string sAddress = ui->addressLineEdit->text().toStdString();
        std::string sThronePrivKey = ui->privkeyLineEdit->text().toStdString();
        std::string sTxHash = ui->txhashLineEdit->text().toStdString();
        std::string sOutputIndex = ui->outputindexLineEdit->text().toStdString();

        boost::filesystem::path pathConfigFile = GetDataDir() / "throne.conf";
        boost::filesystem::ofstream stream (pathConfigFile.string(), std::ios::out | std::ios::app);
        if (stream.is_open())
        {
            stream << sAlias << " " << sAddress << " " << sThronePrivKey << " " << sTxHash << " " << sOutputIndex << std::endl;
            stream.close();
        }
        masternodeConfig.add(sAlias, sAddress, sThronePrivKey, sTxHash, sOutputIndex);
        accept();
    }
}

void AddEditThrone::on_cancelButton_clicked()
{
    reject();
}

void AddEditThrone::on_AddEditAddressPasteButton_clicked()
{
    // Paste text from clipboard into recipient field
    ui->addressLineEdit->setText(QApplication::clipboard()->text());
}

void AddEditThrone::on_AddEditPrivkeyPasteButton_clicked()
{
    // Paste text from clipboard into recipient field
    ui->privkeyLineEdit->setText(QApplication::clipboard()->text());
}

void AddEditThrone::on_AddEditTxhashPasteButton_clicked()
{
    // Paste text from clipboard into recipient field
    ui->txhashLineEdit->setText(QApplication::clipboard()->text());
}