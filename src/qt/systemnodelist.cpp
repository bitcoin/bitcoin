#include "systemnodelist.h"
#include "ui_systemnodelist.h"

#include "sync.h"
#include "clientmodel.h"
#include "walletmodel.h"
#include "activesystemnode.h"
#include "systemnode-sync.h"
#include "systemnodeconfig.h"
#include "systemnodeman.h"
#include "wallet.h"
#include "init.h"
#include "guiutil.h"
#include "datetablewidgetitem.h"
#include "privatekeywidget.h"
#include "createsystemnodedialog.h"
#include "addresstablemodel.h"
#include "transactiontablemodel.h"
#include "optionsmodel.h"
#include "startmissingdialog.h"

#include <QTimer>
#include <QMessageBox>
#include <QCheckBox>

CCriticalSection cs_systemnodes;

SystemnodeList::SystemnodeList(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SystemnodeList),
    clientModel(0),
    walletModel(0)
{
    ui->setupUi(this);

    ui->startButton->setEnabled(false);

    int columnAliasWidth = 100;
    int columnAddressWidth = 200;
    int columnProtocolWidth = 60;
    int columnStatusWidth = 80;
    int columnActiveWidth = 130;
    int columnLastSeenWidth = 130;

    ui->tableWidgetMySystemnodes->setColumnWidth(0, columnAliasWidth);
    ui->tableWidgetMySystemnodes->setColumnWidth(1, columnAddressWidth);
    ui->tableWidgetMySystemnodes->setColumnWidth(2, columnProtocolWidth);
    ui->tableWidgetMySystemnodes->setColumnWidth(3, columnStatusWidth);
    ui->tableWidgetMySystemnodes->setColumnWidth(4, columnActiveWidth);
    ui->tableWidgetMySystemnodes->setColumnWidth(5, columnLastSeenWidth);

    ui->tableWidgetSystemnodes->setColumnWidth(0, columnAddressWidth);
    ui->tableWidgetSystemnodes->setColumnWidth(1, columnProtocolWidth);
    ui->tableWidgetSystemnodes->setColumnWidth(2, columnStatusWidth);
    ui->tableWidgetSystemnodes->setColumnWidth(3, columnActiveWidth);
    ui->tableWidgetSystemnodes->setColumnWidth(4, columnLastSeenWidth);

    ui->tableWidgetMySystemnodes->setContextMenuPolicy(Qt::CustomContextMenu);

    QAction *startAliasAction = new QAction(tr("Start alias"), this);
    QAction *editAction = new QAction(tr("Edit"), this);
    contextMenu = new QMenu();
    contextMenu->addAction(startAliasAction);
    contextMenu->addAction(editAction);
    connect(ui->tableWidgetMySystemnodes, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(showContextMenu(const QPoint&)));
    connect(startAliasAction, SIGNAL(triggered()), this, SLOT(on_startButton_clicked()));
    connect(editAction, SIGNAL(triggered()), this, SLOT(on_editButton_clicked()));

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(updateNodeList()));
    connect(timer, SIGNAL(timeout()), this, SLOT(updateMyNodeList()));
    timer->start(1000);

    updateNodeList();

    sendDialog = new SendCollateralDialog(SendCollateralDialog::SYSTEMNODE, this);
}

SystemnodeList::~SystemnodeList()
{
    delete ui;
}

void SystemnodeList::setClientModel(ClientModel *model)
{
    if (this->clientModel == NULL)
    {
        this->clientModel = model;
        if(model)
        {
            // try to update list when systemnode count changes
            connect(clientModel, SIGNAL(strSystemnodesChanged(QString)), this, SLOT(updateNodeList()));
        }
    }
}

void SystemnodeList::setWalletModel(WalletModel *model)
{
    if (this->walletModel == NULL)
    {
        this->walletModel = model;
    }
}

void SystemnodeList::showContextMenu(const QPoint &point)
{
    QTableWidgetItem *item = ui->tableWidgetMySystemnodes->itemAt(point);
    if(item) contextMenu->exec(QCursor::pos());
}

void SystemnodeList::StartAlias(std::string strAlias)
{
    std::string statusObj;
    statusObj += "<center>Alias: " + strAlias;

    BOOST_FOREACH(CNodeEntry mne, systemnodeConfig.getEntries()) {
        if(mne.getAlias() == strAlias) {
            std::string errorMessage;
            CSystemnodeBroadcast mnb;

            bool result = CSystemnodeBroadcast::Create(mne.getIp(), mne.getPrivKey(), mne.getTxHash(), mne.getOutputIndex(), errorMessage, mnb);

            if(result) {
                statusObj += "<br>Successfully started systemnode." ;
                snodeman.UpdateSystemnodeList(mnb);
                mnb.Relay();
            } else {
                statusObj += "<br>Failed to start systemnode.<br>Error: " + errorMessage;
            }
            break;
        }
    }
    statusObj += "</center>";

    QMessageBox msg;
    msg.setText(QString::fromStdString(statusObj));

    msg.exec();
}

