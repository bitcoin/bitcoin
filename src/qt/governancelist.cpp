#include "governancelist.h"
#include "qt/forms/ui_governancelist.h"
#include <masternodes/sync.h>
#include <messagesigner.h>
#include <governance/governance.h>
#include <governance/governance-vote.h>
#include <governance/governance-classes.h>
#include <governance/governance-validators.h>
#include <qt/governancedialog.h>
#include <messagesigner.h>
#include <qt/clientmodel.h>
#include <validation.h>
#include <wallet/wallet.h>
#include <uint256.h>
#include <qt/guiutil.h>

#include <QTimer>
#include <QMessageBox>

GovernanceList::GovernanceList(const PlatformStyle *platformStyle, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::GovernanceList),
    clientModel(0),
    walletModel(0)
{
    ui->setupUi(this);
    int columnHash = 50;
    int columnName = 250;
    int columnUrl = 250;
    int columnAmount = 120;
    int columnvoteYes = 80;
    int columnvoteNo = 80;
    int columnAbsoluteYes = 150;
    int columnFund = 50;
    ui->tableWidgetGobjects->setColumnWidth(0, columnHash);
    ui->tableWidgetGobjects->setColumnWidth(1, columnName);
    ui->tableWidgetGobjects->setColumnWidth(2, columnUrl);
    ui->tableWidgetGobjects->setColumnWidth(3, columnAmount);
    ui->tableWidgetGobjects->setColumnWidth(4, columnvoteYes);
    ui->tableWidgetGobjects->setColumnWidth(5, columnvoteNo);
    ui->tableWidgetGobjects->setColumnWidth(6, columnAbsoluteYes);
    ui->tableWidgetGobjects->setColumnWidth(7, columnFund);
    contextMenu = new QMenu();
    connect(ui->tableWidgetGobjects, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(showContextMenu(const QPoint&)));
    connect(ui->tableWidgetGobjects, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(on_GovernanceButton_clicked()));
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(updateGobjects()));
    timer->start(1000);
    fFilterUpdated = false;
    nTimeFilterUpdated = GetTime();
}

void GovernanceList::on_voteYesButton_clicked()
{
    std::string gobjectSingle;
    {
        LOCK(cs_gobjlist);
        // Find selected gobject
        QItemSelectionModel* selectionModel = ui->tableWidgetGobjects->selectionModel();
        QModelIndexList selected = selectionModel->selectedRows();
        if(selected.count() == 0) return;
        QModelIndex index = selected.at(0);
        int nSelectedRow = index.row();
        gobjectSingle = ui->tableWidgetGobjects->item(nSelectedRow, 0)->text().toStdString();
    }
    uint256 parsedGobjectHash;
    parsedGobjectHash.SetHex(gobjectSingle);
    // Display message box
    QMessageBox::StandardButton retval = QMessageBox::question(this, tr("Confirm YES vote"),
                                         tr("Are you sure you want vote YES on this proposal with all your masternodes?"),
                                         QMessageBox::Yes | QMessageBox::Cancel,
                                         QMessageBox::Cancel);
    if(retval != QMessageBox::Yes) return;
    WalletModel::EncryptionStatus encStatus = walletModel->getEncryptionStatus();
    if(encStatus == walletModel->Locked || encStatus == walletModel->UnlockedForStakingOnly) {
        WalletModel::UnlockContext ctx(walletModel->requestUnlock());
        if(!ctx.isValid()) return; // Unlock wallet was cancelled
        vote_outcome_enum_t VoteValue = vote_outcome_enum_t(1);
        Vote(parsedGobjectHash, VoteValue);
        return;
    }
    vote_outcome_enum_t VoteValue = vote_outcome_enum_t(1);
    Vote(parsedGobjectHash, VoteValue);
}

