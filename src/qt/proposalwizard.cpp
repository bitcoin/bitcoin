// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/proposalwizard.h>

#include <qt/forms/ui_proposalwizard.h>

#include <qt/guiutil.h>
#include <qt/rpcconsole.h>
#include <interfaces/node.h>
#include <qt/walletmodel.h>
#include <qt/bitcoinunits.h>
#include <qt/optionsmodel.h>
#include <governance/object.h>
#include <governance/validators.h>

#include <QDateTime>
#include <QDateTimeEdit>
#include <QDoubleSpinBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <qt/qvalidatedlineedit.h>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QStackedWidget>
#include <QTimer>

namespace {
// Minimal helper to encode plain ASCII JSON to hex for RPC
static QString toHex(const QByteArray& bytes)
{
    return QString(bytes.toHex());
}
}

ProposalWizard::ProposalWizard(interfaces::Node& node, WalletModel* walletModel, QWidget* parent)
    : QDialog(parent)
    , m_node(node)
    , m_walletModel(walletModel)
{
    m_ui = std::make_unique<Ui::ProposalWizard>();
    auto* ui = m_ui.get();
    ui->setupUi(this);

    // Prefer the minimum vertical size needed for current content
    if (this->layout()) {
        this->layout()->setSizeConstraint(QLayout::SetMinimumSize);
    }
    this->adjustSize();
    this->setMinimumHeight(this->sizeHint().height());

    // Bind UI pointers
    stacked = ui->stackedWidget;
    editName = ui->editName;
    editUrl = ui->editUrl;
    editPayAddr = ui->editPayAddr;
    // Attach address validators
    GUIUtil::setupAddressWidget(static_cast<QValidatedLineEdit*>(editPayAddr), this, /*fAllowURI=*/false);
    comboFirstPayment = ui->comboFirstPayment;
    comboPayments = ui->comboPayments;
    spinAmount = ui->spinAmount;
    labelFeeValue = ui->labelFeeValue;
    labelTotalValue = ui->labelTotalValue;
    labelSubheader = ui->labelSubheader;
    btnNext1 = ui->btnNext1;

    plainJson = ui->plainJson;
    editHex = ui->editHex;
    labelValidateStatus = ui->labelValidateStatus;
    btnNext2 = ui->btnNext2;

    // Txid widgets
    labelTxid = ui->labelTxidCaption;
    editTxid = ui->editTxid;
    QPushButton* btnCopyTxid = ui->btnCopyTxid;
    labelConfStatus = ui->labelConfStatus;
    labelEta = ui->labelEta;
    progressConfirmations = ui->progressConfirmations;
    // Submit page mirrors
    labelConfStatus2 = ui->labelConfStatus2;
    labelEta2 = ui->labelEta2;
    progressConfirmations2 = ui->progressConfirmations2;
    btnNext3 = ui->btnNext3;

    editGovObjId = ui->editGovObjId;
    btnCopyGovId = ui->btnCopyGovId;
    btnSubmit = ui->btnSubmit;
    labelPrepare = ui->labelPrepare;

    // Initialize fields
    // Populate payments dropdown (mainnet 1..12 by default; adjust by network later if needed)
    for (int i = 1; i <= 12; ++i) {
        comboPayments->addItem(tr("%1").arg(i), i);
    }
    comboPayments->setCurrentIndex(0);

    // Load proposal fee and set dynamic labels using display unit
    auto updateFeeAndLabels = [this]() {
        const auto info = m_walletModel->node().gov().getGovernanceInfo();
        const CAmount fee_amount = info.proposalfee;
        const auto unit = m_walletModel && m_walletModel->getOptionsModel() ? m_walletModel->getOptionsModel()->getDisplayUnit() : BitcoinUnit::DASH;
        const QString feeFormatted = BitcoinUnits::formatWithUnit(unit, fee_amount, false, BitcoinUnits::SeparatorStyle::ALWAYS);
        labelFeeValue->setText(feeFormatted.isEmpty() ? QString("-") : feeFormatted);
        // Dynamic header/subheader and prepare text
        if (labelSubheader) labelSubheader->setText(tr("A fee of %1 will be burned when you prepare the proposal.").arg(feeFormatted));
        if (labelPrepare) labelPrepare->setText(tr("Prepare (burn %1) and wait for %2 confirmations.")
                                      .arg(feeFormatted)
                                      .arg(GOVERNANCE_FEE_CONFIRMATIONS));
        // If we ever decide to show unit suffix in the spin box, do it dynamically here
        // const QString unitName = BitcoinUnits::name(unit);
        // spinAmount->setSuffix(" " + unitName);
    };
    updateFeeAndLabels();

    // Populate first-payment options by default using governance info
    {
        const auto info = m_walletModel->node().gov().getGovernanceInfo();
        comboFirstPayment->clear();
        const int nextSb = info.nextsuperblock;
        const int cycle = info.superblockcycle;
        for (int i = 0; i < 12; ++i) {
            const int sbHeight = nextSb + i * cycle;
            const qint64 secs = static_cast<qint64>(i) * cycle * Params().GetConsensus().nPowTargetSpacing;
            const auto dt = QDateTime::currentDateTimeUtc().addSecs(secs).toLocalTime();
            comboFirstPayment->addItem(QLocale().toString(dt, QLocale::ShortFormat), sbHeight);
        }
    }

    // Initialize total amount display (formatted with current unit)
    {
        const auto unit = m_walletModel && m_walletModel->getOptionsModel() ? m_walletModel->getOptionsModel()->getDisplayUnit() : BitcoinUnit::DASH;
        const CAmount totalAmount = static_cast<CAmount>(spinAmount->value() * comboPayments->currentData().toInt() * COIN);
        labelTotalValue->setText(BitcoinUnits::formatWithUnit(unit, totalAmount, false, BitcoinUnits::SeparatorStyle::ALWAYS));
    }

    // First payment options are populated on load. No separate suggest-times button.
    connect(comboPayments, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int){
        const auto unit = m_walletModel && m_walletModel->getOptionsModel() ? m_walletModel->getOptionsModel()->getDisplayUnit() : BitcoinUnit::DASH;
        const CAmount totalAmount = static_cast<CAmount>(spinAmount->value() * comboPayments->currentData().toInt() * COIN);
        labelTotalValue->setText(BitcoinUnits::formatWithUnit(unit, totalAmount, false, BitcoinUnits::SeparatorStyle::ALWAYS));
    });
    connect(spinAmount, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double){
        const auto unit = m_walletModel && m_walletModel->getOptionsModel() ? m_walletModel->getOptionsModel()->getDisplayUnit() : BitcoinUnit::DASH;
        const CAmount totalAmount = static_cast<CAmount>(spinAmount->value() * comboPayments->currentData().toInt() * COIN);
        labelTotalValue->setText(BitcoinUnits::formatWithUnit(unit, totalAmount, false, BitcoinUnits::SeparatorStyle::ALWAYS));
    });
    connect(btnNext1, &QPushButton::clicked, this, &ProposalWizard::onNextFromDetails);
    connect(ui->btnBack1, &QPushButton::clicked, this, &ProposalWizard::onBackToDetails);
    connect(ui->btnValidate, &QPushButton::clicked, this, &ProposalWizard::onValidateJson);
    connect(btnNext2, &QPushButton::clicked, this, &ProposalWizard::onNextFromReview);
    connect(ui->btnBack2, &QPushButton::clicked, this, &ProposalWizard::onBackToReview);
    btnPrepare = ui->btnPrepare;
    connect(btnPrepare, &QPushButton::clicked, this, &ProposalWizard::onPrepare);
    connect(btnCopyTxid, &QPushButton::clicked, this, [this]() { if (editTxid) { editTxid->selectAll(); editTxid->copy(); } });
    connect(btnNext3, &QPushButton::clicked, this, &ProposalWizard::onGoToSubmit);
    connect(ui->btnSubmit, &QPushButton::clicked, this, &ProposalWizard::onSubmit);
    connect(btnCopyGovId, &QPushButton::clicked, this, [this]() { if (editGovObjId) { editGovObjId->selectAll(); editGovObjId->copy(); } });
    connect(ui->btnClose, &QPushButton::clicked, this, &QDialog::accept);

    // Update fee labels on display unit change
    if (m_walletModel && m_walletModel->getOptionsModel()) {
        connect(m_walletModel->getOptionsModel(), &OptionsModel::displayUnitChanged, this, [this, updateFeeAndLabels]() {
            updateFeeAndLabels();
            const auto unit = m_walletModel->getOptionsModel()->getDisplayUnit();
            const CAmount totalAmount = static_cast<CAmount>(spinAmount->value() * comboPayments->currentData().toInt() * COIN);
            labelTotalValue->setText(BitcoinUnits::formatWithUnit(unit, totalAmount, false, BitcoinUnits::SeparatorStyle::ALWAYS));
        });
    }

    // Re-compute minimum vertical size when switching pages
    connect(stacked, &QStackedWidget::currentChanged, this, [this](int){
        this->adjustSize();
        this->setMinimumHeight(this->sizeHint().height());
    });
}