void SystemnodeList::StartAll(std::string strCommand)
{
    int total = 0;
    int successful = 0;
    int fail = 0;
    std::string statusObj;

    BOOST_FOREACH(CNodeEntry mne, systemnodeConfig.getEntries()) {
        std::string errorMessage;
        CSystemnodeBroadcast mnb;

        CTxIn vin = CTxIn(uint256S(mne.getTxHash()), uint32_t(atoi(mne.getOutputIndex().c_str())));
        CSystemnode *pmn = snodeman.Find(vin);

        if(strCommand == "start-missing" && pmn) continue;

        bool result = CSystemnodeBroadcast::Create(mne.getIp(), mne.getPrivKey(), mne.getTxHash(), mne.getOutputIndex(), errorMessage, mnb);

        if(result) {
            successful++;
            snodeman.UpdateSystemnodeList(mnb);
            mnb.Relay();
        } else {
            fail++;
            statusObj += "\nFailed to start " + mne.getAlias() + ". Error: " + errorMessage;
        }
        total++;
    }
    pwalletMain->Lock();

    std::string returnObj;
    returnObj = strprintf("Successfully started %d systemnodes, failed to start %d, total %d", successful, fail, total);
    if (fail > 0)
        returnObj += statusObj;

    QMessageBox msg;
    msg.setText(QString::fromStdString(returnObj));
    msg.exec();
}

void SystemnodeList::selectAliasRow(const QString& alias)
{
    for(int i=0; i < ui->tableWidgetMySystemnodes->rowCount(); i++)
    {
        if(ui->tableWidgetMySystemnodes->item(i, 0)->text() == alias)
        {
            ui->tableWidgetMySystemnodes->selectRow(i);
            ui->tableWidgetMySystemnodes->setFocus();
            ui->tabWidget->setCurrentIndex(0);
            return;
        }
    }
}

void SystemnodeList::updateMySystemnodeInfo(QString alias, QString addr, QString privkey, QString txHash, QString txIndex, CSystemnode *pmn)
{
    LOCK(cs_mnlistupdate);
    bool bFound = false;
    int nodeRow = 0;

    for(int i=0; i < ui->tableWidgetMySystemnodes->rowCount(); i++)
    {
        if(ui->tableWidgetMySystemnodes->item(i, 0)->text() == alias)
        {
            bFound = true;
            nodeRow = i;
            break;
        }
    }

    if(nodeRow == 0 && !bFound) {
        nodeRow = ui->tableWidgetMySystemnodes->rowCount();
        ui->tableWidgetMySystemnodes->insertRow(nodeRow);
    }

    QTableWidgetItem *aliasItem = new QTableWidgetItem(alias);
    QTableWidgetItem *addrItem = new QTableWidgetItem(addr);
    PrivateKeyWidget *privateKeyWidget = new PrivateKeyWidget(privkey);
    QTableWidgetItem *protocolItem = new QTableWidgetItem(QString::number(pmn ? pmn->protocolVersion : -1));
    QTableWidgetItem *statusItem = new QTableWidgetItem(QString::fromStdString(pmn ? pmn->Status() : "MISSING"));
    DateTableWidgetItem *activeSecondsItem = new DateTableWidgetItem(pmn ? (pmn->lastPing.sigTime - pmn->sigTime) : 0);
    QTableWidgetItem *lastSeenItem = new QTableWidgetItem(QString::fromStdString(DateTimeStrFormat("%Y-%m-%d %H:%M", pmn ? pmn->lastPing.sigTime : 0)));
    QTableWidgetItem *pubkeyItem = new QTableWidgetItem(QString::fromStdString(pmn ? CBitcoinAddress(pmn->pubkey.GetID()).ToString() : ""));

    ui->tableWidgetMySystemnodes->setItem(nodeRow, 0, aliasItem);
    ui->tableWidgetMySystemnodes->setItem(nodeRow, 1, addrItem);
    ui->tableWidgetMySystemnodes->setCellWidget(nodeRow, 2, privateKeyWidget);
    ui->tableWidgetMySystemnodes->setColumnWidth(2, 150);
    ui->tableWidgetMySystemnodes->setItem(nodeRow, 3, protocolItem);
    ui->tableWidgetMySystemnodes->setItem(nodeRow, 4, statusItem);
    ui->tableWidgetMySystemnodes->setItem(nodeRow, 5, activeSecondsItem);
    ui->tableWidgetMySystemnodes->setItem(nodeRow, 6, lastSeenItem);
    ui->tableWidgetMySystemnodes->setItem(nodeRow, 7, pubkeyItem);
}

