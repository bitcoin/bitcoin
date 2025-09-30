// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/proposalwizard.h>

#include <qt/forms/ui_proposalwizard.h>

#include <governance/object.h>
#include <governance/validators.h>

#include <interfaces/node.h>
#include <qt/bitcoinamountfield.h>
#include <qt/bitcoinunits.h>
#include <qt/guiutil.h>
#include <qt/optionsmodel.h>
#include <qt/qvalidatedlineedit.h>
#include <qt/rpcconsole.h>
#include <qt/walletmodel.h>

#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QStackedWidget>
#include <QTimer>

#include <algorithm>

namespace {
// Minimal helper to encode plain ASCII JSON to hex for RPC
static QString toHex(const QByteArray& bytes) { return QString(bytes.toHex()); }
} // namespace

ProposalWizard::ProposalWizard(interfaces::Node& node, WalletModel* walletModel, QWidget* parent) :
    QDialog(parent),
    m_node(node),
    m_walletModel(walletModel),
    m_ui(new Ui::ProposalWizard)
{
    m_ui->setupUi(this);

    GUIUtil::setFont({m_ui->labelHeader}, GUIUtil::FontWeight::Bold, 16);
    GUIUtil::disableMacFocusRect(this);
    GUIUtil::updateFonts();

    // Prefer the minimum vertical size needed for current content
    if (this->layout()) {
        this->layout()->setSizeConstraint(QLayout::SetMinimumSize);
    }
    this->adjustSize();
    this->setMinimumHeight(this->sizeHint().height());

    // Attach address validators
    GUIUtil::setupAddressWidget(static_cast<QValidatedLineEdit*>(m_ui->editPayAddr), this, /*fAllowURI=*/false);

    // Initialize fields
    // Populate payments dropdown (mainnet 1..12 by default; adjust by network later if needed)
    for (int i = 1; i <= 12; ++i) {
        m_ui->comboPayments->addItem(tr("%1").arg(i), i);
    }
    m_ui->comboPayments->setCurrentIndex(0);

    {
        // Load governance parameters
        const auto info = m_walletModel->node().gov().getGovernanceInfo();
        m_relayRequiredConfs = info.relayRequiredConfs;
        m_requiredConfs = info.requiredConfs;

        // Populate first-payment options by default using governance info
        m_ui->comboFirstPayment->clear();
        const int nextSb = info.nextsuperblock;
        const int cycle = info.superblockcycle;
        for (int i = 0; i < 12; ++i) {
            const int sbHeight = nextSb + i * cycle;
            const qint64 secs = static_cast<qint64>(i) * cycle * Params().GetConsensus().nPowTargetSpacing;
            const auto dt = QDateTime::currentDateTimeUtc().addSecs(secs).toLocalTime();
            m_ui->comboFirstPayment->addItem(QLocale().toString(dt, QLocale::ShortFormat), sbHeight);
        }
    }

    // Initialize total amount display (formatted with current unit)
    updateDisplayUnit();

    // First payment options are populated on load. No separate suggest-times button.
    connect(m_ui->comboPayments, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ProposalWizard::updateLabels);
    connect(m_ui->paymentAmount, &BitcoinAmountField::valueChanged, this, &ProposalWizard::updateLabels);
    connect(m_ui->btnNext1, &QPushButton::clicked, this, &ProposalWizard::onNextFromDetails);
    connect(m_ui->btnBack1, &QPushButton::clicked, this, &ProposalWizard::onBackToDetails);
    connect(m_ui->btnValidate, &QPushButton::clicked, this, &ProposalWizard::onValidateJson);
    connect(m_ui->btnNext2, &QPushButton::clicked, this, &ProposalWizard::onNextFromReview);
    connect(m_ui->btnBack2, &QPushButton::clicked, this, &ProposalWizard::onBackToReview);
    connect(m_ui->btnPrepare, &QPushButton::clicked, this, &ProposalWizard::onPrepare);
    connect(m_ui->btnCopyTxid, &QPushButton::clicked, this, [this]() {
        if (m_ui->editTxid) {
            m_ui->editTxid->selectAll();
            m_ui->editTxid->copy();
        }
    });
    connect(m_ui->btnNext3, &QPushButton::clicked, this, &ProposalWizard::onGoToSubmit);
    connect(m_ui->btnSubmit, &QPushButton::clicked, this, &ProposalWizard::onSubmit);
    connect(m_ui->btnCopyGovId, &QPushButton::clicked, this, [this]() {
        if (m_ui->editGovObjId) {
            m_ui->editGovObjId->selectAll();
            m_ui->editGovObjId->copy();
        }
    });
    connect(m_ui->btnClose, &QPushButton::clicked, this, &QDialog::accept);

    // Update fee labels on display unit change
    if (m_walletModel && m_walletModel->getOptionsModel()) {
        connect(m_walletModel->getOptionsModel(), &OptionsModel::displayUnitChanged, this,
                &ProposalWizard::updateDisplayUnit);
    }

    // Re-compute minimum vertical size when switching pages
    connect(m_ui->stackedWidget, &QStackedWidget::currentChanged, this, [this](int) {
        this->adjustSize();
        this->setMinimumHeight(this->sizeHint().height());
    });
}

