// Copyright (c) 2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/masternodedialogs.h>

#include <bls/bls.h>
#include <chainparams.h>
#include <evo/providertx.h>
#include <evo/types.h>
#include <interfaces/node.h>
#include <interfaces/wallet.h>
#include <key_io.h>
#include <script/standard.h>
#include <support/cleanse.h>
#include <uint256.h>
#include <util/strencodings.h>

#include <qt/guiutil.h>
#include <qt/masternodemodel.h>
#include <qt/masternodewidgets.h>
#include <qt/qvalidatedlineedit.h>
#include <qt/walletmodel.h>

#include <QApplication>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSizePolicy>
#include <QSpinBox>
#include <QStringList>
#include <QVBoxLayout>

#include <limits>
#include <optional>
#include <string>
#include <utility>

namespace {

bool IsHexString(const QString& text, int length) { return text.size() == length && IsHex(text.toStdString()); }

bool IsValidPlatformNodeId(const QString& text)
{
    if (!IsHexString(text, 40)) return false;
    uint160 platform_node_id;
    platform_node_id.SetHex(text.toStdString());
    return !platform_node_id.IsNull();
}

bool IsNonNullP2PKHAddress(const QString& text)
{
    const CTxDestination destination{DecodeDestination(text.trimmed().toStdString())};
    const auto* key_id{std::get_if<PKHash>(&destination)};
    return key_id != nullptr && !ToKeyID(*key_id).IsNull();
}

uint16_t PortOrDefault(const QStringList& endpoints, uint16_t default_port)
{
    if (endpoints.isEmpty()) return default_port;
    uint16_t port{0};
    std::string host;
    if (SplitHostPort(endpoints.first().trimmed().toStdString(), port, host) && !host.empty() && port != 0) return port;
    return default_port;
}

std::optional<CTxDestination> ParseDestination(const QString& text, QString& error)
{
    const CTxDestination destination{DecodeDestination(text.trimmed().toStdString())};
    if (!IsValidDestination(destination)) {
        error = QObject::tr("Invalid Dash address: %1").arg(text);
        return std::nullopt;
    }
    return destination;
}

std::vector<std::string> ToStringVector(const QStringList& entries)
{
    std::vector<std::string> result;
    result.reserve(entries.size());
    for (const QString& entry : entries)
        result.push_back(entry.toStdString());
    return result;
}

bool OperatorKeyMatches(const CBLSSecretKey& secret_key, const std::vector<unsigned char>& public_key)
{
    return secret_key.IsValid() && !public_key.empty() &&
           secret_key.GetPublicKey().ToByteVector(/*specificLegacyScheme=*/false) == public_key;
}

} // anonymous namespace

namespace MasternodeMaintenance {

ActionAvailability actionAvailability(bool can_sign, bool owns_owner_key)
{
    return {
        /*update_service=*/can_sign,
        /*update_registrar=*/can_sign && owns_owner_key,
        /*revoke=*/can_sign,
    };
}

} // namespace MasternodeMaintenance

OperatorSecretWidget::OperatorSecretWidget(QWidget* parent) :
    QWidget(parent)
{
    m_manual_edit = new QLineEdit(this);
    m_manual_edit->setEchoMode(QLineEdit::Password);
    m_manual_edit->setPlaceholderText(tr("Operator BLS secret key (64 hexadecimal characters)"));

    auto* layout{new QVBoxLayout(this)};
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_manual_edit);
    connect(m_manual_edit, &QLineEdit::textChanged, this, &OperatorSecretWidget::changed);
}

bool OperatorSecretWidget::isValid() const
{
    std::string encoded{m_manual_edit->text().trimmed().toStdString()};
    CBLSSecretKey key;
    const bool valid{key.SetHexStr(encoded, /*specificLegacyScheme=*/false)};
    if (!encoded.empty()) memory_cleanse(encoded.data(), encoded.size());
    return valid;
}

std::optional<CBLSSecretKey> OperatorSecretWidget::takeSecret()
{
    std::string encoded{m_manual_edit->text().trimmed().toStdString()};
    // QLineEdit does not offer secure storage. Overwrite its visible backing
    // value before clearing it, and cleanse the explicit byte copy below.
    m_manual_edit->setText(QString(m_manual_edit->text().size(), QLatin1Char('0')));
    m_manual_edit->clear();

    CBLSSecretKey key;
    const bool valid{key.SetHexStr(encoded, /*specificLegacyScheme=*/false)};
    if (!encoded.empty()) memory_cleanse(encoded.data(), encoded.size());
    if (!valid) return std::nullopt;
    return key;
}

