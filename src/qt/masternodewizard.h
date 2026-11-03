// Copyright (c) 2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_MASTERNODEWIZARD_H
#define BITCOIN_QT_MASTERNODEWIZARD_H

#include <consensus/amount.h>
#include <interfaces/providertx.h>
#include <qt/masternodeoperationrunner.h>

#include <QDialog>
#include <QString>
#include <QVector>

#include <memory>
#include <optional>
#include <set>

class FeeSourcePicker;
class MasternodeWidgetTests;
class OperatorKeyWidget;
class QValidatedLineEdit;
class WalletModel;

namespace interfaces {
class Node;
} // namespace interfaces

QT_BEGIN_NAMESPACE
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QProgressBar;
class QPushButton;
class QRadioButton;
class QSpinBox;
class QStackedWidget;
class QVBoxLayout;
QT_END_NAMESPACE

//! Multi-page dialog registering a masternode or EvoNode: collects collateral,
//! service, key, payout and fee-source details, then invokes the typed provider
//! transaction interface on a worker thread. Supports funding the collateral
//! from the wallet, referencing an exact-denomination UTXO already in the
//! wallet, and external collateral signed out of band.
class RegisterMasternodeWizard : public QDialog
{
    Q_OBJECT

public:
    explicit RegisterMasternodeWizard(interfaces::Node& node, WalletModel* walletModel, QWidget* parent = nullptr);
    ~RegisterMasternodeWizard() override;

public Q_SLOTS:
    void reject() override;

private Q_SLOTS:
    void onNext();
    void onBack();

private:
    friend class MasternodeWidgetTests;

    static constexpr int VOUT_ROLE{Qt::UserRole + 1};
    static constexpr int COLLATERAL_ADDRESS_ROLE{Qt::UserRole + 2};

    enum Page : int {
        PageType = 0,
        PageCollateral,
        PageService,
        PageKeys,
        PagePayout,
        PagePlatform,
        PageFee,
        PageReview,
        PageSecret,
        PageSign,
        PageResult,
    };
    //! Which typed operation is in flight
    enum class Stage {
        None,
        Register,
        Prepare,
        Submit,
    };

    QWidget* createTypePage();
    QWidget* createCollateralPage();
    QWidget* createServicePage();
    QWidget* createKeysPage();
    QWidget* createPayoutPage();
    QWidget* createPlatformPage();
    QWidget* createFeePage();
    QWidget* createReviewPage();
    QWidget* createSecretPage();
    QWidget* createSignPage();
    QWidget* createResultPage();

    bool isEvo() const;
    bool usesExtendedAddresses() const;
    bool isExternalCollateral() const;
    bool isFundCollateral() const;
    std::optional<CTxDestination> knownCollateralDestination() const;
    QString collateralAddress() const;
    QString ownerAddress() const;
    QString votingAddress() const;
    CAmount collateralAmount() const;
    //! True when the user typed the generated operator secret's last 4 characters
    bool secretConfirmed() const;
    //! True while the generated operator secret exists nowhere but this dialog.
    bool secretGateRequired() const;

    void rebuildOrder();
    Page currentPage() const;
    QString pageTitle(Page page) const;
    void updateProgress();
    void goToPage(Page page);
    void enterPage(Page page);
    bool validatePage(Page page, QString& err);
    interfaces::ProviderNetInfo providerNetInfo() const;
    bool validateProviderNetInfo(bool optional, QString& err) const;
    void updateButtons();
    void onPageEdited();
    void setBusy(bool busy, const QString& busy_text = QString());
    void showError(const QString& message);
    bool confirmBroadcast();

