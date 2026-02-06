// Copyright (c) 2016-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/masternodelist.h>
#include <qt/forms/ui_masternodelist.h>

#include <coins.h>
#include <evo/deterministicmns.h>
#include <qt/clientmodel.h>
#include <qt/guiutil.h>
#include <qt/guiutil_font.h>
#include <qt/walletmodel.h>

#include <QApplication>
#include <QClipboard>
#include <QMessageBox>
#include <QTableWidgetItem>

template <typename T>
class CMasternodeListWidgetItem : public QTableWidgetItem
{
    T itemData;

public:
    explicit CMasternodeListWidgetItem(const QString& text, const T& data, int type = Type) :
        QTableWidgetItem(text, type),
        itemData(data) {}

    bool operator<(const QTableWidgetItem& other) const override
    {
        return itemData < ((CMasternodeListWidgetItem*)&other)->itemData;
    }
};

int MasternodeList::columnWidth(int column)
{
    switch (column) {
    case Column::SERVICE:
        return 200;
    case Column::TYPE:
        return 160;
    case Column::STATUS:
    case Column::POSE:
    case Column::REGISTERED:
    case Column::LAST_PAYMENT:
        return 80;
    case Column::NEXT_PAYMENT:
        return 100;
    case Column::PAYOUT_ADDRESS:
    case Column::OPERATOR_REWARD:
    case Column::COLLATERAL_ADDRESS:
    case Column::OWNER_ADDRESS:
    case Column::VOTING_ADDRESS:
        return 130;
    case Column::PROTX_HASH:
    default:
        return 80;
    }
}

