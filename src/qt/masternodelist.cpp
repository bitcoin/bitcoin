#include <qt/masternodelist.h>
#include <qt/forms/ui_masternodelist.h>

#include <evo/deterministicmns.h>
#include <masternode/activemasternode.h>
#include <qt/clientmodel.h>
#include <clientversion.h>
#include <coins.h>
#include <qt/guiutil.h>
#include <init.h>
#include <netbase.h>
#include <sync.h>
#include <validation.h>
#include <wallet/wallet.h>
#include <qt/walletmodel.h>

#include <univalue.h>

#include <QMessageBox>
#include <QTimer>
#include <QtGui/QClipboard>
#include <interfaces/node.h>
#include <node/context.h>
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

MasternodeList::MasternodeList(const PlatformStyle* platformStyle, QWidget* parent) :
    QWidget(parent),
    nTimeFilterUpdatedDIP3(0),
    nTimeUpdatedDIP3(0),
    fFilterUpdatedDIP3(true),
    ui(new Ui::MasternodeList),
    clientModel(0),
    walletModel(0),
    mnListChanged(true)
{
    ui->setupUi(this);

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

    ui->tableWidgetMasternodesDIP3->setColumnWidth(0, columnAddressWidth);
    ui->tableWidgetMasternodesDIP3->setColumnWidth(1, columnStatusWidth);
    ui->tableWidgetMasternodesDIP3->setColumnWidth(2, columnPoSeScoreWidth);
    ui->tableWidgetMasternodesDIP3->setColumnWidth(3, columnRegisteredWidth);
    ui->tableWidgetMasternodesDIP3->setColumnWidth(4, columnLastPaidWidth);
    ui->tableWidgetMasternodesDIP3->setColumnWidth(5, columnNextPaymentWidth);
    ui->tableWidgetMasternodesDIP3->setColumnWidth(6, columnPayeeWidth);
    ui->tableWidgetMasternodesDIP3->setColumnWidth(7, columnOperatorRewardWidth);
    ui->tableWidgetMasternodesDIP3->setColumnWidth(8, columnCollateralWidth);
    ui->tableWidgetMasternodesDIP3->setColumnWidth(9, columnOwnerWidth);
    ui->tableWidgetMasternodesDIP3->setColumnWidth(10, columnVotingWidth);

    // dummy column for proTxHash
    // TODO use a proper table model for the MN list
    ui->tableWidgetMasternodesDIP3->insertColumn(11);
    ui->tableWidgetMasternodesDIP3->setColumnHidden(11, true);

    ui->tableWidgetMasternodesDIP3->setContextMenuPolicy(Qt::CustomContextMenu);

    ui->filterLineEditDIP3->setPlaceholderText(tr("Filter by any property (e.g. address or protx hash)"));
    ui->checkBoxMyMasternodesOnly->setEnabled(false);

    QAction* copyProTxHashAction = new QAction(tr("Copy ProTx Hash"), this);
    QAction* copyCollateralOutpointAction = new QAction(tr("Copy Collateral Outpoint"), this);
    QAction* copyServiceAction = new QAction(tr("Copy Service"), this);
    QAction* copyPayoutAction = new QAction(tr("Copy Payout Address"), this);
    QAction* copyCollateralAction = new QAction(tr("Copy Collateral Address"), this);
    QAction* copyOwnerAction = new QAction(tr("Copy Owner Address"), this);
    QAction* copyVotingAction = new QAction(tr("Copy Voting Address"), this);
    contextMenuDIP3 = new QMenu();
    contextMenuDIP3->addAction(copyServiceAction);
    contextMenuDIP3->addAction(copyPayoutAction);
    contextMenuDIP3->addAction(copyCollateralAction);
    contextMenuDIP3->addAction(copyOwnerAction);
    contextMenuDIP3->addAction(copyVotingAction);
    contextMenuDIP3->addSeparator();
    contextMenuDIP3->addAction(copyProTxHashAction);
    contextMenuDIP3->addAction(copyCollateralOutpointAction);
    connect(ui->tableWidgetMasternodesDIP3, &QWidget::customContextMenuRequested, this, &MasternodeList::showContextMenuDIP3);
    connect(ui->tableWidgetMasternodesDIP3, &QTableView::doubleClicked, this, &MasternodeList::extraInfoDIP3_clicked);
    connect(copyProTxHashAction, &QAction::triggered, this, &MasternodeList::copyProTxHash_clicked);
    connect(copyCollateralOutpointAction, &QAction::triggered, this, &MasternodeList::copyCollateralOutpoint_clicked);
    connect(copyServiceAction, &QAction::triggered, this, &MasternodeList::copyService_clicked);
    connect(copyPayoutAction, &QAction::triggered, this, &MasternodeList::copyPayout_clicked);
    connect(copyCollateralAction, &QAction::triggered, this, &MasternodeList::copyCollateral_clicked);
    connect(copyOwnerAction, &QAction::triggered, this, &MasternodeList::copyOwner_clicked);
    connect(copyVotingAction, &QAction::triggered, this, &MasternodeList::copyVoting_clicked);

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MasternodeList::updateDIP3ListScheduled);
    timer->start(1000);
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
        connect(clientModel, &ClientModel::masternodeListChanged, this, &MasternodeList::handleMasternodeListChanged);
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
        ui->countLabelDIP3->setText(QString::fromStdString(strprintf("Please wait... %d", nSecondsToWait)));

        if (nSecondsToWait <= 0) {
            updateDIP3List();
            fFilterUpdatedDIP3 = false;
        }
    } else if (mnListChanged) {
        int64_t nMnListUpdateSecods = clientModel->masternodeSync().isBlockchainSynced() ? MASTERNODELIST_UPDATE_SECONDS : MASTERNODELIST_UPDATE_SECONDS*10;
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
        LOCK(cs_main);
        mnList.ForEachMN(false, [&](const CDeterministicMNCPtr& dmn) {
            CTxDestination collateralDest;
            Coin coin;
            if (GetUTXOCoin(*clientModel->node().context()->chainman, dmn->collateralOutpoint, coin) && ExtractDestination(coin.out.scriptPubKey, collateralDest)) {
                mapCollateralDests.try_emplace(dmn->proTxHash, collateralDest);
            }
        });
    }

    LOCK(cs_dip3list);

    QString strToFilter;
    ui->countLabelDIP3->setText("Updating...");
    ui->tableWidgetMasternodesDIP3->setSortingEnabled(false);
    ui->tableWidgetMasternodesDIP3->clearContents();
    ui->tableWidgetMasternodesDIP3->setRowCount(0);

    nTimeUpdatedDIP3 = GetTime();
    std::vector<CDeterministicMNCPtr> projectedPayees;
    mnList.GetProjectedMNPayees(mnList.GetValidMNsCount(), projectedPayees);
    std::map<uint256, int> nextPayments;
    for (size_t i = 0; i < projectedPayees.size(); i++) {
        const auto& dmn = projectedPayees[i];
        nextPayments.try_emplace(dmn->proTxHash, mnList.GetHeight() + (int)i + 1);
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
                walletModel->wallet().isSpendable(CTxDestination(WitnessV0KeyHash(dmn->pdmnState->keyIDOwner))) ||
                walletModel->wallet().isSpendable(CTxDestination(WitnessV0KeyHash(dmn->pdmnState->keyIDVoting))) ||
                walletModel->wallet().isSpendable(dmn->pdmnState->scriptPayout) ||
                walletModel->wallet().isSpendable(dmn->pdmnState->scriptOperatorPayout);
            if (!fMyMasternode) return;
        }
        // populate list
        // Address, Protocol, Status, Active Seconds, Last Seen, Pub Key
        QTableWidgetItem* addressItem = new QTableWidgetItem(QString::fromStdString(dmn->pdmnState->addr.ToString()));
        QTableWidgetItem* statusItem = new QTableWidgetItem(mnList.IsMNValid(dmn) ? tr("ENABLED") : (mnList.IsMNPoSeBanned(dmn) ? tr("POSE_BANNED") : tr("UNKNOWN")));
        QTableWidgetItem* PoSeScoreItem = new QTableWidgetItem(QString::number(dmn->pdmnState->nPoSePenalty));
        QTableWidgetItem* registeredItem = new QTableWidgetItem(QString::number(dmn->pdmnState->nRegisteredHeight));
        QTableWidgetItem* lastPaidItem = new QTableWidgetItem(QString::number(dmn->pdmnState->nLastPaidHeight));
        QTableWidgetItem* nextPaymentItem = new QTableWidgetItem(nextPayments.count(dmn->proTxHash) ? QString::number(nextPayments[dmn->proTxHash]) : tr("UNKNOWN"));

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
        QTableWidgetItem* operatorRewardItem = new QTableWidgetItem(operatorRewardStr);

        QString collateralStr = tr("UNKNOWN");
        auto collateralDestIt = mapCollateralDests.find(dmn->proTxHash);
        if (collateralDestIt != mapCollateralDests.end()) {
            collateralStr = QString::fromStdString(EncodeDestination(collateralDestIt->second));
        }
        QTableWidgetItem* collateralItem = new QTableWidgetItem(collateralStr);

        QString ownerStr = QString::fromStdString(EncodeDestination(WitnessV0KeyHash(dmn->pdmnState->keyIDOwner)));
        QTableWidgetItem* ownerItem = new QTableWidgetItem(ownerStr);

        QString votingStr = QString::fromStdString(EncodeDestination(WitnessV0KeyHash(dmn->pdmnState->keyIDVoting)));
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
        ui->tableWidgetMasternodesDIP3->setItem(0, 0, addressItem);
        ui->tableWidgetMasternodesDIP3->setItem(0, 1, statusItem);
        ui->tableWidgetMasternodesDIP3->setItem(0, 2, PoSeScoreItem);
        ui->tableWidgetMasternodesDIP3->setItem(0, 3, registeredItem);
        ui->tableWidgetMasternodesDIP3->setItem(0, 4, lastPaidItem);
        ui->tableWidgetMasternodesDIP3->setItem(0, 5, nextPaymentItem);
        ui->tableWidgetMasternodesDIP3->setItem(0, 6, payeeItem);
        ui->tableWidgetMasternodesDIP3->setItem(0, 7, operatorRewardItem);
        ui->tableWidgetMasternodesDIP3->setItem(0, 8, collateralItem);
        ui->tableWidgetMasternodesDIP3->setItem(0, 9, ownerItem);
        ui->tableWidgetMasternodesDIP3->setItem(0, 10, votingItem);
        ui->tableWidgetMasternodesDIP3->setItem(0, 11, proTxHashItem);
    });

    ui->countLabelDIP3->setText(QString::number(ui->tableWidgetMasternodesDIP3->rowCount()));
    ui->tableWidgetMasternodesDIP3->setSortingEnabled(true);
}

