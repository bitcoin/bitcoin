// Copyright (c) 2011-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/wallet.h>

#include <qt/receivecoinsdialog.h>
#include <qt/forms/ui_receivecoinsdialog.h>

#include <qt/addresstablemodel.h>
#include <qt/guiutil.h>
#include <qt/optionsmodel.h>
#include <qt/platformstyle.h>
#include <qt/receiverequestdialog.h>
#include <qt/recentrequeststablemodel.h>
#include <qt/walletmodel.h>

#include <QAction>
#include <QCursor>
#include <QMessageBox>
#include <QScrollBar>
#include <QSettings>
#include <QTextDocument>

#include <ranges>

ReceiveCoinsDialog::ReceiveCoinsDialog(const PlatformStyle *_platformStyle, QWidget *parent) :
    QDialog(parent, GUIUtil::dialog_flags),
    ui(new Ui::ReceiveCoinsDialog),
    platformStyle(_platformStyle)
{
    ui->setupUi(this);

    m_sort_proxy = new QSortFilterProxyModel(this);
    m_sort_proxy->setSortRole(Qt::UserRole);

    if (!_platformStyle->getImagesOnButtons()) {
        ui->clearButton->setIcon(QIcon());
        ui->receiveButton->setIcon(QIcon());
        ui->showRequestButton->setIcon(QIcon());
        ui->removeRequestButton->setIcon(QIcon());
    } else {
        ui->clearButton->setIcon(_platformStyle->SingleColorIcon(":/icons/remove"));
        ui->receiveButton->setIcon(_platformStyle->SingleColorIcon(":/icons/receiving_addresses"));
        ui->showRequestButton->setIcon(_platformStyle->SingleColorIcon(":/icons/eye"));
        ui->removeRequestButton->setIcon(_platformStyle->SingleColorIcon(":/icons/remove"));
    }

    // context menu
    contextMenu = new QMenu(this);
    contextMenu->addAction(tr("Copy &URI"), this, &ReceiveCoinsDialog::copyURI);
    contextMenu->addAction(tr("&Copy address"), this, &ReceiveCoinsDialog::copyAddress);
    copyLabelAction = contextMenu->addAction(tr("Copy &label"), this, &ReceiveCoinsDialog::copyLabel);
    copyMessageAction = contextMenu->addAction(tr("Copy &message"), this, &ReceiveCoinsDialog::copyMessage);
    copyAmountAction = contextMenu->addAction(tr("Copy &amount"), this, &ReceiveCoinsDialog::copyAmount);
    connect(ui->recentRequestsView, &QWidget::customContextMenuRequested, this, &ReceiveCoinsDialog::showMenu);

    connect(ui->clearButton, &QPushButton::clicked, this, &ReceiveCoinsDialog::clear);
}

