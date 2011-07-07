#include "addressbookpage.h"
#include "ui_addressbookpage.h"

#include "addresstablemodel.h"
#include "editaddressdialog.h"

#include <QSortFilterProxyModel>
#include <QClipboard>
#include <QDebug>

AddressBookPage::AddressBookPage(Mode mode, Tabs tab, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddressBookPage),
    model(0),
    mode(mode),
    tab(tab)
{
    ui->setupUi(this);
    switch(mode)
    {
    case ForSending:
        connect(ui->tableView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(on_buttonBox_accepted()));
        ui->tableView->setFocus();
        break;
    case ForEditing:
        ui->buttonBox->hide();
        break;
    }
    switch(tab)
    {
    case SendingTab:
        ui->labelExplanation->hide();
        break;
    case ReceivingTab:
        break;
    }
}

AddressBookPage::~AddressBookPage()
{
    delete ui;
}

void AddressBookPage::setModel(AddressTableModel *model)
{
    this->model = model;
    // Refresh list from core
    model->updateList();

    switch(tab)
    {
    case ReceivingTab: {
        // Receive filter
        QSortFilterProxyModel *receive_model = new QSortFilterProxyModel(this);
        receive_model->setSourceModel(model);
        receive_model->setDynamicSortFilter(true);
        receive_model->setFilterRole(AddressTableModel::TypeRole);
        receive_model->setFilterFixedString(AddressTableModel::Receive);
        ui->tableView->setModel(receive_model);
        ui->tableView->sortByColumn(0, Qt::AscendingOrder);
        } break;
    case SendingTab: {
        // Send filter
        QSortFilterProxyModel *send_model = new QSortFilterProxyModel(this);
        send_model->setSourceModel(model);
        send_model->setDynamicSortFilter(true);
        send_model->setFilterRole(AddressTableModel::TypeRole);
        send_model->setFilterFixedString(AddressTableModel::Send);
        ui->tableView->setModel(send_model);
        ui->tableView->sortByColumn(0, Qt::AscendingOrder);
        } break;
    }

    // Set column widths
    ui->tableView->horizontalHeader()->resizeSection(
            AddressTableModel::Address, 320);
    ui->tableView->horizontalHeader()->setResizeMode(
            AddressTableModel::Label, QHeaderView::Stretch);

    connect(ui->tableView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(selectionChanged()));

    if(mode == ForSending)
    {
        // Auto-select first row when in sending mode
        ui->tableView->selectRow(0);
    }
    selectionChanged();
}

QTableView *AddressBookPage::getCurrentTable()
{
    return ui->tableView;
}

void AddressBookPage::on_copyToClipboard_clicked()
{
    // Copy currently selected address to clipboard
    //   (or nothing, if nothing selected)
    QTableView *table = getCurrentTable();
    QModelIndexList indexes = table->selectionModel()->selectedRows(AddressTableModel::Address);

    foreach (QModelIndex index, indexes)
    {
        QVariant address = index.data();
        QApplication::clipboard()->setText(address.toString());
    }
}

void AddressBookPage::on_newAddressButton_clicked()
{
    EditAddressDialog dlg(
            tab == SendingTab ?
            EditAddressDialog::NewSendingAddress :
            EditAddressDialog::NewReceivingAddress);
    dlg.setModel(model);
    dlg.exec();
}

void AddressBookPage::on_deleteButton_clicked()
{
    QTableView *table = getCurrentTable();
    QModelIndexList indexes = table->selectionModel()->selectedRows();
    if(!indexes.isEmpty())
    {
        table->model()->removeRow(indexes.at(0).row());
    }
}

void AddressBookPage::on_buttonBox_accepted()
{
    QTableView *table = getCurrentTable();
    QModelIndexList indexes = table->selectionModel()->selectedRows(AddressTableModel::Address);

    foreach (QModelIndex index, indexes)
    {
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

void AddressBookPage::selectionChanged()
{
    // Set button states based on selected tab and selection
    QTableView *table = getCurrentTable();

    if(table->selectionModel()->hasSelection())
    {
        switch(tab)
        {
        case SendingTab:
            ui->deleteButton->setEnabled(true);
            break;
        case ReceivingTab:
            ui->deleteButton->setEnabled(false);
            break;
        }
        ui->copyToClipboard->setEnabled(true);
    }
    else
    {
        ui->deleteButton->setEnabled(false);
        ui->copyToClipboard->setEnabled(false);
    }
}

void AddressBookPage::done(int retval)
{
    // When this is a tab/widget and not a model dialog, ignore "done"
    if(mode == ForEditing)
        return;
    QDialog::done(retval);
}
