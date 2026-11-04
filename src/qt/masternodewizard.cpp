// Copyright (c) 2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/masternodewizard.h>

#include <chainparams.h>
#include <evo/dmn_types.h>
#include <interfaces/node.h>
#include <interfaces/wallet.h>
#include <key_io.h>
#include <primitives/transaction.h>
#include <script/standard.h>
#include <util/strencodings.h>

#include <qt/bitcoinunits.h>
#include <qt/guiutil.h>
#include <qt/masternodeoperationrunner.h>
#include <qt/masternodewidgets.h>
#include <qt/optionsmodel.h>
#include <qt/qvalidatedlineedit.h>
#include <qt/sendcoinsdialog.h>
#include <qt/walletmodel.h>

#include <QButtonGroup>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayoutItem>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QtMath>

#include <limits>
#include <set>
#include <string>
#include <variant>
#include <vector>

namespace {
using MasternodeWidgetUtil::CARD_PADDING;
using MasternodeWidgetUtil::GROUP_SPACING;
using MasternodeWidgetUtil::ROW_SPACING;
using MasternodeWidgetUtil::TITLE_SPACING;
using MasternodeWidgetUtil::makeBlock;
using MasternodeWidgetUtil::makeBlockTitle;
using MasternodeWidgetUtil::makeCard;
using MasternodeWidgetUtil::makeHint;
using MasternodeWidgetUtil::makeOptionCard;
using MasternodeWidgetUtil::makePageLayout;
using MasternodeWidgetUtil::makePageTitle;
using MasternodeWidgetUtil::makeValue;

std::set<COutPoint> RegisteredCollaterals(interfaces::Wallet& wallet)
{
    const auto registered{wallet.listProTxCoins()};
    return {registered.begin(), registered.end()};
}

//! Formats collateral amounts, which are whole coins in every display unit, so
//! a zero fraction is dropped entirely ("1 000.00000000 DASH" -> "1 000 DASH").
QString FormatCollateralAmount(const WalletModel* wallet_model, CAmount amount)
{
    const auto unit{wallet_model && wallet_model->getOptionsModel() ?
                        wallet_model->getOptionsModel()->getDisplayUnit() :
                        BitcoinUnits::Unit::DASH};
    QString str{BitcoinUnits::format(unit, amount, /*plussign=*/false, BitcoinUnits::SeparatorStyle::ALWAYS)};
    const int decimals{BitcoinUnits::decimals(unit)};
    if (decimals > 0 && amount % BitcoinUnits::factor(unit) == 0) {
        str.chop(decimals + 1);
    }
    return str + QLatin1Char(' ') + BitcoinUnits::name(unit);
}
} // anonymous namespace

//! Keeps the wallet unlocked on the GUI thread while a typed provider operation is in
//! flight. UnlockContext is neither copyable nor movable, so it is constructed
//! in place from requestUnlock()'s prvalue.
struct RegisterMasternodeWizard::UnlockHolder
{
    WalletModel::UnlockContext ctx;
    explicit UnlockHolder(WalletModel& wallet_model) :
        ctx(wallet_model.requestUnlock()) {}
};

RegisterMasternodeWizard::RegisterMasternodeWizard(interfaces::Node& node, WalletModel* walletModel, QWidget* parent) :
    QDialog(parent),
    m_node(node),
    m_walletModel(walletModel)
{
    setWindowTitle(tr("Register Masternode"));

    m_pages = new QStackedWidget(this);
    // Insertion order must match the Page enum: the enum value doubles as the
    // stack index.
    m_pages->insertWidget(PageType, createTypePage());
    m_pages->insertWidget(PageCollateral, createCollateralPage());
    m_pages->insertWidget(PageService, createServicePage());
    m_pages->insertWidget(PageKeys, createKeysPage());
    m_pages->insertWidget(PagePayout, createPayoutPage());
    m_pages->insertWidget(PagePlatform, createPlatformPage());
    m_pages->insertWidget(PageFee, createFeePage());
    m_pages->insertWidget(PageReview, createReviewPage());
    m_pages->insertWidget(PageSecret, createSecretPage());
    m_pages->insertWidget(PageSign, createSignPage());
    m_pages->insertWidget(PageResult, createResultPage());

    m_progress_label = makeHint(QString(), this);
    m_error_label = new QLabel(this);
    m_error_label->setWordWrap(true);
    m_error_label->setStyleSheet(GUIUtil::getThemedStyleQString(GUIUtil::ThemedStyle::TS_ERROR));
    m_error_label->setMinimumHeight(m_error_label->fontMetrics().lineSpacing() * 2);

    m_busy_bar = new QProgressBar(this);
    m_busy_bar->setRange(0, 0);
    m_busy_bar->setTextVisible(false);
    m_busy_bar->setMaximumHeight(4);
    m_busy_bar->setVisible(false);

    m_back_button = new QPushButton(tr("Back"), this);
    m_next_button = new QPushButton(tr("Next"), this);
    m_next_button->setDefault(true);
    m_cancel_button = new QPushButton(tr("Cancel"), this);

    auto* buttons{new QHBoxLayout()};
    buttons->addWidget(m_back_button);
    buttons->addStretch();
    buttons->addWidget(m_cancel_button);
    buttons->addWidget(m_next_button);

    // The dialog owns the page margins; the pages themselves use none so the
    // two do not add up.
    auto* layout{new QVBoxLayout(this)};
    layout->setContentsMargins(24, 12, 24, 12);
    layout->setSpacing(GROUP_SPACING);
    layout->addWidget(m_progress_label);
    layout->addWidget(m_pages, /*stretch=*/1);
    layout->addWidget(m_busy_bar);
    layout->addWidget(m_error_label);
    layout->addLayout(buttons);

    connect(m_back_button, &QPushButton::clicked, this, &RegisterMasternodeWizard::onBack);
    connect(m_next_button, &QPushButton::clicked, this, &RegisterMasternodeWizard::onNext);
    connect(m_cancel_button, &QPushButton::clicked, this, &RegisterMasternodeWizard::reject);

    const auto edited = [this] { onPageEdited(); };
    for (QLineEdit* const edit :
         {static_cast<QLineEdit*>(m_col_address), m_col_txid, m_service_edit, static_cast<QLineEdit*>(m_owner_edit),
          static_cast<QLineEdit*>(m_voting_edit), static_cast<QLineEdit*>(m_payout_edit), m_platform_nodeid,
          m_platform_p2p, m_platform_https, m_confirm_edit}) {
        connect(edit, &QLineEdit::textChanged, this, edited);
    }
    connect(m_sig_edit, &QPlainTextEdit::textChanged, this, edited);
    connect(m_col_utxo_combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, edited);
    connect(m_col_vout, QOverload<int>::of(&QSpinBox::valueChanged), this, edited);
    connect(m_platform_p2p_port, QOverload<int>::of(&QSpinBox::valueChanged), this, edited);
    connect(m_platform_https_port, QOverload<int>::of(&QSpinBox::valueChanged), this, edited);
    connect(m_fee_picker, QOverload<int>::of(&QComboBox::currentIndexChanged), this, edited);
    connect(m_operator_widget, &OperatorKeyWidget::changed, this, [this] {
        rebuildOrder();
        onPageEdited();
    });
    if (m_walletModel != nullptr) {
        m_runner = std::make_unique<MasternodeOperationRunner>(m_node.evo(), m_walletModel->wallet(), this);
    }

    rebuildOrder();
    enterPage(PageType);

    GUIUtil::disableMacFocusRect(this);
    GUIUtil::updateFonts();
    setMinimumSize(700, 560);
    resize(760, 680);
}

RegisterMasternodeWizard::~RegisterMasternodeWizard()
{
    // Finish and synchronously deliver any backend result before releasing
    // session-owned state. A prepare may have acquired a coin lock.
    m_destroying = true;
    if (m_runner) m_runner->shutdown();
    m_runner.reset();
    if (m_prepared_collateral_lock_acquired && m_walletModel != nullptr) {
        m_walletModel->wallet().unlockCoin(m_prepared_collateral_outpoint);
    }
    for (QLineEdit* const edit : {m_secret_edit, m_conf_line_edit, m_confirm_edit}) {
        edit->setText(QString(edit->text().size(), QLatin1Char('0')));
        edit->clear();
    }
}

QWidget* RegisterMasternodeWizard::createTypePage()
{
    auto* page{new QWidget(this)};
    auto* layout{makePageLayout(page)};
    layout->addWidget(makePageTitle(tr("Masternode type"), page));

    m_type_regular = new QRadioButton(
        tr("Masternode — %1 collateral")
            .arg(FormatCollateralAmount(m_walletModel, GetMnType(MnType::Regular).collat_amount)),
        page);
    m_type_regular->setChecked(true);
    auto regular_card{makeOptionCard(page, m_type_regular,
                                     tr("Provides Core network services and earns regular masternode rewards."))};
    regular_card.body->setVisible(false);
    layout->addWidget(regular_card.card);

    m_type_evo = new QRadioButton(
        tr("EvoNode — %1 collateral").arg(FormatCollateralAmount(m_walletModel, GetMnType(MnType::Evo).collat_amount)),
        page);
    auto evo_card{makeOptionCard(page, m_type_evo,
                                 tr("Additionally hosts Dash Platform, has four times the voting weight and earns a "
                                    "larger share of rewards. Requires a Platform node ID and extra services."))};
    evo_card.body->setVisible(false);
    layout->addWidget(evo_card.card);
    layout->addStretch();

    auto* group{new QButtonGroup(page)};
    group->addButton(m_type_regular);
    group->addButton(m_type_evo);

    connect(m_type_regular, &QRadioButton::toggled, this, [this] {
        setWindowTitle(isEvo() ? tr("Register EvoNode") : tr("Register Masternode"));
        rebuildOrder();
        refreshCollateralCandidates();
        updateProgress();
    });
    return page;
}

