// Copyright (c) 2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/test/sharedmnwidgettests.h>

#include <qt/test/masternodetestutil.h>

#include <qt/mnsharesession.h>
#include <qt/sharedmnwidgets.h>

#include <chainparams.h>
#include <consensus/amount.h>
#include <core_io.h>
#include <key.h>
#include <key_io.h>
#include <primitives/transaction.h>
#include <script/standard.h>
#include <test/util/setup_common.h>

#include <univalue.h>

#include <QLabel>

#include <optional>
#include <string>
#include <vector>

using MasternodeTestUtil::FreshP2PKHAddress;

namespace {
//! A draft session with two named shares and one recorded contribution
MnShareSession TwoShareSession()
{
    MnShareSession session;
    MnShareSession::Share alice;
    alice.label = "alice";
    alice.amount = 600 * COIN;
    alice.ownerAddress = FreshP2PKHAddress();
    alice.refundAddress = FreshP2PKHAddress();
    session.shares().push_back(alice);

    // A participant name is written by somebody else and is never trusted
    MnShareSession::Share bob;
    bob.label = "<b>bob</b>";
    bob.amount = 400 * COIN;
    bob.ownerAddress = FreshP2PKHAddress();
    bob.refundAddress = FreshP2PKHAddress();
    bob.rewardAddress = FreshP2PKHAddress();
    session.shares().push_back(bob);

    session.terms().votingAddress = FreshP2PKHAddress();
    session.terms().earlyPeriodBlocks = 5000;
    session.terms().earlyPenalty = 5 * COIN;
    session.setOperatorSecretHolder("alice");

    MnShareSession::Contribution contribution;
    contribution.label = "alice";
    MnShareSession::Input input;
    input.txid = QString(64, QChar::fromLatin1('1'));
    contribution.inputs.push_back(input);
    contribution.hasChange = true;
    contribution.changeAddress = FreshP2PKHAddress();
    contribution.changeAmount = COIN;
    QString error;
    session.addContribution(contribution, error);
    return session;
}

QString TransactionHex(uint16_t type)
{
    CMutableTransaction tx;
    tx.nVersion = 3;
    tx.nType = type;
    tx.vin.emplace_back(COutPoint(uint256::ONE, 0));
    tx.vout.emplace_back(1000 * COIN, CScript() << OP_TRUE);
    return QString::fromStdString(EncodeHexTx(CTransaction(tx)));
}
} // anonymous namespace

void SharedMnWidgetTests::importDetection()
{
    BasicTestingSetup setup{CBaseChainParams::REGTEST};

    // A session envelope is recognised from its own type field
    const MnShareSession session{TwoShareSession()};
    const auto detected_session{SharedMnImport::Detect(session.toJsonString())};
    QCOMPARE(int(detected_session.kind), int(SharedMnImport::Kind::Session));
    QVERIFY(detected_session.error.isEmpty());

    // A maintenance signing envelope carries the masternode and the operation
    UniValue sigs(UniValue::VOBJ);
    sigs.pushKV("type", "dash-shared-mn-sigs");
    sigs.pushKV("version", 1);
    sigs.pushKV("network", Params().NetworkIDString());
    sigs.pushKV("kind", "dissolve");
    sigs.pushKV("proTxHash", std::string(64, 'a'));
    shared_mn::AppendFingerprint(sigs);
    const auto detected_sigs{SharedMnImport::Detect(QString::fromStdString(sigs.write(2)))};
    QCOMPARE(int(detected_sigs.kind), int(SharedMnImport::Kind::Sigs));
    QCOMPARE(detected_sigs.sigKind, QString("dissolve"));
    QCOMPARE(detected_sigs.proTxHash, QString(64, QChar::fromLatin1('a')));

    // A standby dissolution arrives as raw transaction hex
    const auto detected_standby{SharedMnImport::Detect(TransactionHex(TRANSACTION_PROVIDER_DISSOLVE))};
    QCOMPARE(int(detected_standby.kind), int(SharedMnImport::Kind::StandbyHex));
    // Surrounding whitespace from a clipboard round trip is tolerated
    QCOMPARE(int(SharedMnImport::Detect("  " + TransactionHex(TRANSACTION_PROVIDER_DISSOLVE) + "\n ").kind),
             int(SharedMnImport::Kind::StandbyHex));

    // Anything else is Unknown with one sentence explaining why
    for (const QString& text : {QString(), QString("   "), QString("not json, not hex"), QString("{\"type\":\"other\"}"),
                                TransactionHex(TRANSACTION_PROVIDER_REGISTER)}) {
        const auto unknown{SharedMnImport::Detect(text)};
        QVERIFY2(unknown.kind == SharedMnImport::Kind::Unknown, qPrintable(text));
        QVERIFY2(!unknown.error.isEmpty(), qPrintable(text));
    }
}