struct MasternodeActionDialog::UnlockHolder {
    WalletModel::UnlockContext ctx;
    explicit UnlockHolder(WalletModel& wallet_model) :
        ctx(wallet_model.requestUnlock())
    {
    }
};

MasternodeActionDialog::MasternodeActionDialog(interfaces::Node& node, WalletModel* wallet_model,
                                               const MasternodeEntry& entry, QWidget* parent) :
    QDialog(parent),
    m_node{node},
    m_wallet_model{wallet_model},
    m_protx_hash{entry.proTxHashRaw()},
    m_operator_pubkey{entry.operatorPubKeyBytes()},
    m_runner{wallet_model ? new MasternodeOperationRunner(node.evo(), wallet_model->wallet(), this) : nullptr}
{
}

MasternodeActionDialog::~MasternodeActionDialog()
{
    m_destroying = true;
    if (m_runner) m_runner->shutdown();
}

void MasternodeActionDialog::setupUi(const QString& title, const QString& intro, QLayout* form, const QString& ok_text)
{
    setWindowTitle(title);
    setMinimumWidth(650);

    auto* layout{new QVBoxLayout(this)};
    auto* intro_label{new QLabel(intro, this)};
    intro_label->setWordWrap(true);
    layout->addWidget(intro_label);

    auto* protx_label{new QLabel(tr("Masternode: %1").arg(QString::fromStdString(m_protx_hash.ToString())), this)};
    protx_label->setWordWrap(true);
    protx_label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(protx_label);
    layout->addLayout(form);

    m_status_label = new QLabel(this);
    m_status_label->setWordWrap(true);
    m_status_label->setStyleSheet(GUIUtil::getThemedStyleQString(GUIUtil::ThemedStyle::TS_ERROR));
    m_status_label->setVisible(false);
    layout->addWidget(m_status_label);

    m_button_box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_ok_button = m_button_box->button(QDialogButtonBox::Ok);
    m_ok_button->setText(ok_text);
    if (m_wallet_model == nullptr) {
        m_ok_button->setToolTip(tr("No wallet is available."));
    } else if (m_wallet_model->wallet().privateKeysDisabled()) {
        m_ok_button->setToolTip(tr("This wallet is watch-only and cannot sign transactions."));
    }
    connect(m_button_box, &QDialogButtonBox::accepted, this, &MasternodeActionDialog::submit);
    connect(m_button_box, &QDialogButtonBox::rejected, this, &MasternodeActionDialog::reject);
    layout->addWidget(m_button_box);

    GUIUtil::updateFonts();
    validate();
}

FeeSourcePicker* MasternodeActionDialog::addFeeSourceRow(QFormLayout* form, const QString& automatic_note)
{
    auto* picker{new FeeSourcePicker(this)};
    picker->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    picker->setAutomaticOption(tr("Automatic (recommended)"));
    picker->setWalletModel(m_wallet_model);
    auto* note{new QLabel(automatic_note, this)};
    note->setWordWrap(true);
    auto* box{new QVBoxLayout()};
    box->addWidget(picker);
    box->addWidget(note);
    form->addRow(tr("Fee source:"), box);
    return picker;
}

bool MasternodeActionDialog::canSign() const
{
    return m_wallet_model != nullptr && !m_wallet_model->wallet().privateKeysDisabled();
}

void MasternodeActionDialog::setOkValid(bool valid)
{
    m_valid = valid;
    if (m_ok_button) m_ok_button->setEnabled(valid && canSign() && !m_busy);
}

void MasternodeActionDialog::showError(const QString& message)
{
    m_status_label->setText(message);
    m_status_label->setVisible(true);
}

void MasternodeActionDialog::clearError()
{
    m_status_label->clear();
    m_status_label->setVisible(false);
}

void MasternodeActionDialog::abortOperation(const QString& message)
{
    m_unlock.reset();
    setBusy(false);
    showError(message);
}

void MasternodeActionDialog::reject()
{
    if (m_busy) return;
    QDialog::reject();
}

