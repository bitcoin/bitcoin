// Copyright (c) 2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/proposalresume.h>
#include <qt/forms/ui_proposalresume.h>

#include <governance/governance.h>

#include <qt/guiutil_font.h>
#include <qt/proposalmodel.h>

#include <interfaces/node.h>
#include <interfaces/wallet.h>

#include <qt/bitcoinunits.h>
#include <qt/clientmodel.h>
#include <qt/optionsmodel.h>
#include <qt/walletmodel.h>

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayoutItem>
#include <QMessageBox>
#include <QPushButton>
#include <QShowEvent>
#include <QSpacerItem>
#include <QStyle>
#include <QTextEdit>
#include <QTimer>
#include <QVBoxLayout>

#include <algorithm>

ProposalResume::ProposalResume(interfaces::Node& node, ClientModel* client_model, WalletModel* wallet_model,
                               const std::vector<Governance::Object>& proposals, QWidget* parent) :
    QDialog{parent},
    m_ui{new Ui::ProposalResume},
    m_node{node},
    m_client_model{client_model},
    m_wallet_model{wallet_model}
{
    m_ui->setupUi(this);

    m_ui->scrollArea->viewport()->setAutoFillBackground(false);
    m_ui->scrollContents->setAttribute(Qt::WA_StyledBackground, true);

    m_emptyLabel = new QLabel(tr("No pending proposals to broadcast."), m_ui->scrollContents);
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    connect(m_ui->btnClose, &QPushButton::clicked, this, &QDialog::reject);

    // Load parameters and add proposals
    const auto info = m_node.gov().getGovernanceInfo();
    m_relay_confs = info.relayRequiredConfs;
    for (const auto& proposal : proposals) {
        addProposal(proposal);
    }
    m_ui->proposalsLayout->addStretch();

    startRefreshTimer();
    updateEmptyState();
    GUIUtil::updateFonts();
    QTimer::singleShot(0, this, &ProposalResume::recalculateDescHeights);
}

ProposalResume::~ProposalResume()
{
    if (m_refresh_timer) {
        m_refresh_timer->stop();
    }
    delete m_ui;
}

void ProposalResume::changeEvent(QEvent* e)
{
    QDialog::changeEvent(e);
    if (e->type() == QEvent::StyleChange) {
        QTimer::singleShot(0, this, &ProposalResume::refreshStyle);
    }
}

void ProposalResume::showEvent(QShowEvent* e)
{
    QDialog::showEvent(e);
    if (!e->spontaneous()) {
        refreshStyle();
    }
}

void ProposalResume::refreshStyle()
{
    style()->unpolish(m_ui->scrollContents);
    style()->polish(m_ui->scrollContents);
    m_ui->scrollContents->update();
    for (auto& entry : m_entries) {
        if (entry.container) {
            style()->unpolish(entry.container);
            style()->polish(entry.container);
            entry.container->update();
        }
    }
}

int ProposalResume::queryConfirmations(const uint256& tx_hash)
{
    if (!m_wallet_model) return -1;
    interfaces::WalletTxStatus tx_status; int num_blocks{}; int64_t block_time{};
    if (!m_wallet_model->wallet().tryGetTxStatus(tx_hash, tx_status, num_blocks, block_time)) {
        return -1;
    }
    return tx_status.depth_in_main_chain;
}

QString ProposalResume::formatProposalHtml(const Governance::Object& obj, int confirmations)
{
    const CGovernanceObject gov_obj(obj.hashParent, obj.revision, obj.time, obj.collateralHash, obj.GetDataAsHexString());
    const Proposal proposal(m_client_model, gov_obj, m_node.gov().getGovernanceInfo(), confirmations, /*is_broadcast=*/false);
    const BitcoinUnit unit = [this]() {
        if (m_client_model && m_client_model->getOptionsModel()) {
            return m_client_model->getOptionsModel()->getDisplayUnit();
        }
        return BitcoinUnit::DASH;
    }();
    const QString fund_summary{tr("For %1 payment(s) of %2 to %3")
        .arg(proposal.paymentsRequested())
        .arg(BitcoinUnits::formatWithUnit(unit, proposal.paymentAmount(), false, BitcoinUnits::SeparatorStyle::ALWAYS))
        .arg(proposal.paymentAddress())};

    QString conf_summary;
    if (confirmations < 0) {
        conf_summary = tr("Unknown");
    } else if (confirmations < m_relay_confs) {
        conf_summary = tr("Pending");
    } else {
        conf_summary = tr("Ready");
    }

    QString html;
    html += "<table border='0' cellspacing='0' cellpadding='2' width='100%'>";
    html += "<tr><td colspan='2'><h3>" + proposal.title().toHtmlEscaped() + "</h3></td></tr>";
    html += "<tr><td colspan='2'>" + proposal.url().toHtmlEscaped() + "</td></tr>";
    html += "<tr><td colspan='2'>" + fund_summary.toHtmlEscaped() + "</td></tr>";
    html += "<tr><td colspan='2'><b>" + tr("Collateral Hash") + ":</b> " + proposal.collateralHash() + "</td></tr>";
    html += "<tr><td colspan='2'><b>" + tr("Collateral Status") + ":</b> " + conf_summary + "</td></tr>";
    html += "</table>";
    return html;
}

