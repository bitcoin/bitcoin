#include "thronelist.h"
#include "ui_thronelist.h"

#include "sync.h"
#include "clientmodel.h"
#include "walletmodel.h"
#include "activethrone.h"
#include "throne-budget.h"
#include "throne-sync.h"
#include "throneconfig.h"
#include "throneman.h"
#include "wallet.h"
#include "init.h"
#include "guiutil.h"

#include <QTimer>
#include <QMessageBox>

CCriticalSection cs_thrones;

ThroneList::ThroneList(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ThroneList),
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

    ui->tableWidgetMyThrones->setColumnWidth(0, columnAliasWidth);
    ui->tableWidgetMyThrones->setColumnWidth(1, columnAddressWidth);
    ui->tableWidgetMyThrones->setColumnWidth(2, columnProtocolWidth);
    ui->tableWidgetMyThrones->setColumnWidth(3, columnStatusWidth);
    ui->tableWidgetMyThrones->setColumnWidth(4, columnActiveWidth);
    ui->tableWidgetMyThrones->setColumnWidth(5, columnLastSeenWidth);

    ui->tableWidgetThrones->setColumnWidth(0, columnAddressWidth);
    ui->tableWidgetThrones->setColumnWidth(1, columnProtocolWidth);
    ui->tableWidgetThrones->setColumnWidth(2, columnStatusWidth);
    ui->tableWidgetThrones->setColumnWidth(3, columnActiveWidth);
    ui->tableWidgetThrones->setColumnWidth(4, columnLastSeenWidth);

    ui->tableWidgetMyThrones->setContextMenuPolicy(Qt::CustomContextMenu);

    QAction *startAliasAction = new QAction(tr("Start alias"), this);
    contextMenu = new QMenu();
    contextMenu->addAction(startAliasAction);
    connect(ui->tableWidgetMyThrones, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(showContextMenu(const QPoint&)));
    connect(startAliasAction, SIGNAL(triggered()), this, SLOT(on_startButton_clicked()));

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(updateNodeList()));
    connect(timer, SIGNAL(timeout()), this, SLOT(updateVoteList()));
    connect(timer, SIGNAL(timeout()), this, SLOT(updateMyNodeList()));
    timer->start(1000);

    updateNodeList();
    updateVoteList();

    CBlockIndex* pindexPrev = chainActive.Tip();
    int nNext = pindexPrev->nHeight - pindexPrev->nHeight % GetBudgetPaymentCycleBlocks() + GetBudgetPaymentCycleBlocks();
    ui->superblockLabel->setText(QString::number(nNext));
}

ThroneList::~ThroneList()
{
    delete ui;
}

void ThroneList::setClientModel(ClientModel *model)
{
    this->clientModel = model;
    if(model)
    {
        // try to update list when throne count changes
        connect(clientModel, SIGNAL(strThronesChanged(QString)), this, SLOT(updateNodeList()));
    }
}

void ThroneList::setWalletModel(WalletModel *model)
{
    this->walletModel = model;
}

void ThroneList::showContextMenu(const QPoint &point)
{
    QTableWidgetItem *item = ui->tableWidgetMyThrones->itemAt(point);
    if(item) contextMenu->exec(QCursor::pos());
}

void ThroneList::StartAlias(std::string strAlias)
{
    std::string statusObj;
    statusObj += "<center>Alias: " + strAlias;

    BOOST_FOREACH(CThroneConfig::CThroneEntry mne, throneConfig.getEntries()) {
        if(mne.getAlias() == strAlias) {
            std::string errorMessage;
            CThroneBroadcast mnb;

            bool result = CThroneBroadcast::Create(mne.getIp(), mne.getPrivKey(), mne.getTxHash(), mne.getOutputIndex(), errorMessage, mnb);

            if(result) {
                statusObj += "<br>Successfully started throne." ;
                mnodeman.UpdateThroneList(mnb);
                mnb.Relay();
            } else {
                statusObj += "<br>Failed to start throne.<br>Error: " + errorMessage;
            }
            break;
        }
    }
    statusObj += "</center>";

    QMessageBox msg;
    msg.setText(QString::fromStdString(statusObj));

    msg.exec();
}