ProposalWizard::~ProposalWizard() = default;

void ProposalWizard::buildJsonAndHex()
{
    // Compute start/end epochs from selected superblocks
    int start_epoch = 0;
    int end_epoch = 0;
    int firstSb = comboFirstPayment->currentData().toInt();
    int payments = comboPayments->currentData().toInt();
    if (firstSb > 0 && payments > 0) {
        const auto info = m_walletModel->node().gov().getGovernanceInfo();
        const int cycle = info.superblockcycle;
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
            start_epoch = static_cast<int>(std::clamp<int64_t>(startEpoch64, std::numeric_limits<int>::min(), std::numeric_limits<int>::max()));
            end_epoch = static_cast<int>(std::clamp<int64_t>(endEpoch64, std::numeric_limits<int>::min(), std::numeric_limits<int>::max()));
        }
    }

    QJsonObject o;
    o.insert("name", editName->text());
    o.insert("payment_address", editPayAddr->text());
    o.insert("payment_amount", spinAmount->value());
    o.insert("url", editUrl->text());
    if (start_epoch > 0) o.insert("start_epoch", start_epoch);
    if (end_epoch > 0) o.insert("end_epoch", end_epoch);
    o.insert("type", 1);
    const auto json = QJsonDocument(o).toJson(QJsonDocument::Compact);
    plainJson->setPlainText(QString::fromUtf8(json));
    m_hex = toHex(json);
    editHex->setText(m_hex);
}

