// Copyright (c) 2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/proposalinfo.h>
#include <qt/forms/ui_proposalinfo.h>

#include <chainparams.h>
#include <evo/dmn_types.h>
#include <interfaces/node.h>
#include <script/standard.h>

#include <qt/bitcoinunits.h>
#include <qt/clientfeeds.h>
#include <qt/clientmodel.h>
#include <qt/donutchart.h>
#include <qt/guiutil.h>
#include <qt/guiutil_font.h>
#include <qt/optionsmodel.h>
#include <qt/proposalmodel.h>

#ifdef ENABLE_WALLET
#include <interfaces/wallet.h>
#include <qt/masternodemodel.h>
#include <qt/walletmodel.h>
#endif // ENABLE_WALLET

#include <QEvent>

#include <algorithm>

namespace {
uint16_t GetCycleCount(const interfaces::GOV::GovernanceInfo& gov_info, const Consensus::Params& consensus_params)
{
    const auto first_offset{(gov_info.superblockcycle - consensus_params.nSuperblockStartBlock % gov_info.superblockcycle) % gov_info.superblockcycle};
    const auto first_height{consensus_params.nSuperblockStartBlock + first_offset};
    return gov_info.lastsuperblock >= first_height ? ((gov_info.lastsuperblock - first_height) / gov_info.superblockcycle + 1) : 0;
}

QString FormatBlocksWithTime(int blocks, int64_t block_spacing, bool past)
{
    if (blocks <= 0) return QObject::tr("now");
    const QString duration{GUIUtil::formatBlockDuration(blocks, block_spacing)};
    return past ? QObject::tr("%1 ago").arg(duration)
                : QObject::tr("%1 left").arg(duration);
}
} // anonymous namespace

ProposalInfo::ProposalInfo(QWidget* parent) :
    QWidget(parent),
    ui(new Ui::ProposalInfo)
{
    ui->setupUi(this);
    GUIUtil::setFont({ui->labelGeneral,
                      ui->labelNode,
                      ui->labelParticipation,
                      ui->labelProposals},
                      {GUIUtil::FontWeight::Bold, 16});

    for (auto* element : {ui->labelNode, ui->labelParticipation, ui->labelProposals}) {
        element->setContentsMargins(0, 10, 0, 0);
    }
}

ProposalInfo::~ProposalInfo()
{
    delete ui;
}

void ProposalInfo::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::StyleChange) {
        updateProposalInfo();
    }
    QWidget::changeEvent(e);
}

void ProposalInfo::setClientModel(ClientModel* model)
{
    this->m_client_model = model;
    if (!m_client_model) {
        return;
    }
    m_feed_masternode = m_client_model->feedMasternode();
    m_feed_proposal = m_client_model->feedProposal();
    if (m_feed_masternode && m_feed_proposal) {
        connect(m_feed_masternode, &MasternodeFeed::dataReady, this, &ProposalInfo::updateProposalInfo);
        connect(m_feed_proposal, &ProposalFeed::dataReady, this, &ProposalInfo::updateProposalInfo);
        updateProposalInfo();
    }
}

#ifdef ENABLE_WALLET
void ProposalInfo::setWalletModel(WalletModel* model)
{
    this->m_wallet_model = model;
    if (!m_wallet_model) {
        return;
    }
    if (m_feed_masternode && m_feed_proposal) {
        updateProposalInfo();
    }
}
#endif // ENABLE_WALLET