void MasternodeList::on_filterLineEditDIP3_textChanged(const QString& strFilterIn)
{
    strCurrentFilterDIP3 = strFilterIn;
    nTimeFilterUpdatedDIP3 = GetTime();
    fFilterUpdatedDIP3 = true;
    ui->countLabelDIP3->setText(QString::fromStdString(strprintf("Please wait... %d", MASTERNODELIST_FILTER_COOLDOWN_SECONDS)));
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
        strProTxHash = ui->tableWidgetMasternodesDIP3->item(nSelectedRow, 11)->text().toStdString();
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
    dmn->ToJson(*clientModel->node().context()->chainman, json);

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

void MasternodeList::copyService_clicked()
{
    auto dmn = GetSelectedDIP3MN();
    if (!dmn) {
        return;
    }
    QApplication::clipboard()->setText(QString::fromStdString(dmn->pdmnState->addr.ToString()));
}
void MasternodeList::copyPayout_clicked()
{
    auto dmn = GetSelectedDIP3MN();
    if (!dmn) {
        return;
    }
    QString operatorStr;
    CTxDestination operatorDest;
    if (ExtractDestination(dmn->pdmnState->scriptOperatorPayout, operatorDest)) {
        operatorStr = QString::fromStdString(EncodeDestination(operatorDest));
    } 
    
    QApplication::clipboard()->setText(operatorStr);
}
void MasternodeList::copyCollateral_clicked()
{
    auto dmn = GetSelectedDIP3MN();
    if (!dmn) {
        return;
    }
    auto mnList = clientModel->getMasternodeList();
    
    QString collateralStr;
    CTxDestination collateralDest;
    Coin coin;
    if (GetUTXOCoin(*clientModel->node().context()->chainman, dmn->collateralOutpoint, coin) && ExtractDestination(coin.out.scriptPubKey, collateralDest)) {
        collateralStr = QString::fromStdString(EncodeDestination(collateralDest));
    }
    QApplication::clipboard()->setText(collateralStr);
}
void MasternodeList::copyOwner_clicked()
{
    auto dmn = GetSelectedDIP3MN();
    if (!dmn) {
        return;
    }
    QString ownerStr = QString::fromStdString(EncodeDestination(WitnessV0KeyHash(dmn->pdmnState->keyIDOwner)));
    QApplication::clipboard()->setText(ownerStr);
}
void MasternodeList::copyVoting_clicked()
{
    auto dmn = GetSelectedDIP3MN();
    if (!dmn) {
        return;
    }
    QString votingStr = QString::fromStdString(EncodeDestination(WitnessV0KeyHash(dmn->pdmnState->keyIDVoting)));
    QApplication::clipboard()->setText(votingStr);
}