int ProposalWizard::queryConfirmations(const QString& txid)
{
    if (!m_walletModel) return -1;
    interfaces::WalletTxStatus tx_status;
    int num_blocks{}; int64_t block_time{};
    if (!m_walletModel->wallet().tryGetTxStatus(uint256S(txid.toStdString()), tx_status, num_blocks, block_time)) return -1;
    return tx_status.depth_in_main_chain;
}

void ProposalWizard::onNextFromDetails()
{
    buildJsonAndHex();
    stacked->setCurrentIndex(1);
}

void ProposalWizard::onBackToDetails()
{
    stacked->setCurrentIndex(0);
}

void ProposalWizard::onValidateJson()
{
    buildJsonAndHex();
    CProposalValidator validator(m_hex.toStdString());
    if (validator.Validate()) {
        labelValidateStatus->setText(tr("Valid"));
        btnNext2->setEnabled(true);
    } else {
        labelValidateStatus->setText(tr("Invalid: %1").arg(QString::fromStdString(validator.GetErrorMessages())));
        btnNext2->setEnabled(false);
    }
}

void ProposalWizard::onNextFromReview()
{
    stacked->setCurrentIndex(2);
}

void ProposalWizard::onBackToReview()
{
    stacked->setCurrentIndex(1);
}

void ProposalWizard::onPrepare()
{
    // Unlock wallet if necessary
    WalletModel::UnlockContext ctx(m_walletModel->requestUnlock());
    if (!ctx.isValid()) return;

    // Confirm burn
    if (QMessageBox::question(this,
                              tr("Burn 1 DASH"),
                              tr("Burn 1 DASH to create the fee transaction?"),
                              QMessageBox::StandardButton::Cancel | QMessageBox::StandardButton::Yes,
                              QMessageBox::StandardButton::Cancel) != QMessageBox::StandardButton::Yes) {
        return;
    }

    const int64_t now = QDateTime::currentSecsSinceEpoch();
    std::string txid_str; std::string error;
    COutPoint none; // null by default
    if (!m_walletModel->node().gov().prepareProposal(m_walletModel->wallet(), uint256(), 1, now, m_hex.toStdString(), none, txid_str, error)) {
        QMessageBox::critical(this, tr("Prepare failed"), QString::fromStdString(error));
        return;
    }
    m_txid = QString::fromStdString(txid_str);
    editTxid->setText(m_txid);
    btnPrepare->setEnabled(false);
    m_prepareTime = now;

    // Start polling confirmations every 10s
    if (!m_confirmTimer) {
        m_confirmTimer = new QTimer(this);
        connect(m_confirmTimer, &QTimer::timeout, this, &ProposalWizard::onMaybeAdvanceAfterConfirmations);
    }
    m_confirmTimer->start(10000);
}

