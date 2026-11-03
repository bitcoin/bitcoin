// Copyright (c) 2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/test/masternodewidgettests.h>

#include <qt/test/masternodetestutil.h>

#include <bls/bls.h>
#include <chainparams.h>
#include <evo/providertx.h>
#include <evo/specialtx.h>
#include <evo/types.h>
#include <interfaces/node.h>
#include <interfaces/providertx.h>
#include <interfaces/wallet.h>
#include <key_io.h>
#include <qt/clientmodel.h>
#include <qt/masternodelist.h>
#include <qt/masternodeoperationrunner.h>
#include <qt/masternodewidgets.h>
#include <qt/masternodewizard.h>
#include <qt/optionsmodel.h>
#include <qt/qvalidatedlineedit.h>
#include <qt/sharedmnwidgets.h>
#include <qt/walletmodel.h>
#include <primitives/transaction.h>
#include <random.h>
#include <script/script.h>
#include <script/standard.h>
#include <test/util/setup_common.h>
#include <util/system.h>
#include <util/translation.h>
#include <validation.h>
#include <wallet/wallet.h>

#include <QAbstractButton>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QSemaphore>
#include <QSignalSpy>
#include <QSpinBox>
#include <QStackedWidget>
#include <QTest>
#include <QThread>
#include <QTimer>

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using wallet::CreateMockWalletDatabase;
using wallet::CWallet;
using wallet::WalletContext;

namespace {
interfaces::ProviderTxError TestProviderError()
{
    return {interfaces::ProviderTxErrorCode::INTERNAL_ERROR, Untranslated("fake provider failure"), {}, std::nullopt};
}

QString TestP2PKHAddress(uint8_t marker)
{
    uint160 key_id;
    key_id.begin()[0] = marker;
    return QString::fromStdString(EncodeDestination(PKHash{key_id}));
}

void ClickMessageBox(QMessageBox::StandardButton standard_button, QString* text = nullptr)
{
    QTimer::singleShot(0, [standard_button, text] {
        for (QWidget* const widget : QApplication::topLevelWidgets()) {
            auto* const message_box{qobject_cast<QMessageBox*>(widget)};
            if (message_box == nullptr) continue;
            if (text != nullptr) *text = message_box->text();
            QAbstractButton* const button{message_box->button(standard_button)};
            if (button != nullptr) button->click();
        }
    });
}

class BlockingEVO final : public interfaces::EVO
{
public:
    enum class Behavior : uint8_t {
        BLOCK_AND_ERROR,
        THROW_STANDARD,
        THROW_UNKNOWN,
        RETURN_SUCCESS,
    };

    std::pair<interfaces::MnListPtr, const CBlockIndex*> getListAtChainTip() override { return {}; }
    interfaces::ProviderTxCapabilities getProviderTxCapabilities() override { return {}; }
    std::optional<interfaces::ProviderTxError> validateProviderNetInfo(const interfaces::ProviderNetInfo&, MnType,
                                                                       uint16_t, bool, bool) override
    {
        return std::nullopt;
    }
    std::optional<std::string> findProviderNetInfoInUse(const interfaces::ProviderNetInfo&, MnType, uint16_t,
                                                        bool) override
    {
        return std::nullopt;
    }

    interfaces::ProviderTxResult<interfaces::ProviderTxSubmission> registerMasternode(
        interfaces::Wallet&, const interfaces::ProviderRegistrationRequest&) override
    {
        operation_thread.store(QThread::currentThread());
        ++operation_count;
        entered.release();
        switch (behavior.load()) {
        case Behavior::BLOCK_AND_ERROR:
            proceed.acquire();
            return TestProviderError();
        case Behavior::THROW_STANDARD:
            throw std::runtime_error("fake provider exception");
        case Behavior::THROW_UNKNOWN:
            throw 42;
        case Behavior::RETURN_SUCCESS:
            return interfaces::ProviderTxSubmission{success_tx, submitted.load()};
        }
        return TestProviderError();
    }

    interfaces::ProviderTxResult<interfaces::PreparedProviderRegistration> prepareMasternodeRegistration(
        interfaces::Wallet&, const interfaces::ProviderRegistrationRequest&) override
    {
        return TestProviderError();
    }
    interfaces::ProviderTxResult<interfaces::ProviderTxSubmission> submitMasternodeRegistration(
        interfaces::Wallet&, const CTransactionRef&, const std::vector<unsigned char>&) override
    {
        return TestProviderError();
    }
    interfaces::ProviderTxResult<interfaces::ProviderTxSubmission> updateMasternodeService(
        interfaces::Wallet&, const interfaces::ProviderUpdateServiceRequest&) override
    {
        return TestProviderError();
    }
    interfaces::ProviderTxResult<interfaces::ProviderTxSubmission> updateMasternodeRegistrar(
        interfaces::Wallet&, const interfaces::ProviderUpdateRegistrarRequest&) override
    {
        return TestProviderError();
    }
    interfaces::ProviderTxResult<interfaces::ProviderTxSubmission> revokeMasternode(
        interfaces::Wallet&, const interfaces::ProviderRevokeRequest&) override
    {
        return TestProviderError();
    }
    interfaces::ProviderTxResult<interfaces::PreparedSharedRegistration> prepareSharedRegistration(
        const interfaces::SharedRegistrationRequest&) override
    {
        return TestProviderError();
    }
    interfaces::ProviderTxResult<interfaces::SharedSignResult> signShared(interfaces::Wallet&,
                                                                          const interfaces::SharedSignRequest&) override
    {
        return TestProviderError();
    }
    interfaces::ProviderTxResult<interfaces::ProviderTxSubmission> combineShared(
        interfaces::Wallet&, const interfaces::SharedCombineRequest&) override
    {
        return TestProviderError();
    }
    interfaces::ProviderTxResult<interfaces::ProviderTxSubmission> dissolveShared(
        interfaces::Wallet&, const interfaces::SharedDissolveRequest&) override
    {
        return TestProviderError();
    }
    interfaces::ProviderTxResult<interfaces::PreparedSharedConsent> prepareSharedDissolution(
        const interfaces::SharedDissolvePrepareRequest&) override
    {
        return TestProviderError();
    }
    interfaces::ProviderTxResult<interfaces::ProviderTxSubmission> updateShare(
        interfaces::Wallet&, const interfaces::SharedUpdateShareRequest&) override
    {
        return TestProviderError();
    }
    interfaces::ProviderTxResult<interfaces::PreparedSharedConsent> prepareSharedRegistrarUpdate(
        interfaces::Wallet&, const interfaces::SharedRegistrarUpdatePrepareRequest&) override
    {
        return TestProviderError();
    }

    QSemaphore entered;
    QSemaphore proceed;
    std::atomic<Behavior> behavior{Behavior::BLOCK_AND_ERROR};
    std::atomic<QThread*> operation_thread{nullptr};
    std::atomic<int> operation_count{0};
    std::atomic<bool> submitted{true};
    CTransactionRef success_tx{MakeTransactionRef(CMutableTransaction{})};
};

using MasternodeTestUtil::TestKeyID;

CScript TestScript(uint8_t marker)
{
    return GetScriptForDestination(PKHash{TestKeyID(marker)});
}

//! Masternode entry fake for the list and model integration: a regular
//! masternode, or a shared one when constructed with collateral shares.
class TestListMnEntry final : public interfaces::MnEntry
{
public:
    explicit TestListMnEntry(uint8_t marker, std::vector<interfaces::MnShare> shares = {}) :
        interfaces::MnEntry(CDeterministicMNCPtr{}),
        m_shares{std::move(shares)},
        m_owner{m_shares.empty() ? TestKeyID(marker) : CKeyID{}},
        m_voting{TestKeyID(static_cast<uint8_t>(marker + 1))},
        m_payout{TestScript(static_cast<uint8_t>(marker + 2))},
        m_collateral{uint256::ONE, marker}
    {
        m_hash.begin()[0] = marker;
    }

    bool isBanned() const override { return false; }
    CService getNetInfoPrimary() const override { return {}; }
    std::vector<CService> getPlatformHTTPSAddrs() const override { return {}; }
    MnType getType() const override { return MnType::Regular; }
    UniValue toJson() const override { return UniValue{UniValue::VOBJ}; }
    const CKeyID& getKeyIdOwner() const override { return m_owner; }
    const CKeyID& getKeyIdVoting() const override { return m_voting; }
    const COutPoint& getCollateralOutpoint() const override { return m_collateral; }
    const CScript& getScriptPayout() const override { return m_payout; }
    //! Mirrors CDeterministicMNState::GetOwnerRewardScripts(): a shared
    //! masternode pays every share's reward script, the refund script when
    //! that share never set one
    std::vector<CScript> getScriptPayouts() const override
    {
        if (m_shares.empty()) return {m_payout};
        std::vector<CScript> ret;
        ret.reserve(m_shares.size());
        for (const auto& share : m_shares) ret.push_back(share.rewardScript());
        return ret;
    }
    const CScript& getScriptOperatorPayout() const override { return m_operator_payout; }
    const int32_t& getLastPaidHeight() const override { return m_last_paid; }
    const int32_t& getPoSePenalty() const override { return m_penalty; }
    const int32_t& getRegisteredHeight() const override { return m_registered; }
    const uint16_t& getOperatorReward() const override { return m_operator_reward; }
    const uint256& getProTxHash() const override { return m_hash; }
    bool isShared() const override { return !m_shares.empty(); }
    std::vector<interfaces::MnShare> getShares() const override { return m_shares; }
    const uint32_t& getEarlyPeriodBlocks() const override { return m_early_period_blocks; }
    const CAmount& getEarlyPenalty() const override { return m_early_penalty; }