bool MasternodeActionDialog::ensureUnlocked()
{
    if (!canSign()) return false;
    if (!m_unlock) m_unlock = std::make_unique<UnlockHolder>(*m_wallet_model);
    if (m_unlock->ctx.isValid()) return true;
    m_unlock.reset();
    showError(tr("The wallet must be unlocked to create and sign this transaction."));
    return false;
}

void MasternodeActionDialog::setBusy(bool busy)
{
    if (m_busy == busy) return;
    m_busy = busy;
    if (busy) {
        QApplication::setOverrideCursor(Qt::WaitCursor);
    } else {
        QApplication::restoreOverrideCursor();
    }
    if (m_button_box) m_button_box->setEnabled(!busy);
    setOkValid(m_valid);
}

void MasternodeActionDialog::resolveOperatorKey(OperatorSecretWidget& widget, SecretCallback callback)
{
    if (!ensureUnlocked()) return;
    clearError();
    setBusy(true);

    auto key{widget.takeSecret()};
    if (!key) {
        abortOperation(tr("Enter a valid basic-scheme BLS operator secret key."));
        return;
    }
    if (!OperatorKeyMatches(*key, m_operator_pubkey)) {
        abortOperation(tr("The operator secret key does not match this masternode's registered public key."));
        return;
    }
    callback(*key);
}

void MasternodeActionDialog::startSubmission(std::function<bool(MasternodeOperationRunner::SubmissionCallback)> start)
{
    if (!m_busy) {
        if (!ensureUnlocked()) return;
        clearError();
        setBusy(true);
    }
    if (!start([this](auto result) { finishSubmission(std::move(result)); })) {
        abortOperation(tr("Another masternode operation is already in progress."));
    }
}

void MasternodeActionDialog::finishSubmission(MasternodeOperationRunner::SubmissionResult result)
{
    m_unlock.reset();
    if (m_destroying) {
        setBusy(false);
        return;
    }
    setBusy(false);
    if (const auto* error{std::get_if<interfaces::ProviderTxError>(&result)}) {
        showError(MasternodeOperationRunner::errorText(*error));
        return;
    }
    const auto& submission{std::get<interfaces::ProviderTxSubmission>(result)};
    if (!submission.tx) {
        showError(tr("The operation completed without returning a transaction."));
        return;
    }
    if (!submission.submitted) {
        showError(tr("The transaction was created but was not sent to the network."));
        return;
    }
    QMessageBox::information(this, windowTitle(),
                             tr("The transaction was sent successfully.") + "\n\n" +
                                 tr("Transaction ID: %1").arg(QString::fromStdString(submission.tx->GetHash().ToString())));
    accept();
}

UpdateServiceDialog::UpdateServiceDialog(interfaces::Node& node, WalletModel* wallet_model,
                                         const MasternodeEntry& entry, QWidget* parent) :
    UpdateServiceDialog(node, wallet_model, entry, node.evo().getProviderTxCapabilities(), parent)
{
}

