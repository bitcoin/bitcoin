// Copyright (c) 2011-2015 The Bitcoin Core developers
// Copyright (c) 2014-2021 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/coincontroldialog.h>
#include <qt/forms/ui_coincontroldialog.h>

#include <qt/addresstablemodel.h>
#include <qt/bitcoinunits.h>
#include <qt/guiutil.h>
#include <qt/optionsmodel.h>
#include <txmempool.h>
#include <qt/walletmodel.h>

#include <wallet/coincontrol.h>
#include <interfaces/node.h>
#include <key_io.h>
#include <policy/fees.h>
#include <policy/policy.h>
#include <validation.h> // For mempool
#include <wallet/fees.h>
#include <wallet/wallet.h>

#include <QApplication>
#include <QCheckBox>
#include <QCursor>
#include <QDialogButtonBox>
#include <QFlags>
#include <QIcon>
#include <QSettings>
#include <QTreeWidget>

QList<CAmount> CoinControlDialog::payAmounts;
bool CoinControlDialog::fSubtractFeeFromAmount = false;

bool CCoinControlWidgetItem::operator<(const QTreeWidgetItem &other) const {
    int column = treeWidget()->sortColumn();
    if (column == CoinControlDialog::COLUMN_AMOUNT || column == CoinControlDialog::COLUMN_DATE || column == CoinControlDialog::COLUMN_CONFIRMATIONS || column == CoinControlDialog::COLUMN_COINJOIN_ROUNDS)
        return data(column, Qt::UserRole).toLongLong() < other.data(column, Qt::UserRole).toLongLong();
    return QTreeWidgetItem::operator<(other);
}