    uint32_t m_early_period_blocks{0};
    CAmount m_early_penalty{0};

private:
    std::vector<interfaces::MnShare> m_shares;
    CKeyID m_owner;
    CKeyID m_voting;
    CScript m_payout;
    CScript m_operator_payout;
    COutPoint m_collateral;
    uint256 m_hash;
    int32_t m_last_paid{10};
    int32_t m_penalty{0};
    int32_t m_registered{100};
    uint16_t m_operator_reward{0};
};

std::shared_ptr<MasternodeEntry> MakeListEntry(const std::shared_ptr<TestListMnEntry>& dmn)
{
    return std::make_shared<MasternodeEntry>(dmn, TestP2PKHAddress(0xee), /*next_payment_height=*/0);
}
} // namespace

void MasternodeWidgetTests::endpointTokenization()
{
    QCOMPARE(MasternodeWidgetUtil::tokenizeEndpointList(QString()), QStringList{});
    QCOMPARE(MasternodeWidgetUtil::tokenizeEndpointList("  1.2.3.4:19999,\n[2001:db8::1]:443\texample.com:443  "),
             (QStringList{"1.2.3.4:19999", "[2001:db8::1]:443", "example.com:443"}));
    // Tokenization deliberately preserves duplicates and syntax. The typed
    // provider validator below owns those rules.
    QCOMPARE(MasternodeWidgetUtil::tokenizeEndpointList("1.2.3.4:19999, 1.2.3.4:19999"),
             (QStringList{"1.2.3.4:19999", "1.2.3.4:19999"}));
}

void MasternodeWidgetTests::providerNetInfoValidation_data()
{
    QTest::addColumn<int>("mn_type");
    QTest::addColumn<int>("version");
    QTest::addColumn<bool>("optional");
    QTest::addColumn<QStringList>("core");
    QTest::addColumn<QStringList>("platform_p2p");
    QTest::addColumn<QStringList>("platform_https");
    QTest::addColumn<int>("legacy_p2p_port");
    QTest::addColumn<int>("legacy_https_port");
    QTest::addColumn<bool>("valid");
    QTest::addColumn<QString>("error_fragment");

    const int regular{static_cast<int>(MnType::Regular)};
    const int evo{static_cast<int>(MnType::Evo)};
    const int basic{ProTxVersion::BasicBLS};
    const int extended{ProTxVersion::ExtAddr};
    // providerNetInfoValidation() runs on mainnet
    const QString core1{QString("1.1.1.1:%1").arg(MainParams().GetDefaultPort())};
    const QString core2{QString("1.1.1.2:%1").arg(MainParams().GetDefaultPort())};
    const int wrong_network_port{MainParams().GetDefaultPort() + 1};

    const auto add_row = [](const char* name, int type, int version, bool optional, QStringList core, QStringList p2p,
                            QStringList https, int p2p_port, int https_port, bool valid, QString error = {}) {
        QTest::newRow(name) << type << version << optional << core << p2p << https << p2p_port << https_port << valid
                            << error;
    };

    add_row("regular-basic-empty-optional", regular, basic, true, {}, {}, {}, -1, -1, true);
    add_row("regular-basic-empty-required", regular, basic, false, {}, {}, {}, -1, -1, false, "cannot be empty");
    add_row("regular-basic-ipv4", regular, basic, true, {core1}, {}, {}, -1, -1, true);
    add_row("regular-basic-second-address", regular, basic, true, {core1, core2}, {}, {}, -1, -1, false,
            "too many entries");
    add_row("regular-basic-ipv6", regular, basic, true,
            {QString("[2606:4700:4700::1111]:%1").arg(Params().GetDefaultPort())}, {}, {}, -1, -1, false, "invalid input");
    add_row("regular-basic-wrong-network-port", regular, basic, true, {QString("1.1.1.1:%1").arg(wrong_network_port)},
            {}, {}, -1, -1, false, "invalid port");
    add_row("regular-extended-multiple", regular, extended, true, {core1, core2}, {}, {}, -1, -1, true);
    add_row("regular-extended-duplicate", regular, extended, true, {core1, core1}, {}, {}, -1, -1, false, "duplicate");
    add_row("regular-extended-over-limit", regular, extended, true,
            {core1, core2, QString("1.1.1.3:%1").arg(Params().GetDefaultPort()),
             QString("1.1.1.4:%1").arg(Params().GetDefaultPort()), QString("1.1.1.5:%1").arg(Params().GetDefaultPort())},
            {}, {}, -1, -1, false, "too many entries");
    add_row("regular-extended-core-domain", regular, extended, true, {"example.com:9999"}, {}, {}, -1, -1, false,
            "invalid input");

    add_row("evo-basic-ports", evo, basic, true, {core1}, {}, {}, 26656, 443, true);
    add_row("evo-basic-zero-port", evo, basic, true, {core1}, {}, {}, 0, 443, false, "valid port");
    add_row("evo-extended-all-empty", evo, extended, true, {}, {}, {}, -1, -1, true);
    add_row("evo-extended-complete", evo, extended, true, {core1}, {"1.1.1.1:26656"}, {"1.1.1.1:443"}, -1, -1, true);
    add_row("evo-extended-platform-only", evo, extended, true, {}, {"1.1.1.2:26656"}, {"example.com:443"}, -1, -1,
            false, "bad-protx-netinfo-empty");
    add_row("evo-extended-https-domain", evo, extended, true, {core1}, {"1.1.1.1:26656"},
            {"1.1.1.1:443", "rpc.example.org:443"}, -1, -1, true);
    add_row("evo-extended-p2p-domain", evo, extended, true, {core1}, {"p2p.example.org:26656"}, {"rpc.example.org:443"},
            -1, -1, false, "invalid input");
    add_row("evo-extended-core-only", evo, extended, true, {core1}, {}, {}, -1, -1, false,
            "cannot be empty if other fields populated");
    add_row("evo-extended-one-platform-family", evo, extended, true, {core1}, {"1.1.1.1:26656"}, {}, -1, -1, false,
            "cannot be empty if other fields populated");
    add_row("evo-extended-cross-purpose-duplicate", evo, extended, true, {core1}, {"1.1.1.1:26656"}, {"1.1.1.1:26656"},
            -1, -1, false, "duplicate");
    add_row("invalid-version", regular, 0, true, {}, {}, {}, -1, -1, false, "invalid provider transaction version");
    add_row("invalid-type", static_cast<int>(MnType::Invalid), basic, true, {}, {}, {}, -1, -1, false,
            "invalid masternode type");
}

void MasternodeWidgetTests::providerNetInfoValidation()
{
    BasicTestingSetup test{CBaseChainParams::MAIN};
    m_node.setContext(&test.m_node);
    QFETCH(int, mn_type);
    QFETCH(int, version);
    QFETCH(bool, optional);
    QFETCH(QStringList, core);
    QFETCH(QStringList, platform_p2p);
    QFETCH(QStringList, platform_https);
    QFETCH(int, legacy_p2p_port);
    QFETCH(int, legacy_https_port);
    QFETCH(bool, valid);
    QFETCH(QString, error_fragment);

    interfaces::ProviderNetInfo net_info;
    for (const QString& entry : core)
        net_info.core_p2p.push_back(entry.toStdString());
    if (static_cast<MnType>(mn_type) == MnType::Evo) {
        if (version >= ProTxVersion::ExtAddr) {
            std::vector<std::string> p2p;
            std::vector<std::string> https;
            for (const QString& entry : platform_p2p)
                p2p.push_back(entry.toStdString());
            for (const QString& entry : platform_https)
                https.push_back(entry.toStdString());
            net_info.platform_p2p = std::move(p2p);
            net_info.platform_https = std::move(https);
        } else {
            net_info.platform_p2p = static_cast<uint16_t>(legacy_p2p_port);
            net_info.platform_https = static_cast<uint16_t>(legacy_https_port);
        }
    }

    const auto error{m_node.evo().validateProviderNetInfo(net_info, static_cast<MnType>(mn_type),
                                                          static_cast<uint16_t>(version), /*ipv4_optional=*/false,
                                                          optional)};
    QCOMPARE(!error.has_value(), valid);
    if (!valid) {
        QVERIFY(error.has_value());
        QVERIFY2(QString::fromStdString(error->message.original).contains(error_fragment), error->message.original.c_str());
    }
}

