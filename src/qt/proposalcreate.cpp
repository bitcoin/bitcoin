// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/proposalcreate.h>
#include <qt/forms/ui_proposalcreate.h>

#include <governance/object.h>
#include <governance/validators.h>
#include <interfaces/node.h>
#include <util/moneystr.h>
#include <util/strencodings.h>

#include <qt/descriptiondialog.h>
#include <qt/guiutil_font.h>

#include <qt/bitcoinamountfield.h>
#include <qt/bitcoinunits.h>
#include <qt/guiutil.h>
#include <qt/optionsmodel.h>
#include <qt/qvalidatedlineedit.h>
#include <qt/sendcoinsdialog.h>
#include <qt/walletmodel.h>

#include <QDateTime>
#include <QLineEdit>
#include <QMessageBox>
#include <QSpinBox>

#include <univalue.h>

#include <algorithm>

ProposalCreate::ProposalCreate(WalletModel* walletModel, QWidget* parent) :
    QDialog(parent),
    m_walletModel(walletModel),
    m_ui(new Ui::ProposalCreate)
{
    m_ui->setupUi(this);
    m_ui->labelError->setStyleSheet(GUIUtil::getThemedStyleQString(GUIUtil::ThemedStyle::TS_ERROR));
    m_ui->labelTotalValue->setFont(
        GUIUtil::getScaledFont(GUIUtil::FontRegistry::DEFAULT_FONT_SIZE, /*bold=*/true, /*multiplier=*/1.05));

    // Allow payment amount field to stretch horizontally
    if (auto* lineEdit = m_ui->paymentAmount->findChild<QLineEdit*>()) {
        lineEdit->setMaximumWidth(QWIDGETSIZE_MAX);
    }

    // Attach address validators
    GUIUtil::setupAddressWidget(static_cast<QValidatedLineEdit*>(m_ui->editPayAddr), this, /*fAllowURI=*/false);
    {
        // Load governance parameters
        const auto info = m_walletModel->node().gov().getGovernanceInfo();
        m_relay_confs = info.relayRequiredConfs;
        m_superblock_cycle = info.superblockcycle;
        m_target_spacing = info.targetSpacing;

        // Populate first-payment options by default using governance info
        m_ui->comboFirstPayment->clear();
        const int nextSb = info.nextsuperblock;
        const int cycle = info.superblockcycle;
        for (int i = 0; i < 12; ++i) {
            const int sbHeight = nextSb + i * cycle;
            const qint64 secs = static_cast<qint64>(i) * cycle * m_target_spacing;
            const auto dt = QDateTime::currentDateTimeUtc().addSecs(secs).toLocalTime();
            m_ui->comboFirstPayment->addItem(QLocale().toString(dt, QLocale::ShortFormat), sbHeight);
        }
    }

    // Initialize total amount display (formatted with current unit)
    updateDisplayUnit();

    connect(m_ui->btnViewJson, &QPushButton::clicked, this, &ProposalCreate::onViewJson);
    connect(m_ui->btnViewPayload, &QPushButton::clicked, this, &ProposalCreate::onViewPayload);
    connect(m_ui->editName, &QLineEdit::textChanged, this, &ProposalCreate::validateFields);
    connect(m_ui->editUrl, &QLineEdit::textChanged, this, &ProposalCreate::validateFields);
    connect(m_ui->editPayAddr, &QLineEdit::textChanged, this, &ProposalCreate::validateFields);
    connect(m_ui->spinPayments, QOverload<int>::of(&QSpinBox::valueChanged), this, &ProposalCreate::updateLabels);
    connect(m_ui->paymentAmount, &BitcoinAmountField::valueChanged, this, &ProposalCreate::updateLabels);
    connect(m_ui->paymentAmount, &BitcoinAmountField::valueChanged, this, &ProposalCreate::validateFields);
    connect(m_ui->btnCreate, &QPushButton::clicked, this, &ProposalCreate::onCreate);

    // Update fee labels on display unit change
    if (m_walletModel && m_walletModel->getOptionsModel()) {
        connect(m_walletModel->getOptionsModel(), &OptionsModel::displayUnitChanged, this,
                &ProposalCreate::updateDisplayUnit);
    }

    GUIUtil::disableMacFocusRect(this);
    GUIUtil::updateFonts();
    setFixedSize(size());
}