CoinControlDialog::CoinControlDialog(CCoinControl& coin_control, WalletModel* _model, QWidget* parent) :
    QDialog(parent),
    ui(new Ui::CoinControlDialog),
    m_coin_control(coin_control),
    model(_model)
{
    ui->setupUi(this);

    GUIUtil::setFont({ui->labelCoinControlQuantityText,
                      ui->labelCoinControlBytesText,
                      ui->labelCoinControlAmountText,
                      ui->labelCoinControlLowOutputText,
                      ui->labelCoinControlFeeText,
                      ui->labelCoinControlAfterFeeText,
                      ui->labelCoinControlChangeText
                     }, GUIUtil::FontWeight::Bold);

    GUIUtil::updateFonts();

    GUIUtil::disableMacFocusRect(this);

    // context menu actions
    QAction *copyAddressAction = new QAction(tr("Copy address"), this);
    QAction *copyLabelAction = new QAction(tr("Copy label"), this);
    QAction *copyAmountAction = new QAction(tr("Copy amount"), this);
             copyTransactionHashAction = new QAction(tr("Copy transaction ID"), this);  // we need to enable/disable this
             lockAction = new QAction(tr("Lock unspent"), this);                        // we need to enable/disable this
             unlockAction = new QAction(tr("Unlock unspent"), this);                    // we need to enable/disable this

    // context menu
    contextMenu = new QMenu(this);
    contextMenu->addAction(copyAddressAction);
    contextMenu->addAction(copyLabelAction);
    contextMenu->addAction(copyAmountAction);
    contextMenu->addAction(copyTransactionHashAction);
    contextMenu->addSeparator();
    contextMenu->addAction(lockAction);
    contextMenu->addAction(unlockAction);

    // context menu signals
    connect(ui->treeWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showMenu(QPoint)));
    connect(copyAddressAction, SIGNAL(triggered()), this, SLOT(copyAddress()));
    connect(copyLabelAction, SIGNAL(triggered()), this, SLOT(copyLabel()));
    connect(copyAmountAction, SIGNAL(triggered()), this, SLOT(copyAmount()));
    connect(copyTransactionHashAction, SIGNAL(triggered()), this, SLOT(copyTransactionHash()));
    connect(lockAction, SIGNAL(triggered()), this, SLOT(lockCoin()));
    connect(unlockAction, SIGNAL(triggered()), this, SLOT(unlockCoin()));

    // clipboard actions
    QAction *clipboardQuantityAction = new QAction(tr("Copy quantity"), this);
    QAction *clipboardAmountAction = new QAction(tr("Copy amount"), this);
    QAction *clipboardFeeAction = new QAction(tr("Copy fee"), this);
    QAction *clipboardAfterFeeAction = new QAction(tr("Copy after fee"), this);
    QAction *clipboardBytesAction = new QAction(tr("Copy bytes"), this);
    QAction *clipboardLowOutputAction = new QAction(tr("Copy dust"), this);
    QAction *clipboardChangeAction = new QAction(tr("Copy change"), this);

    connect(clipboardQuantityAction, SIGNAL(triggered()), this, SLOT(clipboardQuantity()));
    connect(clipboardAmountAction, SIGNAL(triggered()), this, SLOT(clipboardAmount()));
    connect(clipboardFeeAction, SIGNAL(triggered()), this, SLOT(clipboardFee()));
    connect(clipboardAfterFeeAction, SIGNAL(triggered()), this, SLOT(clipboardAfterFee()));
    connect(clipboardBytesAction, SIGNAL(triggered()), this, SLOT(clipboardBytes()));
    connect(clipboardLowOutputAction, SIGNAL(triggered()), this, SLOT(clipboardLowOutput()));
    connect(clipboardChangeAction, SIGNAL(triggered()), this, SLOT(clipboardChange()));

    ui->labelCoinControlQuantity->addAction(clipboardQuantityAction);
    ui->labelCoinControlAmount->addAction(clipboardAmountAction);
    ui->labelCoinControlFee->addAction(clipboardFeeAction);
    ui->labelCoinControlAfterFee->addAction(clipboardAfterFeeAction);
    ui->labelCoinControlBytes->addAction(clipboardBytesAction);
    ui->labelCoinControlLowOutput->addAction(clipboardLowOutputAction);
    ui->labelCoinControlChange->addAction(clipboardChangeAction);

    // toggle tree/list mode
    connect(ui->radioTreeMode, SIGNAL(toggled(bool)), this, SLOT(radioTreeMode(bool)));
    connect(ui->radioListMode, SIGNAL(toggled(bool)), this, SLOT(radioListMode(bool)));

    // click on checkbox
    connect(ui->treeWidget, SIGNAL(itemChanged(QTreeWidgetItem*, int)), this, SLOT(viewItemChanged(QTreeWidgetItem*, int)));

    // click on header
    ui->treeWidget->header()->setSectionsClickable(true);
    connect(ui->treeWidget->header(), SIGNAL(sectionClicked(int)), this, SLOT(headerSectionClicked(int)));

    // ok button
    connect(ui->buttonBox, SIGNAL(clicked( QAbstractButton*)), this, SLOT(buttonBoxClicked(QAbstractButton*)));

    // (un)select all
    connect(ui->pushButtonSelectAll, SIGNAL(clicked()), this, SLOT(buttonSelectAllClicked()));

    // Toggle lock state
    connect(ui->pushButtonToggleLock, SIGNAL(clicked()), this, SLOT(buttonToggleLockClicked()));

    ui->treeWidget->setColumnWidth(COLUMN_CHECKBOX, 94);
    ui->treeWidget->setColumnWidth(COLUMN_AMOUNT, 100);
    ui->treeWidget->setColumnWidth(COLUMN_LABEL, 170);
    ui->treeWidget->setColumnWidth(COLUMN_COINJOIN_ROUNDS, 110);
    ui->treeWidget->setColumnWidth(COLUMN_DATE, 120);
    ui->treeWidget->setColumnWidth(COLUMN_CONFIRMATIONS, 110);

    ui->treeWidget->header()->setStretchLastSection(false);
    ui->treeWidget->header()->setSectionResizeMode(COLUMN_ADDRESS, QHeaderView::Stretch);

    // default view is sorted by amount desc
    sortView(COLUMN_AMOUNT, Qt::DescendingOrder);

    // restore list mode and sortorder as a convenience feature
    QSettings settings;
    if (settings.contains("nCoinControlMode") && !settings.value("nCoinControlMode").toBool())
        ui->radioTreeMode->click();
    if (settings.contains("nCoinControlSortColumn") && settings.contains("nCoinControlSortOrder"))
        sortView(settings.value("nCoinControlSortColumn").toInt(), (static_cast<Qt::SortOrder>(settings.value("nCoinControlSortOrder").toInt())));

    if(_model->getOptionsModel() && _model->getAddressTableModel())
    {
        updateView();
        updateLabelLocked();
        CoinControlDialog::updateLabels(m_coin_control, _model, this);
    }
}

CoinControlDialog::~CoinControlDialog()
{
    QSettings settings;
    settings.setValue("nCoinControlMode", ui->radioListMode->isChecked());
    settings.setValue("nCoinControlSortColumn", sortColumn);
    settings.setValue("nCoinControlSortOrder", (int)sortOrder);

    delete ui;
}

// ok button
void CoinControlDialog::buttonBoxClicked(QAbstractButton* button)
{
    if (ui->buttonBox->buttonRole(button) == QDialogButtonBox::AcceptRole)
        done(QDialog::Accepted); // closes the dialog
}

