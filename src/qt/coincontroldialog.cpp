#include "coincontroldialog.h"
#include "ui_coincontroldialog.h"

#include "init.h"
#include "base58.h"
#include "bitcoinunits.h"
#include "script.h"
#include "walletmodel.h"
#include "addresstablemodel.h"
#include "optionsmodel.h"
#include "coincontrol.h"
#include "dialogwindowflags.h"

#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QColor>
#include <QCursor>
#include <QDateTime>
#include <QDialogButtonBox>
#include <QFlags>
#include <QIcon>
#include <QString>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QKeyEvent>

using namespace std;
QList<qint64> CoinControlDialog::payAmounts;
CCoinControl* CoinControlDialog::coinControl = new CCoinControl();

CoinControlDialog::CoinControlDialog(QWidget *parent) :
    QWidget(parent, DIALOGWINDOWHINTS),
    ui(new Ui::CoinControlDialog),
    model(0)
{
    ui->setupUi(this);

    // context menu actions
    QAction *copyAddressAction = new QAction(tr("Copy address"), this);
    QAction *copyLabelAction = new QAction(tr("Copy label"), this);
    QAction *copyAmountAction = new QAction(tr("Copy amount"), this);
             copyTransactionHashAction = new QAction(tr("Copy transaction ID"), this);  // we need to enable/disable this
             //lockAction = new QAction(tr("Lock unspent"), this);                        // we need to enable/disable this
             //unlockAction = new QAction(tr("Unlock unspent"), this);                    // we need to enable/disable this

    // context menu
    contextMenu = new QMenu();
    contextMenu->addAction(copyAddressAction);
    contextMenu->addAction(copyLabelAction);
    contextMenu->addAction(copyAmountAction);
    contextMenu->addAction(copyTransactionHashAction);
    //contextMenu->addSeparator();
    //contextMenu->addAction(lockAction);
    //contextMenu->addAction(unlockAction);

    // context menu signals
    connect(ui->treeWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showMenu(QPoint)));
    connect(copyAddressAction, SIGNAL(triggered()), this, SLOT(copyAddress()));
    connect(copyLabelAction, SIGNAL(triggered()), this, SLOT(copyLabel()));
    connect(copyAmountAction, SIGNAL(triggered()), this, SLOT(copyAmount()));
    connect(copyTransactionHashAction, SIGNAL(triggered()), this, SLOT(copyTransactionHash()));
    //connect(lockAction, SIGNAL(triggered()), this, SLOT(lockCoin()));
    //connect(unlockAction, SIGNAL(triggered()), this, SLOT(unlockCoin()));

    // clipboard actions
    QAction *clipboardQuantityAction = new QAction(tr("Copy quantity"), this);
    QAction *clipboardAmountAction = new QAction(tr("Copy amount"), this);
    QAction *clipboardFeeAction = new QAction(tr("Copy fee"), this);
    QAction *clipboardAfterFeeAction = new QAction(tr("Copy after fee"), this);
    QAction *clipboardBytesAction = new QAction(tr("Copy bytes"), this);
    QAction *clipboardPriorityAction = new QAction(tr("Copy priority"), this);
    QAction *clipboardLowOutputAction = new QAction(tr("Copy low output"), this);
    QAction *clipboardChangeAction = new QAction(tr("Copy change"), this);

    connect(clipboardQuantityAction, SIGNAL(triggered()), this, SLOT(clipboardQuantity()));
    connect(clipboardAmountAction, SIGNAL(triggered()), this, SLOT(clipboardAmount()));
    connect(clipboardFeeAction, SIGNAL(triggered()), this, SLOT(clipboardFee()));
    connect(clipboardAfterFeeAction, SIGNAL(triggered()), this, SLOT(clipboardAfterFee()));
    connect(clipboardBytesAction, SIGNAL(triggered()), this, SLOT(clipboardBytes()));
    connect(clipboardPriorityAction, SIGNAL(triggered()), this, SLOT(clipboardPriority()));
    connect(clipboardLowOutputAction, SIGNAL(triggered()), this, SLOT(clipboardLowOutput()));
    connect(clipboardChangeAction, SIGNAL(triggered()), this, SLOT(clipboardChange()));

    ui->labelCoinControlQuantity->addAction(clipboardQuantityAction);
    ui->labelCoinControlAmount->addAction(clipboardAmountAction);
    ui->labelCoinControlFee->addAction(clipboardFeeAction);
    ui->labelCoinControlAfterFee->addAction(clipboardAfterFeeAction);
    ui->labelCoinControlBytes->addAction(clipboardBytesAction);
    ui->labelCoinControlPriority->addAction(clipboardPriorityAction);
    ui->labelCoinControlLowOutput->addAction(clipboardLowOutputAction);
    ui->labelCoinControlChange->addAction(clipboardChangeAction);

    // toggle tree/list mode
    connect(ui->radioTreeMode, SIGNAL(toggled(bool)), this, SLOT(radioTreeMode(bool)));
    connect(ui->radioListMode, SIGNAL(toggled(bool)), this, SLOT(radioListMode(bool)));

    // click on checkbox
    connect(ui->treeWidget, SIGNAL(itemChanged( QTreeWidgetItem*, int)), this, SLOT(viewItemChanged( QTreeWidgetItem*, int)));

    // click on header
