// Copyright (c) 2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/test/masternodemaintenancetests.h>

#include <qt/test/masternodetestutil.h>

#include <bls/bls.h>
#include <chainparams.h>
#include <interfaces/node.h>
#include <key_io.h>
#include <netaddress.h>
#include <netbase.h>
#include <core_io.h>
#include <evo/providertx.h>
#include <evo/specialtx.h>
#include <interfaces/wallet.h>
#include <node/context.h>
#include <qt/bitcoinamountfield.h>
#include <qt/clientmodel.h>
#include <qt/masternodedialogs.h>
#include <qt/masternodemodel.h>
#include <qt/masternodewidgets.h>
#include <qt/mnsharesession.h>
#include <qt/optionsmodel.h>
#include <qt/qvalidatedlineedit.h>
#include <qt/sharedmndialogs.h>
#include <qt/sharedmnwidgets.h>
#include <qt/walletmodel.h>
#include <script/descriptor.h>
#include <script/standard.h>
#include <test/util/setup_common.h>
#include <tinyformat.h>
#include <univalue.h>
#include <util/system.h>
#include <util/translation.h>
#include <validation.h>
#include <wallet/context.h>
#include <wallet/wallet.h>

#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QFocusEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalSpy>
#include <QSpinBox>
#include <QTabWidget>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>

#include <limits>
#include <memory>
#include <string>
#include <utility>

using wallet::WalletContext;
using MasternodeTestUtil::MakeCoinbaseWallet;
using MasternodeTestUtil::MakeTestWallet;
using MasternodeTestUtil::WalletGuard;

namespace {

class TestMnEntry final : public interfaces::MnEntry
{
public:
    explicit TestMnEntry(MnType type = MnType::Evo, bool legacy_operator_display = false, uint16_t operator_reward = 0) :
        interfaces::MnEntry(CDeterministicMNCPtr{}),
        m_type{type},
        m_owner{NewKeyID()},
        m_voting{NewKeyID()},
        m_payout{GetScriptForDestination(PKHash{NewKeyID()})},
        m_operator_payout{operator_reward ? GetScriptForDestination(PKHash{NewKeyID()}) : CScript{}},
        m_operator_reward{operator_reward},
        m_core_endpoint{strprintf("127.0.0.1:%d", Params().GetDefaultPort())}
    {
        m_hash.SetHex("01");
        m_primary_service = LookupNumeric("127.0.0.1", 19999);
        setOperatorKey(legacy_operator_display);
    }

    bool isBanned() const override { return m_banned; }
    CService getNetInfoPrimary() const override { return m_primary_service; }
    std::vector<CService> getPlatformHTTPSAddrs() const override { return {}; }
    MnType getType() const override { return m_type; }
    UniValue toJson() const override
    {
        UniValue result(UniValue::VOBJ);
        result.pushKV("collateralHash", uint256::ONE.ToString());
        result.pushKV("collateralIndex", 0);
        UniValue state(UniValue::VOBJ);
        state.pushKV("version", m_operator_version);
        state.pushKV("consecutivePayments", 1);
        state.pushKV("PoSeBanHeight", -1);
        state.pushKV("PoSeRevivedHeight", -1);
        state.pushKV("pubKeyOperator", m_operator_display);
        state.pushKV("platformNodeID", m_platform_node_id);
        if (!m_core_endpoint.empty() || !m_platform_p2p.empty() || !m_platform_https.empty()) {
            UniValue addresses(UniValue::VOBJ);
            addresses.pushKV("core_p2p", StringArray(m_core_endpoint));
            addresses.pushKV("platform_p2p", StringArray(m_platform_p2p));
            addresses.pushKV("platform_https", StringArray(m_platform_https));
            state.pushKV("addresses", addresses);
        }
        result.pushKV("state", state);
        return result;
    }
    const CKeyID& getKeyIdOwner() const override { return m_owner; }
    bool isShared() const override { return !m_shares.empty(); }
    std::vector<interfaces::MnShare> getShares() const override { return m_shares; }
    const uint32_t& getEarlyPeriodBlocks() const override { return m_early_period_blocks; }
    const CAmount& getEarlyPenalty() const override { return m_early_penalty; }

    //! Turn this entry into a shared masternode with `amounts` as its share
    //! table; every share gets a distinct owner key and refund script
    void makeShared(const std::vector<CAmount>& amounts, CAmount early_penalty, uint32_t early_period_blocks)
    {
        m_hash.SetHex("2f2f2f2f2f2f2f2f2f2f2f2f2f2f2f2f2f2f2f2f2f2f2f2f2f2f2f2f2f2f2f2f");
        m_early_penalty = early_penalty;
        m_early_period_blocks = early_period_blocks;
        m_shares.clear();
        for (const CAmount amount : amounts) {
            interfaces::MnShare share;
            share.amount = amount;
            share.keyIDOwner = NewKeyID();
            share.scriptRefund = GetScriptForDestination(PKHash{NewKeyID()});
            m_shares.push_back(share);
        }
    }
    const CKeyID& getKeyIdVoting() const override { return m_voting; }
    const COutPoint& getCollateralOutpoint() const override { return m_collateral; }
    const CScript& getScriptPayout() const override { return m_payout; }
    std::vector<CScript> getScriptPayouts() const override { return {m_payout}; }
    const CScript& getScriptOperatorPayout() const override { return m_operator_payout; }
    const int32_t& getLastPaidHeight() const override { return m_last_paid; }
    const int32_t& getPoSePenalty() const override { return m_penalty; }
    const int32_t& getRegisteredHeight() const override { return m_registered; }
    const uint16_t& getOperatorReward() const override { return m_operator_reward; }
    const uint256& getProTxHash() const override { return m_hash; }
    const CBLSSecretKey& operatorSecret() const { return m_operator_secret; }

    void mutate()
    {
        m_voting = NewKeyID();
        m_payout = GetScriptForDestination(PKHash{NewKeyID()});
        m_operator_payout = GetScriptForDestination(PKHash{NewKeyID()});
        m_operator_reward = 500;
        m_core_endpoint = "127.0.0.2:20001";
        m_platform_p2p = "127.0.0.2:26657";
        m_platform_https = "api.example.org:8443";
        m_platform_node_id = "2222222222222222222222222222222222222222";
        setOperatorKey(/*legacy_display=*/false);
    }

    //! Point one share's owner key at a key the test wallet holds, so the
    //! dialogs treat that share as this wallet's own
    void setShareOwner(size_t index, const CKeyID& key_id) { m_shares.at(index).keyIDOwner = key_id; }

    void clearEndpoints()
    {
        m_primary_service = {};
        m_core_endpoint.clear();
        m_platform_p2p.clear();
        m_platform_https.clear();
    }

    void setPlatformEndpoints(std::string p2p, std::string https)
    {
        m_platform_p2p = std::move(p2p);
        m_platform_https = std::move(https);
    }

private:
    static CKeyID NewKeyID()
    {
        static unsigned int counter{1};
        CKeyID key_id;
        key_id.SetHex(strprintf("%040u", counter++));
        return key_id;
    }

    static UniValue StringArray(const std::string& value)
    {
        UniValue result(UniValue::VARR);
        if (!value.empty()) result.push_back(value);
        return result;
    }