QWidget* RegisterMasternodeWizard::createCollateralPage()
{
    auto* page{new QWidget(this)};
    auto* layout{makePageLayout(page)};
    layout->addWidget(makePageTitle(tr("Collateral"), page));

    m_col_fund = new QRadioButton(tr("Send collateral from this wallet to a new address"), page);
    m_col_fund->setChecked(true);
    auto fund_card{makeOptionCard(page, m_col_fund,
                                  tr("A single transaction funds the collateral and registers the masternode."))};
    m_col_fund_box = fund_card.body;
    {
        auto* row{new QHBoxLayout()};
        m_col_address = new QValidatedLineEdit(m_col_fund_box);
        GUIUtil::setupAddressWidget(m_col_address, this);
        row->addWidget(m_col_address, /*stretch=*/1);
        auto* fresh{new QPushButton(tr("Use new address"), m_col_fund_box)};
        connect(fresh, &QPushButton::clicked, this, [this] {
            QString err;
            const QString addr{MasternodeWidgetUtil::freshAddress(m_walletModel, err)};
            if (addr.isEmpty()) {
                showError(err);
            } else {
                m_col_address->setText(addr);
            }
        });
        row->addWidget(fresh);
        fund_card.body_layout->addLayout(row);
    }
    layout->addWidget(fund_card.card);

    m_col_wallet = new QRadioButton(tr("Use an existing collateral output of this wallet"), page);
    auto wallet_card{makeOptionCard(page, m_col_wallet,
                                    tr("An unspent P2PKH output of exactly the collateral amount, confirmed and not "
                                       "used by another masternode."))};
    m_col_wallet_box = wallet_card.body;
    {
        m_col_utxo_combo = new QComboBox(m_col_wallet_box);
        m_col_utxo_combo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
        m_col_utxo_combo->setMinimumContentsLength(40);
        wallet_card.body_layout->addWidget(m_col_utxo_combo);
        // Empty state of the card: the exact requirement, filled in with the
        // type's collateral amount by refreshCollateralCandidates().
        m_col_utxo_none = makeHint(QString(), m_col_wallet_box);
        m_col_utxo_none->setVisible(false);
        wallet_card.body_layout->addWidget(m_col_utxo_none);
    }
    layout->addWidget(wallet_card.card);

    m_col_external = new QRadioButton(tr("Reference an external collateral (e.g. hardware wallet)"), page);
    auto external_card{makeOptionCard(page, m_col_external,
                                      tr("A confirmed P2PKH output of exactly the collateral amount, held outside "
                                         "this wallet."))};
    m_col_external_box = external_card.body;
    {
        external_card.body_layout->addWidget(
            makeHint(tr("After review you will be asked to sign a message with the collateral key outside this "
                        "wallet."),
                     m_col_external_box));
        auto* outpoint_form{new QFormLayout()};
        outpoint_form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
        m_col_txid = new QLineEdit(m_col_external_box);
        m_col_txid->setPlaceholderText(tr("Collateral transaction id (64 hexadecimal characters)"));
        m_col_txid->setMaxLength(64);
        outpoint_form->addRow(tr("Transaction ID:"), m_col_txid);
        m_col_vout = new QSpinBox(m_col_external_box);
        m_col_vout->setRange(0, 99999);
        m_col_vout->setToolTip(tr("Output index"));
        m_col_vout->setMaximumWidth(140);
        outpoint_form->addRow(tr("Output index:"), m_col_vout);
        external_card.body_layout->addLayout(outpoint_form);
    }
    layout->addWidget(external_card.card);
    layout->addStretch();

    auto* group{new QButtonGroup(page)};
    group->addButton(m_col_fund);
    group->addButton(m_col_wallet);
    group->addButton(m_col_external);

    const auto update_boxes = [this] {
        m_col_fund_box->setVisible(m_col_fund->isChecked());
        m_col_wallet_box->setVisible(m_col_wallet->isChecked());
        m_col_external_box->setVisible(m_col_external->isChecked());
        if (m_col_wallet->isChecked()) refreshCollateralCandidates();
        rebuildOrder();
    };
    connect(m_col_fund, &QRadioButton::toggled, this, update_boxes);
    connect(m_col_wallet, &QRadioButton::toggled, this, update_boxes);
    connect(m_col_external, &QRadioButton::toggled, this, update_boxes);
    update_boxes();
    return page;
}

QWidget* RegisterMasternodeWizard::createServicePage()
{
    auto* page{new QWidget(this)};
    auto* layout{makePageLayout(page)};
    layout->addWidget(makePageTitle(tr("Service addresses"), page));
    auto* block{makeBlock(layout)};
    block->addWidget(makeHint(tr("Public addresses your masternode will serve the Core P2P network on, separated "
                                 "by commas or spaces. Each entry must be unique on the network."),
                              page));
    m_service_edit = new QLineEdit(page);
    m_service_edit->setPlaceholderText(QString("1.2.3.4:%1").arg(Params().GetDefaultPort()));
    block->addWidget(m_service_edit);
    block->addWidget(makeHint(tr("May be left empty; the masternode then stays inactive until you send a service "
                                 "update with an address."),
                              page));
    layout->addStretch();
    return page;
}

QWidget* RegisterMasternodeWizard::createKeysPage()
{
    auto* page{new QWidget(this)};
    auto* layout{makePageLayout(page)};
    layout->addWidget(makePageTitle(tr("Keys"), page));

    auto [key_container, key_layout]{
        MasternodeWidgetUtil::makeScrollBody(page, layout, Qt::ScrollBarAlwaysOff)};

    // Owner and voting address rows share the "fill in a fresh wallet address"
    // button, differing only in the field they write to.
    const auto address_row = [this, key_container](QValidatedLineEdit*& edit) {
        auto* row{new QHBoxLayout()};
        edit = new QValidatedLineEdit(key_container);
        GUIUtil::setupAddressWidget(edit, this);
        row->addWidget(edit, /*stretch=*/1);
        auto* fresh{new QPushButton(tr("Use new address"), key_container)};
        QValidatedLineEdit* const target{edit};
        connect(fresh, &QPushButton::clicked, this, [this, target] {
            QString err;
            const QString addr{MasternodeWidgetUtil::freshAddress(m_walletModel, err)};
            if (addr.isEmpty()) {
                showError(err);
            } else {
                target->setText(addr);
            }
        });
        row->addWidget(fresh);
        return row;
    };

    auto* owner_block{makeBlock(key_layout)};
    owner_block->addWidget(makeBlockTitle(tr("Owner address"), key_container));
    owner_block->addWidget(makeHint(tr("Controls this masternode (P2PKH): its key signs registrar updates. Use "
                                       "a new address to keep that key in this wallet, or enter an address "
                                       "controlled by the owner."),
                                    key_container));
    owner_block->addLayout(address_row(m_owner_edit));

    auto* voting_block{makeBlock(key_layout)};
    voting_block->addWidget(makeBlockTitle(tr("Voting address"), key_container));
    voting_block->addWidget(makeHint(tr("May be delegated (P2PKH). Leave empty to vote with the owner key; use "
                                        "a new address to keep a separate voting key in this wallet."),
                                     key_container));
    voting_block->addLayout(address_row(m_voting_edit));
    m_voting_edit->setPlaceholderText(tr("Leave empty to use the owner address"));

    auto* operator_block{makeBlock(key_layout)};
    operator_block->addWidget(makeBlockTitle(tr("Operator key"), key_container));
    operator_block->addWidget(makeHint(tr("The operator runs the masternode server (BLS); only the public key is "
                                          "registered on-chain."),
                                       key_container));
    m_operator_widget = new OperatorKeyWidget(key_container);
    operator_block->addWidget(makeHint(tr("A generated secret key is shown and must be confirmed before registering. "
                                          "It is not stored in this wallet."),
                                       key_container));
    operator_block->addWidget(m_operator_widget);
    key_layout->addStretch();
    return page;
}

