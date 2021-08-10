#include <qt/masternodelist.h>
#include <qt/forms/ui_masternodelist.h>

#include <evo/deterministicmns.h>
#include <qt/clientmodel.h>
#include <clientversion.h>
#include <coins.h>
#include <qt/guiutil.h>
#include <netbase.h>
#include <qt/walletmodel.h>

#include <univalue.h>

#include <QMessageBox>
#include <QTableWidgetItem>
#include <QtGui/QClipboard>

int GetOffsetFromUtc()
{
#if QT_VERSION < 0x050200
    const QDateTime dateTime1 = QDateTime::currentDateTime();
    const QDateTime dateTime2 = QDateTime(dateTime1.date(), dateTime1.time(), Qt::UTC);
    return dateTime1.secsTo(dateTime2);
#else
    return QDateTime::currentDateTime().offsetFromUtc();
#endif
}

template <typename T>
class CMasternodeListWidgetItem : public QTableWidgetItem
{
    T itemData;

public:
    explicit CMasternodeListWidgetItem(const QString& text, const T& data, int type = Type) :
        QTableWidgetItem(text, type),
        itemData(data) {}

    bool operator<(const QTableWidgetItem& other) const
    {
        return itemData < ((CMasternodeListWidgetItem*)&other)->itemData;
    }
};

MasternodeList::MasternodeList(QWidget* parent) :
    QWidget(parent),
    ui(new Ui::MasternodeList),
    clientModel(0),
    walletModel(0),
    fFilterUpdatedDIP3(true),
    nTimeFilterUpdatedDIP3(0),
    nTimeUpdatedDIP3(0),
    mnListChanged(true)
{
    ui->setupUi(this);

    GUIUtil::setFont({ui->label_count_2,
                      ui->countLabelDIP3
                     }, GUIUtil::FontWeight::Bold, 14);
    GUIUtil::setFont({ui->label_filter_2}, GUIUtil::FontWeight::Normal, 15);

    int columnAddressWidth = 200;
    int columnStatusWidth = 80;
    int columnPoSeScoreWidth = 80;
    int columnRegisteredWidth = 80;
    int columnLastPaidWidth = 80;
    int columnNextPaymentWidth = 100;
    int columnPayeeWidth = 130;
    int columnOperatorRewardWidth = 130;
    int columnCollateralWidth = 130;
    int columnOwnerWidth = 130;
    int columnVotingWidth = 130;

    ui->tableWidgetMasternodesDIP3->setColumnWidth(COLUMN_SERVICE, columnAddressWidth);
    ui->tableWidgetMasternodesDIP3->setColumnWidth(COLUMN_STATUS, columnStatusWidth);
    ui->tableWidgetMasternodesDIP3->setColumnWidth(COLUMN_POSE, columnPoSeScoreWidth);
    ui->tableWidgetMasternodesDIP3->setColumnWidth(COLUMN_REGISTERED, columnRegisteredWidth);
    ui->tableWidgetMasternodesDIP3->setColumnWidth(COLUMN_LAST_PAYMENT, columnLastPaidWidth);
    ui->tableWidgetMasternodesDIP3->setColumnWidth(COLUMN_NEXT_PAYMENT, columnNextPaymentWidth);
    ui->tableWidgetMasternodesDIP3->setColumnWidth(COLUMN_PAYOUT_ADDRESS, columnPayeeWidth);
    ui->tableWidgetMasternodesDIP3->setColumnWidth(COLUMN_OPERATOR_REWARD, columnOperatorRewardWidth);
    ui->tableWidgetMasternodesDIP3->setColumnWidth(COLUMN_COLLATERAL_ADDRESS, columnCollateralWidth);
    ui->tableWidgetMasternodesDIP3->setColumnWidth(COLUMN_OWNER_ADDRESS, columnOwnerWidth);
    ui->tableWidgetMasternodesDIP3->setColumnWidth(COLUMN_VOTING_ADDRESS, columnVotingWidth);

    // dummy column for proTxHash
    // TODO use a proper table model for the MN list
    ui->tableWidgetMasternodesDIP3->insertColumn(COLUMN_PROTX_HASH);
    ui->tableWidgetMasternodesDIP3->setColumnHidden(COLUMN_PROTX_HASH, true);

    ui->tableWidgetMasternodesDIP3->setContextMenuPolicy(Qt::CustomContextMenu);

    ui->filterLineEditDIP3->setPlaceholderText(tr("Filter by any property (e.g. address or protx hash)"));
    ui->checkBoxMyMasternodesOnly->setEnabled(false);

    QAction* copyProTxHashAction = new QAction(tr("Copy ProTx Hash"), this);
    QAction* copyCollateralOutpointAction = new QAction(tr("Copy Collateral Outpoint"), this);
    contextMenuDIP3 = new QMenu(this);
    contextMenuDIP3->addAction(copyProTxHashAction);
    contextMenuDIP3->addAction(copyCollateralOutpointAction);
    connect(ui->tableWidgetMasternodesDIP3, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(showContextMenuDIP3(const QPoint&)));
    connect(ui->tableWidgetMasternodesDIP3, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(extraInfoDIP3_clicked()));
    connect(copyProTxHashAction, SIGNAL(triggered()), this, SLOT(copyProTxHash_clicked()));
    connect(copyCollateralOutpointAction, SIGNAL(triggered()), this, SLOT(copyCollateralOutpoint_clicked()));

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(updateDIP3ListScheduled()));
    timer->start(1000);

    GUIUtil::updateFonts();
}