void ReceiveCoinsDialog::setModel(WalletModel *_model)
{
    this->model = _model;

    if(_model && _model->getOptionsModel())
    {
        connect(_model->getOptionsModel(), &OptionsModel::displayUnitChanged, this, &ReceiveCoinsDialog::updateDisplayUnit);
        updateDisplayUnit();
        connect(_model->getOptionsModel(), &OptionsModel::fontForMoneyChanged, this, &ReceiveCoinsDialog::updateFontForMoney);
        updateFontForMoney();

        QTableView* tableView = ui->recentRequestsView;

        tableView->verticalHeader()->hide();
        tableView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        tableView->setModel(m_sort_proxy);
        m_sort_proxy->setSourceModel(_model->getRecentRequestsTableModel());
        tableView->sortByColumn(RecentRequestsTableModel::Date, Qt::DescendingOrder);

        tableView->setAlternatingRowColors(true);
        tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
        tableView->setSelectionMode(QAbstractItemView::ExtendedSelection);

        QSettings settings;
        if (!tableView->horizontalHeader()->restoreState(settings.value("RecentRequestsViewHeaderState").toByteArray())) {
            tableView->setColumnWidth(RecentRequestsTableModel::Date, DATE_COLUMN_WIDTH);
            tableView->setColumnWidth(RecentRequestsTableModel::Label, LABEL_COLUMN_WIDTH);
            tableView->setColumnWidth(RecentRequestsTableModel::Amount, AMOUNT_MINIMUM_COLUMN_WIDTH);
        }

        connect(tableView->selectionModel(),
            &QItemSelectionModel::selectionChanged, this,
            &ReceiveCoinsDialog::recentRequestsView_selectionChanged);
        // Last 2 columns are set by the columnResizingFixer, when the table geometry is ready.
        columnResizingFixer = new GUIUtil::TableViewLastColumnResizingFixer(tableView, AMOUNT_MINIMUM_COLUMN_WIDTH, DATE_COLUMN_WIDTH, this);

        // Populate address type dropdown and select default
        auto add_address_type = [&](OutputType type, const QString& text, const QString& tooltip) {
            const auto index = ui->addressType->count();
            ui->addressType->addItem(text, (int) type);
            ui->addressType->setItemData(index, tooltip, Qt::ToolTipRole);
            if (model->wallet().getDefaultAddressType() == type) ui->addressType->setCurrentIndex(index);
        };
        add_address_type(OutputType::LEGACY, tr("Base58 (Legacy)"), tr("Not recommended due to higher fees and less protection against typos."));
        add_address_type(OutputType::P2SH_SEGWIT, tr("Base58 (P2SH-SegWit)"), tr("Generates an address compatible with older wallets."));
        add_address_type(OutputType::BECH32, tr("Bech32 (SegWit)"), tr("Generates a native segwit address (BIP-173). Some old wallets don't support it."));
        if (model->wallet().taprootEnabled()) {
            add_address_type(OutputType::BECH32M, tr("Bech32m (Taproot)"), tr("Bech32m (BIP-350) is an upgrade to Bech32, wallet support is still limited."));
        }

        // Set the button to be enabled or disabled based on whether the wallet can give out new addresses.
        ui->receiveButton->setEnabled(model->wallet().canGetAddresses());

        // Enable/disable the receive button if the wallet is now able/unable to give out new addresses.
        connect(model, &WalletModel::canGetAddressesChanged, [this] {
            ui->receiveButton->setEnabled(model->wallet().canGetAddresses());
        });
    }
}

ReceiveCoinsDialog::~ReceiveCoinsDialog()
{
    QSettings settings;
    settings.setValue("RecentRequestsViewHeaderState", ui->recentRequestsView->horizontalHeader()->saveState());
    delete ui;
}