// (un)select all
void CoinControlDialog::buttonSelectAllClicked()
{
    Qt::CheckState state = Qt::Checked;
    for (int i = 0; i < ui->treeWidget->topLevelItemCount(); i++)
    {
        if (ui->treeWidget->topLevelItem(i)->checkState(COLUMN_CHECKBOX) != Qt::Unchecked)
        {
            state = Qt::Unchecked;
            break;
        }
    }
    ui->treeWidget->setEnabled(false);
    for (int i = 0; i < ui->treeWidget->topLevelItemCount(); i++)
            if (ui->treeWidget->topLevelItem(i)->checkState(COLUMN_CHECKBOX) != state)
                ui->treeWidget->topLevelItem(i)->setCheckState(COLUMN_CHECKBOX, state);
    ui->treeWidget->setEnabled(true);
    if (state == Qt::Unchecked)
        m_coin_control.UnSelectAll(); // just to be sure
    CoinControlDialog::updateLabels(m_coin_control, model, this);
}

// Toggle lock state
void CoinControlDialog::buttonToggleLockClicked()
{
    QTreeWidgetItem *item;
    // Works in list-mode only
    if(ui->radioListMode->isChecked()){
        ui->treeWidget->setEnabled(false);
        for (int i = 0; i < ui->treeWidget->topLevelItemCount(); i++){
            item = ui->treeWidget->topLevelItem(i);
            COutPoint outpt(uint256S(item->data(COLUMN_ADDRESS, TxHashRole).toString().toStdString()), item->data(COLUMN_ADDRESS, VOutRole).toUInt());
            // Don't toggle the lock state of partially mixed coins if they are not hidden in CoinJoin mode
            if (m_coin_control.IsUsingCoinJoin() && !fHideAdditional && !model->isFullyMixed(outpt)) {
                continue;
            }
            if (model->wallet().isLockedCoin(outpt)) {
                model->wallet().unlockCoin(outpt);
                item->setDisabled(false);
                item->setIcon(COLUMN_CHECKBOX, QIcon());
            }
            else{
                model->wallet().lockCoin(outpt);
                item->setDisabled(true);
                item->setIcon(COLUMN_CHECKBOX, GUIUtil::getIcon("lock_closed", GUIUtil::ThemedColor::RED));
            }
            updateLabelLocked();
        }
        ui->treeWidget->setEnabled(true);
        CoinControlDialog::updateLabels(m_coin_control, model, this);
    }
    else{
        QMessageBox msgBox(this);
        msgBox.setObjectName("lockMessageBox");
        msgBox.setText(tr("Please switch to \"List mode\" to use this function."));
        msgBox.exec();
    }
}

// context menu
void CoinControlDialog::showMenu(const QPoint &point)
{
    QTreeWidgetItem *item = ui->treeWidget->itemAt(point);
    if(item)
    {
        contextMenuItem = item;

        // disable some items (like Copy Transaction ID, lock, unlock) for tree roots in context menu
        if (item->data(COLUMN_ADDRESS, TxHashRole).toString().length() == 64) // transaction hash is 64 characters (this means it is a child node, so it is not a parent node in tree mode)
        {
            copyTransactionHashAction->setEnabled(true);
            if (model->wallet().isLockedCoin(COutPoint(uint256S(item->data(COLUMN_ADDRESS, TxHashRole).toString().toStdString()), item->data(COLUMN_ADDRESS, VOutRole).toUInt())))
            {
                lockAction->setEnabled(false);
                unlockAction->setEnabled(true);
            }
            else
            {
                lockAction->setEnabled(true);
                unlockAction->setEnabled(false);
            }
        }
        else // this means click on parent node in tree mode -> disable all
        {
            copyTransactionHashAction->setEnabled(false);
            lockAction->setEnabled(false);
            unlockAction->setEnabled(false);
        }

        // show context menu
        contextMenu->exec(QCursor::pos());
    }
}

// context menu action: copy amount
void CoinControlDialog::copyAmount()
{
    GUIUtil::setClipboard(BitcoinUnits::removeSpaces(contextMenuItem->text(COLUMN_AMOUNT)));
}

// context menu action: copy label
void CoinControlDialog::copyLabel()
{
    if (ui->radioTreeMode->isChecked() && contextMenuItem->text(COLUMN_LABEL).length() == 0 && contextMenuItem->parent())
        GUIUtil::setClipboard(contextMenuItem->parent()->text(COLUMN_LABEL));
    else
        GUIUtil::setClipboard(contextMenuItem->text(COLUMN_LABEL));
}

// context menu action: copy address
void CoinControlDialog::copyAddress()
{
    if (ui->radioTreeMode->isChecked() && contextMenuItem->text(COLUMN_ADDRESS).length() == 0 && contextMenuItem->parent())
        GUIUtil::setClipboard(contextMenuItem->parent()->text(COLUMN_ADDRESS));
    else
        GUIUtil::setClipboard(contextMenuItem->text(COLUMN_ADDRESS));
}

// context menu action: copy transaction id
void CoinControlDialog::copyTransactionHash()
{
    GUIUtil::setClipboard(contextMenuItem->data(COLUMN_ADDRESS, TxHashRole).toString());
}