MasternodeList::~MasternodeList()
{
    delete ui;
}

void MasternodeList::setClientModel(ClientModel* model)
{
    this->clientModel = model;
    if (model) {
        // try to update list when masternode count changes
        connect(clientModel, SIGNAL(masternodeListChanged()), this, SLOT(handleMasternodeListChanged()));
    }
}

void MasternodeList::setWalletModel(WalletModel* model)
{
    this->walletModel = model;
    ui->checkBoxMyMasternodesOnly->setEnabled(model != nullptr);
}

void MasternodeList::showContextMenuDIP3(const QPoint& point)
{
    QTableWidgetItem* item = ui->tableWidgetMasternodesDIP3->itemAt(point);
    if (item) contextMenuDIP3->exec(QCursor::pos());
}

void MasternodeList::handleMasternodeListChanged()
{
    LOCK(cs_dip3list);
    mnListChanged = true;
}

void MasternodeList::updateDIP3ListScheduled()
{
    TRY_LOCK(cs_dip3list, fLockAcquired);
    if (!fLockAcquired) return;

    if (!clientModel || clientModel->node().shutdownRequested()) {
        return;
    }

    // To prevent high cpu usage update only once in MASTERNODELIST_FILTER_COOLDOWN_SECONDS seconds
    // after filter was last changed unless we want to force the update.
    if (fFilterUpdatedDIP3) {
        int64_t nSecondsToWait = nTimeFilterUpdatedDIP3 - GetTime() + MASTERNODELIST_FILTER_COOLDOWN_SECONDS;
        ui->countLabelDIP3->setText(tr("Please wait...") + " " + QString::number(nSecondsToWait));

        if (nSecondsToWait <= 0) {
            updateDIP3List();
            fFilterUpdatedDIP3 = false;
        }
    } else if (mnListChanged) {
        int64_t nMnListUpdateSecods = clientModel->masternodeSync().isBlockchainSynced() ? MASTERNODELIST_UPDATE_SECONDS : MASTERNODELIST_UPDATE_SECONDS * 10;
        int64_t nSecondsToWait = nTimeUpdatedDIP3 - GetTime() + nMnListUpdateSecods;

        if (nSecondsToWait <= 0) {
            updateDIP3List();
            mnListChanged = false;
        }
    }
}

