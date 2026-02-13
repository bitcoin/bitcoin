// Copyright (c) 2016-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/masternodelist.h>
#include <qt/forms/ui_masternodelist.h>

#include <coins.h>
#include <evo/deterministicmns.h>
#include <evo/dmn_types.h>
#include <saltedhasher.h>

#include <qt/clientmodel.h>
#include <qt/descriptiondialog.h>
#include <qt/guiutil.h>
#include <qt/guiutil_font.h>
#include <qt/walletmodel.h>

#include <QApplication>
#include <QClipboard>
#include <QHeaderView>
#include <QMetaObject>
#include <QThread>

#include <set>

namespace {
constexpr int MASTERNODELIST_UPDATE_SECONDS{3};
} // anonymous namespace

bool MasternodeListSortFilterProxyModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const
{
    // "Type" filter
    if (m_type_filter != TypeFilter::All) {
        QModelIndex idx = sourceModel()->index(source_row, MasternodeModel::TYPE, source_parent);
        int type = sourceModel()->data(idx, Qt::EditRole).toInt();
        if (m_type_filter == TypeFilter::Regular && type != static_cast<int>(MnType::Regular)) {
            return false;
        }
        if (m_type_filter == TypeFilter::Evo && type != static_cast<int>(MnType::Evo)) {
            return false;
        }
    }

    // Banned filter
    if (m_hide_banned) {
        QModelIndex idx = sourceModel()->index(source_row, MasternodeModel::STATUS, source_parent);
        int banned = sourceModel()->data(idx, Qt::EditRole).toInt();
        if (banned != 0) {
            return false;
        }
    }

    // Text-matching filter
    if (const auto& regex = filterRegularExpression(); !regex.pattern().isEmpty()) {
        QModelIndex idx = sourceModel()->index(source_row, 0, source_parent);
        QString searchText = sourceModel()->data(idx, Qt::UserRole).toString();
        if (!searchText.contains(regex)) {
            return false;
        }
    }

    // "Owned" filter
    if (m_show_owned_only) {
        QModelIndex idx = sourceModel()->index(source_row, MasternodeModel::PROTX_HASH, source_parent);
        QString proTxHash = sourceModel()->data(idx, Qt::DisplayRole).toString();
        if (!m_owned_mns.contains(proTxHash)) {
            return false;
        }
    }

    return true;
}

MasternodeList::MasternodeList(QWidget* parent) :
    QWidget(parent),
    ui(new Ui::MasternodeList),
    m_proxy_model(new MasternodeListSortFilterProxyModel(this)),
    m_model(new MasternodeModel(this)),
    m_worker(new QObject),
    m_thread{new QThread(this)},
    m_timer{new QTimer(this)}
{
    ui->setupUi(this);

    GUIUtil::setFont({ui->label_count, ui->countLabel}, {GUIUtil::FontWeight::Bold, 14});

    // Set up proxy model
    m_proxy_model->setSourceModel(m_model);
    m_proxy_model->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_proxy_model->setSortRole(Qt::EditRole);

    // Set up table view
    ui->tableViewMasternodes->setModel(m_proxy_model);
    ui->tableViewMasternodes->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->tableViewMasternodes->verticalHeader()->setVisible(false);

    // Set column widths
    auto* header = ui->tableViewMasternodes->horizontalHeader();
    header->setStretchLastSection(false);
    for (int col = 0; col < MasternodeModel::COUNT; ++col) {
        if (col == MasternodeModel::SERVICE) {
            header->setSectionResizeMode(col, QHeaderView::Stretch);
        } else {
            header->setSectionResizeMode(col, QHeaderView::ResizeToContents);
        }
    }

    // Hide ProTx Hash column (used for internal lookup)
    ui->tableViewMasternodes->setColumnHidden(MasternodeModel::PROTX_HASH, true);

    // Hide PoSe column by default (since "Hide banned" is checked by default)
    ui->tableViewMasternodes->setColumnHidden(MasternodeModel::POSE, true);

    ui->checkBoxOwned->setEnabled(false);

    contextMenuDIP3 = new QMenu(this);
    contextMenuDIP3->addAction(tr("Copy ProTx Hash"), this, &MasternodeList::copyProTxHash_clicked);
    contextMenuDIP3->addAction(tr("Copy Collateral Outpoint"), this, &MasternodeList::copyCollateralOutpoint_clicked);

    QMenu* filterMenu = contextMenuDIP3->addMenu(tr("Filter by"));
    filterMenu->addAction(tr("Collateral Address"), this, &MasternodeList::filterByCollateralAddress);
    filterMenu->addAction(tr("Payout Address"), this, &MasternodeList::filterByPayoutAddress);
    filterMenu->addAction(tr("Owner Address"), this, &MasternodeList::filterByOwnerAddress);
    filterMenu->addAction(tr("Voting Address"), this, &MasternodeList::filterByVotingAddress);

    connect(ui->tableViewMasternodes, &QTableView::customContextMenuRequested, this, &MasternodeList::showContextMenuDIP3);
    connect(ui->tableViewMasternodes, &QTableView::doubleClicked, this, &MasternodeList::extraInfoDIP3_clicked);
    connect(m_proxy_model, &QSortFilterProxyModel::rowsInserted, this, &MasternodeList::updateFilteredCount);
    connect(m_proxy_model, &QSortFilterProxyModel::rowsRemoved, this, &MasternodeList::updateFilteredCount);
    connect(m_proxy_model, &QSortFilterProxyModel::modelReset, this, &MasternodeList::updateFilteredCount);
    connect(m_proxy_model, &QSortFilterProxyModel::layoutChanged, this, &MasternodeList::updateFilteredCount);

    GUIUtil::updateFonts();

    // Background thread for calculating masternode list
    m_worker->moveToThread(m_thread);
    // Make sure executor object is deleted in its own thread
    connect(m_thread, &QThread::finished, m_worker, &QObject::deleteLater);
    m_thread->start();

    // Debounce timer to apply masternode list changes
    m_timer->setSingleShot(true);
    connect(m_timer, &QTimer::timeout, this, &MasternodeList::updateDIP3ListScheduled);
}

