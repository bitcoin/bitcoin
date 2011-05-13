#include "editaddressdialog.h"
#include "ui_editaddressdialog.h"

EditAddressDialog::EditAddressDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::EditAddressDialog)
{
    ui->setupUi(this);
}

EditAddressDialog::~EditAddressDialog()
{
    delete ui;
}
