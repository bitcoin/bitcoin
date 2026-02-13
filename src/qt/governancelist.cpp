// Copyright (c) 2021-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/forms/ui_governancelist.h>

#include <evo/deterministicmns.h>
#include <governance/common.h>
#include <governance/governance.h>
#include <governance/vote.h>
#include <util/underlying.h>

#include <qt/descriptiondialog.h>
#include <qt/governancelist.h>
#include <qt/guiutil_font.h>
#include <qt/proposalcreate.h>
#include <qt/proposalmodel.h>
#include <qt/proposalresume.h>

#include <chainparams.h>
#include <interfaces/node.h>
#include <interfaces/wallet.h>
#include <script/standard.h>
#include <util/strencodings.h>
#include <util/time.h>

#include <qt/bitcoinunits.h>
#include <qt/clientmodel.h>
#include <qt/guiutil.h>
#include <qt/optionsmodel.h>
#include <qt/walletmodel.h>

#include <QAbstractItemView>
#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QEvent>
#include <QMessageBox>
#include <QMetaObject>
#include <QResizeEvent>
#include <QShowEvent>
#include <QUrl>

#include <univalue.h>

namespace {
constexpr int TITLE_MIN_WIDTH{220};
} // anonymous namespace

//
// Governance Tab main widget.
//
GovernanceList::GovernanceList(QWidget* parent) :
    QWidget(parent),
    ui{new Ui::GovernanceList},
    proposalModel{new ProposalModel(this)},
    proposalContextMenu{new QMenu(this)},
    m_worker(new QObject),
    proposalModelProxy{new QSortFilterProxyModel(this)},
    m_thread{new QThread(this)},
    m_timer{new QTimer(this)}
{
    ui->setupUi(this);

    GUIUtil::setFont({ui->label_count_2, ui->countLabel, ui->label_mn_count, ui->mnCountLabel},
                     {GUIUtil::FontWeight::Bold, 14});

    ui->govTableView->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->govTableView->setModel(proposalModelProxy);
    ui->govTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->govTableView->setSortingEnabled(true);
    ui->govTableView->sortByColumn(ProposalModel::Column::TITLE, Qt::AscendingOrder);
    ui->govTableView->verticalHeader()->setVisible(false);
    connect(ui->govTableView, &QTableView::customContextMenuRequested, this, &GovernanceList::showProposalContextMenu);
    connect(ui->govTableView, &QTableView::doubleClicked, this, &GovernanceList::showAdditionalInfo);
    connect(ui->govTableView->horizontalHeader(), &QHeaderView::sectionResized, this, &GovernanceList::refreshColumnWidths);

    ui->emptyPage->setAutoFillBackground(true);
    updateEmptyPagePalette();

    ui->proposalSourceCombo->addItem(tr("Active Proposals"), ToUnderlying(ProposalSource::Active));
    ui->proposalSourceCombo->setMinimumWidth(250);
    connect(ui->proposalSourceCombo, qOverload<int>(&QComboBox::activated), this, &GovernanceList::setProposalSource);

    // Set up filtering.
    proposalModelProxy->setSourceModel(proposalModel);
    proposalModelProxy->setSortRole(Qt::EditRole);
    proposalModelProxy->setFilterKeyColumn(ProposalModel::Column::TITLE); // filter by title column...
    connect(ui->filterLineEdit, &QLineEdit::textChanged, proposalModelProxy, &QSortFilterProxyModel::setFilterFixedString);

    // Changes to number of rows should update proposal count display.
    connect(proposalModelProxy, &QSortFilterProxyModel::rowsInserted, this, &GovernanceList::updateProposalCount);
    connect(proposalModelProxy, &QSortFilterProxyModel::rowsRemoved, this, &GovernanceList::updateProposalCount);
    connect(proposalModelProxy, &QSortFilterProxyModel::layoutChanged, this, &GovernanceList::updateProposalCount);

    // Connect buttons
    connect(ui->btnCreateProposal, &QPushButton::clicked, this, &GovernanceList::showCreateProposalDialog);
    connect(ui->btnResumeProposal, &QPushButton::clicked, this, &GovernanceList::showResumeProposalDialog);
    updateProposalButtons();

    // Initialize masternode count to 0
    ui->mnCountLabel->setText("0");

    GUIUtil::updateFonts();

    // Background thread for calculating proposal list
    m_worker->moveToThread(m_thread);
    // Make sure executor object is deleted in its own thread
    connect(m_thread, &QThread::finished, m_worker, &QObject::deleteLater);
    m_thread->start();

    // Debounce timer to apply proposal list changes
    m_timer->setSingleShot(true);
    connect(m_timer, &QTimer::timeout, this, &GovernanceList::updateProposalList);
}