void ReceiveCoinsDialog::clear()
{
    ui->reqAmount->clear();
    ui->reqLabel->setText("");
    ui->reqMessage->setText("");
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

void ReceiveCoinsDialog::updateFontForMoney()
{
    if(model && model->getOptionsModel())
    {
        ui->reqAmount->setFontForMoney(model->getOptionsModel()->getFontForMoney());
    }
}

void ReceiveCoinsDialog::on_receiveButton_clicked()
{
    if(!model || !model->getOptionsModel() || !model->getAddressTableModel() || !model->getRecentRequestsTableModel())
        return;

    QString address;
    QString label = ui->reqLabel->text();
    /* Generate new receiving address */
    const OutputType address_type = (OutputType)ui->addressType->currentData().toInt();
    address = model->getAddressTableModel()->addRow(AddressTableModel::Receive, label, "", address_type);

    switch(model->getAddressTableModel()->getEditStatus())
    {
    case AddressTableModel::EditStatus::OK: {
        // Success
        SendCoinsRecipient info(address, label,
            ui->reqAmount->value(), ui->reqMessage->text());
        ReceiveRequestDialog *dialog = new ReceiveRequestDialog(this);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->setModel(model);
        dialog->setInfo(info);
        dialog->show();

        /* Store request for later reference */
        model->getRecentRequestsTableModel()->addNewRequest(info);
        break;
    }
    case AddressTableModel::EditStatus::WALLET_UNLOCK_FAILURE:
        QMessageBox::critical(this, windowTitle(),
            tr("Could not unlock wallet."),
            QMessageBox::Ok, QMessageBox::Ok);
        break;
    case AddressTableModel::EditStatus::KEY_GENERATION_FAILURE:
        QMessageBox::critical(this, windowTitle(),
            tr("Could not generate new %1 address").arg(QString::fromStdString(FormatOutputType(address_type))),
            QMessageBox::Ok, QMessageBox::Ok);
        break;
    // These aren't valid return values for our action
    case AddressTableModel::EditStatus::INVALID_ADDRESS:
    case AddressTableModel::EditStatus::DUPLICATE_ADDRESS:
    case AddressTableModel::EditStatus::NO_CHANGES:
        assert(false);
    }
    clear();
}

void ReceiveCoinsDialog::on_recentRequestsView_doubleClicked(const QModelIndex &index)
{
    QModelIndexList selection = SelectedRows();
    if (!selection.isEmpty() && selection.at(0).isValid()) {
        ShowReceiveRequestDialogForItem(selection.at(0));
    }
}

void ReceiveCoinsDialog::recentRequestsView_selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    // Enable Show/Remove buttons only if anything is selected.
    bool enable = !ui->recentRequestsView->selectionModel()->selectedRows().isEmpty();
    ui->showRequestButton->setEnabled(enable);
    ui->removeRequestButton->setEnabled(enable);
}

void ReceiveCoinsDialog::on_showRequestButton_clicked()
{
    QModelIndexList selection = SelectedRows();

    for (const QModelIndex& index : selection) {
        ShowReceiveRequestDialogForItem(index);
    }
}

void ReceiveCoinsDialog::on_removeRequestButton_clicked()
{
    QModelIndexList selection = SelectedRows();
    if(selection.empty())
        return;

    // Collect row indices in a set (sorted) and pass in reverse order to removeRows
    // to avoid having to keep track of changed source indices after each removal
    std::set<int> row_indices;
    for (const QModelIndex& ind : selection) {
        row_indices.insert(ind.row());
    }

    for (auto row_ind : row_indices | std::views::reverse) {
        model->getRecentRequestsTableModel()->removeRows(row_ind, 1);
    }
}

// We override the virtual resizeEvent of the QWidget to adjust tables column
// sizes as the tables width is proportional to the dialogs width.
void ReceiveCoinsDialog::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    columnResizingFixer->stretchColumnWidth(RecentRequestsTableModel::Message);
}

QModelIndexList ReceiveCoinsDialog::SelectedRows()
{
    if(!model || !model->getRecentRequestsTableModel() || !ui->recentRequestsView->selectionModel())
        return QModelIndexList();
    QModelIndexList selection = ui->recentRequestsView->selectionModel()->selectedRows();
    QModelIndexList source_mapped;
    for (auto row : selection) {
        source_mapped.append(m_sort_proxy->mapToSource(row));
    }

    return source_mapped;
}

// copy column of selected row to clipboard
void ReceiveCoinsDialog::copyColumnToClipboard(int column)
{
    const QModelIndexList sel = SelectedRows();
    if (sel.isEmpty()) {
        return;
    }

    const RecentRequestsTableModel* const submodel = model->getRecentRequestsTableModel();
    QString column_value;
    for (int sel_ind = 0; sel_ind < sel.size(); ++sel_ind) {
        if (!sel.at(sel_ind).isValid()) {
            continue;
        }
        column_value += submodel->index(sel.at(sel_ind).row(), column).data(Qt::EditRole).toString();
        if (sel_ind < sel.size() - 1) {
            column_value += QString("\n");
        }
    }
    GUIUtil::setClipboard(column_value);
}

