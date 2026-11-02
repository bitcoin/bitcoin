// Copyright (c) 2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/test/mnsharesessiontests.h>

#include <qt/test/masternodetestutil.h>

#include <qt/mnsharesession.h>

#include <bls/bls.h>
#include <chainparams.h>
#include <consensus/amount.h>
#include <core_io.h>
#include <evo/dmn_types.h>
#include <evo/providertx.h>
#include <evo/sharedcollateral.h>
#include <evo/specialtx.h>
#include <key.h>
#include <key_io.h>
#include <messagesigner.h>
#include <primitives/transaction.h>
#include <test/util/setup_common.h>
#include <util/strencodings.h>

#include <optional>
#include <string>
#include <vector>

namespace {
using MasternodeTestUtil::FakeTxid;
using MasternodeTestUtil::DraftReply;
using MasternodeTestUtil::PrepareRegistration;
using MasternodeTestUtil::PreparedRegistration;
using MasternodeTestUtil::FreshP2PKHAddress;

MnShareSession::Share MakeShare(CAmount amount, const QString& owner, const QString& refund,
                                const QString& reward = {})
{
    MnShareSession::Share share;
    share.amount = amount;
    share.ownerAddress = owner;
    share.refundAddress = refund;
    share.rewardAddress = reward;
    return share;
}

//! A session whose share table passes every draft-side check, funded the way a
//! draft is once every participant has replied: each share contributes its own
//! coins, so the session carries a complete funding transaction
MnShareSession ValidSession(std::vector<CKey>& owner_keys)
{
    MnShareSession session;
    owner_keys.resize(3);
    CKey dummy;
    const char* labels[]{"alice", "bob", "carol"};
    const CAmount amounts[]{400 * COIN, 350 * COIN, 250 * COIN};
    for (size_t i = 0; i < 3; ++i) {
        MnShareSession::Share share{MakeShare(amounts[i], FreshP2PKHAddress(&owner_keys[i]), FreshP2PKHAddress(&dummy))};
        share.label = QString::fromLatin1(labels[i]);
        session.shares().push_back(share);
    }
    CKey voting_key;
    session.terms().votingAddress = FreshP2PKHAddress(&voting_key);
    session.terms().earlyPeriodBlocks = 5000;
    session.terms().earlyPenalty = 5 * COIN;
    for (size_t i = 0; i < 3; ++i) {
        MnShareSession::Contribution contribution;
        contribution.label = QString::fromLatin1(labels[i]);
        MnShareSession::Input input;
        input.txid = FakeTxid(static_cast<char>('1' + i));
        contribution.inputs.push_back(input);
        if (i == 0) {
            // Two inputs for one contribution, so a merge test has one input
            // per participant to sign independently
            input.vout = 1;
            contribution.inputs.push_back(input);
        }
        if (i + 1 < 3) {
            contribution.hasChange = true;
            contribution.changeAddress = FreshP2PKHAddress(&dummy);
            contribution.changeAmount = COIN;
        }
        QString error;
        if (!session.addContribution(contribution, error)) return session;
    }
    return session;
}

//! A fresh basic-scheme operator key recorded in `session`'s terms
CBLSSecretKey SetFreshOperatorKey(MnShareSession& session)
{
    CBLSSecretKey secret;
    secret.MakeNewKey();
    session.terms().operatorPubKey =
        QString::fromStdString(secret.GetPublicKey().ToString(/*specificLegacyScheme=*/false));
    return secret;
}
//! A coordinator's invitation: four named shares with amounts and terms, but
//! no addresses and no funding yet
MnShareSession InvitationSession()
{
    return MasternodeTestUtil::MakeInvitation(
        {{"alice", 250 * COIN}, {"bob", 250 * COIN}, {"carol", 250 * COIN}, {"dave", 250 * COIN}});
}

} // anonymous namespace