GovernanceList::~GovernanceList()
{
    m_timer->stop();
    m_thread->quit();
    m_thread->wait();
    delete ui;
}

void GovernanceList::changeEvent(QEvent* event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::StyleChange) {
        QTimer::singleShot(0, proposalModel, &ProposalModel::refreshIcons);
        QTimer::singleShot(0, this, &GovernanceList::updateEmptyPagePalette);
    }
}

void GovernanceList::updateEmptyPagePalette()
{
    QPalette emptyPalette = ui->emptyPage->palette();
    emptyPalette.setColor(QPalette::Window, ui->govTableView->palette().color(QPalette::Base));
    ui->emptyPage->setPalette(emptyPalette);
}

void GovernanceList::setClientModel(ClientModel* model)
{
    this->clientModel = model;
    if (!clientModel) {
        m_timer->stop();
        return;
    }
    connect(clientModel, &ClientModel::additionalDataSyncProgressChanged, this, &GovernanceList::updateProposalButtons);
    connect(clientModel, &ClientModel::governanceChanged, this, [this] { handleProposalListChanged(/*force=*/false); });
    connect(clientModel, &ClientModel::numBlocksChanged, this, [this] { handleProposalListChanged(/*force=*/false); });
    connect(clientModel->getOptionsModel(), &OptionsModel::displayUnitChanged, this, &GovernanceList::updateDisplayUnit);
    if (walletModel && ui->proposalSourceCombo->findData(ToUnderlying(ProposalSource::Local)) == -1) {
        ui->proposalSourceCombo->addItem(tr("My Proposals"), ToUnderlying(ProposalSource::Local));
    }
    m_timer->start(0);
}

std::vector<Governance::Object> GovernanceList::getWalletProposals(std::optional<bool> pending) const
{
    if (!clientModel || !walletModel) {
        return {};
    }

    const auto wallet_objs = walletModel->wallet().getGovernanceObjects();

    const int64_t now{GetTime()};
    std::vector<Governance::Object> result{};
    for (const auto& obj : wallet_objs) {
        const bool is_broadcast{clientModel->node().gov().existsObj(obj.GetHash())};
        bool is_stale{false};
        try {
            UniValue json_data;
            json_data.read(obj.GetDataAsPlainString());
            if (json_data.exists("type") && json_data["type"].getInt<int>() != 1) {
                // Not a voting proposal
                continue;
            }
            if (json_data.exists("end_epoch")) {
                is_stale = json_data["end_epoch"].getInt<int64_t>() < now;
            }
        } catch (const std::exception&) {
            // Unparsable objects are skipped
            continue;
        }
        if (!pending.has_value() || *pending != (is_broadcast || is_stale)) {
            result.push_back(obj);
        }
    }

    return result;
}

