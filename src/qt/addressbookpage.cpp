#include "addressbookpage.h"
#include "ui_addressbookpage.h"

#include "addresstablemodel.h"
#include "editaddressdialog.h"
#include "csvmodelwriter.h"

#include <QSortFilterProxyModel>
#include <QClipboard>
#include <QFileDialog>
#include <QMessageBox>

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
        connect(ui->tableView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(accept()));
        ui->tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
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
    ui->tableView->setTabKeyNavigation(false);

    connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
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

    proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(model);
    proxyModel->setDynamicSortFilter(true);
    switch(tab)
    {
    case ReceivingTab:
        // Receive filter
        proxyModel->setFilterRole(AddressTableModel::TypeRole);
        proxyModel->setFilterFixedString(AddressTableModel::Receive);
        break;
    case SendingTab:
        // Send filter
        proxyModel->setFilterRole(AddressTableModel::TypeRole);
        proxyModel->setFilterFixedString(AddressTableModel::Send);
        break;
    }
    ui->tableView->setModel(proxyModel);
    ui->tableView->sortByColumn(0, Qt::AscendingOrder);

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
    if(dlg.exec())
    {
        // Select row for newly created address
        QString address = dlg.getAddress();
        QModelIndexList lst = proxyModel->match(proxyModel->index(0,
                          AddressTableModel::Address, QModelIndex()),
                          Qt::EditRole, address, 1, Qt::MatchExactly);
        if(!lst.isEmpty())
        {
            ui->tableView->setFocus();
            ui->tableView->selectRow(lst.at(0).row());
        }
    }
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

    // Figure out which address was selected, and return it
    QTableView *table = getCurrentTable();
    QModelIndexList indexes = table->selectionModel()->selectedRows(AddressTableModel::Address);

    foreach (QModelIndex index, indexes)
    {
        QVariant address = table->model()->data(index);
        returnValue = address.toString();
    }

    if(returnValue.isEmpty())
    {
        retval = Rejected;
    }

    QDialog::done(retval);
}

void AddressBookPage::exportClicked()
{
    // CSV is currently the only supported format
    QString filename = QFileDialog::getSaveFileName(
            this,
            tr("Export Address Book Data"),
            QDir::currentPath(),
            tr("Comma separated file (*.csv)"));

    if (filename.isNull()) return;

    CSVModelWriter writer(filename);

    // name, column, role
    writer.setModel(proxyModel);
    writer.addColumn("Label", AddressTableModel::Label, Qt::EditRole);
    writer.addColumn("Address", AddressTableModel::Address, Qt::EditRole);

    if(!writer.write())
    {
        QMessageBox::critical(this, tr("Error exporting"), tr("Could not write to file %1.").arg(filename),
                              QMessageBox::Abort, QMessageBox::Abort);
    }
}
