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

}