void GovernanceList::setWalletModel(WalletModel* model)
{
    this->walletModel = model;
    if (!walletModel || !clientModel) {
        m_timer->stop();
        return;
    }
    connect(walletModel, &WalletModel::balanceChanged, this, &GovernanceList::updateProposalButtons);
    if (clientModel && ui->proposalSourceCombo->findData(ToUnderlying(ProposalSource::Local)) == -1) {
        ui->proposalSourceCombo->addItem(tr("My Proposals"), ToUnderlying(ProposalSource::Local));
    }
    m_timer->start(0);
}

void GovernanceList::updateDisplayUnit()
{
    if (this->clientModel) {
        proposalModel->setDisplayUnit(this->clientModel->getOptionsModel()->getDisplayUnit());
        ui->govTableView->update();
    }
}

void GovernanceList::handleProposalListChanged(bool force)
{
    if (!clientModel) {
        return;
    }
    if (force) {
        updateProposalList();
    } else if (!m_timer->isActive()) {
        int delay{GOVERNANCELIST_UPDATE_SECONDS * 1000};
        if (!clientModel->masternodeSync().isBlockchainSynced()) {
            // Currently syncing, reduce refreshes
            delay *= 6;
        }
        m_timer->start(delay);
    }
}

int GovernanceList::queryCollateralDepth(const uint256& collateralHash) const
{
    if (!walletModel) return 0;
    interfaces::WalletTxStatus tx_status;
    int num_blocks{};
    int64_t block_time{};
    if (walletModel->wallet().tryGetTxStatus(collateralHash, tx_status, num_blocks, block_time)) {
        return tx_status.depth_in_main_chain;
    }
    return 0;
}

void GovernanceList::updateProposalList()
{
    if (!clientModel || clientModel->node().shutdownRequested()) {
        return;
    }

    if (m_in_progress.exchange(true)) {
        // Already applying, re-arm for next attempt
        handleProposalListChanged(/*force=*/false);
        return;
    }

    QMetaObject::invokeMethod(m_worker, [this] {
        auto result = std::make_shared<CalcProposalList>(calcProposalList());
        m_in_progress.store(false);
        QTimer::singleShot(0, this, [this, result] {
            setProposalList(std::move(*result));
        });
    });
}

GovernanceList::CalcProposalList GovernanceList::calcProposalList() const
{
    CalcProposalList ret;
    if (!clientModel || clientModel->node().shutdownRequested()) {
        return ret;
    }

    const auto [dmn, pindex] = clientModel->getMasternodeList();
    if (!dmn || !pindex) {
        return ret;
    }

    // A proposal is considered passing if (YES votes - NO votes) >= (Total Weight of Masternodes / 10),
    // count total valid (ENABLED) masternodes to determine passing threshold.
    // Need to query number of masternodes here with access to clientModel.
    const int nWeightedMnCount = dmn->getValidWeightedMNsCount();
    ret.m_abs_vote_req = std::max(Params().GetConsensus().nGovernanceMinQuorum, nWeightedMnCount / 10);
    ret.m_gov_info = clientModel->node().gov().getGovernanceInfo();
    if (m_proposal_source == ProposalSource::Active) {
        std::vector<CGovernanceObject> govObjList;
        clientModel->getAllGovernanceObjects(govObjList);
        for (const auto& govObj : govObjList) {
            if (govObj.GetObjectType() != GovernanceObject::PROPOSAL) {
                continue; // Skip triggers.
            }
            ret.m_proposals.emplace_back(std::make_unique<Proposal>(this->clientModel, govObj, ret.m_gov_info, ret.m_gov_info.requiredConfs,
                                                                    /*is_broadcast=*/true));
        }
        // Include unrelayed wallet proposals (0 confs, not yet broadcast)
        for (const auto& obj : getWalletProposals(/*pending=*/true)) {
            CGovernanceObject govObj(obj.hashParent, obj.revision, obj.time, obj.collateralHash, obj.GetDataAsHexString());
            ret.m_proposals.emplace_back(std::make_unique<Proposal>(this->clientModel, govObj, ret.m_gov_info, queryCollateralDepth(obj.collateralHash),
                                                                    /*is_broadcast=*/false));
        }
    } else if (m_proposal_source == ProposalSource::Local) {
        for (const auto& obj : getWalletProposals(/*pending=*/std::nullopt)) {
            CGovernanceObject govObj(obj.hashParent, obj.revision, obj.time, obj.collateralHash, obj.GetDataAsHexString());
            ret.m_proposals.emplace_back(std::make_unique<Proposal>(this->clientModel, govObj, ret.m_gov_info, queryCollateralDepth(obj.collateralHash),
                                                                    /*is_broadcast=*/clientModel->node().gov().existsObj(obj.GetHash())));
        }
    }

    auto fundable{clientModel->node().gov().getFundableProposalHashes()};
    ret.m_fundable_hashes = std::move(fundable.hashes);

    // Discover voting capability if we now have both client and wallet models
    if (walletModel) {
        dmn->forEachMN(/*only_valid=*/true, [&](const auto& dmn) {
            // Check if wallet owns the voting key using the same logic as RPC
            const auto script = GetScriptForDestination(PKHash(dmn.getKeyIdVoting()));
            if (walletModel->wallet().isSpendable(script)) {
                ret.m_votable_masternodes[dmn.getProTxHash()] = dmn.getKeyIdVoting();
            }
        });
    }

    return ret;
}