QWidget* RegisterMasternodeWizard::createPayoutPage()
{
    auto* page{new QWidget(this)};
    auto* layout{makePageLayout(page)};
    layout->addWidget(makePageTitle(tr("Payout"), page));

    auto* payout_block{makeBlock(layout)};
    payout_block->addWidget(makeBlockTitle(tr("Payout address"), page));
    payout_block->addWidget(makeHint(tr("Receives this masternode's block rewards (P2PKH or P2SH)."), page));
    auto* payout_row{new QHBoxLayout()};
    m_payout_edit = new QValidatedLineEdit(page);
    GUIUtil::setupAddressWidget(m_payout_edit, this);
    payout_row->addWidget(m_payout_edit, /*stretch=*/1);
    auto* payout_fresh{new QPushButton(tr("Use new address"), page)};
    connect(payout_fresh, &QPushButton::clicked, this, [this] {
        QString err;
        const QString addr{MasternodeWidgetUtil::freshAddress(m_walletModel, err)};
        if (addr.isEmpty()) {
            showError(err);
        } else {
            m_payout_edit->setText(addr);
        }
    });
    payout_row->addWidget(payout_fresh);
    payout_block->addLayout(payout_row);

    auto* reward_block{makeBlock(layout)};
    reward_block->addWidget(makeBlockTitle(tr("Operator reward"), page));
    reward_block->addWidget(makeHint(tr("Share of the reward promised to the operator."), page));
    m_operator_reward = new QDoubleSpinBox(page);
    m_operator_reward->setRange(0.0, 100.0);
    m_operator_reward->setDecimals(2);
    m_operator_reward->setSuffix(QString::fromUtf8(" %"));
    m_operator_reward->setMinimumWidth(120);
    m_operator_reward->setMaximumWidth(160);
    auto* reward_row{new QHBoxLayout()};
    reward_row->addWidget(m_operator_reward);
    reward_row->addStretch();
    reward_block->addLayout(reward_row);
    m_reward_warning = makeHint(tr("The operator will permanently receive this share of all rewards of this "
                                   "masternode. Leave it at 0 unless you have an agreement with your operator."),
                                page);
    m_reward_warning->setStyleSheet(GUIUtil::getThemedStyleQString(GUIUtil::ThemedStyle::TS_WARNING));
    m_reward_warning->setVisible(false);
    reward_block->addWidget(m_reward_warning);
    connect(m_operator_reward, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            [this](double value) { m_reward_warning->setVisible(value > 0.0); });
    layout->addStretch();
    return page;
}

QWidget* RegisterMasternodeWizard::createPlatformPage()
{
    auto* page{new QWidget(this)};
    auto* layout{makePageLayout(page)};
    layout->addWidget(makePageTitle(tr("Platform services"), page));

    auto* nodeid_block{makeBlock(layout)};
    nodeid_block->addWidget(makeBlockTitle(tr("Platform node ID"), page));
    nodeid_block->addWidget(makeHint(tr("Derived from the Platform P2P public key (40 hexadecimal characters)."),
                                     page));
    m_platform_nodeid = new QLineEdit(page);
    m_platform_nodeid->setMaxLength(40);
    m_platform_nodeid->setPlaceholderText(QString("f2dbd9b0a1f541a7c44d34a58674d0262f5feca5"));
    nodeid_block->addWidget(m_platform_nodeid);

    // Only one of the two cards is ever shown: which one depends on whether v24
    // is active, and with it on the ProTx version the node will build.
    {
        auto card{makeOptionCard(page, makeBlockTitle(tr("Platform addresses"), page),
                                 tr("ADDR:PORT entries, separated by commas or spaces."))};
        m_platform_addr_box = card.card;
        card.body_layout->addWidget(makeHint(tr("Platform P2P"), m_platform_addr_box));
        m_platform_p2p = new QLineEdit(card.body);
        m_platform_p2p->setPlaceholderText(QString("1.2.3.4:26656"));
        card.body_layout->addWidget(m_platform_p2p);
        card.body_layout->addWidget(makeHint(tr("Platform HTTPS API"), m_platform_addr_box));
        m_platform_https = new QLineEdit(card.body);
        m_platform_https->setPlaceholderText(QString("1.2.3.4:443"));
        card.body_layout->addWidget(m_platform_https);
        layout->addWidget(m_platform_addr_box);
    }

    {
        auto card{makeOptionCard(page, makeBlockTitle(tr("Platform ports"), page),
                                 tr("Before v24 activation only the Platform ports can be registered; they apply "
                                    "to the first service address."))};
        m_platform_port_box = card.card;
        auto* row{new QHBoxLayout()};
        row->addWidget(new QLabel(tr("Platform P2P port:"), card.body));
        m_platform_p2p_port = new QSpinBox(card.body);
        m_platform_p2p_port->setRange(1, 65535);
        m_platform_p2p_port->setValue(Params().GetDefaultPlatformP2PPort());
        row->addWidget(m_platform_p2p_port);
        row->addSpacing(GROUP_SPACING);
        row->addWidget(new QLabel(tr("Platform HTTPS port:"), card.body));
        m_platform_https_port = new QSpinBox(card.body);
        m_platform_https_port->setRange(1, 65535);
        m_platform_https_port->setValue(Params().GetDefaultPlatformHTTPPort());
        row->addWidget(m_platform_https_port);
        row->addStretch();
        card.body_layout->addLayout(row);
        layout->addWidget(m_platform_port_box);
    }
    layout->addStretch();
    return page;
}

QWidget* RegisterMasternodeWizard::createFeePage()
{
    auto* page{new QWidget(this)};
    auto* layout{makePageLayout(page)};
    layout->addWidget(makePageTitle(tr("Fee source"), page));
    auto* block{makeBlock(layout)};
    m_fee_explain = makeHint(QString(), page);
    block->addWidget(m_fee_explain);
    m_fee_picker = new FeeSourcePicker(page);
    m_fee_picker->setWalletModel(m_walletModel);
    block->addWidget(m_fee_picker);
    layout->addStretch();
    return page;
}

QWidget* RegisterMasternodeWizard::createReviewPage()
{
    auto* page{new QWidget(this)};
    auto* layout{makePageLayout(page)};
    layout->addWidget(makePageTitle(tr("Review"), page));
    // An EvoNode summary is a third longer than a masternode one, so the review
    // scrolls instead of squeezing its cards
    const auto review{MasternodeWidgetUtil::makeScrollBody(page, layout, Qt::ScrollBarAlwaysOff)};
    m_review_container = review.container;
    m_review_layout = review.layout;
    m_review_layout->addStretch();
    return page;
}

QWidget* RegisterMasternodeWizard::createSignPage()
{
    auto* page{new QWidget(this)};
    auto* layout{makePageLayout(page)};
    layout->addWidget(makePageTitle(tr("Prove collateral ownership"), page));
    auto* message_block{makeBlock(layout)};
    m_sign_address_label = makeHint(QString(), page);
    message_block->addWidget(m_sign_address_label);
    message_block->addWidget(makeHint(tr("Sign the following message with the collateral key (for example with "
                                         "your hardware wallet's sign-message feature), then paste the base64 "
                                         "signature below."),
                                      page));
    m_sign_message = new QPlainTextEdit(page);
    m_sign_message->setReadOnly(true);
    m_sign_message->setMaximumHeight(90);
    message_block->addWidget(m_sign_message);
    auto* copy_row{new QHBoxLayout()};
    auto* copy_button{new QPushButton(tr("Copy message"), page)};
    connect(copy_button, &QPushButton::clicked, this,
            [this] { GUIUtil::setClipboard(m_sign_message->toPlainText()); });
    copy_row->addWidget(copy_button);
    copy_row->addStretch();
    message_block->addLayout(copy_row);

    auto* signature_block{makeBlock(layout)};
    signature_block->addWidget(makeBlockTitle(tr("Signature"), page));
    m_sig_edit = new QPlainTextEdit(page);
    m_sig_edit->setPlaceholderText(tr("Paste the base64 signature here"));
    m_sig_edit->setMaximumHeight(90);
    signature_block->addWidget(m_sig_edit);
    layout->addStretch();
    return page;
}

