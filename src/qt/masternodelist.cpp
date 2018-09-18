#include "masternodelist.h"
#include "ui_masternodelist.h"

#include "sync.h"
#include "clientmodel.h"
#include "walletmodel.h"
#include "activemasternode.h"
#include "masternode-budget.h"
#include "masternode-sync.h"
#include "masternodeconfig.h"
#include "masternodeman.h"
#include "wallet.h"
#include "init.h"
#include "guiutil.h"
#include "datetablewidgetitem.h"
#include "privatekeywidget.h"
#include "createmasternodedialog.h"
#include "addresstablemodel.h"
#include "transactiontablemodel.h"
#include "optionsmodel.h"
#include "startmissingdialog.h"

#include <QTimer>
#include <QMessageBox>

CCriticalSection cs_masternodes;

MasternodeList::MasternodeList(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MasternodeList),
    clientModel(0),
    walletModel(0)
{
    ui->setupUi(this);

    ui->startButton->setEnabled(false);
    ui->voteManyYesButton->setEnabled(false);
    ui->voteManyNoButton->setEnabled(false);

    int columnAliasWidth = 100;
    int columnAddressWidth = 200;
    int columnProtocolWidth = 60;
    int columnStatusWidth = 80;
    int columnActiveWidth = 130;
    int columnLastSeenWidth = 130;

    ui->tableWidgetMyMasternodes->setColumnWidth(0, columnAliasWidth);
    ui->tableWidgetMyMasternodes->setColumnWidth(1, columnAddressWidth);
    ui->tableWidgetMyMasternodes->setColumnWidth(2, columnProtocolWidth);
    ui->tableWidgetMyMasternodes->setColumnWidth(3, columnStatusWidth);
    ui->tableWidgetMyMasternodes->setColumnWidth(4, columnActiveWidth);
    ui->tableWidgetMyMasternodes->setColumnWidth(5, columnLastSeenWidth);

    ui->tableWidgetMasternodes->setColumnWidth(0, columnAddressWidth);
    ui->tableWidgetMasternodes->setColumnWidth(1, columnProtocolWidth);
    ui->tableWidgetMasternodes->setColumnWidth(2, columnStatusWidth);
    ui->tableWidgetMasternodes->setColumnWidth(3, columnActiveWidth);
    ui->tableWidgetMasternodes->setColumnWidth(4, columnLastSeenWidth);

    ui->tableWidgetMyMasternodes->setContextMenuPolicy(Qt::CustomContextMenu);

    QAction *startAliasAction = new QAction(tr("Start alias"), this);
    QAction *editAction = new QAction(tr("Edit"), this);
    contextMenu = new QMenu();
    contextMenu->addAction(startAliasAction);
    contextMenu->addAction(editAction);
    connect(ui->tableWidgetMyMasternodes, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(showContextMenu(const QPoint&)));
    connect(startAliasAction, SIGNAL(triggered()), this, SLOT(on_startButton_clicked()));
    connect(editAction, SIGNAL(triggered()), this, SLOT(on_editButton_clicked()));

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(updateNodeList()));
    connect(timer, SIGNAL(timeout()), this, SLOT(updateVoteList()));
    connect(timer, SIGNAL(timeout()), this, SLOT(updateMyNodeList()));
    connect(timer, SIGNAL(timeout()), this, SLOT(updateNextSuperblock()));
    timer->start(1000);

    updateNodeList();
    updateVoteList();
    updateNextSuperblock();

    sendDialog = new SendCollateralDialog(SendCollateralDialog::MASTERNODE, this);
}

MasternodeList::~MasternodeList()
{
    delete ui;
}

void MasternodeList::setClientModel(ClientModel *model)
{
    if (this->clientModel == NULL)
    {
        this->clientModel = model;
        if(model)
        {
            // try to update list when masternode count changes
            connect(clientModel, SIGNAL(strMasternodesChanged(QString)), this, SLOT(updateNodeList()));
        }
    }
}

void MasternodeList::setWalletModel(WalletModel *model)
{
    if (this->walletModel == NULL)
    {
        this->walletModel = model;
    }
}

void MasternodeList::showContextMenu(const QPoint &point)
{
    QTableWidgetItem *item = ui->tableWidgetMyMasternodes->itemAt(point);
    if(item) contextMenu->exec(QCursor::pos());
}

