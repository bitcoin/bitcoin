// Copyright (c) 2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/test/sharedmnwalkthroughtests.h>

#include <qt/test/masternodetestutil.h>

#include <bls/bls.h>
#include <chain.h>
#include <chainparams.h>
#include <consensus/amount.h>
#include <evo/deterministicmns.h>
#include <interfaces/node.h>
#include <interfaces/wallet.h>
#include <key_io.h>
#include <node/context.h>
#include <qt/bitcoinamountfield.h>
#include <qt/clientfeeds.h>
#include <qt/clientmodel.h>
#include <qt/guiutil.h>
#include <qt/masternodelist.h>
#include <qt/masternodemodel.h>
#include <qt/masternodewidgets.h>
#include <qt/mnsharesession.h>
#include <qt/optionsmodel.h>
#include <qt/qvalidatedlineedit.h>
#include <qt/sharedmncreatedialog.h>
#include <qt/sharedmndialogs.h>
#include <qt/sharedmnwidgets.h>
#include <qt/walletmodel.h>
#include <rpc/register.h>
#include <script/standard.h>
#include <test/util/setup_common.h>
#include <univalue.h>
#include <util/system.h>
#include <util/translation.h>
#include <validation.h>
#include <validationinterface.h>
#include <wallet/receive.h>
#include <wallet/wallet.h>

#include <QAbstractButton>
#include <QApplication>
#include <QButtonGroup>
#include <QClipboard>
#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QLineEdit>
#include <QDebug>
#include <QMenu>
#include <QMessageBox>
#include <QPixmap>
#include <QPushButton>
#include <QRadioButton>
#include <QStyleFactory>
#include <QTabWidget>
#include <QTableView>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QScrollArea>
#include <QSpinBox>
#include <QTest>
#include <QTextStream>
#include <QTimer>

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

using wallet::CWallet;
using wallet::WalletContext;
using MasternodeTestUtil::FreshOperatorPubKey;
using MasternodeTestUtil::MakeTestWallet;
using MasternodeTestUtil::WalletGuard;
using wallet::WalletRescanReserver;

//! The dialogs run their provider operations on a worker thread; wait until
//! the last result has been delivered before looking at what it did
#define WAIT_UNTIL_IDLE(busy) QTRY_VERIFY_WITH_TIMEOUT(!(busy), 60000)

namespace {

//! Fixed size every wizard page is grabbed at, so the screenshots are directly
//! comparable. The dialog's own minimum is smaller; one page is additionally
//! grabbed at that minimum to prove nothing clips there.
constexpr int SHOT_WIDTH{1000};
constexpr int SHOT_HEIGHT{800};
constexpr int MIN_WIDTH{700};
constexpr int MIN_HEIGHT{560};

//! Writes one PNG per visited page into DASH_QT_SHOTS_DIR plus a manifest.
//! Disabled (and free) when the environment variable is not set, so the
//! walkthrough runs as an ordinary test in CI.
//!
//! The bare "minimal" platform has no font database, so its grabs show layout
//! without text; run with QT_QPA_PLATFORM=minimal:enable_fonts when the PNGs
//! are meant to be read.
class ShotRecorder
{
public:
    ShotRecorder()
    {
        m_dir = qEnvironmentVariable("DASH_QT_SHOTS_DIR");
        if (m_dir.isEmpty()) return;
        QDir().mkpath(m_dir);
        QFile manifest(manifestPath());
        if (manifest.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
            QTextStream(&manifest) << "filename\tdescription\n";
        }
    }

    bool enabled() const { return !m_dir.isEmpty(); }

    //! Grab `widget` whole. Wizard pages are grabbed at a fixed size; anything
    //! that sizes itself (message boxes, maintenance dialogs, menus) keeps its
    //! natural size.
    void capture(QWidget* widget, const QString& role, const QString& page, const QString& description,
                 bool fixed_size = true)
    {
        captureAt(widget, role, page, description, fixed_size ? QSize(SHOT_WIDTH, SHOT_HEIGHT) : QSize());
    }

    void captureAt(QWidget* widget, const QString& role, const QString& page, const QString& description,
                   const QSize& size)
    {
        if (!enabled() || widget == nullptr) return;
        if (size.isValid()) widget->resize(size);
        widget->show();
        settle();
        const QPixmap pixmap{widget->grab()};
        const QString name{QStringLiteral("%1-%2-%3.png")
                               .arg(m_counter++, 2, 10, QLatin1Char('0'))
                               .arg(role, page)};
        pixmap.save(m_dir + QLatin1Char('/') + name, "PNG");
        QFile manifest(manifestPath());
        if (manifest.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append)) {
            QTextStream(&manifest) << name << '\t' << description << '\n';
        }
    }

    static void settle()
    {
        for (int i = 0; i < 8; ++i) {
            QApplication::processEvents();
        }
    }

private:
    QString manifestPath() const { return m_dir + QStringLiteral("/manifest.txt"); }

    QString m_dir;
    int m_counter{1};
};

ShotRecorder& Shots()
{
    static ShotRecorder recorder;
    return recorder;
}

//! Answers the modal dialogs the flows raise and optionally screenshots them
//! first. Message boxes are accepted (waiting out the send-confirmation
//! countdown, whose button starts disabled); a file dialog, which cannot be
//! driven headless, is cancelled.
class ModalPilot : public QObject
{
public:
    ModalPilot()
    {
        m_timer.setInterval(10);
        connect(&m_timer, &QTimer::timeout, this, &ModalPilot::poll);
        m_timer.start();
    }

    //! Screenshot the next modal that appears, then answer it. Queue several
    //! calls when one click raises more than one box in a row.
    void captureNext(const QString& role, const QString& page, const QString& description)
    {
        m_queued.append({role, page, description});
    }

