#include "sendcoinsdialog.h"
#include "ui_sendcoinsdialog.h"

#include "addressbookdialog.h"
#include "bitcoinaddressvalidator.h"

#include <QApplication>
#include <QClipboard>
#include <QMessageBox>
#include <QLocale>
#include <QDebug>

#include "util.h"
#include "base58.h"

SendCoinsDialog::SendCoinsDialog(QWidget *parent, const QString &address) :
    QDialog(parent),
    ui(new Ui::SendCoinsDialog)
{
    ui->setupUi(this);

    /* Set up validators */
    ui->payTo->setMaxLength(BitcoinAddressValidator::MaxAddressLength);
    ui->payTo->setValidator(new BitcoinAddressValidator(this));
    QDoubleValidator *amountValidator = new QDoubleValidator(this);
    amountValidator->setDecimals(8);
    amountValidator->setBottom(0.0);
    ui->payAmount->setValidator(amountValidator);

    /* Set initial address if provided */
    if(!address.isEmpty())
    {
        ui->payTo->setText(address);
        ui->payAmount->setFocus();
    }
}

SendCoinsDialog::~SendCoinsDialog()
{
    delete ui;
}

void SendCoinsDialog::on_sendButton_clicked()
{
    QByteArray payTo = ui->payTo->text().toUtf8();
    uint160 payToHash = 0;
    int64 payAmount = 0.0;
    bool valid = false;

    if(!AddressToHash160(payTo.constData(), payToHash))
    {
        QMessageBox::warning(this, tr("Warning"),
                                       tr("The recepient address is not valid, please recheck."),
                                       QMessageBox::Ok,
                                       QMessageBox::Ok);
        ui->payTo->setFocus();
        return;
    }
    valid = ParseMoney(ui->payAmount->text().toStdString(), payAmount);

    if(!valid || payAmount <= 0)
    {
        QMessageBox::warning(this, tr("Warning"),
                                       tr("The amount to pay must be a valid number larger than 0."),
                                       QMessageBox::Ok,
                                       QMessageBox::Ok);
        ui->payAmount->setFocus();
        return;
    }
    qDebug() << "Pay " << payAmount;

    /* TODO: send command to core, once this succeeds do accept() */
    accept();
}

void SendCoinsDialog::on_pasteButton_clicked()
{
    /* Paste text from clipboard into recipient field */
    ui->payTo->setText(QApplication::clipboard()->text());
}

void SendCoinsDialog::on_addressBookButton_clicked()
{
    AddressBookDialog dlg;
    dlg.exec();
    ui->payTo->setText(dlg.getReturnValue());
}

void SendCoinsDialog::on_buttonBox_rejected()
{
    reject();
}