void GovernanceList::on_voteNoButton_clicked()
{
    std::string gobjectSingle;
    {
        LOCK(cs_gobjlist);
        // Find selected gobject
        QItemSelectionModel* selectionModel = ui->tableWidgetGobjects->selectionModel();
        QModelIndexList selected = selectionModel->selectedRows();
        if(selected.count() == 0) return;
        QModelIndex index = selected.at(0);
        int nSelectedRow = index.row();
        gobjectSingle = ui->tableWidgetGobjects->item(nSelectedRow, 0)->text().toStdString();
    }
    uint256 parsedGobjectHash;
    parsedGobjectHash.SetHex(gobjectSingle);
    // Display message box
    QMessageBox::StandardButton retval = QMessageBox::question(this, tr("Confirm NO vote"),
                                         tr("Are you sure you want vote NO on this proposal with all your masternodes?"),
                                         QMessageBox::Yes | QMessageBox::Cancel,
                                         QMessageBox::Cancel);
    if(retval != QMessageBox::Yes) return;
    WalletModel::EncryptionStatus encStatus = walletModel->getEncryptionStatus();
    if(encStatus == walletModel->Locked || encStatus == walletModel->UnlockedForStakingOnly) {
        WalletModel::UnlockContext ctx(walletModel->requestUnlock());
        if(!ctx.isValid()) return; // Unlock wallet was cancelled
        vote_outcome_enum_t VoteValue = vote_outcome_enum_t(2);
        Vote(parsedGobjectHash, VoteValue);
        return;
    }
    vote_outcome_enum_t VoteValue = vote_outcome_enum_t(2);
    Vote(parsedGobjectHash, VoteValue);
}

void GovernanceList::on_voteAbstainButton_clicked()
{
    std::string gobjectSingle;
    {
        LOCK(cs_gobjlist);
        // Find selected gobject
        QItemSelectionModel* selectionModel = ui->tableWidgetGobjects->selectionModel();
        QModelIndexList selected = selectionModel->selectedRows();
        if(selected.count() == 0) return;
        QModelIndex index = selected.at(0);
        int nSelectedRow = index.row();
        gobjectSingle = ui->tableWidgetGobjects->item(nSelectedRow, 0)->text().toStdString();
    }
    uint256 parsedGobjectHash;
    parsedGobjectHash.SetHex(gobjectSingle);

    // Display message box
    QMessageBox::StandardButton retval = QMessageBox::question(this, tr("Confirm ABSTAIN vote"),
                                         tr("Are you sure you want vote ABSTAIN on this proposal with all your masternodes?"),
                                         QMessageBox::Yes | QMessageBox::Cancel,
                                         QMessageBox::Cancel);
    if(retval != QMessageBox::Yes) return;
    WalletModel::EncryptionStatus encStatus = walletModel->getEncryptionStatus();
    if(encStatus == walletModel->Locked || encStatus == walletModel->UnlockedForStakingOnly) {
        WalletModel::UnlockContext ctx(walletModel->requestUnlock());
        if(!ctx.isValid()) return; // Unlock wallet was cancelled
        vote_outcome_enum_t VoteValue = vote_outcome_enum_t(3);
        Vote(parsedGobjectHash, VoteValue);
        return;
    }
    vote_outcome_enum_t VoteValue = vote_outcome_enum_t(3);
    Vote(parsedGobjectHash, VoteValue);
}