void MasternodeList::updateDIP3List()
{
    if (!clientModel || clientModel->node().shutdownRequested()) {
        return;
    }

    auto mnList = clientModel->getMasternodeList();
    std::map<uint256, CTxDestination> mapCollateralDests;

    {
        // Get all UTXOs for each MN collateral in one go so that we can reduce locking overhead for cs_main
        // We also do this outside of the below Qt list update loop to reduce cs_main locking time to a minimum
        mnList.ForEachMN(false, [&](const CDeterministicMNCPtr& dmn) {
            CTxDestination collateralDest;
            Coin coin;
            if (clientModel->node().getUnspentOutput(dmn->collateralOutpoint, coin) && ExtractDestination(coin.out.scriptPubKey, collateralDest)) {
                mapCollateralDests.emplace(dmn->proTxHash, collateralDest);
            }
        });
    }

    LOCK(cs_dip3list);

    QString strToFilter;
    ui->countLabelDIP3->setText(tr("Updating..."));
    ui->tableWidgetMasternodesDIP3->setSortingEnabled(false);
    ui->tableWidgetMasternodesDIP3->clearContents();
    ui->tableWidgetMasternodesDIP3->setRowCount(0);

    nTimeUpdatedDIP3 = GetTime();

    auto projectedPayees = mnList.GetProjectedMNPayees(mnList.GetValidMNsCount());
    std::map<uint256, int> nextPayments;
    for (size_t i = 0; i < projectedPayees.size(); i++) {
        const auto& dmn = projectedPayees[i];
        nextPayments.emplace(dmn->proTxHash, mnList.GetHeight() + (int)i + 1);
    }

    std::set<COutPoint> setOutpts;
    if (walletModel && ui->checkBoxMyMasternodesOnly->isChecked()) {
        std::vector<COutPoint> vOutpts;
        walletModel->wallet().listProTxCoins(vOutpts);
        for (const auto& outpt : vOutpts) {
            setOutpts.emplace(outpt);
        }
    }

    mnList.ForEachMN(false, [&](const CDeterministicMNCPtr& dmn) {
        if (walletModel && ui->checkBoxMyMasternodesOnly->isChecked()) {
            bool fMyMasternode = setOutpts.count(dmn->collateralOutpoint) ||
                walletModel->wallet().isSpendable(dmn->pdmnState->keyIDOwner) ||
                walletModel->wallet().isSpendable(dmn->pdmnState->keyIDVoting) ||
                walletModel->wallet().isSpendable(dmn->pdmnState->scriptPayout) ||
                walletModel->wallet().isSpendable(dmn->pdmnState->scriptOperatorPayout);
            if (!fMyMasternode) return;
        }
        // populate list
        // Address, Protocol, Status, Active Seconds, Last Seen, Pub Key
        auto addr_key = dmn->pdmnState->addr.GetKey();
        QByteArray addr_ba(reinterpret_cast<const char*>(addr_key.data()), addr_key.size());
        QTableWidgetItem* addressItem = new CMasternodeListWidgetItem<QByteArray>(QString::fromStdString(dmn->pdmnState->addr.ToString()), addr_ba);
        QTableWidgetItem* statusItem = new QTableWidgetItem(mnList.IsMNValid(dmn) ? tr("ENABLED") : (mnList.IsMNPoSeBanned(dmn) ? tr("POSE_BANNED") : tr("UNKNOWN")));
        QTableWidgetItem* PoSeScoreItem = new CMasternodeListWidgetItem<int>(QString::number(dmn->pdmnState->nPoSePenalty), dmn->pdmnState->nPoSePenalty);
        QTableWidgetItem* registeredItem = new CMasternodeListWidgetItem<int>(QString::number(dmn->pdmnState->nRegisteredHeight), dmn->pdmnState->nRegisteredHeight);
        QTableWidgetItem* lastPaidItem = new CMasternodeListWidgetItem<int>(QString::number(dmn->pdmnState->nLastPaidHeight), dmn->pdmnState->nLastPaidHeight);

        QString strNextPayment = "UNKNOWN";
        int nNextPayment = 0;
        if (nextPayments.count(dmn->proTxHash)) {
            nNextPayment = nextPayments[dmn->proTxHash];
            strNextPayment = QString::number(nNextPayment);
        }
        QTableWidgetItem* nextPaymentItem = new CMasternodeListWidgetItem<int>(strNextPayment, nNextPayment);

        CTxDestination payeeDest;
        QString payeeStr = tr("UNKNOWN");
        if (ExtractDestination(dmn->pdmnState->scriptPayout, payeeDest)) {
            payeeStr = QString::fromStdString(EncodeDestination(payeeDest));
        }
        QTableWidgetItem* payeeItem = new QTableWidgetItem(payeeStr);

        QString operatorRewardStr = tr("NONE");
        if (dmn->nOperatorReward) {
            operatorRewardStr = QString::number(dmn->nOperatorReward / 100.0, 'f', 2) + "% ";

            if (dmn->pdmnState->scriptOperatorPayout != CScript()) {
                CTxDestination operatorDest;
                if (ExtractDestination(dmn->pdmnState->scriptOperatorPayout, operatorDest)) {
                    operatorRewardStr += tr("to %1").arg(QString::fromStdString(EncodeDestination(operatorDest)));
                } else {
                    operatorRewardStr += tr("to UNKNOWN");
                }
            } else {
                operatorRewardStr += tr("but not claimed");
            }
        }
        QTableWidgetItem* operatorRewardItem = new CMasternodeListWidgetItem<uint16_t>(operatorRewardStr, dmn->nOperatorReward);

        QString collateralStr = tr("UNKNOWN");
        auto collateralDestIt = mapCollateralDests.find(dmn->proTxHash);
        if (collateralDestIt != mapCollateralDests.end()) {
            collateralStr = QString::fromStdString(EncodeDestination(collateralDestIt->second));
        }
        QTableWidgetItem* collateralItem = new QTableWidgetItem(collateralStr);

        QString ownerStr = QString::fromStdString(EncodeDestination(dmn->pdmnState->keyIDOwner));
        QTableWidgetItem* ownerItem = new QTableWidgetItem(ownerStr);

        QString votingStr = QString::fromStdString(EncodeDestination(dmn->pdmnState->keyIDVoting));
        QTableWidgetItem* votingItem = new QTableWidgetItem(votingStr);

        QTableWidgetItem* proTxHashItem = new QTableWidgetItem(QString::fromStdString(dmn->proTxHash.ToString()));

        if (strCurrentFilterDIP3 != "") {
            strToFilter = addressItem->text() + " " +
                          statusItem->text() + " " +
                          PoSeScoreItem->text() + " " +
                          registeredItem->text() + " " +
                          lastPaidItem->text() + " " +
                          nextPaymentItem->text() + " " +
                          payeeItem->text() + " " +
                          operatorRewardItem->text() + " " +
                          collateralItem->text() + " " +
                          ownerItem->text() + " " +
                          votingItem->text() + " " +
                          proTxHashItem->text();
            if (!strToFilter.contains(strCurrentFilterDIP3)) return;
        }

        ui->tableWidgetMasternodesDIP3->insertRow(0);
        ui->tableWidgetMasternodesDIP3->setItem(0, COLUMN_SERVICE, addressItem);
        ui->tableWidgetMasternodesDIP3->setItem(0, COLUMN_STATUS, statusItem);
        ui->tableWidgetMasternodesDIP3->setItem(0, COLUMN_POSE, PoSeScoreItem);
        ui->tableWidgetMasternodesDIP3->setItem(0, COLUMN_REGISTERED, registeredItem);
        ui->tableWidgetMasternodesDIP3->setItem(0, COLUMN_LAST_PAYMENT, lastPaidItem);
        ui->tableWidgetMasternodesDIP3->setItem(0, COLUMN_NEXT_PAYMENT, nextPaymentItem);
        ui->tableWidgetMasternodesDIP3->setItem(0, COLUMN_PAYOUT_ADDRESS, payeeItem);
        ui->tableWidgetMasternodesDIP3->setItem(0, COLUMN_OPERATOR_REWARD, operatorRewardItem);
        ui->tableWidgetMasternodesDIP3->setItem(0, COLUMN_COLLATERAL_ADDRESS, collateralItem);
        ui->tableWidgetMasternodesDIP3->setItem(0, COLUMN_OWNER_ADDRESS, ownerItem);
        ui->tableWidgetMasternodesDIP3->setItem(0, COLUMN_VOTING_ADDRESS, votingItem);
        ui->tableWidgetMasternodesDIP3->setItem(0, COLUMN_PROTX_HASH, proTxHashItem);
    });

    ui->countLabelDIP3->setText(QString::number(ui->tableWidgetMasternodesDIP3->rowCount()));
    ui->tableWidgetMasternodesDIP3->setSortingEnabled(true);
}