UpdateServiceDialog::UpdateServiceDialog(interfaces::Node& node, WalletModel* wallet_model, const MasternodeEntry& entry,
                                         interfaces::ProviderTxCapabilities capabilities, QWidget* parent) :
    MasternodeActionDialog(node, wallet_model, entry, parent),
    m_type{entry.type()},
    m_operator_reward_pct{entry.operatorRewardPct()},
    m_capabilities{capabilities}
{
    auto* form{new QFormLayout()};
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    m_addrs_edit = new QLineEdit(this);
    m_addrs_edit->setText(entry.coreP2PAddresses());
    m_addrs_edit->setPlaceholderText(tr("IP:PORT, comma-separated for multiple entries"));
    if (!m_capabilities.extended_addresses) {
        m_addrs_edit->setToolTip(tr("Multiple addresses require extended provider transactions."));
    }
    connect(m_addrs_edit, &QLineEdit::textChanged, this, &UpdateServiceDialog::validate);
    form->addRow(m_capabilities.extended_addresses ? tr("Service addresses:") : tr("Service address:"), m_addrs_edit);

    m_operator_key = new OperatorSecretWidget(this);
    m_operator_key->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(m_operator_key, &OperatorSecretWidget::changed, this, &UpdateServiceDialog::validate);
    form->addRow(tr("Operator key:"), m_operator_key);

    if (m_type == MnType::Evo) {
        m_platform_node_id_edit = new QLineEdit(this);
        m_platform_node_id_edit->setText(entry.platformNodeID());
        m_platform_node_id_edit->setPlaceholderText(tr("Platform node ID (40 hexadecimal characters)"));
        connect(m_platform_node_id_edit, &QLineEdit::textChanged, this, &UpdateServiceDialog::validate);
        form->addRow(tr("Platform node ID:"), m_platform_node_id_edit);

        if (m_capabilities.extended_addresses) {
            m_platform_p2p_edit = new QLineEdit(this);
            m_platform_p2p_edit->setText(entry.platformP2PAddresses());
            m_platform_p2p_edit->setPlaceholderText(tr("IP:PORT, comma-separated for multiple entries"));
            connect(m_platform_p2p_edit, &QLineEdit::textChanged, this, &UpdateServiceDialog::validate);
            form->addRow(tr("Platform P2P addresses:"), m_platform_p2p_edit);

            m_platform_https_edit = new QLineEdit(this);
            m_platform_https_edit->setText(entry.platformHTTPSAddresses());
            m_platform_https_edit->setPlaceholderText(tr("IP:PORT or domain:port, comma-separated"));
            connect(m_platform_https_edit, &QLineEdit::textChanged, this, &UpdateServiceDialog::validate);
            form->addRow(tr("Platform HTTPS addresses:"), m_platform_https_edit);
        } else {
            const QStringList p2p{entry.platformP2PAddresses().split(',', Qt::SkipEmptyParts)};
            const QStringList https{entry.platformHTTPSAddresses().split(',', Qt::SkipEmptyParts)};
            m_platform_p2p_port_edit = new QSpinBox(this);
            m_platform_p2p_port_edit->setRange(1, std::numeric_limits<uint16_t>::max());
            m_platform_p2p_port_edit->setValue(PortOrDefault(p2p, Params().GetDefaultPlatformP2PPort()));
            form->addRow(tr("Platform P2P port:"), m_platform_p2p_port_edit);

            m_platform_https_port_edit = new QSpinBox(this);
            m_platform_https_port_edit->setRange(1, std::numeric_limits<uint16_t>::max());
            m_platform_https_port_edit->setValue(PortOrDefault(https, Params().GetDefaultPlatformHTTPPort()));
            form->addRow(tr("Platform HTTPS port:"), m_platform_https_port_edit);
        }
    }

    if (m_operator_reward_pct > 0) {
        m_operator_payout_edit = new QValidatedLineEdit(this);
        GUIUtil::setupAddressWidget(m_operator_payout_edit, this);
        m_operator_payout_edit->setText(entry.operatorPayoutAddress());
        connect(m_operator_payout_edit, &QLineEdit::textChanged, this, &UpdateServiceDialog::validate);
        form->addRow(tr("Operator payout address:"), m_operator_payout_edit);
    }

    // Without an operator payout, Automatic funds from the first share's reward
    // address, which is usually another wallet's: pay from this wallet instead
    if (entry.isShared()) {
        m_fee_source = addFeeSourceRow(form, tr("The fee is paid from this address of your wallet."));
        m_fee_source->setAutomaticOption(QString{});
    } else {
        m_fee_source = addFeeSourceRow(form, tr("Automatic uses the current operator payout, then the masternode "
                                                "payout. Select an address only to override that choice."));
    }

    setupUi(m_type == MnType::Evo ? tr("Update EvoNode Service") : tr("Update Masternode Service"),
            tr("Update the registered service endpoints. A successful update also revives a PoSe-banned "
               "masternode. Every current value is prefilled so unchanged fields are preserved."),
            form, tr("Send Update"));
}

void UpdateServiceDialog::validate()
{
    QString error;
    bool ok{buildNetInfo(error).has_value()};
    ok &= m_operator_key->isValid();
    if (m_operator_payout_edit) {
        ok &= m_operator_payout_edit->text().trimmed().isEmpty() || m_operator_payout_edit->isValid();
    }
    if (m_type == MnType::Evo) {
        ok &= IsValidPlatformNodeId(m_platform_node_id_edit->text().trimmed());
    }
    setOkValid(ok);
}

