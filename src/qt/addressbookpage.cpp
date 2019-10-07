// Copyright (c) 2011-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <qt/addressbookpage.h>
#include <qt/forms/ui_addressbookpage.h>

#include <qt/addresstablemodel.h>
#include <qt/bitcoingui.h>
#include <qt/csvmodelwriter.h>
#include <qt/editaddressdialog.h>
#include <qt/guiutil.h>
#include <qt/platformstyle.h>

#include <base58.h>
#include <script/standard.h>
#include <sync.h>
#include <util.h>
#include <wallet/wallet.h>

#include <QList>
#include <QIcon>
#include <QMenu>
#include <QModelIndex>
#include <QMessageBox>
#include <QSortFilterProxyModel>

AddressBookPage::AddressBookPage(const PlatformStyle *platformStyle, Mode _mode, Tabs _tab, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddressBookPage),
    model(0),
    mode(_mode),
    tab(_tab),
    setPrimaryAction(0)
{
    ui->setupUi(this);

    if (!platformStyle->getImagesOnButtons()) {
        ui->newAddress->setIcon(QIcon());
        ui->copyAddress->setIcon(QIcon());
        ui->deleteAddress->setIcon(QIcon());
        ui->exportButton->setIcon(QIcon());
    } else {
        ui->newAddress->setIcon(platformStyle->SingleColorIcon(":/icons/add"));
        ui->copyAddress->setIcon(platformStyle->SingleColorIcon(":/icons/editcopy"));
        ui->deleteAddress->setIcon(platformStyle->SingleColorIcon(":/icons/remove"));
        ui->exportButton->setIcon(platformStyle->SingleColorIcon(":/icons/export"));
    }

    switch(mode)
    {
    case ForSelection:
        switch(tab)
        {
        case SendingTab: setWindowTitle(tr("Choose the address to send coins to")); break;
        case ReceivingTab: setWindowTitle(tr("Choose the address to receive coins with")); break;
        }
        connect(ui->tableView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(accept()));
        ui->tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
        ui->tableView->setFocus();
        ui->closeButton->setText(tr("C&hoose"));
        ui->exportButton->hide();
        break;
    case ForEditing:
        switch(tab)
        {
        case SendingTab: setWindowTitle(tr("Sending addresses")); break;
        case ReceivingTab: setWindowTitle(tr("Receiving addresses")); break;
        }
        break;
    }
    switch(tab)
    {
    case SendingTab:
        ui->labelExplanation->setText(tr("These are your BitcoinHD addresses for sending payments. Always check the amount and the receiving address before sending coins."));
        ui->newAddress->setVisible(true);
        ui->deleteAddress->setVisible(mode == AddressBookPage::ForEditing);
        break;
    case ReceivingTab:
        ui->labelExplanation->setText(tr("These are your BitcoinHD addresses for receiving payments."));
        ui->newAddress->setVisible(false);
        ui->deleteAddress->setVisible(mode == AddressBookPage::ForEditing);
        break;
    }

    // Context menu actions
    QAction *copyAddressAction = new QAction(tr("&Copy Address"), this);
    QAction *copyLabelAction = new QAction(tr("Copy &Label"), this);
    QAction *editAction = new QAction(tr("&Edit"), this);
    deleteAction = new QAction(ui->deleteAddress->text(), this);
    setPrimaryAction = new QAction(tr("Set as &Primary Address"), this);

    // Build context menu
    contextMenu = new QMenu(this);
    contextMenu->addAction(copyAddressAction);
    contextMenu->addAction(copyLabelAction);
    if (mode == AddressBookPage::ForEditing) {
        contextMenu->addAction(editAction);
        contextMenu->addAction(deleteAction);
        if (tab == ReceivingTab) {
            contextMenu->addSeparator();
            contextMenu->addAction(setPrimaryAction);
        }
    }

    // Connect signals for context menu actions
    connect(copyAddressAction, SIGNAL(triggered()), this, SLOT(on_copyAddress_clicked()));
    connect(copyLabelAction, SIGNAL(triggered()), this, SLOT(onCopyLabelAction()));
    connect(editAction, SIGNAL(triggered()), this, SLOT(onEditAction()));
    connect(deleteAction, SIGNAL(triggered()), this, SLOT(on_deleteAddress_clicked()));
    connect(setPrimaryAction, SIGNAL(triggered()), this, SLOT(onSetPrimaryAction()));

    connect(ui->tableView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextualMenu(QPoint)));

    connect(ui->closeButton, SIGNAL(clicked()), this, SLOT(accept()));
}

AddressBookPage::~AddressBookPage()
{
    delete ui;
}