void MasternodeWidgetTests::operationRunnerThreading()
{
    TestChain100Setup test;
    m_node.setContext(&test.m_node);
    WalletContext& context{*m_node.walletLoader().context()};
    const auto wallet{std::make_shared<CWallet>(m_node.context()->chain.get(), m_node.context()->coinjoin_loader.get(),
                                                "", gArgs, CreateMockWalletDatabase())};
    wallet->LoadWallet();
    auto wallet_interface{interfaces::MakeWallet(context, wallet)};
    QVERIFY(wallet_interface != nullptr);

    BlockingEVO evo;
    int callback_count{0};
    QThread* callback_thread{nullptr};
    auto callback = [&](MasternodeOperationRunner::SubmissionResult completion) {
        ++callback_count;
        callback_thread = QThread::currentThread();
        QVERIFY(std::holds_alternative<interfaces::ProviderTxError>(completion));
    };
    MasternodeOperationRunner runner(evo, *wallet_interface);

    QVERIFY(runner.registerMasternode(interfaces::ProviderRegistrationRequest{}, callback));
    const bool entered_first{evo.entered.tryAcquire(1, 5000)};
    const bool busy_first{runner.isBusy()};
    const bool accepted_second{runner.registerMasternode(interfaces::ProviderRegistrationRequest{}, callback)};
    const int first_operation_count{evo.operation_count.load()};
    QThread* const first_operation_thread{evo.operation_thread.load()};
    // Release before asserting so an assertion failure cannot strand the worker
    // and deadlock the runner destructor.
    evo.proceed.release(accepted_second ? 2 : 1);
    QVERIFY(entered_first);
    QVERIFY(busy_first);
    QVERIFY(!accepted_second);
    QCOMPARE(first_operation_count, 1);
    QVERIFY(first_operation_thread != QThread::currentThread());
    QTRY_COMPARE_WITH_TIMEOUT(callback_count, 1, 5000);
    QCOMPARE(callback_thread, QThread::currentThread());
    QVERIFY(!runner.isBusy());

    // Completion clears the one-flight guard and allows the runner to be reused.
    QVERIFY(runner.registerMasternode(interfaces::ProviderRegistrationRequest{}, callback));
    const bool entered_reuse{evo.entered.tryAcquire(1, 5000)};
    const int reuse_operation_count{evo.operation_count.load()};
    evo.proceed.release();
    QVERIFY(entered_reuse);
    QCOMPARE(reuse_operation_count, 2);
    QTRY_COMPARE_WITH_TIMEOUT(callback_count, 2, 5000);
}

void MasternodeWidgetTests::operationRunnerExceptions()
{
    TestChain100Setup test;
    m_node.setContext(&test.m_node);
    WalletContext& context{*m_node.walletLoader().context()};
    const auto wallet{std::make_shared<CWallet>(m_node.context()->chain.get(), m_node.context()->coinjoin_loader.get(),
                                                "", gArgs, CreateMockWalletDatabase())};
    wallet->LoadWallet();
    auto wallet_interface{interfaces::MakeWallet(context, wallet)};
    QVERIFY(wallet_interface != nullptr);

    BlockingEVO evo;
    std::optional<MasternodeOperationRunner::SubmissionResult> callback_result;
    const auto callback = [&callback_result](MasternodeOperationRunner::SubmissionResult completion) {
        callback_result.emplace(std::move(completion));
    };
    MasternodeOperationRunner runner(evo, *wallet_interface);

    evo.behavior.store(BlockingEVO::Behavior::THROW_STANDARD);
    QVERIFY(runner.registerMasternode(interfaces::ProviderRegistrationRequest{}, callback));
    QTRY_VERIFY_WITH_TIMEOUT(callback_result.has_value(), 5000);
    const auto* standard_error{std::get_if<interfaces::ProviderTxError>(&*callback_result)};
    QVERIFY(standard_error != nullptr);
    QCOMPARE(standard_error->code, interfaces::ProviderTxErrorCode::INTERNAL_ERROR);
    QVERIFY(QString::fromStdString(standard_error->message.original).contains("fake provider exception"));
    QVERIFY(!runner.isBusy());

    callback_result.reset();
    evo.behavior.store(BlockingEVO::Behavior::THROW_UNKNOWN);
    QVERIFY(runner.registerMasternode(interfaces::ProviderRegistrationRequest{}, callback));
    QTRY_VERIFY_WITH_TIMEOUT(callback_result.has_value(), 5000);
    const auto* unknown_error{std::get_if<interfaces::ProviderTxError>(&*callback_result)};
    QVERIFY(unknown_error != nullptr);
    QCOMPARE(unknown_error->code, interfaces::ProviderTxErrorCode::INTERNAL_ERROR);
    QVERIFY(QString::fromStdString(unknown_error->message.original).contains("operation failed"));
    QVERIFY(!runner.isBusy());
}

void MasternodeWidgetTests::operationRunnerShutdownDelivery()
{
    TestChain100Setup test;
    m_node.setContext(&test.m_node);
    WalletContext& context{*m_node.walletLoader().context()};
    const auto wallet{std::make_shared<CWallet>(m_node.context()->chain.get(), m_node.context()->coinjoin_loader.get(),
                                                "", gArgs, CreateMockWalletDatabase())};
    wallet->LoadWallet();
    auto wallet_interface{interfaces::MakeWallet(context, wallet)};
    QVERIFY(wallet_interface != nullptr);

    BlockingEVO evo;
    int callback_count{0};
    QThread* callback_thread{nullptr};
    auto runner{std::make_unique<MasternodeOperationRunner>(evo, *wallet_interface)};
    QVERIFY(runner->registerMasternode(interfaces::ProviderRegistrationRequest{},
                                       [&](MasternodeOperationRunner::SubmissionResult completion) {
                                           ++callback_count;
                                           callback_thread = QThread::currentThread();
                                           QVERIFY(std::holds_alternative<interfaces::ProviderTxError>(completion));
                                           runner.reset();
                                       }));

    const bool entered{evo.entered.tryAcquire(1, 5000)};
    evo.proceed.release();
    QVERIFY(entered);
    QCOMPARE(callback_count, 0);

    auto* const runner_ptr{runner.get()};
    runner_ptr->shutdown();
    QCOMPARE(callback_count, 1);
    QCOMPARE(callback_thread, QThread::currentThread());
    QVERIFY(runner == nullptr);
    QApplication::processEvents();
    QCOMPARE(callback_count, 1);

    // Shutdown must drain work that was only queued, map its exception, and
    // deliver it without requiring another GUI event-loop turn. This covers
    // the owner-destruction path immediately after an operation is launched.
    BlockingEVO throwing_evo;
    throwing_evo.behavior.store(BlockingEVO::Behavior::THROW_STANDARD);
    std::optional<MasternodeOperationRunner::SubmissionResult> throwing_result;
    QThread* throwing_callback_thread{nullptr};
    auto throwing_runner{std::make_unique<MasternodeOperationRunner>(throwing_evo, *wallet_interface)};
    QVERIFY(throwing_runner->registerMasternode(interfaces::ProviderRegistrationRequest{},
                                                [&](MasternodeOperationRunner::SubmissionResult completion) {
                                                    throwing_result.emplace(std::move(completion));
                                                    throwing_callback_thread = QThread::currentThread();
                                                }));
    throwing_runner.reset();
    QVERIFY(throwing_result.has_value());
    const auto* error{std::get_if<interfaces::ProviderTxError>(&*throwing_result)};
    QVERIFY(error != nullptr);
    QVERIFY(QString::fromStdString(error->message.original).contains("fake provider exception"));
    QCOMPARE(throwing_callback_thread, QThread::currentThread());
}

