// Copyright (c) 2017-2018 The Particl Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/mnemonicdialog.h>
#include <qt/forms/ui_mnemonicdialog.h>

#include <qt/guiutil.h>

#include <qt/walletmodel.h>

#include <interfaces/wallet.h>

#include <rpc/rpcutil.h>
#include <util.h>
#include <univalue.h>
#include <key/mnemonic.h>
#include <key/extkey.h>

#include <QDebug>

void RPCExecutor2::request(const QString &command, const QString &walletID)
{
    bool passed = false;
    UniValue rv;
    CallRPCVoidRv(command.toStdString(), walletID.toStdString(), &passed, &rv);

    QString sError;
    if (rv["Error"].isStr())
        sError = QString::fromStdString(rv["Error"].get_str());
    else
        sError = QString::fromStdString(rv.write(1));

    Q_EMIT reply(passed, sError);   // Can't pass back a UniValue or signal won't get detected ?
};

MnemonicDialog::MnemonicDialog(QWidget *parent, WalletModel *wm) :
    QDialog(parent), walletModel(wm),
    ui(new Ui::MnemonicDialog)
{
    setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
    ui->setupUi(this);

    QObject::connect(ui->btnCancel2, SIGNAL(clicked()), this, SLOT(on_btnCancel_clicked()));
    QObject::connect(ui->btnCancel3, SIGNAL(clicked()), this, SLOT(on_btnCancel_clicked()));

    QObject::connect(this, SIGNAL(startRescan()), walletModel, SLOT(startRescan()), Qt::QueuedConnection);

#if QT_VERSION >= 0x040700
    ui->edtPath->setPlaceholderText(tr("Path to derive account from, if not using default. (optional, default=%1)").arg(QString::fromStdString(GetDefaultAccountPath())));
    ui->edtPassword->setPlaceholderText(tr("Enter a passphrase to protect your Recovery Phrase. (optional)"));
#if QT_VERSION >= 0x050200
    ui->tbxMnemonic->setPlaceholderText(tr("Enter your BIP39 compliant Recovery Phrase/Mnemonic."));
#endif
#endif

    if (!wm->wallet().isDefaultAccountSet())
    {
        ui->lblHelp->setText(
            "This wallet has no HD account loaded.\n"
            "An account must first be loaded in order to generate receiving addresses.\n"
            "Importing a recovery phrase will load a new master key and account.\n"
            "You can generate a new recovery phrase from the 'Create' page below.\n");
    } else
    {
        ui->lblHelp->setText(
            "This wallet already has an HD account loaded.\n"
            "By importing another recovery phrase a new account will be created and set as the default.\n"
            "The wallet will receive on addresses from the new and existing account/s.\n"
            "New addresses will be generated from the new account.\n");
    };

    ui->cbxLanguage->clear();
    for (int l = 1; l < WLL_MAX; ++l)
        ui->cbxLanguage->addItem(mnLanguagesDesc[l], QString(mnLanguagesTag[l]));

    return;
};

MnemonicDialog::~MnemonicDialog()
{
    if (m_thread)
    {
        Q_EMIT stopExecutor();
        m_thread->wait();
    };
};

void MnemonicDialog::on_btnCancel_clicked()
{
    close();
    return;
};

void MnemonicDialog::on_btnImport_clicked()
{
    QString sCommand = (ui->chkImportChain->checkState() == Qt::Unchecked)
        ? "extkeyimportmaster" : "extkeygenesisimport";
    sCommand += " \"" + ui->tbxMnemonic->toPlainText() + "\"";

    QString sPassword = ui->edtPassword->text();
    sCommand += " \"" + sPassword + "\" false \"Master Key\" \"Default Account\" -1";

    UniValue rv;
    if (walletModel->tryCallRpc(sCommand, rv))
    {
        close();
        if (!rv["warnings"].isNull())
        {
            for (size_t i = 0; i < rv["warnings"].size(); ++i)
                walletModel->warningBox(tr("Import"), QString::fromStdString(rv["warnings"][i].get_str()));
        };
        startRescan();
    };

    return;
};

void MnemonicDialog::on_btnGenerate_clicked()
{
    int nBytesEntropy = ui->spinEntropy->value();
    QString sLanguage = ui->cbxLanguage->itemData(ui->cbxLanguage->currentIndex()).toString();

    QString sCommand = "mnemonic new  \"\" " + sLanguage + " " + QString::number(nBytesEntropy);

    UniValue rv;
    if (walletModel->tryCallRpc(sCommand, rv))
    {
        ui->tbxMnemonicOut->setText(QString::fromStdString(rv["mnemonic"].get_str()));
    };

    return;
};

void MnemonicDialog::on_btnImportFromHwd_clicked()
{
    if (m_thread)
        return;
    QString sCommand = "initaccountfromdevice \"From Hardware Device\"";

    QString sPath = ui->edtPath->text();
    sCommand += " \"" + sPath + "\" true -1";

    ui->tbxHwdOut->appendPlainText("Waiting for device.");
    setEnabled(false);

    m_thread = new QThread;
    RPCExecutor2 *executor = new RPCExecutor2();
    executor->moveToThread(m_thread);

    connect(executor, SIGNAL(reply(bool,QString)), this, SLOT(hwImportComplete(bool,QString)));         // Replies from executor object must go to this object
    connect(this, SIGNAL(cmdRequest(QString, QString)), executor, SLOT(request(QString, QString)));     // Requests from this object must go to executor

    connect(this, SIGNAL(stopExecutor()), m_thread, SLOT(quit()));                                      // - quit the Qt event loop in the execution thread
    connect(m_thread, SIGNAL(finished()), executor, SLOT(deleteLater()), Qt::DirectConnection);         // - queue executor for deletion (in execution thread)
    m_thread->start();

    Q_EMIT cmdRequest(sCommand, walletModel->getWalletName());

    return;
};

void MnemonicDialog::hwImportComplete(bool passed, QString reply)
{
    setEnabled(true);

    Q_EMIT stopExecutor();
    m_thread->wait();
    m_thread = nullptr;

    if (!passed)
    {
        QString sError = reply;
        ui->tbxHwdOut->appendPlainText(sError);
        if (sError == "No device found."
            || sError.indexOf("6982") > -1)
            ui->tbxHwdOut->appendPlainText("Open particl app on device before importing.");
    } else
    {
        UniValue rv;
        QString sCommand = "devicegetnewstealthaddress \"default stealth\"";
        walletModel->tryCallRpc(sCommand, rv);
        close();

        startRescan();
    };

    return;
};
