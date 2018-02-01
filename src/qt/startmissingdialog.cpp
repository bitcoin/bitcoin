#include "startmissingdialog.h"
#include "ui_startmissingdialog.h"
#include <QPushButton>

StartMissingDialog::StartMissingDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::StartMissingDialog)
{
    ui->setupUi(this);
    ui->buttonBox->button(QDialogButtonBox::Cancel)->setFocus();
    connect(ui->checkBox, SIGNAL(stateChanged(int)),this,SLOT(CheckBoxStateChanged(int)));
    ui->warningLabel->hide();
}

StartMissingDialog::~StartMissingDialog()
{
    delete ui;
}

void StartMissingDialog::setText(const QString& text)
{
    ui->textLabel->setText(text);
}

void StartMissingDialog::setWarningText(const QString& text)
{
    ui->warningLabel->setText(text);
}

void StartMissingDialog::setCheckboxText(const QString& text)
{
    ui->checkBox->setText(text);
}

bool StartMissingDialog::checkboxChecked()
{
    bool result = false;
    if (ui->checkBox->checkState() == Qt::Checked) {
        result = true;
    }
    return result;
}

void StartMissingDialog::CheckBoxStateChanged(int state)
{
    if (state == Qt::Checked) {
        ui->warningLabel->show();
    } else {
        ui->warningLabel->hide();
    }
}