// context menu action: lock coin
void CoinControlDialog::lockCoin()
{
    if (contextMenuItem->checkState(COLUMN_CHECKBOX) == Qt::Checked)
        contextMenuItem->setCheckState(COLUMN_CHECKBOX, Qt::Unchecked);

    COutPoint outpt(uint256S(contextMenuItem->data(COLUMN_ADDRESS, TxHashRole).toString().toStdString()), contextMenuItem->data(COLUMN_ADDRESS, VOutRole).toUInt());
    model->wallet().lockCoin(outpt);
    contextMenuItem->setDisabled(true);
    contextMenuItem->setIcon(COLUMN_CHECKBOX, GUIUtil::getIcon("lock_closed", GUIUtil::ThemedColor::RED));
    updateLabelLocked();
}

// context menu action: unlock coin
void CoinControlDialog::unlockCoin()
{
    COutPoint outpt(uint256S(contextMenuItem->data(COLUMN_ADDRESS, TxHashRole).toString().toStdString()), contextMenuItem->data(COLUMN_ADDRESS, VOutRole).toUInt());
    model->wallet().unlockCoin(outpt);
    contextMenuItem->setDisabled(false);
    contextMenuItem->setIcon(COLUMN_CHECKBOX, QIcon());
    updateLabelLocked();
}

// copy label "Quantity" to clipboard
void CoinControlDialog::clipboardQuantity()
{
    GUIUtil::setClipboard(ui->labelCoinControlQuantity->text());
}

// copy label "Amount" to clipboard
void CoinControlDialog::clipboardAmount()
{
    GUIUtil::setClipboard(ui->labelCoinControlAmount->text().left(ui->labelCoinControlAmount->text().indexOf(" ")));
}

// copy label "Fee" to clipboard
void CoinControlDialog::clipboardFee()
{
    GUIUtil::setClipboard(ui->labelCoinControlFee->text().left(ui->labelCoinControlFee->text().indexOf(" ")).replace(ASYMP_UTF8, ""));
}

// copy label "After fee" to clipboard
void CoinControlDialog::clipboardAfterFee()
{
    GUIUtil::setClipboard(ui->labelCoinControlAfterFee->text().left(ui->labelCoinControlAfterFee->text().indexOf(" ")).replace(ASYMP_UTF8, ""));
}

// copy label "Bytes" to clipboard
void CoinControlDialog::clipboardBytes()
{
    GUIUtil::setClipboard(ui->labelCoinControlBytes->text().replace(ASYMP_UTF8, ""));
}

// copy label "Dust" to clipboard
void CoinControlDialog::clipboardLowOutput()
{
    GUIUtil::setClipboard(ui->labelCoinControlLowOutput->text());
}

// copy label "Change" to clipboard
void CoinControlDialog::clipboardChange()
{
    GUIUtil::setClipboard(ui->labelCoinControlChange->text().left(ui->labelCoinControlChange->text().indexOf(" ")).replace(ASYMP_UTF8, ""));
}

// treeview: sort
void CoinControlDialog::sortView(int column, Qt::SortOrder order)
{
    sortColumn = column;
    sortOrder = order;
    ui->treeWidget->sortItems(column, order);
    ui->treeWidget->header()->setSortIndicator(sortColumn, sortOrder);
}

// treeview: clicked on header
void CoinControlDialog::headerSectionClicked(int logicalIndex)
{
    if (logicalIndex == COLUMN_CHECKBOX) // click on most left column -> do nothing
    {
        ui->treeWidget->header()->setSortIndicator(sortColumn, sortOrder);
    }
    else
    {
        if (sortColumn == logicalIndex)
            sortOrder = ((sortOrder == Qt::AscendingOrder) ? Qt::DescendingOrder : Qt::AscendingOrder);
        else
        {
            sortColumn = logicalIndex;
            sortOrder = ((sortColumn == COLUMN_LABEL || sortColumn == COLUMN_ADDRESS) ? Qt::AscendingOrder : Qt::DescendingOrder); // if label or address then default => asc, else default => desc
        }

        sortView(sortColumn, sortOrder);
    }
}

// toggle tree mode
void CoinControlDialog::radioTreeMode(bool checked)
{
    if (checked && model)
        updateView();
}

// toggle list mode
void CoinControlDialog::radioListMode(bool checked)
{
    if (checked && model)
        updateView();
}