#if QT_VERSION < 0x050000
    ui->treeWidget->header()->setClickable(true);
#else
    ui->treeWidget->header()->setSectionsClickable(true);
#endif
    connect(ui->treeWidget->header(), SIGNAL(sectionClicked(int)), this, SLOT(headerSectionClicked(int)));

    // (un)select all
    connect(ui->pushButtonSelectAll, SIGNAL(clicked()), this, SLOT(buttonSelectAllClicked()));
    
    ui->treeWidget->headerItem()->setText(COLUMN_CHECKBOX, QString());

    ui->treeWidget->setColumnWidth(COLUMN_CHECKBOX, 84);
    ui->treeWidget->setColumnWidth(COLUMN_AMOUNT, 100);
    ui->treeWidget->setColumnWidth(COLUMN_LABEL, 170);
    ui->treeWidget->setColumnWidth(COLUMN_ADDRESS, 250);
    ui->treeWidget->setColumnWidth(COLUMN_DATE, 90);
    ui->treeWidget->setColumnWidth(COLUMN_CONFIRMATIONS, 70);
    ui->treeWidget->setColumnWidth(COLUMN_PRIORITY, 100);
    ui->treeWidget->setColumnWidth(COLUMN_WEIGHT, 100);
    ui->treeWidget->setColumnHidden(COLUMN_TXHASH, true);         // store transacton hash in this column, but dont show it
    ui->treeWidget->setColumnHidden(COLUMN_VOUT_INDEX, true);     // store vout index in this column, but dont show it
    ui->treeWidget->setColumnHidden(COLUMN_AMOUNT_INT64, true);   // store amount int64_t in this column, but dont show it
    ui->treeWidget->setColumnHidden(COLUMN_PRIORITY_INT64, true); // store priority int64_t in this column, but dont show it

    // default view is sorted by amount desc
    sortView(COLUMN_AMOUNT_INT64, Qt::DescendingOrder);
}

CoinControlDialog::~CoinControlDialog()
{
    delete ui;
}

void CoinControlDialog::setModel(WalletModel *model)
{
    this->model = model;

    if(model && model->getOptionsModel() && model->getAddressTableModel())
    {
        updateView();
        //updateLabelLocked();
        CoinControlDialog::updateLabels(model, this);
    }
}

// helper function str_pad
QString CoinControlDialog::strPad(QString s, int nPadLength, QString sPadding)
{
    while (s.length() < nPadLength)
        s = sPadding + s;

    return s;
}

// ok button
void CoinControlDialog::on_buttonBox_accepted()
{
    close(); // closes the dialog
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
    CoinControlDialog::updateLabels(model, this);
}