    void refreshCollateralCandidates();
    void refreshCollateralCandidates(const std::set<COutPoint>& registered_collaterals);
    void completeRegistration(const CTransactionRef& transaction);
    void populateReview();
    //! Width of the review's label column, shared by every section
    int reviewLabelWidth(const QWidget* card) const;
    //! One line saying where the operator key comes from and whether it can be
    //! seen again
    QString operatorKeyProvenance() const;
    void populateSecret();
    void populateResult(const QString& pro_tx_hash);
    std::optional<interfaces::ProviderRegistrationRequest> buildRegistrationRequest(QString& error) const;
    void startRegistration(bool skip_confirmation = false);
    void startSubmit(bool skip_confirmation = false);
    void finishSubmission(MasternodeOperationRunner::SubmissionResult result);
    void finishPrepare(interfaces::ProviderTxResult<interfaces::PreparedProviderRegistration> result);

    interfaces::Node& m_node;
    WalletModel* const m_walletModel;
    std::unique_ptr<MasternodeOperationRunner> m_runner;
    Stage m_stage{Stage::None};
    struct UnlockHolder;
    std::unique_ptr<UnlockHolder> m_unlock;

    QVector<Page> m_order;
    int m_pos{0};
    bool m_busy{false};
    bool m_destroying{false};
    bool m_registered{false};
    bool m_platform_extended_addresses{false};
    uint16_t m_platform_provider_version{0};
    bool m_platform_ipv4_optional{false};
    std::optional<Page> m_validation_page;

    QStackedWidget* m_pages;
    QLabel* m_progress_label{nullptr};
    QLabel* m_error_label;
    QProgressBar* m_busy_bar;
    QPushButton* m_back_button;
    QPushButton* m_next_button;
    QPushButton* m_cancel_button;

    // Type page
    QRadioButton* m_type_regular;
    QRadioButton* m_type_evo;
    // Collateral page
    QRadioButton* m_col_fund;
    QRadioButton* m_col_wallet;
    QRadioButton* m_col_external;
    QWidget* m_col_fund_box;
    QWidget* m_col_wallet_box;
    QWidget* m_col_external_box;
    QValidatedLineEdit* m_col_address;
    QComboBox* m_col_utxo_combo;
    QLabel* m_col_utxo_none;
    QLineEdit* m_col_txid;
    QSpinBox* m_col_vout;
    // Service page
    QLineEdit* m_service_edit;
    // Keys page
    QValidatedLineEdit* m_owner_edit;
    QValidatedLineEdit* m_voting_edit;
    OperatorKeyWidget* m_operator_widget{nullptr};
    // Payout page
    QValidatedLineEdit* m_payout_edit;
    QDoubleSpinBox* m_operator_reward;
    QLabel* m_reward_warning;
    // Platform page. Pre-v24 ProTxs only store bare platform ports; the
    // ADDR:PORT list form requires a version 3 ProTx (v24 active).
    QLineEdit* m_platform_nodeid;
    QWidget* m_platform_addr_box;
    QLineEdit* m_platform_p2p;
    QLineEdit* m_platform_https;
    QWidget* m_platform_port_box;
    QSpinBox* m_platform_p2p_port;
    QSpinBox* m_platform_https_port;
    // Fee page
    FeeSourcePicker* m_fee_picker;
    QLabel* m_fee_explain;
    // Review page. The sections are rebuilt on every entry, so the layout owns
    // the rows rather than a single rich-text label.
    QWidget* m_review_container;
    QVBoxLayout* m_review_layout;
    // Generated-secret page. Confirmation is required before any operation
    // can broadcast a registration using the generated key.
    QLabel* m_secret_note;
    QLineEdit* m_secret_edit;
    QLineEdit* m_conf_line_edit;
    QLineEdit* m_confirm_edit;
    // Sign page (external collateral)
    QLabel* m_sign_address_label;
    QPlainTextEdit* m_sign_message;
    QPlainTextEdit* m_sig_edit;
    CTransactionRef m_prepared_tx;
    COutPoint m_prepared_collateral_outpoint;
    bool m_prepared_collateral_lock_acquired{false};
    // Result page
    QLabel* m_result_label;
    QLabel* m_result_hash;
    QLabel* m_result_tx_note;
    QLabel* m_next_steps;
};

#endif // BITCOIN_QT_MASTERNODEWIZARD_H