    void setOperatorKey(bool legacy_display)
    {
        CBLSSecretKey key;
        key.MakeNewKey();
        m_operator_secret = key;
        const auto public_key{key.GetPublicKey()};
        m_operator_display = public_key.ToString(legacy_display);
        m_operator_version = legacy_display ? ProTxVersion::LegacyBLS : ProTxVersion::BasicBLS;
    }

    bool m_banned{false};
    MnType m_type;
    CKeyID m_owner;
    CKeyID m_voting;
    COutPoint m_collateral{uint256::ONE, 0};
    CScript m_payout;
    CScript m_operator_payout;
    int32_t m_last_paid{10};
    int32_t m_penalty{0};
    int32_t m_registered{1};
    uint16_t m_operator_reward{0};
    std::vector<interfaces::MnShare> m_shares;
    CAmount m_early_penalty{0};
    uint32_t m_early_period_blocks{0};
    uint256 m_hash;
    uint16_t m_operator_version{ProTxVersion::BasicBLS};
    CBLSSecretKey m_operator_secret;
    CService m_primary_service;
    std::string m_operator_display;
    std::string m_core_endpoint;
    std::string m_platform_p2p{"127.0.0.1:26656"};
    std::string m_platform_https{"api.example.com:443"};
    std::string m_platform_node_id{"1111111111111111111111111111111111111111"};
};

} // anonymous namespace

void MasternodeMaintenanceTests::automaticFeeSource()
{
    FeeSourcePicker picker;
    picker.setAutomaticOption("Automatic (recommended)");
    QCOMPARE(picker.count(), 1);
    QCOMPARE(picker.currentText(), QString("Automatic (recommended)"));
    QVERIFY(picker.selectedAddress().isEmpty());

    // A wallet funded at an address that is neither a payout nor a share
    // reward address of the masternode, like a hosting operator's
    TestChain100Setup test;
    m_node.setContext(&test.m_node);
    WalletContext& context{*m_node.walletLoader().context()};
    const auto wallet{MakeTestWallet(m_node, context, "operator")};
    QVERIFY(wallet != nullptr);
    WalletGuard guard{context, wallet};
    CTxDestination funded;
    {
        LOCK(wallet->cs_wallet);
        const auto destination{wallet->GetNewDestination("")};
        QVERIFY(destination);
        funded = *destination;
        const CBlockIndex* tip{
            WITH_LOCK(test.m_node.chainman->GetMutex(), return test.m_node.chainman->ActiveChain().Tip())};
        wallet->SetLastBlockProcessed(tip->nHeight, tip->GetBlockHash());
        CMutableTransaction funding;
        funding.vin.emplace_back(GetRandHash(), 0);
        funding.vout.emplace_back(10 * COIN, GetScriptForDestination(funded));
        QVERIFY(wallet->AddToWallet(MakeTransactionRef(funding),
                                    wallet::TxStateConfirmed{tip->GetBlockHash(), tip->nHeight, /*index=*/0}) != nullptr);
    }
    MasternodeTestUtil::GuiModels models{m_node};
    QVERIFY2(models.ok, qPrintable(QString::fromStdString(models.error.translated)));
    WalletModel wallet_model(interfaces::MakeWallet(context, wallet), models.client);
    const interfaces::ProviderTxCapabilities capabilities{ProTxVersion::BasicBLS, false};

    // A regular masternode's dialogs leave the fee source to Core
    auto regular{std::make_shared<TestMnEntry>(MnType::Regular)};
    MasternodeEntry regular_entry{regular, "collateral", 50};
    UpdateServiceDialog regular_service(m_node, &wallet_model, regular_entry, capabilities, /*parent=*/nullptr);
    QCOMPARE(regular_service.m_fee_source->currentText(), QString("Automatic (recommended)"));
    QString error;
    const auto regular_update{regular_service.buildRequest(regular->operatorSecret(), error)};
    QVERIFY2(regular_update.has_value(), qPrintable(error));
    QVERIFY(!regular_update->fee_source.has_value());
    RevokeDialog regular_revoke(m_node, &wallet_model, regular_entry, /*parent=*/nullptr);
    QCOMPARE(regular_revoke.m_fee_source->currentText(), QString("Automatic (recommended)"));
    const auto regular_revocation{regular_revoke.buildRequest(regular->operatorSecret(), error)};
    QVERIFY2(regular_revocation.has_value(), qPrintable(error));
    QVERIFY(!regular_revocation->fee_source.has_value());

    // Core's automatic choice for a shared masternode is the first share's
    // reward address, which this wallet does not own: pay from this wallet
    auto shared{std::make_shared<TestMnEntry>(MnType::Regular)};
    shared->makeShared({400 * COIN, 300 * COIN, 300 * COIN}, 5 * COIN, /*early_period_blocks=*/1000);
    MasternodeEntry shared_entry{shared, "collateral", 50};
    UpdateServiceDialog shared_service(m_node, &wallet_model, shared_entry, capabilities, /*parent=*/nullptr);
    QCOMPARE(shared_service.m_fee_source->findText("Automatic (recommended)"), -1);
    const auto shared_update{shared_service.buildRequest(shared->operatorSecret(), error)};
    QVERIFY2(shared_update.has_value(), qPrintable(error));
    QVERIFY(shared_update->fee_source == std::optional<CTxDestination>{funded});
    RevokeDialog shared_revoke(m_node, &wallet_model, shared_entry, /*parent=*/nullptr);
    QCOMPARE(shared_revoke.m_fee_source->findText("Automatic (recommended)"), -1);
    const auto shared_revocation{shared_revoke.buildRequest(shared->operatorSecret(), error)};
    QVERIFY2(shared_revocation.has_value(), qPrintable(error));
    QVERIFY(shared_revocation->fee_source == std::optional<CTxDestination>{funded});
}

void MasternodeMaintenanceTests::operatorSecretValidation()
{
    OperatorSecretWidget widget;
    QVERIFY(!widget.isValid());

    CBLSSecretKey key;
    key.MakeNewKey();
    auto* edit{widget.findChild<QLineEdit*>()};
    QVERIFY(edit != nullptr);
    edit->setText(QString::fromStdString(key.ToString(/*specificLegacyScheme=*/false)));
    QVERIFY(widget.isValid());
    const auto parsed{widget.takeSecret()};
    QVERIFY(parsed.has_value());
    QCOMPARE(parsed->GetPublicKey().ToByteVector(false), key.GetPublicKey().ToByteVector(false));
    QVERIFY(edit->text().isEmpty());
    QVERIFY(!widget.isValid());

    edit->setText("not-a-secret");
    QVERIFY(!widget.takeSecret().has_value());
    QVERIFY(edit->text().isEmpty());
}

void MasternodeMaintenanceTests::operatorPublicKeyEncoding()
{
    auto source{std::make_shared<TestMnEntry>(MnType::Evo, /*legacy_operator_display=*/true)};
    MasternodeEntry entry{source, "collateral", 50};

    const auto raw{entry.operatorPubKeyBytes()};
    CBLSPublicKey public_key;
    public_key.SetBytes(raw, /*specificLegacyScheme=*/false);
    QVERIFY(public_key.IsValid());
    QCOMPARE(entry.operatorPubKey(/*legacy_scheme=*/false),
             QString::fromStdString(public_key.ToString(/*specificLegacyScheme=*/false)));
    QCOMPARE(entry.operatorPubKey(/*legacy_scheme=*/true),
             QString::fromStdString(public_key.ToString(/*specificLegacyScheme=*/true)));
    QVERIFY(entry.operatorPubKey(/*legacy_scheme=*/false) != entry.operatorPubKey(/*legacy_scheme=*/true));
}

