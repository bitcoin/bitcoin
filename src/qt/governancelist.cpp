// Copyright (c) 2021-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/forms/ui_governancelist.h>

#include <evo/deterministicmns.h>
#include <governance/governance.h>
#include <governance/vote.h>

#include <qt/governancelist.h>
#include <qt/guiutil_font.h>
#include <qt/proposalmodel.h>
#include <qt/proposalwizard.h>

#include <chainparams.h>
#include <interfaces/node.h>
#include <interfaces/wallet.h>
#include <script/standard.h>
#include <util/strencodings.h>
#include <wallet/wallet.h>

#include <qt/clientmodel.h>
#include <qt/guiutil.h>
#include <qt/optionsmodel.h>
#include <qt/walletmodel.h>

#include <QAbstractItemView>
#include <QMessageBox>

//
// Governance Tab main widget.
//

GovernanceList::GovernanceList(QWidget* parent) :
    QWidget(parent),
    ui(std::make_unique<Ui::GovernanceList>()),
    proposalModel(new ProposalModel(this)),
    proposalModelProxy(new QSortFilterProxyModel(this)),
    proposalContextMenu(new QMenu(this)),
    timer(new QTimer(this))
{
    ui->setupUi(this);

    GUIUtil::setFont({ui->label_count_2, ui->countLabel, ui->label_mn_count, ui->mnCountLabel},
                     {GUIUtil::FontWeight::Bold, 14});
    GUIUtil::setFont({ui->label_filter_2}, {GUIUtil::FontWeight::Normal, 15});

    proposalModelProxy->setSourceModel(proposalModel);
    ui->govTableView->setModel(proposalModelProxy);
    ui->govTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->govTableView->horizontalHeader()->setStretchLastSection(true);
    ui->govTableView->verticalHeader()->setVisible(false);

    for (int i = 0; i < proposalModel->columnCount(); ++i) {
        ui->govTableView->setColumnWidth(i, proposalModel->columnWidth(i));
    }

    // Set up sorting.
    proposalModelProxy->setSortRole(Qt::EditRole);
    ui->govTableView->setSortingEnabled(true);
    ui->govTableView->sortByColumn(ProposalModel::Column::START_DATE, Qt::DescendingOrder);

    // Set up filtering.
    proposalModelProxy->setFilterKeyColumn(ProposalModel::Column::TITLE); // filter by title column...
    connect(ui->filterLineEdit, &QLineEdit::textChanged, proposalModelProxy, &QSortFilterProxyModel::setFilterFixedString);

    // Changes to number of rows should update proposal count display.
    connect(proposalModelProxy, &QSortFilterProxyModel::rowsInserted, this, &GovernanceList::updateProposalCount);
    connect(proposalModelProxy, &QSortFilterProxyModel::rowsRemoved, this, &GovernanceList::updateProposalCount);
    connect(proposalModelProxy, &QSortFilterProxyModel::layoutChanged, this, &GovernanceList::updateProposalCount);

    // Enable CustomContextMenu on the table to make the view emit customContextMenuRequested signal.
    ui->govTableView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->govTableView, &QTableView::customContextMenuRequested, this, &GovernanceList::showProposalContextMenu);

    // Create Proposal button
    connect(ui->btnCreateProposal, &QPushButton::clicked, this, &GovernanceList::showCreateProposalDialog);
    connect(ui->govTableView, &QTableView::doubleClicked, this, &GovernanceList::showAdditionalInfo);

    connect(timer, &QTimer::timeout, this, &GovernanceList::updateProposalList);

    // Initialize masternode count to 0
    ui->mnCountLabel->setText("0");

    GUIUtil::updateFonts();
}

GovernanceList::~GovernanceList() = default;

void GovernanceList::setClientModel(ClientModel* model)
{
    this->clientModel = model;
    if (model != nullptr) {
        connect(model->getOptionsModel(), &OptionsModel::displayUnitChanged, this, &GovernanceList::updateDisplayUnit);

        updateProposalList();
    }
}

void GovernanceList::setWalletModel(WalletModel* model)
{
    this->walletModel = model;
    if (model && clientModel) {
        updateVotingCapability();
    }
}

void GovernanceList::updateDisplayUnit()
{
    if (this->clientModel) {
        proposalModel->setDisplayUnit(this->clientModel->getOptionsModel()->getDisplayUnit());
        ui->govTableView->update();
    }
}