void MnShareSessionTests::validateSharesMirror()
{
    BasicTestingSetup setup{CBaseChainParams::REGTEST};
    std::vector<CKey> keys;
    MnShareSession session{ValidSession(keys)};
    QVERIFY(session.validateShares().isEmpty());

    // Too few shares
    MnShareSession too_few{ValidSession(keys)};
    too_few.shares().resize(1);
    QVERIFY(!too_few.validateShares().isEmpty());

    // Sum must be exactly the collateral amount
    MnShareSession bad_sum{ValidSession(keys)};
    bad_sum.shares()[0].amount += COIN;
    QVERIFY(!bad_sum.validateShares().isEmpty());

    // Minimum share amount
    MnShareSession too_small{ValidSession(keys)};
    too_small.shares()[0].amount = 950 * COIN;
    too_small.shares()[1].amount = 25 * COIN; // below the 100 DASH minimum
    too_small.shares()[2].amount = 25 * COIN;
    QVERIFY(!too_small.validateShares().isEmpty());

    // Duplicate owner keys
    MnShareSession dup_owner{ValidSession(keys)};
    dup_owner.shares()[1].ownerAddress = dup_owner.shares()[0].ownerAddress;
    QVERIFY(!dup_owner.validateShares().isEmpty());

    // Duplicate refund scripts
    MnShareSession dup_refund{ValidSession(keys)};
    dup_refund.shares()[1].refundAddress = dup_refund.shares()[0].refundAddress;
    QVERIFY(!dup_refund.validateShares().isEmpty());

    // A reward script may not reuse a share owner address
    MnShareSession payee_reuse{ValidSession(keys)};
    payee_reuse.shares()[1].rewardAddress = payee_reuse.shares()[0].ownerAddress;
    QVERIFY(!payee_reuse.validateShares().isEmpty());

    // The early penalty must stay below the smallest share
    MnShareSession bad_penalty{ValidSession(keys)};
    bad_penalty.terms().earlyPenalty = 250 * COIN;
    QVERIFY(!bad_penalty.validateShares().isEmpty());

    // Without an early period there is no penalty to set
    MnShareSession penalty_without_period{ValidSession(keys)};
    penalty_without_period.terms().earlyPeriodBlocks = 0;
    penalty_without_period.terms().earlyPenalty = 5 * COIN;
    QVERIFY(!penalty_without_period.validateShares().isEmpty());
    penalty_without_period.terms().earlyPenalty = 0;
    QVERIFY(penalty_without_period.validateShares().isEmpty());
    const auto unit{BitcoinUnits::Unit::DASH};
    QVERIFY(!MnShareSession::CheckEarlyPenalty({250 * COIN, 750 * COIN}, unit, 0, 5 * COIN).isEmpty());
    QVERIFY(MnShareSession::CheckEarlyPenalty({250 * COIN, 750 * COIN}, unit, 5000, 5 * COIN).isEmpty());

    // Amounts alone, before any address exists
    QVERIFY(MnShareSession::CheckShareAmounts({500 * COIN, 500 * COIN}, unit).isEmpty());
    QVERIFY(MnShareSession::CheckShareAmounts(std::vector<CAmount>(8, 125 * COIN), unit).isEmpty());
    QVERIFY(!MnShareSession::CheckShareAmounts({1000 * COIN}, unit).isEmpty());
    QVERIFY(!MnShareSession::CheckShareAmounts({50 * COIN, 950 * COIN}, unit).isEmpty());
    QVERIFY(!MnShareSession::CheckShareAmounts({500 * COIN, 501 * COIN}, unit).isEmpty());

    // The early period is capped
    MnShareSession bad_period{ValidSession(keys)};
    bad_period.terms().earlyPeriodBlocks = CProRegTx::MAX_EARLY_PERIOD_BLOCKS + 1;
    QVERIFY(!bad_period.validateShares().isEmpty());
}

void MnShareSessionTests::envelopeRoundTrip()
{
    BasicTestingSetup setup{CBaseChainParams::REGTEST};
    std::vector<CKey> keys;
    MnShareSession session{ValidSession(keys)};
    session.shares()[0].label = "alice";
    session.setOperatorSecretHolder("bob");
    const int initial_revision{session.revision()};
    session.noteDraftChange();
    QCOMPARE(session.revision(), initial_revision + 1);

    MnShareSession restored;
    QString error;
    QVERIFY2(restored.fromJson(session.toJsonString().toStdString(), error), qPrintable(error));
    QCOMPARE(restored.sessionId(), session.sessionId());
    QCOMPARE(restored.revision(), session.revision());
    QCOMPARE(int(restored.stage()), int(MnShareSession::Stage::Draft));
    QCOMPARE(restored.shares().size(), session.shares().size());
    QCOMPARE(restored.shares()[0].label, QString("alice"));
    QCOMPARE(restored.shares()[2].amount, 250 * COIN);
    QCOMPARE(restored.terms().earlyPenalty, 5 * COIN);
    QCOMPARE(restored.operatorSecretHolder(), QString("bob"));

    // A different network is a hard error
    UniValue json{session.toJson()};
    json.pushKV("network", "main");
    MnShareSession wrong_network;
    QVERIFY(!wrong_network.fromJson(json, error));
    QVERIFY(error.contains("main"));

    // Unknown stages are rejected
    json = session.toJson();
    json.pushKV("network", QString::fromStdString(Params().NetworkIDString()).toStdString());
    json.pushKV("stage", "warp");
    MnShareSession wrong_stage;
    QVERIFY(!wrong_stage.fromJson(json, error));
}

