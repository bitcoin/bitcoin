#include "sendcoinsdialog.h"
#include "ui_sendcoinsdialog.h"

#include "addressbookdialog.h"
#include "bitcoinaddressvalidator.h"

#include <QApplication>
#include <QClipboard>

SendCoinsDialog::SendCoinsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SendCoinsDialog)
{
    ui->setupUi(this);
    ui->payTo->setMaxLength(BitcoinAddressValidator::MaxAddressLength);
    ui->payTo->setValidator(new BitcoinAddressValidator(this));
    ui->payAmount->setValidator(new QDoubleValidator(this));
}

SendCoinsDialog::~SendCoinsDialog()
{
    delete ui;
}

void SendCoinsDialog::on_sendButton_clicked()
{
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