void MasternodeMaintenanceTests::actionRoleGating()
{
    using MasternodeMaintenance::actionAvailability;

    const auto no_wallet{actionAvailability(false, false)};
    QVERIFY(!no_wallet.update_service);
    QVERIFY(!no_wallet.update_registrar);
    QVERIFY(!no_wallet.revoke);

    const auto operator_only{actionAvailability(true, false)};
    QVERIFY(operator_only.update_service);
    QVERIFY(!operator_only.update_registrar);
    QVERIFY(operator_only.revoke);

    const auto owner{actionAvailability(true, true)};
    QVERIFY(owner.update_service);
    QVERIFY(owner.update_registrar);
    QVERIFY(owner.revoke);
}

void MasternodeMaintenanceTests::reconcileMutableFields()
{
    auto initial{std::make_shared<TestMnEntry>()};
    MasternodeEntryList first;
    first.push_back(std::make_shared<MasternodeEntry>(initial, "collateral", 50));

    MasternodeModel model;
    model.reconcile(std::move(first));
    const MasternodeEntry* entry{model.getEntryAt(model.index(0, 0))};
    QVERIFY(entry != nullptr);
    const QString initial_operator{entry->operatorPubKey()};
    const QString initial_voting{entry->votingAddress()};
    const QString initial_payout{entry->payoutAddress()};
    QCOMPARE(entry->coreP2PAddresses(), QString("127.0.0.1:%1").arg(Params().GetDefaultPort()));
    QCOMPARE(entry->platformNodeID(), QString("1111111111111111111111111111111111111111"));

    auto updated{std::make_shared<TestMnEntry>()};
    updated->mutate();
    MasternodeEntryList second;
    second.push_back(std::make_shared<MasternodeEntry>(updated, "collateral", 50));
    model.reconcile(std::move(second));

    entry = model.getEntryAt(model.index(0, 0));
    QVERIFY(entry != nullptr);
    QCOMPARE(entry->coreP2PAddresses(), QString("127.0.0.2:20001"));
    QCOMPARE(entry->platformP2PAddresses(), QString("127.0.0.2:26657"));
    QCOMPARE(entry->platformHTTPSAddresses(), QString("api.example.org:8443"));
    QCOMPARE(entry->platformNodeID(), QString("2222222222222222222222222222222222222222"));
    QVERIFY(entry->operatorPubKey() != initial_operator);
    QVERIFY(entry->votingAddress() != initial_voting);
    QVERIFY(entry->payoutAddress() != initial_payout);
    QVERIFY(!entry->operatorPayoutAddress().isEmpty());
    QCOMPARE(entry->operatorRewardPct(), uint16_t{500});

    auto inactive{std::make_shared<TestMnEntry>()};
    inactive->clearEndpoints();
    const MasternodeEntry inactive_entry{inactive, "collateral", 50};
    QVERIFY(inactive_entry.coreP2PAddresses().isEmpty());
    QVERIFY(inactive_entry.platformP2PAddresses().isEmpty());
    QVERIFY(inactive_entry.platformHTTPSAddresses().isEmpty());
    const QString inactive_html{inactive_entry.toHtml()};
    QVERIFY(!inactive_html.contains("Network Addresses"));
    QVERIFY(!inactive_html.contains("Platform P2P Addresses"));
    QVERIFY(!inactive_html.contains("Platform HTTPS Addresses"));
}

void MasternodeMaintenanceTests::serviceRequestConstruction()
{
    TestChain100Setup test;
    m_node.setContext(&test.m_node);

    const auto check_request = [this](MnType type, interfaces::ProviderTxCapabilities capabilities) {
        auto source{std::make_shared<TestMnEntry>(type, capabilities.version == ProTxVersion::LegacyBLS,
                                                  /*operator_reward=*/500)};
        MasternodeEntry entry{source, "collateral", 50};
        UpdateServiceDialog dialog(m_node, /*wallet_model=*/nullptr, entry, capabilities, /*parent=*/nullptr);
        QString error;
        const auto request{dialog.buildRequest(source->operatorSecret(), error)};
        QVERIFY2(request.has_value(), qPrintable(error));
        QCOMPARE(request->type, type);
        QCOMPARE(request->pro_tx_hash, source->getProTxHash());
        QCOMPARE(request->operator_key.GetPublicKey().ToByteVector(false),
                 source->operatorSecret().GetPublicKey().ToByteVector(false));
        QCOMPARE(request->net_info.core_p2p, std::vector<std::string>{entry.coreP2PAddresses().toStdString()});
        QVERIFY(request->operator_payout.has_value());
        QVERIFY(!request->fee_source.has_value());
        QVERIFY(request->submit);

        if (type == MnType::Regular) {
            QVERIFY(std::holds_alternative<std::monostate>(request->net_info.platform_p2p));
            QVERIFY(std::holds_alternative<std::monostate>(request->net_info.platform_https));
            QVERIFY(!request->platform_node_id.has_value());
        } else if (capabilities.extended_addresses) {
            QCOMPARE(std::get<std::vector<std::string>>(request->net_info.platform_p2p),
                     std::vector<std::string>{"127.0.0.1:26656"});
            QCOMPARE(std::get<std::vector<std::string>>(request->net_info.platform_https),
                     std::vector<std::string>{"api.example.com:443"});
            QVERIFY(request->platform_node_id.has_value());
        } else {
            QCOMPARE(std::get<uint16_t>(request->net_info.platform_p2p), uint16_t{26656});
            QCOMPARE(std::get<uint16_t>(request->net_info.platform_https), uint16_t{443});
            QVERIFY(request->platform_node_id.has_value());
        }

        if (type == MnType::Evo) {
            auto* const secret_edit{dialog.m_operator_key->findChild<QLineEdit*>()};
            QVERIFY(secret_edit != nullptr);
            secret_edit->setText(QString::fromStdString(source->operatorSecret().ToString(/*specificLegacyScheme=*/false)));
            QVERIFY(dialog.m_valid);

            dialog.m_platform_node_id_edit->setText(QString(40, QLatin1Char('0')));
            QVERIFY(!dialog.m_valid);
            error.clear();
            QVERIFY(!dialog.buildRequest(source->operatorSecret(), error).has_value());
            QVERIFY(error.contains("Platform node ID"));
        }
    };

    check_request(MnType::Regular, {ProTxVersion::BasicBLS, false});
    check_request(MnType::Evo, {ProTxVersion::BasicBLS, false});
    check_request(MnType::Evo, {ProTxVersion::ExtAddr, true});

    auto malformed_source{std::make_shared<TestMnEntry>(MnType::Evo)};
    malformed_source->setPlatformEndpoints("missing-port", "api.example.com:not-a-port");
    MasternodeEntry malformed_entry{malformed_source, "collateral", 50};
    UpdateServiceDialog fallback_dialog(m_node, /*wallet_model=*/nullptr, malformed_entry,
                                        interfaces::ProviderTxCapabilities{ProTxVersion::BasicBLS, false},
                                        /*parent=*/nullptr);
    QCOMPARE(fallback_dialog.m_platform_p2p_port_edit->value(), Params().GetDefaultPlatformP2PPort());
    QCOMPARE(fallback_dialog.m_platform_https_port_edit->value(), Params().GetDefaultPlatformHTTPPort());

    QString error;
    const auto fallback_request{fallback_dialog.buildRequest(malformed_source->operatorSecret(), error)};
    QVERIFY2(fallback_request.has_value(), qPrintable(error));
    QCOMPARE(std::get<uint16_t>(fallback_request->net_info.platform_p2p), Params().GetDefaultPlatformP2PPort());
    QCOMPARE(std::get<uint16_t>(fallback_request->net_info.platform_https), Params().GetDefaultPlatformHTTPPort());
}