void SystemnodeList::updateMyNodeList(bool reset) {
    static int64_t lastMyListUpdate = 0;

    // automatically update my systemnode list only once in MY_SYSTEMNODELIST_UPDATE_SECONDS seconds,
    // this update still can be triggered manually at any time via button click
    int64_t timeTillUpdate = lastMyListUpdate + MY_SYSTEMNODELIST_UPDATE_SECONDS - GetTime();
    ui->secondsLabel->setText(QString::number(timeTillUpdate));

    if(timeTillUpdate > 0 && !reset) return;
    lastMyListUpdate = GetTime();

    ui->tableWidgetSystemnodes->setSortingEnabled(false);
    BOOST_FOREACH(CNodeEntry mne, systemnodeConfig.getEntries()) {
        CTxIn vin = CTxIn(uint256S(mne.getTxHash()), uint32_t(atoi(mne.getOutputIndex().c_str())));
        CSystemnode *pmn = snodeman.Find(vin);

        updateMySystemnodeInfo(QString::fromStdString(mne.getAlias()), QString::fromStdString(mne.getIp()), QString::fromStdString(mne.getPrivKey()), QString::fromStdString(mne.getTxHash()),
            QString::fromStdString(mne.getOutputIndex()), pmn);
    }
    ui->tableWidgetSystemnodes->setSortingEnabled(true);

    // reset "timer"
    ui->secondsLabel->setText("0");
}

void SystemnodeList::updateNodeList()
{
    static int64_t nTimeListUpdate = 0;

      // to prevent high cpu usage update only once in SYSTEMNODELIST_UPDATE_SECONDS secondsLabel
      // or SYSTEMNODELIST_FILTER_COOLDOWN_SECONDS seconds after filter was last changed
      int64_t nTimeToWait =   fFilterUpdated
                              ? nTimeFilterUpdate - GetTime() + SYSTEMNODELIST_FILTER_COOLDOWN_SECONDS
                              : nTimeListUpdate - GetTime() + SYSTEMNODELIST_UPDATE_SECONDS;

    if(fFilterUpdated) ui->countLabel->setText(QString::fromStdString(strprintf("Please wait... %d", nTimeToWait)));
    if(nTimeToWait > 0) return;

    nTimeListUpdate = GetTime();
    fFilterUpdated = false;

    TRY_LOCK(cs_systemnodes, lockSystemnodes);
    if(!lockSystemnodes)
        return;

    QString strToFilter;
    ui->countLabel->setText("Updating...");
    ui->tableWidgetSystemnodes->setSortingEnabled(false);
    ui->tableWidgetSystemnodes->clearContents();
    ui->tableWidgetSystemnodes->setRowCount(0);
    std::vector<CSystemnode> vSystemnodes = snodeman.GetFullSystemnodeVector();

    BOOST_FOREACH(CSystemnode& mn, vSystemnodes)
    {
        // populate list
        // Address, Protocol, Status, Active Seconds, Last Seen, Pub Key
        QTableWidgetItem *addressItem = new QTableWidgetItem(QString::fromStdString(mn.addr.ToString()));
        QTableWidgetItem *protocolItem = new QTableWidgetItem(QString::number(mn.protocolVersion));
        QTableWidgetItem *statusItem = new QTableWidgetItem(QString::fromStdString(mn.Status()));
        DateTableWidgetItem *activeSecondsItem = new DateTableWidgetItem(mn.lastPing.sigTime - mn.sigTime);
        QTableWidgetItem *lastSeenItem = new QTableWidgetItem(QString::fromStdString(DateTimeStrFormat("%Y-%m-%d %H:%M", mn.lastPing.sigTime)));
        QTableWidgetItem *pubkeyItem = new QTableWidgetItem(QString::fromStdString(CBitcoinAddress(mn.pubkey.GetID()).ToString()));

        if (strCurrentFilter != "")
        {
            strToFilter =   addressItem->text() + " " +
                            protocolItem->text() + " " +
                            statusItem->text() + " " +
                            activeSecondsItem->text() + " " +
                            lastSeenItem->text() + " " +
                            pubkeyItem->text();
            if (!strToFilter.contains(strCurrentFilter)) continue;
        }

        ui->tableWidgetSystemnodes->insertRow(0);
        ui->tableWidgetSystemnodes->setItem(0, 0, addressItem);
        ui->tableWidgetSystemnodes->setItem(0, 1, protocolItem);
        ui->tableWidgetSystemnodes->setItem(0, 2, statusItem);
        ui->tableWidgetSystemnodes->setItem(0, 3, activeSecondsItem);
        ui->tableWidgetSystemnodes->setItem(0, 4, lastSeenItem);
        ui->tableWidgetSystemnodes->setItem(0, 5, pubkeyItem);
    }

    ui->countLabel->setText(QString::number(ui->tableWidgetSystemnodes->rowCount()));
    ui->tableWidgetSystemnodes->setSortingEnabled(true);

}