void MasternodeList::on_filterLineEditDIP3_textChanged(const QString& strFilterIn)
{
    strCurrentFilterDIP3 = strFilterIn;
    nTimeFilterUpdatedDIP3 = GetTime();
    fFilterUpdatedDIP3 = true;
    ui->countLabelDIP3->setText(tr("Please wait...") + " " + QString::number(MASTERNODELIST_FILTER_COOLDOWN_SECONDS));
}

void MasternodeList::on_checkBoxMyMasternodesOnly_stateChanged(int state)
{
    // no cooldown
    nTimeFilterUpdatedDIP3 = GetTime() - MASTERNODELIST_FILTER_COOLDOWN_SECONDS;
    fFilterUpdatedDIP3 = true;
}

CDeterministicMNCPtr MasternodeList::GetSelectedDIP3MN()
{
    if (!clientModel) {
        return nullptr;
    }

    std::string strProTxHash;
    {
        LOCK(cs_dip3list);

        QItemSelectionModel* selectionModel = ui->tableWidgetMasternodesDIP3->selectionModel();
        QModelIndexList selected = selectionModel->selectedRows();

        if (selected.count() == 0) return nullptr;

        QModelIndex index = selected.at(0);
        int nSelectedRow = index.row();
        strProTxHash = ui->tableWidgetMasternodesDIP3->item(nSelectedRow, COLUMN_PROTX_HASH)->text().toStdString();
    }

    uint256 proTxHash;
    proTxHash.SetHex(strProTxHash);

    auto mnList = clientModel->getMasternodeList();
    return mnList.GetMN(proTxHash);
}

void MasternodeList::extraInfoDIP3_clicked()
{
    auto dmn = GetSelectedDIP3MN();
    if (!dmn) {
        return;
    }

    UniValue json(UniValue::VOBJ);
    dmn->ToJson(json);

    // Title of popup window
    QString strWindowtitle = tr("Additional information for DIP3 Masternode %1").arg(QString::fromStdString(dmn->proTxHash.ToString()));
    QString strText = QString::fromStdString(json.write(2));

    QMessageBox::information(this, strWindowtitle, strText);
}

void MasternodeList::copyProTxHash_clicked()
{
    auto dmn = GetSelectedDIP3MN();
    if (!dmn) {
        return;
    }

    QApplication::clipboard()->setText(QString::fromStdString(dmn->proTxHash.ToString()));
}

void MasternodeList::copyCollateralOutpoint_clicked()
{
    auto dmn = GetSelectedDIP3MN();
    if (!dmn) {
        return;
    }

    QApplication::clipboard()->setText(QString::fromStdString(dmn->collateralOutpoint.ToStringShort()));
}