void MasternodeWidgetTests::feeSourcePickerEligibility()
{
    TestChain100Setup test;
    m_node.setContext(&test.m_node);
    WalletContext& context{*m_node.walletLoader().context()};
    const auto wallet{std::make_shared<CWallet>(m_node.context()->chain.get(), m_node.context()->coinjoin_loader.get(),
                                                "", gArgs, CreateMockWalletDatabase())};
    wallet->LoadWallet();
    wallet->SetWalletFlag(wallet::WALLET_FLAG_DESCRIPTORS);

    CTxDestination first;
    CTxDestination second;
    const CBlockIndex* tip{nullptr};
    {
        LOCK(wallet->cs_wallet);
        wallet->SetupDescriptorScriptPubKeyMans("", "");
        const auto first_result{wallet->GetNewDestination("")};
        const auto second_result{wallet->GetNewDestination("")};
        QVERIFY(first_result);
        QVERIFY(second_result);
        first = *first_result;
        second = *second_result;
        tip = WITH_LOCK(test.m_node.chainman->GetMutex(), return test.m_node.chainman->ActiveChain().Tip());
        QVERIFY(tip != nullptr);
        wallet->SetLastBlockProcessed(tip->nHeight, tip->GetBlockHash());
    }

    CMutableTransaction funding;
    funding.vin.emplace_back(GetRandHash(), 0);
    funding.vout.emplace_back(10 * COIN, GetScriptForDestination(first));
    funding.vout.emplace_back(20 * COIN, GetScriptForDestination(first));
    funding.vout.emplace_back(5 * COIN, GetScriptForDestination(second));
    const CTransactionRef funding_tx{MakeTransactionRef(funding)};
    QVERIFY(wallet->AddToWallet(funding_tx, wallet::TxStateConfirmed{tip->GetBlockHash(), tip->nHeight, /*index=*/0}) !=
            nullptr);

    MasternodeTestUtil::GuiModels models{m_node};
    QVERIFY2(models.ok, qPrintable(QString::fromStdString(models.error.translated)));
    WalletModel wallet_model(interfaces::MakeWallet(context, wallet), models.client);
    FeeSourcePicker picker;
    picker.setWalletModel(&wallet_model);

    QCOMPARE(picker.count(), 2);
    QCOMPARE(picker.selectedAddress(), QString::fromStdString(EncodeDestination(first)));
    picker.setMinimumBalance(30 * COIN);
    QCOMPARE(picker.count(), 1);

    // Registration excludes its selected existing collateral coin from the
    // address's available fee balance, but leaves other outputs at that exact
    // destination eligible.
    const COutPoint excluded{funding_tx->GetHash(), 0};
    picker.setMinimumBalance(20 * COIN);
    picker.setExcludedOutpoint(excluded);
    QCOMPARE(picker.count(), 1);
    QCOMPARE(picker.selectedAddress(), QString::fromStdString(EncodeDestination(first)));
    picker.setMinimumBalance(20 * COIN + 1);
    QCOMPARE(picker.count(), 0);

    picker.setExcludedOutpoint(std::nullopt);
    picker.setMinimumBalance(30 * COIN);
    QVERIFY(wallet_model.wallet().lockCoin(excluded, /*write_to_db=*/false));
    picker.refresh();
    QCOMPARE(picker.count(), 0);
    QVERIFY(wallet_model.wallet().unlockCoin(excluded));
}

void MasternodeWidgetTests::registeredCollateralExclusion()
{
    TestChain100Setup test;
    m_node.setContext(&test.m_node);
    WalletContext& context{*m_node.walletLoader().context()};
    const auto wallet{std::make_shared<CWallet>(m_node.context()->chain.get(), m_node.context()->coinjoin_loader.get(),
                                                "", gArgs, CreateMockWalletDatabase())};
    wallet->LoadWallet();
    wallet->SetWalletFlag(wallet::WALLET_FLAG_DESCRIPTORS);

    CTxDestination destination;
    const CBlockIndex* tip{nullptr};
    {
        LOCK(wallet->cs_wallet);
        wallet->SetupDescriptorScriptPubKeyMans("", "");
        const auto destination_result{wallet->GetNewDestination("")};
        QVERIFY(destination_result);
        destination = *destination_result;
        tip = WITH_LOCK(test.m_node.chainman->GetMutex(), return test.m_node.chainman->ActiveChain().Tip());
        QVERIFY(tip != nullptr);
        wallet->SetLastBlockProcessed(tip->nHeight, tip->GetBlockHash());
    }

    CMutableTransaction funding;
    funding.vin.emplace_back(GetRandHash(), 0);
    funding.vout.emplace_back(GetMnType(MnType::Regular).collat_amount, GetScriptForDestination(destination));
    const CTransactionRef funding_tx{MakeTransactionRef(funding)};
    QVERIFY(wallet->AddToWallet(funding_tx, wallet::TxStateConfirmed{tip->GetBlockHash(), tip->nHeight, /*index=*/0}) !=
            nullptr);

    MasternodeTestUtil::GuiModels models{m_node};
    QVERIFY2(models.ok, qPrintable(QString::fromStdString(models.error.translated)));
    WalletModel wallet_model(interfaces::MakeWallet(context, wallet), models.client);
    RegisterMasternodeWizard wizard(m_node, &wallet_model);
    wizard.m_col_wallet->setChecked(true);

    wizard.refreshCollateralCandidates({});
    QCOMPARE(wizard.m_col_utxo_combo->count(), 1);
    const COutPoint collateral{funding_tx->GetHash(), 0};
    QCOMPARE(wizard.m_col_utxo_combo->currentData().toString(), QString::fromStdString(collateral.hash.ToString()));

    wizard.refreshCollateralCandidates({collateral});
    QCOMPARE(wizard.m_col_utxo_combo->count(), 0);
    QVERIFY(!wizard.m_col_utxo_none->isHidden());
}

void MasternodeWidgetTests::wizardMinimumGeometry()
{
#if defined(Q_OS_MACOS)
    if (QApplication::platformName() == "minimal") {
        QWARN("Skipping wizardMinimumGeometry on macOS with the minimal platform due to QTBUG-49686");
        return;
    }
#endif
    TestChain100Setup test;
    m_node.setContext(&test.m_node);

    RegisterMasternodeWizard wizard(m_node, /*walletModel=*/nullptr);
    QCOMPARE(wizard.m_platform_p2p_port->value(), Params().GetDefaultPlatformP2PPort());
    QCOMPARE(wizard.m_platform_https_port->value(), Params().GetDefaultPlatformHTTPPort());
    wizard.resize(700, 560);
    wizard.show();
    QApplication::processEvents();
    QCOMPARE(wizard.minimumSize(), QSize(700, 560));
    QCOMPARE(wizard.size(), QSize(700, 560));
    QVERIFY(wizard.m_progress_label->text().contains("Step 1"));
    QVERIFY(wizard.m_progress_label->text().contains("of 8"));
    QVERIFY(wizard.m_order.contains(RegisterMasternodeWizard::PageSecret));

    const auto operator_modes{wizard.m_operator_widget->findChildren<QRadioButton*>()};
    const auto existing_mode{std::find_if(operator_modes.begin(), operator_modes.end(), [](const QRadioButton* radio) {
        return radio->text().contains("existing operator", Qt::CaseInsensitive);
    })};
    QVERIFY(existing_mode != operator_modes.end());
    (*existing_mode)->setChecked(true);
    QVERIFY(!wizard.m_order.contains(RegisterMasternodeWizard::PageSecret));
    QVERIFY(wizard.m_progress_label->text().contains("of 7"));
    wizard.m_type_evo->setChecked(true);
    QCOMPARE(wizard.windowTitle(), QString("Register EvoNode"));
    QVERIFY(wizard.m_progress_label->text().contains("of 8"));

    const auto controls_fit = [&wizard](RegisterMasternodeWizard::Page page_id) {
        wizard.enterPage(page_id);
        QApplication::processEvents();
        QWidget* const page{wizard.m_pages->widget(page_id)};
        QVERIFY(page != nullptr);
        const QRect available{page->rect()};
        for (QWidget* const child : page->findChildren<QWidget*>()) {
            const bool interactive{
                qobject_cast<QAbstractButton*>(child) != nullptr || qobject_cast<QComboBox*>(child) != nullptr ||
                qobject_cast<QDoubleSpinBox*>(child) != nullptr || qobject_cast<QLineEdit*>(child) != nullptr ||
                qobject_cast<QPlainTextEdit*>(child) != nullptr || qobject_cast<QSpinBox*>(child) != nullptr};
            if (!interactive || !child->isVisibleTo(page)) continue;
            const QRect child_rect{child->mapTo(page, QPoint{0, 0}), child->size()};
            const QString available_text{
                QString("%1,%2 %3x%4").arg(available.x()).arg(available.y()).arg(available.width()).arg(available.height())};
            const QString child_text{
                QString("%1,%2 %3x%4").arg(child_rect.x()).arg(child_rect.y()).arg(child_rect.width()).arg(child_rect.height())};
            QVERIFY2(available.contains(child_rect), qPrintable(QString("%1 on page %2 falls outside %3 (geometry %4)")
                                                                    .arg(child->metaObject()->className())
                                                                    .arg(static_cast<int>(page_id))
                                                                    .arg(available_text)
                                                                    .arg(child_text)));
        }
    };

    for (const auto page : {RegisterMasternodeWizard::PageType, RegisterMasternodeWizard::PageCollateral,
                            RegisterMasternodeWizard::PageService, RegisterMasternodeWizard::PagePayout,
                            RegisterMasternodeWizard::PageFee, RegisterMasternodeWizard::PageSign}) {
        controls_fit(page);
    }
    controls_fit(RegisterMasternodeWizard::PagePlatform);

    for (const auto page_id : {RegisterMasternodeWizard::PageKeys, RegisterMasternodeWizard::PageReview,
                               RegisterMasternodeWizard::PageSecret, RegisterMasternodeWizard::PageResult}) {
        wizard.enterPage(page_id);
        QApplication::processEvents();
        QWidget* const page{wizard.m_pages->widget(page_id)};
        auto* const scroll{page->findChild<QScrollArea*>()};
        QVERIFY(scroll != nullptr);
        QCOMPARE(scroll->objectName(), QString("mnWizardScroll"));
        QVERIFY(scroll->widgetResizable());
        QCOMPARE(scroll->horizontalScrollBarPolicy(), Qt::ScrollBarAlwaysOff);
        QCOMPARE(scroll->verticalScrollBarPolicy(), Qt::ScrollBarAsNeeded);
        QVERIFY(scroll->widget() != nullptr);
        QVERIFY(page->rect().contains(QRect{scroll->mapTo(page, QPoint{0, 0}), scroll->size()}));
    }

    wizard.enterPage(RegisterMasternodeWizard::PageFee);
    QVERIFY(wizard.m_fee_explain->text().contains("exact fee", Qt::CaseInsensitive));
    QVERIFY(!wizard.m_fee_explain->text().contains("0.001"));

    wizard.m_col_external->setChecked(true);
    wizard.goToPage(RegisterMasternodeWizard::PageSign);
    QVERIFY(!wizard.m_back_button->isEnabled());
    QVERIFY(wizard.m_back_button->toolTip().contains("invalidate", Qt::CaseInsensitive));

    wizard.setBusy(true, "Testing…");
    QVERIFY(wizard.m_busy_bar->isVisible());
    QCOMPARE(wizard.m_next_button->text(), QString("Testing…"));
    wizard.setBusy(false);
    QVERIFY(!wizard.m_busy_bar->isVisible());
}