void MasternodeMaintenanceTests::registrarRequestConstruction()
{
    TestChain100Setup test;
    m_node.setContext(&test.m_node);

    for (const interfaces::ProviderTxCapabilities capabilities :
         {interfaces::ProviderTxCapabilities{ProTxVersion::LegacyBLS, false},
          interfaces::ProviderTxCapabilities{ProTxVersion::ExtAddr, true}}) {
        auto source{std::make_shared<TestMnEntry>(MnType::Regular, capabilities.version == ProTxVersion::LegacyBLS)};
        MasternodeEntry entry{source, "collateral", 50};
        UpdateRegistrarDialog dialog(m_node, /*wallet_model=*/nullptr, entry, capabilities, /*parent=*/nullptr);
        QString error;

        const auto unchanged{dialog.buildRequest(error)};
        QVERIFY2(unchanged.has_value(), qPrintable(error));
        QVERIFY(!unchanged->operator_key.has_value());
        QVERIFY(!unchanged->voting_key.has_value());
        QVERIFY(!unchanged->payouts.has_value());
        QVERIFY(unchanged->submit);

        dialog.m_voting_edit->setText(QString::fromStdString(EncodeDestination(PKHash{uint160{}})));
        QVERIFY(!dialog.m_valid);
        error.clear();
        QVERIFY(!dialog.buildRequest(error).has_value());
        QVERIFY(error.contains("voting address"));

        CBLSSecretKey replacement;
        replacement.MakeNewKey();
        dialog.m_operator_pubkey_edit->setText(
            QString::fromStdString(replacement.GetPublicKey().ToString(capabilities.version == ProTxVersion::LegacyBLS)));
        CKeyID voting;
        voting.SetHex("42");
        dialog.m_voting_edit->setText(QString::fromStdString(EncodeDestination(PKHash{voting})));
        CKeyID payout;
        payout.SetHex("43");
        dialog.m_payout_edit->setText(QString::fromStdString(EncodeDestination(PKHash{payout})));

        error.clear();
        const auto changed{dialog.buildRequest(error)};
        QVERIFY2(changed.has_value(), qPrintable(error));
        QVERIFY(changed->operator_key.has_value());
        QCOMPARE(changed->operator_key->ToByteVector(false), replacement.GetPublicKey().ToByteVector(false));
        QCOMPARE(changed->voting_key, std::optional<CKeyID>{voting});
        QVERIFY(changed->payouts.has_value());
        QCOMPARE(changed->payouts->size(), size_t{1});
        QCOMPARE(changed->payouts->front().destination, CTxDestination{PKHash{payout}});
        QCOMPARE(changed->payouts->front().reward, interfaces::ProviderPayout::MAX_REWARD);
        QCOMPARE(changed->uses_extended_payouts, capabilities.version >= ProTxVersion::ExtAddr);
        QCOMPARE(changed->use_legacy_bls_scheme, capabilities.version == ProTxVersion::LegacyBLS);
    }
}

void MasternodeMaintenanceTests::revokeRequestConstruction()
{
    TestChain100Setup test;
    m_node.setContext(&test.m_node);
    auto source{std::make_shared<TestMnEntry>(MnType::Regular)};
    MasternodeEntry entry{source, "collateral", 50};
    RevokeDialog dialog(m_node, /*wallet_model=*/nullptr, entry, /*parent=*/nullptr);

    for (int reason = 0; reason <= 3; ++reason) {
        dialog.m_reason_combo->setCurrentIndex(reason);
        QString error;
        const auto request{dialog.buildRequest(source->operatorSecret(), error)};
        QVERIFY2(request.has_value(), qPrintable(error));
        QCOMPARE(request->pro_tx_hash, source->getProTxHash());
        QCOMPARE(request->reason, static_cast<uint16_t>(reason));
        QVERIFY(!request->fee_source.has_value());
        QVERIFY(request->submit);
    }
}

void MasternodeMaintenanceTests::dialogFieldGeometry()
{
#if defined(Q_OS_MACOS)
    if (QApplication::platformName() == "minimal") {
        QWARN("Skipping dialogFieldGeometry on macOS with the minimal platform due to QTBUG-49686");
        return;
    }
#endif
    TestChain100Setup test;
    m_node.setContext(&test.m_node);
    auto source{std::make_shared<TestMnEntry>(MnType::Regular)};
    MasternodeEntry entry{source, "collateral", 50};

    RevokeDialog revoke(m_node, /*wallet_model=*/nullptr, entry, /*parent=*/nullptr);
    revoke.show();
    QApplication::processEvents();
    const auto field_rect = [](QWidget* field, QWidget* dialog) {
        return QRect{field->mapTo(dialog, QPoint{}), field->size()};
    };
    const QRect reason_rect{field_rect(revoke.m_reason_combo, &revoke)};
    const QRect operator_rect{field_rect(revoke.m_operator_key, &revoke)};
    const QRect fee_rect{field_rect(revoke.m_fee_source, &revoke)};
    const auto fields_align = [](const QRect& expected, const QRect& actual) {
        // Native controls can extend a few pixels outside their layout item.
        return qAbs(expected.left() - actual.left()) <= 12 && qAbs(expected.right() - actual.right()) <= 12;
    };
    QVERIFY(reason_rect.width() > 400);
    QVERIFY(fields_align(reason_rect, operator_rect));
    QVERIFY(fields_align(reason_rect, fee_rect));

    UpdateRegistrarDialog registrar(m_node, /*wallet_model=*/nullptr, entry,
                                    interfaces::ProviderTxCapabilities{ProTxVersion::BasicBLS, false},
                                    /*parent=*/nullptr);
    registrar.show();
    QApplication::processEvents();
    const QRect public_key_rect{field_rect(registrar.m_operator_pubkey_edit, &registrar)};
    for (QWidget* field : {static_cast<QWidget*>(registrar.m_voting_edit), static_cast<QWidget*>(registrar.m_payout_edit),
                           static_cast<QWidget*>(registrar.m_fee_source)}) {
        const QRect rect{field_rect(field, &registrar)};
        QVERIFY(fields_align(public_key_rect, rect));
    }
}

