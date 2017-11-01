// Copyright (c) 2011-2015 The Bitcoin Core developers
// Copyright (c) 2015-2017 The Bitcoin Unlimited developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "receivecoinsdialog.h"
#include "ui_receivecoinsdialog.h"

#include "addressbookpage.h"
#include "addresstablemodel.h"
#include "bitcoinunits.h"
#include "guiutil.h"
#include "optionsmodel.h"
#include "platformstyle.h"
#include "receivefreezedialog.h"
#include "receiverequestdialog.h"
#include "recentrequeststablemodel.h"
#include "unlimited.h"

ReceiveFreezeDialog *freezeDialog;

#include <QAction>
#include <QCheckBox>
#include <QCursor>
#include <QItemSelection>
#include <QMessageBox>
#include <QScrollBar>
#include <QTextDocument>

ReceiveCoinsDialog::ReceiveCoinsDialog(const PlatformStyle *platformStyle, QWidget *parent)
    : QDialog(parent), ui(new Ui::ReceiveCoinsDialog), model(0), platformStyle(platformStyle)
{
    ui->setupUi(this);

    if (!platformStyle->getImagesOnButtons())
    {
        ui->clearButton->setIcon(QIcon());
        ui->receiveButton->setIcon(QIcon());
        ui->showRequestButton->setIcon(QIcon());
        ui->removeRequestButton->setIcon(QIcon());
    }
    else
    {
        ui->clearButton->setIcon(platformStyle->SingleColorIcon(":/icons/remove"));
        ui->receiveButton->setIcon(platformStyle->SingleColorIcon(":/icons/receiving_addresses"));
        ui->showRequestButton->setIcon(platformStyle->SingleColorIcon(":/icons/edit"));
        ui->removeRequestButton->setIcon(platformStyle->SingleColorIcon(":/icons/remove"));
    }

    // context menu actions
    QAction *copyAddressAction = new QAction(tr("Copy address"), this);
    QAction *copyLabelAction = new QAction(tr("Copy label"), this);
    QAction *copyMessageAction = new QAction(tr("Copy message"), this);
    QAction *copyAmountAction = new QAction(tr("Copy amount"), this);

    // context menu
    contextMenu = new QMenu();
    contextMenu->addAction(copyAddressAction);
    contextMenu->addAction(copyLabelAction);
    contextMenu->addAction(copyMessageAction);
    contextMenu->addAction(copyAmountAction);

    // context menu signals
    connect(ui->recentRequestsView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showMenu(QPoint)));
    connect(copyAddressAction, SIGNAL(triggered()), this, SLOT(copyAddress()));
    connect(copyLabelAction, SIGNAL(triggered()), this, SLOT(copyLabel()));
    connect(copyMessageAction, SIGNAL(triggered()), this, SLOT(copyMessage()));
    connect(copyAmountAction, SIGNAL(triggered()), this, SLOT(copyAmount()));

    connect(ui->clearButton, SIGNAL(clicked()), this, SLOT(clear()));

    // initialize freeze
    nFreezeLockTime = CScriptNum(0);
}

void ReceiveCoinsDialog::setModel(WalletModel *model)
{
    this->model = model;

    if (model && model->getOptionsModel())
    {
        model->getRecentRequestsTableModel()->sort(RecentRequestsTableModel::Date, Qt::DescendingOrder);
        connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));
        updateDisplayUnit();

        QTableView *tableView = ui->recentRequestsView;

        tableView->verticalHeader()->hide();
        tableView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        tableView->setModel(model->getRecentRequestsTableModel());
        tableView->setAlternatingRowColors(true);
        tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
        tableView->setSelectionMode(QAbstractItemView::ContiguousSelection);
        tableView->setColumnWidth(RecentRequestsTableModel::Date, DATE_COLUMN_WIDTH);
        tableView->setColumnWidth(RecentRequestsTableModel::Label, LABEL_COLUMN_WIDTH);

        connect(tableView->selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)), this,
            SLOT(recentRequestsView_selectionChanged(QItemSelection, QItemSelection)));
        // Last 2 columns are set by the columnResizingFixer, when the table geometry is ready.
        columnResizingFixer =
            new GUIUtil::TableViewLastColumnResizingFixer(tableView, AMOUNT_MINIMUM_COLUMN_WIDTH, DATE_COLUMN_WIDTH);
    }
}