void GovernanceList::Vote(uint256 nHash, vote_outcome_enum_t eVoteOutcome)
{
    int nTotalVotes = 0;
    int nSuccessful = 0;
    int nFailed = 0;
    vote_signal_enum_t eVoteSignal = CGovernanceVoting::ConvertVoteSignal("funding");
    if (eVoteSignal == VOTE_SIGNAL_NONE || eVoteOutcome == VOTE_OUTCOME_NONE) {
        LogPrint(BCLog::GOVERNANCE, "We are not going to be voting.");
        return;
    }

    std::map<uint256, CKey> votingKeys;
    auto mnList = deterministicMNManager->GetListAtChainTip();
    mnList.ForEachMN(true, [&](const CDeterministicMNCPtr& dmn) {
        CKey votingKey;
        if (GetMainWallet()->GetKey(dmn->pdmnState->keyIDVoting, votingKey)) {
            nTotalVotes++;
            votingKeys.emplace(dmn->proTxHash, votingKey);
        }
    });

    LogPrint(BCLog::GOVERNANCE, "Will be voting using %d deterministic masternodes.", nTotalVotes);

    {
        LOCK(governance.cs);
        CGovernanceObject* pGovObj = governance.FindGovernanceObject(nHash);
        int govObjType = pGovObj->GetObjectType();
    }

    // iterate through the mn keys
    for (const auto& p : votingKeys) {
        const auto& proTxHash = p.first;
        const auto& key = p.second;
        auto dmn = mnList.GetValidMN(proTxHash);
        if (!dmn) {
            nFailed++;
            continue;
        }

        CGovernanceVote vote(dmn->collateralOutpoint, nHash, eVoteSignal, eVoteOutcome);
        if (!vote.Sign(key, key.GetPubKey().GetID())) {
            nFailed++;
            continue;
        }

        CGovernanceException exception;
        if (governance.ProcessVoteAndRelay(vote, exception, *g_connman)) {
            nSuccessful++;
        } else {
            nFailed++;
        }
    }
    LogPrint(BCLog::GOVERNANCE, "Voted successfully %d time(s) and failed %d time(s).", nSuccessful, nFailed);
}

GovernanceList::~GovernanceList()
{
    delete ui;
}

void GovernanceList::setWalletModel(WalletModel *model)
{
    this->walletModel = model;
}

void GovernanceList::showContextMenu(const QPoint &point)
{
    QTableWidgetItem *item = ui->tableWidgetGobjects->itemAt(point);
    if(item) contextMenu->exec(QCursor::pos());
}