void MasternodeMaintenanceTests::dialogLifecycleAndSubmissionStates()
{
#if defined(Q_OS_MACOS)
    if (QApplication::platformName() == "minimal") {
        QWARN("Skipping dialogLifecycleAndSubmissionStates on macOS with the minimal platform due to QTBUG-49686");
        return;
    }
#endif
    TestChain100Setup test;
    m_node.setContext(&test.m_node);
    auto source{std::make_shared<TestMnEntry>(MnType::Regular)};
    MasternodeEntry entry{source, "collateral", 50};
    UpdateRegistrarDialog dialog(m_node, /*wallet_model=*/nullptr, entry,
                                 interfaces::ProviderTxCapabilities{ProTxVersion::BasicBLS, false},
                                 /*parent=*/nullptr);
    QSignalSpy rejected_spy(&dialog, &QDialog::rejected);
    dialog.show();

    dialog.setBusy(true);
    dialog.reject();
    QCOMPARE(rejected_spy.count(), 0);
    QVERIFY(dialog.isVisible());
    dialog.setBusy(false);

    dialog.finishSubmission(interfaces::ProviderTxSubmission{MakeTransactionRef(CMutableTransaction{}), false});
    QVERIFY(dialog.m_status_label->isVisible());
    QVERIFY(dialog.m_status_label->text().contains("not sent", Qt::CaseInsensitive));
    QCOMPARE(rejected_spy.count(), 0);

    dialog.finishSubmission(interfaces::ProviderTxError{interfaces::ProviderTxErrorCode::INTERNAL_ERROR,
                                                        Untranslated("maintenance failed"),
                                                        {},
                                                        std::nullopt});
    QVERIFY(dialog.m_status_label->text().contains("maintenance failed", Qt::CaseInsensitive));

    // shutdown() can synchronously deliver a completed operation from the
    // destructor. It must release busy/unlock state without opening modal UI
    // or changing the dialog result during teardown.
    bool message_box_seen{false};
    QTimer::singleShot(0, [&message_box_seen] {
        for (QWidget* const widget : QApplication::topLevelWidgets()) {
            auto* const message_box{qobject_cast<QMessageBox*>(widget)};
            if (message_box == nullptr) continue;
            message_box_seen = true;
            message_box->accept();
        }
    });
    QSignalSpy accepted_spy(&dialog, &QDialog::accepted);
    dialog.m_destroying = true;
    dialog.setBusy(true);
    dialog.finishSubmission(interfaces::ProviderTxSubmission{MakeTransactionRef(CMutableTransaction{}), /*submitted=*/true});
    QApplication::processEvents();
    QVERIFY(!message_box_seen);
    QCOMPARE(accepted_spy.count(), 0);
    QVERIFY(!dialog.m_busy);
    QVERIFY(QApplication::overrideCursor() == nullptr);
}

namespace {
//! A shared masternode entry with `amounts` as its share table
std::shared_ptr<TestMnEntry> MakeSharedSource(const std::vector<CAmount>& amounts, CAmount early_penalty,
                                              uint32_t early_period_blocks)
{
    auto source{std::make_shared<TestMnEntry>(MnType::Regular)};
    source->makeShared(amounts, early_penalty, early_period_blocks);
    return source;
}

//! A "dash-shared-mn-sigs" registrar envelope carrying an unsigned prepared key
//! rotation that spends `fee_input`, exactly as the preparing wallet's node
//! would have built it
QString MakeRegistrarEnvelope(const uint256& pro_tx_hash, const COutPoint& fee_input)
{
    CMutableTransaction tx;
    tx.nVersion = 3;
    tx.nType = TRANSACTION_PROVIDER_UPDATE_SHARED_REGISTRAR;
    tx.vin.emplace_back(fee_input);
    CProUpSharedRegTx payload;
    payload.proTxHash = pro_tx_hash;
    CBLSSecretKey operator_secret;
    operator_secret.MakeNewKey();
    payload.pubKeyOperator.Set(operator_secret.GetPublicKey(), /*specificLegacyScheme=*/false);
    SetTxPayload(tx, payload);

    UniValue json(UniValue::VOBJ);
    json.pushKV("type", "dash-shared-mn-sigs");
    json.pushKV("version", 1);
    json.pushKV("network", Params().NetworkIDString());
    json.pushKV("kind", "registrar");
    json.pushKV("proTxHash", pro_tx_hash.ToString());
    json.pushKV("tx", EncodeHexTx(CTransaction(tx)));
    json.pushKV("signatures", UniValue{UniValue::VARR});
    shared_mn::AppendFingerprint(json);
    return QString::fromStdString(json.write(/*prettyIndent=*/2));
}

//! Duffs; a plausible dissolution fee, well inside CProDisTx::MAX_FEE
constexpr CAmount TEST_DISSOLVE_FEE{100000};

//! The unsigned ProDisTx "protx shared_dissolve_prepare" builds for `shares`:
//! every non-actor share is paid its principal at its own refund script in
//! share order, and the actor takes the fee out of its own
CMutableTransaction MakeDissolveTemplate(const uint256& pro_tx_hash, uint16_t actor_index,
                                         const std::vector<interfaces::MnShare>& shares, CAmount fee)
{
    CMutableTransaction tx;
    tx.nVersion = 3;
    tx.nType = TRANSACTION_PROVIDER_DISSOLVE;
    tx.vin.emplace_back(COutPoint{uint256::ONE, 0});
    for (size_t i = 0; i < shares.size(); ++i) {
        if (i == actor_index) continue;
        tx.vout.emplace_back(shares[i].amount, shares[i].scriptRefund);
    }
    tx.vout.emplace_back(shares[actor_index].amount - fee, shares[actor_index].scriptRefund);
    CProDisTx payload;
    payload.proTxHash = pro_tx_hash;
    payload.actorIndex = actor_index;
    SetTxPayload(tx, payload);
    return tx;
}

//! A "dash-shared-mn-sigs" envelope carrying `tx`, exactly as one participant
//! would paste it to another
QString MakeDissolveEnvelopeFor(const uint256& pro_tx_hash, const CMutableTransaction& tx)
{
    UniValue json(UniValue::VOBJ);
    json.pushKV("type", "dash-shared-mn-sigs");
    json.pushKV("version", 1);
    json.pushKV("network", Params().NetworkIDString());
    json.pushKV("kind", "dissolve");
    json.pushKV("proTxHash", pro_tx_hash.ToString());
    json.pushKV("tx", EncodeHexTx(CTransaction(tx)));
    json.pushKV("signatures", UniValue{UniValue::VARR});
    shared_mn::AppendFingerprint(json);
    return QString::fromStdString(json.write(/*prettyIndent=*/2));
}

//! The honest dissolution envelope for `shares`, with share 0 paying the fee
QString MakeDissolveEnvelope(const uint256& pro_tx_hash, const std::vector<interfaces::MnShare>& shares)
{
    return MakeDissolveEnvelopeFor(pro_tx_hash,
                                   MakeDissolveTemplate(pro_tx_hash, /*actor_index=*/0, shares, TEST_DISSOLVE_FEE));
}
} // anonymous namespace