// context menu
void CoinControlDialog::showMenu(const QPoint &point)
{
    QTreeWidgetItem *item = ui->treeWidget->itemAt(point);
    if(item)
    {
        contextMenuItem = item;

        // disable some items (like Copy Transaction ID, lock, unlock) for tree roots in context menu
        if (item->text(COLUMN_TXHASH).length() == 64) // transaction hash is 64 characters (this means its a child node, so its not a parent node in tree mode)
        {
            copyTransactionHashAction->setEnabled(true);
            //if (model->isLockedCoin(uint256(item->text(COLUMN_TXHASH).toStdString()), item->text(COLUMN_VOUT_INDEX).toUInt()))
            //{
            //    lockAction->setEnabled(false);
            //    unlockAction->setEnabled(true);
            //}
            //else
            //{
            //    lockAction->setEnabled(true);
            //    unlockAction->setEnabled(false);
            //}
        }
        else // this means click on parent node in tree mode -> disable all
        {
            copyTransactionHashAction->setEnabled(false);
            //lockAction->setEnabled(false);
            //unlockAction->setEnabled(false);
        }

        // show context menu
        contextMenu->exec(QCursor::pos());
    }
}

// context menu action: copy amount
void CoinControlDialog::copyAmount()
{
    QApplication::clipboard()->setText(contextMenuItem->text(COLUMN_AMOUNT));
}

// context menu action: copy label
void CoinControlDialog::copyLabel()
{
    if (ui->radioTreeMode->isChecked() && contextMenuItem->text(COLUMN_LABEL).length() == 0 && contextMenuItem->parent())
        QApplication::clipboard()->setText(contextMenuItem->parent()->text(COLUMN_LABEL));
    else
        QApplication::clipboard()->setText(contextMenuItem->text(COLUMN_LABEL));
}

// context menu action: copy address
void CoinControlDialog::copyAddress()
{
    if (ui->radioTreeMode->isChecked() && contextMenuItem->text(COLUMN_ADDRESS).length() == 0 && contextMenuItem->parent())
        QApplication::clipboard()->setText(contextMenuItem->parent()->text(COLUMN_ADDRESS));
    else
        QApplication::clipboard()->setText(contextMenuItem->text(COLUMN_ADDRESS));
}

// context menu action: copy transaction id
void CoinControlDialog::copyTransactionHash()
{
    QApplication::clipboard()->setText(contextMenuItem->text(COLUMN_TXHASH));
}

// context menu action: lock coin
/*void CoinControlDialog::lockCoin()
{
    if (contextMenuItem->checkState(COLUMN_CHECKBOX) == Qt::Checked)
        contextMenuItem->setCheckState(COLUMN_CHECKBOX, Qt::Unchecked);

    COutPoint outpt(uint256(contextMenuItem->text(COLUMN_TXHASH).toStdString()), contextMenuItem->text(COLUMN_VOUT_INDEX).toUInt());
    model->lockCoin(outpt);
    contextMenuItem->setDisabled(true);
    contextMenuItem->setIcon(COLUMN_CHECKBOX, QIcon(":/icons/lock_closed"));
    updateLabelLocked();
}*/

// context menu action: unlock coin
/*void CoinControlDialog::unlockCoin()
{
    COutPoint outpt(uint256(contextMenuItem->text(COLUMN_TXHASH).toStdString()), contextMenuItem->text(COLUMN_VOUT_INDEX).toUInt());
    model->unlockCoin(outpt);
    contextMenuItem->setDisabled(false);
    contextMenuItem->setIcon(COLUMN_CHECKBOX, QIcon());
    updateLabelLocked();
}*/

// copy label "Quantity" to clipboard
void CoinControlDialog::clipboardQuantity()
{
    QApplication::clipboard()->setText(ui->labelCoinControlQuantity->text());
}

// copy label "Amount" to clipboard
void CoinControlDialog::clipboardAmount()
{
    QApplication::clipboard()->setText(ui->labelCoinControlAmount->text().left(ui->labelCoinControlAmount->text().indexOf(" ")));
}

// copy label "Fee" to clipboard
void CoinControlDialog::clipboardFee()
{
    QApplication::clipboard()->setText(ui->labelCoinControlFee->text().left(ui->labelCoinControlFee->text().indexOf(" ")));
}

// copy label "After fee" to clipboard
void CoinControlDialog::clipboardAfterFee()
{
    QApplication::clipboard()->setText(ui->labelCoinControlAfterFee->text().left(ui->labelCoinControlAfterFee->text().indexOf(" ")));
}

// copy label "Bytes" to clipboard
void CoinControlDialog::clipboardBytes()
{
    QApplication::clipboard()->setText(ui->labelCoinControlBytes->text());
}