QWidget* RegisterMasternodeWizard::createSecretPage()
{
    auto* page{new QWidget(this)};
    auto* layout{makePageLayout(page)};
    layout->addWidget(makePageTitle(tr("Save operator key"), page));
    layout->addWidget(makeHint(tr("Save this generated key before registering. It is kept nowhere else and cannot "
                                  "be recovered from the wallet."),
                               page));

    auto [secret_container, secret_layout]{
        MasternodeWidgetUtil::makeScrollBody(page, layout, Qt::ScrollBarAlwaysOff)};

    auto* secret_box{makeCard(secret_container)};
    auto* box{new QVBoxLayout(secret_box)};
    box->setContentsMargins(CARD_PADDING, CARD_PADDING, CARD_PADDING, CARD_PADDING);
    box->setSpacing(TITLE_SPACING);
    box->addWidget(makeBlockTitle(tr("Operator secret key"), secret_box));
    m_secret_note = makeHint(tr("Save it now — registration cannot start until you confirm it."), secret_box);
    m_secret_note->setStyleSheet(GUIUtil::getThemedStyleQString(GUIUtil::ThemedStyle::TS_WARNING));
    box->addWidget(m_secret_note);
    m_secret_edit = new QLineEdit(secret_box);
    m_secret_edit->setReadOnly(true);
    m_secret_edit->setFont(GUIUtil::fixedPitchFont());
    box->addWidget(m_secret_edit);
    box->addWidget(makeHint(tr("Add this line to dash.conf on your masternode server:"), secret_box));
    auto* conf_row{new QHBoxLayout()};
    m_conf_line_edit = new QLineEdit(secret_box);
    m_conf_line_edit->setReadOnly(true);
    m_conf_line_edit->setFont(GUIUtil::fixedPitchFont());
    conf_row->addWidget(m_conf_line_edit, /*stretch=*/1);
    auto* copy_conf{new QPushButton(tr("Copy"), secret_box)};
    connect(copy_conf, &QPushButton::clicked, this, [this] { GUIUtil::setClipboard(m_conf_line_edit->text()); });
    conf_row->addWidget(copy_conf);
    box->addLayout(conf_row);

    auto* confirm_box{new QWidget(secret_box)};
    auto* confirm_layout{new QVBoxLayout(confirm_box)};
    confirm_layout->setContentsMargins(0, 0, 0, 0);
    confirm_layout->setSpacing(TITLE_SPACING);
    confirm_layout->addWidget(makeHint(tr("Type the last 4 characters of the secret key to confirm you saved it "
                                          "before registering:"),
                                       confirm_box));
    m_confirm_edit = new QLineEdit(confirm_box);
    m_confirm_edit->setMaxLength(4);
    m_confirm_edit->setMaximumWidth(120);
    confirm_layout->addWidget(m_confirm_edit);
    box->addWidget(confirm_box);
    secret_layout->addWidget(secret_box);
    secret_layout->addStretch();
    return page;
}

QWidget* RegisterMasternodeWizard::createResultPage()
{
    auto* page{new QWidget(this)};
    auto* layout{makePageLayout(page)};
    layout->addWidget(makePageTitle(tr("Masternode registered"), page));

    auto [result_container, result_layout]{
        MasternodeWidgetUtil::makeScrollBody(page, layout, Qt::ScrollBarAlwaysOff)};

    // What happened
    {
        auto* card{makeCard(result_container)};
        auto* box{new QVBoxLayout(card)};
        box->setContentsMargins(CARD_PADDING, CARD_PADDING, CARD_PADDING, CARD_PADDING);
        box->setSpacing(TITLE_SPACING);
        m_result_label = makeHint(QString(), card);
        box->addWidget(m_result_label);
        box->addWidget(makeBlockTitle(tr("Provider transaction hash"), card));
        m_result_hash = makeValue(QString(), card, /*monospace=*/true);
        box->addWidget(m_result_hash);
        m_result_tx_note = makeHint(QString(), card);
        box->addWidget(m_result_tx_note);
        result_layout->addWidget(card);
    }

    // What to do next
    {
        auto* card{makeCard(result_container)};
        auto* box{new QVBoxLayout(card)};
        box->setContentsMargins(CARD_PADDING, CARD_PADDING, CARD_PADDING, CARD_PADDING);
        box->setSpacing(TITLE_SPACING);
        box->addWidget(makeBlockTitle(tr("Next steps"), card));
        m_next_steps = new QLabel(card);
        m_next_steps->setWordWrap(true);
        m_next_steps->setTextInteractionFlags(Qt::TextSelectableByMouse);
        box->addWidget(m_next_steps);
        result_layout->addWidget(card);
    }
    result_layout->addStretch();
    return page;
}

bool RegisterMasternodeWizard::isEvo() const
{
    return m_type_evo->isChecked();
}

bool RegisterMasternodeWizard::usesExtendedAddresses() const
{
    return m_node.evo().getProviderTxCapabilities().extended_addresses;
}

bool RegisterMasternodeWizard::isExternalCollateral() const
{
    return m_col_external->isChecked();
}

bool RegisterMasternodeWizard::isFundCollateral() const
{
    return m_col_fund->isChecked();
}

std::optional<CTxDestination> RegisterMasternodeWizard::knownCollateralDestination() const
{
    if (isFundCollateral()) {
        const CTxDestination destination{DecodeDestination(collateralAddress().toStdString())};
        if (IsValidDestination(destination)) return destination;
        return std::nullopt;
    }
    if (!m_col_wallet->isChecked() || m_col_utxo_combo->currentIndex() < 0) {
        return std::nullopt;
    }
    const CTxDestination destination{DecodeDestination(
        m_col_utxo_combo->itemData(m_col_utxo_combo->currentIndex(), COLLATERAL_ADDRESS_ROLE).toString().toStdString())};
    return IsValidDestination(destination) ? std::optional{destination} : std::nullopt;
}

QString RegisterMasternodeWizard::collateralAddress() const
{
    return isFundCollateral() ? m_col_address->text().trimmed() : QString();
}

QString RegisterMasternodeWizard::ownerAddress() const
{
    return m_owner_edit->text().trimmed();
}

QString RegisterMasternodeWizard::votingAddress() const
{
    return m_voting_edit->text().trimmed();
}

CAmount RegisterMasternodeWizard::collateralAmount() const
{
    return GetMnType(isEvo() ? MnType::Evo : MnType::Regular).collat_amount;
}

bool RegisterMasternodeWizard::secretGateRequired() const
{
    return m_operator_widget->hasGeneratedSecret();
}

bool RegisterMasternodeWizard::secretConfirmed() const
{
    return m_confirm_edit->text().trimmed().compare(m_operator_widget->secretHex().right(4),
                                                    Qt::CaseInsensitive) == 0;
}

void RegisterMasternodeWizard::rebuildOrder()
{
    const std::optional<Page> current{m_order.isEmpty() ? std::nullopt : std::optional<Page>{currentPage()}};
    m_order = {PageType, PageCollateral, PageService, PageKeys, PagePayout};
    if (isEvo()) m_order << PagePlatform;
    m_order << PageFee << PageReview;
    if (m_operator_widget != nullptr && secretGateRequired()) m_order << PageSecret;
    if (isExternalCollateral()) m_order << PageSign;
    m_order << PageResult;
    if (current && m_order.contains(*current)) {
        m_pos = m_order.indexOf(*current);
    } else if (m_pos >= m_order.size()) {
        m_pos = m_order.size() - 1;
    }
    updateProgress();
}

RegisterMasternodeWizard::Page RegisterMasternodeWizard::currentPage() const
{
    return m_order.isEmpty() ? PageType : m_order.at(m_pos);
}

QString RegisterMasternodeWizard::pageTitle(Page page) const
{
    switch (page) {
    case PageType:
        return tr("Masternode type");
    case PageCollateral:
        return tr("Collateral");
    case PageService:
        return tr("Service addresses");
    case PageKeys:
        return tr("Keys");
    case PagePayout:
        return tr("Payout");
    case PagePlatform:
        return tr("Platform services");
    case PageFee:
        return tr("Fee source");
    case PageReview:
        return tr("Review");
    case PageSecret:
        return tr("Save operator key");
    case PageSign:
        return tr("Prove collateral ownership");
    case PageResult:
        return tr("Complete");
    }
    return {};
}

void RegisterMasternodeWizard::updateProgress()
{
    if (m_progress_label == nullptr || m_order.isEmpty()) return;
    if (currentPage() == PageResult) {
        m_progress_label->setText(tr("Complete"));
        return;
    }
    const int total{m_order.size() - 1};
    m_progress_label->setText(tr("Step %1 of %2 · %3").arg(m_pos + 1).arg(total).arg(pageTitle(currentPage())));
}

void RegisterMasternodeWizard::goToPage(Page page)
{
    const int pos{m_order.indexOf(page)};
    if (pos < 0) return;
    m_pos = pos;
    enterPage(page);
}

void RegisterMasternodeWizard::enterPage(Page page)
{
    m_pages->setCurrentIndex(page);
    m_validation_page.reset();
    showError(QString());
    if (page != PageReview) m_unlock.reset();
    switch (page) {
    case PageCollateral:
        if (m_col_address->text().isEmpty() && m_walletModel != nullptr) {
            QString err;
            const QString addr{MasternodeWidgetUtil::freshAddress(m_walletModel, err)};
            if (!addr.isEmpty()) m_col_address->setText(addr);
        }
        refreshCollateralCandidates();
        break;
    case PageKeys: {
        if (m_owner_edit->text().isEmpty() && m_walletModel != nullptr) {
            QString err;
            const QString addr{MasternodeWidgetUtil::freshAddress(m_walletModel, err)};
            if (!addr.isEmpty()) m_owner_edit->setText(addr);
        }
        break;
    }
    case PagePayout:
        if (m_payout_edit->text().isEmpty() && m_walletModel != nullptr) {
            QString err;
            const QString addr{MasternodeWidgetUtil::freshAddress(m_walletModel, err)};
            if (!addr.isEmpty()) m_payout_edit->setText(addr);
        }
        break;
    case PagePlatform: {
        const auto capabilities{m_node.evo().getProviderTxCapabilities()};
        const bool v3{capabilities.extended_addresses};
        m_platform_provider_version = capabilities.version;
        m_platform_ipv4_optional = capabilities.ipv4_optional;
        m_platform_extended_addresses = v3;
        m_platform_addr_box->setVisible(v3);
        m_platform_port_box->setVisible(!v3);
        break;
    }
    case PageFee: {
        // The exact fee depends on selected coins and current wallet fee
        // settings. Do not reject a viable source using a fixed estimate; the
        // typed funding operation remains the authority.
        const CAmount required{isFundCollateral() ? collateralAmount() : 0};
        std::optional<COutPoint> excluded_outpoint;
        if (m_col_wallet->isChecked() && m_col_utxo_combo->currentIndex() >= 0) {
            uint256 hash;
            hash.SetHex(m_col_utxo_combo->currentData().toString().toStdString());
            excluded_outpoint.emplace(
                hash, static_cast<uint32_t>(m_col_utxo_combo->itemData(m_col_utxo_combo->currentIndex(), VOUT_ROLE).toInt()));
        }
        m_fee_picker->setExcludedOutpoint(std::move(excluded_outpoint));
        m_fee_picker->setMinimumBalance(required);
        m_fee_picker->refresh();
        if (isFundCollateral()) {
            m_fee_explain->setText(
                tr("The selected address funds the %1 collateral plus the transaction fee. The exact fee is "
                   "calculated from your wallet settings when you register, and change returns to this address.")
                    .arg(FormatCollateralAmount(m_walletModel, collateralAmount())));
        } else {
            m_fee_explain->setText(tr("The selected address pays the transaction fee."));
        }
        break;
    }
    case PageReview:
        populateReview();
        break;
    case PageSecret:
        populateSecret();
        break;
    default:
        break;
    }
    updateProgress();
    updateButtons();
}