void MasternodeMaintenanceTests::updateShareRewardValidation()
{
    TestChain100Setup test;
    m_node.setContext(&test.m_node);
    auto source{MakeSharedSource({400 * COIN, 300 * COIN, 300 * COIN}, 5 * COIN, /*early_period_blocks=*/1000)};
    MasternodeEntry entry{source, "collateral", 50};
    const auto& shares{entry.shares()};

    const QString& voting{entry.votingAddress()};
    const QString owner{QString::fromStdString(EncodeDestination(PKHash(shares.front().keyIDOwner)))};
    CKeyID unrelated;
    unrelated.SetHex("7e");
    const QString fresh{QString::fromStdString(EncodeDestination(PKHash{unrelated}))};

    QVERIFY(!UpdateShareDialog::RewardAddressProblem("", voting, shares).isEmpty());
    QVERIFY(!UpdateShareDialog::RewardAddressProblem("not-an-address", voting, shares).isEmpty());
    QVERIFY(UpdateShareDialog::RewardAddressProblem(voting, voting, shares).contains("voting address"));
    QVERIFY(UpdateShareDialog::RewardAddressProblem(owner, voting, shares).contains("owner address"));
    QVERIFY(UpdateShareDialog::RewardAddressProblem(fresh, voting, shares).isEmpty());

    UpdateShareDialog dialog(m_node, /*wallet_model=*/nullptr, entry, /*parent=*/nullptr);
    QCOMPARE(dialog.m_share_combo->count(), 3);
    QVERIFY(dialog.m_share_combo->itemText(0).startsWith("Share 1 of 3"));
    QVERIFY(dialog.m_share_combo->itemText(2).startsWith("Share 3 of 3"));

    // "Use refund address" is the only way to point a share's rewards back at
    // its refund address: the RPC rejects an empty reward address
    dialog.m_share_combo->setCurrentIndex(1);
    dialog.useRefundAddress();
    CTxDestination refund_dest;
    QVERIFY(ExtractDestination(shares[1].scriptRefund, refund_dest));
    QCOMPARE(dialog.m_reward_edit->text(), QString::fromStdString(EncodeDestination(refund_dest)));
    QVERIFY(UpdateShareDialog::RewardAddressProblem(dialog.m_reward_edit->text(), voting, shares).isEmpty());
}

void MasternodeMaintenanceTests::updateShareUnlocksTheWallet()
{
    TestChain100Setup test;
    m_node.setContext(&test.m_node);
    WalletContext& context{*m_node.walletLoader().context()};
    const auto wallet{MakeCoinbaseWallet(m_node, context, test, "owner", /*encrypt=*/true)};
    QVERIFY(wallet != nullptr);
    WalletGuard guard{context, wallet};

    MasternodeTestUtil::GuiModels models{m_node};
    QVERIFY2(models.ok, qPrintable(QString::fromStdString(models.error.translated)));
    WalletModel wallet_model(interfaces::MakeWallet(context, wallet), models.client);
    QCOMPARE(int(wallet_model.getEncryptionStatus()), int(WalletModel::Locked));

    auto source{MakeSharedSource({400 * COIN, 300 * COIN, 300 * COIN}, 5 * COIN, /*early_period_blocks=*/1000)};
    source->setShareOwner(0, ToKeyID(PKHash(test.coinbaseKey.GetPubKey())));
    MasternodeEntry entry{source, "collateral", 50};

    UpdateShareDialog dialog(m_node, &wallet_model, entry, /*parent=*/nullptr);
    QCOMPARE(dialog.m_share_combo->count(), 1);
    dialog.useNewAddress();
    QVERIFY(!dialog.m_reward_edit->text().isEmpty());

    // "protx shared_update_share" calls EnsureWalletIsUnlocked, so submitting
    // without asking for the passphrase first fails on every encrypted wallet
    QSignalSpy unlock_spy(&wallet_model, &WalletModel::requireUnlock);
    dialog.submit();
    QCOMPARE(unlock_spy.count(), 1);
    // Nothing was sent: the unlock was not granted (no passphrase dialog is
    // answered here), so the dialog says so instead of going busy
    QVERIFY(!dialog.m_busy);
    QVERIFY2(dialog.m_status_label->text().contains("unlocked", Qt::CaseInsensitive),
             qPrintable(dialog.m_status_label->text()));
}

void MasternodeMaintenanceTests::dissolveDialogTabsAndPayouts()
{
    TestChain100Setup test;
    m_node.setContext(&test.m_node);
    const std::vector<CAmount> amounts{400 * COIN, 300 * COIN, 300 * COIN};
    auto source{MakeSharedSource(amounts, 5 * COIN, /*early_period_blocks=*/1000)};
    MasternodeEntry entry{source, "collateral", 50};

    DissolveDialog dialog(m_node, /*wallet_model=*/nullptr, entry, /*current_height=*/100, /*parent=*/nullptr);
    QVERIFY(dialog.m_tabs != nullptr);
    QCOMPARE(dialog.m_tabs->count(), 3);

    dialog.m_now_actor->setCurrentIndex(0);
    dialog.updateNowPreview();
    QCOMPARE(dialog.m_now_table->rowCount(), 3);
    QCOMPARE(dialog.m_now_table->item(0, 0)->text(), QString("Share 1 of 3 (you)"));
    QCOMPARE(dialog.m_now_table->item(1, 0)->text(), QString("Share 2 of 3"));

    const auto preview{MnShareSession::PenaltyPreviewFor(amounts, /*actor_index=*/0, 5 * COIN,
                                                         /*early_period_blocks=*/1000, dialog.m_now_fee->value(),
                                                         /*at_height=*/100, /*registered_height=*/1)};
    QVERIFY(preview.valid);
    QVERIFY(preview.early);
    for (int row = 0; row < 3; ++row) {
        QCOMPARE(dialog.m_now_table->item(row, 2)->text(),
                 SharedMnFormatAmount(BitcoinUnits::Unit::DASH, preview.payouts[row]));
    }
    // The actor pays the penalty and the fee out of its principal; everybody
    // else is paid their principal plus a share of that penalty
    QVERIFY(preview.payouts[0] < amounts[0]);
    QVERIFY(preview.payouts[1] > amounts[1]);
    QVERIFY(preview.payouts[2] > amounts[2]);

    // A typed fee outside the allowed range is clamped when the field loses
    // focus, and the preview follows the fee that will actually be sent
    auto* fee_edit{dialog.m_now_fee->findChild<QLineEdit*>()};
    QVERIFY(fee_edit != nullptr);
    for (const auto& [typed, clamped] : {std::pair{"0.5", CProDisTx::MAX_FEE}, std::pair{"0.000005", CAmount{1000}}}) {
        fee_edit->clear();
        QTest::keyClicks(fee_edit, typed);
        QFocusEvent focus_out{QEvent::FocusOut};
        QApplication::sendEvent(fee_edit, &focus_out);
        QCOMPARE(dialog.m_now_fee->value(), clamped);
        const auto clamped_preview{MnShareSession::PenaltyPreviewFor(amounts, /*actor_index=*/0, 5 * COIN,
                                                                     /*early_period_blocks=*/1000, clamped,
                                                                     /*at_height=*/100, /*registered_height=*/1)};
        QCOMPARE(dialog.m_now_table->item(0, 2)->text(),
                 SharedMnFormatAmount(BitcoinUnits::Unit::DASH, clamped_preview.payouts[0]));
    }
}