void GovernanceList::updateGobjects()
{
    TRY_LOCK(cs_gobjlist, fLockAcquired);
    if(!fLockAcquired) {
        return;
    }
    static int64_t nTimeListUpdated = GetTime();
    // to prevent high cpu usage update only once in GOBJECT_UPDATE_SECONDS seconds
    // or GOBJECT_COOLDOWN_SECONDS seconds after filter was last changed
    int64_t nSecondsToWait = fFilterUpdated
                             ? nTimeFilterUpdated - GetTime() + GOBJECT_COOLDOWN_SECONDS
                             : nTimeListUpdated - GetTime() + GOBJECT_UPDATE_SECONDS;
    if(fFilterUpdated) ui->countGobjectLabel->setText(QString::fromStdString(strprintf("Please wait... %d", nSecondsToWait)));
    if(!fFilterUpdated) ui->countGobjectLabel->setText(QString::fromStdString(strprintf("Please wait... %d", nSecondsToWait)));
    if(nSecondsToWait > 0) return;
    nTimeListUpdated = GetTime();
    fFilterUpdated = false;
    QString strToFilter;
    ui->countGobjectLabel->setText("Updating...");
    ui->tableWidgetGobjects->setSortingEnabled(false);
    ui->tableWidgetGobjects->clearContents();
    ui->tableWidgetGobjects->setRowCount(0);
    int nStartTime = 0;
for (const auto& pGovObj : governance.GetAllNewerThan(nStartTime)) {
        int gobject = pGovObj->GetObjectType();
        if (gobject == 1) {
            // populate list
            // Address, Protocol, Status, Active Seconds, Last Seen, Pub Key
            // Get Object as Hex and convert to std::string
            std::string HexStr = pGovObj->GetDataAsHexString();
            std::vector<unsigned char> v = ParseHex(HexStr);
            std::string s(v.begin(), v.end());
            std::string nameStr = getValue(s, "name", true);
            std::string urlStr = getValue(s, "url", true);
            std::string amountStr = getNumericValue(s, "payment_amount");
            // Define "Funding" for Vote count
            vote_signal_enum_t VoteCountType = vote_signal_enum_t(1);
            std::string vFunding;
            if (pGovObj->IsSetCachedFunding()) {
                vFunding = "Yes";
            } else {vFunding = "No";}
            QString name =  QString::fromStdString(nameStr);
            QString url = QString::fromStdString(urlStr);
            QString amount = QString::fromStdString(amountStr);
            std::string hash = pGovObj->GetHash().GetHex();
            QTableWidgetItem *Hash = new QTableWidgetItem(QString::fromStdString(hash));
            QTableWidgetItem *nameItem = new QTableWidgetItem(name);
            QTableWidgetItem *urlItem = new QTableWidgetItem(url);
            QTableWidgetItem *amounItem = new QTableWidgetItem(amount);
            QTableWidgetItem *voteYes = new QTableWidgetItem(QString::number(pGovObj->GetYesCount(VoteCountType)));
            QTableWidgetItem *voteNo = new QTableWidgetItem(QString::number(pGovObj->GetNoCount(VoteCountType)));
            QTableWidgetItem *AbsoluteYes = new QTableWidgetItem(QString::number(pGovObj->GetAbsoluteYesCount(VoteCountType)));
            QTableWidgetItem *fundingStatus = new QTableWidgetItem(QString::fromStdString(vFunding));
            if (strCurrentFilter != "") {
                strToFilter =   Hash->text() + " " +
                                nameItem->text() + " " +
                                amounItem->text() + " " +
                                voteYes->text() + " " +
                                voteNo->text() + " " +
                                AbsoluteYes->text() + " " +
                                fundingStatus->text();
                if (!strToFilter.contains(strCurrentFilter)) continue;
            }
            ui->tableWidgetGobjects->insertRow(0);
            ui->tableWidgetGobjects->setItem(0, 0, Hash);
            ui->tableWidgetGobjects->setItem(0, 1, nameItem);
            ui->tableWidgetGobjects->setItem(0, 2, urlItem);
            ui->tableWidgetGobjects->setItem(0, 3, amounItem);
            ui->tableWidgetGobjects->setItem(0, 4, voteYes);
            ui->tableWidgetGobjects->setItem(0, 5, voteNo);
            ui->tableWidgetGobjects->setItem(0, 6, AbsoluteYes);
            ui->tableWidgetGobjects->setItem(0, 7, fundingStatus);
        }
    }
    ui->countGobjectLabel->setText(QString::number(ui->tableWidgetGobjects->rowCount()));
    ui->tableWidgetGobjects->setSortingEnabled(true);
}

void GovernanceList::setClientModel(ClientModel *model)
{
    this->clientModel = model;
}

void GovernanceList::on_UpdateButton_clicked()
{
    updateGobjects();
}
void GovernanceList::on_filterLineEdit_textChanged(const QString &strFilterIn)
{
    strCurrentFilter = strFilterIn;
    nTimeFilterUpdated = GetTime();
    fFilterUpdated = true;
    ui->countGobjectLabel->setText(QString::fromStdString(strprintf("Please wait... %d", GOBJECT_UPDATE_SECONDS)));
}

void GovernanceList::on_GovernanceButton_clicked()
{
    std::string gobjectSingle;
    {
        LOCK(cs_gobjlist);
        // Find selected gobject
        QItemSelectionModel* selectionModel = ui->tableWidgetGobjects->selectionModel();
        QModelIndexList selected = selectionModel->selectedRows();
        if(selected.count() == 0) return;
        QModelIndex index = selected.at(0);
        int nSelectedRow = index.row();
        gobjectSingle = ui->tableWidgetGobjects->item(nSelectedRow, 0)->text().toStdString();
    }
    uint256 parsedGobjectHash;
    parsedGobjectHash.SetHex(gobjectSingle);
    ShowGovernanceObject(parsedGobjectHash);
}

