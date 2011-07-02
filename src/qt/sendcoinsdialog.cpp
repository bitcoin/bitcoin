#include "sendcoinsdialog.h"
#include "ui_sendcoinsdialog.h"
#include "walletmodel.h"
#include "guiutil.h"

#include "addressbookdialog.h"
#include "optionsmodel.h"

#include <QApplication>
#include <QClipboard>
#include <QMessageBox>
#include <QLocale>
#include <QDebug>

SendCoinsDialog::SendCoinsDialog(QWidget *parent, const QString &address) :
    QDialog(parent),
    ui(new Ui::SendCoinsDialog),
    model(0)
{
    ui->setupUi(this);

    GUIUtil::setupAddressWidget(ui->payTo, this);

    // Set initial send-to address if provided
    if(!address.isEmpty())
    {
        ui->payTo->setText(address);
        ui->payAmount->setFocus();
    }
}

void SendCoinsDialog::setModel(WalletModel *model)
{
    this->model = model;
}

SendCoinsDialog::~SendCoinsDialog()
{
    delete ui;
}

void SendCoinsDialog::on_sendButton_clicked()
{
    bool valid;
    QString payAmount = ui->payAmount->text();
    QString label;
    qint64 payAmountParsed;

    valid = GUIUtil::parseMoney(payAmount, &payAmountParsed);

    if(!valid || payAmount.isEmpty())
    {
        QMessageBox::warning(this, tr("Send Coins"),
            tr("Must fill in an amount to pay."),
            QMessageBox::Ok, QMessageBox::Ok);
        return;
    }

    // Add address to address book under label, if specified
    label = ui->addAsLabel->text();

    switch(model->sendCoins(ui->payTo->text(), payAmountParsed, label))
    {
    case WalletModel::InvalidAddress:
        QMessageBox::warning(this, tr("Send Coins"),
            tr("The recepient address is not valid, please recheck."),
            QMessageBox::Ok, QMessageBox::Ok);
        ui->payTo->setFocus();
        break;
    case WalletModel::InvalidAmount:
        QMessageBox::warning(this, tr("Send Coins"),
            tr("The amount to pay must be larger than 0."),
            QMessageBox::Ok, QMessageBox::Ok);
        ui->payAmount->setFocus();
        break;
    case WalletModel::AmountExceedsBalance:
        QMessageBox::warning(this, tr("Send Coins"),
            tr("Amount exceeds your balance"),
            QMessageBox::Ok, QMessageBox::Ok);
        ui->payAmount->setFocus();
        break;
    case WalletModel::AmountWithFeeExceedsBalance:
        QMessageBox::warning(this, tr("Send Coins"),
            tr("Total exceeds your balance when the %1 transaction fee is included").
            arg(GUIUtil::formatMoney(model->getOptionsModel()->getTransactionFee())),
            QMessageBox::Ok, QMessageBox::Ok);
        ui->payAmount->setFocus();
        break;
    case WalletModel::OK:
        accept();
        break;
    }
}

void SendCoinsDialog::on_pasteButton_clicked()
{
    // Paste text from clipboard into recipient field
    ui->payTo->setText(QApplication::clipboard()->text());
}

void SendCoinsDialog::on_addressBookButton_clicked()
{
    AddressBookDialog dlg(AddressBookDialog::ForSending);
    dlg.setModel(model->getAddressTableModel());
    dlg.setTab(AddressBookDialog::SendingTab);
    dlg.exec();
    ui->payTo->setText(dlg.getReturnValue());
    ui->payAmount->setFocus();
}

void SendCoinsDialog::on_buttonBox_rejected()
{
    reject();
}

void SendCoinsDialog::on_payTo_textChanged(const QString &address)
{
    ui->addAsLabel->setText(model->labelForAddress(address));
}