void MasternodeList::StartAlias(std::string strAlias)
{
    std::string statusObj;
    statusObj += "<center>Alias: " + strAlias;

    BOOST_FOREACH(CNodeEntry mne, masternodeConfig.getEntries()) {
        if(mne.getAlias() == strAlias) {
            std::string errorMessage;
            CMasternodeBroadcast mnb;

            bool result = CMasternodeBroadcast::Create(mne.getIp(), mne.getPrivKey(), mne.getTxHash(), mne.getOutputIndex(), errorMessage, mnb);

            if(result) {
                statusObj += "<br>Successfully started masternode." ;
                mnodeman.UpdateMasternodeList(mnb);
                mnb.Relay();
            } else {
                statusObj += "<br>Failed to start masternode.<br>Error: " + errorMessage;
            }
            break;
        }
    }
    statusObj += "</center>";

    QMessageBox msg;
    msg.setText(QString::fromStdString(statusObj));

    msg.exec();
}

void MasternodeList::StartAll(std::string strCommand)
{
    int total = 0;
    int successful = 0;
    int fail = 0;
    std::string statusObj;

    BOOST_FOREACH(CNodeEntry mne, masternodeConfig.getEntries()) {
        std::string errorMessage;
        CMasternodeBroadcast mnb;

        CTxIn vin = CTxIn(uint256S(mne.getTxHash()), uint32_t(atoi(mne.getOutputIndex().c_str())));
        CMasternode *pmn = mnodeman.Find(vin);

        if(strCommand == "start-missing" && pmn) continue;

        bool result = CMasternodeBroadcast::Create(mne.getIp(), mne.getPrivKey(), mne.getTxHash(), mne.getOutputIndex(), errorMessage, mnb);

        if(result) {
            successful++;
            mnodeman.UpdateMasternodeList(mnb);
            mnb.Relay();
        } else {
            fail++;
            statusObj += "\nFailed to start " + mne.getAlias() + ". Error: " + errorMessage;
        }
        total++;
    }
    pwalletMain->Lock();

    std::string returnObj;
    returnObj = strprintf("Successfully started %d masternodes, failed to start %d, total %d", successful, fail, total);
    if (fail > 0)
        returnObj += statusObj;

    QMessageBox msg;
    msg.setText(QString::fromStdString(returnObj));
    msg.exec();
}

void MasternodeList::selectAliasRow(const QString& alias)
{
    for(int i=0; i < ui->tableWidgetMyMasternodes->rowCount(); ++i)
    {
        if(ui->tableWidgetMyMasternodes->item(i, 0)->text() == alias)
        {
            ui->tableWidgetMyMasternodes->selectRow(i);
            ui->tableWidgetMyMasternodes->setFocus();
            ui->tabWidget->setCurrentIndex(0);
            return;
        }
    }
}

void MasternodeList::updateMyMasternodeInfo(QString alias, QString addr, QString privkey, QString txHash, QString txIndex, CMasternode *pmn)
{
    LOCK(cs_mnlistupdate);
    bool bFound = false;
    int nodeRow = 0;

    for(int i=0; i < ui->tableWidgetMyMasternodes->rowCount(); i++)
    {
        if(ui->tableWidgetMyMasternodes->item(i, 0)->text() == alias)
        {
            bFound = true;
            nodeRow = i;
            break;
        }
    }

    if(nodeRow == 0 && !bFound) {
        nodeRow = ui->tableWidgetMyMasternodes->rowCount();
        ui->tableWidgetMyMasternodes->insertRow(nodeRow);
    }

    QTableWidgetItem *aliasItem = new QTableWidgetItem(alias);
    QTableWidgetItem *addrItem = new QTableWidgetItem(addr);
    PrivateKeyWidget *privateKeyWidget = new PrivateKeyWidget(privkey);
    QTableWidgetItem *protocolItem = new QTableWidgetItem(QString::number(pmn ? pmn->protocolVersion : -1));
    QTableWidgetItem *statusItem = new QTableWidgetItem(QString::fromStdString(pmn ? pmn->Status() : "MISSING"));
    DateTableWidgetItem *activeSecondsItem = new DateTableWidgetItem(pmn ? (pmn->lastPing.sigTime - pmn->sigTime) : 0);
    QTableWidgetItem *lastSeenItem = new QTableWidgetItem(QString::fromStdString(DateTimeStrFormat("%Y-%m-%d %H:%M", pmn ? pmn->lastPing.sigTime : 0)));
    QTableWidgetItem *pubkeyItem = new QTableWidgetItem(QString::fromStdString(pmn ? CBitcoinAddress(pmn->pubkey.GetID()).ToString() : ""));

    ui->tableWidgetMyMasternodes->setItem(nodeRow, 0, aliasItem);
    ui->tableWidgetMyMasternodes->setItem(nodeRow, 1, addrItem);
    ui->tableWidgetMyMasternodes->setCellWidget(nodeRow, 2, privateKeyWidget);
    ui->tableWidgetMyMasternodes->setColumnWidth(2, 150);
    ui->tableWidgetMyMasternodes->setItem(nodeRow, 3, protocolItem);
    ui->tableWidgetMyMasternodes->setItem(nodeRow, 4, statusItem);
    ui->tableWidgetMyMasternodes->setItem(nodeRow, 5, activeSecondsItem);
    ui->tableWidgetMyMasternodes->setItem(nodeRow, 6, lastSeenItem);
    ui->tableWidgetMyMasternodes->setItem(nodeRow, 7, pubkeyItem);
}