MasternodeList::~MasternodeList()
{
    m_timer->stop();
    m_thread->quit();
    m_thread->wait();
    delete ui;
}

void MasternodeList::changeEvent(QEvent* event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::StyleChange) {
        QTimer::singleShot(0, m_model, &MasternodeModel::refreshIcons);
    }
}

void MasternodeList::setClientModel(ClientModel* model)
{
    this->clientModel = model;
    if (model) {
        connect(clientModel, &ClientModel::masternodeListChanged, this, &MasternodeList::handleMasternodeListChanged);
        m_timer->start(0);
    } else {
        m_timer->stop();
    }
}

void MasternodeList::setWalletModel(WalletModel* model)
{
    this->walletModel = model;
    ui->checkBoxOwned->setEnabled(model != nullptr);
}

void MasternodeList::showContextMenuDIP3(const QPoint& point)
{
    QModelIndex index = ui->tableViewMasternodes->indexAt(point);
    if (index.isValid()) {
        contextMenuDIP3->exec(QCursor::pos());
    }
}

void MasternodeList::handleMasternodeListChanged()
{
    if (!clientModel || m_timer->isActive()) {
        // Too early or already processing, nothing to do
        return;
    }

    int delay{MASTERNODELIST_UPDATE_SECONDS * 1000};
    if (!clientModel->masternodeSync().isBlockchainSynced()) {
        // Currently syncing, reduce refreshes
        delay *= 10;
    }
    m_timer->start(delay);
}

void MasternodeList::updateDIP3ListScheduled()
{
    if (!clientModel || clientModel->node().shutdownRequested()) {
        return;
    }

    if (m_in_progress.exchange(true)) {
        // Already applying, re-arm for next attempt
        handleMasternodeListChanged();
        return;
    }

    QMetaObject::invokeMethod(m_worker, [this] {
        auto result = std::make_shared<CalcMnList>(calcMasternodeList());
        m_in_progress.store(false);
        QTimer::singleShot(0, this, [this, result] {
            if (result->m_valid) {
                setMasternodeList(std::move(*result));
            } else {
                // Something went wrong, try again
                handleMasternodeListChanged();
            }
        });
    });
}

MasternodeList::CalcMnList MasternodeList::calcMasternodeList() const
{
    CalcMnList ret;
    if (!clientModel || clientModel->node().shutdownRequested()) {
        return ret;
    }

    auto [mnList, pindex] = clientModel->getMasternodeList();
    if (!pindex) return ret;
    auto projectedPayees = mnList->getProjectedMNPayees(pindex);

    if (projectedPayees.empty() && mnList->getValidMNsCount() > 0) {
        // GetProjectedMNPayees failed to provide results for a list with valid mns.
        // Keep current list and let it try again later.
        return ret;
    }

    ret.m_list_height = mnList->getHeight();

    Uint256HashMap<CTxDestination> mapCollateralDests;
    mnList->forEachMN(/*only_valid=*/false, [&](const auto& dmn) {
        CTxDestination collateralDest;
        Coin coin;
        if (clientModel->node().getUnspentOutput(dmn.getCollateralOutpoint(), coin) &&
            ExtractDestination(coin.out.scriptPubKey, collateralDest)) {
            mapCollateralDests.emplace(dmn.getProTxHash(), collateralDest);
        }
    });

    Uint256HashMap<int> nextPayments;
    for (size_t i = 0; i < projectedPayees.size(); i++) {
        const auto& dmn = projectedPayees[i];
        nextPayments.emplace(dmn->getProTxHash(), ret.m_list_height + (int)i + 1);
    }

    mnList->forEachMN(/*only_valid=*/false, [&](const auto& dmn) {
        QString collateralStr = QObject::tr("UNKNOWN");
        auto collateralDestIt = mapCollateralDests.find(dmn.getProTxHash());
        if (collateralDestIt != mapCollateralDests.end()) {
            collateralStr = QString::fromStdString(EncodeDestination(collateralDestIt->second));
        }

        int nNextPayment = 0;
        auto nextPaymentIt = nextPayments.find(dmn.getProTxHash());
        if (nextPaymentIt != nextPayments.end()) {
            nNextPayment = nextPaymentIt->second;
        }

        ret.m_entries.push_back(std::make_unique<MasternodeEntry>(dmn, collateralStr, nNextPayment));
    });

    // Compute "owned" masternode hashes for the filter
    if (walletModel) {
        std::set<COutPoint> setOutpts;
        for (const auto& outpt : walletModel->wallet().listProTxCoins()) {
            setOutpts.emplace(outpt);
        }

        mnList->forEachMN(/*only_valid=*/false, [&](const auto& dmn) {
            bool fMyMasternode = setOutpts.count(dmn.getCollateralOutpoint()) ||
                                 walletModel->wallet().isSpendable(PKHash(dmn.getKeyIdOwner())) ||
                                 walletModel->wallet().isSpendable(PKHash(dmn.getKeyIdVoting())) ||
                                 walletModel->wallet().isSpendable(dmn.getScriptPayout()) ||
                                 walletModel->wallet().isSpendable(dmn.getScriptOperatorPayout());
            if (fMyMasternode) {
                ret.m_owned_mns.insert(QString::fromStdString(dmn.getProTxHash().ToString()));
            }
        });
    }

    ret.m_valid = true;
    return ret;
}