void ThroneList::StartAll(std::string strCommand)
{
    int total = 0;
    int successful = 0;
    int fail = 0;
    std::string statusObj;

    BOOST_FOREACH(CThroneConfig::CThroneEntry mne, throneConfig.getEntries()) {
        std::string errorMessage;
        CThroneBroadcast mnb;

        CTxIn vin = CTxIn(uint256S(mne.getTxHash()), uint32_t(atoi(mne.getOutputIndex().c_str())));
        CThrone *pmn = mnodeman.Find(vin);

        if(strCommand == "start-missing" && pmn) continue;

        bool result = CThroneBroadcast::Create(mne.getIp(), mne.getPrivKey(), mne.getTxHash(), mne.getOutputIndex(), errorMessage, mnb);

        if(result) {
            successful++;
            mnodeman.UpdateThroneList(mnb);
            mnb.Relay();
        } else {
            fail++;
            statusObj += "\nFailed to start " + mne.getAlias() + ". Error: " + errorMessage;
        }
        total++;
    }
    pwalletMain->Lock();

    std::string returnObj;
    returnObj = strprintf("Successfully started %d thrones, failed to start %d, total %d", successful, fail, total);
    if (fail > 0)
        returnObj += statusObj;

    QMessageBox msg;
    msg.setText(QString::fromStdString(returnObj));
    msg.exec();
}

void ThroneList::updateMyThroneInfo(QString alias, QString addr, QString privkey, QString txHash, QString txIndex, CThrone *pmn)
{
    LOCK(cs_mnlistupdate);
    bool bFound = false;
    int nodeRow = 0;

    for(int i=0; i < ui->tableWidgetMyThrones->rowCount(); i++)
    {
        if(ui->tableWidgetMyThrones->item(i, 0)->text() == alias)
        {
            bFound = true;
            nodeRow = i;
            break;
        }
    }

    if(nodeRow == 0 && !bFound) {
        nodeRow = ui->tableWidgetMyThrones->rowCount();
        ui->tableWidgetMyThrones->insertRow(nodeRow);
    }

    QTableWidgetItem *aliasItem = new QTableWidgetItem(alias);
    QTableWidgetItem *addrItem = new QTableWidgetItem(addr);
    QTableWidgetItem *protocolItem = new QTableWidgetItem(QString::number(pmn ? pmn->protocolVersion : -1));
    QTableWidgetItem *statusItem = new QTableWidgetItem(QString::fromStdString(pmn ? pmn->Status() : "MISSING"));
    QTableWidgetItem *activeSecondsItem = new QTableWidgetItem(QString::fromStdString(DurationToDHMS(pmn ? (pmn->lastPing.sigTime - pmn->sigTime) : 0)));
    QTableWidgetItem *lastSeenItem = new QTableWidgetItem(QString::fromStdString(DateTimeStrFormat("%Y-%m-%d %H:%M", pmn ? pmn->lastPing.sigTime : 0)));
    QTableWidgetItem *pubkeyItem = new QTableWidgetItem(QString::fromStdString(pmn ? CBitcoinAddress(pmn->pubkey.GetID()).ToString() : ""));

    ui->tableWidgetMyThrones->setItem(nodeRow, 0, aliasItem);
    ui->tableWidgetMyThrones->setItem(nodeRow, 1, addrItem);
    ui->tableWidgetMyThrones->setItem(nodeRow, 2, protocolItem);
    ui->tableWidgetMyThrones->setItem(nodeRow, 3, statusItem);
    ui->tableWidgetMyThrones->setItem(nodeRow, 4, activeSecondsItem);
    ui->tableWidgetMyThrones->setItem(nodeRow, 5, lastSeenItem);
    ui->tableWidgetMyThrones->setItem(nodeRow, 6, pubkeyItem);
}