void SharedMnWidgetTests::statusBoardCells()
{
    SharedMnStatusBoard board;
    board.setColumns({QStringLiteral("Details"), QStringLiteral("Funded"), QStringLiteral("Approved"),
                      QStringLiteral("Signed")});
    board.setShares({{QStringLiteral("alice"), 600 * COIN, QString()}, {QStringLiteral("bob"), 400 * COIN, QString()}});
    QCOMPARE(board.rowCount(), 2);
    QCOMPARE(board.columnCount(), 4);
    QCOMPARE(int(board.cellState(1, 3)), int(SharedMnStatusBoard::State::Pending));

    board.setCell(0, 0, SharedMnStatusBoard::State::Done);
    board.setCell(1, 0, SharedMnStatusBoard::State::Problem, QStringLiteral("a coin was spent"));
    QCOMPARE(int(board.cellState(0, 0)), int(SharedMnStatusBoard::State::Done));
    QCOMPARE(int(board.cellState(1, 0)), int(SharedMnStatusBoard::State::Problem));
    QCOMPARE(int(board.cellState(0, 1)), int(SharedMnStatusBoard::State::Pending));

    // Out-of-range writes and reads are ignored rather than crashing
    board.setCell(5, 0, SharedMnStatusBoard::State::Done);
    board.setCell(0, 9, SharedMnStatusBoard::State::Done);
    QCOMPARE(int(board.cellState(-1, 0)), int(SharedMnStatusBoard::State::Pending));
    QCOMPARE(int(board.cellState(0, 9)), int(SharedMnStatusBoard::State::Pending));
    QCOMPARE(board.rowName(-1), QString());
    QCOMPARE(board.rowName(5), QString());

    // The wallet's own row is marked, and only that row
    QCOMPARE(board.rowName(0), QString("alice"));
    board.setYouRow(1);
    QCOMPARE(board.rowName(0), QString("alice"));
    QVERIFY(board.rowName(1).startsWith("bob"));
    QVERIFY(board.rowName(1).contains("(you)"));

    // Changing the columns resets the cells but keeps the rows
    board.setColumns({QStringLiteral("Approved")});
    QCOMPARE(board.columnCount(), 1);
    QCOMPARE(board.rowCount(), 2);
    QCOMPARE(int(board.cellState(0, 0)), int(SharedMnStatusBoard::State::Pending));
    QVERIFY(board.rowName(1).contains("(you)"));
}