ReceiveCoinsDialog::~ReceiveCoinsDialog() { delete ui; }
void ReceiveCoinsDialog::clear()
{
    ui->reqAmount->clear();
    ui->reqLabel->setText("");
    ui->reqMessage->setText("");
    ui->reuseAddress->setChecked(false);
    ui->freezeCheck->setChecked(false);
    ui->freezeCheck->setText("Coin &Freeze");
    nFreezeLockTime = CScriptNum(0);
    freezeDialog = NULL;
    updateDisplayUnit();
}

void ReceiveCoinsDialog::reject() { clear(); }
void ReceiveCoinsDialog::accept() { clear(); }
void ReceiveCoinsDialog::updateDisplayUnit()
{
    if (model && model->getOptionsModel())
    {
        ui->reqAmount->setDisplayUnit(model->getOptionsModel()->getDisplayUnit());
    }
}

void ReceiveCoinsDialog::on_freezeDialog_hide()
{
    freezeDialog->getFreezeLockTime(nFreezeLockTime);

    if (nFreezeLockTime == 0)
    {
        // user cancelled freeze
        ui->freezeCheck->setChecked(false);
        ui->freezeCheck->setText("Coin Freeze");
    }
    else
    {
        // user selected freeze
        ui->freezeCheck->setChecked(true);

        QString freezeLabel;
        if (nFreezeLockTime.getint64() < LOCKTIME_THRESHOLD)

        {
            uint64_t height = GetBlockchainHeight();
            uint64_t freezeHeight = nFreezeLockTime.getint();
            uint64_t approxTimeMs =
                ((freezeHeight - height) * 10 * 60 * 1000) + QDateTime::currentDateTime().toMSecsSinceEpoch();

            freezeLabel = (QString)("block: ") + QString::number(freezeHeight) + (QString)(" (approximately: ") +
                          QDateTime::fromMSecsSinceEpoch(approxTimeMs).date().toString() + ")";
        }
        else
            freezeLabel =
                QDateTime::fromMSecsSinceEpoch(nFreezeLockTime.getint64() * 1000).toString("yyyy/MM/dd hh:mm");
        ui->freezeCheck->setText("Coin freeze until " + freezeLabel);
    }
}

void ReceiveCoinsDialog::on_freezeCheck_clicked()
{
    if (ui->freezeCheck->isChecked()) // If the user clicked on coin freeze, bring up the freeze dialog box
    {
        if (!freezeDialog)
        {
            freezeDialog = new ReceiveFreezeDialog(this);
            freezeDialog->setModel(model->getOptionsModel());
        }
        freezeDialog->show();
    }
    else // if the user unchecked, then hide the freeze dialog if its still showing
    {
        if (freezeDialog)
            freezeDialog->hide();
    }
}

void ReceiveCoinsDialog::on_receiveButton_clicked()
{
    if (!model || !model->getOptionsModel() || !model->getAddressTableModel() || !model->getRecentRequestsTableModel())
        return;

    QString address;
    QString label = ui->reqLabel->text();
    QString sFreezeLockTime = "";

    if (ui->reuseAddress->isChecked())
    {
        /* Choose existing receiving address */
        AddressBookPage dlg(platformStyle, AddressBookPage::ForSelection, AddressBookPage::ReceivingTab, this);
        dlg.setModel(model->getAddressTableModel());
        if (dlg.exec())
        {
            address = dlg.getReturnValue();
            if (label.isEmpty()) /* If no label provided, use the previously used label */
            {
                label = model->getAddressTableModel()->labelForAddress(address);
            }
        }
        else
        {
            return;
        }
    }
    else
    {
        /* Generate new receiving address and add to the address table */
        address = model->getAddressTableModel()->addRow(AddressTableModel::Receive, label, "", CScriptNum(0));

        // only use coin freeze if the freeze value is valid and the check box is still set
        if ((nFreezeLockTime > 0) && (ui->freezeCheck) && (ui->freezeCheck->isChecked()))
        {
            /* Generate the freeze redeemScript and add to the address table.
             * The address variable needs to show the freeze P2SH public key  */
            address = model->getAddressTableModel()->addRow(AddressTableModel::Receive, label, "", nFreezeLockTime);
            sFreezeLockTime = model->getAddressTableModel()->labelForFreeze(address);
        }
    }
    SendCoinsRecipient info(address, label, ui->reqAmount->value(), ui->reqMessage->text(), sFreezeLockTime, "");
    ReceiveRequestDialog *dialog = new ReceiveRequestDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setModel(model->getOptionsModel());
    dialog->setInfo(info);
    dialog->show();
    clear();

    /* Store request for later reference */
    model->getRecentRequestsTableModel()->addNewRequest(info);
}