ProposalCreate::~ProposalCreate()
{
    delete m_ui;
}

void ProposalCreate::buildJsonAndHex()
{
    const int64_t multiplier = std::numeric_limits<int64_t>::max() / std::max<int64_t>(1, m_target_spacing);

    // Compute start/end epochs from selected superblocks
    int64_t start_epoch = 0;
    int64_t end_epoch = 0;
    const int64_t first_sb = m_ui->comboFirstPayment->currentData().toInt();
    const int64_t payments = m_ui->spinPayments->value();
    if (first_sb > 0 && payments > 0) {
        if (m_superblock_cycle > 0) {
            const int64_t prev_sb{first_sb - m_superblock_cycle};
            const int64_t last_sb{first_sb + (payments - 1) * m_superblock_cycle};
            const int64_t next_sb{last_sb + m_superblock_cycle};
            // Midpoints in blocks; convert roughly to seconds relative to now
            const int64_t start_blocks{(first_sb + prev_sb) / 2};
            const int64_t end_blocks{(last_sb + next_sb) / 2};
            // We don't know absolute time for those heights in GUI; approximate using consensus block time
            // Use now as baseline; this is only to pass validator and give a stable window
            const qint64 now{QDateTime::currentSecsSinceEpoch()};
            const int64_t delta_start{start_blocks - first_sb};
            const int64_t delta_end{end_blocks - first_sb};
            // Guard against overflow when multiplying by clamping deltas first
            start_epoch = now + std::clamp<int64_t>(delta_start, -multiplier, multiplier) * m_target_spacing;
            end_epoch = now + std::clamp<int64_t>(delta_end, -multiplier, multiplier) * m_target_spacing;
        }
    }

    UniValue o(UniValue::VOBJ);
    o.pushKV("name", m_ui->editName->text().toStdString());
    o.pushKV("payment_address", m_ui->editPayAddr->text().toStdString());
    UniValue amount;
    amount.setNumStr(FormatMoney(m_ui->paymentAmount->value()));
    o.pushKV("payment_amount", amount);
    o.pushKV("url", m_ui->editUrl->text().toStdString());
    if (start_epoch > 0) o.pushKV("start_epoch", start_epoch);
    if (end_epoch > 0) o.pushKV("end_epoch", end_epoch);
    o.pushKV("type", 1);
    const auto json{o.write()};
    m_json = QString::fromStdString(json);
    m_hex = QString::fromStdString(HexStr(json));
}

void ProposalCreate::onViewJson()
{
    buildJsonAndHex();
    const QString html = QString("<code style=\"white-space: pre-wrap;\">%1</code>").arg(m_json.toHtmlEscaped());
    DescriptionDialog* dlg = new DescriptionDialog("", html, this);
    dlg->resize(700, 300);
    dlg->setWindowModality(Qt::WindowModal);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->show();
}

void ProposalCreate::onViewPayload()
{
    buildJsonAndHex();
    const QString html = QString("<code style=\"word-wrap: break-word;\">%1</code>").arg(m_hex.toHtmlEscaped());
    DescriptionDialog* dlg = new DescriptionDialog("", html, this);
    dlg->resize(700, 300);
    dlg->setWindowModality(Qt::WindowModal);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->show();
}