void ProposalWizard::onMaybeAdvanceAfterConfirmations()
{
    if (m_txid.isEmpty()) return;
    const int confs = queryConfirmations(m_txid);
    if (confs >= 0 && confs != m_lastConfs) {
        m_lastConfs = confs;
        const int bounded = std::min(confs, m_requiredConfs);
        progressConfirmations->setMaximum(m_requiredConfs);
        progressConfirmations->setValue(bounded);
        labelConfStatus->setText(tr("Confirmations: %1 / %2 required").arg(bounded).arg(m_requiredConfs));
        // Mirror to submit page
        progressConfirmations2->setMaximum(m_requiredConfs);
        progressConfirmations2->setValue(bounded);
        labelConfStatus2->setText(tr("Confirmations: %1 / %2 required").arg(bounded).arg(m_requiredConfs));
        // Simple ETA: 2.5 min per block remaining
        const int remaining = std::max(0, m_requiredConfs - bounded);
        const int secs = remaining * 150;
        if (remaining == 0) {
            labelEta->setText(tr("Estimated time remaining: Ready"));
            labelEta2->setText(tr("Estimated time remaining: Ready"));
            if (m_confirmTimer) m_confirmTimer->stop();
        } else {
            const auto mins = (secs + 59) / 60;
            labelEta->setText(tr("Estimated time remaining: %1 min").arg(mins));
            labelEta2->setText(tr("Estimated time remaining: %1 min").arg(mins));
        }
    }
    // Allow submitting (relay/postpone) at 1 confirmation and enable Next to proceed
    if (confs >= m_relayRequiredConfs) {
        btnSubmit->setEnabled(true);
        btnNext3->setEnabled(true);
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
    if (!m_walletModel->node().gov().submitProposal(uint256(), 1, now, m_hex.toStdString(), uint256S(m_txid.toStdString()), obj_hash, error)) {
        QMessageBox::critical(this, tr("Submission failed"), QString::fromStdString(error));
        return;
    }
    const QString govId = QString::fromStdString(obj_hash);
    editGovObjId->setText(govId);
    QMessageBox::information(this, tr("Proposal submitted"), tr("Your proposal was submitted successfully.\nID: %1").arg(govId));
    m_submitted = true;
    btnSubmit->setEnabled(false);
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
    stacked->setCurrentIndex(3);
}


