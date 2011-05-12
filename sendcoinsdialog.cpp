#include "sendcoinsdialog.h"
#include "ui_sendcoinsdialog.h"

SendCoinsDialog::SendCoinsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SendCoinsDialog)
{
    ui->setupUi(this);
}

SendCoinsDialog::~SendCoinsDialog()
{
    delete ui;
}