void ReceiveCoinsDialog::ShowReceiveRequestDialogForItem(const QModelIndex& index)
{
    if (!index.isValid()) {
        return;
    }
    const RecentRequestsTableModel* submodel = model->getRecentRequestsTableModel();
    ReceiveRequestDialog* dialog = new ReceiveRequestDialog(this);
    dialog->setModel(model);
    dialog->setInfo(submodel->entry(index.row()).recipient);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
}

// context menu
void ReceiveCoinsDialog::showMenu(const QPoint &point)
{
    const QModelIndexList sel = SelectedRows();
    if (sel.isEmpty()) {
        return;
    }

    if (sel.size() == 1 && sel.at(0).isValid()) {
    // disable context menu actions when appropriate
    const RecentRequestsTableModel* const submodel = model->getRecentRequestsTableModel();
        const RecentRequestEntry& req = submodel->entry(sel.at(0).row());
    copyLabelAction->setDisabled(req.recipient.label.isEmpty());
    copyMessageAction->setDisabled(req.recipient.message.isEmpty());
    copyAmountAction->setDisabled(req.recipient.amount == 0);
    } else if (sel.size() > 1) {
        // multiple selection

        copyLabelAction->setDisabled(true);
        copyMessageAction->setDisabled(true);
        copyAmountAction->setDisabled(true);

        // disable context menu actions when appropriate
        const RecentRequestsTableModel* const submodel = model->getRecentRequestsTableModel();

        for (auto selection : sel) {
            if (!selection.isValid()) {
                continue;
            }
            const RecentRequestEntry& req = submodel->entry(selection.row());
            if (!req.recipient.label.isEmpty()) {
                copyLabelAction->setDisabled(false);
            }
            if (!req.recipient.message.isEmpty()) {
                copyMessageAction->setDisabled(false);
            }
            if (req.recipient.amount != 0) {
                copyAmountAction->setDisabled(false);
            }
        }
    }

    contextMenu->exec(QCursor::pos());
}

// context menu action: copy URI
void ReceiveCoinsDialog::copyURI()
{
    const QModelIndexList sel = SelectedRows();
    if (sel.isEmpty()) {
        return;
    }

    const RecentRequestsTableModel * const submodel = model->getRecentRequestsTableModel();
    QString uri;
    for (int sel_ind = 0; sel_ind < sel.size(); ++sel_ind) {
        if (!sel.at(sel_ind).isValid()) {
            continue;
        }
        const RecentRequestEntry& req = submodel->entry(sel.at(sel_ind).row());
        uri += GUIUtil::formatBitcoinURI(req.recipient);
        if (sel_ind < sel.size() - 1) {
            uri += QString("\n");
        }
    }
    GUIUtil::setClipboard(uri);
}

// context menu action: copy address
void ReceiveCoinsDialog::copyAddress()
{
    const QModelIndexList sel = SelectedRows();
    if (sel.isEmpty()) {
        return;
    }

    const RecentRequestsTableModel* const submodel = model->getRecentRequestsTableModel();
    QString address;
    for (int sel_ind = 0; sel_ind < sel.size(); ++sel_ind) {
        if (!sel.at(sel_ind).isValid()) {
            continue;
        }
        const RecentRequestEntry& req = submodel->entry(sel.at(sel_ind).row());
        address += req.recipient.address;
        if (sel_ind < sel.size() - 1) {
            address += QString("\n");
        }
    }
    GUIUtil::setClipboard(address);
}

// context menu action: copy label
void ReceiveCoinsDialog::copyLabel()
{
    copyColumnToClipboard(RecentRequestsTableModel::Label);
}

// context menu action: copy message
void ReceiveCoinsDialog::copyMessage()
{
    copyColumnToClipboard(RecentRequestsTableModel::Message);
}

// context menu action: copy amount
void ReceiveCoinsDialog::copyAmount()
{
    copyColumnToClipboard(RecentRequestsTableModel::Amount);
}