void GovernanceList::setProposalList(CalcProposalList&& data)
{
    proposalModel->setVotingParams(data.m_abs_vote_req);
    proposalModel->reconcile(std::move(data.m_proposals), std::move(data.m_fundable_hashes));
    m_gov_info = std::move(data.m_gov_info);
    votableMasternodes = std::move(data.m_votable_masternodes);
    updateMasternodeCount();
    updateProposalButtons();
}

void GovernanceList::updateProposalCount()
{
    ui->countLabel->setText(QString::number(proposalModelProxy->rowCount()));
    refreshColumnWidths();
    updateEmptyState();
}

void GovernanceList::updateEmptyState()
{
    const bool hasProposals = proposalModelProxy->rowCount() > 0;
    if (hasProposals) {
        ui->proposalStack->setCurrentWidget(ui->govTableView);
    } else {
        switch (m_proposal_source) {
        case ProposalSource::Active:
            ui->emptyLabel->setText(tr("No active proposals on the network."));
            break;
        case ProposalSource::Local:
            ui->emptyLabel->setText(tr("No proposals recorded in wallet file."));
            break;
        } // no default case, so the compiler can warn about missing cases
        ui->proposalStack->setCurrentWidget(ui->emptyPage);
    }
}

void GovernanceList::showCreateProposalDialog()
{
    if (!this->clientModel || !this->walletModel) {
        QMessageBox::warning(this, tr("Unavailable"), tr("A synced node and an unlocked wallet are required."));
        return;
    }
    ProposalCreate* proposalCreate = new ProposalCreate(this->walletModel, this);
    // Ensure closing the dialog actually destroys it so a fresh flow starts next time
    proposalCreate->setAttribute(Qt::WA_DeleteOnClose, true);
    // Modeless window that does not block the parent
    proposalCreate->setWindowModality(Qt::NonModal);
    proposalCreate->setModal(false);
    proposalCreate->setWindowFlag(Qt::Window, true);
    // Auto-open Resume dialog after successful creation and refresh the governance list
    connect(proposalCreate, &QDialog::accepted, this, [this] { handleProposalListChanged(/*force=*/true); });
    connect(proposalCreate, &QDialog::accepted, this, &GovernanceList::updateProposalButtons);
    connect(proposalCreate, &QDialog::accepted, this, &GovernanceList::showResumeProposalDialog);
    proposalCreate->show();
}