void AddressBookPage::setModel(AddressTableModel *_model)
{
    this->model = _model;
    if(!_model)
        return;

    proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(_model);
    proxyModel->setDynamicSortFilter(true);
    proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
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
    ui->tableView->sortByColumn(AddressTableModel::Label, Qt::AscendingOrder);

    // Set column widths
#if QT_VERSION < 0x050000
    if (tab == ReceivingTab) {
        ui->tableView->horizontalHeader()->setResizeMode(AddressTableModel::Status, QHeaderView::ResizeToContents);
        ui->tableView->horizontalHeader()->setResizeMode(AddressTableModel::Watchonly, QHeaderView::ResizeToContents);
    } else {
        ui->tableView->setColumnHidden(AddressTableModel::Status, true);
        ui->tableView->setColumnHidden(AddressTableModel::Watchonly, true);
    }
    ui->tableView->horizontalHeader()->setResizeMode(AddressTableModel::Label, QHeaderView::Stretch);
    ui->tableView->horizontalHeader()->setResizeMode(AddressTableModel::Address, QHeaderView::ResizeToContents);
    if (tab == ReceivingTab) {
        ui->tableView->horizontalHeader()->setResizeMode(AddressTableModel::Amount, QHeaderView::ResizeToContents);
        ui->tableView->horizontalHeader()->setResizeMode(AddressTableModel::LockedAmount, QHeaderView::ResizeToContents);
        ui->tableView->horizontalHeader()->setResizeMode(AddressTableModel::LoanAmount, QHeaderView::ResizeToContents);
        ui->tableView->horizontalHeader()->setResizeMode(AddressTableModel::BorrowAmount, QHeaderView::ResizeToContents);
    } else {
        ui->tableView->setColumnHidden(AddressTableModel::Amount, true);
        ui->tableView->setColumnHidden(AddressTableModel::LockedAmount, true);
        ui->tableView->setColumnHidden(AddressTableModel::LoanAmount, true);
        ui->tableView->setColumnHidden(AddressTableModel::BorrowAmount, true);
    }
#else
    if (tab == ReceivingTab) {
        ui->tableView->horizontalHeader()->setSectionResizeMode(AddressTableModel::Status, QHeaderView::ResizeToContents);
        ui->tableView->horizontalHeader()->setSectionResizeMode(AddressTableModel::Watchonly, QHeaderView::ResizeToContents);
    } else {
        ui->tableView->setColumnHidden(AddressTableModel::Status, true);
        ui->tableView->setColumnHidden(AddressTableModel::Watchonly, true);
    }
    ui->tableView->horizontalHeader()->setSectionResizeMode(AddressTableModel::Label, QHeaderView::Stretch);
    ui->tableView->horizontalHeader()->setSectionResizeMode(AddressTableModel::Address, QHeaderView::ResizeToContents);
    if (tab == ReceivingTab) {
        ui->tableView->horizontalHeader()->setSectionResizeMode(AddressTableModel::Amount, QHeaderView::ResizeToContents);
        ui->tableView->horizontalHeader()->setSectionResizeMode(AddressTableModel::LockedAmount, QHeaderView::ResizeToContents);
        ui->tableView->horizontalHeader()->setSectionResizeMode(AddressTableModel::LoanAmount, QHeaderView::ResizeToContents);
        ui->tableView->horizontalHeader()->setSectionResizeMode(AddressTableModel::BorrowAmount, QHeaderView::ResizeToContents);
    } else {
        ui->tableView->setColumnHidden(AddressTableModel::Amount, true);
        ui->tableView->setColumnHidden(AddressTableModel::LockedAmount, true);
        ui->tableView->setColumnHidden(AddressTableModel::LoanAmount, true);
        ui->tableView->setColumnHidden(AddressTableModel::BorrowAmount, true);
    }
#endif

    connect(ui->tableView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
        this, SLOT(selectionChanged()));

    // Select row for newly created address
    connect(_model, SIGNAL(rowsInserted(QModelIndex,int,int)), this, SLOT(selectNewAddress(QModelIndex,int,int)));

    selectionChanged();
}

void AddressBookPage::on_copyAddress_clicked()
{
    GUIUtil::copyEntryData(ui->tableView, AddressTableModel::Address);
}

void AddressBookPage::onCopyLabelAction()
{
    GUIUtil::copyEntryData(ui->tableView, AddressTableModel::Label);
}

void AddressBookPage::onSetPrimaryAction()
{
    QList<QModelIndex> currentIndexes = GUIUtil::getEntryData(ui->tableView, AddressTableModel::Address);
    if (!currentIndexes.isEmpty()) {
        std::string strAddress = currentIndexes.at(0).data(Qt::EditRole).toString().toStdString();
        CTxDestination dest = DecodeDestination(strAddress);
        {
            LOCK(model->getWallet()->cs_wallet);
            if (model->getWallet()->SetPrimaryDestination(dest)) {
                model->reload();
            }
        }
    }
}

void AddressBookPage::onEditAction()
{
    if(!model)
        return;

    if(!ui->tableView->selectionModel())
        return;
    QModelIndexList indexes = ui->tableView->selectionModel()->selectedRows();
    if(indexes.isEmpty())
        return;

    EditAddressDialog dlg(
        tab == SendingTab ?
        EditAddressDialog::EditSendingAddress :
        EditAddressDialog::EditReceivingAddress, this);
    dlg.setModel(model);
    QModelIndex origIndex = proxyModel->mapToSource(indexes.at(0));
    dlg.loadRow(origIndex.row());
    dlg.exec();
}