void GovernanceList::ShowGovernanceObject(uint256 gobjectSingle)
{
    if(!walletModel || !walletModel->getOptionsModel())
        return;
    CGovernanceObject* pGovObj = governance.FindGovernanceObject(gobjectSingle);
    // Title of popup window
    QString strWindowtitle = tr("Additional information for Governance Object");
    // Title above QR-Code
    QString strQRCodeTitle = "Governance Object";
    vote_signal_enum_t VotesFunding = vote_signal_enum_t(1);
    vote_signal_enum_t VotesValid = vote_signal_enum_t(2);
    vote_signal_enum_t VotesDelete = vote_signal_enum_t(3);
    vote_signal_enum_t VotesEndorsed = vote_signal_enum_t(4);
    std::string s = pGovObj->GetDataAsHexString();
    std::string dataString = pGovObj->GetDataAsHexString();
    std::string dataHex = pGovObj->GetDataAsHexString();
    std::string name = getValue(s,"name", true);
    std::string url = getValue(s,"url", true);
    std::string amount = getNumericValue(s,"payment_amount");
    std::string hash = gobjectSingle.ToString();
    // Funding Variables
    std::string FundingYes = std::to_string(pGovObj->GetYesCount(VotesFunding));
    std::string FundingNo = std::to_string(pGovObj->GetNoCount(VotesFunding));
    std::string FundingAbstain = std::to_string(pGovObj->GetAbstainCount(VotesFunding));
    std::string FundingAbYes = std::to_string(pGovObj->GetAbsoluteYesCount(VotesFunding));
    // Valid Variables
    std::string ValidYes = std::to_string(pGovObj->GetYesCount(VotesValid));
    std::string ValidNo = std::to_string(pGovObj->GetNoCount(VotesValid));
    std::string ValidAbstain = std::to_string(pGovObj->GetAbstainCount(VotesValid));
    std::string ValidAbYes = std::to_string(pGovObj->GetAbsoluteYesCount(VotesValid));
    // Delete Variables
    std::string DeleteYes = std::to_string(pGovObj->GetYesCount(VotesDelete));
    std::string DeleteNo = std::to_string(pGovObj->GetNoCount(VotesDelete));
    std::string DeleteAbstain = std::to_string(pGovObj->GetAbstainCount(VotesDelete));
    std::string DeleteAbYes = std::to_string(pGovObj->GetAbsoluteYesCount(VotesDelete));
    // Endorse Variables
    std::string EndorseYes = std::to_string(pGovObj->GetYesCount(VotesEndorsed));
    std::string EndorseNo = std::to_string(pGovObj->GetNoCount(VotesEndorsed));
    std::string EndorseAbstain = std::to_string(pGovObj->GetAbstainCount(VotesEndorsed));
    std::string EndorseAbYes = std::to_string(pGovObj->GetAbsoluteYesCount(VotesEndorsed));
    // Create dialog text as HTML
    QString strHTML = "<html><font face='verdana, arial, helvetica, sans-serif'>";
    strHTML += "<b>" + tr("Hash") +          ": </b>" + GUIUtil::HtmlEscape(hash) + "<br>";
    strHTML += "<b>" + tr("Proposal Name") +      ": </b>" + GUIUtil::HtmlEscape(name) + "<br>";
    strHTML += "<b>" + tr("Proposal Url") +       ": </b>" + GUIUtil::HtmlEscape(url) + "<br>";
    strHTML += "<b>" + tr("Payment Amount") +     ": </b>" + GUIUtil::HtmlEscape(amount) + "<br>";
    strHTML += "<p><b>" + tr("Funding Votes") +":</b>" + "<br><span>Yes: " + GUIUtil::HtmlEscape(FundingYes) + "</span>" + "<br><span>No: " + GUIUtil::HtmlEscape(FundingNo) + "</span>" + "<br><span>Abstain: " + GUIUtil::HtmlEscape(FundingAbstain) + "</span>" + "<br><span>Absolute Yes: " + GUIUtil::HtmlEscape(FundingAbYes) + "</span></p>";
    strHTML += "<p><b>" + tr("Valid Votes") +":</b>" + "<br><span>Yes: " + GUIUtil::HtmlEscape(ValidYes) + "</span>" + "<br><span>No: " + GUIUtil::HtmlEscape(ValidNo) + "</span>" + "<br><span>Abstain: " + GUIUtil::HtmlEscape(ValidAbstain) + "</span>" + "<br><span>Absolute Yes: " + GUIUtil::HtmlEscape(ValidAbYes) + "</span></p>";
    strHTML += "<p><b>" + tr("Delete Votes") +":</b>" + "<br><span>Yes: " + GUIUtil::HtmlEscape(DeleteYes) + "</span>" + "<br><span>No: " + GUIUtil::HtmlEscape(DeleteNo) + "</span>" + "<br><span>Abstain: " + GUIUtil::HtmlEscape(DeleteAbstain) + "</span>" + "<br><span>Absolute Yes: " + GUIUtil::HtmlEscape(DeleteAbYes) + "</span></p>";
    strHTML += "<p><b>" + tr("Endorse Votes") +":</b>" + "<br><span>Yes: " + GUIUtil::HtmlEscape(EndorseYes) + "</span>" + "<br><span>No: " + GUIUtil::HtmlEscape(EndorseNo) + "</span>" + "<br><span>Abstain: " + GUIUtil::HtmlEscape(EndorseAbstain) + "</span>" + "<br><span>Absolute Yes: " + GUIUtil::HtmlEscape(EndorseAbYes) + "</span></p>";
    strHTML += "<b>" + tr("Raw Information (Hex)") +     ": </b>" + GUIUtil::HtmlEscape(dataHex) + "<br>";
    strHTML += "<b>" + tr("Raw Information (String)") +     ": </b>" + GUIUtil::HtmlEscape(dataString) + "<br>";
    //strHTML += "<b>" + tr("Sentinel") +     ": </b>" + (mn.lastPing.nSentinelVersion > DEFAULT_SENTINEL_VERSION ? GUIUtil::HtmlEscape(SafeIntVersionToString(mn.lastPing.nSentinelVersion)) : tr("Unknown")) + "<br>";
    //strHTML += "<b>" + tr("Status") +       ": </b>" + GUIUtil::HtmlEscape(CMasternode::StateToString(mn.nActiveState)) + "<br>";
    //strHTML += "<b>" + tr("Payee") +        ": </b>" + GUIUtil::HtmlEscape(CBitcoinAddress(mn.pubKeyCollateralAddress.GetID()).ToString()) + "<br>";
    //strHTML += "<b>" + tr("Active") +       ": </b>" + GUIUtil::HtmlEscape(DurationToDHMS(mn.lastPing.sigTime - mn.sigTime)) + "<br>";
    //strHTML += "<b>" + tr("Last Seen") +    ": </b>" + GUIUtil::HtmlEscape(DateTimeStrFormat("%Y-%m-%d %H:%M", mn.lastPing.sigTime + GetOffsetFromUtc())) + "<br>";
    // Open Governance dialog
    GovernanceDialog *dialog = new GovernanceDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setModel(walletModel->getOptionsModel());
    dialog->setInfo(strWindowtitle, strWindowtitle,strHTML, "");
    dialog->show();
}

std::string getValue(std::string str,std::string key, bool format)
{
    std::string s_pattern = "\"" + key + "\"";
    int beg = str.find(s_pattern);
    int f_comma = str.find("\"", beg+s_pattern.size());
    int s_comma = str.find("\"", f_comma+1);
    std::string s2 = str.substr(f_comma+1, s_comma-f_comma-1);
    return s2;
}



std::string getNumericValue(std::string str, std::string key)
{
    std::string s_pattern = "\"" + key + "\"";
    int beg = str.find(s_pattern);
    int f_comma = str.find(":", beg+s_pattern.size());
    int s_comma = str.find(",", f_comma+1);
    std::string s2 = str.substr(f_comma+1, s_comma-f_comma-1);
    return s2;
}