void ProposalResume::addProposal(const Governance::Object& proposal)
{
    ProposalEntry entry;
    entry.proposal = proposal;
    entry.container = new QFrame(this);
    entry.container->setFrameShape(QFrame::NoFrame);
    entry.container->setObjectName("proposalEntry");

    // Create entity with proposal details
    entry.description = new QTextEdit(entry.container);
    entry.description->setReadOnly(true);
    entry.description->setFrameStyle(QFrame::NoFrame);
    entry.description->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    entry.description->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    entry.description->setTextInteractionFlags(Qt::NoTextInteraction);
    entry.description->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    GUIUtil::registerWidget(entry.description, formatProposalHtml(proposal, -1));

    // Create "Broadcast" action
    entry.broadcast_btn = new QPushButton(tr("Broadcast"), entry.container);
    entry.broadcast_btn->setEnabled(false);
    entry.broadcast_btn->setMinimumWidth(100);
    const uint256 obj_hash = proposal.GetHash();
    connect(entry.broadcast_btn, &QPushButton::clicked, this, [this, obj_hash]() {
        onBroadcast(obj_hash);
    });

    // Create entity with actions
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setContentsMargins(0, 5, 0, 0);
    buttonLayout->setSpacing(0);
    buttonLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));

    // Register actions
    buttonLayout->addWidget(entry.broadcast_btn);

    // Create parent entity
    QVBoxLayout* layout = new QVBoxLayout(entry.container);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(0);

    // Register description and actions
    layout->addWidget(entry.description);
    layout->addLayout(buttonLayout);

    m_ui->proposalsLayout->addWidget(entry.container);
    m_entries.push_back(entry);
}

void ProposalResume::onBroadcast(const uint256& obj_hash)
{
    auto it = std::find_if(m_entries.begin(), m_entries.end(),
        [&obj_hash](const ProposalEntry& e) {
            return e.proposal.GetHash() == obj_hash;
        });
    if (it == m_entries.end() || !m_wallet_model) {
        return;
    }
    auto& entry = *it;

    std::string error, result_hash;
    if (m_node.gov().submitProposal(entry.proposal.hashParent, entry.proposal.revision,
                                    entry.proposal.time, entry.proposal.GetDataAsHexString(),
                                    entry.proposal.collateralHash, result_hash, error)) {
        QMessageBox::information(this, tr("Broadcast proposal"),
            tr("Proposal has been broadcasted to the network with hash %1").arg(QString::fromStdString(result_hash)));
    } else {
        QMessageBox::critical(this, tr("Broadcast proposal"),
            tr("Unable to broadcast proposal, %1").arg(QString::fromStdString(error)));
        return;
    }

    Q_EMIT proposalBroadcasted();

    // Remove the entry's container widget and mark as removed
    if (entry.container) {
        m_ui->proposalsLayout->removeWidget(entry.container);
        entry.container->deleteLater();
        entry.container = nullptr;
    }
    m_entries.erase(it);

    // Update empty state
    updateEmptyState();
}

void ProposalResume::recalculateDescHeights()
{
    for (auto& entry : m_entries) {
        if (!entry.description) continue;

        // Set document width to viewport width for proper text wrapping
        const int viewportWidth = entry.description->viewport()->width();
        if (viewportWidth > 0) {
            entry.description->document()->setTextWidth(viewportWidth);
        }

        // Calculate height from document and set as fixed height
        const int docHeight = static_cast<int>(entry.description->document()->size().height());
        const int margins = entry.description->contentsMargins().top() + entry.description->contentsMargins().bottom();
        entry.description->setFixedHeight(docHeight + margins);
    }
}

void ProposalResume::refreshConfirmations()
{
    bool all_confirmed{true};

    for (auto& entry : m_entries) {
        const int confs = queryConfirmations(entry.proposal.collateralHash);
        if (confs != entry.collateral_confs) {
            entry.collateral_confs = confs;
            GUIUtil::registerWidget(entry.description, formatProposalHtml(entry.proposal, confs));
            entry.broadcast_btn->setEnabled(confs >= m_relay_confs);
        }
        if (confs < m_relay_confs) {
            all_confirmed = false;
        }
    }

    // Stop polling when all entries are confirmed
    if (all_confirmed && m_refresh_timer) {
        m_refresh_timer->stop();
    }
}

void ProposalResume::startRefreshTimer()
{
    if (m_refresh_timer) {
        return;
    }
    m_refresh_timer = new QTimer(this);
    connect(m_refresh_timer, &QTimer::timeout, this, &ProposalResume::refreshConfirmations);
    m_refresh_timer->start(5000);
    refreshConfirmations();
}

void ProposalResume::updateEmptyState()
{
    if (m_entries.empty()) {
        // Clear any existing layout items
        while (QLayoutItem* item = m_ui->proposalsLayout->takeAt(0)) {
            if (QWidget* w = item->widget(); w && w != m_emptyLabel) {
                w->deleteLater();
            }
            delete item;
        }

        // Add centered empty label
        m_ui->proposalsLayout->addStretch(1);
        m_ui->proposalsLayout->addWidget(m_emptyLabel);
        m_ui->proposalsLayout->addStretch(1);
        m_emptyLabel->setVisible(true);

        // Stop timer
        if (m_refresh_timer) {
            m_refresh_timer->stop();
        }
    } else {
        m_emptyLabel->setVisible(false);
    }
}