MasternodeList::MasternodeList(QWidget* parent) :
    QWidget(parent),
    ui(new Ui::MasternodeList)
{
    ui->setupUi(this);

    GUIUtil::setFont({ui->label_count_2, ui->countLabelDIP3}, {GUIUtil::FontWeight::Bold, 14});
    GUIUtil::setFont({ui->label_filter_2}, {GUIUtil::FontWeight::Normal, 15});

    for (int col = 0; col <= Column::COUNT; col++) {
        ui->tableWidgetMasternodesDIP3->setColumnWidth(col, columnWidth(col));
    }

    // dummy column for proTxHash
    ui->tableWidgetMasternodesDIP3->insertColumn(Column::PROTX_HASH);
    ui->tableWidgetMasternodesDIP3->setColumnHidden(Column::PROTX_HASH, true);

    ui->tableWidgetMasternodesDIP3->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->tableWidgetMasternodesDIP3->verticalHeader()->setVisible(false);

    ui->checkBoxMyMasternodesOnly->setEnabled(false);

    contextMenuDIP3 = new QMenu(this);
    contextMenuDIP3->addAction(tr("Copy ProTx Hash"), this, &MasternodeList::copyProTxHash_clicked);
    contextMenuDIP3->addAction(tr("Copy Collateral Outpoint"), this, &MasternodeList::copyCollateralOutpoint_clicked);

    connect(ui->tableWidgetMasternodesDIP3, &QTableWidget::customContextMenuRequested, this, &MasternodeList::showContextMenuDIP3);
    connect(ui->tableWidgetMasternodesDIP3, &QTableWidget::doubleClicked, this, &MasternodeList::extraInfoDIP3_clicked);

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MasternodeList::updateDIP3ListScheduled);
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
        ui->countLabelDIP3->setText(tr("Please wait…") + " " + QString::number(nSecondsToWait));

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

    auto [mnList, pindex] = clientModel->getMasternodeList();
    if (!pindex) return;
    auto projectedPayees = mnList->getProjectedMNPayees(pindex);

    if (projectedPayees.empty() && mnList->getValidMNsCount() > 0) {
        // GetProjectedMNPayees failed to provide results for a list with valid mns.
        // Keep current list and let it try again later.
        return;
    }

    std::map<uint256, CTxDestination> mapCollateralDests;

    {
        // Get all UTXOs for each MN collateral in one go so that we can reduce locking overhead for cs_main
        // We also do this outside of the below Qt list update loop to reduce cs_main locking time to a minimum
        mnList->forEachMN(/*only_valid=*/false, [&](const auto& dmn) {
            CTxDestination collateralDest;
            Coin coin;
            if (clientModel->node().getUnspentOutput(dmn.getCollateralOutpoint(), coin) &&
                ExtractDestination(coin.out.scriptPubKey, collateralDest)) {
                mapCollateralDests.emplace(dmn.getProTxHash(), collateralDest);
            }
        });
    }

    LOCK(cs_dip3list);

    ui->countLabelDIP3->setText(tr("Updating…"));
    ui->tableWidgetMasternodesDIP3->setSortingEnabled(false);
    ui->tableWidgetMasternodesDIP3->clearContents();
    ui->tableWidgetMasternodesDIP3->setRowCount(0);

    nTimeUpdatedDIP3 = GetTime();

    std::map<uint256, int> nextPayments;
    for (size_t i = 0; i < projectedPayees.size(); i++) {
        const auto& dmn = projectedPayees[i];
        nextPayments.emplace(dmn->getProTxHash(), mnList->getHeight() + (int)i + 1);
    }

    std::set<COutPoint> setOutpts;
    if (walletModel && ui->checkBoxMyMasternodesOnly->isChecked()) {
        for (const auto& outpt : walletModel->wallet().listProTxCoins()) {
            setOutpts.emplace(outpt);
        }
    }

    m_entries.clear();
    mnList->forEachMN(/*only_valid=*/false, [&](const auto& dmn) {
        if (walletModel && ui->checkBoxMyMasternodesOnly->isChecked()) {
            bool fMyMasternode = setOutpts.count(dmn.getCollateralOutpoint()) ||
                                 walletModel->wallet().isSpendable(PKHash(dmn.getKeyIdOwner())) ||
                                 walletModel->wallet().isSpendable(PKHash(dmn.getKeyIdVoting())) ||
                                 walletModel->wallet().isSpendable(dmn.getScriptPayout()) ||
                                 walletModel->wallet().isSpendable(dmn.getScriptOperatorPayout());
            if (!fMyMasternode) return;
        }

        if (auto base_entry = mnList->getMN(dmn.getProTxHash())) {
            int nNextPayment = 0;
            if (auto it = nextPayments.find(dmn.getProTxHash()); it != nextPayments.end()) {
                nNextPayment = it->second;
            }

            QString collateralStr = tr("UNKNOWN");
            if (auto it = mapCollateralDests.find(dmn.getProTxHash()); it != mapCollateralDests.end()) {
                collateralStr = QString::fromStdString(EncodeDestination(it->second));
            }

            m_entries.push_back(std::make_unique<MasternodeEntry>(*base_entry, collateralStr, nNextPayment));
        }
    });

    for (const auto& entry : m_entries) {
        if (!strCurrentFilterDIP3.isEmpty()) {
            QString strToFilter = entry->service() + " " +
                                  entry->typeDescription() + " " +
                                  (entry->isBanned() ? tr("POSE_BANNED") : tr("ENABLED")) + " " +
                                  QString::number(entry->posePenalty()) + " " +
                                  QString::number(entry->registeredHeight()) + " " +
                                  QString::number(entry->lastPaidHeight()) + " " +
                                  (entry->nextPaymentHeight() > 0 ? QString::number(entry->nextPaymentHeight()) : tr("UNKNOWN")) + " " +
                                  entry->payoutAddress() + " " +
                                  entry->operatorReward() + " " +
                                  entry->collateralAddress() + " " +
                                  entry->ownerAddress() + " " +
                                  entry->votingAddress() + " " +
                                  entry->proTxHash();
            if (!strToFilter.contains(strCurrentFilterDIP3)) continue;
        }

        int row = ui->tableWidgetMasternodesDIP3->rowCount();
        ui->tableWidgetMasternodesDIP3->insertRow(row);

        auto* addressItem = new CMasternodeListWidgetItem<QByteArray>(entry->service(), entry->serviceKey());
        auto* typeItem = new QTableWidgetItem(entry->typeDescription());
        auto* statusItem = new QTableWidgetItem(entry->isBanned() ? tr("POSE_BANNED") : tr("ENABLED"));
        auto* PoSeScoreItem = new CMasternodeListWidgetItem<int>(QString::number(entry->posePenalty()), entry->posePenalty());
        auto* registeredItem = new CMasternodeListWidgetItem<int>(QString::number(entry->registeredHeight()), entry->registeredHeight());
        auto* lastPaidItem = new CMasternodeListWidgetItem<int>(QString::number(entry->lastPaidHeight()), entry->lastPaidHeight());
        QString nextPaymentStr = entry->nextPaymentHeight() > 0 ? QString::number(entry->nextPaymentHeight()) : tr("UNKNOWN");
        auto* nextPaymentItem = new CMasternodeListWidgetItem<int>(nextPaymentStr, entry->nextPaymentHeight());
        auto* payeeItem = new QTableWidgetItem(entry->payoutAddress());
        auto* operatorRewardItem = new CMasternodeListWidgetItem<uint16_t>(entry->operatorReward(), entry->operatorRewardPct());
        auto* collateralItem = new QTableWidgetItem(entry->collateralAddress());
        auto* ownerItem = new QTableWidgetItem(entry->ownerAddress());
        auto* votingItem = new QTableWidgetItem(entry->votingAddress());
        auto* proTxHashItem = new QTableWidgetItem(entry->proTxHash());

        ui->tableWidgetMasternodesDIP3->setItem(row, Column::SERVICE, addressItem);
        ui->tableWidgetMasternodesDIP3->setItem(row, Column::TYPE, typeItem);
        ui->tableWidgetMasternodesDIP3->setItem(row, Column::STATUS, statusItem);
        ui->tableWidgetMasternodesDIP3->setItem(row, Column::POSE, PoSeScoreItem);
        ui->tableWidgetMasternodesDIP3->setItem(row, Column::REGISTERED, registeredItem);
        ui->tableWidgetMasternodesDIP3->setItem(row, Column::LAST_PAYMENT, lastPaidItem);
        ui->tableWidgetMasternodesDIP3->setItem(row, Column::NEXT_PAYMENT, nextPaymentItem);
        ui->tableWidgetMasternodesDIP3->setItem(row, Column::PAYOUT_ADDRESS, payeeItem);
        ui->tableWidgetMasternodesDIP3->setItem(row, Column::OPERATOR_REWARD, operatorRewardItem);
        ui->tableWidgetMasternodesDIP3->setItem(row, Column::COLLATERAL_ADDRESS, collateralItem);
        ui->tableWidgetMasternodesDIP3->setItem(row, Column::OWNER_ADDRESS, ownerItem);
        ui->tableWidgetMasternodesDIP3->setItem(row, Column::VOTING_ADDRESS, votingItem);
        ui->tableWidgetMasternodesDIP3->setItem(row, Column::PROTX_HASH, proTxHashItem);
    }

    ui->countLabelDIP3->setText(QString::number(ui->tableWidgetMasternodesDIP3->rowCount()));
    ui->tableWidgetMasternodesDIP3->setSortingEnabled(true);
}