void GovernanceList::updateProposalList()
{
    if (this->clientModel) {
        // A proposal is considered passing if (YES votes - NO votes) >= (Total Weight of Masternodes / 10),
        // count total valid (ENABLED) masternodes to determine passing threshold.
        // Need to query number of masternodes here with access to clientModel.
        const int nWeightedMnCount = clientModel->getMasternodeList().first->getValidWeightedMNsCount();
        const int nAbsVoteReq = std::max(Params().GetConsensus().nGovernanceMinQuorum, nWeightedMnCount / 10);
        proposalModel->setVotingParams(nAbsVoteReq);

        std::vector<CGovernanceObject> govObjList;
        clientModel->getAllGovernanceObjects(govObjList);
        ProposalList newProposals;
        for (const auto& govObj : govObjList) {
            if (govObj.GetObjectType() != GovernanceObject::PROPOSAL) {
                continue; // Skip triggers.
            }
            newProposals.emplace_back(std::make_unique<Proposal>(this->clientModel, govObj));
        }
        proposalModel->reconcile(std::move(newProposals));

        // Update voting capability if we now have both client and wallet models
        if (walletModel) {
            updateVotingCapability();
        }
    }

    // Schedule next update.
    timer->start(GOVERNANCELIST_UPDATE_SECONDS * 1000);
}

void GovernanceList::updateProposalCount() const
{
    ui->countLabel->setText(QString::number(proposalModelProxy->rowCount()));
}

void GovernanceList::showCreateProposalDialog()
{
    if (!this->clientModel || !this->walletModel) {
        QMessageBox::warning(this, tr("Unavailable"), tr("A synced node and an unlocked wallet are required."));
        return;
    }
    ProposalWizard* proposalWizard = new ProposalWizard(this->walletModel, this);
    // Ensure closing the dialog actually destroys it so a fresh flow starts next time
    proposalWizard->setAttribute(Qt::WA_DeleteOnClose, true);
    // Modeless window that does not block the parent
    proposalWizard->setWindowModality(Qt::NonModal);
    proposalWizard->setModal(false);
    proposalWizard->setWindowFlag(Qt::Window, true);
    proposalWizard->show();
}

void GovernanceList::showProposalContextMenu(const QPoint& pos)
{
    const auto index = ui->govTableView->indexAt(pos);

    if (!index.isValid()) {
        return;
    }

    const auto proposal = proposalModel->getProposalAt(proposalModelProxy->mapToSource(index));
    if (proposal == nullptr) {
        return;
    }

    // right click menu with option to open proposal url
    QString proposal_url = proposal->url();
    proposal_url.replace(QChar('&'), QString("&&"));

    proposalContextMenu->clear();
    proposalContextMenu->addAction(proposal_url, [proposal]() { proposal->openUrl(); });

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
    if (!index.isValid()) {
        return;
    }

    const auto proposal = proposalModel->getProposalAt(proposalModelProxy->mapToSource(index));
    if (proposal == nullptr) {
        return;
    }

    const auto windowTitle = tr("Proposal Info: %1").arg(proposal->title());
    const auto json = proposal->toJson();

    QMessageBox::information(this, windowTitle, json);
}

void GovernanceList::updateVotingCapability()
{
    if (!walletModel || !clientModel) return;

    auto [mn_list, pindex] = clientModel->getMasternodeList();
    if (!pindex) return;

    votableMasternodes.clear();
    mn_list->forEachMN(/*only_valid=*/true, [&](const auto& dmn) {
        // Check if wallet owns the voting key using the same logic as RPC
        const CScript script = GetScriptForDestination(PKHash(dmn.getKeyIdVoting()));
        if (walletModel->wallet().isSpendable(script)) {
            votableMasternodes[dmn.getProTxHash()] = dmn.getKeyIdVoting();
        }
    });

    // Update masternode count display
    updateMasternodeCount();
}

void GovernanceList::updateMasternodeCount() const
{
    if (ui && ui->mnCountLabel) {
        ui->mnCountLabel->setText(QString::number(votableMasternodes.size()));
    }
}

void GovernanceList::voteYes() { voteForProposal(VOTE_OUTCOME_YES); }

void GovernanceList::voteNo() { voteForProposal(VOTE_OUTCOME_NO); }

void GovernanceList::voteAbstain() { voteForProposal(VOTE_OUTCOME_ABSTAIN); }

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
    if (proposal == nullptr) return;

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

        // Sign vote using CWallet member function
        if (!walletModel->wallet().wallet()->SignGovernanceVote(votingKeyID, vote)) {
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
    updateProposalList();
}