void GovernanceList::showResumeProposalDialog()
{
    if (!clientModel || !walletModel) {
        QMessageBox::warning(this, tr("Resume proposal"), tr("A synced node and an unlocked wallet are required."));
        return;
    }

    const auto proposals = getWalletProposals(/*pending=*/true);
    ProposalResume* dialog = new ProposalResume(clientModel->node(), clientModel, walletModel, proposals, this);
    connect(dialog, &ProposalResume::proposalBroadcasted, this, [this] { handleProposalListChanged(/*force=*/true); });
    dialog->setAttribute(Qt::WA_DeleteOnClose, true);
    dialog->setWindowModality(Qt::NonModal);
    dialog->setModal(false);
    dialog->setWindowFlag(Qt::Window, true);
    dialog->show();
}

void GovernanceList::showProposalContextMenu(const QPoint& pos)
{
    const auto index = ui->govTableView->indexAt(pos);

    if (!index.isValid()) {
        return;
    }

    const auto proposal = proposalModel->getProposalAt(proposalModelProxy->mapToSource(index));
    if (!proposal) {
        return;
    }

    proposalContextMenu->clear();
    proposalContextMenu->addAction(tr("Copy Raw JSON"), this, &GovernanceList::copyProposalJson);
    if (!proposal->url().isEmpty()) {
        proposalContextMenu->addAction(tr("Open Proposal URL…"), this, &GovernanceList::openProposalUrl);
    }

    // Add voting options if wallet is available and has voting capability
    if (walletModel && canVote()) {
        proposalContextMenu->addSeparator();
        proposalContextMenu->addAction(tr("Vote Yes"), this, &GovernanceList::voteYes);
        proposalContextMenu->addAction(tr("Vote No"), this, &GovernanceList::voteNo);
        proposalContextMenu->addAction(tr("Vote Abstain"), this, &GovernanceList::voteAbstain);
    }

    proposalContextMenu->exec(QCursor::pos());
}

void GovernanceList::showAdditionalInfo(const QModelIndex& index)
{
    if (!index.isValid() || !clientModel) {
        return;
    }

    const auto proposal = proposalModel->getProposalAt(proposalModelProxy->mapToSource(index));
    if (!proposal) {
        return;
    }

    DescriptionDialog* dialog = new DescriptionDialog(
        tr("Details for %1").arg(proposal->title()),
        proposal->toHtml(clientModel->getOptionsModel()->getDisplayUnit()),
        /*parent=*/this);
    dialog->resize(800, 380);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
}

void GovernanceList::updateMasternodeCount() const
{
    if (ui && ui->mnCountLabel) {
        ui->mnCountLabel->setText(QString::number(votableMasternodes.size()));
    }
}

void GovernanceList::updateProposalButtons()
{
    if (!clientModel || !clientModel->masternodeSync().isGovernanceSynced()) {
        const QString tooltip = tr("Cannot interact with governance before sync completes");
        ui->btnCreateProposal->setEnabled(false);
        ui->btnCreateProposal->setToolTip(tooltip);
        ui->btnResumeProposal->setEnabled(false);
        ui->btnResumeProposal->setToolTip(tooltip);
        return;
    }

    // Using filler tooltips as tooltips once set cannot be disabled
    ui->btnCreateProposal->setEnabled(true);
    ui->btnCreateProposal->setToolTip(tr("Creates a new proposal"));
    ui->btnResumeProposal->setEnabled(true);
    ui->btnResumeProposal->setToolTip(tr("Resumes an existing proposal"));

    // Wallets with insufficient balance cannot create proposals
    if (walletModel && walletModel->getOptionsModel()) {
        const auto proposal_fee = m_gov_info.proposalfee;
        if (walletModel->wallet().getBalance() < proposal_fee) {
            ui->btnCreateProposal->setEnabled(false);
            ui->btnCreateProposal->setToolTip(
                tr("Creating proposals costs %1, insufficient balance")
                    .arg(BitcoinUnits::formatWithUnit(walletModel->getOptionsModel()->getDisplayUnit(), proposal_fee, false,
                                                      BitcoinUnits::SeparatorStyle::ALWAYS)));
        }
    }
}