    //! Everything answered so far, for failure messages
    const QStringList& seen() const { return m_seen; }

private:
    void poll()
    {
        QWidget* const modal{QApplication::activeModalWidget()};
        if (modal == nullptr) {
            m_pending = nullptr;
            return;
        }
        // Give the box one event cycle to lay itself out before grabbing it
        if (m_pending != modal) {
            m_pending = modal;
            return;
        }
        if (auto* const box = qobject_cast<QMessageBox*>(modal)) {
            QAbstractButton* target{box->button(QMessageBox::Yes)};
            if (target == nullptr) target = box->button(QMessageBox::Ok);
            if (target == nullptr) target = box->defaultButton();
            if (target == nullptr && !box->buttons().isEmpty()) target = box->buttons().first();
            // The send confirmation keeps its accept button disabled while it
            // counts down; wait for it rather than cancelling the flow.
            if (target != nullptr && !target->isEnabled()) return;
            m_pending = nullptr;
            m_seen << (box->text() + QLatin1Char('\n') + box->informativeText() + QLatin1Char('\n') + box->detailedText());
            if (!m_queued.isEmpty()) {
                const Shot shot{m_queued.takeFirst()};
                Shots().capture(box, shot.role, shot.page, shot.description, /*fixed_size=*/false);
            }
            if (target != nullptr) {
                target->click();
            } else {
                box->accept();
            }
            return;
        }
        m_pending = nullptr;
        if (auto* const dialog = qobject_cast<QDialog*>(modal); dialog != nullptr) {
            dialog->reject();
        }
    }

    struct Shot {
        QString role;
        QString page;
        QString description;
    };

    QTimer m_timer;
    QWidget* m_pending{nullptr};
    QStringList m_seen;
    QList<Shot> m_queued;
};

void SyncWallet(interfaces::Node& node, CWallet& wallet)
{
    const CBlockIndex* const tip{WITH_LOCK(node.context()->chainman->GetMutex(),
                                           return node.context()->chainman->ActiveChain().Tip())};
    {
        LOCK(wallet.cs_wallet);
        wallet.SetLastBlockProcessed(tip->nHeight, tip->GetBlockHash());
    }
    WalletRescanReserver reserver(wallet);
    reserver.reserve();
    wallet.ScanForWalletTransactions(Params().GetConsensus().hashGenesisBlock, /*start_height=*/0, /*max_height=*/{},
                                     reserver, /*fUpdate=*/true, /*save_progress=*/false);
}

//! Run one RPC through the node the dialogs use. Returns the error text, or an
//! empty string on success, with the result in `out`.
QString RunRpc(interfaces::Node& node, const std::string& method, const UniValue& params, UniValue& out,
               const std::string& uri = "")
{
    try {
        out = node.executeRpc(method, params, uri);
    } catch (const UniValue& error) {
        return QString::fromStdString(error.write());
    } catch (const std::exception& error) {
        return QString::fromStdString(error.what());
    }
    return {};
}

//! Mine `blocks` blocks to `dest` through the node's own RPC layer.
//!
//! In a running node CDSNotificationInterface tells the deterministic
//! masternode manager about every new tip; the test fixture does not install
//! it, and until UpdatedBlockTip() has run
//! CDeterministicMNManager::GetListAtChainTip() - the list every shared
//! masternode RPC resolves a proTxHash in - stays empty. So do it here.
QString MineTo(interfaces::Node& node, const CTxDestination& dest, int blocks)
{
    UniValue params(UniValue::VOBJ);
    params.pushKV("nblocks", blocks);
    params.pushKV("address", EncodeDestination(dest));
    UniValue ignored;
    const QString error{RunRpc(node, "generatetoaddress", params, ignored)};
    SyncWithValidationInterfaceQueue();
    if (auto* const dmnman = node.context()->dmnman.get(); dmnman != nullptr) {
        dmnman->UpdatedBlockTip(WITH_LOCK(node.context()->chainman->GetMutex(),
                                          return node.context()->chainman->ActiveChain().Tip()));
    }
    return error;
}

int ChainHeight(interfaces::Node& node)
{
    return WITH_LOCK(node.context()->chainman->GetMutex(),
                     return node.context()->chainman->ActiveChain().Height());
}

//! Envelope on the clipboard, parsed back, with its fingerprint checked
QString TakeClipboardEnvelope(QString& fingerprint)
{
    const QString text{QApplication::clipboard()->text()};
    fingerprint.clear();
    MnShareSession round_trip;
    QString error;
    if (round_trip.fromJson(text.toStdString(), error)) fingerprint = round_trip.fingerprint();
    return text;
}

} // anonymous namespace

void SharedMnWalkthroughTests::initTestCase()
{
#if defined(Q_OS_MACOS)
    // The native macOS style paints through an NSView, which does not exist on
    // the minimal platform and crashes there (QTBUG-49686). Fusion is what the
    // Linux builds use anyway, and the Dash stylesheet still applies on top.
    if (QApplication::platformName() == "minimal") {
        if (QStyle* const fusion = QStyleFactory::create("Fusion"); fusion != nullptr) {
            QApplication::setStyle(fusion);
        }
    }
#endif
    // Grab the dialogs the way a user sees them: real fonts, real (light) theme
    // stylesheet. Both are loaded from Qt resources, so this works headless
    // under "minimal:enable_fonts". The plain "minimal" platform has no font
    // database at all: the application fonts cannot be registered and applying
    // the stylesheet's font rules aborted test_dash-qt on CI's static Linux
    // build, so the dialogs are then driven unstyled, like every other suite.
    if (GUIUtil::loadFonts()) {
        GUIUtil::loadStyleSheet(/*fForceUpdate=*/true);
        GUIUtil::updateFonts();
    }
}