void SharedMnWidgetTests::termSheetWarnsWhenWePayMoreThanOurShare()
{
    BasicTestingSetup setup{CBaseChainParams::REGTEST};
    const MnShareSession session{TwoShareSession()};
    const CAmount share{session.shares()[1].amount};
    const auto sheet = [&](int you, std::optional<SharedMnOwnFunding> funding) {
        return SharedMnTermSheetHtml(session, you, BitcoinUnits::Unit::DASH, {}, funding);
    };
    const auto paying = [&](CAmount paid) {
        return SharedMnOwnFunding{.spent = paid + 5 * COIN, .change = 5 * COIN, .unvalued = 0};
    };

    // Without a share of our own, or without the wallet's view, there is
    // nothing of ours to show
    QVERIFY(!sheet(1, std::nullopt).contains("Your contribution"));
    QVERIFY(!sheet(-1, paying(share)).contains("Your contribution"));

    // Up to 1 DASH over the share is only shown; signing still refuses a
    // participant who would lose more than its share
    const QString fair{sheet(1, paying(share + COIN))};
    QVERIFY(fair.contains("Your contribution"));
    QVERIFY(fair.contains(SharedMnFormatAmount(BitcoinUnits::Unit::DASH, share + COIN)));
    QVERIFY(!fair.contains("⚠"));

    // Anything more is called out before the reader approves
    QVERIFY(sheet(1, paying(share + COIN + 1)).contains("⚠"));

    // Coins that cannot be valued yet, or none at all, are no reason to warn,
    // but the coins that can be valued may already be too much
    QVERIFY(!sheet(1, SharedMnOwnFunding{.spent = 0, .change = 0, .unvalued = 1}).contains("⚠"));
    QVERIFY(!sheet(1, SharedMnOwnFunding{}).contains("⚠"));
    QVERIFY(sheet(1, SharedMnOwnFunding{.spent = share + 2 * COIN, .change = 0, .unvalued = 1}).contains("⚠"));
}

void SharedMnWidgetTests::termSheetEscapesUntrustedText()
{
    BasicTestingSetup setup{CBaseChainParams::REGTEST};
    const MnShareSession session{TwoShareSession()};
    const QString html{SharedMnTermSheetHtml(session, /*you_share_index=*/1, BitcoinUnits::Unit::DASH)};

    // A participant name is data, not markup: it must not be able to inject a
    // tag into the sheet everybody reads before approving
    QVERIFY(html.contains("&lt;b&gt;bob&lt;/b&gt;"));
    QVERIFY(!html.contains("<b>bob</b>"));
    QVERIFY(html.contains("alice"));

    // Every card is present, with the codes that identify this exact message
    QVERIFY(html.contains(session.sessionCode()));
    QVERIFY(html.contains(session.fingerprint()));
    QVERIFY(html.contains(session.shares()[0].ownerAddress));
    QVERIFY(html.contains(session.shares()[1].refundAddress));
    QVERIFY(html.contains(session.shares()[1].rewardAddress));
    QVERIFY(html.contains(MnShareSession::HumanEarlyPeriod(session.terms().earlyPeriodBlocks)));
    QVERIFY(html.contains("(you)"));

    // With no share of our own, nothing is marked as ours; a share with no
    // reward address falls back to the refund address
    const QString anonymous{SharedMnTermSheetHtml(session, /*you_share_index=*/-1, BitcoinUnits::Unit::DASH)};
    QVERIFY(!anonymous.contains("(you)"));
    QVERIFY(anonymous.contains("same as refund"));

    // Only the addresses this wallet holds are marked as in it
    QVERIFY(!html.contains("in this wallet"));
    const QString ours{session.shares()[1].refundAddress};
    const QString marked{SharedMnTermSheetHtml(session, /*you_share_index=*/1, BitcoinUnits::Unit::DASH,
                                               [&](const QString& address) { return address == ours; })};
    QCOMPARE(marked.count("in this wallet"), 1);
    QVERIFY(marked.contains(ours + QStringLiteral(" <i>")));

    // The status board's two text lines name participants too, and are plain
    // text rather than the QLabel default of Qt::AutoText
    SharedMnStatusBoard board;
    board.setColumns({QStringLiteral("Details")});
    board.setShares({{session.shares()[0].label, session.shares()[0].amount, QString()},
                     {session.shares()[1].label, session.shares()[1].amount, QString()}});
    board.setLastReceived(QStringLiteral("Received %1's Details · Code AB12-CD34").arg(session.shares()[1].label));
    board.setSummary(QStringLiteral("1 of 2 answered"));
    QVERIFY(board.lastReceived().contains("<b>bob</b>"));
    QCOMPARE(board.rowName(1), session.shares()[1].label);
    const auto labels{board.findChildren<QLabel*>()};
    QVERIFY(!labels.isEmpty());
    for (const QLabel* const label : labels) {
        QCOMPARE(int(label->textFormat()), int(Qt::PlainText));
    }
}