// checkbox clicked by user
void CoinControlDialog::viewItemChanged(QTreeWidgetItem* item, int column)
{
    if (column == COLUMN_CHECKBOX && item->data(COLUMN_ADDRESS, TxHashRole).toString().length() == 64) // transaction hash is 64 characters (this means it is a child node, so it is not a parent node in tree mode)
    {
        COutPoint outpt(uint256S(item->data(COLUMN_ADDRESS, TxHashRole).toString().toStdString()), item->data(COLUMN_ADDRESS, VOutRole).toUInt());

        if (item->checkState(COLUMN_CHECKBOX) == Qt::Unchecked)
            m_coin_control.UnSelect(outpt);
        else if (item->isDisabled()) // locked (this happens if "check all" through parent node)
            item->setCheckState(COLUMN_CHECKBOX, Qt::Unchecked);
        else {
            m_coin_control.Select(outpt);
        }

        // selection changed -> update labels
        if (ui->treeWidget->isEnabled()) // do not update on every click for (un)select all
            CoinControlDialog::updateLabels(m_coin_control, model, this);
    }
}

// shows count of locked unspent outputs
void CoinControlDialog::updateLabelLocked()
{
    std::vector<COutPoint> vOutpts;
    model->wallet().listLockedCoins(vOutpts);
    if (vOutpts.size() > 0)
    {
       ui->labelLocked->setText(tr("(%1 locked)").arg(vOutpts.size()));
       ui->labelLocked->setVisible(true);
    }
    else ui->labelLocked->setVisible(false);
}

void CoinControlDialog::on_hideButton_clicked()
{
    fHideAdditional = !fHideAdditional;
    updateView();
    CoinControlDialog::updateLabels(m_coin_control, model, this);
}