void MnShareSessionTests::envelopeFingerprint()
{
    BasicTestingSetup setup{CBaseChainParams::REGTEST};
    std::vector<CKey> keys;
    MnShareSession session{ValidSession(keys)};
    session.shares()[0].label = "alice";

    // The code is a short, stable, human-readable identity for one message
    const QString code{session.fingerprint()};
    QCOMPARE(code.size(), 9);
    QCOMPARE(code.at(4), QChar('-'));
    for (const QChar c : code) {
        QVERIFY(c == QChar('-') || (c >= QChar('0') && c <= QChar('9')) || (c >= QChar('A') && c <= QChar('F')));
    }
    // Deterministic: the same content always yields the same code
    QCOMPARE(session.fingerprint(), code);
    QCOMPARE(shared_mn::EnvelopeFingerprint(session.toJson()), code);

    // The session code names the session for its whole life
    QCOMPARE(session.sessionCode(), session.sessionId().left(6).toUpper());
    QCOMPARE(session.sessionCode().size(), 6);

    // Any field change changes the code
    MnShareSession edited{session};
    edited.shares()[1].amount += COIN;
    edited.shares()[2].amount -= COIN;
    QVERIFY(edited.fingerprint() != code);
    MnShareSession relabelled{session};
    relabelled.shares()[0].label = "alicia";
    QVERIFY(relabelled.fingerprint() != code);
    MnShareSession bumped{session};
    bumped.noteDraftChange();
    QVERIFY(bumped.fingerprint() != code);

    // The envelope carries the code, and a round trip preserves it
    const UniValue json{session.toJson()};
    QVERIFY(json.find_value("fingerprint").isStr());
    QCOMPARE(QString::fromStdString(json.find_value("fingerprint").get_str()), code);
    MnShareSession restored;
    QString error;
    QVERIFY2(restored.fromJson(session.toJsonString().toStdString(), error), qPrintable(error));
    QCOMPARE(restored.fingerprint(), code);
    QCOMPARE(restored.sessionCode(), session.sessionCode());
    QVERIFY(restored.importWarning().isEmpty());

    // Editing the JSON after it was copied is detected, but still parses: the
    // consensus-relevant fields are re-validated on their own
    UniValue tampered{session.toJson()};
    tampered.pushKV("operatorSecretHolder", "mallory");
    MnShareSession imported;
    QVERIFY2(imported.fromJson(tampered, error), qPrintable(error));
    QVERIFY(!imported.importWarning().isEmpty());
    QCOMPARE(imported.operatorSecretHolder(), QString("mallory"));

    // An envelope without a fingerprint is accepted without a warning
    UniValue unstamped(UniValue::VOBJ);
    for (size_t i = 0; i < json.getKeys().size(); ++i) {
        if (json.getKeys()[i] == "fingerprint") continue;
        unstamped.pushKV(json.getKeys()[i], json.getValues()[i]);
    }
    MnShareSession legacy;
    QVERIFY2(legacy.fromJson(unstamped, error), qPrintable(error));
    QVERIFY(legacy.importWarning().isEmpty());

    // The same helpers stamp and check the maintenance signing envelope
    UniValue sigs(UniValue::VOBJ);
    sigs.pushKV("type", "dash-shared-mn-sigs");
    sigs.pushKV("kind", "dissolve");
    shared_mn::AppendFingerprint(sigs);
    QString warning;
    QVERIFY(shared_mn::CheckFingerprint(sigs, warning));
    QVERIFY(warning.isEmpty());
    sigs.pushKV("kind", "registrar");
    QVERIFY(!shared_mn::CheckFingerprint(sigs, warning));
    QVERIFY(!warning.isEmpty());
}

void MnShareSessionTests::draftRepliesMergeInAnyOrder()
{
    BasicTestingSetup setup{CBaseChainParams::REGTEST};
    const MnShareSession invitation{InvitationSession()};
    const MnShareSession bob{DraftReply(invitation, 1, FakeTxid('1'))};
    const MnShareSession carol{DraftReply(invitation, 2, FakeTxid('2'))};
    const MnShareSession dave{DraftReply(invitation, 3, FakeTxid('3'))};
    QVERIFY(!bob.shares()[1].ownerAddress.isEmpty());
    QCOMPARE(bob.revision(), invitation.revision());
    QCOMPARE(bob.coordinatorLabel(), QString("alice"));

    QString error;
    bool all_merged{true};
    const auto absorb_all = [&](std::vector<const MnShareSession*> replies) {
        MnShareSession coordinator{invitation};
        for (const MnShareSession* reply : replies) {
            if (coordinator.absorbDraftReply(*reply, error) != MnShareSession::MergeResult::Merged) all_merged = false;
        }
        return coordinator;
    };
    const MnShareSession in_order{absorb_all({&bob, &carol, &dave})};
    const MnShareSession reversed{absorb_all({&dave, &carol, &bob})};
    const MnShareSession shuffled{absorb_all({&carol, &dave, &bob})};
    QVERIFY2(all_merged, qPrintable(error));

    // Replies can arrive in any order: the same addresses land in the same
    // rows and the funding transaction is byte-identical
    QCOMPARE(in_order.shares()[1].ownerAddress, bob.shares()[1].ownerAddress);
    QCOMPARE(in_order.shares()[2].refundAddress, carol.shares()[2].refundAddress);
    QCOMPARE(in_order.shares()[3].ownerAddress, dave.shares()[3].ownerAddress);
    QCOMPARE(in_order.contributions().size(), size_t{3});
    QCOMPARE(reversed.fundingTxHex(), in_order.fundingTxHex());
    QCOMPARE(shuffled.fundingTxHex(), in_order.fundingTxHex());
    QCOMPARE(reversed.revision(), in_order.revision());
    QCOMPARE(in_order.revision(), invitation.revision() + 3);

    // Re-absorbing a reply already in hand changes nothing and does not bump
    // the revision, so the other copies do not go stale
    MnShareSession repeated{in_order};
    QCOMPARE(int(repeated.absorbDraftReply(bob, error)), int(MnShareSession::MergeResult::Merged));
    QCOMPARE(repeated.toJsonString(), in_order.toJsonString());

    // A second, different reply for a row that is already filled in is a
    // conflict rather than a silent overwrite
    const MnShareSession bob_again{DraftReply(invitation, 1, FakeTxid('4'))};
    MnShareSession target{in_order};
    const QString before{target.toJsonString()};
    QCOMPARE(int(target.absorbDraftReply(bob_again, error)), int(MnShareSession::MergeResult::Conflict));
    QVERIFY(error.contains("bob"));
    QCOMPARE(target.toJsonString(), before);

    // A reply that changed the draft it was answering is a conflict
    MnShareSession edited_invitation{invitation};
    edited_invitation.shares()[1].amount = 300 * COIN;
    edited_invitation.shares()[2].amount = 200 * COIN;
    const MnShareSession changed_amount{DraftReply(edited_invitation, 1, FakeTxid('5'))};
    MnShareSession amount_target{invitation};
    QCOMPARE(int(amount_target.absorbDraftReply(changed_amount, error)), int(MnShareSession::MergeResult::Conflict));

    MnShareSession edited_terms{invitation};
    edited_terms.terms().earlyPenalty = 7 * COIN;
    const MnShareSession changed_terms{DraftReply(edited_terms, 1, FakeTxid('6'))};
    MnShareSession terms_target{invitation};
    QCOMPARE(int(terms_target.absorbDraftReply(changed_terms, error)), int(MnShareSession::MergeResult::Conflict));

    // A reply from another session is never absorbed
    const MnShareSession other_session{DraftReply(InvitationSession(), 1, FakeTxid('7'))};
    MnShareSession session_target{invitation};
    QCOMPARE(int(session_target.absorbDraftReply(other_session, error)), int(MnShareSession::MergeResult::Conflict));
    QCOMPARE(session_target.toJsonString(), invitation.toJsonString());
}

