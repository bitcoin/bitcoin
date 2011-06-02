#include "editaddressdialog.h"
#include "ui_editaddressdialog.h"
#include "guiutil.h"

EditAddressDialog::EditAddressDialog(Mode mode, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::EditAddressDialog)
{
    ui->setupUi(this);

    GUIUtil::setupAddressWidget(ui->addressEdit, this);
}

EditAddressDialog::~EditAddressDialog()
{
    delete ui;
}
