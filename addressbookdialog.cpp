#include "addressbookdialog.h"
#include "ui_addressbookdialog.h"

AddressBookDialog::AddressBookDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddressBookDialog)
{
    ui->setupUi(this);
}

AddressBookDialog::~AddressBookDialog()
{
    delete ui;
}

void AddressBookDialog::setTab(int tab)
{
    ui->tabWidget->setCurrentIndex(tab);
}

void AddressBookDialog::on_OKButton_clicked()
{
    accept();
}

void AddressBookDialog::on_copyToClipboard_clicked()
{

}

void AddressBookDialog::on_editButton_clicked()
{

}

void AddressBookDialog::on_newAddressButton_clicked()
{

}