void MnShareSessionTests::lockedTermsAdoption()
{
    BasicTestingSetup setup{CBaseChainParams::REGTEST};
    MnShareSession invitation{InvitationSession()};
    // The operator key is part of the invitation everybody answers, so the
    // replies carry it too
    const CBLSSecretKey operator_secret{SetFreshOperatorKey(invitation)};
    const MnShareSession bob{DraftReply(invitation, 1, FakeTxid('1'))};
    const MnShareSession carol{DraftReply(invitation, 2, FakeTxid('2'))};
    const MnShareSession dave{DraftReply(invitation, 3, FakeTxid('3'))};

    QString error;
    MnShareSession coordinator{invitation};
    CKey alice_owner;
    CKey alice_refund;
    coordinator.shares()[0].ownerAddress = FreshP2PKHAddress(&alice_owner);
    coordinator.shares()[0].refundAddress = FreshP2PKHAddress(&alice_refund);
    for (const MnShareSession* reply : {&bob, &carol, &dave}) {
        QCOMPARE(int(coordinator.absorbDraftReply(*reply, error)), int(MnShareSession::MergeResult::Merged));
    }

    // Lock the collected terms the way shared_register_prepare would
    const MnShareSession collected{coordinator}; // the draft everybody answered
    const PreparedRegistration prepared{PrepareRegistration(coordinator, operator_secret)};
    QVERIFY2(coordinator.freeze(prepared.txHex, prepared.consentHashHex, prepared.collateralIndex, error),
             qPrintable(error));

    // Bob only ever saw his own row and his own coins. He adopts the locked
    // terms because they contain exactly what he sent.
    MnShareSession bob_copy{bob};
    QVERIFY2(bob_copy.adoptLockedTerms(coordinator, error), qPrintable(error));
    QCOMPARE(int(bob_copy.stage()), int(MnShareSession::Stage::Frozen));
    QCOMPARE(bob_copy.consentHash(), coordinator.consentHash());
    QCOMPARE(bob_copy.shares()[0].ownerAddress, coordinator.shares()[0].ownerAddress);

    // Terms that quietly changed his refund address are refused
    MnShareSession swapped_refund{bob};
    CKey attacker;
    swapped_refund.shares()[1].refundAddress = FreshP2PKHAddress(&attacker);
    QVERIFY(!swapped_refund.adoptLockedTerms(coordinator, error));
    QVERIFY(!error.isEmpty());
    QCOMPARE(int(swapped_refund.stage()), int(MnShareSession::Stage::Draft));

    // So are terms that changed his coins
    MnShareSession swapped_coins{bob};
    QVERIFY(swapped_coins.removeContribution("bob", error));
    MnShareSession::Contribution other;
    other.label = "bob";
    MnShareSession::Input input;
    input.txid = FakeTxid('9');
    other.inputs.push_back(input);
    QVERIFY(swapped_coins.addContribution(other, error));
    QVERIFY(!swapped_coins.adoptLockedTerms(coordinator, error));

    // A draft copy is not locked terms, and another session is never adopted
    MnShareSession not_locked{bob};
    QVERIFY(!not_locked.adoptLockedTerms(invitation, error));
    MnShareSession foreign{DraftReply(InvitationSession(), 1, FakeTxid('1'))};
    QVERIFY(!foreign.adoptLockedTerms(coordinator, error));

    // Bob's own row and coins are untouched, but the coordinator changed the
    // exit terms after he replied. He never saw these terms, so approving them
    // is refused rather than merged: nothing would be left to compare against.
    MnShareSession edited{collected};
    edited.terms().earlyPenalty = 6 * COIN;
    const PreparedRegistration edited_prepared{PrepareRegistration(edited, operator_secret)};
    QVERIFY2(edited.freeze(edited_prepared.txHex, edited_prepared.consentHashHex, edited_prepared.collateralIndex,
                           error),
             qPrintable(error));
    MnShareSession bob_stale{bob};
    QVERIFY(!bob_stale.adoptLockedTerms(edited, error));
    QVERIFY2(error.contains("changed"), qPrintable(error));
    QVERIFY2(error.contains("alice"), qPrintable(error));
    QCOMPARE(int(bob_stale.stage()), int(MnShareSession::Stage::Draft));

    // Another row's amount is just as much a change to the agreement
    MnShareSession reshuffled{collected};
    reshuffled.shares()[2].amount += 10 * COIN;
    reshuffled.shares()[3].amount -= 10 * COIN;
    const PreparedRegistration reshuffled_prepared{PrepareRegistration(reshuffled, operator_secret)};
    QVERIFY2(reshuffled.freeze(reshuffled_prepared.txHex, reshuffled_prepared.consentHashHex,
                               reshuffled_prepared.collateralIndex, error),
             qPrintable(error));
    MnShareSession bob_reshuffled{bob};
    QVERIFY(!bob_reshuffled.adoptLockedTerms(reshuffled, error));

    // A participant who did answer the new invitation carries the new terms and
    // is not blocked by the check
    MnShareSession bob_rereplied{bob};
    bob_rereplied.terms().earlyPenalty = 6 * COIN;
    QVERIFY2(bob_rereplied.adoptLockedTerms(edited, error), qPrintable(error));
    QCOMPARE(int(bob_rereplied.stage()), int(MnShareSession::Stage::Frozen));
}