void MasternodeList::updateMyNodeList(bool reset) {
    static int64_t lastMyListUpdate = 0;

    // automatically update my masternode list only once in MY_MASTERNODELIST_UPDATE_SECONDS seconds,
    // this update still can be triggered manually at any time via button click
    int64_t timeTillUpdate = lastMyListUpdate + MY_MASTERNODELIST_UPDATE_SECONDS - GetTime();
    ui->secondsLabel->setText(QString::number(timeTillUpdate));

    if(timeTillUpdate > 0 && !reset) return;
    lastMyListUpdate = GetTime();

    ui->tableWidgetMasternodes->setSortingEnabled(false);
    BOOST_FOREACH(CNodeEntry mne, masternodeConfig.getEntries()) {
        CTxIn vin = CTxIn(uint256S(mne.getTxHash()), uint32_t(atoi(mne.getOutputIndex().c_str())));
        CMasternode *pmn = mnodeman.Find(vin);

        updateMyMasternodeInfo(QString::fromStdString(mne.getAlias()), QString::fromStdString(mne.getIp()), QString::fromStdString(mne.getPrivKey()), QString::fromStdString(mne.getTxHash()),
            QString::fromStdString(mne.getOutputIndex()), pmn);
    }
    ui->tableWidgetMasternodes->setSortingEnabled(true);

    // reset "timer"
    ui->secondsLabel->setText("0");
}

void MasternodeList::updateNodeList()
{
    static int64_t nTimeListUpdate = 0;

      // to prevent high cpu usage update only once in MASTERNODELIST_UPDATE_SECONDS secondsLabel
      // or MASTERNODELIST_FILTER_COOLDOWN_SECONDS seconds after filter was last changed
      int64_t nTimeToWait =   fFilterUpdated
                              ? nTimeFilterUpdate - GetTime() + MASTERNODELIST_FILTER_COOLDOWN_SECONDS
                              : nTimeListUpdate - GetTime() + MASTERNODELIST_UPDATE_SECONDS;

    if(fFilterUpdated) ui->countLabel->setText(QString::fromStdString(strprintf("Please wait... %d", nTimeToWait)));
    if(nTimeToWait > 0) return;

    nTimeListUpdate = GetTime();
    fFilterUpdated = false;

    TRY_LOCK(cs_masternodes, lockMasternodes);
    if(!lockMasternodes)
        return;

    QString strToFilter;
    ui->countLabel->setText("Updating...");
    ui->tableWidgetMasternodes->setSortingEnabled(false);
    ui->tableWidgetMasternodes->clearContents();
    ui->tableWidgetMasternodes->setRowCount(0);
    std::vector<CMasternode> vMasternodes = mnodeman.GetFullMasternodeVector();

    BOOST_FOREACH(CMasternode& mn, vMasternodes)
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

        ui->tableWidgetMasternodes->insertRow(0);
        ui->tableWidgetMasternodes->setItem(0, 0, addressItem);
        ui->tableWidgetMasternodes->setItem(0, 1, protocolItem);
        ui->tableWidgetMasternodes->setItem(0, 2, statusItem);
        ui->tableWidgetMasternodes->setItem(0, 3, activeSecondsItem);
        ui->tableWidgetMasternodes->setItem(0, 4, lastSeenItem);
        ui->tableWidgetMasternodes->setItem(0, 5, pubkeyItem);
    }

    ui->countLabel->setText(QString::number(ui->tableWidgetMasternodes->rowCount()));
    ui->tableWidgetMasternodes->setSortingEnabled(true);

}

