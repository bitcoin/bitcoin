#include "progressdialog.h"
#include "ui_progressdialog.h"

ProgressDialog::ProgressDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ProgressDialog)
{
    ui->setupUi(this);
}

ProgressDialog::~ProgressDialog()
{
    delete ui;
}

void ProgressDialog::UpdateProgress(int v)
{
    ui->progressBar->setValue(v);
}

void ProgressDialog::setMax(int m)
{
    ui->progressBar->setMaximum(m);
}