void MnShareSessionTests::penaltyPreviewMath()
{
    BasicTestingSetup setup{CBaseChainParams::REGTEST};
    const std::vector<CAmount> amounts{400 * COIN, 350 * COIN, 250 * COIN};
    const CAmount penalty{5 * COIN};
    const CAmount fee{100000};
    const uint32_t early_period{5000};
    const int registered{1000};

    // Early: the actor pays the penalty and fee, the others split the penalty
    // pro-rata with the remainder on the last non-actor share
    auto preview{MnShareSession::PenaltyPreviewFor(amounts, /*actor_index=*/0, penalty, early_period, fee,
                                                   /*at_height=*/2000, registered)};
    QVERIFY2(preview.valid, qPrintable(preview.error));
    QVERIFY(preview.early);
    QCOMPARE(preview.penalty, penalty);
    QCOMPARE(preview.penaltyFreeHeight, registered + int(early_period));
    QCOMPARE(preview.payouts.size(), amounts.size());
    QCOMPARE(preview.payouts[0], 400 * COIN - penalty - fee);
    // 350:250 pro-rata split of 5 DASH = 2.916... : 2.083...; floor to the
    // first share, remainder to the last
    const CAmount bonus_1{penalty * 350 / 600};
    QCOMPARE(preview.payouts[1], 350 * COIN + bonus_1);
    QCOMPARE(preview.payouts[2], 250 * COIN + (penalty - bonus_1));
    QCOMPARE(preview.payouts[0] + preview.payouts[1] + preview.payouts[2], 1000 * COIN - fee);

    // Past the boundary: no penalty
    preview = MnShareSession::PenaltyPreviewFor(amounts, /*actor_index=*/0, penalty, early_period, fee,
                                                /*at_height=*/registered + int(early_period), registered);
    QVERIFY2(preview.valid, qPrintable(preview.error));
    QVERIFY(!preview.early);
    QCOMPARE(preview.payouts[0], 400 * COIN - fee);
    QCOMPARE(preview.payouts[1], 350 * COIN);
    QCOMPARE(preview.payouts[2], 250 * COIN);

    // A share the session does not have is reported in the 1-based numbering
    // every screen shows, not the 0-based index the caller passed
    preview = MnShareSession::PenaltyPreviewFor(amounts, /*actor_index=*/3, penalty, early_period, fee,
                                                /*at_height=*/2000, registered);
    QVERIFY(!preview.valid);
    QCOMPARE(preview.error, QString("Share 4 of 3 does not exist."));
}

void MnShareSessionTests::earlyPeriodWording()
{
    // Qt only picks a %n plural form from a loaded catalogue; the tests run
    // without one, so a "%n minute(s)" source string would reach the user as
    // the literal "(s)". Each unit spells its own plural instead.
    // No early period at all is "none", not "about 0 minutes"
    QCOMPARE(MnShareSession::HumanEarlyPeriod(0), QString("none"));

    // Days round to the nearest day: 720 minutes is half a day
    QCOMPARE(MnShareSession::HumanEarlyPeriod(1900), QString("about 3 days")); // 3.30 days, rounds down
    QCOMPARE(MnShareSession::HumanEarlyPeriod(2016), QString("about 4 days")); // 3.50 days, rounds up

    for (const uint32_t blocks : {0u, 24u, 48u, 1000u, 1728u, 2016u}) {
        QVERIFY(!MnShareSession::HumanEarlyPeriod(blocks).contains("(s)"));
    }
}