ProposalWizard::~ProposalWizard()
{
    delete m_confirmTimer;
    delete m_ui;
}

void ProposalWizard::buildJsonAndHex()
{
    // Compute start/end epochs from selected superblocks
    int start_epoch = 0;
    int end_epoch = 0;
    int firstSb = m_ui->comboFirstPayment->currentData().toInt();
    int payments = m_ui->comboPayments->currentData().toInt();
    if (firstSb > 0 && payments > 0) {
        const int cycle = Params().GetConsensus().nSuperblockCycle;
        if (cycle > 0) {
            const int prevSb = firstSb - cycle;
            const int lastSb = firstSb + (payments - 1) * cycle;
            const int nextAfterLast = lastSb + cycle;
            // Midpoints in blocks; convert roughly to seconds relative to now
            const int startMidBlocks = (firstSb + prevSb) / 2;
            const int endMidBlocks = (lastSb + nextAfterLast) / 2;
            // We don't know absolute time for those heights in GUI; approximate using consensus block time
            // Use now as baseline; this is only to pass validator and give a stable window
            const qint64 now = QDateTime::currentSecsSinceEpoch();
            const int64_t targetSpacing = Params().GetConsensus().nPowTargetSpacing; // seconds
            const int64_t deltaStartBlocks = static_cast<int64_t>(startMidBlocks) - static_cast<int64_t>(firstSb);
            const int64_t deltaEndBlocks = static_cast<int64_t>(endMidBlocks) - static_cast<int64_t>(firstSb);
            // Guard against overflow when multiplying
            const int64_t maxMultiplier = std::numeric_limits<int64_t>::max() / std::max<int64_t>(1, targetSpacing);
            const int64_t clampedDeltaStart = std::clamp<int64_t>(deltaStartBlocks, -maxMultiplier, maxMultiplier);
            const int64_t clampedDeltaEnd = std::clamp<int64_t>(deltaEndBlocks, -maxMultiplier, maxMultiplier);
            const int64_t startOffsetSecs = clampedDeltaStart * targetSpacing;
            const int64_t endOffsetSecs = clampedDeltaEnd * targetSpacing;
            const int64_t startEpoch64 = now + startOffsetSecs;
            const int64_t endEpoch64 = now + endOffsetSecs;
            // Clamp to 32-bit int range used by validator
            start_epoch = static_cast<int>(
                std::clamp<int64_t>(startEpoch64, std::numeric_limits<int>::min(), std::numeric_limits<int>::max()));
            end_epoch = static_cast<int>(
                std::clamp<int64_t>(endEpoch64, std::numeric_limits<int>::min(), std::numeric_limits<int>::max()));
        }
    }

    QJsonObject o;
    o.insert("name", m_ui->editName->text());
    o.insert("payment_address", m_ui->editPayAddr->text());
    const auto formatted = BitcoinUnits::format(BitcoinUnits::Unit::DASH, m_ui->paymentAmount->value(), false,
                                                BitcoinUnits::SeparatorStyle::NEVER);
    o.insert("payment_amount", formatted.toDouble());
    o.insert("url", m_ui->editUrl->text());
    if (start_epoch > 0) o.insert("start_epoch", start_epoch);
    if (end_epoch > 0) o.insert("end_epoch", end_epoch);
    o.insert("type", 1);
    const auto json = QJsonDocument(o).toJson(QJsonDocument::Compact);
    m_ui->plainJson->setPlainText(QString::fromUtf8(json));
    m_hex = toHex(json);
    m_ui->editHex->setText(m_hex);
}

