// Copyright (c) 2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_MASTERNODEDIALOGS_H
#define BITCOIN_QT_MASTERNODEDIALOGS_H

#include <evo/dmn_types.h>
#include <interfaces/providertx.h>
#include <qt/masternodeoperationrunner.h>

#include <QDialog>
#include <QString>
#include <QWidget>

#include <functional>
#include <memory>
#include <optional>
#include <vector>

class FeeSourcePicker;
class MasternodeEntry;
class MasternodeMaintenanceTests;
class QValidatedLineEdit;
class WalletModel;

namespace interfaces {
class Node;
} // namespace interfaces

QT_BEGIN_NAMESPACE
class QComboBox;
class QDialogButtonBox;
class QFormLayout;
class QLabel;
class QLayout;
class QLineEdit;
class QPushButton;
class QSpinBox;
QT_END_NAMESPACE

namespace MasternodeMaintenance {

struct ActionAvailability {
    bool update_service{false};
    bool update_registrar{false};
    bool revoke{false};
};

//! Centralized context-menu role policy. Operator transactions require manual
//! secret entry; registrar updates require the owner key.
ActionAvailability actionAvailability(bool can_sign, bool owns_owner_key);

} // namespace MasternodeMaintenance

//! Manual entry for the registered operator secret. The secret is validated as
//! a basic-scheme BLS key and matched against the selected masternode before use.
class OperatorSecretWidget : public QWidget
{
    Q_OBJECT

public:
    explicit OperatorSecretWidget(QWidget* parent = nullptr);

    bool isValid() const;
    //! Parse the entered key and immediately clear the password field and the
    //! temporary encoded buffer. Returns nullopt for malformed input.
    std::optional<CBLSSecretKey> takeSecret();

Q_SIGNALS:
    void changed();

private:
    QLineEdit* m_manual_edit;
};

//! Shared asynchronous scaffolding for typed provider-maintenance operations.
class MasternodeActionDialog : public QDialog
{
    Q_OBJECT

    friend class MasternodeMaintenanceTests;

public Q_SLOTS:
    void reject() override;

protected:
    using SecretCallback = std::function<void(CBLSSecretKey)>;

    MasternodeActionDialog(interfaces::Node& node, WalletModel* wallet_model, const MasternodeEntry& entry,
                           QWidget* parent);
    ~MasternodeActionDialog() override;

    void setupUi(const QString& title, const QString& intro, QLayout* form, const QString& ok_text);
    FeeSourcePicker* addFeeSourceRow(QFormLayout* form, const QString& automatic_note);

    bool canSign() const;
    void setOkValid(bool valid, const QString& tooltip = {});
    void showError(const QString& message);
    void clearError();
    void abortOperation(const QString& message);
    void resolveOperatorKey(OperatorSecretWidget& widget, SecretCallback callback);
    void startSubmission(std::function<bool(MasternodeOperationRunner::SubmissionCallback)> start);

    interfaces::Node& m_node;
    WalletModel* const m_wallet_model;
    const uint256 m_protx_hash;
    const std::vector<unsigned char> m_operator_pubkey;
    const bool m_is_shared;
    MasternodeOperationRunner* const m_runner;

protected Q_SLOTS:
    virtual void validate() = 0;
    virtual void submit() = 0;

private:
    struct UnlockHolder;

    bool ensureUnlocked();
    void finishSubmission(MasternodeOperationRunner::SubmissionResult result);
    void setBusy(bool busy);

    std::unique_ptr<UnlockHolder> m_unlock;
    QDialogButtonBox* m_button_box{nullptr};
    QPushButton* m_ok_button{nullptr};
    QLabel* m_status_label{nullptr};
    bool m_busy{false};
    bool m_valid{false};
    bool m_destroying{false};
};

class UpdateServiceDialog : public MasternodeActionDialog
{
    Q_OBJECT

    friend class MasternodeMaintenanceTests;

public:
    UpdateServiceDialog(interfaces::Node& node, WalletModel* wallet_model, const MasternodeEntry& entry,
                        QWidget* parent = nullptr);

protected:
    void validate() override;
    void submit() override;

private:
    UpdateServiceDialog(interfaces::Node& node, WalletModel* wallet_model, const MasternodeEntry& entry,
                        interfaces::ProviderTxCapabilities capabilities, QWidget* parent);
    std::optional<interfaces::ProviderNetInfo> buildNetInfo(QString& error) const;
    std::optional<interfaces::ProviderUpdateServiceRequest> buildRequest(const CBLSSecretKey& operator_key,
                                                                         QString& error) const;

    const MnType m_type;
    const uint16_t m_operator_reward_pct;
    const interfaces::ProviderTxCapabilities m_capabilities;

    QLineEdit* m_addrs_edit{nullptr};
    OperatorSecretWidget* m_operator_key{nullptr};
    QLineEdit* m_platform_node_id_edit{nullptr};
    QLineEdit* m_platform_p2p_edit{nullptr};
    QLineEdit* m_platform_https_edit{nullptr};
    QSpinBox* m_platform_p2p_port_edit{nullptr};
    QSpinBox* m_platform_https_port_edit{nullptr};
    QValidatedLineEdit* m_operator_payout_edit{nullptr};
    FeeSourcePicker* m_fee_source{nullptr};
};

class UpdateRegistrarDialog : public MasternodeActionDialog
{
    Q_OBJECT

    friend class MasternodeMaintenanceTests;

public:
    UpdateRegistrarDialog(interfaces::Node& node, WalletModel* wallet_model, const MasternodeEntry& entry,
                          QWidget* parent = nullptr);

protected:
    void validate() override;
    void submit() override;

private:
    UpdateRegistrarDialog(interfaces::Node& node, WalletModel* wallet_model, const MasternodeEntry& entry,
                          interfaces::ProviderTxCapabilities capabilities, QWidget* parent);
    std::optional<interfaces::ProviderUpdateRegistrarRequest> buildRequest(QString& error) const;
    bool operatorChanged() const;
    bool votingChanged() const;
    bool payoutChanged() const;

    const interfaces::ProviderTxCapabilities m_capabilities;
    const QString m_initial_operator_pubkey;
    const QString m_initial_voting;
    const QString m_initial_payout;

    QLineEdit* m_operator_pubkey_edit{nullptr};
    QValidatedLineEdit* m_voting_edit{nullptr};
    QValidatedLineEdit* m_payout_edit{nullptr};
    FeeSourcePicker* m_fee_source{nullptr};
};

class RevokeDialog : public MasternodeActionDialog
{
    Q_OBJECT

    friend class MasternodeMaintenanceTests;

public:
    RevokeDialog(interfaces::Node& node, WalletModel* wallet_model, const MasternodeEntry& entry,
                 QWidget* parent = nullptr);

protected:
    void validate() override;
    void submit() override;

private:
    std::optional<interfaces::ProviderRevokeRequest> buildRequest(CBLSSecretKey operator_key, QString& error) const;

    QComboBox* m_reason_combo{nullptr};
    OperatorSecretWidget* m_operator_key{nullptr};
    FeeSourcePicker* m_fee_source{nullptr};
};

#endif // BITCOIN_QT_MASTERNODEDIALOGS_H