std::optional<interfaces::ProviderNetInfo> UpdateServiceDialog::buildNetInfo(QString& error) const
{
    interfaces::ProviderNetInfo net_info;
    net_info.core_p2p = ToStringVector(MasternodeWidgetUtil::tokenizeEndpointList(m_addrs_edit->text()));
    if (m_type == MnType::Evo) {
        if (m_capabilities.extended_addresses) {
            net_info.platform_p2p = ToStringVector(MasternodeWidgetUtil::tokenizeEndpointList(m_platform_p2p_edit->text()));
            net_info.platform_https = ToStringVector(
                MasternodeWidgetUtil::tokenizeEndpointList(m_platform_https_edit->text()));
        } else {
            net_info.platform_p2p = static_cast<uint16_t>(m_platform_p2p_port_edit->value());
            net_info.platform_https = static_cast<uint16_t>(m_platform_https_port_edit->value());
        }
    }
    if (const auto validation_error{
            m_node.evo().validateProviderNetInfo(net_info, m_type, m_capabilities.version, /*optional=*/false)}) {
        error = MasternodeOperationRunner::errorText(*validation_error);
        return std::nullopt;
    }
    return net_info;
}

std::optional<interfaces::ProviderUpdateServiceRequest> UpdateServiceDialog::buildRequest(const CBLSSecretKey& operator_key,
                                                                                          QString& error) const
{
    interfaces::ProviderUpdateServiceRequest request;
    request.type = m_type;
    request.pro_tx_hash = m_protx_hash;
    request.operator_key = operator_key;
    request.submit = true;

    auto net_info{buildNetInfo(error)};
    if (!net_info) return std::nullopt;
    request.net_info = std::move(*net_info);

    if (m_type == MnType::Evo) {
        const QString platform_node_id_text{m_platform_node_id_edit->text().trimmed()};
        if (!IsValidPlatformNodeId(platform_node_id_text)) {
            error = tr("Enter a valid Platform node ID (40 hexadecimal characters).");
            return std::nullopt;
        }
        uint160 platform_node_id;
        platform_node_id.SetHex(platform_node_id_text.toStdString());
        request.platform_node_id = platform_node_id;
    }

    if (m_operator_payout_edit && !m_operator_payout_edit->text().trimmed().isEmpty()) {
        auto payout{ParseDestination(m_operator_payout_edit->text(), error)};
        if (!payout) return std::nullopt;
        request.operator_payout = *payout;
    }
    if (const QString fee_source{m_fee_source->selectedAddress()}; !fee_source.isEmpty()) {
        auto fee{ParseDestination(fee_source, error)};
        if (!fee) return std::nullopt;
        request.fee_source = *fee;
    }
    return request;
}

void UpdateServiceDialog::submit()
{
    resolveOperatorKey(*m_operator_key, [this](CBLSSecretKey operator_key) {
        QString error;
        auto request{buildRequest(operator_key, error)};
        if (!request) {
            abortOperation(error);
            return;
        }
        startSubmission([this, request = std::move(*request)](auto callback) mutable {
            return m_runner->updateMasternodeService(std::move(request), std::move(callback));
        });
    });
}

UpdateRegistrarDialog::UpdateRegistrarDialog(interfaces::Node& node, WalletModel* wallet_model,
                                             const MasternodeEntry& entry, QWidget* parent) :
    UpdateRegistrarDialog(node, wallet_model, entry, node.evo().getProviderTxCapabilities(), parent)
{
}

