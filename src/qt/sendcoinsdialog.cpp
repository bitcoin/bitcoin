#include "sendcoinsdialog.h"
#include "ui_sendcoinsdialog.h"
#include "clientmodel.h"
#include "guiutil.h"

#include "addressbookdialog.h"
#include "optionsmodel.h"

#include <QApplication>
#include <QClipboard>
#include <QMessageBox>
#include <QLocale>
#include <QDebug>

#include "util.h"
#include "base58.h"

SendCoinsDialog::SendCoinsDialog(QWidget *parent, const QString &address) :
    QDialog(parent),
    ui(new Ui::SendCoinsDialog),
    model(0)
{
    ui->setupUi(this);

    GUIUtil::setupAddressWidget(ui->payTo, this);
    GUIUtil::setupAmountWidget(ui->payAmount, this);

    /* Set initial address if provided */
    if(!address.isEmpty())
    {
        ui->payTo->setText(address);
        ui->payAmount->setFocus();
    }
}

void SendCoinsDialog::setModel(ClientModel *model)
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
    qint64 payAmountParsed;

    valid = ParseMoney(payAmount.toStdString(), payAmountParsed);

    if(!valid)
    {
        QMessageBox::warning(this, tr("Send Coins"),
            tr("The amount to pay must be a valid number."),
            QMessageBox::Ok, QMessageBox::Ok);
        return;
    }

    switch(model->sendCoins(ui->payTo->text(), payAmountParsed))
    {
    case ClientModel::InvalidAddress:
        QMessageBox::warning(this, tr("Send Coins"),
            tr("The recepient address is not valid, please recheck."),
            QMessageBox::Ok, QMessageBox::Ok);
        ui->payTo->setFocus();
        break;
    case ClientModel::InvalidAmount:
        QMessageBox::warning(this, tr("Send Coins"),
            tr("The amount to pay must be larger than 0."),
            QMessageBox::Ok, QMessageBox::Ok);
        ui->payAmount->setFocus();
        break;
    case ClientModel::AmountExceedsBalance:
        QMessageBox::warning(this, tr("Send Coins"),
            tr("Amount exceeds your balance"),
            QMessageBox::Ok, QMessageBox::Ok);
        ui->payAmount->setFocus();
        break;
    case ClientModel::AmountWithFeeExceedsBalance:
        QMessageBox::warning(this, tr("Send Coins"),
            tr("Total exceeds your balance when the %1 transaction fee is included").
                arg(QString::fromStdString(FormatMoney(model->getOptionsModel()->getTransactionFee()))),
            QMessageBox::Ok, QMessageBox::Ok);
        ui->payAmount->setFocus();
        break;
    case ClientModel::OK:
        accept();
        break;
    }
}

void SendCoinsDialog::on_pasteButton_clicked()
{
    /* Paste text from clipboard into recipient field */
    ui->payTo->setText(QApplication::clipboard()->text());
}

void SendCoinsDialog::on_addressBookButton_clicked()
{
    AddressBookDialog dlg(AddressBookDialog::ForSending);
    dlg.setModel(model->getAddressTableModel());
    dlg.setTab(AddressBookDialog::SendingTab);
    dlg.exec();
    ui->payTo->setText(dlg.getReturnValue());
}

void SendCoinsDialog::on_buttonBox_rejected()
{
    reject();
}