bool RegisterMasternodeWizard::validatePage(Page page, QString& err)
{
    err.clear();
    switch (page) {
    case PageCollateral:
        if (isFundCollateral()) {
            if (!MasternodeWidgetUtil::isP2PKHorP2SHAddress(collateralAddress())) {
                err = tr("Enter a valid collateral address (P2PKH or P2SH).");
            }
        } else if (m_col_wallet->isChecked()) {
            if (m_col_utxo_combo->currentIndex() < 0) {
                err = tr("This wallet has no unspent output of exactly %1. Fund the collateral from the wallet "
                         "instead.")
                          .arg(FormatCollateralAmount(m_walletModel, collateralAmount()));
            }
        } else {
            const QString txid{m_col_txid->text().trimmed()};
            uint256 hash;
            hash.SetHex(txid.toStdString());
            if (txid.length() != 64 || !IsHex(txid.toStdString()) || hash.IsNull()) {
                err = tr("Enter the collateral transaction id as 64 hexadecimal characters.");
            }
        }
        break;
    case PageService: {
        const QStringList list{MasternodeWidgetUtil::tokenizeEndpointList(m_service_edit->text())};
        if (list.isEmpty() && isEvo() && !usesExtendedAddresses()) {
            err = tr("EvoNodes need at least one service address to carry the Platform ports before v24 "
                     "activation.");
        } else if (!isEvo()) {
            validateProviderNetInfo(/*optional=*/true, err);
        } else if (!list.isEmpty()) {
            interfaces::ProviderNetInfo core_only;
            for (const QString& entry : list) core_only.core_p2p.push_back(entry.toStdString());
            const auto capabilities{m_node.evo().getProviderTxCapabilities()};
            if (const auto error{m_node.evo().validateProviderNetInfo(
                    core_only, MnType::Regular, capabilities.version, capabilities.ipv4_optional, /*optional=*/true)}) {
                err = QString::fromStdString(error->message.translated);
            }
        }
        break;
    }
    case PageKeys: {
        const CTxDestination owner_destination{DecodeDestination(ownerAddress().toStdString())};
        const auto* owner{std::get_if<PKHash>(&owner_destination)};
        const QString voting_text{votingAddress()};
        const CTxDestination voting_destination{
            DecodeDestination((voting_text.isEmpty() ? ownerAddress() : voting_text).toStdString())};
        const auto* voting{std::get_if<PKHash>(&voting_destination)};
        const auto collateral_destination{knownCollateralDestination()};
        if (!owner || ToKeyID(*owner).IsNull()) {
            err = tr("Enter a valid owner address (P2PKH).");
        } else if (!voting || ToKeyID(*voting).IsNull()) {
            err = tr("Enter a valid voting address (P2PKH), or leave it empty to use the owner address.");
        } else if (collateral_destination &&
                   (*collateral_destination == owner_destination || *collateral_destination == voting_destination)) {
            err = tr("The owner and voting addresses must differ from the collateral address.");
        } else if (!m_operator_widget->isValid()) {
            err = tr("Enter a valid operator BLS public key (96 hexadecimal characters, basic scheme).");
        }
        break;
    }
    case PagePayout: {
        const QString payout{m_payout_edit->text().trimmed()};
        const CTxDestination payout_destination{DecodeDestination(payout.toStdString())};
        const CTxDestination owner_destination{DecodeDestination(ownerAddress().toStdString())};
        const CTxDestination voting_destination{
            DecodeDestination((votingAddress().isEmpty() ? ownerAddress() : votingAddress()).toStdString())};
        if (!MasternodeWidgetUtil::isP2PKHorP2SHAddress(payout)) {
            err = tr("Enter a valid payout address (P2PKH or P2SH).");
        } else if (payout_destination == owner_destination || payout_destination == voting_destination) {
            err = tr("The payout address must differ from the owner and voting addresses.");
        } else if (const auto collateral_destination{knownCollateralDestination()};
                   collateral_destination && payout_destination == *collateral_destination) {
            err = tr("The payout address must differ from the collateral address.");
        }
        break;
    }
    case PagePlatform: {
        const QString nodeid{m_platform_nodeid->text().trimmed()};
        uint160 platform_node_id;
        platform_node_id.SetHex(nodeid.toStdString());
        if (nodeid.length() != 40 || !IsHex(nodeid.toStdString()) || platform_node_id.IsNull()) {
            err = tr("Enter the Platform node ID as 40 hexadecimal characters.");
            break;
        }
        if (m_platform_extended_addresses) {
            const QStringList p2p{MasternodeWidgetUtil::tokenizeEndpointList(m_platform_p2p->text())};
            const QStringList https{MasternodeWidgetUtil::tokenizeEndpointList(m_platform_https->text())};
            const bool has_service{!MasternodeWidgetUtil::tokenizeEndpointList(m_service_edit->text()).isEmpty()};
            if (p2p.isEmpty() != https.isEmpty()) {
                err = tr("Enter both a Platform P2P and a Platform HTTPS address; one cannot be registered "
                         "without the other.");
            } else if (has_service && p2p.isEmpty()) {
                err = tr("Enter at least one Platform P2P and one Platform HTTPS address, or clear the service "
                         "addresses to set everything later with a service update.");
            } else if (!has_service && !p2p.isEmpty()) {
                err = tr("Enter at least one Core service address when Platform endpoints are set, or clear all "
                         "service endpoints to set them later with a service update.");
            }
        }
        if (err.isEmpty()) validateProviderNetInfo(/*optional=*/true, err);
        break;
    }
    case PageFee:
        if (m_fee_picker->currentIndex() < 0) {
            err = isFundCollateral() ?
                      tr("No spendable wallet address holds the %1 collateral amount. The selected address must "
                         "also have enough for the fee calculated when you register.")
                          .arg(FormatCollateralAmount(m_walletModel, collateralAmount())) :
                      tr("No spendable wallet address has a positive balance available to pay the transaction "
                         "fee.");
        }
        break;
    case PageSign: {
        const QString sig{m_sig_edit->toPlainText().simplified().remove(' ')};
        if (sig.isEmpty() || !DecodeBase64(sig.toStdString()).has_value()) {
            err = tr("Paste the base64-encoded signature of the message above, made with the collateral key.");
        }
        break;
    }
    default:
        break;
    }
    return err.isEmpty();
}

interfaces::ProviderNetInfo RegisterMasternodeWizard::providerNetInfo() const
{
    interfaces::ProviderNetInfo net_info;
    for (const QString& entry : MasternodeWidgetUtil::tokenizeEndpointList(m_service_edit->text())) {
        net_info.core_p2p.push_back(entry.toStdString());
    }
    if (!isEvo()) return net_info;

    if (m_platform_extended_addresses) {
        std::vector<std::string> p2p;
        for (const QString& entry : MasternodeWidgetUtil::tokenizeEndpointList(m_platform_p2p->text())) {
            p2p.push_back(entry.toStdString());
        }
        std::vector<std::string> https;
        for (const QString& entry : MasternodeWidgetUtil::tokenizeEndpointList(m_platform_https->text())) {
            https.push_back(entry.toStdString());
        }
        net_info.platform_p2p = std::move(p2p);
        net_info.platform_https = std::move(https);
    } else {
        net_info.platform_p2p = static_cast<uint16_t>(m_platform_p2p_port->value());
        net_info.platform_https = static_cast<uint16_t>(m_platform_https_port->value());
    }
    return net_info;
}