void CoinControlDialog::updateLabels(CCoinControl& m_coin_control, WalletModel *model, QDialog* dialog)
{
    if (!model)
        return;

    // nPayAmount
    CAmount nPayAmount = 0;
    bool fDust = false;
    CMutableTransaction txDummy;
    for (const CAmount &amount : CoinControlDialog::payAmounts)
    {
        nPayAmount += amount;

        if (amount > 0)
        {
            CTxOut txout(amount, static_cast<CScript>(std::vector<unsigned char>(24, 0)));
            txDummy.vout.push_back(txout);
            fDust |= IsDust(txout, model->node().getDustRelayFee());
        }
    }

    CAmount nAmount             = 0;
    CAmount nPayFee             = 0;
    CAmount nAfterFee           = 0;
    CAmount nChange             = 0;
    unsigned int nBytes         = 0;
    unsigned int nBytesInputs   = 0;
    unsigned int nQuantity      = 0;
    int nQuantityUncompressed   = 0;
    bool fUnselectedSpent{false};
    bool fUnselectedNonMixed{false};

    std::vector<COutPoint> vCoinControl;
    m_coin_control.ListSelected(vCoinControl);

    size_t i = 0;
    for (const auto& out : model->wallet().getCoins(vCoinControl)) {
        if (out.depth_in_main_chain < 0) continue;

        // unselect already spent, very unlikely scenario, this could happen
        // when selected are spent elsewhere, like rpc or another computer
        const COutPoint& outpt = vCoinControl[i++];
        if (out.is_spent)
        {
            m_coin_control.UnSelect(outpt);
            fUnselectedSpent = true;
            continue;
        }

        // Quantity
        nQuantity++;

        // Amount
        nAmount += out.txout.nValue;

        // Bytes
        CTxDestination address;
        if(ExtractDestination(out.txout.scriptPubKey, address))
        {
            CPubKey pubkey;
            CKeyID *keyid = boost::get<CKeyID>(&address);
            if (keyid && model->wallet().getPubKey(*keyid, pubkey))
            {
                nBytesInputs += (pubkey.IsCompressed() ? 148 : 180);
            }
            else
                nBytesInputs += 148; // in all error cases, simply assume 148 here
        }
        else nBytesInputs += 148;
    }

    // calculation
    if (nQuantity > 0)
    {
        // Bytes
        nBytes = nBytesInputs + ((CoinControlDialog::payAmounts.size() > 0 ? CoinControlDialog::payAmounts.size() + 1 : 2) * 34) + 10; // always assume +1 output for change here

        // in the subtract fee from amount case, we can tell if zero change already and subtract the bytes, so that fee calculation afterwards is accurate
        if (CoinControlDialog::fSubtractFeeFromAmount)
            if (nAmount - nPayAmount == 0)
                nBytes -= 34;

        // Fee
        nPayFee = model->wallet().getMinimumFee(nBytes, m_coin_control, nullptr /* returned_target */, nullptr /* reason */);

        if (nPayAmount > 0)
        {
            nChange = nAmount - nPayAmount;

            // CoinJoin Fee = overpay
            if(m_coin_control.IsUsingCoinJoin() && nChange > 0)
            {
                nPayFee = std::max(nChange, nPayFee);
                nChange = 0;
                if (CoinControlDialog::fSubtractFeeFromAmount)
                    nBytes -= 34; // we didn't detect lack of change above
            } else if (!CoinControlDialog::fSubtractFeeFromAmount) {
                nChange -= nPayFee;
            }

            // Never create dust outputs; if we would, just add the dust to the fee.
            if (nChange > 0 && nChange < MIN_CHANGE)
            {
                CTxOut txout(nChange, static_cast<CScript>(std::vector<unsigned char>(24, 0)));
                if (IsDust(txout, model->node().getDustRelayFee()))
                {
                    nPayFee += nChange;
                    nChange = 0;
                    if (CoinControlDialog::fSubtractFeeFromAmount)
                        nBytes -= 34; // we didn't detect lack of change above
                }
            }

            if (nChange == 0 && !CoinControlDialog::fSubtractFeeFromAmount)
                nBytes -= 34;
        }

        // after fee
        nAfterFee = std::max<CAmount>(nAmount - nPayFee, 0);
    }

    // actually update labels
    int nDisplayUnit = BitcoinUnits::DASH;
    if (model && model->getOptionsModel())
        nDisplayUnit = model->getOptionsModel()->getDisplayUnit();

    QLabel *l1 = dialog->findChild<QLabel *>("labelCoinControlQuantity");
    QLabel *l2 = dialog->findChild<QLabel *>("labelCoinControlAmount");
    QLabel *l3 = dialog->findChild<QLabel *>("labelCoinControlFee");
    QLabel *l4 = dialog->findChild<QLabel *>("labelCoinControlAfterFee");
    QLabel *l5 = dialog->findChild<QLabel *>("labelCoinControlBytes");
    QLabel *l7 = dialog->findChild<QLabel *>("labelCoinControlLowOutput");
    QLabel *l8 = dialog->findChild<QLabel *>("labelCoinControlChange");

    // enable/disable "dust" and "change"
    dialog->findChild<QLabel *>("labelCoinControlLowOutputText")->setEnabled(nPayAmount > 0);
    dialog->findChild<QLabel *>("labelCoinControlLowOutput")    ->setEnabled(nPayAmount > 0);
    dialog->findChild<QLabel *>("labelCoinControlChangeText")   ->setEnabled(nPayAmount > 0);
    dialog->findChild<QLabel *>("labelCoinControlChange")       ->setEnabled(nPayAmount > 0);

    // stats
    l1->setText(QString::number(nQuantity));                                 // Quantity
    l2->setText(BitcoinUnits::formatWithUnit(nDisplayUnit, nAmount));        // Amount
    l3->setText(BitcoinUnits::formatWithUnit(nDisplayUnit, nPayFee));        // Fee
    l4->setText(BitcoinUnits::formatWithUnit(nDisplayUnit, nAfterFee));      // After Fee
    l5->setText(((nBytes > 0) ? ASYMP_UTF8 : "") + QString::number(nBytes));        // Bytes
    l7->setText(fDust ? tr("yes") : tr("no"));                               // Dust
    l8->setText(BitcoinUnits::formatWithUnit(nDisplayUnit, nChange));        // Change
    if (nPayFee > 0)
    {
        l3->setText(ASYMP_UTF8 + l3->text());
        l4->setText(ASYMP_UTF8 + l4->text());
        if (nChange > 0 && !CoinControlDialog::fSubtractFeeFromAmount)
            l8->setText(ASYMP_UTF8 + l8->text());
    }

    // turn label red when dust
    l7->setStyleSheet((fDust) ? GUIUtil::getThemedStyleQString(GUIUtil::ThemedStyle::TS_ERROR) : "");

    // tool tips
    QString toolTipDust = tr("This label turns red if any recipient receives an amount smaller than the current dust threshold.");

    // how many satoshis the estimated fee can vary per byte we guess wrong
    double dFeeVary = (nBytes != 0) ? (double)nPayFee / nBytes : 0;

    QString toolTip4 = tr("Can vary +/- %1 duff(s) per input.").arg(dFeeVary);

    l3->setToolTip(toolTip4);
    l4->setToolTip(toolTip4);
    l7->setToolTip(toolTipDust);
    l8->setToolTip(toolTip4);
    dialog->findChild<QLabel *>("labelCoinControlFeeText")      ->setToolTip(l3->toolTip());
    dialog->findChild<QLabel *>("labelCoinControlAfterFeeText") ->setToolTip(l4->toolTip());
    dialog->findChild<QLabel *>("labelCoinControlBytesText")    ->setToolTip(l5->toolTip());
    dialog->findChild<QLabel *>("labelCoinControlLowOutputText")->setToolTip(l7->toolTip());
    dialog->findChild<QLabel *>("labelCoinControlChangeText")   ->setToolTip(l8->toolTip());

    // Insufficient funds
    QLabel *label = dialog->findChild<QLabel *>("labelCoinControlInsuffFunds");
    if (label)
        label->setVisible(nChange < 0);

    // Warn about unselected coins
    if (fUnselectedSpent) {
        QMessageBox::warning(dialog, "CoinControl",
            tr("Some coins were unselected because they were spent."),
            QMessageBox::Ok, QMessageBox::Ok);
    } else if (fUnselectedNonMixed) {
        QMessageBox::warning(dialog, "CoinControl",
            tr("Some coins were unselected because they do not have enough mixing rounds."),
            QMessageBox::Ok, QMessageBox::Ok);
    }
}