void SharedMnWalkthroughTests::walkthrough()
{
    // A ten-block window with an eight-block threshold and no EHF: the blocks
    // mined below signal the started deployment, so v24 locks in and activates
    // around height 30, well before the wallets are funded.
    RegTestingSetup test{{"-vbparams=v24:0:999999999999:0:10:8:6:5:0"}};
    m_node.setContext(&test.m_node);
    WalletContext& context{*m_node.walletLoader().context()};
    ModalPilot pilot;

    // The dialogs go through interfaces::EVO, but the checks below read the
    // result back with "protx list" and "protx info". The test fixture builds
    // a wallet loader but never registers its commands, so do here what
    // AppInitMain() does: without this those calls come back as "This command
    // is not available".
    for (const auto& client : test.m_node.chain_clients) {
        client->registerRpcs();
    }
    m_node.walletLoader().registerOtherRpcs(GetWalletEvoRPCCommands());

    // Three separate descriptor wallets: each role signs with its own wallet,
    // as it would on its own machine.
    WalletGuard wallets{context};
    const auto coord_wallet{MakeTestWallet(m_node, context, "coord", /*broadcast=*/true)};
    const auto alice_wallet{MakeTestWallet(m_node, context, "alice", /*broadcast=*/true)};
    const auto bob_wallet{MakeTestWallet(m_node, context, "bob", /*broadcast=*/true)};
    for (const auto& wallet : {coord_wallet, alice_wallet, bob_wallet}) {
        wallets.keep(wallet);
    }

    for (const auto& wallet : {coord_wallet, alice_wallet, bob_wallet}) {
        const auto dest{wallet->GetNewDestination("mining")};
        QVERIFY(dest);
        QVERIFY2(MineTo(m_node, *dest, 12).isEmpty(), "generatetoaddress must succeed");
    }
    // Past the regtest DIP3 enforcement height (500): below DIP3 activation
    // (432) a special transaction is rejected outright, and between activation
    // and enforcement a ProRegTx must reuse the owner key as the voting key,
    // which a shared registration never does. The same blocks bring the
    // wallets' coinbases to maturity.
    CKey sink_key;
    sink_key.MakeNewKey(/*fCompressed=*/true);
    QVERIFY2(MineTo(m_node, PKHash(sink_key.GetPubKey()), 500).isEmpty(), "maturity blocks must be mined");
    QVERIFY2(ChainHeight(m_node) > Params().GetConsensus().DIP0003EnforcementHeight,
             "the chain must be past DIP3 enforcement before a shared registration can be relayed");

    QVERIFY2(m_node.isV24Active(), "shared masternodes require an active v24 deployment");

    for (const auto& wallet : {coord_wallet, alice_wallet, bob_wallet}) {
        SyncWallet(m_node, *wallet);
        const CAmount balance{wallet::GetBalance(*wallet).m_mine_trusted};
        QVERIFY2(balance >= 700 * COIN,
                 qPrintable(QStringLiteral("wallet %1 only has %2 duffs")
                                .arg(QString::fromStdString(wallet->GetName()))
                                .arg(balance)));
    }

    // Alice sweeps her coins to herself, so her share can only be funded from
    // her own unconfirmed change, which the wallet itself would spend
    {
        const auto dest{alice_wallet->GetNewDestination("sweep")};
        QVERIFY(dest);
        UniValue recipients(UniValue::VARR);
        recipients.push_back(EncodeDestination(*dest));
        UniValue params(UniValue::VOBJ);
        params.pushKV("recipients", recipients);
        UniValue sent;
        const QString error{RunRpc(m_node, "sendall", params, sent, "/wallet/alice")};
        QVERIFY2(error.isEmpty(), qPrintable(QStringLiteral("sendall failed: %1").arg(error)));
    }

    MasternodeTestUtil::GuiModels models{m_node};
    QVERIFY2(models.ok, qPrintable(QString::fromStdString(models.error.translated)));
    WalletModel coord_model(interfaces::MakeWallet(context, coord_wallet), models.client);
    WalletModel alice_model(interfaces::MakeWallet(context, alice_wallet), models.client);
    WalletModel bob_model(interfaces::MakeWallet(context, bob_wallet), models.client);
    // The dialogs spend through the wallet model, which compares against the
    // cached balance the GUI's poll timer maintains.
    for (WalletModel* model : {&coord_model, &alice_model, &bob_model}) {
        model->pollBalanceChanged();
    }

    // --- empty state: no wallet at all -------------------------------------
    {
        SharedMnCreateDialog no_wallet(m_node, /*wallet_model=*/nullptr);
        Shots().capture(&no_wallet, "nowallet", "landing",
                        "Landing page with no wallet loaded: the gate line and the disabled start button");
    }

    // =====================================================================
    // Round 0 - the coordinator drafts the invitation
    // =====================================================================
    SharedMnCreateDialog coord(m_node, &coord_model);
    coord.resize(SHOT_WIDTH, SHOT_HEIGHT);
    coord.show();
    QCOMPARE(int(coord.currentPage()), int(SharedMnCreateDialog::PageLanding));
    Shots().capture(&coord, "coord", "01-landing", "Coordinator landing page: start a session or paste one");
    Shots().captureAt(&coord, "coord", "01-landing-minimum",
                      "Landing page at the dialog's minimum size (700x560), to show what clips",
                      QSize(MIN_WIDTH, MIN_HEIGHT));
    coord.resize(SHOT_WIDTH, SHOT_HEIGHT);

    coord.m_start_button->click();
    QCOMPARE(int(coord.currentPage()), int(SharedMnCreateDialog::PageParticipants));
    QCOMPARE(coord.m_share_table->rowCount(), 2);
    Shots().capture(&coord, "coord", "02-participants-default",
                    "Participants page as the wizard starts it: two unnamed rows splitting the collateral");

    coord.m_add_share_button->click();
    QCOMPARE(coord.m_share_table->rowCount(), 3);

    // Fill one row the way a user does: the name cell is a line edit and the
    // amount cell a real amount field, both widgets inside the table.
    const auto set_share_row = [&coord](int row, const QString& name, CAmount amount) {
        auto* const name_edit{qobject_cast<QLineEdit*>(
            coord.m_share_table->cellWidget(row, SharedMnCreateDialog::SHARE_COL_NAME))};
        auto* const amount_field{qobject_cast<BitcoinAmountField*>(
            coord.m_share_table->cellWidget(row, SharedMnCreateDialog::SHARE_COL_AMOUNT))};
        if (name_edit != nullptr) name_edit->setText(name);
        if (amount_field != nullptr) amount_field->setValue(amount);
    };

    // Deliberately invalid roster first: a duplicate name and a share below the
    // 100 DASH minimum, to capture how the dialog reports them.
    set_share_row(0, QStringLiteral("Coordinator"), 400 * COIN);
    set_share_row(1, QStringLiteral("Coordinator"), 50 * COIN);
    set_share_row(2, QStringLiteral("Bob"), 300 * COIN);
    if (auto* const me = qobject_cast<QRadioButton*>(coord.m_me_group->button(0)); me != nullptr) me->setChecked(true);
    coord.m_next_button->click();
    QCOMPARE(int(coord.currentPage()), int(SharedMnCreateDialog::PageParticipants));
    QVERIFY2(!coord.m_error_label->text().isEmpty(), "a duplicate name must be reported");
    Shots().capture(&coord, "coord", "03-participants-invalid",
                    "Participants page refusing a duplicate name and a 50 DASH share (below the 100 DASH minimum)");

    set_share_row(1, QStringLiteral("Alice"), 300 * COIN);
    set_share_row(2, QStringLiteral("Bob"), 300 * COIN);
    Shots().capture(&coord, "coord", "04-participants",
                    "Participants page with a valid roster: 400 / 300 / 300 and the sum meter at 1000 of 1000");

    coord.m_next_button->click();
    QCOMPARE(int(coord.currentPage()), int(SharedMnCreateDialog::PageSettings));
    QCOMPARE(coord.m_session.shares().size(), size_t{3});
    QCOMPARE(coord.m_session.shares()[0].amount, 400 * COIN);

    // --- masternode settings ------------------------------------------------
    coord.m_service_edit->setText(QStringLiteral("127.0.0.1:19999"));
    QVERIFY2(coord.m_operator_widget->isValid(), "the operator key widget generates a key on construction");
    QVERIFY2(coord.m_operator_widget->hasGeneratedSecret(), "the generated key must gate on the save-secret page");
    coord.m_node_run_by_edit->setText(QStringLiteral("Coordinator"));
    QVERIFY2(!coord.m_voting_edit->text().isEmpty(), "a voting address is filled in on first entry");
    Shots().capture(&coord, "coord", "05-settings",
                    "Masternode settings: service address, generated operator key, node run by, voting address");

    coord.m_next_button->click();
    QCOMPARE(int(coord.currentPage()), int(SharedMnCreateDialog::PageExitTerms));

    // --- exit terms ---------------------------------------------------------
    coord.m_early_period_spin->setValue(30);
    coord.m_early_penalty_field->setValue(5 * COIN);
    Shots().capture(&coord, "coord", "06-exit-terms",
                    "Exit terms: a 30 block early period and a 5 DASH early-exit penalty, with the plain "
                    "language preview");

    coord.m_next_button->click();
    QCOMPARE(int(coord.currentPage()), int(SharedMnCreateDialog::PageContribution));
    QCOMPARE(coord.myShareIndex(), 0);

    // --- the coordinator's own contribution ---------------------------------
    QVERIFY2(!coord.m_owner_edit->text().isEmpty(), "the owner address is filled in on first entry");
    QVERIFY2(!coord.m_refund_edit->text().isEmpty(), "the refund address is filled in on first entry");
    Shots().capture(&coord, "coord", "07-contribution",
                    "Your contribution: owner/refund/reward addresses and the coins that would be used");

    coord.m_reserve_button->click();
    QVERIFY2(coord.myContribution() != nullptr,
             qPrintable(QStringLiteral("Reserve Coins failed: %1").arg(coord.m_error_label->text())));
    {
        const auto* const mine{coord.myContribution()};
        QVERIFY(!mine->inputs.empty());
        for (const auto& input : mine->inputs) {
            const COutPoint outpoint{uint256S(input.txid.toStdString()), input.vout};
            QVERIFY2(coord_model.wallet().isLockedCoin(outpoint), "reserving must lock the contributed coins");
        }
    }
    Shots().capture(&coord, "coord", "08-contribution-reserved",
                    "Your contribution after Reserve Coins: the coins are locked and the addresses are frozen");

    coord.m_next_button->click();
    QCOMPARE(int(coord.currentPage()), int(SharedMnCreateDialog::PageSecret));
    Shots().capture(&coord, "coord", "09-save-operator-key",
                    "Save operator key gate: the generated secret, the dash.conf line and the 4 character "
                    "confirmation");
    const QString operator_secret{coord.m_operator_widget->secretHex()};
    QVERIFY(!operator_secret.isEmpty());
    QVERIFY2(!coord.m_next_button->isEnabled(), "the gate must hold until the secret is confirmed");
    coord.m_confirm_edit->setText(operator_secret.right(4));
    QVERIFY2(coord.secretConfirmed(), "typing the last 4 characters must satisfy the gate");
    coord.m_next_button->click();
    QCOMPARE(int(coord.currentPage()), int(SharedMnCreateDialog::PageInvite));

    // =====================================================================
    // Round 1 - Invitation out, Details back
    // =====================================================================
    Shots().capture(&coord, "coord", "10-invite",
                    "Invite participants: the status board with only the coordinator's row ticked");

    QApplication::clipboard()->clear();
    coord.m_copy_invitation_button->click();
    QString invitation_code;
    const QString invitation{TakeClipboardEnvelope(invitation_code)};
    QVERIFY2(!invitation.isEmpty(), "the invitation envelope must land on the clipboard");
    QVERIFY2(!invitation_code.isEmpty(), "the copied envelope must carry a fingerprint");
    QCOMPARE(coord.m_sent_code, invitation_code);
    QVERIFY(coord.m_status_label->text().contains(invitation_code));
    Shots().capture(&coord, "coord", "11-invite-copied",
                    "Invite participants after Copy Invitation: the confirmation line names session and code");

    SharedMnCreateDialog alice(m_node, &alice_model);
    SharedMnCreateDialog bob(m_node, &bob_model);
    struct Participant {
        SharedMnCreateDialog* dialog;
        WalletModel* model;
        QString role;
        QString who;
        int share_index;
    };
    const std::vector<Participant> participants{{&alice, &alice_model, QStringLiteral("alice"),
                                                 QStringLiteral("Alice"), 1},
                                                {&bob, &bob_model, QStringLiteral("bob"), QStringLiteral("Bob"), 2}};

    QStringList details_replies;
    for (const auto& who : participants) {
        SharedMnCreateDialog& dialog{*who.dialog};
        dialog.resize(SHOT_WIDTH, SHOT_HEIGHT);
        dialog.show();
        dialog.openSharedMessage(invitation);
        QCOMPARE(int(dialog.currentPage()), int(SharedMnCreateDialog::PageContribution));
        QCOMPARE(dialog.m_session.shares().size(), size_t{3});
        Shots().capture(&dialog, who.role, QStringLiteral("12-%1-share-imported").arg(who.role),
                        who.who + " just pasted the invitation: the whole draft is read-only except their own row");

        const int combo_row{dialog.m_who_am_i_combo->findData(who.share_index)};
        QVERIFY(combo_row >= 0);
        dialog.m_who_am_i_combo->setCurrentIndex(combo_row);
        QCOMPARE(dialog.myShareIndex(), who.share_index);
        ShotRecorder::settle();

        Shots().capture(&dialog, who.role, QStringLiteral("13-%1-share-ready").arg(who.role),
                        who.who + " chose their row and is ready to press \"Reserve Coins and Copy Details\"");

        // One press of the primary button reserves the coins and copies the
        // reply: the page never asks the user to reserve first.
        QApplication::clipboard()->clear();
        dialog.m_next_button->click();
        QVERIFY2(int(dialog.currentPage()) == int(SharedMnCreateDialog::PageWaitTerms),
                 qPrintable(QStringLiteral("%1 could not reserve and copy in one step: %2")
                                .arg(who.who, dialog.m_error_label->text())));
        QVERIFY2(dialog.myContribution() != nullptr,
                 qPrintable(QStringLiteral("%1 has no recorded contribution: %2")
                                .arg(who.who, dialog.m_error_label->text())));
        for (const auto& input : dialog.myContribution()->inputs) {
            const COutPoint outpoint{uint256S(input.txid.toStdString()), input.vout};
            QVERIFY2(who.model->wallet().isLockedCoin(outpoint),
                     "the primary button must lock the coins it reserved");
        }
        QString reply_code;
        const QString reply{TakeClipboardEnvelope(reply_code)};
        QVERIFY2(!reply.isEmpty(), qPrintable(who.who + " must copy a Details reply"));
        QVERIFY(!reply_code.isEmpty());
        details_replies << reply;

        // The waiting page copies the same reply again for a lost clipboard
        QApplication::clipboard()->clear();
        const auto waiting{std::find_if(dialog.m_waiting_pages.begin(), dialog.m_waiting_pages.end(),
                                        [](const auto& entry) {
                                            return entry.first == SharedMnCreateDialog::PageWaitTerms;
                                        })};
        QVERIFY(waiting != dialog.m_waiting_pages.end());
        waiting->second.copy->click();
        QCOMPARE(QApplication::clipboard()->text(), reply);
        Shots().capture(&dialog, who.role, QStringLiteral("14-%1-waiting-for-terms").arg(who.role),
                        who.who + " waiting for the coordinator: what was sent and what comes next");
    }
    for (const auto& input : alice.myContribution()->inputs) {
        const COutPoint outpoint{uint256S(input.txid.toStdString()), input.vout};
        QCOMPARE(alice_model.wallet().getCoins({outpoint}).front().depth_in_main_chain, 0);
    }

    for (int i = 0; i < details_replies.size(); ++i) {
        coord.handleImportedText(details_replies.at(i));
        QVERIFY2(coord.m_error_label->text().isEmpty(),
                 qPrintable(QStringLiteral("merging a Details reply failed: %1").arg(coord.m_error_label->text())));
        Shots().capture(&coord, "coord", QStringLiteral("16-invite-after-reply%1").arg(i + 1),
                        QStringLiteral("Coordinator board after reply %1 of 2 was pasted").arg(i + 1));
    }
    QCOMPARE(coord.m_session.contributions().size(), size_t{3});
    QVERIFY2(coord.allDetailsCollected(), "every share must have details and funding before the terms can be locked");
    QVERIFY(!coord.m_boards.empty());
    for (int row = 0; row < 3; ++row) {
        QCOMPARE(int(coord.m_boards.front().second->cellState(row, 0)), int(SharedMnStatusBoard::State::Done));
        QCOMPARE(int(coord.m_boards.front().second->cellState(row, 1)), int(SharedMnStatusBoard::State::Done));
    }

    // =====================================================================
    // Round 2 - Locked Terms out, Approvals back
    // =====================================================================
    coord.m_next_button->click();
    QCOMPARE(int(coord.currentPage()), int(SharedMnCreateDialog::PageInvite));
    QVERIFY2(coord.m_lock_confirm_card->isVisible(), "the lock confirmation replaces the page in place");
    Shots().capture(&coord, "coord", "17-lock-confirm",
                    "Lock confirmation: the full term sheet and the funding check, in place on the invite page");

    coord.m_next_button->click();
    WAIT_UNTIL_IDLE(coord.m_busy);
    ShotRecorder::settle();
    QVERIFY2(int(coord.currentPage()) == int(SharedMnCreateDialog::PageApprovals),
             qPrintable(QStringLiteral("Lock and Approve did not reach the Approvals page. Error: \"%1\"; modals: %2")
                            .arg(coord.m_error_label->text(), pilot.seen().join(QStringLiteral(" | ")))));
    QVERIFY2(coord.m_session.stage() == MnShareSession::Stage::Frozen ||
                 coord.m_session.stage() == MnShareSession::Stage::Signing,
             qPrintable(QStringLiteral("unexpected stage after locking: %1")
                            .arg(MnShareSession::StageName(coord.m_session.stage()))));
    if (coord.m_session.signedCount() == 0) {
        // goToPage() clears the error label, so the reason the automatic own
        // approval failed never reaches the user; ask for it again here.
        QString sign_error;
        coord.signOwnConsent([&sign_error](bool, const QString& error) { sign_error = error; });
        WAIT_UNTIL_IDLE(coord.m_busy);
        QVERIFY2(coord.m_session.signedCount() >= 1,
                 qPrintable(QStringLiteral("locking did not produce the coordinator's own approval: %1")
                                .arg(sign_error)));
    }
    QCOMPARE(coord.m_session.signedCount(), 1);
    Shots().capture(&coord, "coord", "18-approvals",
                    "Approvals: the locked term sheet, the board with only the coordinator approved");

    QApplication::clipboard()->clear();
    coord.m_copy_terms_button->click();
    QString terms_code;
    const QString locked_terms{TakeClipboardEnvelope(terms_code)};
    QVERIFY(!locked_terms.isEmpty());
    QVERIFY(!terms_code.isEmpty());

    QStringList approvals;
    for (const auto& who : participants) {
        SharedMnCreateDialog& dialog{*who.dialog};
        dialog.handleImportedText(locked_terms);
        QVERIFY2(int(dialog.currentPage()) == int(SharedMnCreateDialog::PageApprovals),
                 qPrintable(QStringLiteral("%1 did not reach Approve terms: %2")
                                .arg(who.who, dialog.m_error_label->text())));
        Shots().capture(&dialog, who.role, QStringLiteral("19-%1-approve-terms").arg(who.role),
                        who.who + " reading the locked terms before approving them");

        QApplication::clipboard()->clear();
        dialog.m_next_button->click();
        WAIT_UNTIL_IDLE(dialog.m_busy);
        QVERIFY2(int(dialog.currentPage()) == int(SharedMnCreateDialog::PageWaitSigning),
                 qPrintable(QStringLiteral("%1 could not approve: %2").arg(who.who, dialog.m_error_label->text())));
        QString code;
        const QString approval{TakeClipboardEnvelope(code)};
        QVERIFY2(!approval.isEmpty(), qPrintable(who.who + " must copy an Approval reply"));
        approvals << approval;
        Shots().capture(&dialog, who.role, QStringLiteral("20-%1-waiting-for-signing").arg(who.role),
                        who.who + " waiting for the signing request");
    }

    coord.handleImportedText(approvals.at(0));
    QVERIFY2(coord.m_error_label->text().isEmpty(), qPrintable(coord.m_error_label->text()));
    QCOMPARE(coord.m_session.signedCount(), 2);
    Shots().capture(&coord, "coord", "21-approvals-partial",
                    "Coordinator with two of three approvals in: the board summary counts them");

    // Absorbing the last approval also combines them and signs this wallet's
    // own funding inputs, with no further decision to make.
    coord.handleImportedText(approvals.at(1));
    WAIT_UNTIL_IDLE(coord.m_busy);
    ShotRecorder::settle();
    QVERIFY2(int(coord.currentPage()) == int(SharedMnCreateDialog::PageSignatures),
             qPrintable(QStringLiteral("combine/sign did not advance to Signatures. Error: \"%1\"")
                            .arg(coord.m_error_label->text())));
    QCOMPARE(int(coord.m_session.stage()), int(MnShareSession::Stage::Combined));

    // =====================================================================
    // Round 3 - Signing Request out, Signed Contributions back
    // =====================================================================
    Shots().capture(&coord, "coord", "22-signatures",
                    "Signatures: the board's Signed column with only the coordinator's own inputs done");

    QApplication::clipboard()->clear();
    coord.m_copy_signing_button->click();
    QString request_code;
    const QString signing_request{TakeClipboardEnvelope(request_code)};
    QVERIFY(!signing_request.isEmpty());
    QVERIFY(!request_code.isEmpty());

    QStringList signed_contributions;
    for (const auto& who : participants) {
        SharedMnCreateDialog& dialog{*who.dialog};
        dialog.handleImportedText(signing_request);
        QVERIFY2(int(dialog.currentPage()) == int(SharedMnCreateDialog::PageSignatures),
                 qPrintable(QStringLiteral("%1 did not reach Sign contribution: %2")
                                .arg(who.who, dialog.m_error_label->text())));
        // The last step before coins move shows the approved terms again
        QVERIFY(dialog.m_signatures_unchanged->isVisibleTo(&dialog));
        QVERIFY(dialog.m_signatures_terms->text().contains(dialog.m_session.shares()[dialog.m_my_share].ownerAddress));
        QVERIFY(dialog.m_signatures_terms->text().contains("Your contribution"));
        Shots().capture(&dialog, who.role, QStringLiteral("23-%1-sign-contribution").arg(who.role),
                        who.who + " about to sign the coins they reserved");

        QApplication::clipboard()->clear();
        dialog.m_next_button->click();
        QVERIFY2(int(dialog.currentPage()) == int(SharedMnCreateDialog::PageWaitBroadcast),
                 qPrintable(QStringLiteral("%1 could not sign: %2").arg(who.who, dialog.m_error_label->text())));
        QString code;
        const QString reply{TakeClipboardEnvelope(code)};
        QVERIFY2(!reply.isEmpty(), qPrintable(who.who + " must copy a Signed Contribution"));
        signed_contributions << reply;
        Shots().capture(&dialog, who.role, QStringLiteral("24-%1-waiting-for-broadcast").arg(who.role),
                        who.who + " waiting for the coordinator to broadcast");
    }

    for (const QString& reply : signed_contributions) {
        coord.handleImportedText(reply);
        QVERIFY2(coord.m_error_label->text().isEmpty(),
                 qPrintable(QStringLiteral("merging a Signed Contribution failed: %1")
                                .arg(coord.m_error_label->text())));
    }
    QVERIFY2(coord.allFundingSigned(),
             qPrintable(QStringLiteral("every funding input should be signed, stage is %1")
                            .arg(MnShareSession::StageName(coord.m_session.stage()))));
    Shots().capture(&coord, "coord", "25-signatures-complete",
                    "Signatures complete: the primary button is now Broadcast Registration");

    // --- broadcast ----------------------------------------------------------
    pilot.captureNext("coord", "26-broadcast-confirm",
                      "The send confirmation shown before the registration is broadcast");
    coord.m_next_button->click();
    ShotRecorder::settle();
    QVERIFY2(int(coord.currentPage()) == int(SharedMnCreateDialog::PageComplete),
             qPrintable(QStringLiteral("broadcast did not complete. Error: \"%1\"; modals: %2")
                            .arg(coord.m_error_label->text(), pilot.seen().join(QStringLiteral(" | ")))));
    // Show Details carries the whole registration, not only the codes naming it
    {
        const auto confirm{std::find_if(pilot.seen().begin(), pilot.seen().end(), [](const QString& text) {
            return text.startsWith(QStringLiteral("Broadcast this shared masternode registration?"));
        })};
        QVERIFY2(confirm != pilot.seen().end(), qPrintable(pilot.seen().join(QStringLiteral(" | "))));
        QVERIFY(confirm->contains(QStringLiteral("Transaction ID: ")));
        QVERIFY(confirm->contains(QStringLiteral("\n1. ") + coord.m_session.shares()[0].label));
        for (const auto& share : coord.m_session.shares()) {
            QVERIFY(confirm->contains(share.ownerAddress));
        }
    }
    Shots().capture(&coord, "coord", "27-complete",
                    "Complete: the proTxHash, the \"what to keep safe\" checklist and the next steps");

    const QString protx_hash{coord.m_complete_hash->text().trimmed()};
    QVERIFY2(!protx_hash.isEmpty(), "the complete page must show the proTxHash");

    // The coordinator's courtesy Final Record moves a participant to Complete
    QApplication::clipboard()->clear();
    coord.m_copy_record_button->click();
    const QString final_record{QApplication::clipboard()->text()};
    QVERIFY(!final_record.isEmpty());
    alice.handleImportedText(final_record);
    QCOMPARE(int(alice.currentPage()), int(SharedMnCreateDialog::PageComplete));
    Shots().capture(&alice, "alice", "28-alice-complete",
                    "Alice's Complete page after pasting the coordinator's Final Record");

    // The registration spends every contributed coin, so the reservations must
    // not outlive it in the wallets' persisted locked sets - the user would
    // have to hunt them down in Coin Control
    for (const auto& [dialog, model] : {std::pair<SharedMnCreateDialog*, WalletModel*>{&coord, &coord_model},
                                        {&alice, &alice_model}}) {
        for (const auto& contribution : dialog->m_session.contributions()) {
            for (const auto& input : contribution.inputs) {
                const COutPoint outpoint{uint256S(input.txid.toStdString()), input.vout};
                QVERIFY2(!model->wallet().isLockedCoin(outpoint),
                         "a broadcast registration must release the coins it spent");
            }
        }
    }

    // =====================================================================
    // On chain
    // =====================================================================
    QVERIFY2(MineTo(m_node, PKHash(sink_key.GetPubKey()), 1).isEmpty(), "the registration block must be mined");
    {
        UniValue params(UniValue::VOBJ);
        params.pushKV("type", "registered");
        params.pushKV("detailed", true);
        UniValue listed;
        const QString rpc_error{RunRpc(m_node, "protx list", params, listed)};
        QVERIFY2(rpc_error.isEmpty(), qPrintable(QStringLiteral("protx list failed: %1").arg(rpc_error)));
        bool found{false};
        for (const auto& entry : listed.getValues()) {
            const UniValue& hash{entry.find_value("proTxHash")};
            if (hash.isStr() && QString::fromStdString(hash.get_str()) == protx_hash) found = true;
        }
        QVERIFY2(found, qPrintable(QStringLiteral("proTxHash %1 is not in protx list: %2")
                                       .arg(protx_hash, QString::fromStdString(listed.write()))));
        qInfo("shared masternode confirmed at height %d, proTxHash %s", ChainHeight(m_node), qPrintable(protx_hash));
    }

    // =====================================================================
    // The masternode list
    // =====================================================================
    for (WalletModel* model : {&coord_model, &alice_model, &bob_model}) {
        model->pollBalanceChanged();
    }
    SyncWallet(m_node, *coord_wallet);

    MasternodeList list;
    list.setWalletModel(&coord_model);
    list.setClientModel(&models.client);
    auto* const feed{models.client.feedMasternode()};
    QVERIFY(feed != nullptr);
    feed->fetch();
    QMetaObject::invokeMethod(&list, "updateMasternodeList", Qt::DirectConnection);
    ShotRecorder::settle();

    const auto feed_data{feed->data()};
    QVERIFY(feed_data != nullptr);
    std::shared_ptr<MasternodeEntry> shared_entry;
    for (const auto& entry : feed_data->m_entries) {
        if (entry->proTxHash() == protx_hash) shared_entry = entry;
    }
    QVERIFY2(shared_entry != nullptr, "the registered masternode must be in the list feed");
    QVERIFY2(shared_entry->isShared(), "the registered masternode must be recognised as shared");
    QCOMPARE(shared_entry->shares().size(), size_t{3});

    auto* const type_combo{list.findChild<QComboBox*>("comboBoxType")};
    QVERIFY(type_combo != nullptr);
    type_combo->setCurrentIndex(static_cast<int>(MasternodeListSortFilterProxyModel::TypeFilter::Shared));
    list.resize(1100, 640);
    ShotRecorder::settle();
    Shots().captureAt(&list, "masternodes", "29-tab-shared",
                      "Masternodes tab filtered to Shared, showing the masternode just registered", QSize(1100, 640));

    auto* const table{list.findChild<QTableView*>("tableViewMasternodes")};
    QVERIFY(table != nullptr);
    QVERIFY2(table->model() != nullptr && table->model()->rowCount() >= 1,
             "the Shared filter must show the registered shared masternode");

    // Details: the tooltip/extra-info HTML, rendered the way the dialog does
    {
        QSet<int> my_shares;
        for (size_t i = 0; i < shared_entry->shares().size(); ++i) {
            if (coord_model.wallet().isSpendable(PKHash(shared_entry->shares()[i].keyIDOwner))) {
                my_shares.insert(static_cast<int>(i));
            }
        }
        QVERIFY2(!my_shares.isEmpty(), "the coordinator wallet holds its own share owner key");
        QScrollArea details;
        auto* const rendered{new QLabel(&details)};
        rendered->setTextFormat(Qt::RichText);
        rendered->setWordWrap(true);
        rendered->setMargin(12);
        rendered->setAlignment(Qt::AlignTop | Qt::AlignLeft);
        rendered->setText(shared_entry->toHtml(ChainHeight(m_node), my_shares, SharedMnDisplayUnit(&coord_model)));
        details.setWidget(rendered);
        details.setWidgetResizable(true);
        Shots().captureAt(&details, "masternodes", "30-details-html",
                          "The Details view's HTML for the shared masternode, with the share table",
                          QSize(760, 900));
    }

    // Context menu: opened the way a right click opens it, then grabbed
    {
        table->selectRow(0);
        QMenu* const menu{list.findChild<QMenu*>(QString{}, Qt::FindDirectChildrenOnly)};
        QVERIFY(menu != nullptr);
        QTimer::singleShot(150, &list, [menu] {
            if (menu->isVisible()) {
                Shots().capture(menu, "masternodes", "31-context-menu",
                                "The masternode list context menu on the shared masternode",
                                /*fixed_size=*/false);
            }
            menu->close();
        });
        const QRect rect{table->visualRect(table->model()->index(0, 0))};
        QMetaObject::invokeMethod(&list, "showContextMenuDIP3", Qt::DirectConnection, Q_ARG(QPoint, rect.center()));
        ShotRecorder::settle();
    }

    // =====================================================================
    // The maintenance dialogs, against the masternode that was just created
    // =====================================================================
    // A dissolution spending the collateral in the block that created it would
    // be rejected, so the dialog refuses to build its tabs until the
    // registration has one confirmation. Capture that state, then mine it away.
    {
        DissolveDialog too_early(m_node, &coord_model, *shared_entry, shared_entry->registeredHeight());
        Shots().capture(&too_early, "maintenance", "32-dissolve-too-early",
                        "Dissolve opened in the registration block: it asks for one confirmation first",
                        /*fixed_size=*/false);
        QVERIFY2(too_early.m_tabs == nullptr, "no dissolution is offered before the first confirmation");
    }
    QVERIFY2(MineTo(m_node, PKHash(sink_key.GetPubKey()), 1).isEmpty(), "one confirmation must be mined");
    feed->fetch();
    QMetaObject::invokeMethod(&list, "updateMasternodeList", Qt::DirectConnection);
    ShotRecorder::settle();
    {
        const auto refreshed{feed->data()};
        QVERIFY(refreshed != nullptr);
        for (const auto& entry : refreshed->m_entries) {
            if (entry->proTxHash() == protx_hash) shared_entry = entry;
        }
    }
    {
        UniValue params(UniValue::VOBJ);
        params.pushKV("proTxHash", protx_hash.toStdString());
        UniValue info;
        const QString info_error{RunRpc(m_node, "protx info", params, info)};
        QVERIFY2(info_error.isEmpty(),
                 qPrintable(QStringLiteral("protx info %1 failed: %2").arg(protx_hash, info_error)));
    }
    const int height{ChainHeight(m_node)};
    {
        UpdateShareDialog dialog(m_node, &coord_model, *shared_entry);
        Shots().capture(&dialog, "maintenance", "33-update-share",
                          "Change Reward Address for one share of the registered masternode", /*fixed_size=*/false);
        QVERIFY(dialog.m_share_combo != nullptr);
        QVERIFY2(dialog.m_share_combo->count() >= 1, "the wallet's own share must be offered");
        dialog.m_use_new_button->click();
        ShotRecorder::settle();
        QVERIFY2(!dialog.m_reward_edit->text().isEmpty(), "Use new address must fill the reward field");
        Shots().capture(&dialog, "maintenance", "34-update-share-filled",
                          "Change Reward Address with a fresh address filled in", /*fixed_size=*/false);
    }
    {
        DissolveDialog dialog(m_node, &coord_model, *shared_entry, height);
        QVERIFY(dialog.m_tabs != nullptr);

        dialog.m_tabs->setCurrentIndex(0);
        ShotRecorder::settle();
        QVERIFY2(dialog.m_now_table != nullptr && dialog.m_now_table->rowCount() > 0,
                 qPrintable(QStringLiteral("the dissolve-now preview is empty: %1")
                                .arg(dialog.m_now_error_label == nullptr ? QString{}
                                                                         : dialog.m_now_error_label->text())));
        Shots().capture(&dialog, "maintenance", "35-dissolve-natural-size",
                        "Dissolve dialog at the size it opens itself at: everything down to the submit button "
                        "is on screen",
                        /*fixed_size=*/false);
        Shots().capture(&dialog, "maintenance", "35-dissolve-now",
                          "Dissolve now: the unilateral payout preview with the early-exit penalty",
                          /*fixed_size=*/false);

        dialog.m_tabs->setCurrentIndex(1);
        ShotRecorder::settle();
        Shots().capture(&dialog, "maintenance", "36-dissolve-together",
                          "Dissolve together, before the request is prepared", /*fixed_size=*/false);
        dialog.m_un_prepare->click();
        WAIT_UNTIL_IDLE(dialog.isBusy());
        ShotRecorder::settle();
        QVERIFY2(dialog.m_un_collector->hasTransaction(),
                 qPrintable(QStringLiteral("Prepare Request produced no transaction: %1")
                                .arg(dialog.m_un_status->text())));
        QVERIFY2(!dialog.m_un_collector->signHashHex().isEmpty(), "the signing digest must be recomputed natively");
        Shots().capture(&dialog, "maintenance", "37-dissolve-together-prepared",
                          "Dissolve together after Prepare Request: the envelope and the approval board",
                          /*fixed_size=*/false);

        dialog.selectStandbyTab();
        ShotRecorder::settle();
        QCOMPARE(dialog.m_tabs->currentIndex(), dialog.m_standby_tab_index);
        Shots().capture(&dialog, "maintenance", "38-standby-empty",
                          "Standby dissolution tab before anything is generated", /*fixed_size=*/false);
        // The save dialog cannot run headless: the pilot cancels it, then the
        // file is written where the test can check it.
        dialog.m_sb_create->click();
        WAIT_UNTIL_IDLE(dialog.isBusy());
        ShotRecorder::settle();
        QVERIFY2(!dialog.m_sb_hex_full.isEmpty() && !dialog.m_sb_hex_immediate.isEmpty(),
                 qPrintable(QStringLiteral("no standby dissolutions were generated: %1")
                                .arg(dialog.m_sb_status->text())));
        QTemporaryDir standby_dir;
        QVERIFY(standby_dir.isValid());
        const QString standby_path{standby_dir.filePath(dialog.standbyFileName())};
        QString write_error;
        QVERIFY2(dialog.writeStandbyFile(standby_path, write_error), qPrintable(write_error));
        QVERIFY(QFile::exists(standby_path));
        dialog.recordStandbySaved();
        ShotRecorder::settle();
        Shots().capture(&dialog, "maintenance", "39-standby-created",
                          "Standby dissolution tab with both offline transactions generated and stored",
                          /*fixed_size=*/false);
    }
    {
        RotateSharedKeysDialog dialog(m_node, &coord_model, *shared_entry);
        Shots().capture(&dialog, "maintenance", "40-rotate-keys",
                          "Rotate operator or voting key, before a request is prepared", /*fixed_size=*/false);
        dialog.m_operator_edit->setText(FreshOperatorPubKey());
        ShotRecorder::settle();
        dialog.m_prepare_button->click();
        WAIT_UNTIL_IDLE(dialog.isBusy());
        ShotRecorder::settle();
        QVERIFY2(dialog.m_collector->hasTransaction(),
                 qPrintable(QStringLiteral("Prepare Request produced no transaction: %1")
                                .arg(dialog.m_status_label->text())));
        Shots().capture(&dialog, "maintenance", "41-rotate-keys-prepared",
                          "Rotate keys after Prepare Request: the approval board and the finish banner",
                          /*fixed_size=*/false);
    }
}