void MasternodeMaintenanceTests::standbyDissolutionFile()
{
    TestChain100Setup test;
    m_node.setContext(&test.m_node);
    auto source{MakeSharedSource({400 * COIN, 300 * COIN, 300 * COIN}, 5 * COIN, /*early_period_blocks=*/1000)};
    MasternodeEntry entry{source, "collateral", 50};

    DissolveDialog dialog(m_node, /*wallet_model=*/nullptr, entry, /*current_height=*/100, /*parent=*/nullptr);
    const QString full_hex{QString(64, QLatin1Char('a'))};
    const QString immediate_hex{QString(64, QLatin1Char('b'))};
    dialog.m_sb_share_index = 1;
    dialog.m_sb_hex_full = full_hex;
    dialog.m_sb_hex_immediate = immediate_hex;

    QCOMPARE(dialog.standbyFileName(),
             QStringLiteral("standby-dissolution-%1-share2.txt").arg(entry.proTxHash().left(8)));

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path{dir.filePath("standby.txt")};
    QString error;
    QVERIFY2(dialog.writeStandbyFile(path, error), qPrintable(error));

    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QStringList lines{QString::fromUtf8(file.readAll()).split(QLatin1Char('\n'))};
    file.close();

    // Both variants are in one file, each introduced by a header line saying
    // when it can be broadcast
    const auto header_line = [&lines](const QString& prefix) {
        for (int i = 0; i < lines.size(); ++i) {
            if (lines.at(i).startsWith(prefix)) return i;
        }
        return -1;
    };
    const int full_header{header_line(QStringLiteral("FULL PRINCIPAL"))};
    const int immediate_header{header_line(QStringLiteral("IMMEDIATE"))};
    QVERIFY(full_header >= 0);
    QVERIFY(immediate_header > full_header);
    QCOMPARE(lines.at(full_header + 1), full_hex);
    QCOMPARE(lines.at(immediate_header + 1), immediate_hex);
    QVERIFY(lines.contains(QStringLiteral("Share 2 of 3")));
    QVERIFY(lines.contains(QStringLiteral("Masternode: %1").arg(entry.proTxHash())));
}

void MasternodeMaintenanceTests::rotateOperatorKeyValidation()
{
    // A 96-character hex string is the right length for a basic-scheme BLS
    // public key but says nothing about whether the bytes are a curve point
    QVERIFY(!RotateSharedKeysDialog::IsValidOperatorKey(QString(96, QLatin1Char('f'))));
    QVERIFY(!RotateSharedKeysDialog::IsValidOperatorKey(QString(96, QLatin1Char('z'))));
    QVERIFY(!RotateSharedKeysDialog::IsValidOperatorKey(QString(94, QLatin1Char('0'))));
    QVERIFY(!RotateSharedKeysDialog::IsValidOperatorKey(""));

    CBLSSecretKey key;
    key.MakeNewKey();
    QVERIFY(RotateSharedKeysDialog::IsValidOperatorKey(
        QString::fromStdString(key.GetPublicKey().ToString(/*specificLegacyScheme=*/false))));
}

void MasternodeMaintenanceTests::rotationSenderComesFromTheInputs()
{
    TestChain100Setup test;
    m_node.setContext(&test.m_node);
    WalletContext& context{*m_node.walletLoader().context()};
    const auto wallet{MakeCoinbaseWallet(m_node, context, test, "preparer", /*encrypt=*/false)};
    QVERIFY(wallet != nullptr);
    WalletGuard guard{context, wallet};

    MasternodeTestUtil::GuiModels models{m_node};
    QVERIFY2(models.ok, qPrintable(QString::fromStdString(models.error.translated)));
    WalletModel wallet_model(interfaces::MakeWallet(context, wallet), models.client);

    auto source{MakeSharedSource({400 * COIN, 300 * COIN, 300 * COIN}, 5 * COIN, /*early_period_blocks=*/1000)};
    source->setShareOwner(0, ToKeyID(PKHash(test.coinbaseKey.GetPubKey())));
    MasternodeEntry entry{source, "collateral", 50};

    // The request this wallet prepared, pasted back into a fresh dialog: a
    // rotation takes as long as the other share owners take to answer, so the
    // modal window that prepared it is long gone
    const COutPoint fee_input{test.m_coinbase_txns.at(0)->GetHash(), 0};
    const QString envelope{MakeRegistrarEnvelope(source->getProTxHash(), fee_input)};

    RotateSharedKeysDialog ours(m_node, &wallet_model, entry, /*parent=*/nullptr);
    ours.preloadEnvelope(envelope);
    QVERIFY(ours.m_collector->hasTransaction());
    QVERIFY2(ours.preparedByThisWallet(), qPrintable(ours.m_status_label->text()));
    // The editable form stays, the requested change is shown like on any other
    // wallet, and Send only waits for the other approvals
    QVERIFY(!ours.m_requested_change->isHidden());
    QVERIFY(!ours.m_operator_edit->isHidden());
    QVERIFY(!ours.m_finish_banner->isHidden());
    QVERIFY(ours.m_reason_label->text().contains("approve"));

    // Another share owner's wallet does not own the fee inputs, so it reads the
    // request instead of trying to send it
    const auto other_wallet{MakeTestWallet(m_node, context, "approver")};
    QVERIFY(other_wallet != nullptr);
    WalletGuard other_guard{context, other_wallet};
    WalletModel other_model(interfaces::MakeWallet(context, other_wallet), models.client);

    RotateSharedKeysDialog theirs(m_node, &other_model, entry, /*parent=*/nullptr);
    theirs.preloadEnvelope(envelope);
    QVERIFY(theirs.m_collector->hasTransaction());
    QVERIFY(!theirs.preparedByThisWallet());
    QVERIFY(!theirs.m_requested_change->isHidden());
    QVERIFY(!theirs.m_send_button->isEnabled());
    QVERIFY(theirs.m_reason_label->text().contains("prepared the request"));
}

void MasternodeMaintenanceTests::maintenanceEnvelopePreload()
{
    TestChain100Setup test;
    m_node.setContext(&test.m_node);
    auto source{MakeSharedSource({400 * COIN, 300 * COIN, 300 * COIN}, 5 * COIN, /*early_period_blocks=*/1000)};
    MasternodeEntry entry{source, "collateral", 50};

    DissolveDialog dialog(m_node, /*wallet_model=*/nullptr, entry, /*current_height=*/100, /*parent=*/nullptr);
    QCOMPARE(dialog.m_tabs->currentIndex(), 0);

    const QString envelope{MakeDissolveEnvelope(source->getProTxHash(), entry.shares())};
    UniValue json;
    QVERIFY(json.read(envelope.toStdString()));
    const QString expected_code{shared_mn::EnvelopeFingerprint(json)};

    dialog.preloadEnvelope(envelope);
    QCOMPARE(dialog.m_tabs->currentIndex(), dialog.m_together_tab_index);
    QVERIFY(dialog.m_un_collector->hasTransaction());
    QCOMPARE(dialog.m_un_collector->code(), expected_code);
    QVERIFY(dialog.m_un_collector->m_board->lastReceived().contains(expected_code));

    // An envelope for another masternode is refused and leaves the adopted one alone
    QString other_error;
    QVERIFY(!dialog.m_un_collector->importEnvelope(MakeDissolveEnvelope(uint256::ONE, entry.shares()), other_error));
    QVERIFY(!other_error.isEmpty());
    QVERIFY(dialog.m_un_collector->hasTransaction());
}