bool RegisterMasternodeWizard::validateProviderNetInfo(bool optional, QString& err) const
{
    // The Platform page pins the rules it was entered with, so typing there doesn't wait for cs_main
    const interfaces::ProviderTxCapabilities pinned{.version = m_platform_provider_version,
                                                    .extended_addresses = m_platform_extended_addresses,
                                                    .ipv4_optional = m_platform_ipv4_optional};
    const auto capabilities{isEvo() && m_platform_provider_version != 0 ? pinned
                                                                        : m_node.evo().getProviderTxCapabilities()};
    const MnType type{isEvo() ? MnType::Evo : MnType::Regular};
    if (const auto error{m_node.evo().validateProviderNetInfo(providerNetInfo(), type, capabilities.version,
                                                              capabilities.ipv4_optional, optional)}) {
        err = QString::fromStdString(error->message.translated);
        return false;
    }
    return true;
}

void RegisterMasternodeWizard::onNext()
{
    if (m_busy) return;
    const Page current{currentPage()};
    QString err;
    if (!validatePage(current, err)) {
        m_validation_page = current;
        showError(err);
        updateButtons();
        return;
    }
    m_validation_page.reset();
    showError(QString());
    if (current == PageReview) {
        if (secretGateRequired()) {
            goToPage(PageSecret);
        } else {
            startRegistration();
        }
    } else if (current == PageSecret) {
        startRegistration();
    } else if (current == PageSign) {
        startSubmit();
    } else if (current == PageResult) {
        accept();
    } else {
        ++m_pos;
        enterPage(m_order[m_pos]);
    }
}

void RegisterMasternodeWizard::onBack()
{
    if (m_busy || m_pos == 0) return;
    --m_pos;
    enterPage(m_order[m_pos]);
}

void RegisterMasternodeWizard::updateButtons()
{
    const Page current{currentPage()};
    m_back_button->setVisible(current != PageType && current != PageResult);
    // After register_prepare the transaction is fixed; going back would
    // silently invalidate the message being signed.
    m_back_button->setEnabled(!m_busy && current != PageSign);
    m_back_button->setToolTip(current == PageSign ? tr("Back is unavailable after preparation because changing the "
                                                       "registration would invalidate the message being signed.")
                                                  : QString{});
    m_cancel_button->setVisible(current != PageResult);
    m_cancel_button->setEnabled(!m_busy);

    if (!m_busy) {
        QString next_text{tr("Next")};
        if (current == PageReview) {
            next_text = secretGateRequired() ? tr("Continue") : (isExternalCollateral() ? tr("Prepare") : tr("Register"));
        } else if (current == PageSecret) {
            next_text = isExternalCollateral() ? tr("Prepare") : tr("Register");
        } else if (current == PageSign) {
            next_text = tr("Submit");
        } else if (current == PageResult) {
            next_text = tr("Finish");
        }
        m_next_button->setText(next_text);
    }

    bool enabled{!m_busy};
    QString tooltip;
    if (m_walletModel == nullptr) {
        enabled = false;
        tooltip = tr("No wallet is available. Masternode registration requires a wallet.");
    } else if (m_walletModel->wallet().privateKeysDisabled()) {
        enabled = false;
        tooltip = tr("This wallet is watch-only. Masternode registration requires a wallet that can sign "
                     "transactions.");
    } else if (current == PageSecret && !secretConfirmed()) {
        enabled = false;
        tooltip = tr("Confirm you saved the operator secret key by typing its last 4 characters.");
    } else if (m_validation_page && *m_validation_page == current) {
        QString error;
        if (!validatePage(current, error)) {
            enabled = false;
            tooltip = error;
        }
    }
    m_next_button->setEnabled(enabled);
    m_next_button->setToolTip(tooltip);
}

void RegisterMasternodeWizard::onPageEdited()
{
    if (m_validation_page && *m_validation_page == currentPage()) {
        QString error;
        validatePage(currentPage(), error);
        showError(error);
        if (error.isEmpty()) m_validation_page.reset();
    } else if (!m_error_label->text().isEmpty()) {
        showError(QString());
    }
    updateButtons();
}

void RegisterMasternodeWizard::setBusy(bool busy, const QString& busy_text)
{
    m_busy = busy;
    m_busy_bar->setVisible(busy);
    if (busy) {
        m_next_button->setText(busy_text);
    }
    updateButtons();
}

void RegisterMasternodeWizard::showError(const QString& message)
{
    m_error_label->setText(message);
}

void RegisterMasternodeWizard::refreshCollateralCandidates()
{
    if (m_walletModel == nullptr) {
        m_col_utxo_combo->clear();
        m_col_utxo_none->setVisible(true);
        return;
    }
    refreshCollateralCandidates(RegisteredCollaterals(m_walletModel->wallet()));
}

void RegisterMasternodeWizard::refreshCollateralCandidates(const std::set<COutPoint>& registered_collaterals)
{
    m_col_utxo_combo->clear();
    if (m_walletModel == nullptr) return;
    const CAmount collateral{collateralAmount()};
    for (const auto& [dest, coins] : m_walletModel->wallet().listCoins()) {
        for (const auto& [outpoint, txout] : coins) {
            if (txout.txout.nValue != collateral || txout.is_spent || txout.depth_in_main_chain < 1) continue;
            if (registered_collaterals.count(outpoint) != 0) continue;
            if (m_walletModel->wallet().isLockedCoin(outpoint)) continue;
            if (!m_walletModel->wallet().isSpendable(txout.txout.scriptPubKey)) continue;
            // protx register only accepts P2PKH collaterals; check the coin's own
            // script, as listCoins() groups change under the parent's address
            CTxDestination coin_dest;
            if (!ExtractDestination(txout.txout.scriptPubKey, coin_dest) ||
                !std::holds_alternative<PKHash>(coin_dest)) {
                continue;
            }
            const QString address{QString::fromStdString(EncodeDestination(coin_dest))};
            const QString txid{QString::fromStdString(outpoint.hash.ToString())};
            m_col_utxo_combo->addItem(QString("%1:%2 (%3)").arg(txid).arg(outpoint.n).arg(address), txid);
            m_col_utxo_combo->setItemData(m_col_utxo_combo->count() - 1, static_cast<int>(outpoint.n), VOUT_ROLE);
            m_col_utxo_combo->setItemData(m_col_utxo_combo->count() - 1, address, COLLATERAL_ADDRESS_ROLE);
        }
    }
    // An empty combo box says nothing; show the reason instead
    const bool have_candidates{m_col_utxo_combo->count() > 0};
    m_col_utxo_combo->setVisible(have_candidates);
    m_col_utxo_none->setVisible(!have_candidates);
    m_col_utxo_none->setText(tr("This wallet has no unspent output of exactly %1 with a confirmation. Send the "
                                "collateral from this wallet instead, or reference one held elsewhere.")
                                 .arg(FormatCollateralAmount(m_walletModel, collateralAmount())));
}

int RegisterMasternodeWizard::reviewLabelWidth(const QWidget* card) const
{
    // The widest label decides the column, so every card shares one alignment
    static const QStringList labels{tr("Type"),         tr("Service"),        tr("Platform node ID"),
                                    tr("Platform P2P"), tr("Platform HTTPS"), tr("Platform ports"),
                                    tr("Amount"),       tr("Destination"),    tr("Output"),
                                    tr("Source"),       tr("Owner"),          tr("Voting"),
                                    tr("Operator"),     tr("Address"),        tr("Operator share"),
                                    tr("Fee source"),   tr("Network fee")};
    const QFontMetrics metrics{card->font()};
    int width{0};
    for (const QString& label : labels) {
        width = std::max(width, metrics.horizontalAdvance(label));
    }
    return width;
}