void MasternodeWidgetTests::wizardPageValidation()
{
#if defined(Q_OS_MACOS)
    if (QApplication::platformName() == "minimal") {
        QWARN("Skipping wizardPageValidation on macOS with the minimal platform due to QTBUG-49686");
        return;
    }
#endif
    TestChain100Setup test;
    m_node.setContext(&test.m_node);

    RegisterMasternodeWizard wizard(m_node, /*walletModel=*/nullptr);
    const QString null_address{TestP2PKHAddress(0)};
    const QString collateral{TestP2PKHAddress(1)};
    const QString owner{TestP2PKHAddress(2)};
    const QString voting{TestP2PKHAddress(3)};
    const QString payout{TestP2PKHAddress(4)};
    QString error;

    const auto expect_invalid = [&wizard, &error](RegisterMasternodeWizard::Page page) {
        error.clear();
        QVERIFY(!wizard.validatePage(page, error));
        QVERIFY2(!error.isEmpty(), "Invalid page value must have an actionable error");
    };

    // A syntactically valid but null outpoint must not reach the provider
    // transaction builder.
    wizard.m_col_external->setChecked(true);
    wizard.m_col_txid->setText(QString(64, QLatin1Char('0')));
    expect_invalid(RegisterMasternodeWizard::PageCollateral);

    // Encoded all-zero key identifiers are valid Base58 strings, but are null
    // owner/voting identities and must be rejected explicitly.
    wizard.m_col_fund->setChecked(true);
    wizard.m_col_address->setText(collateral);
    wizard.m_owner_edit->setText(null_address);
    wizard.m_voting_edit->clear();
    expect_invalid(RegisterMasternodeWizard::PageKeys);

    wizard.m_owner_edit->setText(owner);
    wizard.m_voting_edit->setText(null_address);
    expect_invalid(RegisterMasternodeWizard::PageKeys);

    wizard.m_voting_edit->setText(collateral);
    expect_invalid(RegisterMasternodeWizard::PageKeys);

    // The exact-wallet flow stores its collateral destination in the combo's
    // dedicated role, and applies the same consensus role-separation rules.
    wizard.m_col_wallet->setChecked(true);
    wizard.m_col_utxo_combo->addItem("collateral", QString(64, QLatin1Char('1')));
    wizard.m_col_utxo_combo->setItemData(0, collateral, RegisterMasternodeWizard::COLLATERAL_ADDRESS_ROLE);
    wizard.m_owner_edit->setText(collateral);
    wizard.m_voting_edit->setText(voting);
    expect_invalid(RegisterMasternodeWizard::PageKeys);
    wizard.m_owner_edit->setText(owner);
    wizard.m_voting_edit->setText(collateral);
    expect_invalid(RegisterMasternodeWizard::PageKeys);
    wizard.m_voting_edit->setText(voting);
    wizard.m_payout_edit->setText(collateral);
    expect_invalid(RegisterMasternodeWizard::PagePayout);

    // Role collisions are invalid regardless of which collateral workflow the
    // user selected. The funded-collateral address is another prohibited role.
    for (QRadioButton* mode : {wizard.m_col_fund, wizard.m_col_wallet, wizard.m_col_external}) {
        mode->setChecked(true);
        wizard.m_owner_edit->setText(owner);
        wizard.m_voting_edit->setText(voting);

        wizard.m_payout_edit->setText(owner);
        expect_invalid(RegisterMasternodeWizard::PagePayout);

        wizard.m_payout_edit->setText(voting);
        expect_invalid(RegisterMasternodeWizard::PagePayout);
    }

    wizard.m_col_fund->setChecked(true);
    wizard.m_col_address->setText(collateral);
    wizard.m_payout_edit->setText(collateral);
    expect_invalid(RegisterMasternodeWizard::PagePayout);

    // A non-null, unrelated payout remains valid after the collision checks.
    wizard.m_payout_edit->setText(payout);
    error.clear();
    QVERIFY2(wizard.validatePage(RegisterMasternodeWizard::PagePayout, error), qPrintable(error));

    wizard.m_fee_picker->addItem("fee", payout);
    const auto request{wizard.buildRegistrationRequest(error)};
    QVERIFY2(request.has_value(), qPrintable(error));
    QCOMPARE(request->payouts.size(), size_t{1});
    QCOMPARE(request->payouts.front().reward, interfaces::ProviderPayout::MAX_REWARD);

    wizard.m_type_evo->setChecked(true);
    wizard.m_platform_nodeid->setText(QString(40, QLatin1Char('0')));
    error.clear();
    QVERIFY(!wizard.validatePage(RegisterMasternodeWizard::PagePlatform, error));
    QVERIFY(error.contains("node ID", Qt::CaseInsensitive));

    // Once validation has failed, edits revalidate immediately and the
    // reserved error row keeps the button bar from jumping.
    wizard.m_type_regular->setChecked(true);
    wizard.m_col_external->setChecked(true);
    wizard.goToPage(RegisterMasternodeWizard::PageCollateral);
    wizard.show();
    QApplication::processEvents();
    const int button_y{wizard.m_next_button->y()};
    wizard.m_col_txid->setText("invalid");
    wizard.onNext();
    QVERIFY(wizard.m_validation_page == RegisterMasternodeWizard::PageCollateral);
    QVERIFY(!wizard.m_error_label->text().isEmpty());
    QApplication::processEvents();
    QCOMPARE(wizard.m_next_button->y(), button_y);
    wizard.m_col_txid->setText(QString(64, QLatin1Char('1')));
    QVERIFY(!wizard.m_validation_page.has_value());
    QVERIFY(wizard.m_error_label->text().isEmpty());
    QApplication::processEvents();
    QCOMPARE(wizard.m_next_button->y(), button_y);
}

void MasternodeWidgetTests::broadcastConfirmation()
{
#if defined(Q_OS_MACOS)
    if (QApplication::platformName() == "minimal") {
        QWARN("Skipping broadcastConfirmation on macOS with the minimal platform due to QTBUG-49686");
        return;
    }
#endif
    TestChain100Setup test;
    m_node.setContext(&test.m_node);

    RegisterMasternodeWizard wizard(m_node, /*walletModel=*/nullptr);
    wizard.m_fee_picker->addItem("fee source", TestP2PKHAddress(1));

    bool found{false};
    bool send_disabled{false};
    bool unsigned_hidden{false};
    QString prompt;
    QString details;
    QTimer::singleShot(0, [&] {
        for (QWidget* const widget : QApplication::topLevelWidgets()) {
            auto* const confirmation{qobject_cast<QMessageBox*>(widget)};
            if (confirmation == nullptr || !confirmation->isVisible()) continue;
            found = true;
            prompt = confirmation->text();
            details = confirmation->informativeText() + confirmation->detailedText();
            send_disabled = !confirmation->button(QMessageBox::Yes)->isEnabled();
            unsigned_hidden = confirmation->button(QMessageBox::Save) == nullptr;
            confirmation->reject();
            break;
        }
    });

    QVERIFY(!wizard.confirmBroadcast());
    QVERIFY(found);
    QVERIFY(send_disabled);
    QVERIFY(unsigned_hidden);
    QVERIFY(prompt.contains("registration", Qt::CaseInsensitive));
    QVERIFY(details.contains("fee source", Qt::CaseInsensitive));
    QVERIFY(details.contains("wallet", Qt::CaseInsensitive));
}