int ProposalWizard::queryConfirmations(const QString& txid)
{
    if (!m_walletModel) return -1;
    interfaces::WalletTxStatus tx_status;
    int num_blocks{};
    int64_t block_time{};
    if (!m_walletModel->wallet().tryGetTxStatus(uint256S(txid.toStdString()), tx_status, num_blocks, block_time)) {
        return -1;
    }
    return tx_status.depth_in_main_chain;
}

void ProposalWizard::onNextFromDetails()
{
    buildJsonAndHex();
    m_ui->stackedWidget->setCurrentIndex(1);
}

void ProposalWizard::onBackToDetails() { m_ui->stackedWidget->setCurrentIndex(0); }

void ProposalWizard::onValidateJson()
{
    buildJsonAndHex();
    CProposalValidator validator(m_hex.toStdString());
    if (validator.Validate()) {
        m_ui->labelValidateStatus->setText(tr("Valid"));
        m_ui->btnNext2->setEnabled(true);
    } else {
        m_ui->labelValidateStatus->setText(tr("Invalid: %1").arg(QString::fromStdString(validator.GetErrorMessages())));
        m_ui->btnNext2->setEnabled(false);
    }
}

void ProposalWizard::onNextFromReview() { m_ui->stackedWidget->setCurrentIndex(2); }

void ProposalWizard::onBackToReview() { m_ui->stackedWidget->setCurrentIndex(1); }

void ProposalWizard::onPrepare()
{
    // Unlock wallet if necessary
    WalletModel::UnlockContext ctx(m_walletModel->requestUnlock());
    if (!ctx.isValid()) return;

    // Confirm burn
    if (QMessageBox::question(this, tr("Burn %1").arg(m_fee_formatted),
                              tr("Burn %1 to create the fee transaction?").arg(m_fee_formatted),
                              QMessageBox::StandardButton::Cancel | QMessageBox::StandardButton::Yes,
                              QMessageBox::StandardButton::Cancel) != QMessageBox::StandardButton::Yes) {
        return;
    }

    const int64_t now = QDateTime::currentSecsSinceEpoch();
    std::string txid_str;
    std::string error;
    COutPoint none; // null by default

    // TODO: VALIDATE HERE IF blockchain synced
    // instead
    // do something like clientModel->masternodeSync().isBlockchainSynced()
    auto govobj = m_walletModel->node().gov().createProposal(1, now, m_hex.toStdString(), error);
    if (!govobj) {
        QMessageBox::critical(this, tr("Prepare failed"), QString::fromStdString(error));
        return;
    }
    if (!m_walletModel->wallet().prepareProposal(govobj->GetHash(), govobj->GetMinCollateralFee(), 1, now,
                                                 m_hex.toStdString(), none, txid_str, error)) {
        QMessageBox::critical(this, tr("Prepare failed"), QString::fromStdString(error));
        return;
    }
    m_txid = QString::fromStdString(txid_str);
    m_ui->editTxid->setText(m_txid);
    m_ui->btnPrepare->setEnabled(false);
    m_prepareTime = now;

    // Start polling confirmations every 10s
    if (!m_confirmTimer) {
        m_confirmTimer = new QTimer(this);
        connect(m_confirmTimer, &QTimer::timeout, this, &ProposalWizard::onMaybeAdvanceAfterConfirmations);
    }
    m_confirmTimer->start(10000);
    // Update labels right away too
    onMaybeAdvanceAfterConfirmations();
}