void AddressBookPage::on_newAddress_clicked()
{
    if(!model)
        return;

    EditAddressDialog dlg(
        tab == SendingTab ?
        EditAddressDialog::NewSendingAddress :
        EditAddressDialog::NewReceivingAddress, this);
    dlg.setModel(model);
    if(dlg.exec())
    {
        newAddressToSelect = dlg.getAddress();
    }
}

void AddressBookPage::on_deleteAddress_clicked()
{
    QTableView *table = ui->tableView;
    if(!table->selectionModel())
        return;

    QModelIndexList indexes = table->selectionModel()->selectedRows();
    if(!indexes.isEmpty())
    {
        table->model()->removeRow(indexes.at(0).row());
    }
}

void AddressBookPage::selectionChanged()
{
    // Set button states based on selected tab and selection
    QTableView *table = ui->tableView;
    if(!table->selectionModel())
        return;

    QModelIndexList indexes = table->selectionModel()->selectedRows();
    if(!indexes.isEmpty())
    {
        switch(tab)
        {
        case SendingTab:
            // In sending tab, allow deletion of selection
            ui->deleteAddress->setEnabled(true);
            deleteAction->setEnabled(true);
            break;
        case ReceivingTab:
            // Deleting receiving addresses, however, is not allowed
            {
                LOCK(model->getWallet()->cs_wallet);
                QModelIndex index = indexes.at(0);
                std::string strAddress = index.sibling(index.row(), (int)AddressTableModel::Address).data(Qt::EditRole).toString().toStdString();
                CTxDestination dest = DecodeDestination(strAddress);
                bool fDontDelete = model->getWallet()->IsPrimaryDestination(dest) || (::IsMine(*model->getWallet(), dest) & ISMINE_SPENDABLE) != 0;
                ui->deleteAddress->setEnabled(!fDontDelete);
                deleteAction->setEnabled(!fDontDelete);
            }
            break;
        }
        ui->copyAddress->setEnabled(true);
    }
    else
    {
        ui->deleteAddress->setEnabled(false);
        ui->copyAddress->setEnabled(false);
    }
}

void AddressBookPage::done(int retval)
{
    QTableView *table = ui->tableView;
    if(!table->selectionModel() || !table->model())
        return;

    // Figure out which address was selected, and return it
    QModelIndexList indexes = table->selectionModel()->selectedRows(AddressTableModel::Address);

    for (const QModelIndex& index : indexes) {
        QVariant address = table->model()->data(index);
        returnValue = address.toString();
    }

    if(returnValue.isEmpty())
    {
        // If no address entry selected, return rejected
        retval = Rejected;
    }

    QDialog::done(retval);
}

void AddressBookPage::on_exportButton_clicked()
{
    // CSV is currently the only supported format
    QString filename = GUIUtil::getSaveFileName(this,
        tr("Export Address List"), QString(),
        tr("Comma separated file (*.csv)"), nullptr);

    if (filename.isNull())
        return;

    CSVModelWriter writer(filename);

    // name, column, role
    writer.setModel(proxyModel);
    writer.addColumn("Status", AddressTableModel::Status, Qt::EditRole);
    writer.addColumn("Watchonly", AddressTableModel::Watchonly, Qt::EditRole);
    writer.addColumn("Label", AddressTableModel::Label, Qt::EditRole);
    writer.addColumn("Address", AddressTableModel::Address, Qt::EditRole);
    writer.addColumn("Amount", AddressTableModel::Amount, Qt::EditRole);
    writer.addColumn("Locked amount", AddressTableModel::LockedAmount, Qt::EditRole);
    writer.addColumn("Loan amount", AddressTableModel::LoanAmount, Qt::EditRole);
    writer.addColumn("Borrow amount", AddressTableModel::BorrowAmount, Qt::EditRole);

    if(!writer.write()) {
        QMessageBox::critical(this, tr("Exporting Failed"),
            tr("There was an error trying to save the address list to %1. Please try again.").arg(filename));
    }
}

void AddressBookPage::contextualMenu(const QPoint &point)
{
    QModelIndex index = ui->tableView->indexAt(point);
    if (index.isValid())
    {
        if (setPrimaryAction) {
            std::string strAddress = index.sibling(index.row(), (int)AddressTableModel::Address).data(Qt::EditRole).toString().toStdString();
            CTxDestination dest = DecodeDestination(strAddress);
            setPrimaryAction->setEnabled(!model->getWallet()->IsPrimaryDestination(dest));
        }

        contextMenu->exec(QCursor::pos());
    }
}

void AddressBookPage::selectNewAddress(const QModelIndex &parent, int begin, int /*end*/)
{
    QModelIndex idx = proxyModel->mapFromSource(model->index(begin, AddressTableModel::Address, parent));
    if(idx.isValid() && (idx.data(Qt::EditRole).toString() == newAddressToSelect))
    {
        // Select row of newly created address, once
        ui->tableView->setFocus();
        ui->tableView->selectRow(idx.row());
        newAddressToSelect.clear();
    }
}