// copy label "Priority" to clipboard
void CoinControlDialog::clipboardPriority()
{
    QApplication::clipboard()->setText(ui->labelCoinControlPriority->text());
}

// copy label "Low output" to clipboard
void CoinControlDialog::clipboardLowOutput()
{
    QApplication::clipboard()->setText(ui->labelCoinControlLowOutput->text());
}

// copy label "Change" to clipboard
void CoinControlDialog::clipboardChange()
{
    QApplication::clipboard()->setText(ui->labelCoinControlChange->text().left(ui->labelCoinControlChange->text().indexOf(" ")));
}

// treeview: sort
void CoinControlDialog::sortView(int column, Qt::SortOrder order)
{
    sortColumn = column;
    sortOrder = order;
    ui->treeWidget->sortItems(column, order);
    ui->treeWidget->header()->setSortIndicator((sortColumn == COLUMN_AMOUNT_INT64 ? COLUMN_AMOUNT : (sortColumn == COLUMN_PRIORITY_INT64 ? COLUMN_PRIORITY : sortColumn)), sortOrder);
}

// treeview: clicked on header
void CoinControlDialog::headerSectionClicked(int logicalIndex)
{
    if (logicalIndex == COLUMN_CHECKBOX) // click on most left column -> do nothing
    {
        ui->treeWidget->header()->setSortIndicator((sortColumn == COLUMN_AMOUNT_INT64 ? COLUMN_AMOUNT : (sortColumn == COLUMN_PRIORITY_INT64 ? COLUMN_PRIORITY : sortColumn)), sortOrder);
    }
    else
    {
        if (logicalIndex == COLUMN_AMOUNT) // sort by amount
            logicalIndex = COLUMN_AMOUNT_INT64;

        if (logicalIndex == COLUMN_PRIORITY) // sort by priority
            logicalIndex = COLUMN_PRIORITY_INT64;

        if (sortColumn == logicalIndex)
            sortOrder = ((sortOrder == Qt::AscendingOrder) ? Qt::DescendingOrder : Qt::AscendingOrder);
        else
        {
            sortColumn = logicalIndex;
            sortOrder = ((sortColumn == COLUMN_AMOUNT_INT64 || sortColumn == COLUMN_PRIORITY_INT64 || sortColumn == COLUMN_DATE || sortColumn == COLUMN_CONFIRMATIONS || sortColumn == COLUMN_WEIGHT) ? Qt::DescendingOrder : Qt::AscendingOrder); // if amount,date,conf,priority then default => desc, else default => asc
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
    if (column == COLUMN_CHECKBOX && item->text(COLUMN_TXHASH).length() == 64) // transaction hash is 64 characters (this means its a child node, so its not a parent node in tree mode)
    {
        COutPoint outpt(uint256(item->text(COLUMN_TXHASH).toStdString()), item->text(COLUMN_VOUT_INDEX).toUInt());

        if (item->checkState(COLUMN_CHECKBOX) == Qt::Unchecked)
            coinControl->UnSelect(outpt);
        else if (item->isDisabled()) // locked (this happens if "check all" through parent node)
            item->setCheckState(COLUMN_CHECKBOX, Qt::Unchecked);
        else
            coinControl->Select(outpt);

        // selection changed -> update labels
        if (ui->treeWidget->isEnabled()) // do not update on every click for (un)select all
            CoinControlDialog::updateLabels(model, this);
    }
}

// helper function, return human readable label for priority number
QString CoinControlDialog::getPriorityLabel(double dPriority)
{
    if (dPriority > 576000ULL) // at least medium, this number is from AllowFree(), the other thresholds are kinda random
    {
        if      (dPriority > 5760000000ULL)   return tr("highest");
        else if (dPriority > 576000000ULL)    return tr("high");
        else if (dPriority > 57600000ULL)     return tr("medium-high");
        else                                    return tr("medium");
    }
    else
    {
        if      (dPriority > 5760ULL) return tr("low-medium");
        else if (dPriority > 58ULL)   return tr("low");
        else                            return tr("lowest");
    }
}

// shows count of locked unspent outputs
/*void CoinControlDialog::updateLabelLocked()
{
    vector<COutPoint> vOutpts;
    model->listLockedCoins(vOutpts);
    if (vOutpts.size() > 0)
    {
       ui->labelLocked->setText(tr("(%1 locked)").arg(vOutpts.size()));
       ui->labelLocked->setVisible(true); 
    }
    else ui->labelLocked->setVisible(false);
}*/

void CoinControlDialog::updateLabels(WalletModel *model, QWidget* dialog)
{
    if (!model) return;

    // nPayAmount
    qint64 nPayAmount = 0;
    bool fLowOutput = false;
    bool fDust = false;
    CTransaction txDummy;
    foreach(const qint64 &amount, CoinControlDialog::payAmounts)
    {
        nPayAmount += amount;

        if (amount > 0)
        {
            if (amount < CENT)
                fLowOutput = true;
            if (amount < MIN_TX_FEE)
                fDust = true;

            CTxOut txout(amount, (CScript)vector<unsigned char>(24, 0));
            txDummy.vout.push_back(txout);
        }
    }

    QString sPriorityLabel      = "";
    int64_t nAmount               = 0;
    int64_t nPayFee               = 0;
    int64_t nAfterFee             = 0;
    int64_t nChange               = 0;
    unsigned int nBytes         = 0;
    unsigned int nBytesInputs   = 0;
    double dPriority            = 0;
    double dPriorityInputs      = 0;
    unsigned int nQuantity      = 0;

    vector<COutPoint> vCoinControl;
    vector<COutput>   vOutputs;
    coinControl->ListSelected(vCoinControl);
    model->getOutputs(vCoinControl, vOutputs);

    BOOST_FOREACH(const COutput& out, vOutputs)
    {
        // Quantity
        nQuantity++;

        // Amount
        nAmount += out.tx->vout[out.i].nValue;

        // Priority
        dPriorityInputs += (double)out.tx->vout[out.i].nValue * (out.nDepth+1);

        // Bytes
        CBitcoinAddress address;
        if(ExtractAddress(*pwalletMain, out.tx->vout[out.i].scriptPubKey, address))
        {
            if (address.IsPair())
                nBytesInputs += 213;
            else if (address.IsPubKey())
            {
                CPubKey pubkey;
                CKeyID keyid;
                if (address.GetKeyID(keyid) && model->getPubKey(keyid, pubkey))
                    nBytesInputs += (pubkey.IsCompressed() ? 148 : 180);
                else
                    nBytesInputs += 148; // in all error cases, simply assume 148 here
            }
        }
    }

    // calculation
    if (nQuantity > 0)
    {
        // Bytes
        nBytes = nBytesInputs + ((CoinControlDialog::payAmounts.size() > 0 ? CoinControlDialog::payAmounts.size() + 1 : 2) * 34) + 10; // always assume +1 output for change here

        // Priority
        dPriority = dPriorityInputs / nBytes;
        sPriorityLabel = CoinControlDialog::getPriorityLabel(dPriority);
        bool fAllowFree = CTransaction::AllowFree(dPriority);

        // Fee
        int64_t nFee = nTransactionFee * (1 + (int64_t)nBytes / 1000);
        // Min Fee
        int64_t nMinFee = txDummy.GetMinFee(1, fAllowFree, GMF_SEND, nBytes);

        nPayFee = max(nFee, nMinFee);

        if (nPayAmount > 0)
        {
            nChange = nAmount - nPayFee - nPayAmount;

            if (nChange == 0)
                nBytes -= 34;
        }

        // after fee
        nAfterFee = nAmount - nPayFee;
        if (nAfterFee < 0)
            nAfterFee = 0;
    }

    // actually update labels
    int nDisplayUnit = BitcoinUnits::BTC;
    if (model && model->getOptionsModel())
        nDisplayUnit = model->getOptionsModel()->getDisplayUnit();

    QLabel *l1 = dialog->findChild<QLabel *>("labelCoinControlQuantity");
    QLabel *l2 = dialog->findChild<QLabel *>("labelCoinControlAmount");
    QLabel *l3 = dialog->findChild<QLabel *>("labelCoinControlFee");
    QLabel *l4 = dialog->findChild<QLabel *>("labelCoinControlAfterFee");
    QLabel *l5 = dialog->findChild<QLabel *>("labelCoinControlBytes");
    QLabel *l6 = dialog->findChild<QLabel *>("labelCoinControlPriority");
    QLabel *l7 = dialog->findChild<QLabel *>("labelCoinControlLowOutput");
    QLabel *l8 = dialog->findChild<QLabel *>("labelCoinControlChange");

    // enable/disable "low output" and "change"
    dialog->findChild<QLabel *>("labelCoinControlLowOutputText")->setEnabled(nPayAmount > 0);
    dialog->findChild<QLabel *>("labelCoinControlLowOutput")    ->setEnabled(nPayAmount > 0);
    dialog->findChild<QLabel *>("labelCoinControlChangeText")   ->setEnabled(nPayAmount > 0);
    dialog->findChild<QLabel *>("labelCoinControlChange")       ->setEnabled(nPayAmount > 0);

    // stats
    l1->setText(QString::number(nQuantity));                                     // Quantity
    l2->setText(BitcoinUnits::formatWithUnit(nDisplayUnit, nAmount));            // Amount
    l3->setText(BitcoinUnits::formatWithUnit(nDisplayUnit, nPayFee, false, 3));  // Fee
    l4->setText(BitcoinUnits::formatWithUnit(nDisplayUnit, nAfterFee));          // After Fee
    l5->setText(((nBytes > 0) ? ASYMP_UTF8 : "") + QString::number(nBytes));     // Bytes
    l6->setText(sPriorityLabel);                                                 // Priority
    l7->setText((fLowOutput ? (fDust ? tr("DUST") : tr("yes")) : tr("no")));     // Low Output / Dust
    l8->setText(BitcoinUnits::formatWithUnit(nDisplayUnit, nChange));            // Change

    // turn labels "red"
    l5->setStyleSheet((nBytes >= 1000) ? "color:red;" : "");                 // Bytes >= 1000
    l6->setStyleSheet((dPriority <= 576000) ? "color:red;" : "");            // Priority < "medium"
    l7->setStyleSheet((fLowOutput) ? "color:red;" : "");                     // Low Output = "yes"
    l8->setStyleSheet((nChange > 0 && nChange < CENT) ? "color:red;" : "");  // Change < 0.01BTC

    // tool tips
    l5->setToolTip(tr("This label turns red, if the transaction size is bigger than 1000 bytes.\n\n This means a fee of at least %1 per kb is required.\n\n Can vary +/- 1 Byte per input.").arg(BitcoinUnits::formatWithUnit(nDisplayUnit, MIN_TX_FEE)));
    l6->setToolTip(tr("Transactions with higher priority get more likely into a block.\n\nThis label turns red, if the priority is smaller than \"medium\".\n\n This means a fee of at least %1 per kb is required.").arg(BitcoinUnits::formatWithUnit(nDisplayUnit, MIN_TX_FEE)));
    l7->setToolTip(tr("This label turns red, if any recipient receives an amount smaller than %1.\n\n This means a fee of at least %2 is required. \n\n Amounts below the minimum fee are shown as DUST.").arg(BitcoinUnits::formatWithUnit(nDisplayUnit, CENT)).arg(BitcoinUnits::formatWithUnit(nDisplayUnit, MIN_RELAY_TX_FEE)));
    l8->setToolTip(tr("This label turns red, if the change is smaller than %1.\n\n This means a fee of at least %2 is required.").arg(BitcoinUnits::formatWithUnit(nDisplayUnit, CENT)).arg(BitcoinUnits::formatWithUnit(nDisplayUnit, MIN_TX_FEE)));
    dialog->findChild<QLabel *>("labelCoinControlBytesText")    ->setToolTip(l5->toolTip());
    dialog->findChild<QLabel *>("labelCoinControlPriorityText") ->setToolTip(l6->toolTip());
    dialog->findChild<QLabel *>("labelCoinControlLowOutputText")->setToolTip(l7->toolTip());
    dialog->findChild<QLabel *>("labelCoinControlChangeText")   ->setToolTip(l8->toolTip());

    // Insufficient funds
    QLabel *label = dialog->findChild<QLabel *>("labelCoinControlInsuffFunds");
    if (label)
        label->setVisible(nChange < 0);
}

void CoinControlDialog::updateView()
{
    bool treeMode = ui->radioTreeMode->isChecked();

    ui->treeWidget->clear();
    ui->treeWidget->setEnabled(false); // performance, otherwise updateLabels would be called for every checked checkbox
    ui->treeWidget->setAlternatingRowColors(!treeMode);
    QFlags<Qt::ItemFlag> flgCheckbox=Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable;
    QFlags<Qt::ItemFlag> flgTristate=Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsTristate;    

    int nDisplayUnit = BitcoinUnits::BTC;
    if (model && model->getOptionsModel())
        nDisplayUnit = model->getOptionsModel()->getDisplayUnit();

    map<QString, vector<COutput> > mapCoins;
    model->listCoins(mapCoins);

    BOOST_FOREACH(PAIRTYPE(QString, vector<COutput>) coins, mapCoins)
    {
        QTreeWidgetItem *itemWalletAddress = new QTreeWidgetItem();
        QString sWalletAddress = coins.first;
        QString sWalletLabel = "";
        if (model->getAddressTableModel())
            sWalletLabel = model->getAddressTableModel()->labelForAddress(sWalletAddress);
        if (sWalletLabel.length() == 0)
            sWalletLabel = tr("(no label)");

        if (treeMode)
        {
            // wallet address
            ui->treeWidget->addTopLevelItem(itemWalletAddress);

            itemWalletAddress->setFlags(flgTristate);
            itemWalletAddress->setCheckState(COLUMN_CHECKBOX,Qt::Unchecked);

            for (int i = 0; i < ui->treeWidget->columnCount(); i++)
                itemWalletAddress->setBackground(i, QColor(248, 247, 246));

            // label
            itemWalletAddress->setText(COLUMN_LABEL, sWalletLabel);

            // address
            itemWalletAddress->setText(COLUMN_ADDRESS, sWalletAddress);
        }

        int64_t nSum = 0;
        double dPrioritySum = 0;
        int nChildren = 0;
        int nInputSum = 0;
        uint64_t nTxWeight = 0, nTxWeightSum = 0;
        BOOST_FOREACH(const COutput& out, coins.second)
        {
            int nInputSize = 148; // 180 if uncompressed public key
            nSum += out.tx->vout[out.i].nValue;
            model->getStakeWeightFromValue(out.tx->GetTxTime(), out.tx->vout[out.i].nValue, nTxWeight);
            nTxWeightSum += nTxWeight;
            nChildren++;

            QTreeWidgetItem *itemOutput;
            if (treeMode)    itemOutput = new QTreeWidgetItem(itemWalletAddress);
            else             itemOutput = new QTreeWidgetItem(ui->treeWidget);
            itemOutput->setFlags(flgCheckbox);
            itemOutput->setCheckState(COLUMN_CHECKBOX,Qt::Unchecked);

            // address
            CBitcoinAddress outputAddress;
            QString sAddress = "";

            if(ExtractAddress(*pwalletMain, out.tx->vout[out.i].scriptPubKey, outputAddress))
            {
                sAddress = CBitcoinAddress(outputAddress).ToString().c_str();

                // if listMode or change => show bitcoin address. In tree mode, address is not shown again for direct wallet address outputs
                if (!treeMode || (!(sAddress == sWalletAddress)))
                    itemOutput->setText(COLUMN_ADDRESS, sAddress);

                if (outputAddress.IsPubKey())
                {
                    CPubKey pubkey;
                    CKeyID keyid;
                    if (outputAddress.GetKeyID(keyid) && model->getPubKey(keyid, pubkey) && !pubkey.IsCompressed())
                        nInputSize = 180;
                }
            }

            // label
            if (!(sAddress == sWalletAddress)) // change
            {
                // tooltip from where the change comes from
                itemOutput->setToolTip(COLUMN_LABEL, tr("change from %1 (%2)").arg(sWalletLabel).arg(sWalletAddress));
                itemOutput->setText(COLUMN_LABEL, tr("(change)"));
            }
            else if (!treeMode)
            {
                QString sLabel = "";
                if (model->getAddressTableModel())
                    sLabel = model->getAddressTableModel()->labelForAddress(sAddress);
                if (sLabel.length() == 0)
                    sLabel = tr("(no label)");
                itemOutput->setText(COLUMN_LABEL, sLabel); 
            }

            // amount
            itemOutput->setText(COLUMN_AMOUNT, BitcoinUnits::format(nDisplayUnit, out.tx->vout[out.i].nValue));
            itemOutput->setText(COLUMN_AMOUNT_INT64, strPad(QString::number(out.tx->vout[out.i].nValue), 15, " ")); // padding so that sorting works correctly

            // date
            itemOutput->setText(COLUMN_DATE, QDateTime::fromTime_t(out.tx->GetTxTime()).toUTC().toString("yy-MM-dd hh:mm"));

            // immature PoS reward
            if (out.tx->IsCoinStake() && out.tx->GetBlocksToMaturity() > 0 && out.tx->GetDepthInMainChain() > 0) {
              itemOutput->setBackground(COLUMN_CONFIRMATIONS, Qt::red);
              itemOutput->setDisabled(true);
            }

            // confirmations
            itemOutput->setText(COLUMN_CONFIRMATIONS, strPad(QString::number(out.nDepth), 8, " "));

            // priority
            double dPriority = ((double)out.tx->vout[out.i].nValue  / (nInputSize + 78)) * (out.nDepth+1); // 78 = 2 * 34 + 10
            itemOutput->setText(COLUMN_PRIORITY, CoinControlDialog::getPriorityLabel(dPriority));
            itemOutput->setText(COLUMN_PRIORITY_INT64, strPad(QString::number((int64_t)dPriority), 20, " "));
            dPrioritySum += (double)out.tx->vout[out.i].nValue  * (out.nDepth+1);
            nInputSum    += nInputSize;

            // List Mode Weight
            itemOutput->setText(COLUMN_WEIGHT, strPad(QString::number(nTxWeight), 8, " "));

            // transaction hash
            uint256 txhash = out.tx->GetHash();
            itemOutput->setText(COLUMN_TXHASH, txhash.GetHex().c_str());

            // vout index
            itemOutput->setText(COLUMN_VOUT_INDEX, QString::number(out.i));

            // disable locked coins
            /*if (model->isLockedCoin(txhash, out.i))
            {
                COutPoint outpt(txhash, out.i);
                coinControl->UnSelect(outpt); // just to be sure
                itemOutput->setDisabled(true);
                itemOutput->setIcon(COLUMN_CHECKBOX, QIcon(":/icons/lock_closed"));
            }*/

            // set checkbox
            if (coinControl->IsSelected(txhash, out.i))
                itemOutput->setCheckState(COLUMN_CHECKBOX,Qt::Checked);
        }

        // amount
        if (treeMode)
        {
            dPrioritySum = dPrioritySum / (nInputSum + 78);
            itemWalletAddress->setText(COLUMN_CHECKBOX, "(" + QString::number(nChildren) + ")");
            itemWalletAddress->setText(COLUMN_AMOUNT, BitcoinUnits::format(nDisplayUnit, nSum));
            itemWalletAddress->setText(COLUMN_AMOUNT_INT64, strPad(QString::number(nSum), 15, " "));
            itemWalletAddress->setText(COLUMN_PRIORITY, CoinControlDialog::getPriorityLabel(dPrioritySum));
            itemWalletAddress->setText(COLUMN_PRIORITY_INT64, strPad(QString::number((int64_t)dPrioritySum), 20, " "));
            itemWalletAddress->setText(COLUMN_WEIGHT, strPad(QString::number((uint64_t)nTxWeightSum),8," "));

        }
    }

    // expand all partially selected
    if (treeMode)
    {
        for (int i = 0; i < ui->treeWidget->topLevelItemCount(); i++)
            if (ui->treeWidget->topLevelItem(i)->checkState(COLUMN_CHECKBOX) == Qt::PartiallyChecked)
                ui->treeWidget->topLevelItem(i)->setExpanded(true);
    }

    // sort view
    sortView(sortColumn, sortOrder);
    ui->treeWidget->setEnabled(true);
}

void CoinControlDialog::keyPressEvent(QKeyEvent *event)
{
#ifdef ANDROID
    if(event->key() == Qt::Key_Back)
    {
        close();
    }
#else
    if(event->key() == Qt::Key_Escape)
    {
        close();
    }
#endif
}

void CoinControlDialog::closeEvent(QCloseEvent* e)
{
    QWidget::closeEvent(e);
    emit beforeClose();
}
