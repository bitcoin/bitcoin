// Copyright (c) 2017 The Particl Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "mnemonicdialog.h"
#include "ui_mnemonicdialog.h"

#include "guiutil.h"

#ifdef ENABLE_WALLET
#include "walletmodel.h"
#include "wallet/hdwallet.h"
#endif

#include "rpc/rpcutil.h"
#include "util.h"
#include "univalue.h"

#include <QDebug>


MnemonicDialog::MnemonicDialog(QWidget *parent, WalletModel *wm) :
    QDialog(parent), walletModel(wm),
    ui(new Ui::MnemonicDialog)
{
    ui->setupUi(this);
}

MnemonicDialog::~MnemonicDialog()
{
    
}

void MnemonicDialog::on_btnCancel_clicked()
{
    close();
}

void MnemonicDialog::on_btnImport_clicked()
{
#ifdef ENABLE_WALLET
    
    QString sCommand = (ui->chkImportChain->checkState() == Qt::Unchecked)
        ? "extkeyimportmaster" : "extkeygenesisimport";
    sCommand += " \"" + ui->tbxMnemonic->toPlainText() + "\"";
    
    QString sPassword = ui->edtPassword->text();
    if (!sPassword.isEmpty())
        sCommand += " \"" + sPassword + "\"";
   
    UniValue rv;
    if (tryCallRpc(sCommand, rv))
    {
        
        close();
    }
#endif
}

void MnemonicDialog::on_btnGenerate_clicked()
{
#ifdef ENABLE_WALLET
    
    int nBytesEntropy = ui->spinEntropy->value();
    QString sLanguage = ui->cbxLanguage->currentText().toLower();;
    
    QString sCommand = "mnemonic new  \"\" " + sLanguage + " " + QString::number(nBytesEntropy);
    
    UniValue rv;
    if (tryCallRpc(sCommand, rv))
    {
        QMessageBox::information(this, tr("New Seed"),
            QString::fromStdString(rv["mnemonic"].get_str()),
            QMessageBox::Ok,
            QMessageBox::Ok);
    }
    
#endif
}

bool MnemonicDialog::tryCallRpc(const QString &sCommand, UniValue &rv)
{
    try {
        rv = CallRPC(sCommand.toStdString());
    } catch (UniValue& objError)
    {
        try { // Nice formatting for standard-format error
            int code = find_value(objError, "code").get_int();
            std::string message = find_value(objError, "message").get_str();
            warningBox(QString::fromStdString(message) + " (code " + QString::number(code) + ")");
            return false;
        } catch (const std::runtime_error&) // raised when converting to invalid type, i.e. missing code or message
        {   // Show raw JSON object
            warningBox(QString::fromStdString(objError.write()));
            return false;
        };
    } catch (const std::exception& e)
    {
        warningBox(QString::fromStdString(e.what()));
        return false;
    };
    
    return true;
};

void MnemonicDialog::warningBox(QString msg)
{
    qWarning() << msg;
#ifdef ENABLE_WALLET
    QPair<QString, CClientUIInterface::MessageBoxFlags> msgParams;
    msgParams.second = CClientUIInterface::MSG_WARNING;
    msgParams.first = msg;

    Q_EMIT walletModel->message(tr("HD Wallet Dialog"), msgParams.first, msgParams.second);
#endif
}