void MnShareSessionTests::signatureVerification()
{
    BasicTestingSetup setup{CBaseChainParams::REGTEST};
    std::vector<CKey> owner_keys;
    MnShareSession session{ValidSession(owner_keys)};
    // The operator key is a first-class term; shared_register_prepare would put
    // it in the payload, so the session must carry the matching pubkey
    const CBLSSecretKey operator_secret{SetFreshOperatorKey(session)};

    // Build the shared registration the way shared_register_prepare would: the
    // session's funding transaction with the collateral output appended and the
    // share table in its payload
    const PreparedRegistration prepared{PrepareRegistration(session, operator_secret)};
    const CProRegTx& payload{prepared.payload};
    const CMutableTransaction& tx{prepared.tx};
    const uint256 consent_hash{prepared.consentHash};
    const QString tx_hex{prepared.txHex};
    const int collateral_index{prepared.collateralIndex};

    QString error;
    QVERIFY2(session.freeze(tx_hex, prepared.consentHashHex, collateral_index, error), qPrintable(error));
    QCOMPARE(int(session.stage()), int(MnShareSession::Stage::Frozen));

    // freeze() rejects a consent hash that does not match the transaction
    std::vector<CKey> other_keys;
    MnShareSession session2{ValidSession(other_keys)};
    QVERIFY(!session2.freeze(tx_hex, QString::fromStdString(uint256::ONE.ToString()), collateral_index, error));

    // A valid signature for share 1 is accepted
    std::vector<unsigned char> sig;
    QVERIFY(CHashSigner::SignHash(consent_hash, owner_keys[1], sig));
    const QString sig_b64{QString::fromStdString(EncodeBase64(sig))};
    QVERIFY2(session.addSignature(1, sig_b64, error), qPrintable(error));
    QCOMPARE(session.signedCount(), 1);
    QVERIFY(session.signatureFor(0).isEmpty());
    QVERIFY(!session.signatureFor(1).isEmpty());
    QVERIFY(session.signatureFor(2).isEmpty());

    // A byte-identical duplicate is a silent success, and does not double-count
    QVERIFY(session.addSignature(1, sig_b64, error));
    QCOMPARE(session.signedCount(), 1);

    // The same signature under the wrong index is rejected
    QVERIFY(!session.addSignature(0, sig_b64, error));

    // A signature over a different digest (a stale draft) is rejected
    std::vector<unsigned char> stale_sig;
    QVERIFY(CHashSigner::SignHash(uint256::ONE, owner_keys[0], stale_sig));
    QVERIFY(!session.addSignature(0, QString::fromStdString(EncodeBase64(stale_sig)), error));
    QCOMPARE(session.signedCount(), 1);

    // Unfreezing discards the collected signatures and bumps the revision
    const int revision{session.revision()};
    session.unfreeze();
    QCOMPARE(int(session.stage()), int(MnShareSession::Stage::Draft));
    QCOMPARE(session.signedCount(), 0);
    QVERIFY(session.revision() > revision);

    // A payload that disagrees with the displayed share table must never freeze
    // or verify: the participant reviews m_shares but the wallet signs the
    // payload digest, so a mismatch is a money-loss trap
    MnShareSession tampered{session}; // Draft again, with the same funding
    // The displayed table shows share 0 refunding to its own fresh address,
    // but the payload refunds share 0 to an attacker address
    CProRegTx evil{payload};
    CKey attacker;
    attacker.MakeNewKey(true);
    evil.shares[0].scriptRefund = GetScriptForDestination(PKHash(attacker.GetPubKey()));
    CMutableTransaction evil_tx{tx};
    SetTxPayload(evil_tx, evil);
    const uint256 evil_hash{evil.MakeSharedRegConsentHash(CTransaction(evil_tx))};
    const QString evil_hex{QString::fromStdString(EncodeHexTx(CTransaction(evil_tx)))};
    QVERIFY(!tampered.freeze(evil_hex, QString::fromStdString(evil_hash.ToString()), collateral_index, error));

    // The same tampered payload delivered through a frozen envelope is rejected
    // on import. Re-freeze the matching session to get a well-formed envelope,
    // then swap in the attacker's payload.
    QVERIFY2(session.freeze(tx_hex, prepared.consentHashHex, collateral_index, error), qPrintable(error));
    UniValue envelope{session.toJson()};
    envelope.pushKV("protx", evil_hex.toStdString());
    envelope.pushKV("consentHash", evil_hash.ToString());
    MnShareSession imported;
    QVERIFY(!imported.fromJson(envelope, error));
}

