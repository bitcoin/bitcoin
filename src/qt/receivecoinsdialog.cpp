#include "receivecoinsdialog.h"
#include "ui_receivecoinsdialog.h"

#include "walletmodel.h"
#include "bitcoinunits.h"
#include "addressbookpage.h"
#include "optionsmodel.h"
#include "guiutil.h"
#include "receiverequestdialog.h"
#include "addresstablemodel.h"

#include <QMessageBox>
#include <QTextDocument>
#include <QScrollBar>

ReceiveCoinsDialog::ReceiveCoinsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ReceiveCoinsDialog),
    model(0)
{
    ui->setupUi(this);

#ifdef Q_OS_MAC // Icons on push buttons are very uncommon on Mac
    ui->clearButton->setIcon(QIcon());
    ui->receiveButton->setIcon(QIcon());
#endif
    connect(ui->clearButton, SIGNAL(clicked()), this, SLOT(clear()));
}

void ReceiveCoinsDialog::setModel(WalletModel *model)
{
    this->model = model;

    if(model && model->getOptionsModel())
    {
        connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));
        updateDisplayUnit();
    }
}

ReceiveCoinsDialog::~ReceiveCoinsDialog()
{
    delete ui;
}

void ReceiveCoinsDialog::clear()
{
    ui->reqAmount->clear();
    ui->reqLabel->setText("");
    ui->reqMessage->setText("");
    ui->reuseAddress->setChecked(false);
    updateDisplayUnit();
}

void ReceiveCoinsDialog::reject()
{
    clear();
}

void ReceiveCoinsDialog::accept()
{
    clear();
}

void ReceiveCoinsDialog::updateDisplayUnit()
{
    if(model && model->getOptionsModel())
    {
        ui->reqAmount->setDisplayUnit(model->getOptionsModel()->getDisplayUnit());
    }
}

void ReceiveCoinsDialog::on_receiveButton_clicked()
{
    if(!model || !model->getOptionsModel() || !model->getAddressTableModel())
        return;

    QString address;
    QString label = ui->reqLabel->text();
    if(ui->reuseAddress->isChecked())
    {
        /* Choose existing receiving address */
        AddressBookPage dlg(AddressBookPage::ForSelection, AddressBookPage::ReceivingTab, this);
        dlg.setModel(model->getAddressTableModel());
        if(dlg.exec())
        {
            address = dlg.getReturnValue();
            if(label.isEmpty()) /* If no label provided, use the previously used label */
            {
                label = model->getAddressTableModel()->labelForAddress(address);
            }
        } else {
            return;
        }
    } else {
        /* Generate new receiving address */
        address = model->getAddressTableModel()->addRow(AddressTableModel::Receive, label, "");
    }
    SendCoinsRecipient info(address, label,
            ui->reqAmount->value(), ui->reqMessage->text());
    ReceiveRequestDialog *dialog = new ReceiveRequestDialog(this);
    dialog->setModel(model->getOptionsModel());
    dialog->setInfo(info);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
    clear();
}
