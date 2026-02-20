// Copyright (c) 2016-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/masternodelist.h>
#include <qt/forms/ui_masternodelist.h>

#include <coins.h>
#include <evo/deterministicmns.h>
#include <evo/dmn_types.h>
#include <saltedhasher.h>

#include <qt/clientfeeds.h>
#include <qt/clientmodel.h>
#include <qt/descriptiondialog.h>
#include <qt/guiutil.h>
#include <qt/guiutil_font.h>
#include <qt/walletmodel.h>

#include <QApplication>
#include <QClipboard>
#include <QDebug>
#include <QHeaderView>
#include <QMetaObject>
#include <QSettings>
#include <QThread>

#include <set>

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
        int status_value = sourceModel()->data(idx, Qt::EditRole).toInt();
        if (status_value > 0) {
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

bool MasternodeListSortFilterProxyModel::lessThan(const QModelIndex& lhs, const QModelIndex& rhs) const
{
    if (lhs.column() == MasternodeModel::SERVICE) {
        QVariant lhs_data{sourceModel()->data(lhs, sortRole())};
        QVariant rhs_data{sourceModel()->data(rhs, sortRole())};
        if (lhs_data.userType() == QMetaType::QByteArray && rhs_data.userType() == QMetaType::QByteArray) {
            return lhs_data.toByteArray() < rhs_data.toByteArray();
        }
    }
    return QSortFilterProxyModel::lessThan(lhs, rhs);
}

MasternodeList::MasternodeList(QWidget* parent) :
    QWidget(parent),
    ui(new Ui::MasternodeList),
    m_proxy_model(new MasternodeListSortFilterProxyModel(this)),
    m_model(new MasternodeModel(this))
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

    // Load filter settings
    QSettings settings;
    ui->checkBoxHideBanned->setChecked(settings.value("mnListHideBanned", false).toBool());
    ui->comboBoxType->setCurrentIndex(settings.value("mnListTypeFilter", 0).toInt());
    ui->filterText->setText(settings.value("mnListFilterText", "").toString());
}

MasternodeList::~MasternodeList()
{
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
    if (!clientModel) {
        return;
    }
    m_feed = clientModel->feedMasternode();
    if (m_feed) {
        connect(m_feed, &MasternodeFeed::dataReady, this, &MasternodeList::updateMasternodeList);
        updateMasternodeList();
    }
}

void MasternodeList::setWalletModel(WalletModel* model)
{
    this->walletModel = model;
    ui->checkBoxOwned->setEnabled(walletModel != nullptr);
    if (walletModel) {
        QSettings settings;
        ui->checkBoxOwned->setChecked(settings.value("mnListOwnedOnly", false).toBool());
    }
}

void MasternodeList::showContextMenuDIP3(const QPoint& point)
{
    QModelIndex index = ui->tableViewMasternodes->indexAt(point);
    if (index.isValid()) {
        contextMenuDIP3->exec(QCursor::pos());
    }
}

void MasternodeList::updateMasternodeList()
{
    if (!clientModel || !m_feed) {
        return;
    }

    auto feed = m_feed->data();
    if (!feed) {
        return;
    }

    if (!feed->m_valid) {
        qWarning() << "MasternodeList: fetch returned invalid data, scheduling retry";
        m_feed->requestRefresh();
        return;
    }

    MasternodeData ret;
    ret.m_list_height = feed->m_list_height;
    ret.m_entries = feed->m_entries;
    ret.m_valid = feed->m_valid;

    // If we don't have a wallet, nothing else to do...
    if (!walletModel) {
        setMasternodeList(std::move(ret), {});
        return;
    }

    std::set<COutPoint> setOutpts;
    for (const auto& outpt : walletModel->wallet().listProTxCoins()) {
        setOutpts.emplace(outpt);
    }

    QSet<QString> owned_mns;
    const auto [dmn, pindex] = clientModel->getMasternodeList();
    if (dmn && pindex) {
        dmn->forEachMN(/*only_valid=*/false, [&](const auto& dmn) {
            bool fMyMasternode = setOutpts.count(dmn.getCollateralOutpoint()) ||
                                walletModel->wallet().isSpendable(PKHash(dmn.getKeyIdOwner())) ||
                                walletModel->wallet().isSpendable(PKHash(dmn.getKeyIdVoting())) ||
                                walletModel->wallet().isSpendable(dmn.getScriptPayout()) ||
                                walletModel->wallet().isSpendable(dmn.getScriptOperatorPayout());
            if (fMyMasternode) {
                owned_mns.insert(QString::fromStdString(dmn.getProTxHash().ToString()));
            }
        });
    }
    setMasternodeList(std::move(ret), std::move(owned_mns));
}

void MasternodeList::setMasternodeList(MasternodeData&& list, QSet<QString>&& owned_mns)
{
    m_model->setCurrentHeight(list.m_list_height);
    m_model->reconcile(std::move(list.m_entries));

    if (walletModel) {
        m_proxy_model->setMyMasternodeHashes(std::move(owned_mns));
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

    QSettings settings;
    settings.setValue("mnListFilterText", strFilterIn);
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

    QSettings settings;
    settings.setValue("mnListTypeFilter", index);
}

void MasternodeList::on_checkBoxOwned_stateChanged(int state)
{
    m_proxy_model->setShowOwnedOnly(state == Qt::Checked);
    m_proxy_model->forceInvalidateFilter();
    updateFilteredCount();

    QSettings settings;
    settings.setValue("mnListOwnedOnly", state == Qt::Checked);
}

void MasternodeList::on_checkBoxHideBanned_stateChanged(int state)
{
    const bool hide_banned{state == Qt::Checked};
    ui->tableViewMasternodes->setColumnHidden(MasternodeModel::POSE, hide_banned);
    m_proxy_model->setHideBanned(hide_banned);
    m_proxy_model->forceInvalidateFilter();
    updateFilteredCount();

    QSettings settings;
    settings.setValue("mnListHideBanned", hide_banned);
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