void MasternodeMaintenanceTests::dissolveRequestMustReturnPrincipal()
{
    TestChain100Setup test;
    m_node.setContext(&test.m_node);
    WalletContext& context{*m_node.walletLoader().context()};
    const auto wallet{MakeCoinbaseWallet(m_node, context, test, "approver", /*encrypt=*/false)};
    QVERIFY(wallet != nullptr);
    WalletGuard guard{context, wallet};
    MasternodeTestUtil::GuiModels models{m_node};
    QVERIFY2(models.ok, qPrintable(QString::fromStdString(models.error.translated)));
    WalletModel wallet_model(interfaces::MakeWallet(context, wallet), models.client);

    const std::vector<CAmount> amounts{400 * COIN, 300 * COIN, 300 * COIN};
    auto source{MakeSharedSource(amounts, 5 * COIN, /*early_period_blocks=*/1000)};
    source->setShareOwner(1, ToKeyID(PKHash(test.coinbaseKey.GetPubKey())));
    MasternodeEntry entry{source, "collateral", 50};
    const auto& shares{entry.shares()};
    const uint256& protx{source->getProTxHash()};

    DissolveDialog dialog(m_node, &wallet_model, entry, /*current_height=*/100, /*parent=*/nullptr);

    // The attack a unanimous dissolution makes possible: name the victim as
    // the actor, drop the actor output, and sweep the whole collateral to
    // another share's refund script. Consensus accepts it once every share
    // owner signs, so the wallet must never offer it for signing.
    {
        CMutableTransaction tx{MakeDissolveTemplate(protx, /*actor_index=*/0, shares, TEST_DISSOLVE_FEE)};
        tx.vout.clear();
        tx.vout.emplace_back(amounts[0] + amounts[1] - TEST_DISSOLVE_FEE, shares[1].scriptRefund);
        tx.vout.emplace_back(amounts[2], shares[2].scriptRefund);
        CProDisTx payload;
        payload.proTxHash = protx;
        payload.actorIndex = 0;
        SetTxPayload(tx, payload);

        QString error;
        QVERIFY(!dialog.m_un_collector->importEnvelope(MakeDissolveEnvelopeFor(protx, tx), error));
        QVERIFY2(error.contains("principal", Qt::CaseInsensitive), qPrintable(error));
        QVERIFY(!dialog.m_un_collector->hasTransaction());
    }

    // An output nobody's share asked for
    {
        CKeyID stranger;
        stranger.SetHex("99");
        CMutableTransaction tx{MakeDissolveTemplate(protx, /*actor_index=*/0, shares, TEST_DISSOLVE_FEE)};
        tx.vout.emplace_back(1000, GetScriptForDestination(PKHash{stranger}));
        QString error;
        QVERIFY(!dialog.m_un_collector->importEnvelope(MakeDissolveEnvelopeFor(protx, tx), error));
        QVERIFY(!error.isEmpty());
        QVERIFY(!dialog.m_un_collector->hasTransaction());
    }

    // A fee past the consensus ceiling burns the actor's principal
    {
        const CMutableTransaction tx{
            MakeDissolveTemplate(protx, /*actor_index=*/0, shares, CProDisTx::MAX_FEE + 1)};
        QString error;
        QVERIFY(!dialog.m_un_collector->importEnvelope(MakeDissolveEnvelopeFor(protx, tx), error));
        QVERIFY2(error.contains("fee", Qt::CaseInsensitive), qPrintable(error));
        QVERIFY(!dialog.m_un_collector->hasTransaction());
    }

    // The honest template is adopted, and its payouts are on screen next to
    // the button that approves them
    const QString genuine{MakeDissolveEnvelope(protx, shares)};
    dialog.preloadEnvelope(genuine);
    QVERIFY2(dialog.m_un_collector->hasTransaction(), qPrintable(dialog.m_un_collector->m_status_label->text()));
    QVERIFY(dialog.m_un_table->isVisibleTo(&dialog));
    QCOMPARE(dialog.m_un_table->rowCount(), 3);
    QCOMPARE(dialog.m_un_table->item(0, 0)->text(), QString("Share 1 of 3"));
    QCOMPARE(dialog.m_un_table->item(0, 2)->text(),
             SharedMnFormatAmount(BitcoinUnits::Unit::DASH, amounts[0] - TEST_DISSOLVE_FEE));
    QCOMPARE(dialog.m_un_table->item(1, 2)->text(), SharedMnFormatAmount(BitcoinUnits::Unit::DASH, amounts[1]));
    QCOMPARE(dialog.m_un_table->item(2, 2)->text(), SharedMnFormatAmount(BitcoinUnits::Unit::DASH, amounts[2]));
    QVERIFY(dialog.m_un_fee_label->text().contains(SharedMnFormatAmount(BitcoinUnits::Unit::DASH, TEST_DISSOLVE_FEE)));
    QVERIFY(dialog.m_un_fee_label->text().contains("Share 1 of 3"));

    // A refused message blocks approving, even with a valid request loaded
    // before it: approving would sign that earlier request, not the refused
    // one. v24 is not active here, so the button is disabled regardless; what
    // this checks is the status and that nothing reaching the slot signs.
    const QString loaded_tx{dialog.m_un_collector->txHex()};
    CMutableTransaction underpaying{MakeDissolveTemplate(protx, /*actor_index=*/0, shares, TEST_DISSOLVE_FEE)};
    underpaying.vout[1].nValue -= COIN;
    UniValue other_network;
    QVERIFY(other_network.read(genuine.toStdString()));
    other_network.pushKV("network", "main");
    for (const auto& [refused, reason] : {std::pair{MakeDissolveEnvelopeFor(protx, underpaying), "principal"},
                                          std::pair{QString::fromStdString(other_network.write()), "network"}}) {
        dialog.preloadEnvelope(genuine);
        dialog.preloadEnvelope(refused);
        const QString status{dialog.m_un_collector->m_status_label->text()};
        QVERIFY2(status.contains(reason) && status.contains("cannot be approved") && !status.contains("ignored"),
                 qPrintable(status));

        dialog.approveTogether();
        const bool signing_started{dialog.isBusy()};
        QTRY_VERIFY(!dialog.isBusy());
        QVERIFY(!signing_started);
        QCOMPARE(dialog.m_un_collector->signedCount(), 0);
        QCOMPARE(dialog.m_un_collector->txHex(), loaded_tx);
    }

    // A pasted message too large to read is refused the same way
    dialog.preloadEnvelope(genuine);
    QApplication::clipboard()->setText(QString(MAX_ENVELOPE_FILE_BYTES + 1, QLatin1Char('x')));
    dialog.m_un_collector->pasteEnvelope();
    const QString oversized_status{dialog.m_un_collector->m_status_label->text()};
    QVERIFY2(oversized_status.contains("too large") && oversized_status.contains("cannot be approved"),
             qPrintable(oversized_status));

    // Opening the genuine request again lifts the block
    dialog.preloadEnvelope(genuine);
    QVERIFY2(!dialog.m_un_collector->m_status_label->text().contains("cannot be approved"),
             qPrintable(dialog.m_un_collector->m_status_label->text()));
}