void MasternodeList::setMasternodeList(CalcMnList&& list)
{
    m_model->setCurrentHeight(list.m_list_height);
    m_model->reconcile(std::move(list.m_entries));

    if (walletModel) {
        m_proxy_model->setMyMasternodeHashes(std::move(list.m_owned_mns));
        if (ui->checkBoxOwned->isChecked()) {
            m_proxy_model->forceInvalidateFilter();
        }
    }

    updateFilteredCount();
}

void MasternodeList::updateFilteredCount()
{
    ui->countLabel->setText(QString::number(m_proxy_model->rowCount()));
}

void MasternodeList::on_filterText_textChanged(const QString& strFilterIn)
{
    m_proxy_model->setFilterRegularExpression(
        QRegularExpression(QRegularExpression::escape(strFilterIn), QRegularExpression::CaseInsensitiveOption));
    updateFilteredCount();
}

void MasternodeList::on_comboBoxType_currentIndexChanged(int index)
{
    if (index < 0 || index >= static_cast<int>(MasternodeListSortFilterProxyModel::TypeFilter::COUNT)) {
        return;
    }
    const auto index_enum{static_cast<MasternodeListSortFilterProxyModel::TypeFilter>(index)};
    ui->tableViewMasternodes->setColumnHidden(MasternodeModel::TYPE, index_enum != MasternodeListSortFilterProxyModel::TypeFilter::All);
    m_proxy_model->setTypeFilter(index_enum);
    m_proxy_model->forceInvalidateFilter();
    updateFilteredCount();
}

void MasternodeList::on_checkBoxOwned_stateChanged(int state)
{
    m_proxy_model->setShowOwnedOnly(state == Qt::Checked);
    m_proxy_model->forceInvalidateFilter();
    updateFilteredCount();
}

void MasternodeList::on_checkBoxHideBanned_stateChanged(int state)
{
    const bool hide_banned{state == Qt::Checked};
    ui->tableViewMasternodes->setColumnHidden(MasternodeModel::POSE, hide_banned);
    m_proxy_model->setHideBanned(hide_banned);
    m_proxy_model->forceInvalidateFilter();
    updateFilteredCount();
}

const MasternodeEntry* MasternodeList::GetSelectedEntry()
{
    if (!m_model) {
        return nullptr;
    }

    QItemSelectionModel* selectionModel = ui->tableViewMasternodes->selectionModel();
    if (!selectionModel) {
        return nullptr;
    }

    QModelIndexList selected = selectionModel->selectedRows();
    if (selected.count() == 0) {
        return nullptr;
    }

    // Map from proxy to source model
    return m_model->getEntryAt(m_proxy_model->mapToSource(selected.at(0)));
}

void MasternodeList::extraInfoDIP3_clicked()
{
    const auto* entry = GetSelectedEntry();
    if (!entry) {
        return;
    }

    auto* dialog = new DescriptionDialog(tr("Details for Masternode %1").arg(entry->proTxHash()), entry->toHtml(), /*parent=*/this);
    dialog->resize(1000, 500);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
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

void MasternodeList::filterByCollateralAddress()
{
    const auto* entry = GetSelectedEntry();
    if (entry) {
        ui->filterText->setText(entry->collateralAddress());
    }
}

void MasternodeList::filterByPayoutAddress()
{
    const auto* entry = GetSelectedEntry();
    if (entry) {
        ui->filterText->setText(entry->payoutAddress());
    }
}

void MasternodeList::filterByOwnerAddress()
{
    const auto* entry = GetSelectedEntry();
    if (entry) {
        ui->filterText->setText(entry->ownerAddress());
    }
}

void MasternodeList::filterByVotingAddress()
{
    const auto* entry = GetSelectedEntry();
    if (entry) {
        ui->filterText->setText(entry->votingAddress());
    }
}