void MasternodeWidgetTests::masternodeListRegistrationAvailability()
{
    TestChain100Setup test;
    m_node.setContext(&test.m_node);
    WalletContext& context{*m_node.walletLoader().context()};
    const auto wallet{std::make_shared<CWallet>(m_node.context()->chain.get(), m_node.context()->coinjoin_loader.get(),
                                                "", gArgs, CreateMockWalletDatabase())};
    wallet->LoadWallet();
    MasternodeTestUtil::GuiModels models{m_node};
    QVERIFY2(models.ok, qPrintable(QString::fromStdString(models.error.translated)));
    WalletModel wallet_model(interfaces::MakeWallet(context, wallet), models.client);

    MasternodeList list;
    auto* const register_button{list.findChild<QPushButton*>("btnRegisterMasternode")};
    auto* const owned_checkbox{list.findChild<QCheckBox*>("checkBoxOwned")};
    auto* const context_menu{list.findChild<QMenu*>(QString{}, Qt::FindDirectChildrenOnly)};
    QVERIFY(register_button != nullptr);
    QVERIFY(owned_checkbox != nullptr);
    QVERIFY(context_menu != nullptr);
    QVERIFY(context_menu->toolTipsVisible());
    QVERIFY(!register_button->isEnabled());
    QVERIFY(register_button->toolTip().contains("requires a wallet", Qt::CaseInsensitive));

    list.setWalletModel(&wallet_model);
    QVERIFY(!register_button->isEnabled());
    QVERIFY(register_button->toolTip().contains("node is ready", Qt::CaseInsensitive));

    list.setClientModel(&models.client);
    QVERIFY(register_button->isEnabled());

    owned_checkbox->setChecked(true);
    list.setWalletModel(nullptr);
    QVERIFY(!register_button->isEnabled());
    QVERIFY(register_button->toolTip().contains("requires a wallet", Qt::CaseInsensitive));
    QVERIFY(!owned_checkbox->isEnabled());
    QVERIFY(!owned_checkbox->isChecked());

    // Attaching the wallet again restores the user's saved owned-only choice.
    list.setWalletModel(&wallet_model);
    QVERIFY(register_button->isEnabled());
    QVERIFY(owned_checkbox->isEnabled());
    QVERIFY(owned_checkbox->isChecked());
    owned_checkbox->setChecked(false);

    // A new tip re-evaluates the gate: the shared masternode button depends on
    // v24 activation, which a wallet opened before the fork would otherwise
    // never see happen.
    auto* const shared_button{list.findChild<QPushButton*>("btnSharedMasternode")};
    QVERIFY(shared_button != nullptr);
    const bool v24_active{m_node.isV24Active()};
    const QString shared_tooltip{shared_button->toolTip()};
    QCOMPARE(shared_button->isEnabled(), v24_active);
    QVERIFY(shared_tooltip.contains(v24_active ? "Create or continue" : "v24 hard fork", Qt::CaseInsensitive));

    for (const bool via_blocks : {true, false}) {
        shared_button->setEnabled(!v24_active);
        shared_button->setToolTip(QStringLiteral("stale"));
        if (via_blocks) {
            Q_EMIT models.client.numBlocksChanged(101, QDateTime::currentDateTime(), QString{}, 1.0, SyncType::BLOCK_SYNC,
                                                 SynchronizationState::POST_INIT);
        } else {
            Q_EMIT models.client.masternodeListChanged();
        }
        QCOMPARE(shared_button->isEnabled(), v24_active);
        QCOMPARE(shared_button->toolTip(), shared_tooltip);
    }

    list.setClientModel(nullptr);
    QVERIFY(!register_button->isEnabled());
    QVERIFY(register_button->toolTip().contains("node is ready", Qt::CaseInsensitive));

    // Detaching the model takes its connections with it: a late tip signal
    // must not reach a list that is no longer showing this node.
    shared_button->setToolTip(QStringLiteral("stale"));
    Q_EMIT models.client.masternodeListChanged();
    QCOMPARE(shared_button->toolTip(), QStringLiteral("stale"));

    // Reapplying the no-wallet state must preserve both the guard and its
    // explanation (for example after the last wallet is closed).
    list.setWalletModel(nullptr);
    QVERIFY(!register_button->isEnabled());
    QVERIFY(register_button->toolTip().contains("requires a wallet", Qt::CaseInsensitive));
}

void MasternodeWidgetTests::sharedMasternodeOwnedFilter()
{
    TestChain100Setup test;
    m_node.setContext(&test.m_node);
    WalletContext& context{*m_node.walletLoader().context()};
    const auto wallet{std::make_shared<CWallet>(m_node.context()->chain.get(), m_node.context()->coinjoin_loader.get(),
                                                "", gArgs, CreateMockWalletDatabase())};
    wallet->LoadWallet();
    wallet->SetWalletFlag(wallet::WALLET_FLAG_DESCRIPTORS);

    CTxDestination refund;
    {
        LOCK(wallet->cs_wallet);
        wallet->SetupDescriptorScriptPubKeyMans("", "");
        const auto refund_result{wallet->GetNewDestination("")};
        QVERIFY(refund_result);
        refund = *refund_result;
    }

    MasternodeTestUtil::GuiModels models{m_node};
    QVERIFY2(models.ok, qPrintable(QString::fromStdString(models.error.translated)));
    WalletModel wallet_model(interfaces::MakeWallet(context, wallet), models.client);

    // This wallet holds the refund destination of the second share only: its
    // rewards are paid elsewhere, so neither a payout script nor an owner or
    // voting key matches. The refund destination is still a stake in the
    // masternode, because that is where the principal returns on dissolution.
    std::vector<interfaces::MnShare> shares{
        {400 * COIN, TestScript(0x11), TestScript(0x12), TestKeyID(0x13)},
        {600 * COIN, GetScriptForDestination(refund), TestScript(0x14), TestKeyID(0x15)}};
    const auto mine{MakeListEntry(std::make_shared<TestListMnEntry>(0x20, shares))};
    QVERIFY(MasternodeList::isOwnedBy(wallet_model.wallet(), {}, *mine));

    shares[1].scriptRefund = TestScript(0x16);
    const auto theirs{MakeListEntry(std::make_shared<TestListMnEntry>(0x30, shares))};
    QVERIFY(!MasternodeList::isOwnedBy(wallet_model.wallet(), {}, *theirs));
}

void MasternodeWidgetTests::sharedMasternodeContextMenu()
{
    TestChain100Setup test;
    m_node.setContext(&test.m_node);
    WalletContext& context{*m_node.walletLoader().context()};
    const auto wallet{std::make_shared<CWallet>(m_node.context()->chain.get(), m_node.context()->coinjoin_loader.get(),
                                                "", gArgs, CreateMockWalletDatabase())};
    wallet->LoadWallet();
    wallet->SetWalletFlag(wallet::WALLET_FLAG_DESCRIPTORS);

    CTxDestination owner;
    {
        LOCK(wallet->cs_wallet);
        wallet->SetupDescriptorScriptPubKeyMans("", "");
        const auto owner_result{wallet->GetNewDestination("")};
        QVERIFY(owner_result);
        owner = *owner_result;
    }
    const auto* owner_hash{std::get_if<PKHash>(&owner)};
    QVERIFY(owner_hash != nullptr);

    MasternodeTestUtil::GuiModels models{m_node};
    QVERIFY2(models.ok, qPrintable(QString::fromStdString(models.error.translated)));
    WalletModel wallet_model(interfaces::MakeWallet(context, wallet), models.client);

    MasternodeList list;
    list.setWalletModel(&wallet_model);
    list.setClientModel(&models.client);

    const std::vector<interfaces::MnShare> theirs{{400 * COIN, TestScript(0x41), CScript{}, TestKeyID(0x42)},
                                                  {600 * COIN, TestScript(0x43), CScript{}, TestKeyID(0x44)}};
    std::vector<interfaces::MnShare> mine{theirs};
    mine[1].keyIDOwner = ToKeyID(*owner_hash);

    // A regular masternode keeps Update Registrar and hides every shared action
    const auto regular{MakeListEntry(std::make_shared<TestListMnEntry>(0x50))};
    list.updateContextMenuActions(regular.get());
    QVERIFY(list.m_action_update_registrar->isVisible());
    for (const QAction* action :
         {list.m_action_update_share, list.m_action_rotate_keys, list.m_action_dissolve, list.m_action_standby}) {
        QVERIFY(!action->isVisible());
    }

    // A shared masternode this wallet holds no share of shows the actions,
    // disabled, and says which key is missing instead of hiding the reason
    const auto foreign_shared{MakeListEntry(std::make_shared<TestListMnEntry>(0x60, theirs))};
    list.updateContextMenuActions(foreign_shared.get());
    QVERIFY(!list.m_action_update_registrar->isVisible());
    for (const QAction* action :
         {list.m_action_update_share, list.m_action_rotate_keys, list.m_action_dissolve, list.m_action_standby}) {
        QVERIFY(action->isVisible());
        QVERIFY(!action->isEnabled());
        QVERIFY2(action->toolTip().contains("share owner key"), qPrintable(action->toolTip()));
    }

    // Holding one share owner key enables all of them; the standby item is the
    // only one that reports the missing offline dissolution
    const auto owned_shared{MakeListEntry(std::make_shared<TestListMnEntry>(0x70, mine))};
    list.updateContextMenuActions(owned_shared.get());
    for (const QAction* action : {list.m_action_update_share, list.m_action_rotate_keys, list.m_action_dissolve}) {
        QVERIFY(action->isEnabled());
        // An unset tooltip reads back as the item's own text
        QCOMPARE(action->toolTip(), action->text());
    }
    QVERIFY(list.m_action_standby->isEnabled());
    QVERIFY(list.m_action_standby->toolTip().contains("not created on this computer", Qt::CaseInsensitive));

    // "Filter by > Owner Address" stays usable and picks the share this wallet
    // holds, which the row's search text matches
    auto* const filter_text{list.findChild<QLineEdit*>("filterText")};
    QVERIFY(filter_text != nullptr);
    QVERIFY(list.m_action_filter_owner->isEnabled());
    QCOMPARE(list.shareOwnerFilterAddress(*owned_shared), QString::fromStdString(EncodeDestination(owner)));
    QCOMPARE(list.shareOwnerFilterAddress(*foreign_shared),
             QString::fromStdString(EncodeDestination(PKHash{TestKeyID(0x42)})));
    QVERIFY(owned_shared->shareAddresses().contains(QString::fromStdString(EncodeDestination(owner))));

    list.setWalletModel(nullptr);
}