void RegisterMasternodeWizard::populateReview()
{
    // The sections mirror the wizard's own pages, so a wrong value can be traced
    // back to the page that set it
    // Two columns: a label column just wide enough for its text, and the value
    // column taking the rest, so every row lines up down the card
    QGridLayout* section{nullptr};
    QWidget* section_card{nullptr};
    const auto begin_section = [this, &section, &section_card](const QString& title) {
        // makeCard() hands back a bare frame; the card owns the layout we build here
        auto* card{makeCard(m_review_container)};
        section_card = card;
        auto* card_layout{new QVBoxLayout(card)};
        card_layout->setContentsMargins(CARD_PADDING, CARD_PADDING, CARD_PADDING, CARD_PADDING);
        card_layout->setSpacing(TITLE_SPACING);
        card_layout->addWidget(makeBlockTitle(title, card));
        section = new QGridLayout();
        section->setContentsMargins(0, 0, 0, 0);
        section->setHorizontalSpacing(GROUP_SPACING);
        section->setVerticalSpacing(ROW_SPACING);
        section->setColumnStretch(1, 1);
        // One label width for every card, so the values line up down the page
        section->setColumnMinimumWidth(0, reviewLabelWidth(card));
        card_layout->addLayout(section);
        m_review_layout->insertWidget(m_review_layout->count() - 1, card);
    };
    const auto row = [&section, &section_card](const QString& key, const QString& value, bool monospace = false) {
        const int r{section->rowCount()};
        section->addWidget(makeHint(key, section_card), r, 0, Qt::AlignLeft | Qt::AlignTop);
        section->addWidget(makeValue(value, section_card, monospace), r, 1);
    };
    // A BLS key is one 96-character word, which a wrapping label refuses to break.
    // Shown in chunks it wraps like the other values, and the Copy button hands
    // out the unbroken key.
    const auto key_row = [&section, &section_card](const QString& key, const QString& value) {
        const int r{section->rowCount()};
        section->addWidget(makeHint(key, section_card), r, 0, Qt::AlignLeft | Qt::AlignTop);
        section->addWidget(MasternodeWidgetUtil::makeCopyableValue(MasternodeWidgetUtil::chunked(value), value,
                                                                   section_card),
                           r, 1);
    };

    // Rebuilt on every entry to the page: the user may have gone back and edited.
    // The trailing stretch stays; only the cards are replaced.
    while (m_review_layout->count() > 1) {
        QLayoutItem* item{m_review_layout->takeAt(0)};
        delete item->widget();
        delete item;
    }

    begin_section(tr("Node"));
    row(tr("Type"), isEvo() ? tr("EvoNode") : tr("Masternode"));
    const QStringList services{MasternodeWidgetUtil::tokenizeEndpointList(m_service_edit->text())};
    row(tr("Service"), services.isEmpty() ? tr("Not set — the node stays inactive until you send a service update") :
                                            services.join(", "),
        !services.isEmpty());
    if (isEvo()) {
        row(tr("Platform node ID"), m_platform_nodeid->text().trimmed(), /*monospace=*/true);
        if (m_platform_extended_addresses) {
            row(tr("Platform P2P"),
                MasternodeWidgetUtil::tokenizeEndpointList(m_platform_p2p->text()).join(", "),
                /*monospace=*/true);
            row(tr("Platform HTTPS"),
                MasternodeWidgetUtil::tokenizeEndpointList(m_platform_https->text()).join(", "),
                /*monospace=*/true);
        } else {
            row(tr("Platform ports"), tr("%1 peer-to-peer, %2 HTTPS")
                                          .arg(m_platform_p2p_port->value())
                                          .arg(m_platform_https_port->value()));
        }
    }

    begin_section(tr("Collateral"));
    row(tr("Amount"), FormatCollateralAmount(m_walletModel, collateralAmount()));
    if (isFundCollateral()) {
        row(tr("Destination"), collateralAddress(), /*monospace=*/true);
        row(tr("Source"), tr("This wallet sends it as part of the registration"));
    } else if (m_col_wallet->isChecked()) {
        row(tr("Output"), m_col_utxo_combo->currentText(), /*monospace=*/true);
        row(tr("Source"), tr("An output this wallet already holds"));
    } else {
        row(tr("Output"), QString("%1:%2").arg(m_col_txid->text().trimmed()).arg(m_col_vout->value()),
            /*monospace=*/true);
        row(tr("Source"), tr("Held outside this wallet — you will sign a message with its key"));
    }

    begin_section(tr("Keys"));
    row(tr("Owner"), ownerAddress(), /*monospace=*/true);
    row(tr("Voting"), votingAddress().isEmpty() ? tr("Same as owner") : votingAddress(),
        !votingAddress().isEmpty());
    const QString operator_key{m_operator_widget->publicKeyHex()};
    key_row(tr("Operator"), operator_key);
    row(QString(), operatorKeyProvenance());

    begin_section(tr("Payout"));
    row(tr("Address"), m_payout_edit->text().trimmed(), /*monospace=*/true);
    row(tr("Operator share"), QString("%1%").arg(QString::number(m_operator_reward->value(), 'f', 2)));

    begin_section(tr("Funding"));
    const QString fee_source{m_fee_picker->selectedAddress()};
    row(tr("Fee source"), fee_source.isEmpty() ? tr("Not selected") : fee_source,
        /*monospace=*/!fee_source.isEmpty());
    row(tr("Network fee"), tr("Calculated at submission using the wallet's current fee settings"));

    m_review_layout->insertWidget(
        m_review_layout->count() - 1,
        makeHint(isExternalCollateral() ?
                     tr("Preparing creates the unsigned transaction and the message to sign with the collateral "
                        "key. Nothing is broadcast yet.") :
                     tr("Registering broadcasts a transaction from this wallet. You may be asked to unlock it."),
                 m_review_container));
}

QString RegisterMasternodeWizard::operatorKeyProvenance() const
{
    switch (m_operator_widget->mode()) {
    case OperatorKeyWidget::Mode::GenerateOnly:
        return tr("Newly generated, shown and confirmed before registration, and kept nowhere else");
    case OperatorKeyWidget::Mode::Existing:
        return tr("Supplied by whoever runs the node");
    }
    return {};
}

void RegisterMasternodeWizard::populateSecret()
{
    m_secret_edit->setText(m_operator_widget->secretHex());
    m_conf_line_edit->setText(QString("masternodeblsprivkey=%1").arg(m_operator_widget->secretHex()));
    m_secret_edit->setCursorPosition(0);
    m_conf_line_edit->setCursorPosition(0);
}

std::optional<interfaces::ProviderRegistrationRequest> RegisterMasternodeWizard::buildRegistrationRequest(
    QString& error) const
{
    error.clear();
    interfaces::ProviderRegistrationRequest request;
    request.type = isEvo() ? MnType::Evo : MnType::Regular;
    if (isFundCollateral()) {
        const CTxDestination destination{DecodeDestination(collateralAddress().toStdString())};
        if (!IsValidDestination(destination)) {
            error = tr("The collateral destination is no longer valid.");
            return std::nullopt;
        }
        request.collateral = interfaces::FundProviderCollateral{destination};
    } else if (m_col_wallet->isChecked()) {
        uint256 hash;
        hash.SetHex(m_col_utxo_combo->currentData().toString().toStdString());
        const COutPoint outpoint{hash,
                                 static_cast<uint32_t>(
                                     m_col_utxo_combo->itemData(m_col_utxo_combo->currentIndex(), VOUT_ROLE).toInt())};
        if (RegisteredCollaterals(m_walletModel->wallet()).count(outpoint) != 0) {
            error = tr("The selected collateral output is already used by another masternode.");
            return std::nullopt;
        }
        request.collateral = interfaces::ExistingProviderCollateral{outpoint};
    } else {
        uint256 hash;
        hash.SetHex(m_col_txid->text().trimmed().toStdString());
        request.collateral = interfaces::ExistingProviderCollateral{
            COutPoint{hash, static_cast<uint32_t>(m_col_vout->value())}};
    }

    request.net_info = providerNetInfo();

    const CTxDestination owner_destination{DecodeDestination(ownerAddress().toStdString())};
    const auto* owner{std::get_if<PKHash>(&owner_destination)};
    if (!owner) {
        error = tr("The owner address is no longer valid.");
        return std::nullopt;
    }
    request.owner_key = ToKeyID(*owner);

    if (votingAddress().isEmpty()) {
        request.voting_key = request.owner_key;
    } else {
        const CTxDestination voting_destination{DecodeDestination(votingAddress().toStdString())};
        const auto* voting{std::get_if<PKHash>(&voting_destination)};
        if (!voting) {
            error = tr("The voting address is no longer valid.");
            return std::nullopt;
        }
        request.voting_key = ToKeyID(*voting);
    }

    if (!request.operator_key.SetHexStr(m_operator_widget->publicKeyHex().toStdString(),
                                        /*specificLegacyScheme=*/false)) {
        error = tr("The operator public key is no longer valid.");
        return std::nullopt;
    }
    request.operator_reward = static_cast<uint16_t>(qRound(m_operator_reward->value() * 100.0));

    const CTxDestination payout{DecodeDestination(m_payout_edit->text().trimmed().toStdString())};
    if (!IsValidDestination(payout)) {
        error = tr("The payout address is no longer valid.");
        return std::nullopt;
    }
    request.payouts.push_back({payout, interfaces::ProviderPayout::MAX_REWARD});

    if (isEvo()) {
        uint160 platform_node_id;
        platform_node_id.SetHex(m_platform_nodeid->text().trimmed().toStdString());
        request.platform_node_id = platform_node_id;
    }

    const CTxDestination fee_source{DecodeDestination(m_fee_picker->selectedAddress().toStdString())};
    if (!IsValidDestination(fee_source)) {
        error = tr("The selected fee source is no longer valid.");
        return std::nullopt;
    }
    request.fee_source = fee_source;
    request.submit = true;
    return request;
}

void RegisterMasternodeWizard::completeRegistration(const CTransactionRef& transaction)
{
    if (m_destroying) return;
    populateResult(QString::fromStdString(transaction->GetHash().ToString()));
    goToPage(PageResult);
}

