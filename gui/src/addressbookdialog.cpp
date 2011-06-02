#include "addressbookdialog.h"
#include "ui_addressbookdialog.h"

#include "addresstablemodel.h"
#include "editaddressdialog.h"

#include <QSortFilterProxyModel>
#include <QClipboard>
#include <QDebug>

AddressBookDialog::AddressBookDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddressBookDialog),
    model(0)
{
    ui->setupUi(this);

    model = new AddressTableModel(this);
    setModel(model);
}

AddressBookDialog::~AddressBookDialog()
{
    delete ui;
}

void AddressBookDialog::setModel(AddressTableModel *model)
{
    /* Receive filter */
    QSortFilterProxyModel *receive_model = new QSortFilterProxyModel(this);
    receive_model->setSourceModel(model);
    receive_model->setDynamicSortFilter(true);
    receive_model->setFilterRole(AddressTableModel::TypeRole);
    receive_model->setFilterFixedString(AddressTableModel::Receive);
    ui->receiveTableView->setModel(receive_model);

    /* Send filter */
    QSortFilterProxyModel *send_model = new QSortFilterProxyModel(this);
    send_model->setSourceModel(model);
    send_model->setDynamicSortFilter(true);
    send_model->setFilterRole(AddressTableModel::TypeRole);
    send_model->setFilterFixedString(AddressTableModel::Send);
    ui->sendTableView->setModel(send_model);

    /* Set column widths */
    ui->receiveTableView->horizontalHeader()->resizeSection(
            AddressTableModel::Address, 320);
    ui->receiveTableView->horizontalHeader()->setResizeMode(
            AddressTableModel::Label, QHeaderView::Stretch);
    ui->sendTableView->horizontalHeader()->resizeSection(
            AddressTableModel::Address, 320);
    ui->sendTableView->horizontalHeader()->setResizeMode(
            AddressTableModel::Label, QHeaderView::Stretch);
}

void AddressBookDialog::setTab(int tab)
{
    ui->tabWidget->setCurrentIndex(tab);
}

QTableView *AddressBookDialog::getCurrentTable()
{
    switch(ui->tabWidget->currentIndex())
    {
    case SendingTab:
        return ui->sendTableView;
    case ReceivingTab:
        return ui->receiveTableView;
    default:
        return 0;
    }
}

void AddressBookDialog::on_copyToClipboard_clicked()
{
    /* Copy currently selected address to clipboard
       (or nothing, if nothing selected)
     */
    QTableView *table = getCurrentTable();
    QModelIndexList indexes = table->selectionModel()->selectedRows(AddressTableModel::Address);

    foreach (QModelIndex index, indexes) {
        QVariant address = table->model()->data(index);
        QApplication::clipboard()->setText(address.toString());
    }
}

void AddressBookDialog::on_editButton_clicked()
{
    /* Double click also triggers edit button */
    EditAddressDialog dlg(
            ui->tabWidget->currentIndex() == SendingTab ?
            EditAddressDialog::EditSendingAddress :
            EditAddressDialog::EditReceivingAddress);
    dlg.exec();
}

void AddressBookDialog::on_newAddressButton_clicked()
{
    EditAddressDialog dlg(
            ui->tabWidget->currentIndex() == SendingTab ?
            EditAddressDialog::NewSendingAddress :
            EditAddressDialog::NewReceivingAddress);
    dlg.exec();
}

void AddressBookDialog::on_tabWidget_currentChanged(int index)
{
    switch(index)
    {
    case SendingTab:
        ui->deleteButton->setEnabled(true);
        break;
    case ReceivingTab:
        ui->deleteButton->setEnabled(false);
        break;
    }
}

void AddressBookDialog::on_deleteButton_clicked()
{
    QTableView *table = getCurrentTable();
    QModelIndexList indexes = table->selectionModel()->selectedRows();

    foreach (QModelIndex index, indexes) {
        table->model()->removeRow(index.row());
    }
}

void AddressBookDialog::on_buttonBox_accepted()
{
    QTableView *table = getCurrentTable();
    QModelIndexList indexes = table->selectionModel()->selectedRows(AddressTableModel::Address);

    foreach (QModelIndex index, indexes) {
        QVariant address = table->model()->data(index);
        returnValue = address.toString();
    }
    if(!returnValue.isEmpty())
    {
        accept();
    }
    else
    {
        reject();
    }
}