UpdateRegistrarDialog::UpdateRegistrarDialog(interfaces::Node& node, WalletModel* wallet_model,
                                             const MasternodeEntry& entry,
                                             interfaces::ProviderTxCapabilities capabilities, QWidget* parent) :
    MasternodeActionDialog(node, wallet_model, entry, parent),
    m_capabilities{capabilities},
    m_initial_operator_pubkey{entry.operatorPubKey(m_capabilities.version == ProTxVersion::LegacyBLS)},
    m_initial_voting{entry.votingAddress()},
    m_initial_payout{entry.payoutAddresses().size() == 1 ? entry.payoutAddresses().front() : QString{}}
{
    auto* form{new QFormLayout()};
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    m_operator_pubkey_edit = new QLineEdit(this);
    m_operator_pubkey_edit->setText(m_initial_operator_pubkey);
    m_operator_pubkey_edit->setPlaceholderText(tr("Operator BLS public key (96 hexadecimal characters)"));
    connect(m_operator_pubkey_edit, &QLineEdit::textChanged, this, &UpdateRegistrarDialog::validate);
    form->addRow(tr("Operator public key:"), m_operator_pubkey_edit);

    auto* warning{new QLabel(tr("Changing the operator key immediately PoSe-bans the masternode. The new operator must "
                                "send an Update "
                                "Service transaction to revive it."),
                             this)};
    warning->setWordWrap(true);
    warning->setStyleSheet(GUIUtil::getThemedStyleQString(GUIUtil::ThemedStyle::TS_WARNING));
    form->addRow(warning);

    m_voting_edit = new QValidatedLineEdit(this);
    GUIUtil::setupAddressWidget(m_voting_edit, this);
    m_voting_edit->setText(m_initial_voting);
    connect(m_voting_edit, &QLineEdit::textChanged, this, &UpdateRegistrarDialog::validate);
    form->addRow(tr("Voting address:"), m_voting_edit);

    m_payout_edit = new QValidatedLineEdit(this);
    GUIUtil::setupAddressWidget(m_payout_edit, this);
    m_payout_edit->setText(m_initial_payout);
    connect(m_payout_edit, &QLineEdit::textChanged, this, &UpdateRegistrarDialog::validate);
    form->addRow(tr("Payout address:"), m_payout_edit);
    if (entry.payoutAddresses().size() != 1) {
        m_payout_edit->setEnabled(false);
        m_payout_edit->setPlaceholderText(tr("Multiple payout shares are preserved; use the RPC to change them"));
        m_payout_edit->setToolTip(tr("This dialog cannot safely flatten a multi-payout masternode into one address."));
    }

    m_fee_source = addFeeSourceRow(form, tr("Automatic uses the masternode's current primary payout address. Select an "
                                            "address only to "
                                            "override that choice."));

    setupUi(tr("Update Masternode Registrar"),
            tr("Update the operator key, voting address, or payout address. All current values are prefilled; "
               "only changed fields are sent. The owner key must be present in this wallet."),
            form, tr("Send Update"));
}

bool UpdateRegistrarDialog::operatorChanged() const
{
    return m_operator_pubkey_edit->text().trimmed() != m_initial_operator_pubkey;
}

bool UpdateRegistrarDialog::votingChanged() const { return m_voting_edit->text().trimmed() != m_initial_voting; }

bool UpdateRegistrarDialog::payoutChanged() const
{
    return m_payout_edit->isEnabled() && m_payout_edit->text().trimmed() != m_initial_payout;
}

void UpdateRegistrarDialog::validate()
{
    const QString operator_pubkey{m_operator_pubkey_edit->text().trimmed()};
    const bool any_change{operatorChanged() || votingChanged() || payoutChanged()};
    CBLSPublicKey parsed_operator;
    const bool operator_valid{
        parsed_operator.SetHexStr(operator_pubkey.toStdString(), m_capabilities.version == ProTxVersion::LegacyBLS)};
    const bool ok{any_change && operator_valid && IsNonNullP2PKHAddress(m_voting_edit->text()) &&
                  (!m_payout_edit->isEnabled() || MasternodeWidgetUtil::isP2PKHorP2SHAddress(m_payout_edit->text()))};
    setOkValid(ok);
}

std::optional<interfaces::ProviderUpdateRegistrarRequest> UpdateRegistrarDialog::buildRequest(QString& error) const
{
    interfaces::ProviderUpdateRegistrarRequest request;
    request.pro_tx_hash = m_protx_hash;
    request.use_legacy_bls_scheme = m_capabilities.version == ProTxVersion::LegacyBLS;
    request.submit = true;

    if (operatorChanged()) {
        CBLSPublicKey key;
        const bool legacy{m_capabilities.version == ProTxVersion::LegacyBLS};
        if (!key.SetHexStr(m_operator_pubkey_edit->text().trimmed().toStdString(), legacy)) {
            error = tr("Enter a valid operator public key for the current provider-transaction version.");
            return std::nullopt;
        }
        request.operator_key = key;
    }
    if (votingChanged()) {
        const auto destination{ParseDestination(m_voting_edit->text(), error)};
        const auto* key_id{destination ? std::get_if<PKHash>(&*destination) : nullptr};
        if (!key_id || ToKeyID(*key_id).IsNull()) {
            if (error.isEmpty()) error = tr("The voting address must be a key address.");
            return std::nullopt;
        }
        request.voting_key = ToKeyID(*key_id);
    }
    if (payoutChanged()) {
        auto payout{ParseDestination(m_payout_edit->text(), error)};
        if (!payout) return std::nullopt;
        request.payouts = std::vector<interfaces::ProviderPayout>{{*payout, interfaces::ProviderPayout::MAX_REWARD}};
        request.uses_extended_payouts = m_capabilities.version >= ProTxVersion::ExtAddr;
    }
    if (const QString fee_source{m_fee_source->selectedAddress()}; !fee_source.isEmpty()) {
        auto fee{ParseDestination(fee_source, error)};
        if (!fee) return std::nullopt;
        request.fee_source = *fee;
    }
    return request;
}