void MasternodeWidgetTests::standbyDissolutionDescription()
{
    const std::vector<interfaces::MnShare> shares{{400 * COIN, TestScript(0xa1), CScript{}, TestKeyID(0xa2)},
                                                  {600 * COIN, TestScript(0xa3), CScript{}, TestKeyID(0xa4)}};
    const auto entry{MakeListEntry(std::make_shared<TestListMnEntry>(0xa0, shares))};
    constexpr CAmount fee{100000};

    const auto make_dissolution = [&entry](uint16_t actor_index, const std::vector<CTxOut>& outputs, size_t sig_count) {
        CMutableTransaction tx;
        tx.nVersion = 3;
        tx.nType = TRANSACTION_PROVIDER_DISSOLVE;
        tx.vin.emplace_back(entry->collateralOutpointRaw());
        tx.vout = outputs;
        CProDisTx payload;
        payload.proTxHash = entry->proTxHashRaw();
        payload.actorIndex = actor_index;
        payload.vchSigs.assign(sig_count, CompactSignature{});
        SetTxPayload(tx, payload);
        return tx;
    };

    MasternodeList list;

    // The honest standby dissolution: share 2 acts, share 1 is paid its
    // principal in full, and only the acting share has signed
    const QString unilateral{list.describeStandbyDissolution(
        make_dissolution(/*actor_index=*/1, {{shares[0].amount, shares[0].scriptRefund},
                                             {shares[1].amount - fee, shares[1].scriptRefund}},
                         /*sig_count=*/1),
        entry.get())};
    QVERIFY2(unilateral.contains("Share 2 of 2"), qPrintable(unilateral));
    QVERIFY(unilateral.contains("Approved by: your signature only"));
    QVERIFY(unilateral.contains("Share 1 of 2 → " + SharedMnFormatAmount(BitcoinUnits::Unit::DASH, 400 * COIN)));
    QVERIFY(unilateral.contains("Fee: " + SharedMnFormatAmount(BitcoinUnits::Unit::DASH, fee)));
    QVERIFY(unilateral.contains("Every share's principal is returned"));
    QVERIFY(!unilateral.contains("not in the current masternode list"));

    // A transaction sweeping the collateral elsewhere says so instead of
    // repeating the reassuring wording
    const CScript stranger{TestScript(0xa9)};
    const QString hostile{list.describeStandbyDissolution(
        make_dissolution(/*actor_index=*/0, {{shares[0].amount + shares[1].amount - fee, stranger}},
                         /*sig_count=*/2),
        entry.get())};
    QVERIFY2(hostile.contains("does not return every share's full principal"), qPrintable(hostile));
    QVERIFY(hostile.contains("Approved by: every share owner"));
    QVERIFY(hostile.contains(QString::fromStdString(EncodeDestination(PKHash{TestKeyID(0xa9)}))));

    // Without the masternode in the list there are no share names to match
    // outputs against, and the description says that rather than inventing them
    const QString unknown{list.describeStandbyDissolution(
        make_dissolution(/*actor_index=*/0, {{shares[0].amount, shares[0].scriptRefund}}, /*sig_count=*/1), nullptr)};
    QVERIFY2(unknown.contains("not in the current masternode list"), qPrintable(unknown));
    QVERIFY(unknown.contains("does not return every share's full principal"));
}

void MasternodeWidgetTests::sharedMasternodeDetails()
{
    const std::vector<interfaces::MnShare> shares{{400 * COIN, TestScript(0x81), TestScript(0x82), TestKeyID(0x83)},
                                                  {600 * COIN, TestScript(0x84), CScript{}, TestKeyID(0x85)}};
    auto dmn{std::make_shared<TestListMnEntry>(0x90, shares)};
    dmn->m_early_period_blocks = 100;
    dmn->m_early_penalty = 5 * COIN;
    auto entry{MakeListEntry(dmn)};

    MasternodeModel model;
    const QString protx_hash{entry->proTxHash()};
    model.append(std::move(entry));
    model.setMyShareCounts({{protx_hash, 1}});

    QCOMPARE(model.data(model.index(0, MasternodeModel::TYPE), Qt::ToolTipRole).toString(),
             QString("Shared masternode · you hold 1 of 2 shares"));
    // The row itself says how many of the shares this wallet holds
    QCOMPARE(model.data(model.index(0, MasternodeModel::TYPE), Qt::DisplayRole).toString(),
             QString("Shared (you hold 1 of 2)"));
    QCOMPARE(model.data(model.index(0, MasternodeModel::TYPE), Qt::EditRole).toInt(), MasternodeModel::TYPE_SHARED);

    const auto* listed{model.getEntryAt(model.index(0, 0))};
    QVERIFY(listed != nullptr);
    const QString html{listed->toHtml(/*current_height=*/150, /*my_share_indexes=*/{1})};

    // Shares are numbered from 1 and the wallet's own share is marked
    QVERIFY(html.contains("Share 1"));
    QVERIFY(html.contains("Share 2 <b>(you)</b>"));
    QVERIFY(!html.contains("Share 1 <b>(you)</b>"));
    QVERIFY(!html.contains("Share 0"));
    QVERIFY(html.contains("(40.0%)"));
    QVERIFY(html.contains("(60.0%)"));
    // A share without its own reward script is paid at its refund address
    QVERIFY(html.contains("same as refund"));
    QVERIFY(html.contains("Early-exit penalty"));
    QVERIFY(html.contains("ends block 200"));
    QVERIFY(html.contains("Standby dissolution"));
    QVERIFY(html.contains("not created on this computer"));

    // Past the early period the details say so instead of counting down
    QVERIFY(listed->toHtml(/*current_height=*/250).contains("ended (block 200)"));
}