void GovernanceList::setProposalSource(int index)
{
    m_proposal_source = static_cast<ProposalSource>(ui->proposalSourceCombo->itemData(index).toInt());
    ui->govTableView->setColumnHidden(ProposalModel::Column::VOTING_STATUS, m_proposal_source == ProposalSource::Local);
    handleProposalListChanged(/*force=*/true);
}

void GovernanceList::voteYes() { voteForProposal(VOTE_OUTCOME_YES); }

void GovernanceList::voteNo() { voteForProposal(VOTE_OUTCOME_NO); }

void GovernanceList::voteAbstain() { voteForProposal(VOTE_OUTCOME_ABSTAIN); }

void GovernanceList::openProposalUrl()
{
    const auto selection = ui->govTableView->selectionModel()->selectedRows();
    if (selection.isEmpty()) {
        return;
    }

    const auto proposal = proposalModel->getProposalAt(proposalModelProxy->mapToSource(selection.first()));
    if (!proposal || proposal->url().isEmpty()) {
        return;
    }

    const QUrl url{QUrl(proposal->url())};
    if (const QString scheme = url.isValid() ? url.scheme().toLower() : QString();
        scheme != QLatin1String("http") && scheme != QLatin1String("https")) {
        QMessageBox::critical(this, tr("Error"), tr("Cannot validate URL, potentially malformed or unknown protocol."));
        return;
    }
    QMessageBox::StandardButton reply = QMessageBox::warning(
        this,
        tr("External Link Warning"),
        tr("You are about to open the following URL in your default browser\n\n%1\n\n"
           "This content was submitted by a user. It may not match what is described in the title.\n\n"
           "Do you wish to continue?").arg(proposal->url()),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        QDesktopServices::openUrl(url);
    }
}

void GovernanceList::copyProposalJson()
{
    const auto selection = ui->govTableView->selectionModel()->selectedRows();
    if (selection.isEmpty()) {
        return;
    }

    const auto proposal = proposalModel->getProposalAt(proposalModelProxy->mapToSource(selection.first()));
    if (!proposal) {
        return;
    }

    QApplication::clipboard()->setText(proposal->toJson());
}

