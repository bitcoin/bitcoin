// Copyright (c) 2011-2021 The Bitcoin Core developers
// Copyright (c) 2014-2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/networkwidget.h>
#include <qt/forms/ui_networkwidget.h>

#include <chainparams.h>

#include <qt/clientfeeds.h>
#include <qt/clientmodel.h>
#include <qt/guiutil_font.h>
#include <qt/guiutil.h>
#include <qt/optionsmodel.h>

#include <QDateTime>
#include <QGridLayout>

#include <string>

namespace {
QString FormatQuorumStr(const std::string& name)
{
    QString qname{QString::fromStdString(name)};
    if (qname.startsWith("llmq_")) {
        qname = qname.mid(5); // Remove "llmq_"
        qname.replace('_', '/');
        return "LLMQ " + qname;
    }
    return qname;
}
} // anonymous namespace

NetworkWidget::NetworkWidget(QWidget* parent) :
    QWidget(parent),
    ui(new Ui::NetworkWidget)
{
    ui->setupUi(this);

    GUIUtil::setFont({ui->labelChainLocks,
                      ui->labelCreditPool,
                      ui->labelInstantSend,
                      ui->labelMasternodes,
                      ui->labelQuorums},
                     {GUIUtil::FontWeight::Bold, 16});

    for (auto* element : {ui->labelChainLocks, ui->labelInstantSend, ui->labelMasternodes}) {
        element->setContentsMargins(0, 10, 0, 0);
    }
}

NetworkWidget::~NetworkWidget()
{
    delete ui;
}

void NetworkWidget::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);

    // Sync bottom grid label column width with left grid (deferred to showEvent
    // so CSS styling and font metrics are fully resolved)
    int maxLabelWidth{0};
    for (int row = 0; row < ui->leftGridLayout->rowCount(); ++row) {
        if (auto* item = ui->leftGridLayout->itemAtPosition(row, 0)) {
            if (auto* widget = item->widget()) {
                maxLabelWidth = std::max(maxLabelWidth, widget->sizeHint().width());
            }
        }
    }
    if (maxLabelWidth > 0) {
        ui->bottomGridLayout->setColumnMinimumWidth(0, maxLabelWidth);
    }
}

void NetworkWidget::setClientModel(ClientModel* model)
{
    clientModel = model;
    if (!clientModel) {
        return;
    }

    m_feed_chainlock = model->feedChainLock();
    if (m_feed_chainlock) {
        connect(m_feed_chainlock, &ChainLockFeed::dataReady, this, &NetworkWidget::handleClDataChanged);
        handleClDataChanged();
    }

    m_feed_creditpool = model->feedCreditPool();
    if (m_feed_creditpool) {
        connect(m_feed_creditpool, &CreditPoolFeed::dataReady, this, &NetworkWidget::handleCrDataChanged);
        handleCrDataChanged();
    }

    m_feed_instantsend = model->feedInstantSend();
    if (m_feed_instantsend) {
        connect(m_feed_instantsend, &InstantSendFeed::dataReady, this, &NetworkWidget::handleIsDataChanged);
        handleIsDataChanged();
    }

    m_feed_masternode = model->feedMasternode();
    if (m_feed_masternode) {
        connect(m_feed_masternode, &MasternodeFeed::dataReady, this, &NetworkWidget::handleMnDataChanged);
        handleMnDataChanged();
    }

    m_feed_quorum = model->feedQuorum();
    if (m_feed_quorum) {
        connect(m_feed_quorum, &QuorumFeed::dataReady, this, &NetworkWidget::handleQrDataChanged);
        connect(model, &ClientModel::additionalDataSyncProgressChanged, this, &NetworkWidget::handleQrDataChanged);
        handleQrDataChanged();
    }

    if (clientModel->getOptionsModel()) {
        m_display_unit = clientModel->getOptionsModel()->getDisplayUnit();
        connect(clientModel->getOptionsModel(), &OptionsModel::displayUnitChanged, this, &NetworkWidget::updateDisplayUnit);
    }
}

void NetworkWidget::handleCrDataChanged()
{
    if (!m_feed_creditpool) {
        return;
    }
    const auto data = m_feed_creditpool->data();
    if (!data) {
        return;
    }

    m_creditpool_diff = data->m_counts.m_diff;
    m_creditpool_limit = data->m_counts.m_limit;
    m_creditpool_locked = data->m_counts.m_locked;

    ui->labelCrLastBlock->setText(GUIUtil::formatAmount(m_display_unit, m_creditpool_diff, /*is_signed=*/true, /*truncate=*/2));
    ui->labelCrLocked->setText(GUIUtil::formatAmount(m_display_unit, m_creditpool_locked, /*is_signed=*/false, /*truncate=*/2));
    ui->labelCrPending->setText(QString::number(data->m_pending_unlocks));
    ui->labelCrLimit->setText(GUIUtil::formatAmount(m_display_unit, m_creditpool_limit, /*is_signed=*/false, /*truncate=*/2));
}

void NetworkWidget::handleMnDataChanged()
{
    if (!m_feed_masternode) {
        return;
    }
    const auto data = m_feed_masternode->data();
    if (!data || !data->m_valid) {
        return;
    }
    ui->masternodeCount->setText(tr("Total: %1 (Enabled: %2)")
        .arg(QString::number(data->m_counts.m_total_mn))
        .arg(QString::number(data->m_counts.m_valid_mn)));
    ui->evoCount->setText(tr("Total: %1 (Enabled: %2)")
        .arg(QString::number(data->m_counts.m_total_evo))
        .arg(QString::number(data->m_counts.m_valid_evo)));
}