void MasternodeList::updateNextSuperblock()
{
    Q_ASSERT(chainActive.Tip() != NULL);

    CBlockIndex* pindexPrev = chainActive.Tip();
    if (pindexPrev)
    {
        const int nextBlock = GetNextSuperblock(pindexPrev->nHeight);
        ui->superblockLabel->setText(QString::number(nextBlock));
    }
}

void MasternodeList::on_filterLineEdit_textChanged(const QString &filterString) {
    strCurrentFilter = filterString;
    nTimeFilterUpdate = GetTime();
    fFilterUpdated = true;
    ui->countLabel->setText(QString::fromStdString(strprintf("Please wait... %d", MASTERNODELIST_FILTER_COOLDOWN_SECONDS)));
}

void MasternodeList::on_startButton_clicked()
{
    // Find selected node alias
    QItemSelectionModel* selectionModel = ui->tableWidgetMyMasternodes->selectionModel();
    QModelIndexList selected = selectionModel->selectedRows();
    if(selected.count() == 0)
        return;

    QModelIndex index = selected.at(0);
    int r = index.row();
    std::string strAlias = ui->tableWidgetMyMasternodes->item(r, 0)->text().toStdString();

    // Display message box
    QMessageBox::StandardButton retval = QMessageBox::question(this, tr("Confirm masternode start"),
        tr("Are you sure you want to start masternode %1?").arg(QString::fromStdString(strAlias)),
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

void MasternodeList::on_editButton_clicked()
{
    CreateMasternodeDialog dialog(this);
    dialog.setWindowModality(Qt::ApplicationModal);
    dialog.setEditMode();
    dialog.setWindowTitle("Edit Masternode");
    
    QItemSelectionModel* selectionModel = ui->tableWidgetMyMasternodes->selectionModel();
    QModelIndexList selected = selectionModel->selectedRows();
    if(selected.count() == 0)
        return;

    QModelIndex index = selected.at(0);
    int r = index.row();
    QString strAlias = ui->tableWidgetMyMasternodes->item(r, 0)->text();
    QString strIP = ui->tableWidgetMyMasternodes->item(r, 1)->text();
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
        BOOST_FOREACH(CNodeEntry &mne, masternodeConfig.getEntries()) {
            if (mne.getAlias() == strAlias.toStdString())
            {
                mne.setAlias(dialog.getAlias().toStdString());
                mne.setIp(dialog.getIP().toStdString() + ":" + port);
                masternodeConfig.write();
                ui->tableWidgetMyMasternodes->removeRow(r);
                updateMyNodeList(true);
            }
        }

    }
    else
    {
        // Cancel pressed
    }
}

void MasternodeList::on_startAllButton_clicked()
{
    // Display message box
    QMessageBox::StandardButton retval = QMessageBox::question(this, tr("Confirm all masternodes start"),
        tr("Are you sure you want to start ALL masternodes?") + 
            QString("<br>") + tr("<b>") + tr("Warning!") + 
            tr("</b>") + tr(" This action will reset all active masternodes."),
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

void MasternodeList::on_startMissingButton_clicked()
{

    if(masternodeSync.RequestedMasternodeAssets <= MASTERNODE_SYNC_LIST ||
      masternodeSync.RequestedMasternodeAssets == MASTERNODE_SYNC_FAILED) {
        QMessageBox::critical(this, tr("Command is not available right now"),
            tr("You can't use this command until masternode list is synced"));
        return;
    }

    StartMissingDialog dg(this);
    dg.setWindowTitle("Confirm missing masternodes start");
    dg.setText("Are you sure you want to start MISSING masternodes?");
    dg.setCheckboxText("Start all nodes");
    dg.setWarningText(QString("<b>") + tr("WARNING!") + QString("</b>") +
            " If checked all ACTIVE masternodes will be reset.");
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

void MasternodeList::on_tableWidgetMyMasternodes_itemSelectionChanged()
{
    if(ui->tableWidgetMyMasternodes->selectedItems().count() > 0)
    {
        ui->startButton->setEnabled(true);
    }
}

void MasternodeList::on_UpdateButton_clicked()
{
    updateMyNodeList(true);
}

void MasternodeList::on_UpdateVotesButton_clicked()
{
    updateVoteList(true);
}

void MasternodeList::updateVoteList(bool reset)
{
    Q_ASSERT(chainActive.Tip() != NULL);

    static int64_t lastVoteListUpdate = 0;

    // automatically update my masternode list only once in MY_MASTERNODELIST_UPDATE_SECONDS seconds,
    // this update still can be triggered manually at any time via button click
    int64_t timeTillUpdate = lastVoteListUpdate + MY_MASTERNODELIST_UPDATE_SECONDS - GetTime();
    ui->voteSecondsLabel->setText(QString::number(timeTillUpdate));

    if(timeTillUpdate > 0 && !reset) return;
    lastVoteListUpdate = GetTime();

    QString strToFilter;
    ui->tableWidgetVoting->setSortingEnabled(false);
    ui->tableWidgetVoting->clearContents();
    ui->tableWidgetVoting->setRowCount(0);

    int64_t nTotalAllotted = 0;
    CBlockIndex* pindexPrev = chainActive.Tip();
    if(pindexPrev == NULL)
        return;

    int blockStart = GetNextSuperblock(pindexPrev->nHeight);
    int blockEnd = blockStart + GetBudgetPaymentCycleBlocks() - 1;

    std::vector<CBudgetProposal*> winningProps = budget.GetAllProposals();
    BOOST_FOREACH(CBudgetProposal* pbudgetProposal, winningProps)
    {

        CTxDestination address1;
        ExtractDestination(pbudgetProposal->GetPayee(), address1);
        CBitcoinAddress address2(address1);

        if((int64_t)pbudgetProposal->GetRemainingPaymentCount() <= 0) continue;

        if (pbudgetProposal->fValid &&
            pbudgetProposal->nBlockStart <= blockStart &&
            pbudgetProposal->nBlockEnd >= blockEnd)
        {

            // populate list
            QTableWidgetItem *nameItem = new QTableWidgetItem(QString::fromStdString(pbudgetProposal->GetName()));
            QLabel *urlItem = new QLabel("<a href=\"" + QString::fromStdString(pbudgetProposal->GetURL()) + "\">" +
                                         QString::fromStdString(pbudgetProposal->GetURL()) + "</a>");
            urlItem->setOpenExternalLinks(true);
            urlItem->setStyleSheet("background-color: transparent;");
            QTableWidgetItem *hashItem = new QTableWidgetItem(QString::fromStdString(pbudgetProposal->GetHash().ToString()));
            GUIUtil::QTableWidgetNumberItem *blockStartItem = new GUIUtil::QTableWidgetNumberItem((int64_t)pbudgetProposal->GetBlockStart());
            GUIUtil::QTableWidgetNumberItem *blockEndItem = new GUIUtil::QTableWidgetNumberItem((int64_t)pbudgetProposal->GetBlockEnd());
            GUIUtil::QTableWidgetNumberItem *paymentsItem = new GUIUtil::QTableWidgetNumberItem((int64_t)pbudgetProposal->GetTotalPaymentCount());
            GUIUtil::QTableWidgetNumberItem *remainingPaymentsItem = new GUIUtil::QTableWidgetNumberItem((int64_t)pbudgetProposal->GetRemainingPaymentCount());
            GUIUtil::QTableWidgetNumberItem *yesVotesItem = new GUIUtil::QTableWidgetNumberItem((int64_t)pbudgetProposal->GetYeas());
            GUIUtil::QTableWidgetNumberItem *noVotesItem = new GUIUtil::QTableWidgetNumberItem((int64_t)pbudgetProposal->GetNays());
            GUIUtil::QTableWidgetNumberItem *abstainVotesItem = new GUIUtil::QTableWidgetNumberItem((int64_t)pbudgetProposal->GetAbstains());
            QTableWidgetItem *AddressItem = new QTableWidgetItem(QString::fromStdString(address2.ToString()));
            GUIUtil::QTableWidgetNumberItem *totalPaymentItem = new GUIUtil::QTableWidgetNumberItem((pbudgetProposal->GetAmount()*pbudgetProposal->GetTotalPaymentCount())/100000000);
            GUIUtil::QTableWidgetNumberItem *monthlyPaymentItem = new GUIUtil::QTableWidgetNumberItem(pbudgetProposal->GetAmount()/100000000);

            ui->tableWidgetVoting->insertRow(0);
            ui->tableWidgetVoting->setItem(0, 0, nameItem);
            ui->tableWidgetVoting->setCellWidget(0, 1, urlItem);
            ui->tableWidgetVoting->setItem(0, 2, hashItem);
            ui->tableWidgetVoting->setItem(0, 3, blockStartItem);
            ui->tableWidgetVoting->setItem(0, 4, blockEndItem);
            ui->tableWidgetVoting->setItem(0, 5, paymentsItem);
            ui->tableWidgetVoting->setItem(0, 6, remainingPaymentsItem);
            ui->tableWidgetVoting->setItem(0, 7, yesVotesItem);
            ui->tableWidgetVoting->setItem(0, 8, noVotesItem);
            ui->tableWidgetVoting->setItem(0, 9, abstainVotesItem);
            ui->tableWidgetVoting->setItem(0, 10, AddressItem);
            ui->tableWidgetVoting->setItem(0, 11, totalPaymentItem);
            ui->tableWidgetVoting->setItem(0, 12, monthlyPaymentItem);

            std::string projected;
            if ((int64_t)pbudgetProposal->GetYeas() - (int64_t)pbudgetProposal->GetNays() > (ui->tableWidgetMasternodes->rowCount()/10)){
                nTotalAllotted += pbudgetProposal->GetAmount()/100000000;
                projected = "Yes";
            } else {
                projected = "No";
            }
            QTableWidgetItem *projectedItem = new QTableWidgetItem(QString::fromStdString(projected));
            ui->tableWidgetVoting->setItem(0, 13, projectedItem);
        }
    }

    ui->totalAllottedLabel->setText(QString::number(nTotalAllotted));
    ui->tableWidgetVoting->setSortingEnabled(true);

    // reset "timer"
    ui->voteSecondsLabel->setText("0");
}

void MasternodeList::VoteMany(std::string strCommand)
{
    std::vector<CNodeEntry> mnEntries;
    mnEntries = masternodeConfig.getEntries();

    int nVote = VOTE_ABSTAIN;
    if(strCommand == "yea") nVote = VOTE_YES;
    if(strCommand == "nay") nVote = VOTE_NO;

    // Find selected Budget Hash
    QItemSelectionModel* selectionModel = ui->tableWidgetVoting->selectionModel();
    QModelIndexList selected = selectionModel->selectedRows();
    if(selected.count() == 0)
        return;

    QModelIndex index = selected.at(0);
    int r = index.row();
    std::string strHash = ui->tableWidgetVoting->item(r, 2)->text().toStdString();
    uint256 hash;
    hash.SetHex(strHash);

    const int blockStart = ui->tableWidgetVoting->item(r, 3)->text().toInt();
    const int blockEnd = ui->tableWidgetVoting->item(r, 4)->text().toInt();
    if (!budget.CanSubmitVotes(blockStart, blockEnd))
    {
        QMessageBox::critical(
            this,
            tr("Voting Details"),
            tr(
                "Sorry, your vote has not been added to the proposal. "
                "The proposal voting is currently disabled as we're too close to the proposal payment."
            )
        );
        return;
    }

    int success = 0;
    int failed = 0;
    std::string statusObj;

    BOOST_FOREACH(CNodeEntry mne, masternodeConfig.getEntries()) {
        std::string errorMessage;
        std::vector<unsigned char> vchMasterNodeSignature;
        std::string strMasterNodeSignMessage;

        CPubKey pubKeyCollateralAddress;
        CKey keyCollateralAddress;
        CPubKey pubKeyMasternode;
        CKey keyMasternode;

        if(!legacySigner.SetKey(mne.getPrivKey(), errorMessage, keyMasternode, pubKeyMasternode)){
            failed++;
            statusObj += "\nFailed to vote with " + mne.getAlias() + ". Masternode signing error, could not set key correctly: " + errorMessage;
            continue;
        }

        CMasternode* pmn = mnodeman.Find(pubKeyMasternode);
        if(pmn == NULL)
        {
            failed++;
            statusObj += "\nFailed to vote with " + mne.getAlias() + ". Error: Can't find masternode by pubkey";
            continue;
        }

        CBudgetVote vote(pmn->vin, hash, nVote);
        if(!vote.Sign(keyMasternode, pubKeyMasternode)){
            failed++;
            statusObj += "\nFailed to vote with " + mne.getAlias() + ". Error: Failure to sign";
            continue;
        }

        std::string strError = "";
        if(budget.SubmitProposalVote(vote, strError)) {
            vote.Relay();
            success++;
        } else {
            failed++;
            statusObj += "\nFailed to vote with " + mne.getAlias() + ". Error: " + strError;
        }
    }
    std::string returnObj;
    returnObj = strprintf("Voted successfully %d time(s) and failed %d time(s).", success, failed);
    if (failed > 0)
        returnObj += statusObj;

    QMessageBox msg;
    msg.setText(QString::fromStdString(returnObj));
    msg.setWindowTitle(tr("Voting Details"));
    msg.exec();
    updateVoteList(true);
}

void MasternodeList::on_voteManyYesButton_clicked()
{
    // Display message box
    QMessageBox::StandardButton retval = QMessageBox::question(this, tr("Confirm vote-many"),
        tr("Are you sure you want to vote with ALL of your masternodes?"),
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
        VoteMany("yea");
        return;
    }

    VoteMany("yea");
}

void MasternodeList::on_voteManyNoButton_clicked()
{
    // Display message box
    QMessageBox::StandardButton retval = QMessageBox::question(this, tr("Confirm vote-many"),
        tr("Are you sure you want to vote with ALL of your masternodes?"),
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
        VoteMany("nay");
        return;
    }

    VoteMany("nay");
}

void MasternodeList::on_voteManyAbstainButton_clicked()
{
    // Display message box
    QMessageBox::StandardButton retval = QMessageBox::question(this, tr("Confirm vote-many"),
        tr("Are you sure you want to vote with ALL of your masternodes?"),
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
        VoteMany("abstain");
        return;
    }

    VoteMany("abstain");
}

void MasternodeList::on_tableWidgetVoting_itemSelectionChanged()
{
    if(ui->tableWidgetVoting->selectedItems().count() > 0)
    {
        ui->voteManyYesButton->setEnabled(true);
        ui->voteManyNoButton->setEnabled(true);
    }
}

void MasternodeList::on_CreateNewMasternode_clicked()
{
    CreateMasternodeDialog dialog(this);
    dialog.setWindowModality(Qt::ApplicationModal);
    dialog.setWindowTitle("Create a New Masternode");
    QString formattedAmount = BitcoinUnits::formatWithUnit(walletModel->getOptionsModel()->getDisplayUnit(), 
                                                           MASTERNODE_COLLATERAL * COIN);
    dialog.setNoteLabel("*This action will send " + formattedAmount + " to yourself");
    if (dialog.exec())
    {
        // OK Pressed
        QString label = dialog.getLabel();
        QString address = walletModel->getAddressTableModel()->addRow(AddressTableModel::Receive, label, "");
        SendCoinsRecipient recipient(address, label, MASTERNODE_COLLATERAL * COIN, "");
        QList<SendCoinsRecipient> recipients;
        recipients.append(recipient);

        // Get outputs before and after transaction
        std::vector<COutput> vPossibleCoinsBefore;
        pwalletMain->AvailableCoins(vPossibleCoinsBefore, true, NULL, ONLY_10000);

        sendDialog->setModel(walletModel);
        sendDialog->send(recipients);

        std::vector<COutput> vPossibleCoinsAfter;
        pwalletMain->AvailableCoins(vPossibleCoinsAfter, true, NULL, ONLY_10000);

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

                masternodeConfig.add(dialog.getAlias().toStdString(), dialog.getIP().toStdString() + ":" + port, 
                        privateKey, out.tx->GetHash().ToString(), strprintf("%d", out.i));
                masternodeConfig.write();
                updateMyNodeList(true);
            }
        }
    } else {
        // Cancel Pressed
    }
}