void ReceiveCoinsDialog::on_recentRequestsView_doubleClicked(const QModelIndex &index)
{
    const RecentRequestsTableModel *submodel = model->getRecentRequestsTableModel();
    ReceiveRequestDialog *dialog = new ReceiveRequestDialog(this);
    dialog->setModel(model->getOptionsModel());
    dialog->setInfo(submodel->entry(index.row()).recipient);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
}

void ReceiveCoinsDialog::recentRequestsView_selectionChanged(const QItemSelection &selected,
    const QItemSelection &deselected)
{
    // Enable Show/Remove buttons only if anything is selected.
    bool enable = !ui->recentRequestsView->selectionModel()->selectedRows().isEmpty();
    ui->showRequestButton->setEnabled(enable);
    ui->removeRequestButton->setEnabled(enable);
}

void ReceiveCoinsDialog::on_showRequestButton_clicked()
{
    if (!model || !model->getRecentRequestsTableModel() || !ui->recentRequestsView->selectionModel())
        return;
    QModelIndexList selection = ui->recentRequestsView->selectionModel()->selectedRows();

    Q_FOREACH (const QModelIndex &index, selection)
    {
        on_recentRequestsView_doubleClicked(index);
    }
}

void ReceiveCoinsDialog::on_removeRequestButton_clicked()
{
    if (!model || !model->getRecentRequestsTableModel() || !ui->recentRequestsView->selectionModel())
        return;
    QModelIndexList selection = ui->recentRequestsView->selectionModel()->selectedRows();
    if (selection.empty())
        return;
    // correct for selection mode ContiguousSelection
    QModelIndex firstIndex = selection.at(0);
    model->getRecentRequestsTableModel()->removeRows(firstIndex.row(), selection.length(), firstIndex.parent());
}

// We override the virtual resizeEvent of the QWidget to adjust tables column
// sizes as the tables width is proportional to the dialogs width.
void ReceiveCoinsDialog::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    columnResizingFixer->stretchColumnWidth(RecentRequestsTableModel::Message);
}

void ReceiveCoinsDialog::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Return)
    {
        // press return -> submit form
        if (ui->reqLabel->hasFocus() || ui->reqAmount->hasFocus() || ui->reqMessage->hasFocus())
        {
            event->ignore();
            on_receiveButton_clicked();
            return;
        }
    }

    this->QDialog::keyPressEvent(event);
}

// copy column of selected row to clipboard
void ReceiveCoinsDialog::copyColumnToClipboard(int column)
{
    if (!model || !model->getRecentRequestsTableModel() || !ui->recentRequestsView->selectionModel())
        return;
    QModelIndexList selection = ui->recentRequestsView->selectionModel()->selectedRows();
    if (selection.empty())
        return;
    // correct for selection mode ContiguousSelection
    QModelIndex firstIndex = selection.at(0);
    GUIUtil::setClipboard(model->getRecentRequestsTableModel()
                              ->data(firstIndex.child(firstIndex.row(), column), Qt::EditRole)
                              .toString());
}

// context menu
void ReceiveCoinsDialog::showMenu(const QPoint &point)
{
    if (!model || !model->getRecentRequestsTableModel() || !ui->recentRequestsView->selectionModel())
        return;
    QModelIndexList selection = ui->recentRequestsView->selectionModel()->selectedRows();
    if (selection.empty())
        return;
    contextMenu->exec(QCursor::pos());
}

// context menu action: copy address
void ReceiveCoinsDialog::copyAddress() { copyColumnToClipboard(RecentRequestsTableModel::Address); }
// context menu action: copy label
void ReceiveCoinsDialog::copyLabel() { copyColumnToClipboard(RecentRequestsTableModel::Label); }
// context menu action: copy message
void ReceiveCoinsDialog::copyMessage() { copyColumnToClipboard(RecentRequestsTableModel::Message); }
// context menu action: copy amount
void ReceiveCoinsDialog::copyAmount() { copyColumnToClipboard(RecentRequestsTableModel::Amount); }