void SystemnodeList::on_filterLineEdit_textChanged(const QString &filterString) {
    strCurrentFilter = filterString;
    nTimeFilterUpdate = GetTime();
    fFilterUpdated = true;
    ui->countLabel->setText(QString::fromStdString(strprintf("Please wait... %d", SYSTEMNODELIST_FILTER_COOLDOWN_SECONDS)));
}

void SystemnodeList::on_startButton_clicked()
{
    // Find selected node alias
    QItemSelectionModel* selectionModel = ui->tableWidgetMySystemnodes->selectionModel();
    QModelIndexList selected = selectionModel->selectedRows();
    if(selected.count() == 0)
        return;

    QModelIndex index = selected.at(0);
    int r = index.row();
    std::string strAlias = ui->tableWidgetMySystemnodes->item(r, 0)->text().toStdString();

    // Display message box
    QMessageBox::StandardButton retval = QMessageBox::question(this, tr("Confirm systemnode start"),
        tr("Are you sure you want to start systemnode %1?").arg(QString::fromStdString(strAlias)),
        QMessageBox::Yes | QMessageBox::Cancel,
        QMessageBox::Cancel);

    if(retval != QMessageBox::Yes)
    {
        return;
    }

    WalletModel::EncryptionStatus encStatus = walletModel->getEncryptionStatus();
    if(encStatus == walletModel->Locked)
    {
        WalletModel::UnlockContext ctx(walletModel->requestUnlock(true));
        if(!ctx.isValid())
        {
            // Unlock wallet was cancelled
            return;
        }
        StartAlias(strAlias);
        return;
    }

    StartAlias(strAlias);
}

void SystemnodeList::on_editButton_clicked()
{
    CreateSystemnodeDialog dialog(this);
    dialog.setWindowModality(Qt::ApplicationModal);
    dialog.setEditMode();
    dialog.setWindowTitle("Edit Systemnode");
    
    QItemSelectionModel* selectionModel = ui->tableWidgetMySystemnodes->selectionModel();
    QModelIndexList selected = selectionModel->selectedRows();
    if(selected.count() == 0)
        return;

    QModelIndex index = selected.at(0);
    int r = index.row();
    QString strAlias = ui->tableWidgetMySystemnodes->item(r, 0)->text();
    QString strIP = ui->tableWidgetMySystemnodes->item(r, 1)->text();
    strIP.replace(QRegExp(":+\\d*"), "");

    dialog.setAlias(strAlias);
    dialog.setIP(strIP);
    if (dialog.exec())
    {
        // OK pressed
        std::string port = "9340";
        if (Params().NetworkID() == CBaseChainParams::TESTNET) {
            port = "19340";
        }
        BOOST_FOREACH(CNodeEntry &sne, systemnodeConfig.getEntries()) {
            if (sne.getAlias() == strAlias.toStdString())
            {
                sne.setAlias(dialog.getAlias().toStdString());
                sne.setIp(dialog.getIP().toStdString() + ":" + port);
                systemnodeConfig.write();
                ui->tableWidgetMySystemnodes->removeRow(r);
                updateMyNodeList(true);
            }
        }

    }
    else
    {
        // Cancel pressed
    }
}

void SystemnodeList::on_startAllButton_clicked()
{
    // Display message box
    QMessageBox::StandardButton retval = QMessageBox::question(this, tr("Confirm all systemnodes start"),
        tr("Are you sure you want to start ALL systemnodes?") + 
            QString("<br>") + tr("<b>") + tr("Warning!") + 
            tr("</b>") + tr(" This action will reset all active systemnodes."),
        QMessageBox::Yes | QMessageBox::Cancel,
        QMessageBox::Cancel);

    if(retval != QMessageBox::Yes)
    {
        return;
    }

    WalletModel::EncryptionStatus encStatus = walletModel->getEncryptionStatus();
    if(encStatus == walletModel->Locked)
    {
        WalletModel::UnlockContext ctx(walletModel->requestUnlock(true));
        if(!ctx.isValid())
        {
            // Unlock wallet was cancelled
            return;
        }
        StartAll();
        return;
    }

    StartAll();
}