void ThroneList::updateMyNodeList(bool reset) {
    static int64_t lastMyListUpdate = 0;

    // automatically update my throne list only once in MY_THRONELIST_UPDATE_SECONDS seconds,
    // this update still can be triggered manually at any time via button click
    int64_t timeTillUpdate = lastMyListUpdate + MY_THRONELIST_UPDATE_SECONDS - GetTime();
    ui->secondsLabel->setText(QString::number(timeTillUpdate));

    if(timeTillUpdate > 0 && !reset) return;
    lastMyListUpdate = GetTime();

    ui->tableWidgetThrones->setSortingEnabled(false);
    BOOST_FOREACH(CThroneConfig::CThroneEntry mne, throneConfig.getEntries()) {
        CTxIn vin = CTxIn(uint256S(mne.getTxHash()), uint32_t(atoi(mne.getOutputIndex().c_str())));
        CThrone *pmn = mnodeman.Find(vin);

        updateMyThroneInfo(QString::fromStdString(mne.getAlias()), QString::fromStdString(mne.getIp()), QString::fromStdString(mne.getPrivKey()), QString::fromStdString(mne.getTxHash()),
            QString::fromStdString(mne.getOutputIndex()), pmn);
    }
    ui->tableWidgetThrones->setSortingEnabled(true);

    // reset "timer"
    ui->secondsLabel->setText("0");
}

void ThroneList::updateNodeList()
{
    static int64_t nTimeListUpdate = 0;

      // to prevent high cpu usage update only once in THRONELIST_UPDATE_SECONDS secondsLabel
      // or THRONELIST_FILTER_COOLDOWN_SECONDS seconds after filter was last changed
      int64_t nTimeToWait =   fFilterUpdated
                              ? nTimeFilterUpdate - GetTime() + THRONELIST_FILTER_COOLDOWN_SECONDS
                              : nTimeListUpdate - GetTime() + THRONELIST_UPDATE_SECONDS;

    if(fFilterUpdated) ui->countLabel->setText(QString::fromStdString(strprintf("Please wait... %d", nTimeToWait)));
    if(nTimeToWait > 0) return;

    nTimeListUpdate = GetTime();
    fFilterUpdated = false;

    TRY_LOCK(cs_thrones, lockThrones);
    if(!lockThrones)
        return;

    QString strToFilter;
    ui->countLabel->setText("Updating...");
    ui->tableWidgetThrones->setSortingEnabled(false);
    ui->tableWidgetThrones->clearContents();
    ui->tableWidgetThrones->setRowCount(0);
    std::vector<CThrone> vThrones = mnodeman.GetFullThroneVector();

    BOOST_FOREACH(CThrone& mn, vThrones)
    {
        // populate list
        // Address, Protocol, Status, Active Seconds, Last Seen, Pub Key
        QTableWidgetItem *addressItem = new QTableWidgetItem(QString::fromStdString(mn.addr.ToString()));
        QTableWidgetItem *protocolItem = new QTableWidgetItem(QString::number(mn.protocolVersion));
        QTableWidgetItem *statusItem = new QTableWidgetItem(QString::fromStdString(mn.Status()));
        QTableWidgetItem *activeSecondsItem = new QTableWidgetItem(QString::fromStdString(DurationToDHMS(mn.lastPing.sigTime - mn.sigTime)));
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

        ui->tableWidgetThrones->insertRow(0);
        ui->tableWidgetThrones->setItem(0, 0, addressItem);
        ui->tableWidgetThrones->setItem(0, 1, protocolItem);
        ui->tableWidgetThrones->setItem(0, 2, statusItem);
        ui->tableWidgetThrones->setItem(0, 3, activeSecondsItem);
        ui->tableWidgetThrones->setItem(0, 4, lastSeenItem);
        ui->tableWidgetThrones->setItem(0, 5, pubkeyItem);
    }

    ui->countLabel->setText(QString::number(ui->tableWidgetThrones->rowCount()));
    ui->tableWidgetThrones->setSortingEnabled(true);

}

void ThroneList::on_filterLineEdit_textChanged(const QString &filterString) {
    strCurrentFilter = filterString;
    nTimeFilterUpdate = GetTime();
    fFilterUpdated = true;
    ui->countLabel->setText(QString::fromStdString(strprintf("Please wait... %d", THRONELIST_FILTER_COOLDOWN_SECONDS)));
}