void ProposalCreate::onCreate()
{
    // Validate fields first
    if (m_ui->editName->text().trimmed().isEmpty() || m_ui->editUrl->text().trimmed().isEmpty() ||
        m_ui->editPayAddr->text().trimmed().isEmpty() || m_ui->paymentAmount->value() <= 0)
    {
        m_ui->labelError->setText(tr("All fields are mandatory"));
        return;
    }
    validateFields();
    if (!m_ui->labelError->text().isEmpty()) {
        return; // Don't proceed if there are errors
    }

    buildJsonAndHex();

    // Unlock wallet if necessary
    WalletModel::UnlockContext ctx(m_walletModel->requestUnlock());
    if (!ctx.isValid()) return;

    SendConfirmationDialog diag_confirm(
        /*title=*/tr("Confirm Proposal"),
        /*text=*/tr("Are you sure you want to create this proposal?"),
        /*informative_text=*/tr("Creating a proposal pays %1 to the network. This fee is non-refundable regardless of outcome.").arg(m_fee_formatted),
        /*detailed_text=*/"", SEND_CONFIRM_DELAY,
        /*enable_send=*/true, /*always_show_unsigned=*/false, /*parent=*/this);
    diag_confirm.setWindowModality(Qt::WindowModal);
    diag_confirm.exec();
    if (diag_confirm.result() != QMessageBox::Yes) {
        return;
    }

    const int64_t now = QDateTime::currentSecsSinceEpoch();
    std::string txid_str;
    std::string error;
    COutPoint none; // null by default

    auto govobj = m_walletModel->node().gov().createProposal(1, now, m_hex.toStdString(), error);
    if (!govobj || !m_walletModel->wallet().prepareProposal(govobj->GetHash(), govobj->GetMinCollateralFee(), 1, now,
                                                            m_hex.toStdString(), none, txid_str, error))
    {
        QMessageBox::critical(this, tr("Creation failed"), QString::fromStdString(error));
        return;
    }

    QMessageBox::information(this, tr("Proposal Created"),
        tr("%1 successfully sent for your proposal \"%2\".\n\n"
           "You will now be redirected to monitor and broadcast your new proposal, "
           "you can resume this later by clicking \"Resume Proposal\".")
            .arg(m_fee_formatted)
            .arg(m_ui->editName->text()));

    accept(); // Close the wizard
}

void ProposalCreate::updateLabels()
{
    if (m_walletModel && m_walletModel->getOptionsModel()) {
        const auto unit = m_walletModel->getOptionsModel()->getDisplayUnit();
        const CAmount per_payment = m_ui->paymentAmount->value();
        const int payments = m_ui->spinPayments->value();
        CAmount total{0};
        if (payments > 0 && per_payment > 0 && per_payment <= MAX_MONEY / payments) {
            total = per_payment * payments;
        } else if (payments > 0 && per_payment > 0) {
            total = MAX_MONEY;
        }
        m_ui->labelTotalValue->setText(
            BitcoinUnits::formatWithUnit(unit, total, /*plussign=*/false, BitcoinUnits::SeparatorStyle::ALWAYS));
        m_fee_formatted = BitcoinUnits::formatWithUnit(unit, GOVERNANCE_PROPOSAL_FEE_TX, /*plussign=*/false,
                                                       BitcoinUnits::SeparatorStyle::ALWAYS);
    }
}

void ProposalCreate::updateDisplayUnit()
{
    if (m_walletModel && m_walletModel->getOptionsModel()) {
        m_ui->paymentAmount->setDisplayUnit(m_walletModel->getOptionsModel()->getDisplayUnit());
    }
    updateLabels();
}

void ProposalCreate::validateFields()
{
    if (m_ui->editName->text().trimmed().isEmpty() && m_ui->editUrl->text().trimmed().isEmpty() &&
        m_ui->editPayAddr->text().trimmed().isEmpty() && m_ui->paymentAmount->value() == 0)
    {
        // If all fields are empty, clear the error label
        m_ui->labelError->clear();
        return;
    }

    buildJsonAndHex();
    CProposalValidator validator(m_hex.toStdString());
    if (validator.Validate()) {
        m_ui->labelError->clear();
    } else {
        // Show first error only
        QString errors = QString::fromStdString(validator.GetErrorMessages());
        if (int semicolon = errors.indexOf(';'); semicolon > 0) {
            errors = errors.left(semicolon);
        }
        m_ui->labelError->setText(errors);
    }
}