void UpdateRegistrarDialog::submit()
{
    QString error;
    auto request{buildRequest(error)};
    if (!request) {
        showError(error);
        return;
    }
    startSubmission([this, request = std::move(*request)](auto callback) mutable {
        return m_runner->updateMasternodeRegistrar(std::move(request), std::move(callback));
    });
}

RevokeDialog::RevokeDialog(interfaces::Node& node, WalletModel* wallet_model, const MasternodeEntry& entry,
                           QWidget* parent) :
    MasternodeActionDialog(node, wallet_model, entry, parent)
{
    auto* form{new QFormLayout()};
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    m_reason_combo = new QComboBox(this);
    m_reason_combo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_reason_combo->addItem(tr("Not specified"), CProUpRevTx::REASON_NOT_SPECIFIED);
    m_reason_combo->addItem(tr("Termination of service"), CProUpRevTx::REASON_TERMINATION_OF_SERVICE);
    m_reason_combo->addItem(tr("Compromised keys"), CProUpRevTx::REASON_COMPROMISED_KEYS);
    m_reason_combo->addItem(tr("Change of keys"), CProUpRevTx::REASON_CHANGE_OF_KEYS);
    form->addRow(tr("Reason:"), m_reason_combo);

    m_operator_key = new OperatorSecretWidget(this);
    m_operator_key->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(m_operator_key, &OperatorSecretWidget::changed, this, &RevokeDialog::validate);
    form->addRow(tr("Operator key:"), m_operator_key);

    // Without an operator payout, Automatic funds from the first share's reward
    // address, which is usually another wallet's: pay from this wallet instead
    if (entry.isShared()) {
        m_fee_source = addFeeSourceRow(form, tr("The fee is paid from this address of your wallet."));
        m_fee_source->setAutomaticOption(QString{});
    } else {
        m_fee_source = addFeeSourceRow(form, tr("Automatic uses the current operator payout, then the masternode "
                                                "payout. Select an address only to override that choice."));
    }

    setupUi(tr("Revoke Masternode Service"),
            tr("Revoking clears the service address and PoSe-bans the masternode until its owner changes the "
               "operator key. The collateral is not affected."),
            form, tr("Revoke"));
}

void RevokeDialog::validate() { setOkValid(m_operator_key->isValid()); }

std::optional<interfaces::ProviderRevokeRequest> RevokeDialog::buildRequest(CBLSSecretKey operator_key, QString& error) const
{
    interfaces::ProviderRevokeRequest request;
    request.pro_tx_hash = m_protx_hash;
    request.operator_key = operator_key;
    request.reason = static_cast<uint16_t>(m_reason_combo->currentData().toUInt());
    request.submit = true;
    if (const QString fee_source{m_fee_source->selectedAddress()}; !fee_source.isEmpty()) {
        auto fee{ParseDestination(fee_source, error)};
        if (!fee) return std::nullopt;
        request.fee_source = *fee;
    }
    return request;
}

void RevokeDialog::submit()
{
    resolveOperatorKey(*m_operator_key, [this](CBLSSecretKey operator_key) {
        QString error;
        auto request{buildRequest(operator_key, error)};
        if (!request) {
            abortOperation(error);
            return;
        }
        startSubmission([this, request = std::move(*request)](auto callback) mutable {
            return m_runner->revokeMasternode(std::move(request), std::move(callback));
        });
    });
}