void GovernanceList::voteForProposal(vote_outcome_enum_t outcome)
{
    if (!walletModel) {
        QMessageBox::warning(this, tr("Voting Failed"), tr("No wallet available."));
        return;
    }

    if (votableMasternodes.empty()) {
        QMessageBox::warning(this, tr("Voting Failed"), tr("No masternode voting keys found in wallet."));
        return;
    }

    // Get the selected proposal
    const auto selection = ui->govTableView->selectionModel()->selectedRows();
    if (selection.isEmpty()) {
        QMessageBox::warning(this, tr("Voting Failed"), tr("Please select a proposal to vote on."));
        return;
    }

    const auto index = selection.first();
    const auto proposal = proposalModel->getProposalAt(proposalModelProxy->mapToSource(index));
    if (!proposal) return;

    const uint256 proposalHash(uint256S(proposal->hash().toStdString()));

    // Request unlock if needed and keep context alive for the voting operation
    WalletModel::UnlockContext ctx(walletModel->requestUnlock());
    if (!ctx.isValid()) {
        // Unlock cancelled or failed
        QMessageBox::warning(this, tr("Voting Failed"), tr("Unable to unlock wallet."));
        return;
    }

    int nSuccessful = 0;
    int nFailed = 0;
    QStringList failedMessages;

    // Get masternode list once before the loop
    auto [mnList, pindex] = clientModel->getMasternodeList();
    if (!pindex) {
        QMessageBox::warning(this, tr("Voting Failed"), tr("Unable to get masternode list. Please try again later."));
        return;
    }

    // Vote with each masternode
    for (const auto& [proTxHash, votingKeyID] : votableMasternodes) {
        // Find the masternode
        auto dmn = mnList->getValidMN(proTxHash);
        if (!dmn) {
            nFailed++;
            failedMessages.append(tr("Masternode %1 not found").arg(QString::fromStdString(proTxHash.ToString())));
            continue;
        }

        // Create vote
        CGovernanceVote vote(dmn->getCollateralOutpoint(), proposalHash, VOTE_SIGNAL_FUNDING, outcome);

        // Sign vote via wallet interface
        if (!walletModel->wallet().signGovernanceVote(votingKeyID, vote)) {
            nFailed++;
            failedMessages.append(
                tr("Failed to sign vote for masternode %1").arg(QString::fromStdString(proTxHash.ToString())));
            continue;
        }

        // Submit vote
        std::string strError;
        if (clientModel->node().gov().processVoteAndRelay(vote, strError)) {
            nSuccessful++;
        } else {
            nFailed++;
            failedMessages.append(
                tr("Masternode %1: %2").arg(QString::fromStdString(proTxHash.ToString()), QString::fromStdString(strError)));
        }
    }

    // Show results
    QString message;
    if (nSuccessful > 0) {
        message += tr("Voted successfully %n time(s)", "", nSuccessful);
    }
    if (nFailed > 0) {
        if (nSuccessful > 0) {
            message += QString("\n");
        }
        message += tr("Failed to vote %n time(s)", "", nFailed);
    }
    if (!failedMessages.isEmpty()) {
        message += QString("\n\n") + tr("Errors:") + QString("\n") + failedMessages.join("\n");
    }

    QMessageBox::information(this, tr("Voting Results"), message);

    // Update proposal list to show new vote counts
    handleProposalListChanged(/*force=*/true);
}

void GovernanceList::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    refreshColumnWidths();
}

void GovernanceList::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    refreshColumnWidths();
}

void GovernanceList::refreshColumnWidths()
{
    // Bail out if resize in progress or viewport is too small
    const int tableWidth = ui->govTableView->viewport()->width();
    if (m_col_refresh || tableWidth <= 0) {
        return;
    } else {
        m_col_refresh = true;
    }

    auto* header = ui->govTableView->horizontalHeader();
    header->setMinimumSectionSize(0);
    header->setSectionResizeMode(ProposalModel::Column::STATUS, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(ProposalModel::Column::PAYMENT_AMOUNT, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(ProposalModel::Column::START_DATE, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(ProposalModel::Column::END_DATE, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(ProposalModel::Column::VOTING_STATUS, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(ProposalModel::Column::HASH, QHeaderView::ResizeToContents);

    // Calculate width used by ResizeToContents columns
    const int availableWidth = [this, &header, &tableWidth](){
        int fixedWidth = 0;
        for (int idx = 0; idx < ProposalModel::Column::_COUNT; idx++) {
            if (idx != ProposalModel::Column::TITLE && idx != ProposalModel::Column::HASH && !ui->govTableView->isColumnHidden(idx)) {
                fixedWidth += header->sectionSize(idx);
            }
        }
        return std::max(0, tableWidth - fixedWidth);
    }();

    // Hash gets what's left after Title takes its minimum, clamped to [0, hashContentWidth]
    const int hashContentWidth = header->sectionSize(ProposalModel::Column::HASH);
    const int hashWidth = std::clamp<int>(availableWidth - TITLE_MIN_WIDTH, 0, hashContentWidth);
    const int titleWidth = availableWidth - hashWidth;
    header->setSectionResizeMode(ProposalModel::Column::TITLE, QHeaderView::Interactive);
    header->setSectionResizeMode(ProposalModel::Column::HASH, QHeaderView::Interactive);
    header->resizeSection(ProposalModel::Column::TITLE, titleWidth);
    header->resizeSection(ProposalModel::Column::HASH, hashWidth);

    m_col_refresh = false;
}