void MnShareSessionTests::frozenEnvelopeMatchesFunding()
{
    BasicTestingSetup setup{CBaseChainParams::REGTEST};
    std::vector<CKey> owner_keys;
    MnShareSession session{ValidSession(owner_keys)};
    session.terms().coreP2PAddrs = QStringLiteral("127.0.0.1:19999");
    const CBLSSecretKey operator_secret{SetFreshOperatorKey(session)};
    const PreparedRegistration prepared{PrepareRegistration(session, operator_secret)};

    QString error;
    MnShareSession frozen{session};
    QVERIFY2(frozen.freeze(prepared.txHex, prepared.consentHashHex, prepared.collateralIndex, error),
             qPrintable(error));
    const UniValue honest{frozen.toJson()};
    MnShareSession imported;
    QVERIFY2(imported.fromJson(honest, error), qPrintable(error));

    // The funding card and the "change returned" line are read out of
    // contributions[]. A frozen envelope carrying a transaction that pays
    // something else must never verify, however well-formed the rest of it is:
    // the consent digest is recomputed from the transaction, so re-signing the
    // attack is free.
    const auto envelope_with_tx = [&honest, &prepared](const CMutableTransaction& tx, int collateral_index) {
        UniValue json{honest};
        json.pushKV("protx", EncodeHexTx(CTransaction(tx)));
        json.pushKV("consentHash", prepared.payload.MakeSharedRegConsentHash(CTransaction(tx)).ToString());
        json.pushKV("collateralIndex", collateral_index);
        return json;
    };
    MnShareSession rejected;

    // One participant's change output pays the coordinator instead
    CKey thief;
    CMutableTransaction redirected{prepared.tx};
    redirected.vout[1].scriptPubKey =
        GetScriptForDestination(DecodeDestination(FreshP2PKHAddress(&thief).toStdString()));
    QVERIFY(!rejected.fromJson(envelope_with_tx(redirected, prepared.collateralIndex), error));
    QVERIFY2(error.contains("funding"), qPrintable(error));

    // ... or is dropped altogether, so the change becomes fee
    CMutableTransaction dropped{prepared.tx};
    dropped.vout.erase(dropped.vout.begin() + 1);
    QVERIFY(!rejected.fromJson(envelope_with_tx(dropped, prepared.collateralIndex - 1), error));
    QVERIFY2(error.contains("funding"), qPrintable(error));

    // ... or an input nobody agreed to is spent as well
    CMutableTransaction extra_input{prepared.tx};
    extra_input.vin.emplace_back(COutPoint(uint256::ONE, 7));
    QVERIFY(!rejected.fromJson(envelope_with_tx(extra_input, prepared.collateralIndex), error));

    // The collateral index must name the collateral output of the transaction
    UniValue wrong_index{honest};
    wrong_index.pushKV("collateralIndex", prepared.collateralIndex - 1);
    QVERIFY(!rejected.fromJson(wrong_index, error));

    // The Service line must be what actually gets registered
    UniValue wrong_service{honest};
    UniValue terms{wrong_service.find_value("terms")};
    terms.pushKV("coreP2PAddrs", "10.9.9.9:19999");
    wrong_service.pushKV("terms", terms);
    QVERIFY(!rejected.fromJson(wrong_service, error));

    // None of the tampered envelopes can be parsed, so adoptLockedTerms never
    // sees them; the honest one is adopted as before
    MnShareSession participant{session};
    QVERIFY2(participant.adoptLockedTerms(imported, error), qPrintable(error));
    QCOMPARE(int(participant.stage()), int(MnShareSession::Stage::Frozen));
}