void ThroneList::on_startButton_clicked()
{
    // Find selected node alias
    QItemSelectionModel* selectionModel = ui->tableWidgetMyThrones->selectionModel();
    QModelIndexList selected = selectionModel->selectedRows();
    if(selected.count() == 0)
        return;

    QModelIndex index = selected.at(0);
    int r = index.row();
    std::string strAlias = ui->tableWidgetMyThrones->item(r, 0)->text().toStdString();

    // Display message box
    QMessageBox::StandardButton retval = QMessageBox::question(this, tr("Confirm throne start"),
        tr("Are you sure you want to start throne %1?").arg(QString::fromStdString(strAlias)),
        QMessageBox::Yes | QMessageBox::Cancel,
        QMessageBox::Cancel);

    if(retval != QMessageBox::Yes)
    {
        return;
    }

    WalletModel::EncryptionStatus encStatus = walletModel->getEncryptionStatus();
    if(encStatus == walletModel->Locked || encStatus == walletModel->UnlockedForAnonymizationOnly)
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

void ThroneList::on_startAllButton_clicked()
{
    // Display message box
    QMessageBox::StandardButton retval = QMessageBox::question(this, tr("Confirm all thrones start"),
        tr("Are you sure you want to start ALL thrones?"),
        QMessageBox::Yes | QMessageBox::Cancel,
        QMessageBox::Cancel);

    if(retval != QMessageBox::Yes)
    {
        return;
    }

    WalletModel::EncryptionStatus encStatus = walletModel->getEncryptionStatus();
    if(encStatus == walletModel->Locked || encStatus == walletModel->UnlockedForAnonymizationOnly)
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

void ThroneList::on_startMissingButton_clicked()
{

    if(throneSync.RequestedThroneAssets <= THRONE_SYNC_LIST ||
      throneSync.RequestedThroneAssets == THRONE_SYNC_FAILED) {
        QMessageBox::critical(this, tr("Command is not available right now"),
            tr("You can't use this command until throne list is synced"));
        return;
    }

    // Display message box
    QMessageBox::StandardButton retval = QMessageBox::question(this,
        tr("Confirm missing thrones start"),
        tr("Are you sure you want to start MISSING thrones?"),
        QMessageBox::Yes | QMessageBox::Cancel,
        QMessageBox::Cancel);

    if(retval != QMessageBox::Yes)
    {
        return;
    }

    WalletModel::EncryptionStatus encStatus = walletModel->getEncryptionStatus();
    if(encStatus == walletModel->Locked || encStatus == walletModel->UnlockedForAnonymizationOnly)
    {
        WalletModel::UnlockContext ctx(walletModel->requestUnlock(true));
        if(!ctx.isValid())
        {
            // Unlock wallet was cancelled
            return;
        }
        StartAll("start-missing");
        return;
    }

    StartAll("start-missing");
}

void ThroneList::on_tableWidgetMyThrones_itemSelectionChanged()
{
    if(ui->tableWidgetMyThrones->selectedItems().count() > 0)
    {
        ui->startButton->setEnabled(true);
    }
}

void ThroneList::on_UpdateButton_clicked()
{
    updateMyNodeList(true);
}

void ThroneList::on_UpdateVotesButton_clicked()
{
    updateVoteList(true);
}

void ThroneList::updateVoteList(bool reset)
{

    static int64_t lastVoteListUpdate = 0;

    // automatically update my throne list only once in MY_THRONELIST_UPDATE_SECONDS seconds,
    // this update still can be triggered manually at any time via button click
    int64_t timeTillUpdate = lastVoteListUpdate + MY_THRONELIST_UPDATE_SECONDS - GetTime();
    ui->voteSecondsLabel->setText(QString::number(timeTillUpdate));

    if(timeTillUpdate > 0 && !reset) return;
    lastVoteListUpdate = GetTime();

    QString strToFilter;
    ui->tableWidgetVoting->setSortingEnabled(false);
    ui->tableWidgetVoting->clearContents();
    ui->tableWidgetVoting->setRowCount(0);

        int64_t nTotalAllotted = 0;
        CBlockIndex* pindexPrev = chainActive.Tip();
        if(pindexPrev == NULL) return;
        int nBlockStart = pindexPrev->nHeight - pindexPrev->nHeight % GetBudgetPaymentCycleBlocks() + GetBudgetPaymentCycleBlocks();
        int nBlockEnd  =  nBlockStart + GetBudgetPaymentCycleBlocks() - 1;

        std::vector<CBudgetProposal*> winningProps = budget.GetAllProposals();
        BOOST_FOREACH(CBudgetProposal* pbudgetProposal, winningProps)
        {

            CTxDestination address1;
            ExtractDestination(pbudgetProposal->GetPayee(), address1);
            CBitcoinAddress address2(address1);

            if((int64_t)pbudgetProposal->GetRemainingPaymentCount() <= 0 ||
                !pbudgetProposal->fValid &&
                !(pbudgetProposal->nBlockStart <= nBlockStart) &&
                !(pbudgetProposal->nBlockEnd >= nBlockEnd) &&
                !pbudgetProposal->IsEstablished()) 
                continue;

            // populate list
            QTableWidgetItem *nameItem = new QTableWidgetItem(QString::fromStdString(pbudgetProposal->GetName()));
            QTableWidgetItem *urlItem = new QTableWidgetItem(QString::fromStdString(pbudgetProposal->GetURL()));
            QTableWidgetItem *hashItem = new QTableWidgetItem(QString::fromStdString(pbudgetProposal->GetHash().ToString()));
            QTableWidgetItem *blockStartItem = new QTableWidgetItem(QString::number((int64_t)pbudgetProposal->GetBlockStart()));
            QTableWidgetItem *blockEndItem = new QTableWidgetItem(QString::number((int64_t)pbudgetProposal->GetBlockEnd()));
            QTableWidgetItem *paymentsItem = new QTableWidgetItem(QString::number((int64_t)pbudgetProposal->GetTotalPaymentCount()));
            QTableWidgetItem *remainingPaymentsItem = new QTableWidgetItem(QString::number((int64_t)pbudgetProposal->GetRemainingPaymentCount()));
            QTableWidgetItem *yesVotesItem = new QTableWidgetItem(QString::number((int64_t)pbudgetProposal->GetYeas()));
            QTableWidgetItem *noVotesItem = new QTableWidgetItem(QString::number((int64_t)pbudgetProposal->GetNays()));
            QTableWidgetItem *abstainVotesItem = new QTableWidgetItem(QString::number((int64_t)pbudgetProposal->GetAbstains()));
            QTableWidgetItem *AddressItem = new QTableWidgetItem(QString::fromStdString(address2.ToString()));
            QTableWidgetItem *totalPaymentItem = new QTableWidgetItem(QString::number((pbudgetProposal->GetAmount()*pbudgetProposal->GetTotalPaymentCount())/100000000 ));
            QTableWidgetItem *monthlyPaymentItem = new QTableWidgetItem(QString::number(pbudgetProposal->GetAmount()/100000000));

            ui->tableWidgetVoting->insertRow(0);
            ui->tableWidgetVoting->setItem(0, 0, nameItem);
            ui->tableWidgetVoting->setItem(0, 1, urlItem);
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
            if ((int64_t)pbudgetProposal->GetYeas() - (int64_t)pbudgetProposal->GetNays() > (ui->tableWidgetThrones->rowCount()/10)){
                nTotalAllotted += pbudgetProposal->GetAmount()/100000000;
                projected = "Yes";
            } else {
                projected = "No";
            }
            QTableWidgetItem *projectedItem = new QTableWidgetItem(QString::fromStdString(projected));
            ui->tableWidgetVoting->setItem(0, 13, projectedItem);
        }

    ui->totalAllottedLabel->setText(QString::number(nTotalAllotted));
    ui->tableWidgetVoting->setSortingEnabled(true);

    // reset "timer"
    ui->voteSecondsLabel->setText("0");

}

void ThroneList::VoteMany(std::string strCommand)
{
    std::vector<CThroneConfig::CThroneEntry> mnEntries;
    mnEntries = throneConfig.getEntries();

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

    int success = 0;
    int failed = 0;
    std::string statusObj;

    BOOST_FOREACH(CThroneConfig::CThroneEntry mne, throneConfig.getEntries()) {
        std::string errorMessage;
        std::vector<unsigned char> vchThroNeSignature;
        std::string strThroNeSignMessage;

        CPubKey pubKeyCollateralAddress;
        CKey keyCollateralAddress;
        CPubKey pubKeyThrone;
        CKey keyThrone;

        if(!darkSendSigner.SetKey(mne.getPrivKey(), errorMessage, keyThrone, pubKeyThrone)){
            failed++;
            statusObj += "\nFailed to vote with " + mne.getAlias() + ". Throne signing error, could not set key correctly: " + errorMessage;
            continue;
        }

        CThrone* pmn = mnodeman.Find(pubKeyThrone);
        if(pmn == NULL)
        {
            failed++;
            statusObj += "\nFailed to vote with " + mne.getAlias() + ". Error: Can't find throne by pubkey";
            continue;
        }

        CBudgetVote vote(pmn->vin, hash, nVote);
        if(!vote.Sign(keyThrone, pubKeyThrone)){
            failed++;
            statusObj += "\nFailed to vote with " + mne.getAlias() + ". Error: Failure to sign";
            continue;
        }

        std::string strError = "";
        if(budget.UpdateProposal(vote, NULL, strError)) {
            budget.mapSeenThroneBudgetVotes.insert(make_pair(vote.GetHash(), vote));
            vote.Relay();
            success++;
        } else {
            failed++;
            statusObj += "\nFailed to update proposal. Error: " + strError;
        }
    }
    std::string returnObj;
    returnObj = strprintf("Voted successfully %d time(s) and failed %d time(s).", success, failed);
    if (failed > 0)
        returnObj += statusObj;

    QMessageBox msg;
    msg.setText(QString::fromStdString(returnObj));
    msg.exec();
    updateVoteList(true);
}

void ThroneList::on_voteManyYesButton_clicked()
{
    // Display message box
    QMessageBox::StandardButton retval = QMessageBox::question(this, tr("Confirm vote-many"),
        tr("Are you sure you want to vote with ALL of your thrones?"),
        QMessageBox::Yes | QMessageBox::Cancel,
        QMessageBox::Cancel);

    if(retval != QMessageBox::Yes)
    {
        return;
    }

    WalletModel::EncryptionStatus encStatus = walletModel->getEncryptionStatus();
    if(encStatus == walletModel->Locked || encStatus == walletModel->UnlockedForAnonymizationOnly)
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

void ThroneList::on_voteManyNoButton_clicked()
{
    // Display message box
    QMessageBox::StandardButton retval = QMessageBox::question(this, tr("Confirm vote-many"),
        tr("Are you sure you want to vote with ALL of your thrones?"),
        QMessageBox::Yes | QMessageBox::Cancel,
        QMessageBox::Cancel);

    if(retval != QMessageBox::Yes)
    {
        return;
    }

    WalletModel::EncryptionStatus encStatus = walletModel->getEncryptionStatus();
    if(encStatus == walletModel->Locked || encStatus == walletModel->UnlockedForAnonymizationOnly)
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

void ThroneList::on_voteManyAbstainButton_clicked()
{
    // Display message box
    QMessageBox::StandardButton retval = QMessageBox::question(this, tr("Confirm vote-many"),
        tr("Are you sure you want to vote with ALL of your thrones?"),
        QMessageBox::Yes | QMessageBox::Cancel,
        QMessageBox::Cancel);

    if(retval != QMessageBox::Yes)
    {
        return;
    }

    WalletModel::EncryptionStatus encStatus = walletModel->getEncryptionStatus();
    if(encStatus == walletModel->Locked || encStatus == walletModel->UnlockedForAnonymizationOnly)
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

void ThroneList::on_tableWidgetVoting_itemSelectionChanged()
{
    if(ui->tableWidgetVoting->selectedItems().count() > 0)
    {
        ui->voteManyYesButton->setEnabled(true);
        ui->voteManyNoButton->setEnabled(true);
    }
}