void MasternodeList::on_filterLineEditDIP3_textChanged(const QString& strFilterIn)
{
    strCurrentFilterDIP3 = strFilterIn;
    nTimeFilterUpdatedDIP3 = GetTime();
    fFilterUpdatedDIP3 = true;
    ui->countLabelDIP3->setText(tr("Please wait…") + " " + QString::number(MASTERNODELIST_FILTER_COOLDOWN_SECONDS));
}

void MasternodeList::on_checkBoxMyMasternodesOnly_stateChanged(int state)
{
    // no cooldown
    nTimeFilterUpdatedDIP3 = GetTime() - MASTERNODELIST_FILTER_COOLDOWN_SECONDS;
    fFilterUpdatedDIP3 = true;
}

const MasternodeEntry* MasternodeList::GetSelectedEntry()
{
    LOCK(cs_dip3list);

    QItemSelectionModel* selectionModel = ui->tableWidgetMasternodesDIP3->selectionModel();
    QModelIndexList selected = selectionModel->selectedRows();

    if (selected.count() == 0) return nullptr;

    QModelIndex index = selected.at(0);
    int nSelectedRow = index.row();
    QString strProTxHash = ui->tableWidgetMasternodesDIP3->item(nSelectedRow, Column::PROTX_HASH)->text();

    for (const auto& entry : m_entries) {
        if (entry->proTxHash() == strProTxHash) {
            return entry.get();
        }
    }
    return nullptr;
}

void MasternodeList::extraInfoDIP3_clicked()
{
    const auto* entry = GetSelectedEntry();
    if (!entry) {
        return;
    }

    // Title of popup window
    QString strWindowtitle = tr("Additional information for DIP3 Masternode %1").arg(entry->proTxHash());
    QString strText = entry->toJson();

    QMessageBox::information(this, strWindowtitle, strText);
}

void MasternodeList::copyProTxHash_clicked()
{
    const auto* entry = GetSelectedEntry();
    if (!entry) {
        return;
    }

    QApplication::clipboard()->setText(entry->proTxHash());
}

void MasternodeList::copyCollateralOutpoint_clicked()
{
    const auto* entry = GetSelectedEntry();
    if (!entry) {
        return;
    }

    QApplication::clipboard()->setText(entry->collateralOutpoint());
}