void SystemnodeList::on_startMissingButton_clicked()
{
    if(systemnodeSync.RequestedSystemnodeAssets <= SYSTEMNODE_SYNC_LIST ||
      systemnodeSync.RequestedSystemnodeAssets == SYSTEMNODE_SYNC_FAILED) {
        QMessageBox::critical(this, tr("Command is not available right now"),
            tr("You can't use this command until systemnode list is synced"));
        return;
    }

    StartMissingDialog dg(this);
    dg.setWindowTitle("Confirm missing systemnodes start");
    dg.setText("Are you sure you want to start MISSING systemnodes?");
    dg.setCheckboxText("Start all nodes");
    dg.setWarningText(QString("<b>") + tr("WARNING!") + QString("</b>") +
            " If checked all ACTIVE systemnodes will be reset.");
    bool startAll = false;

    // Display message box
    if (dg.exec()) {
        if (dg.checkboxChecked()) {
            startAll = true;
        }

        WalletModel::EncryptionStatus encStatus = walletModel->getEncryptionStatus();
        if(encStatus == walletModel->Locked)
        {
            WalletModel::UnlockContext ctx(walletModel->requestUnlock(true));
            if(!ctx.isValid())
            {
                // Unlock wallet was cancelled
                return;
            }
            startAll ? StartAll() : StartAll("start-missing");
            return;
        }
        startAll ? StartAll() : StartAll("start-missing");
    }
}

void SystemnodeList::on_tableWidgetMySystemnodes_itemSelectionChanged()
{
    if(ui->tableWidgetMySystemnodes->selectedItems().count() > 0)
    {
        ui->startButton->setEnabled(true);
    }
}

void SystemnodeList::on_UpdateButton_clicked()
{
    updateMyNodeList(true);
}

void SystemnodeList::on_CreateNewSystemnode_clicked()
{
    CreateSystemnodeDialog dialog(this);
    dialog.setWindowModality(Qt::ApplicationModal);
    QString formattedAmount = BitcoinUnits::formatWithUnit(walletModel->getOptionsModel()->getDisplayUnit(), 
                                                           SYSTEMNODE_COLLATERAL * COIN);
    dialog.setNoteLabel("This action will send " + formattedAmount + " to yourself");
    if (dialog.exec())
    {
        // OK Pressed
        QString label = dialog.getLabel();
        QString address = walletModel->getAddressTableModel()->addRow(AddressTableModel::Receive, label, "");
        SendCoinsRecipient recipient(address, label, SYSTEMNODE_COLLATERAL * COIN, "");
        QList<SendCoinsRecipient> recipients;
        recipients.append(recipient);

        // Get outputs before and after transaction
        std::vector<COutput> vPossibleCoinsBefore;
        pwalletMain->AvailableCoins(vPossibleCoinsBefore, true, NULL, ONLY_500);

        sendDialog->setModel(walletModel);
        sendDialog->send(recipients);

        std::vector<COutput> vPossibleCoinsAfter;
        pwalletMain->AvailableCoins(vPossibleCoinsAfter, true, NULL, ONLY_500);

        BOOST_FOREACH(COutput& out, vPossibleCoinsAfter) {
            std::vector<COutput>::iterator it = std::find(vPossibleCoinsBefore.begin(), vPossibleCoinsBefore.end(), out);
            if (it == vPossibleCoinsBefore.end()) {
                // Not found so this is a new element

                COutPoint outpoint = COutPoint(out.tx->GetHash(), boost::lexical_cast<unsigned int>(out.i));
                pwalletMain->LockCoin(outpoint);

                // Generate a key
                CKey secret;
                secret.MakeNewKey(false);
                std::string privateKey = CBitcoinSecret(secret).ToString();
                std::string port = "9340";
                if (Params().NetworkID() == CBaseChainParams::TESTNET) {
                    port = "19340";
                }

                systemnodeConfig.add(dialog.getAlias().toStdString(), dialog.getIP().toStdString() + ":" + port, 
                        privateKey, out.tx->GetHash().ToString(), strprintf("%d", out.i));
                systemnodeConfig.write();
                updateMyNodeList(true);
            }
        }
    } else {
        // Cancel Pressed
    }
}