void MnShareSessionTests::parallelFundingSignatureMerge()
{
    BasicTestingSetup setup{CBaseChainParams::REGTEST};
    std::vector<CKey> owner_keys;
    MnShareSession combined{ValidSession(owner_keys)};
    const CBLSSecretKey operator_secret{SetFreshOperatorKey(combined)};

    const PreparedRegistration prepared{PrepareRegistration(combined, operator_secret)};
    const CProRegTx& payload{prepared.payload};
    const CMutableTransaction& tx{prepared.tx};
    const uint256 consent_hash{prepared.consentHash};
    const QString tx_hex{prepared.txHex};
    QString error;
    QVERIFY2(combined.freeze(tx_hex, prepared.consentHashHex, prepared.collateralIndex, error), qPrintable(error));

    for (int i = 0; i < static_cast<int>(owner_keys.size()); ++i) {
        std::vector<unsigned char> signature;
        QVERIFY(CHashSigner::SignHash(consent_hash, owner_keys[i], signature));
        QVERIFY2(combined.addSignature(i, QString::fromStdString(EncodeBase64(signature)), error), qPrintable(error));
    }
    QVERIFY2(combined.setCombinedTx(tx_hex, error), qPrintable(error));

    // Each participant signs the inputs of their own contribution, which is
    // what "signrawtransactionwithwallet" does on their wallet
    const auto signed_copy = [&](const std::vector<size_t>& inputs, opcodetype opcode) -> std::optional<MnShareSession> {
        CMutableTransaction partial{tx};
        for (const size_t input_index : inputs) {
            partial.vin[input_index].scriptSig = CScript() << opcode;
        }
        UniValue json{combined.toJson()};
        json.pushKV("protx", EncodeHexTx(CTransaction(partial)));
        MnShareSession result;
        if (!result.fromJson(json, error)) return std::nullopt;
        return result;
    };
    const auto alice_copy{signed_copy({0, 1}, OP_1)};
    QVERIFY2(alice_copy.has_value(), qPrintable(error));
    const MnShareSession& alice{*alice_copy};
    const auto bob_copy{signed_copy({2, 3}, OP_2)};
    QVERIFY2(bob_copy.has_value(), qPrintable(error));
    const MnShareSession& bob{*bob_copy};

    // Both participants sign the same unsigned transaction independently.
    // The coordinator can merge their replies in either order and obtains one
    // fully signed transaction without a serial signing handoff.
    MnShareSession alice_then_bob{combined};
    QCOMPARE(int(alice_then_bob.mergeEnvelope(alice, error)), int(MnShareSession::MergeResult::Merged));
    QCOMPARE(int(alice_then_bob.stage()), int(MnShareSession::Stage::Combined));
    QCOMPARE(int(alice_then_bob.mergeEnvelope(bob, error)), int(MnShareSession::MergeResult::Merged));
    QCOMPARE(int(alice_then_bob.stage()), int(MnShareSession::Stage::FundingSigned));

    MnShareSession bob_then_alice{combined};
    QCOMPARE(int(bob_then_alice.mergeEnvelope(bob, error)), int(MnShareSession::MergeResult::Merged));
    QCOMPARE(int(bob_then_alice.mergeEnvelope(alice, error)), int(MnShareSession::MergeResult::Merged));
    QCOMPARE(bob_then_alice.protxHex(), alice_then_bob.protxHex());

    CMutableTransaction merged;
    QVERIFY(DecodeHexTx(merged, alice_then_bob.protxHex().toStdString()));
    QCOMPARE(merged.vin[0].scriptSig, CScript() << OP_1);
    QCOMPARE(merged.vin[1].scriptSig, CScript() << OP_1);
    QCOMPARE(merged.vin[2].scriptSig, CScript() << OP_2);
    QCOMPARE(merged.vin[3].scriptSig, CScript() << OP_2);

    // Different signatures for the same input, or any non-signature
    // transaction difference, are conflicts rather than last-writer-wins.
    const auto conflicting_copy{signed_copy({0}, OP_2)};
    QVERIFY2(conflicting_copy.has_value(), qPrintable(error));
    const MnShareSession& conflicting{*conflicting_copy};
    MnShareSession conflict_target{alice};
    const QString conflict_target_before{conflict_target.toJsonString()};
    QCOMPARE(int(conflict_target.mergeEnvelope(conflicting, error)), int(MnShareSession::MergeResult::Conflict));
    QCOMPARE(conflict_target.toJsonString(), conflict_target_before);

    CProRegTx changed_payload{payload};
    changed_payload.vchJoinSigs[0][0] = 1;
    CMutableTransaction changed{tx};
    changed.vin[2].scriptSig = CScript() << OP_2;
    SetTxPayload(changed, changed_payload);
    UniValue changed_json{combined.toJson()};
    changed_json.pushKV("protx", EncodeHexTx(CTransaction(changed)));
    MnShareSession structurally_changed;
    QVERIFY(structurally_changed.fromJson(changed_json, error));
    MnShareSession structure_target{alice};
    const QString structure_target_before{structure_target.toJsonString()};
    QCOMPARE(int(structure_target.mergeEnvelope(structurally_changed, error)), int(MnShareSession::MergeResult::Conflict));
    QCOMPARE(structure_target.toJsonString(), structure_target_before);

    // A later-phase envelope cannot claim that all funding is signed while
    // carrying the still-unsigned combined transaction.
    UniValue premature_json{combined.toJson()};
    premature_json.pushKV("stage", "fundingSigned");
    MnShareSession premature;
    QVERIFY(!premature.fromJson(premature_json, error));
    QVERIFY(error.contains("funding inputs"));
}

void MnShareSessionTests::oversizedEnvelopesAreRefused()
{
    BasicTestingSetup setup{CBaseChainParams::REGTEST};
    std::vector<CKey> owner_keys;
    const MnShareSession session{ValidSession(owner_keys)};
    QString error;

    // Every one of these is parsed and then rendered on the GUI thread, three
    // widgets and a radio button per share row, so they are refused by count
    // before anything is allocated for them
    UniValue too_many_shares{session.toJson()};
    UniValue shares(UniValue::VARR);
    for (int i = 0; i < 100; ++i) {
        shares.push_back(session.toJson().find_value("shares")[0]);
    }
    too_many_shares.pushKV("shares", shares);
    MnShareSession rejected;
    QVERIFY(!rejected.fromJson(too_many_shares, error));
    QVERIFY2(error.contains("at most"), qPrintable(error));

    UniValue too_many_contributions{session.toJson()};
    UniValue contributions(UniValue::VARR);
    for (int i = 0; i < 100; ++i) {
        contributions.push_back(session.toJson().find_value("contributions")[0]);
    }
    too_many_contributions.pushKV("contributions", contributions);
    QVERIFY(!rejected.fromJson(too_many_contributions, error));
    QVERIFY2(error.contains("at most"), qPrintable(error));

    UniValue too_many_inputs{session.toJson()};
    UniValue entry{too_many_inputs.find_value("contributions")[0]};
    UniValue inputs(UniValue::VARR);
    for (int i = 0; i < 65; ++i) {
        inputs.push_back(entry.find_value("inputs")[0]);
    }
    entry.pushKV("inputs", inputs);
    UniValue one_fat_contribution(UniValue::VARR);
    one_fat_contribution.push_back(entry);
    too_many_inputs.pushKV("contributions", one_fat_contribution);
    QVERIFY(!rejected.fromJson(too_many_inputs, error));
    QVERIFY2(error.contains("inputs"), qPrintable(error));

    // The session itself still parses
    MnShareSession accepted;
    QVERIFY2(accepted.fromJson(session.toJson(), error), qPrintable(error));
}