void CoinControlDialog::updateView()
{
    if (!model || !model->getOptionsModel() || !model->getAddressTableModel())
        return;

    bool fNormalMode = !m_coin_control.IsUsingCoinJoin();
    ui->treeWidget->setColumnHidden(COLUMN_COINJOIN_ROUNDS, fNormalMode);
    ui->treeWidget->setColumnHidden(COLUMN_LABEL, !fNormalMode);
    ui->radioTreeMode->setVisible(fNormalMode);
    ui->radioListMode->setVisible(fNormalMode);

    if (!model->node().coinJoinOptions().isEnabled()) {
        fHideAdditional = false;
        ui->hideButton->setVisible(false);
    }

    QString strHideButton;
    if (fNormalMode) {
        if (fHideAdditional) {
            strHideButton = tr("Show all coins");
        } else {
            strHideButton = tr("Hide %1 coins").arg("CoinJoin");
        }
    } else {
        if (fHideAdditional) {
            strHideButton = tr("Show all %1 coins").arg("CoinJoin");
        } else {
            strHideButton = tr("Show spendable coins only");
        }
        ui->radioListMode->setChecked(true);
    }
    ui->hideButton->setText(strHideButton);

    bool treeMode = ui->radioTreeMode->isChecked();
    ui->treeWidget->clear();
    ui->treeWidget->setEnabled(false); // performance, otherwise updateLabels would be called for every checked checkbox
    ui->treeWidget->setAlternatingRowColors(!treeMode);
    QFlags<Qt::ItemFlag> flgCheckbox = Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable;
    QFlags<Qt::ItemFlag> flgTristate = Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsTristate;

    int nDisplayUnit = model->getOptionsModel()->getDisplayUnit();

    for (const auto& coins : model->wallet().listCoins()) {
        CCoinControlWidgetItem *itemWalletAddress = new CCoinControlWidgetItem();
        itemWalletAddress->setCheckState(COLUMN_CHECKBOX, Qt::Unchecked);
        QString sWalletAddress = QString::fromStdString(EncodeDestination(coins.first));
        QString sWalletLabel = model->getAddressTableModel()->labelForAddress(sWalletAddress);
        if (sWalletLabel.isEmpty())
            sWalletLabel = tr("(no label)");

        if (treeMode)
        {
            // wallet address
            ui->treeWidget->addTopLevelItem(itemWalletAddress);

            itemWalletAddress->setFlags(flgTristate);
            itemWalletAddress->setCheckState(COLUMN_CHECKBOX, Qt::Unchecked);

            // label
            itemWalletAddress->setText(COLUMN_LABEL, sWalletLabel);
            itemWalletAddress->setToolTip(COLUMN_LABEL, sWalletLabel);

            // address
            itemWalletAddress->setText(COLUMN_ADDRESS, sWalletAddress);
            itemWalletAddress->setToolTip(COLUMN_ADDRESS, sWalletAddress);
        }

        CAmount nSum = 0;
        int nChildren = 0;
        for (const auto& outpair : coins.second) {
            const COutPoint& output = std::get<0>(outpair);
            const interfaces::WalletTxOut& out = std::get<1>(outpair);
            bool fFullyMixed{false};
            CAmount nAmount = out.txout.nValue;
            bool fCoinJoinAmount = model->node().coinJoinOptions().isDenominated(nAmount) || model->node().coinJoinOptions().isCollateralAmount(nAmount);

            if (m_coin_control.IsUsingCoinJoin()) {
                fFullyMixed = model->isFullyMixed(output);
                if ((fHideAdditional && !fFullyMixed) || (!fHideAdditional && !fCoinJoinAmount)) {
                    m_coin_control.UnSelect(output);
                    continue;
                }
            } else {
                if (fHideAdditional && fCoinJoinAmount) {
                    m_coin_control.UnSelect(output);
                    continue;
                }
            }

            nSum += out.txout.nValue;
            nChildren++;

            CCoinControlWidgetItem* itemOutput;
            if (treeMode) {
                itemOutput = new CCoinControlWidgetItem(itemWalletAddress);
            } else {
                itemOutput = new CCoinControlWidgetItem(ui->treeWidget);
            }
            itemOutput->setFlags(flgCheckbox);
            itemOutput->setCheckState(COLUMN_CHECKBOX, Qt::Unchecked);

            // address
            CTxDestination outputAddress;
            QString sAddress = "";
            if (ExtractDestination(out.txout.scriptPubKey, outputAddress)) {
                sAddress = QString::fromStdString(EncodeDestination(outputAddress));

                // if listMode or change => show dash address. In tree mode, address is not shown again for direct wallet address outputs
                if (!treeMode || (!(sAddress == sWalletAddress))) {
                    itemOutput->setText(COLUMN_ADDRESS, sAddress);
                }

                itemOutput->setToolTip(COLUMN_ADDRESS, sAddress);
            }

            // label
            if (!(sAddress == sWalletAddress)) { //change
                // tooltip from where the change comes from
                itemOutput->setToolTip(COLUMN_LABEL, tr("change from %1 (%2)").arg(sWalletLabel).arg(sWalletAddress));
                itemOutput->setText(COLUMN_LABEL, tr("(change)"));
            } else if (!treeMode) {
                QString sLabel = model->getAddressTableModel()->labelForAddress(sAddress);
                if (sLabel.isEmpty()) {
                    sLabel = tr("(no label)");
                }
                itemOutput->setText(COLUMN_LABEL, sLabel);
            }

            // amount
            itemOutput->setText(COLUMN_AMOUNT, BitcoinUnits::format(nDisplayUnit, out.txout.nValue));
            itemOutput->setToolTip(COLUMN_AMOUNT, BitcoinUnits::format(nDisplayUnit, out.txout.nValue));
            itemOutput->setData(COLUMN_AMOUNT, Qt::UserRole, QVariant((qlonglong)out.txout.nValue)); // padding so that sorting works correctly

            // date
            itemOutput->setText(COLUMN_DATE, GUIUtil::dateTimeStr(out.time));
            itemOutput->setToolTip(COLUMN_DATE, GUIUtil::dateTimeStr(out.time));
            itemOutput->setData(COLUMN_DATE, Qt::UserRole, QVariant((qlonglong)out.time));

            // CoinJoin rounds
            int nRounds = model->getRealOutpointCoinJoinRounds(output);
            if (nRounds >= 0 || LogAcceptCategory(BCLog::COINJOIN)) {
                itemOutput->setText(COLUMN_COINJOIN_ROUNDS, QString::number(nRounds));
            } else {
                itemOutput->setText(COLUMN_COINJOIN_ROUNDS, tr("n/a"));
            }
            itemOutput->setData(COLUMN_COINJOIN_ROUNDS, Qt::UserRole, QVariant((qlonglong)nRounds));

            // confirmations
            itemOutput->setText(COLUMN_CONFIRMATIONS, QString::number(out.depth_in_main_chain));
            itemOutput->setData(COLUMN_CONFIRMATIONS, Qt::UserRole, QVariant((qlonglong)out.depth_in_main_chain));

            // transaction hash
            itemOutput->setData(COLUMN_ADDRESS, TxHashRole, QString::fromStdString(output.hash.GetHex()));

            // vout index
            itemOutput->setData(COLUMN_ADDRESS, VOutRole, output.n);

            // disable locked coins
            if (model->wallet().isLockedCoin(output)) {
                m_coin_control.UnSelect(output); // just to be sure
                itemOutput->setDisabled(true);
                itemOutput->setIcon(COLUMN_CHECKBOX, GUIUtil::getIcon("lock_closed", GUIUtil::ThemedColor::RED));
            }

            // set checkbox
            if (m_coin_control.IsSelected(output)) {
                itemOutput->setCheckState(COLUMN_CHECKBOX, Qt::Checked);
            }

            if (m_coin_control.IsUsingCoinJoin() && !fHideAdditional && !fFullyMixed) {
                itemOutput->setDisabled(true);
            }
        }

        // amount
        if (treeMode)
        {
            itemWalletAddress->setText(COLUMN_CHECKBOX, "(" + QString::number(nChildren) + ")");
            itemWalletAddress->setText(COLUMN_AMOUNT, BitcoinUnits::format(nDisplayUnit, nSum));
            itemWalletAddress->setToolTip(COLUMN_AMOUNT, BitcoinUnits::format(nDisplayUnit, nSum));
            itemWalletAddress->setData(COLUMN_AMOUNT, Qt::UserRole, QVariant((qlonglong)nSum));
        }
    }

    // expand all partially selected and hide the empty
    if (treeMode)
    {
        for (int i = 0; i < ui->treeWidget->topLevelItemCount(); i++) {
            QTreeWidgetItem* topLevelItem = ui->treeWidget->topLevelItem(i);
            topLevelItem->setHidden(topLevelItem->childCount() == 0);
            if (topLevelItem->checkState(COLUMN_CHECKBOX) == Qt::PartiallyChecked)
                topLevelItem->setExpanded(true);
        }
    }

    // sort view
    sortView(sortColumn, sortOrder);
    ui->treeWidget->setEnabled(true);
}