void MasternodeWidgetTests::wizardInteractionLifecycle()
{
#if defined(Q_OS_MACOS)
    if (QApplication::platformName() == "minimal") {
        QWARN("Skipping wizardInteractionLifecycle on macOS with the minimal platform due to QTBUG-49686");
        return;
    }
#endif
    TestChain100Setup test;
    m_node.setContext(&test.m_node);
    WalletContext& context{*m_node.walletLoader().context()};
    const auto wallet{std::make_shared<CWallet>(m_node.context()->chain.get(), m_node.context()->coinjoin_loader.get(),
                                                "", gArgs, CreateMockWalletDatabase())};
    wallet->LoadWallet();
    MasternodeTestUtil::GuiModels models{m_node};
    QVERIFY2(models.ok, qPrintable(QString::fromStdString(models.error.translated)));
    WalletModel wallet_model(interfaces::MakeWallet(context, wallet), models.client);
    RegisterMasternodeWizard wizard(m_node, &wallet_model);

    // Cancel, Escape, Back and a window close cannot tear down state owned by
    // an in-flight typed operation.
    wizard.goToPage(RegisterMasternodeWizard::PageReview);
    const int review_position{wizard.m_pos};
    QSignalSpy rejected_spy(&wizard, &QDialog::rejected);
    wizard.show();
    wizard.setBusy(true, "Testing…");
    wizard.onBack();
    QCOMPARE(wizard.m_pos, review_position);
    wizard.reject();
    QCOMPARE(rejected_spy.count(), 0);
    QTest::keyClick(&wizard, Qt::Key_Escape);
    QCOMPARE(rejected_spy.count(), 0);
    QVERIFY(wizard.isVisible());
    QVERIFY(!wizard.close());
    QVERIFY(wizard.isVisible());
    QCOMPARE(rejected_spy.count(), 0);
    wizard.setBusy(false);

    // A typed rejection leaves the same dialog editable and presents the
    // backend's actionable reason.
    wizard.m_stage = RegisterMasternodeWizard::Stage::Register;
    wizard.setBusy(true, "Testing…");
    QString rejection_text;
    ClickMessageBox(QMessageBox::Ok, &rejection_text);
    wizard.finishSubmission(TestProviderError());
    QVERIFY(!wizard.m_busy);
    QVERIFY(wizard.m_stage == RegisterMasternodeWizard::Stage::None);
    QVERIFY(wizard.m_next_button->isEnabled());
    QVERIFY(rejection_text.contains("fake provider failure", Qt::CaseInsensitive));

    // Provider capabilities are sampled again at submission time; a wizard
    // opened across activation returns to Platform instead of submitting stale
    // endpoint semantics.
    wizard.m_type_evo->setChecked(true);
    wizard.rebuildOrder();
    const uint16_t current_version{m_node.evo().getProviderTxCapabilities().version};
    wizard.m_platform_provider_version = current_version == std::numeric_limits<uint16_t>::max()
                                             ? static_cast<uint16_t>(current_version - 1)
                                             : static_cast<uint16_t>(current_version + 1);
    wizard.m_confirm_edit->setText(wizard.m_operator_widget->secretHex().right(4));
    wizard.goToPage(RegisterMasternodeWizard::PageReview);
    wizard.startRegistration(/*skip_confirmation=*/true);
    QCOMPARE(wizard.m_pages->currentIndex(), static_cast<int>(RegisterMasternodeWizard::PagePlatform));
    QVERIFY(wizard.m_error_label->isVisible());
    QVERIFY(wizard.m_error_label->text().contains("changed", Qt::CaseInsensitive));

    // Discarding a prepared external registration releases the collateral
    // lock acquired by this session after explicit confirmation.
    uint256 collateral_txid;
    collateral_txid.SetHex("1");
    const COutPoint collateral_outpoint{collateral_txid, 0};
    QVERIFY(wallet_model.wallet().lockCoin(collateral_outpoint, /*write_to_db=*/false));
    wizard.m_col_external->setChecked(true);
    wizard.m_prepared_tx = MakeTransactionRef(CMutableTransaction{});
    wizard.m_prepared_collateral_outpoint = collateral_outpoint;
    wizard.m_prepared_collateral_lock_acquired = true;
    QString discard_text;
    ClickMessageBox(QMessageBox::Yes, &discard_text);
    wizard.reject();
    QVERIFY(discard_text.contains("released", Qt::CaseInsensitive));
    QVERIFY(!wallet_model.wallet().isLockedCoin(collateral_outpoint));
    QCOMPARE(rejected_spy.count(), 1);
}

void MasternodeWidgetTests::registrationResultStates()
{
#if defined(Q_OS_MACOS)
    if (QApplication::platformName() == "minimal") {
        QWARN("Skipping registrationResultStates on macOS with the minimal platform due to QTBUG-49686");
        return;
    }
#endif
    TestChain100Setup test;
    m_node.setContext(&test.m_node);
    WalletContext& context{*m_node.walletLoader().context()};
    const auto wallet{std::make_shared<CWallet>(m_node.context()->chain.get(), m_node.context()->coinjoin_loader.get(),
                                                "", gArgs, CreateMockWalletDatabase())};
    wallet->LoadWallet();
    MasternodeTestUtil::GuiModels models{m_node};
    QVERIFY2(models.ok, qPrintable(QString::fromStdString(models.error.translated)));
    WalletModel wallet_model(interfaces::MakeWallet(context, wallet), models.client);

    RegisterMasternodeWizard wizard(m_node, &wallet_model);
    wizard.m_col_external->setChecked(true);
    wizard.m_service_edit->clear();

    // A generated key is shown and must be confirmed before any provider
    // operation can start. This guarantees that owner teardown cannot destroy
    // the only key copy after a successful background broadcast.
    wizard.goToPage(RegisterMasternodeWizard::PageReview);
    wizard.onNext();
    QCOMPARE(wizard.m_pages->currentIndex(), static_cast<int>(RegisterMasternodeWizard::PageSecret));
    QVERIFY(wizard.m_secret_note->text().contains("cannot start", Qt::CaseInsensitive));
    QVERIFY(wizard.m_secret_edit->text() == wizard.m_operator_widget->secretHex());
    QVERIFY(wizard.secretGateRequired());
    QVERIFY(!wizard.m_next_button->isEnabled());

    wizard.startRegistration(/*skip_confirmation=*/true);
    QVERIFY(wizard.m_stage == RegisterMasternodeWizard::Stage::None);
    QCOMPARE(wizard.m_pages->currentIndex(), static_cast<int>(RegisterMasternodeWizard::PageSecret));

    const QString suffix{wizard.m_operator_widget->secretHex().right(4)};
    wizard.m_confirm_edit->setText(suffix == "0000" ? "1111" : "0000");
    QVERIFY(!wizard.secretConfirmed());
    QVERIFY(!wizard.m_next_button->isEnabled());
    wizard.m_confirm_edit->setText(suffix);
    QVERIFY(wizard.secretConfirmed());
    QVERIFY(wizard.m_next_button->isEnabled());

    wizard.populateResult(QString(64, QLatin1Char('1')));
    wizard.goToPage(RegisterMasternodeWizard::PageResult);
    wizard.show();
    QApplication::processEvents();

    QVERIFY(wizard.m_result_tx_note->text().contains("external collateral"));
    QVERIFY(wizard.m_result_tx_note->text().contains("not controlled by this wallet"));
    QVERIFY(wizard.m_next_steps->text().contains("Update Service"));
    QVERIFY(wizard.m_next_button->isEnabled());

    // A malformed non-broadcast submit result must leave a lock acquired by
    // prepare under this session's ownership so cancel/destruction releases it.
    wizard.m_stage = RegisterMasternodeWizard::Stage::Submit;
    wizard.m_prepared_collateral_lock_acquired = true;
    wizard.finishSubmission(interfaces::ProviderTxSubmission{CTransactionRef{}, /*submitted=*/false});
    QVERIFY(wizard.m_prepared_collateral_lock_acquired);

    // Once broadcast, the registration owns that lock even if a malformed
    // backend result omits its transaction object.
    wizard.m_stage = RegisterMasternodeWizard::Stage::Submit;
    wizard.finishSubmission(interfaces::ProviderTxSubmission{CTransactionRef{}, /*submitted=*/true});
    QVERIFY(!wizard.m_prepared_collateral_lock_acquired);
}

void MasternodeWidgetTests::destinationValidation()
{
    const QString p2pkh{QString::fromStdString(EncodeDestination(PKHash{uint160{}}))};
    const QString p2sh{QString::fromStdString(EncodeDestination(ScriptHash{CScript() << OP_TRUE}))};

    QVERIFY(MasternodeWidgetUtil::isP2PKHAddress(p2pkh));
    QVERIFY(MasternodeWidgetUtil::isP2PKHorP2SHAddress(p2pkh));
    QVERIFY(!MasternodeWidgetUtil::isP2PKHAddress(p2sh));
    QVERIFY(MasternodeWidgetUtil::isP2PKHorP2SHAddress(p2sh));
    QVERIFY(!MasternodeWidgetUtil::isP2PKHorP2SHAddress("not-an-address"));
}

void MasternodeWidgetTests::operatorKeyModes()
{
    OperatorKeyWidget widget;
    QVERIFY(widget.mode() == OperatorKeyWidget::Mode::GenerateOnly);
    QVERIFY(widget.isValid());
    QVERIFY(!widget.publicKeyHex().isEmpty());
    QVERIFY(!widget.secretHex().isEmpty());

    CBLSSecretKey secret;
    secret.MakeNewKey();
    const auto radios{widget.findChildren<QRadioButton*>()};
    const auto existing_it{std::find_if(radios.begin(), radios.end(), [](const QRadioButton* radio) {
        return radio->text().contains("existing operator");
    })};
    QVERIFY(existing_it != radios.end());
    auto* existing_radio{*existing_it};
    existing_radio->setChecked(true);
    QVERIFY(widget.mode() == OperatorKeyWidget::Mode::Existing);
    QVERIFY(!widget.isValid());

    auto* edit{widget.findChild<QValidatedLineEdit*>()};
    QVERIFY(edit != nullptr);
    edit->setText(QString::fromStdString(secret.GetPublicKey().ToString(false)));
    QVERIFY(widget.isValid());
    QVERIFY(widget.secretHex().isEmpty());
}