void ProposalWizard::onMaybeAdvanceAfterConfirmations()
{
    if (m_txid.isEmpty()) return;
    const int confs = queryConfirmations(m_txid);
    if (confs >= 0 && confs != m_lastConfs) {
        m_lastConfs = confs;
        const int bounded = std::min(confs, m_requiredConfs);
        m_ui->progressConfirmations->setMaximum(m_requiredConfs);
        m_ui->progressConfirmations->setValue(bounded);
        m_ui->labelConfStatus->setText(tr("Confirmations: %1 / %2 required").arg(bounded).arg(m_requiredConfs));
        // Mirror to submit page
        m_ui->progressConfirmations2->setMaximum(m_requiredConfs);
        m_ui->progressConfirmations2->setValue(bounded);
        m_ui->labelConfStatus2->setText(tr("Confirmations: %1 / %2 required").arg(bounded).arg(m_requiredConfs));
        // Simple ETA: 2.5 min per block remaining
        const int remaining = std::max(0, m_requiredConfs - bounded);
        const int secs = remaining * 150;
        if (remaining == 0) {
            m_ui->labelEta->setText(tr("Estimated time remaining: Ready"));
            m_ui->labelEta2->setText(tr("Estimated time remaining: Ready"));
            if (m_confirmTimer) m_confirmTimer->stop();
        } else {
            const auto mins = (secs + 59) / 60;
            m_ui->labelEta->setText(tr("Estimated time remaining: %1 min").arg(mins));
            m_ui->labelEta2->setText(tr("Estimated time remaining: %1 min").arg(mins));
        }
    }
    // Allow submitting (relay/postpone) at 1 confirmation and enable Next to proceed
    if (confs >= m_relayRequiredConfs) {
        m_ui->btnSubmit->setEnabled(true);
        m_ui->btnNext3->setEnabled(true);
    }
    // No auto-advance; user controls when to proceed
}

void ProposalWizard::onSubmit()
{
    // Submit with same parent/revision/time and hex, including fee txid
    const int64_t now = (m_prepareTime > 0 ? m_prepareTime : QDateTime::currentSecsSinceEpoch());
    std::string res;
    if (m_submitted) {
        QMessageBox::information(this, tr("Already submitted"), tr("This proposal has already been submitted."));
        return;
    }
    std::string error;
    std::string obj_hash;
    if (!m_walletModel->node().gov().submitProposal(uint256(), 1, now, m_hex.toStdString(),
                                                    uint256S(m_txid.toStdString()), obj_hash, error)) {
        QMessageBox::critical(this, tr("Submission failed"), QString::fromStdString(error));
        return;
    }
    const QString govId = QString::fromStdString(obj_hash);
    m_ui->editGovObjId->setText(govId);
    QMessageBox::information(this, tr("Proposal submitted"),
                             tr("Your proposal was submitted successfully.\nID: %1").arg(govId));
    m_submitted = true;
    m_ui->btnSubmit->setEnabled(false);
    // When 6 confs are reached show a final success message
}

void ProposalWizard::closeEvent(QCloseEvent* event)
{
    if (m_confirmTimer) m_confirmTimer->stop();
    QDialog::closeEvent(event);
}

void ProposalWizard::onGoToSubmit()
{
    // Only allow entering the submit step if we have a prepared txid and at least relay confirmations
    if (m_txid.isEmpty()) return;
    if (m_lastConfs < m_relayRequiredConfs) return;
    m_ui->stackedWidget->setCurrentIndex(3);
}

void ProposalWizard::updateLabels()
{
    if (m_walletModel && m_walletModel->getOptionsModel()) {
        const auto unit = m_walletModel->getOptionsModel()->getDisplayUnit();
        const CAmount totalAmount = static_cast<CAmount>(m_ui->paymentAmount->value() *
                                                         m_ui->comboPayments->currentData().toInt());
        m_ui->labelTotalValue->setText(
            BitcoinUnits::formatWithUnit(unit, totalAmount, false, BitcoinUnits::SeparatorStyle::ALWAYS));
        m_fee_formatted = BitcoinUnits::formatWithUnit(unit, GOVERNANCE_PROPOSAL_FEE_TX, false,
                                                       BitcoinUnits::SeparatorStyle::ALWAYS);
        m_ui->labelFeeValue->setText(m_fee_formatted.isEmpty() ? QString("-") : m_fee_formatted);
        // Dynamic header/subheader and prepare text
        if (m_ui->labelSubheader) {
            m_ui->labelSubheader->setText(
                tr("A fee of %1 will be burned when you prepare the proposal.").arg(m_fee_formatted));
        }
        if (m_ui->labelPrepare) {
            m_ui->labelPrepare->setText(
                tr("Prepare (burn %1) and wait for %2 confirmations.").arg(m_fee_formatted).arg(m_requiredConfs));
        }
    }
}

void ProposalWizard::updateDisplayUnit()
{
    if (m_walletModel && m_walletModel->getOptionsModel()) {
        m_ui->paymentAmount->setDisplayUnit(m_walletModel->getOptionsModel()->getDisplayUnit());
    }
    updateLabels();
}