void ProposalInfo::updateProposalInfo()
{
    if (!m_client_model || !m_feed_masternode || !m_feed_proposal) {
        return;
    }

    const auto data_mn = m_feed_masternode->data();
    const auto data_pr = m_feed_proposal->data();
    if (!data_mn || !data_mn->m_valid || !data_pr || data_pr->m_gov_info.superblockcycle <= 0) {
        return;
    }

    const auto& gov_info = data_pr->m_gov_info;
    const auto& consensus_params = Params().GetConsensus();
    const int64_t block_spacing = gov_info.targetSpacing;
    const int32_t passing_threshold = data_pr->m_abs_vote_req;

    // General section
    ui->labelSuperblockCycle->setText(QString::number(GetCycleCount(gov_info, consensus_params)));
    ui->labelPassingThreshold->setText(tr("%1 votes").arg(passing_threshold));

    const auto block_height = m_client_model->getNumBlocks();
    ui->labelLastSuperblock->setText(tr("%1 (%2)").arg(gov_info.lastsuperblock)
                                                  .arg(FormatBlocksWithTime(block_height - gov_info.lastsuperblock, block_spacing, /*past=*/true)));
    ui->labelNextSuperblock->setText(tr("%1 (%2)").arg(gov_info.nextsuperblock)
                                                  .arg(FormatBlocksWithTime(gov_info.nextsuperblock - block_height, block_spacing, /*past=*/false)));

    const auto voting_cutoff_height = gov_info.nextsuperblock - gov_info.superblockmaturitywindow;
    ui->labelVotingDeadline->setText(tr("%1 (%2)").arg(voting_cutoff_height)
                                                  .arg(FormatBlocksWithTime(voting_cutoff_height - block_height, block_spacing, /*past=*/false)));

    // Participation section
    ui->labelMasternodesVoting->setText(tr("%1 (%2 eligible)").arg(data_pr->m_max_regular_voters).arg(data_mn->m_counts.m_valid_mn));
    ui->labelEvoNodesVoting->setText(tr("%1 (%2 eligible)").arg(data_pr->m_max_evo_voters).arg(data_mn->m_counts.m_valid_evo));

    // Node section
#ifdef ENABLE_WALLET
    if (m_wallet_model) {
        size_t controlled_count{0};
        int32_t weighted_votes{0};
        for (const auto& entry : data_mn->m_entries) {
            if (entry->isBanned()) {
                continue;
            }
            if (m_wallet_model->wallet().isSpendable(GetScriptForDestination(PKHash(entry->keyIdVotingRaw())))) {
                controlled_count++;
                weighted_votes += GetMnType(entry->type()).voting_weight;
            }
        }
        ui->labelMasternodesControlled->setText(QString::number(controlled_count));
        ui->labelWeightedVotes->setText(QString::number(weighted_votes));
    } else {
        ui->labelMasternodesControlled->setText(tr("N/A"));
        ui->labelWeightedVotes->setText(tr("N/A"));
    }
#else
    ui->labelMasternodesControlled->setText(tr("N/A"));
    ui->labelWeightedVotes->setText(tr("N/A"));
#endif // ENABLE_WALLET

    // Proposals section with budget chart
    const BitcoinUnit display_unit{m_client_model->getOptionsModel()->getDisplayUnit()};
    const CAmount budget_avail{gov_info.governancebudget};
    const QColor base_blue{GUIUtil::getThemedQColor(GUIUtil::ThemedColor::BLUE).darker(160)};

    // Collect passing proposals sorted by vote count
    std::vector<std::shared_ptr<Proposal>> passing_proposals;
    uint16_t proposals_fail{0}, proposals_total{0};
    for (const auto& prop : data_pr->m_proposals) {
        proposals_total++;
        if (prop->getAbsoluteYesCount() >= passing_threshold) {
            passing_proposals.push_back(prop);
        } else {
            proposals_fail++;
        }
    }
    std::sort(passing_proposals.begin(), passing_proposals.end(),
              [](const std::shared_ptr<Proposal>& a, const std::shared_ptr<Proposal>& b) {
                  return a->getAbsoluteYesCount() > b->getAbsoluteYesCount();
              });

    CAmount budget_funded{0}, unfunded_shortfall{0};
    uint16_t unfunded_count{0};
    std::vector<DonutChart::Slice> chart_slices{};

    for (size_t idx{0}; idx < passing_proposals.size(); idx++) {
        const auto& prop{passing_proposals[idx]};
        const CAmount proposed_amount{prop->paymentAmount()};
        if (data_pr->m_fundable_hashes.count(prop->objHash()) > 0) {
            budget_funded += proposed_amount;
        } else {
            unfunded_count++;
            unfunded_shortfall += proposed_amount;
        }
        const double slice_pct{budget_avail <= 0
            ? 0.0
            : (static_cast<double>(proposed_amount) / static_cast<double>(budget_avail)) * 100.0};
        chart_slices.emplace_back(DonutChart::Slice{
            static_cast<double>(proposed_amount),
            (idx == 0) ? base_blue : base_blue.lighter(100 + static_cast<int>(idx * 20)),
            prop->title().isEmpty() ? tr("Proposal %1").arg(idx + 1) : prop->title(),
            GUIUtil::formatAmount(display_unit, proposed_amount),
            tr("%1% of budget").arg(slice_pct, 0, 'f', 1)
        });
    }

    ui->labelProposalCount->setText(QString::number(proposals_total));
    ui->labelPassingProposals->setText(QString::number(passing_proposals.size()));
    ui->labelFailingProposals->setText(QString::number(proposals_fail));

    const double alloc_pct_chart{budget_avail <= 0
        ? 0.0
        : (static_cast<double>(budget_funded) / static_cast<double>(budget_avail)) * 100.0};
    DonutChart::CenterText default_text{
        GUIUtil::formatAmount(display_unit, budget_funded),
        tr("/ %1").arg(GUIUtil::formatAmount(display_unit, budget_avail)),
        tr("%1% allocated").arg(alloc_pct_chart, 0, 'f', 1)
    };
    ui->budgetChart->setData(std::move(chart_slices), static_cast<double>(budget_avail), std::move(default_text));

    if (budget_avail > 0) {
        ui->labelBudgetAllocated->setText(
            tr("%1 / %2 (%3%)").arg(GUIUtil::formatAmount(display_unit, budget_funded, /*is_signed=*/false, /*truncate=*/2))
                               .arg(GUIUtil::formatAmount(display_unit, budget_avail, /*is_signed=*/false, /*truncate=*/2))
                               .arg(alloc_pct_chart, 0, 'f', 2));
    } else {
        ui->labelBudgetAllocated->setText(tr("N/A"));
    }

    if (unfunded_count > 0) {
        ui->labelUnfundedProposals->setText(
            tr("%1 (short %2)").arg(unfunded_count)
                               .arg(GUIUtil::formatAmount(display_unit, unfunded_shortfall)));
    } else {
        ui->labelUnfundedProposals->setText(QString::number(0));
    }
}