bool RegisterMasternodeWizard::confirmBroadcast()
{
    const QString node_type{isEvo() ? tr("EvoNode") : tr("masternode")};
    QString consequence;
    if (isFundCollateral()) {
        consequence = tr("This transaction creates %1 of collateral from this wallet and registers the %2.")
                          .arg(FormatCollateralAmount(m_walletModel, collateralAmount()), node_type);
    } else if (isExternalCollateral()) {
        consequence = tr("This transaction registers the external %1 collateral and makes it the collateral for this "
                         "%2.")
                          .arg(FormatCollateralAmount(m_walletModel, collateralAmount()), node_type);
    } else {
        consequence = tr("This transaction registers and locks the wallet's %1 collateral output for this %2.")
                          .arg(FormatCollateralAmount(m_walletModel, collateralAmount()), node_type);
    }
    consequence += tr(
        " The network fee is calculated from the selected fee source using the wallet's current fee settings.");

    SendConfirmationDialog confirmation{tr("Confirm %1 registration").arg(node_type),
                                        tr("Do you want to broadcast this %1 registration?").arg(node_type),
                                        consequence,
                                        tr("Fee source: %1").arg(m_fee_picker->selectedAddress()),
                                        SEND_CONFIRM_DELAY,
                                        /*enable_send=*/true,
                                        /*always_show_unsigned=*/false,
                                        this};
    confirmation.setWindowModality(Qt::WindowModal);
    return confirmation.exec() == QMessageBox::Yes;
}

void RegisterMasternodeWizard::startRegistration(bool skip_confirmation)
{
    if (m_walletModel == nullptr || !m_runner) return;
    if (secretGateRequired() && !secretConfirmed()) {
        goToPage(PageSecret);
        return;
    }
    if (isEvo() && m_node.evo().getProviderTxCapabilities().version != m_platform_provider_version) {
        goToPage(PagePlatform);
        showError(tr("The network's provider-transaction rules changed while this wizard was open. Review the "
                     "Platform services page again before registering."));
        return;
    }
    if (!isExternalCollateral() && !skip_confirmation && !confirmBroadcast()) return;
    if (!m_unlock) m_unlock = std::make_unique<UnlockHolder>(*m_walletModel);
    if (!m_unlock->ctx.isValid()) {
        m_unlock.reset();
        return;
    }

    QString error;
    auto request{buildRegistrationRequest(error)};
    if (!request) {
        m_unlock.reset();
        showError(error);
        return;
    }
    if (isExternalCollateral()) {
        m_prepared_collateral_outpoint =
            std::get<interfaces::ExistingProviderCollateral>(request->collateral).outpoint;
    }

    m_stage = isExternalCollateral() ? Stage::Prepare : Stage::Register;
    setBusy(true, isExternalCollateral() ? tr("Preparing…") : tr("Registering…"));
    const bool started{
        isExternalCollateral()
            ? m_runner->prepareMasternodeRegistration(std::move(*request),
                                                      [this](auto result) { finishPrepare(std::move(result)); })
            : m_runner->registerMasternode(
                  std::move(*request), [this](auto result) { finishSubmission(std::move(result)); })};
    if (!started) {
        m_stage = Stage::None;
        m_unlock.reset();
        setBusy(false);
    }
}

void RegisterMasternodeWizard::startSubmit(bool skip_confirmation)
{
    if (m_walletModel == nullptr || !m_runner || !m_prepared_tx) return;
    if (isEvo() && m_node.evo().getProviderTxCapabilities().version != m_platform_provider_version) {
        showError(tr("The network's provider-transaction rules changed while the collateral signature was being "
                     "collected. Discard this prepared registration and review the Platform services page again."));
        return;
    }
    const auto signature{DecodeBase64(m_sig_edit->toPlainText().simplified().remove(' ').toStdString())};
    if (!signature) {
        showError(tr("The collateral signature is not valid base64."));
        return;
    }
    if (!skip_confirmation && !confirmBroadcast()) return;
    m_unlock = std::make_unique<UnlockHolder>(*m_walletModel);
    if (!m_unlock->ctx.isValid()) {
        m_unlock.reset();
        return;
    }
    m_stage = Stage::Submit;
    setBusy(true, tr("Submitting…"));
    if (!m_runner->submitMasternodeRegistration(
            m_prepared_tx, *signature, [this](auto result) { finishSubmission(std::move(result)); })) {
        m_stage = Stage::None;
        m_unlock.reset();
        setBusy(false);
    }
}

void RegisterMasternodeWizard::finishSubmission(MasternodeOperationRunner::SubmissionResult result)
{
    const Stage completed_stage{m_stage};
    m_stage = Stage::None;
    if (!m_destroying) setBusy(false);

    if (const auto* error{std::get_if<interfaces::ProviderTxError>(&result)}) {
        m_unlock.reset();
        if (!m_destroying) {
            QMessageBox::critical(this, tr("Registration failed"), MasternodeOperationRunner::errorText(*error));
        }
        return;
    }

    const auto& submission{std::get<interfaces::ProviderTxSubmission>(result)};
    if (!submission.tx) {
        if (completed_stage == Stage::Submit && submission.submitted) {
            // A broadcast submit owns the registered collateral lock even if
            // its malformed result omitted the transaction object.
            m_prepared_collateral_lock_acquired = false;
        }
        m_unlock.reset();
        if (!m_destroying) showError(tr("The registration completed without returning a transaction."));
        return;
    }
    if (!submission.submitted) {
        m_unlock.reset();
        if (!m_destroying) showError(tr("The registration transaction was created but was not sent to the network."));
        return;
    }
    if (completed_stage == Stage::Submit) m_prepared_collateral_lock_acquired = false;
    m_unlock.reset();
    completeRegistration(submission.tx);
}

void RegisterMasternodeWizard::finishPrepare(
    interfaces::ProviderTxResult<interfaces::PreparedProviderRegistration> result)
{
    m_stage = Stage::None;
    m_unlock.reset();
    if (!m_destroying) setBusy(false);

    if (const auto* error{std::get_if<interfaces::ProviderTxError>(&result)}) {
        if (!m_destroying) {
            QMessageBox::critical(this, tr("Registration failed"), MasternodeOperationRunner::errorText(*error));
        }
        return;
    }

    auto prepared{std::get<interfaces::PreparedProviderRegistration>(std::move(result))};
    if (!prepared.tx) {
        if (prepared.collateral_lock_acquired && m_walletModel != nullptr) {
            m_walletModel->wallet().unlockCoin(m_prepared_collateral_outpoint);
        }
        if (!m_destroying) showError(tr("The prepare operation completed without returning a transaction."));
        return;
    }
    m_prepared_tx = std::move(prepared.tx);
    m_prepared_collateral_lock_acquired = prepared.collateral_lock_acquired;
    m_sign_address_label->setText(
        tr("Collateral address holding the key: %1")
            .arg(QString::fromStdString(EncodeDestination(prepared.collateral_address))));
    m_sign_message->setPlainText(QString::fromStdString(prepared.sign_message));
    if (!m_destroying) goToPage(PageSign);
}

void RegisterMasternodeWizard::populateResult(const QString& pro_tx_hash)
{
    m_registered = true;
    m_result_label->setText(tr("The registration transaction was sent to the network."));
    m_result_hash->setText(pro_tx_hash);
    m_result_tx_note->setText(
        isExternalCollateral() ?
            tr("It is recorded in this wallet and appears under Transactions. The external collateral remains "
               "at the outpoint you registered and is not controlled by this wallet.") :
            tr("It is recorded in this wallet and appears under Transactions. The collateral stays in the wallet "
               "but is locked while the masternode is registered."));

    // The card already carries the "Next steps" heading. Keep numbering in
    // markup so conditional steps never force translators to renumber strings.
    QStringList steps;
    if (m_operator_widget->hasGeneratedSecret()) {
        steps << tr(
            "Configure your masternode server with the operator secret key you saved, then restart dashd there.");
    } else {
        steps << tr(
            "Configure your masternode server with the BLS secret key matching the operator public key you provided.");
    }
    const bool has_service{!MasternodeWidgetUtil::tokenizeEndpointList(m_service_edit->text()).isEmpty()};
    if (has_service) {
        steps << tr("Make sure every registered service address and port is reachable from the internet.");
    } else {
        steps << tr("Send an Update Service transaction after the node's service endpoints are ready; it stays "
                    "inactive until then.");
    }
    if (isEvo()) {
        steps << tr("Provision the Dash Platform services (Tenderdash and Drive) with the Platform node key matching "
                    "the node ID you registered.");
        steps << tr("The EvoNode becomes active once the registration is confirmed and the first update is signed by "
                    "the operator.");
    } else {
        steps << tr("The masternode becomes active once the registration is confirmed on the network.");
    }
    QString html{"<ol>"};
    for (const QString& step : steps)
        html += QString("<li>%1</li>").arg(step.toHtmlEscaped());
    html += "</ol>";
    m_next_steps->setText(html);
}

void RegisterMasternodeWizard::reject()
{
    if (m_busy) return;
    if (m_registered) {
        QDialog::accept();
        return;
    }
    if (m_prepared_tx) {
        if (QMessageBox::warning(this, tr("Discard prepared registration?"),
                                 tr("The prepared registration will be discarded. Any collateral lock acquired "
                                    "by this wizard will be released."),
                                 QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes) {
            return;
        }
        if (m_prepared_collateral_lock_acquired && m_walletModel != nullptr) {
            m_walletModel->wallet().unlockCoin(m_prepared_collateral_outpoint);
            m_prepared_collateral_lock_acquired = false;
        }
        m_prepared_tx.reset();
    }
    QDialog::reject();
}