void NetworkWidget::handleClDataChanged()
{
    if (!m_feed_chainlock) {
        return;
    }
    const auto data = m_feed_chainlock->data();
    if (!data) {
        return;
    }
    ui->bestClHash->setText(data->m_hash);
    ui->bestClHeight->setText(QString::number(data->m_height));
    ui->bestClTime->setText(QDateTime::fromSecsSinceEpoch(data->m_block_time).toString());
}

void NetworkWidget::handleIsDataChanged()
{
    if (!m_feed_instantsend) {
        return;
    }
    const auto data = m_feed_instantsend->data();
    if (!data) {
        return;
    }
    ui->labelISLocks->setText(QString::number(data->m_counts.m_verified));
    ui->labelISPending->setText(QString::number(data->m_counts.m_unverified));
    ui->labelISWaiting->setText(QString::number(data->m_counts.m_awaiting_tx));
    ui->labelISUnprotected->setText(QString::number(data->m_counts.m_unprotected_tx));
}

void NetworkWidget::handleQrDataChanged()
{
    if (!clientModel || !m_feed_quorum) {
        return;
    }
    const auto data = m_feed_quorum->data();
    if (!data) {
        return;
    }

    QGridLayout* grid = ui->rightGridLayout;
    if (!grid) {
        return;
    }

    // Find the base row after labelQuorums header
    if (m_quorum_base_row < 0) {
        int idx = grid->indexOf(ui->labelQuorums);
        if (idx >= 0) {
            int row, col, rowSpan, colSpan;
            grid->getItemPosition(idx, &row, &col, &rowSpan, &colSpan);
            m_quorum_base_row = row + 1;
        } else {
            return;
        }
    }

    // Prune labels for quorum types no longer present; track whether any were
    // removed so surviving labels can be re-seated to close the resulting gaps.
    bool needs_reseating = false;
    std::erase_if(m_quorum_labels, [&](const auto& kv) {
        for (const auto& q : data->m_quorums) {
            if (q.m_name == kv.first) {
                return false;
            }
        }
        delete kv.second.first;
        delete kv.second.second;
        needs_reseating = true;
        return true;
    });

    int current_row{m_quorum_base_row};
    for (const auto& q : data->m_quorums) {
        auto [it, inserted] = m_quorum_labels.try_emplace(q.m_name, nullptr, nullptr);
        if (inserted) {
            it->second.first = new QLabel(FormatQuorumStr(q.m_name), this);
            it->second.first->setFont(GUIUtil::getFontNormal());
            it->second.first->setToolTip(tr("Waiting for blockchain sync…"));
            it->second.second = new QLabel(this);
            it->second.second->setFont(GUIUtil::getFontNormal());
            it->second.second->setCursor(Qt::IBeamCursor);
            it->second.second->setTextInteractionFlags(Qt::LinksAccessibleByMouse | Qt::TextSelectableByKeyboard | Qt::TextSelectableByMouse);
            it->second.second->setToolTip(tr("Waiting for blockchain sync…"));
            grid->addWidget(it->second.first, current_row, 0);
            grid->addWidget(it->second.second, current_row, 1);
        } else if (needs_reseating) {
            grid->removeWidget(it->second.first);
            grid->removeWidget(it->second.second);
            grid->addWidget(it->second.first, current_row, 0);
            grid->addWidget(it->second.second, current_row, 1);
        }
        it->second.second->setText(tr("%1 active (%2% health)").arg(q.m_count).arg(q.m_health * 100.0, 0, 'f', 1));

        if (clientModel->masternodeSync().isBlockchainSynced()) {
            const int64_t spacing = Params().GetConsensus().nPowTargetSpacing;
            const int tip_height = clientModel->getNumBlocks();
            const QString line1 = q.m_rotates
                ? tr("Quorum type rotates, data retained for %1").arg(GUIUtil::formatBlockDuration(q.m_data_retention_blocks, spacing))
                : tr("Quorum type does not rotate, data retained for %1").arg(GUIUtil::formatBlockDuration(q.m_data_retention_blocks, spacing));
            const QString line2 = (q.m_expiry_height > tip_height)
                ? tr("Current quorum has %1 remaining").arg(GUIUtil::formatBlockDuration(q.m_expiry_height - tip_height, spacing))
                : tr("Current quorum has expired");
            const QString line3 = (q.m_newest_height > 0 && tip_height > q.m_newest_height)
                ? tr("Last formed %1 ago").arg(GUIUtil::formatBlockDuration(tip_height - q.m_newest_height, spacing))
                : tr("Last formed recently");
            const QString tooltip = QString("<nobr>%1</nobr><br><nobr>%2</nobr><br><nobr>%3</nobr>").arg(line1, line2, line3);
            it->second.first->setToolTip(tooltip);
            it->second.second->setToolTip(tooltip);
        }
        ++current_row;
    }

    GUIUtil::updateFonts();
}

void NetworkWidget::updateDisplayUnit(BitcoinUnit unit)
{
    m_display_unit = unit;
    ui->labelCrLastBlock->setText(GUIUtil::formatAmount(m_display_unit, m_creditpool_diff, /*is_signed=*/true, /*truncate=*/2));
    ui->labelCrLocked->setText(GUIUtil::formatAmount(m_display_unit, m_creditpool_locked, /*is_signed=*/false, /*truncate=*/2));
    ui->labelCrLimit->setText(GUIUtil::formatAmount(m_display_unit, m_creditpool_limit, /*is_signed=*/false, /*truncate=*/2));
}
