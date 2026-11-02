// Copyright (c) 2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/test/sharedmnwizardtests.h>

#include <qt/test/masternodetestutil.h>

#include <qt/bitcoinamountfield.h>
#include <qt/clientmodel.h>
#include <qt/masternodewidgets.h>
#include <qt/mnsharesession.h>
#include <qt/optionsmodel.h>
#include <qt/qvalidatedlineedit.h>
#include <qt/sharedmncreatedialog.h>
#include <qt/sharedmnwidgets.h>
#include <qt/walletmodel.h>

#include <bls/bls.h>
#include <chainparams.h>
#include <consensus/amount.h>
#include <core_io.h>
#include <evo/dmn_types.h>
#include <evo/providertx.h>
#include <evo/sharedcollateral.h>
#include <evo/specialtx.h>
#include <interfaces/node.h>
#include <interfaces/wallet.h>
#include <key.h>
#include <key_io.h>
#include <messagesigner.h>
#include <primitives/transaction.h>
#include <node/context.h>
#include <script/descriptor.h>
#include <script/standard.h>
#include <test/util/setup_common.h>
#include <util/strencodings.h>
#include <util/system.h>
#include <util/translation.h>
#include <wallet/context.h>
#include <wallet/transaction.h>
#include <wallet/wallet.h>

#include <univalue.h>

#include <QAbstractButton>
#include <QApplication>
#include <QButtonGroup>
#include <QClipboard>
#include <QLabel>
#include <QPushButton>
#include <QStandardItemModel>
#include <QString>
#include <QTest>
#include <QTimer>
#include <QWidget>

#include <algorithm>
#include <limits>
#include <memory>
#include <string>
#include <vector>

using wallet::WalletContext;
using MasternodeTestUtil::FakeTxid;
using MasternodeTestUtil::FreshOperatorPubKey;
using MasternodeTestUtil::FreshP2PKHAddress;
using MasternodeTestUtil::DraftReply;
using MasternodeTestUtil::MakeSpendingWallet;
using MasternodeTestUtil::PrepareRegistration;
using MasternodeTestUtil::PreparedRegistration;
using MasternodeTestUtil::WalletGuard;

namespace {

//! A coordinator's invitation: three named shares with amounts and terms, but
//! no addresses and no funding yet
MnShareSession InvitationSession()
{
    return MasternodeTestUtil::MakeInvitation({{"alice", 400 * COIN}, {"bob", 350 * COIN}, {"carol", 250 * COIN}});
}

//! `draft` locked the way shared_register_prepare would lock it, with every
//! share owner's consent signature already collected
MnShareSession FrozenSession(const MnShareSession& draft, const std::vector<CKey>& owner_keys, bool sign_all)
{
    MnShareSession session{draft};
    CBLSSecretKey operator_secret;
    operator_secret.MakeNewKey();
    session.terms().operatorPubKey =
        QString::fromStdString(operator_secret.GetPublicKey().ToString(/*specificLegacyScheme=*/false));

    const PreparedRegistration prepared{PrepareRegistration(session, operator_secret)};
    QString error;
    if (!session.freeze(prepared.txHex, prepared.consentHashHex, prepared.collateralIndex, error)) return draft;
    if (!sign_all) return session;
    for (size_t i = 0; i < owner_keys.size() && i < session.shares().size(); ++i) {
        std::vector<unsigned char> signature;
        if (!CHashSigner::SignHash(prepared.consentHash, owner_keys[i], signature)) return session;
        session.addSignature(static_cast<int>(i), QString::fromStdString(EncodeBase64(signature)), error);
    }
    return session;
}
} // anonymous namespace

void SharedMnWizardTests::coordinatorPageOrder()
{
    TestingSetup test{CBaseChainParams::REGTEST};
    m_node.setContext(&test.m_node);
    SharedMnCreateDialog dialog(m_node, /*wallet_model=*/nullptr, /*parent=*/nullptr);

    // Nothing has been started, so only the landing page is reachable
    QCOMPARE(int(dialog.currentPage()), int(SharedMnCreateDialog::PageLanding));
    QCOMPARE(dialog.m_order.size(), 1);

    dialog.m_role = SharedMnCreateDialog::Role::Coordinator;
    dialog.rebuildOrder();
    const QVector<SharedMnCreateDialog::Page> expected{
        SharedMnCreateDialog::PageLanding,    SharedMnCreateDialog::PageParticipants,
        SharedMnCreateDialog::PageSettings,   SharedMnCreateDialog::PageExitTerms,
        SharedMnCreateDialog::PageContribution, SharedMnCreateDialog::PageSecret,
        SharedMnCreateDialog::PageInvite,     SharedMnCreateDialog::PageApprovals,
        SharedMnCreateDialog::PageSignatures, SharedMnCreateDialog::PageComplete};
    QCOMPARE(dialog.m_order, expected);

    // The five pages that build the invitation are the numbered ones; what
    // comes after them depends on how fast the others answer
    dialog.goToPage(SharedMnCreateDialog::PageParticipants);
    QVERIFY(dialog.m_progress_label->text().contains(dialog.pageTitle(SharedMnCreateDialog::PageParticipants)));
    dialog.goToPage(SharedMnCreateDialog::PageInvite);
    QCOMPARE(int(dialog.currentPage()), int(SharedMnCreateDialog::PageInvite));
    dialog.goToPage(SharedMnCreateDialog::PageApprovals);
    QCOMPARE(dialog.m_progress_label->text(), dialog.pageTitle(SharedMnCreateDialog::PageApprovals));

    // Terms are not locked, so the invitation cannot go out yet
    QVERIFY(!dialog.allDetailsCollected());
}

void SharedMnWizardTests::participantPagesAndRoleInference()
{
    TestingSetup test{CBaseChainParams::REGTEST};
    m_node.setContext(&test.m_node);
    SharedMnCreateDialog dialog(m_node, /*wallet_model=*/nullptr, /*parent=*/nullptr);

    // An invitation where every row but one is already answered can only be
    // about the one row that is left
    MnShareSession invitation{InvitationSession()};
    MnShareSession partial{invitation};
    QString error;
    QVERIFY(partial.absorbDraftReply(DraftReply(invitation, 0, FakeTxid('1')), error) ==
            MnShareSession::MergeResult::Merged);
    QVERIFY(partial.absorbDraftReply(DraftReply(invitation, 2, FakeTxid('2')), error) ==
            MnShareSession::MergeResult::Merged);

    dialog.handleImportedText(partial.toJsonString());
    QCOMPARE(int(dialog.m_role), int(SharedMnCreateDialog::Role::Participant));
    QCOMPARE(int(dialog.currentPage()), int(SharedMnCreateDialog::PageContribution));
    QCOMPARE(dialog.myShareIndex(), 1);
    QCOMPARE(dialog.m_session.sessionId(), invitation.sessionId());

    // A participant never sees the coordinator's drafting pages, but does get
    // the three waiting pages
    const QVector<SharedMnCreateDialog::Page> expected{
        SharedMnCreateDialog::PageLanding,     SharedMnCreateDialog::PageContribution,
        SharedMnCreateDialog::PageWaitTerms,   SharedMnCreateDialog::PageApprovals,
        SharedMnCreateDialog::PageWaitSigning, SharedMnCreateDialog::PageSignatures,
        SharedMnCreateDialog::PageWaitBroadcast, SharedMnCreateDialog::PageComplete};
    QCOMPARE(dialog.m_order, expected);

    // The masternode-wide terms are the coordinator's, and stay read-only here
    QVERIFY(!dialog.m_voting_edit->isEnabled());
    QVERIFY(!dialog.m_early_penalty_field->isEnabled());
    QCOMPARE(dialog.m_voting_edit->text(), invitation.terms().votingAddress);

    // Waiting pages name the coordinator the invitation identified
    QVERIFY(dialog.pageTitle(SharedMnCreateDialog::PageWaitTerms).contains(QStringLiteral("alice")));
}

void SharedMnWizardTests::participantCannotChooseATakenShare()
{
    TestingSetup test{CBaseChainParams::REGTEST};
    m_node.setContext(&test.m_node);
    WalletContext& context{*m_node.walletLoader().context()};

    CKey my_key;
    my_key.MakeNewKey(/*fCompressed=*/true);
    const auto wallet{MakeSpendingWallet(m_node, context, "part", my_key)};
    QVERIFY(wallet != nullptr);
    WalletGuard guard{context, wallet};

    MasternodeTestUtil::GuiModels models{m_node};
    QVERIFY2(models.ok, qPrintable(QString::fromStdString(models.error.translated)));
    WalletModel wallet_model(interfaces::MakeWallet(context, wallet), models.client);

    // alice coordinates and has answered for her own row; bob and carol are open
    const MnShareSession invitation{InvitationSession()};
    MnShareSession claimed{invitation};
    QString error;
    QVERIFY(claimed.absorbDraftReply(DraftReply(invitation, 0, FakeTxid('1')), error) ==
            MnShareSession::MergeResult::Merged);

    const auto combo_row = [](const SharedMnCreateDialog& dialog, int share) {
        auto* const model{qobject_cast<QStandardItemModel*>(dialog.m_who_am_i_combo->model())};
        return model->item(dialog.m_who_am_i_combo->findData(share));
    };

    {
        // Two rows are open, so nothing says which one is ours: the combo must
        // not pretend the first share is chosen, and alice's row is not on offer
        SharedMnCreateDialog dialog(m_node, &wallet_model, /*parent=*/nullptr);
        dialog.handleImportedText(claimed.toJsonString());
        QCOMPARE(int(dialog.m_role), int(SharedMnCreateDialog::Role::Participant));
        QCOMPARE(int(dialog.currentPage()), int(SharedMnCreateDialog::PageContribution));
        QCOMPARE(dialog.myShareIndex(), -1);
        QCOMPARE(dialog.m_who_am_i_combo->currentData().toInt(), -1);
        QVERIFY(!combo_row(dialog, 0)->isEnabled());
        QVERIFY(combo_row(dialog, 0)->text().contains(QStringLiteral("taken")));
        QVERIFY(combo_row(dialog, 1)->isEnabled());
        QVERIFY(combo_row(dialog, 2)->isEnabled());

        // bob answers for the row chosen here before this wallet reserved it:
        // that row is now bob's, and so is the reservation recorded for it
        dialog.m_who_am_i_combo->setCurrentIndex(dialog.m_who_am_i_combo->findData(1));
        QCOMPARE(dialog.myShareIndex(), 1);
        MnShareSession answered{claimed};
        QVERIFY(answered.absorbDraftReply(DraftReply(invitation, 1, FakeTxid('2')), error) ==
                MnShareSession::MergeResult::Merged);
        dialog.handleImportedText(answered.toJsonString());
        QVERIFY2(dialog.m_error_label->text().isEmpty(), qPrintable(dialog.m_error_label->text()));
        QCOMPARE(dialog.myShareIndex(), 2);
        QVERIFY(dialog.myContribution() == nullptr);
        QVERIFY(!combo_row(dialog, 1)->isEnabled());
        QVERIFY(dialog.m_owner_edit->text() != answered.shares()[1].ownerAddress);

        // An owner address from outside this wallet could never approve the share
        dialog.m_owner_edit->setText(FreshP2PKHAddress());
        dialog.reserveCoins();
        QVERIFY(!dialog.m_error_label->text().isEmpty());
        QVERIFY(dialog.myContribution() == nullptr);
        QVERIFY(dialog.m_session.shares()[2].ownerAddress.isEmpty());
    }

    // This wallet already answered for bob's row and now moves to carol's:
    // bob's addresses must not follow it there
    MnShareSession mine{claimed};
    auto& bob{mine.shares()[1]};
    bob.ownerAddress = QString::fromStdString(EncodeDestination(PKHash(my_key.GetPubKey())));
    bob.refundAddress = FreshP2PKHAddress();
    bob.rewardAddress = FreshP2PKHAddress();

    SharedMnCreateDialog dialog(m_node, &wallet_model, /*parent=*/nullptr);
    dialog.handleImportedText(mine.toJsonString());
    QCOMPARE(dialog.myShareIndex(), 1);
    QCOMPARE(dialog.m_owner_edit->text(), bob.ownerAddress);
    QCOMPARE(dialog.m_refund_edit->text(), bob.refundAddress);
    QCOMPARE(dialog.m_reward_edit->text(), bob.rewardAddress);
    QVERIFY(combo_row(dialog, 1)->isEnabled());

    dialog.m_who_am_i_combo->setCurrentIndex(dialog.m_who_am_i_combo->findData(2));
    QCOMPARE(dialog.myShareIndex(), 2);
    QVERIFY(dialog.m_owner_edit->text() != bob.ownerAddress);
    QVERIFY(dialog.m_refund_edit->text() != bob.refundAddress);
    QVERIFY(dialog.m_reward_edit->text().isEmpty());
}

void SharedMnWizardTests::coordinatorResumesSavedSession()
{
    TestingSetup test{CBaseChainParams::REGTEST};
    m_node.setContext(&test.m_node);
    WalletContext& context{*m_node.walletLoader().context()};

    CKey coordinator_key;
    coordinator_key.MakeNewKey(/*fCompressed=*/true);
    const auto wallet{MakeSpendingWallet(m_node, context, "coord", coordinator_key)};
    QVERIFY(wallet != nullptr);
    WalletGuard guard{context, wallet};

    MasternodeTestUtil::GuiModels models{m_node};
    QVERIFY2(models.ok, qPrintable(QString::fromStdString(models.error.translated)));
    WalletModel wallet_model(interfaces::MakeWallet(context, wallet), models.client);
    QCOMPARE(wallet_model.getWalletName(), QStringLiteral("coord"));

    // A session as the coordinator would have saved it from the invite page:
    // prepared by this wallet, alice's row is theirs, and bob and carol have
    // already answered
    MnShareSession invitation{InvitationSession()};
    invitation.setPrepareWallet(QStringLiteral("coord"));
    invitation.terms().operatorPubKey = FreshOperatorPubKey();
    CKey refund_key;
    invitation.shares()[0].ownerAddress =
        QString::fromStdString(EncodeDestination(PKHash(coordinator_key.GetPubKey())));
    invitation.shares()[0].refundAddress = FreshP2PKHAddress(&refund_key);
    MnShareSession::Contribution mine;
    mine.label = QStringLiteral("alice");
    MnShareSession::Input input;
    input.txid = FakeTxid('1');
    mine.inputs.push_back(input);
    QString error;
    QVERIFY2(invitation.addContribution(mine, error), qPrintable(error));

    MnShareSession saved{invitation};
    for (const int share : {1, 2}) {
        QCOMPARE(int(saved.absorbDraftReply(DraftReply(invitation, share, FakeTxid('2' + share)), error)),
                 int(MnShareSession::MergeResult::Merged));
    }

    // Reopening it must not turn the coordinator into a participant: only the
    // coordinator can lock the terms, combine the approvals and broadcast
    SharedMnCreateDialog dialog(m_node, &wallet_model, /*parent=*/nullptr);
    dialog.handleImportedText(saved.toJsonString());
    QVERIFY2(dialog.m_error_label->text().isEmpty(), qPrintable(dialog.m_error_label->text()));
    QCOMPARE(int(dialog.m_role), int(SharedMnCreateDialog::Role::Coordinator));
    QCOMPARE(int(dialog.currentPage()), int(SharedMnCreateDialog::PageInvite));
    QCOMPARE(dialog.myShareIndex(), 0);
    QVERIFY(dialog.allDetailsCollected());

    // alice's coins are reserved, so the coordinator cannot move to another row
    dialog.refreshShareTable();
    QVERIFY(!dialog.m_me_group->button(0)->isEnabled());
    QVERIFY(!dialog.m_me_group->button(1)->isEnabled());

    // Without the reservation the coordinator's own row is free again, but bob's
    // and carol's still belong to them
    dialog.releaseCoins();
    QVERIFY(dialog.myContribution() == nullptr);
    QVERIFY(dialog.m_me_group->button(0)->isEnabled());
    QVERIFY(!dialog.m_me_group->button(1)->isEnabled());
    QVERIFY(!dialog.m_me_group->button(2)->isEnabled());

    // Once no share is this wallet's, no row may stay checked: the next sync
    // of the table would read it back as the coordinator's share
    dialog.m_my_share = -1;
    dialog.refreshShareTable();
    QVERIFY(dialog.m_me_group->checkedButton() == nullptr);
    dialog.syncSharesFromTable();
    QCOMPARE(dialog.myShareIndex(), -1);

    // The operator key everybody was invited to is kept: a fresh one from the
    // key widget would invalidate the terms the others are answering
    QVERIFY(dialog.m_operator_key_from_import);
    dialog.syncTermsToSession();
    QCOMPARE(dialog.m_session.terms().operatorPubKey, saved.terms().operatorPubKey);

    // The same file in a wallet that cannot spend the coordinator's share is
    // still a participant's copy
    SharedMnCreateDialog participant(m_node, /*wallet_model=*/nullptr, /*parent=*/nullptr);
    participant.handleImportedText(saved.toJsonString());
    QCOMPARE(int(participant.m_role), int(SharedMnCreateDialog::Role::Participant));
    QCOMPARE(int(participant.currentPage()), int(SharedMnCreateDialog::PageContribution));
}

void SharedMnWizardTests::coordinatorResumesFullySignedSession()
{
    TestingSetup test{CBaseChainParams::REGTEST};
    m_node.setContext(&test.m_node);
    WalletContext& context{*m_node.walletLoader().context()};

    MnShareSession invitation{InvitationSession()};
    invitation.setPrepareWallet(QStringLiteral("coord"));
    MnShareSession draft{invitation};
    std::vector<CKey> owner_keys(3);
    QString error;
    for (int i = 0; i < 3; ++i) {
        QCOMPARE(int(draft.absorbDraftReply(DraftReply(invitation, i, FakeTxid('1' + i), &owner_keys[i]), error)),
                 int(MnShareSession::MergeResult::Merged));
    }
    MnShareSession session{FrozenSession(draft, owner_keys, /*sign_all=*/true)};
    QCOMPARE(int(session.stage()), int(MnShareSession::Stage::Signing));
    const QString combined{session.protxHex()};
    QVERIFY2(session.setCombinedTx(combined, error), qPrintable(error));

    // Every funding input carries its signature, as it does once the last
    // participant's Signed Contribution has been merged
    CMutableTransaction tx;
    QVERIFY(DecodeHexTx(tx, session.protxHex().toStdString()));
    for (auto& input : tx.vin) {
        input.scriptSig = CScript() << OP_TRUE;
    }
    QVERIFY2(session.setFundingSignedTx(QString::fromStdString(EncodeHexTx(CTransaction(tx))), error),
             qPrintable(error));

    const auto wallet{MakeSpendingWallet(m_node, context, "coord", owner_keys[0])};
    QVERIFY(wallet != nullptr);
    WalletGuard guard{context, wallet};
    MasternodeTestUtil::GuiModels models{m_node};
    QVERIFY2(models.ok, qPrintable(QString::fromStdString(models.error.translated)));
    WalletModel wallet_model(interfaces::MakeWallet(context, wallet), models.client);

    // The coordinator has no waiting page, so a fully signed session must come
    // back on the page that broadcasts it rather than on the landing page
    SharedMnCreateDialog dialog(m_node, &wallet_model, /*parent=*/nullptr);
    dialog.handleImportedText(session.toJsonString());
    QVERIFY2(dialog.m_error_label->text().isEmpty(), qPrintable(dialog.m_error_label->text()));
    QCOMPARE(int(dialog.m_role), int(SharedMnCreateDialog::Role::Coordinator));
    QVERIFY(!dialog.m_order.contains(SharedMnCreateDialog::PageWaitBroadcast));
    QCOMPARE(int(dialog.currentPage()), int(SharedMnCreateDialog::PageSignatures));
    QVERIFY(!dialog.m_next_button->isHidden());
    QVERIFY2(dialog.m_next_button->isEnabled(), qPrintable(dialog.m_next_button->toolTip()));

    // A participant's copy of the same message still waits for the broadcast
    SharedMnCreateDialog participant(m_node, /*wallet_model=*/nullptr, /*parent=*/nullptr);
    participant.handleImportedText(session.toJsonString());
    QCOMPARE(int(participant.m_role), int(SharedMnCreateDialog::Role::Participant));
    QCOMPARE(int(participant.currentPage()), int(SharedMnCreateDialog::PageWaitBroadcast));
}

void SharedMnWizardTests::unauthorisedSignedInputsAreDetected()
{
    BasicTestingSetup setup{CBaseChainParams::REGTEST};
    const auto hex = [](const CMutableTransaction& tx) {
        return QString::fromStdString(EncodeHexTx(CTransaction(tx)));
    };

    CMutableTransaction before;
    before.vin.emplace_back(COutPoint(uint256S(FakeTxid('1').toStdString()), 0));
    before.vin.emplace_back(COutPoint(uint256S(FakeTxid('2').toStdString()), 1));
    before.vout.emplace_back(COIN, CScript() << OP_TRUE);
    const QString before_hex{hex(before)};

    MnShareSession::Contribution mine;
    mine.label = QStringLiteral("alice");
    MnShareSession::Input input;
    input.txid = FakeTxid('1');
    input.vout = 0;
    mine.inputs.push_back(input);

    // Signing the one input we contributed is what the round is for
    CMutableTransaction ours{before};
    ours.vin[0].scriptSig = CScript() << OP_TRUE;
    QString outpoint;
    QVERIFY2(SharedMnCreateDialog::signedOnlyOwnInputs(before_hex, hex(ours), &mine, outpoint),
             qPrintable(outpoint));
    QVERIFY(outpoint.isEmpty());

    // A signature that appeared on somebody else's input is named and refused
    CMutableTransaction foreign{before};
    foreign.vin[1].scriptSig = CScript() << OP_TRUE;
    QVERIFY(!SharedMnCreateDialog::signedOnlyOwnInputs(before_hex, hex(foreign), &mine, outpoint));
    QCOMPARE(outpoint, FakeTxid('2') + QStringLiteral(":1"));

    // With no contribution of our own, nothing at all may be signed
    QVERIFY(!SharedMnCreateDialog::signedOnlyOwnInputs(before_hex, hex(ours), /*mine=*/nullptr, outpoint));
    QCOMPARE(outpoint, FakeTxid('1') + QStringLiteral(":0"));

    // A rewritten prevout spends a coin nobody agreed to, even in our own slot
    CMutableTransaction swapped{before};
    swapped.vin[0].prevout = COutPoint(uint256S(FakeTxid('3').toStdString()), 0);
    QVERIFY(!SharedMnCreateDialog::signedOnlyOwnInputs(before_hex, hex(swapped), &mine, outpoint));
    QCOMPARE(outpoint, FakeTxid('3') + QStringLiteral(":0"));

    // A different input count, or a reply that is not a transaction at all, is
    // refused without an outpoint to name
    CMutableTransaction extra{before};
    extra.vin.emplace_back(COutPoint(uint256S(FakeTxid('4').toStdString()), 0));
    QVERIFY(!SharedMnCreateDialog::signedOnlyOwnInputs(before_hex, hex(extra), &mine, outpoint));
    QVERIFY(outpoint.isEmpty());
    QVERIFY(!SharedMnCreateDialog::signedOnlyOwnInputs(before_hex, QStringLiteral("nonsense"), &mine, outpoint));
    QVERIFY(outpoint.isEmpty());
}

void SharedMnWizardTests::refusesToSignCoinsOutsideOwnContribution()
{
    TestingSetup test{CBaseChainParams::REGTEST};
    m_node.setContext(&test.m_node);
    WalletContext& context{*m_node.walletLoader().context()};

    // The participant's wallet holds the coins of its own share and one
    // further coin, which anybody watching the chain can see
    CKey victim_key;
    victim_key.MakeNewKey(/*fCompressed=*/true);
    const auto wallet{MakeSpendingWallet(m_node, context, "victim", victim_key)};
    QVERIFY(wallet != nullptr);
    WalletGuard guard{context, wallet};

    CMutableTransaction known_coin;
    known_coin.vin.emplace_back(COutPoint(uint256::ONE, 0));
    known_coin.vout.emplace_back(10 * COIN, GetScriptForDestination(PKHash(victim_key.GetPubKey())));
    const QString known_txid{QString::fromStdString(known_coin.GetHash().ToString())};
    QVERIFY(wallet->AddToWallet(MakeTransactionRef(known_coin), wallet::TxStateInactive{}) != nullptr);

    MnShareSession invitation{InvitationSession()};
    MnShareSession draft{invitation};
    std::vector<CKey> owner_keys(3);
    QString error;
    for (int i = 0; i < 3; ++i) {
        QCOMPARE(int(draft.absorbDraftReply(DraftReply(invitation, i, FakeTxid('1' + i), &owner_keys[i]), error)),
                 int(MnShareSession::MergeResult::Merged));
    }
    // This wallet holds bob's share, and bob's own contribution is untouched,
    // so adoptLockedTerms and payloadMatchesEnvelope both accept the session
    owner_keys[1] = victim_key;
    draft.shares()[1].ownerAddress = QString::fromStdString(EncodeDestination(PKHash(victim_key.GetPubKey())));

    // The coordinator adds a contribution of its own that spends the wallet's
    // other coin and sends the change to an address of the coordinator's
    CKey attacker_key;
    MnShareSession::Contribution mallory;
    mallory.label = QStringLiteral("mallory");
    MnShareSession::Input stolen;
    stolen.txid = known_txid;
    stolen.vout = 0;
    mallory.inputs.push_back(stolen);
    mallory.hasChange = true;
    mallory.changeAddress = FreshP2PKHAddress(&attacker_key);
    mallory.changeAmount = 9 * COIN;
    QVERIFY2(draft.addContribution(mallory, error), qPrintable(error));

    // Both halves of the message are consistent: the envelope lists the extra
    // contribution and the transaction spends it, so every envelope check the
    // session makes passes
    MnShareSession session{FrozenSession(draft, owner_keys, /*sign_all=*/true)};
    QCOMPARE(int(session.stage()), int(MnShareSession::Stage::Signing));
    const QString frozen_hex{session.protxHex()};
    QVERIFY2(session.setCombinedTx(frozen_hex, error), qPrintable(error));

    MasternodeTestUtil::GuiModels models{m_node};
    QVERIFY2(models.ok, qPrintable(QString::fromStdString(models.error.translated)));
    WalletModel wallet_model(interfaces::MakeWallet(context, wallet), models.client);

    SharedMnCreateDialog dialog(m_node, &wallet_model, /*parent=*/nullptr);
    dialog.handleImportedText(session.toJsonString());
    QCOMPARE(int(dialog.m_role), int(SharedMnCreateDialog::Role::Participant));
    QCOMPARE(dialog.myShareIndex(), 1);
    QCOMPARE(int(dialog.currentPage()), int(SharedMnCreateDialog::PageSignatures));

    // The term sheet counts the extra coin as paid by this wallet, and none of
    // the change as coming back to it
    QCOMPARE(dialog.ownFunding().spent, 10 * COIN);
    QCOMPARE(dialog.ownFunding().change, CAmount{0});

    // The extra coin is named as soon as the session is adopted, and the
    // primary button will not sign it away
    const QString outpoint{known_txid + QStringLiteral(":0")};
    QVERIFY2(dialog.m_error_label->text().contains(outpoint), qPrintable(dialog.m_error_label->text()));
    QVERIFY(!dialog.m_next_button->isEnabled());
    QVERIFY2(dialog.m_next_button->toolTip().contains(outpoint), qPrintable(dialog.m_next_button->toolTip()));
    QVERIFY(!dialog.validatePage(SharedMnCreateDialog::PageSignatures, error));
    QVERIFY(!dialog.validatePage(SharedMnCreateDialog::PageApprovals, error));

    // Signing refuses before the wallet is asked to unlock, so nothing is
    // applied to the session
    const QString protx_before{dialog.m_session.protxHex()};
    bool complete{true};
    QString sign_error;
    QVERIFY(!dialog.signOwnFundingInputs(complete, sign_error));
    QVERIFY(!complete);
    QVERIFY2(sign_error.contains(outpoint), qPrintable(sign_error));
    QCOMPARE(dialog.m_session.protxHex(), protx_before);
    QCOMPARE(int(dialog.m_session.stage()), int(MnShareSession::Stage::Combined));
    QVERIFY2(!dialog.m_dirty, "a refused signing run must leave nothing to save");
}

void SharedMnWizardTests::refusesToSignShortChangedOwnContribution()
{
    TestingSetup test{CBaseChainParams::REGTEST};
    m_node.setContext(&test.m_node);
    WalletContext& context{*m_node.walletLoader().context()};

    // bob's wallet funds its 350 DASH share with one 450 DASH coin, so 100 DASH
    // of change has to come back to it
    CKey victim_key;
    victim_key.MakeNewKey(/*fCompressed=*/true);
    const auto wallet{MakeSpendingWallet(m_node, context, "victim", victim_key)};
    QVERIFY(wallet != nullptr);
    WalletGuard guard{context, wallet};

    CMutableTransaction coin;
    coin.vin.emplace_back(COutPoint(uint256::ONE, 0));
    coin.vout.emplace_back(450 * COIN, GetScriptForDestination(PKHash(victim_key.GetPubKey())));
    const QString coin_txid{QString::fromStdString(coin.GetHash().ToString())};
    QVERIFY(wallet->AddToWallet(MakeTransactionRef(coin), wallet::TxStateInactive{}) != nullptr);
    const QString own_address{QString::fromStdString(EncodeDestination(PKHash(victim_key.GetPubKey())))};

    MasternodeTestUtil::GuiModels models{m_node};
    QVERIFY2(models.ok, qPrintable(QString::fromStdString(models.error.translated)));
    WalletModel wallet_model(interfaces::MakeWallet(context, wallet), models.client);

    // A combined session in which bob's own contribution spends that coin and
    // sends `change_amount` to `change_address`. Every envelope check passes:
    // the file and the transaction agree with each other. With `coordinated`
    // the session is bob's own, reopened from a backup.
    const auto session_with_change = [&](const QString& change_address, CAmount change_amount,
                                         bool coordinated = false, uint32_t vout = 0) {
        MnShareSession invitation{InvitationSession()};
        if (coordinated) {
            invitation.setCoordinatorLabel(QStringLiteral("bob"));
            invitation.setPrepareWallet(QStringLiteral("victim"));
        }
        MnShareSession draft{invitation};
        std::vector<CKey> owner_keys(3);
        QString error;
        for (int i = 0; i < 3; ++i) {
            draft.absorbDraftReply(DraftReply(invitation, i, FakeTxid('1' + i), &owner_keys[i]), error);
        }
        owner_keys[1] = victim_key;
        draft.shares()[1].ownerAddress = own_address;
        draft.removeContribution(QStringLiteral("bob"), error);
        draft.addContribution({.label = QStringLiteral("bob"),
                               .inputs = {{.txid = coin_txid, .vout = vout}},
                               .hasChange = true,
                               .changeAddress = change_address,
                               .changeAmount = change_amount},
                              error);
        MnShareSession session{FrozenSession(draft, owner_keys, /*sign_all=*/true)};
        session.setCombinedTx(session.protxHex(), error);
        return session;
    };
    const auto amount = [&](CAmount value) { return SharedMnFormatAmount(SharedMnDisplayUnit(&wallet_model), value); };
    // Signing refuses, naming what leaves and what comes back, and the primary
    // button stays disabled
    const auto expect_refusal = [&](const MnShareSession& session, CAmount returned) {
        SharedMnCreateDialog dialog(m_node, &wallet_model, /*parent=*/nullptr);
        dialog.handleImportedText(session.toJsonString());
        QCOMPARE(dialog.myShareIndex(), 1);
        QCOMPARE(int(dialog.currentPage()), int(SharedMnCreateDialog::PageSignatures));
        const QString refusal{dialog.m_funding_refusal};
        QVERIFY2(refusal.contains(QStringLiteral("spends %1").arg(amount(450 * COIN))), qPrintable(refusal));
        QVERIFY2(refusal.contains(QStringLiteral("returns only %1").arg(amount(returned))), qPrintable(refusal));
        QVERIFY(!dialog.m_next_button->isEnabled());
        bool complete{true};
        QString sign_error;
        QVERIFY(!dialog.signOwnFundingInputs(complete, sign_error));
        QCOMPARE(sign_error, refusal);
        // The sheet already warned before signing was ever tried
        QCOMPARE(dialog.ownFunding().spent, 450 * COIN);
        QCOMPARE(dialog.ownFunding().change, returned);
        QVERIFY2(dialog.m_signatures_terms->text().contains(QStringLiteral("⚠")),
                 qPrintable(dialog.m_signatures_terms->text()));
    };

    // Honest: the change comes back to this wallet, so nothing stands in the way
    // of signing (the sign itself fails on the unconfirmed coin, not the check)
    const MnShareSession honest{session_with_change(own_address, 100 * COIN)};
    QCOMPARE(int(honest.stage()), int(MnShareSession::Stage::Combined));
    {
        SharedMnCreateDialog dialog(m_node, &wallet_model, /*parent=*/nullptr);
        dialog.handleImportedText(honest.toJsonString());
        QVERIFY2(dialog.m_funding_refusal.isEmpty(), qPrintable(dialog.m_funding_refusal));
        QVERIFY2(dialog.m_next_button->isEnabled(), qPrintable(dialog.m_next_button->toolTip()));
        // 450 in, 100 back: exactly the 350 share, with nothing to warn about
        QCOMPARE(dialog.ownFunding().spent, 450 * COIN);
        QCOMPARE(dialog.ownFunding().change, 100 * COIN);
        const QString sheet{dialog.m_signatures_terms->text()};
        QVERIFY2(sheet.contains(QStringLiteral("Your contribution: %1").arg(amount(350 * COIN))), qPrintable(sheet));
        QVERIFY(!sheet.contains(QStringLiteral("⚠")));
    }
    // Honest coordinator paying a 0.05 DASH fee, above the usual cap, out of its
    // own coin: what leaves on top of the share is the fee it chose
    {
        SharedMnCreateDialog dialog(m_node, &wallet_model, /*parent=*/nullptr);
        dialog.m_fee_field->setValue(5 * COIN / 100);
        dialog.handleImportedText(session_with_change(own_address, 100 * COIN - 5 * COIN / 100,
                                                      /*coordinated=*/true)
                                      .toJsonString());
        QCOMPARE(int(dialog.m_role), int(SharedMnCreateDialog::Role::Coordinator));
        QVERIFY2(dialog.m_funding_refusal.isEmpty(), qPrintable(dialog.m_funding_refusal));
    }
    // A file naming a coin this wallet holds with an output index past the end
    // of its transaction is not a coin of ours, and must not crash the import
    {
        SharedMnCreateDialog dialog(m_node, &wallet_model, /*parent=*/nullptr);
        dialog.handleImportedText(session_with_change(own_address, 100 * COIN, /*coordinated=*/false,
                                                      /*vout=*/std::numeric_limits<uint32_t>::max())
                                      .toJsonString());
        QCOMPARE(int(dialog.currentPage()), int(SharedMnCreateDialog::PageSignatures));
        QVERIFY2(dialog.m_funding_refusal.isEmpty(), qPrintable(dialog.m_funding_refusal));
    }

    // The change goes to somebody else's address
    CKey attacker_key;
    expect_refusal(session_with_change(FreshP2PKHAddress(&attacker_key), 100 * COIN), 0);
    // The change comes back to this wallet, but 50 DASH short
    expect_refusal(session_with_change(own_address, 50 * COIN), 50 * COIN);
}

void SharedMnWizardTests::savingWaitsForTheOperatorKeyBackup()
{
    TestingSetup test{CBaseChainParams::REGTEST};
    m_node.setContext(&test.m_node);
    WalletContext& context{*m_node.walletLoader().context()};

    CKey coordinator_key;
    coordinator_key.MakeNewKey(/*fCompressed=*/true);
    const auto wallet{MakeSpendingWallet(m_node, context, "coord", coordinator_key)};
    QVERIFY(wallet != nullptr);
    WalletGuard guard{context, wallet};
    MasternodeTestUtil::GuiModels models{m_node};
    QVERIFY2(models.ok, qPrintable(QString::fromStdString(models.error.translated)));
    WalletModel wallet_model(interfaces::MakeWallet(context, wallet), models.client);

    SharedMnCreateDialog dialog(m_node, &wallet_model, /*parent=*/nullptr);
    dialog.m_role = SharedMnCreateDialog::Role::Coordinator;
    dialog.m_session = InvitationSession();
    dialog.m_my_share = 0;
    dialog.m_dirty = true;
    dialog.rebuildOrder();
    dialog.goToPage(SharedMnCreateDialog::PageSecret);
    QCOMPARE(int(dialog.currentPage()), int(SharedMnCreateDialog::PageSecret));
    QVERIFY(dialog.secretGateRequired());
    QVERIFY(dialog.operatorSecretUnsaved());

    // The session file carries only the operator public key, so saving is no
    // way past the gate
    QVERIFY(!dialog.m_save_button->isEnabled());
    QVERIFY(!dialog.m_save_button->toolTip().isEmpty());

    // Nothing may open a file dialog here: it would block with no user to
    // answer it. The timer closes one if it appears and records that it did.
    bool modal_seen{false};
    QTimer::singleShot(0, &dialog, [&modal_seen] {
        if (QWidget* const modal{QApplication::activeModalWidget()}; modal != nullptr) {
            modal_seen = true;
            modal->close();
        }
    });
    dialog.onSave();
    QCoreApplication::processEvents();
    QVERIFY2(!modal_seen, "saving must refuse before it asks for a file name");
    QCOMPARE(int(dialog.currentPage()), int(SharedMnCreateDialog::PageSecret));
    QVERIFY2(!dialog.m_error_label->text().isEmpty(), "the refusal must say why");
    QVERIFY2(dialog.m_dirty, "a refused save must not mark the session saved");

    // Confirming the secret is what releases both
    dialog.m_confirm_edit->setText(dialog.m_operator_widget->secretHex().right(4));
    QVERIFY(dialog.secretConfirmed());
    QVERIFY(!dialog.operatorSecretUnsaved());
    QVERIFY(dialog.m_save_button->isEnabled());
    QVERIFY(dialog.m_save_button->toolTip().isEmpty());
}

void SharedMnWizardTests::coordinatorCanRetryOwnApproval()
{
    TestingSetup test{CBaseChainParams::REGTEST};
    m_node.setContext(&test.m_node);
    WalletContext& context{*m_node.walletLoader().context()};

    MnShareSession invitation{InvitationSession()};
    MnShareSession draft{invitation};
    std::vector<CKey> owner_keys(3);
    QString error;
    for (int i = 0; i < 3; ++i) {
        QCOMPARE(int(draft.absorbDraftReply(DraftReply(invitation, i, FakeTxid('1' + i), &owner_keys[i]), error)),
                 int(MnShareSession::MergeResult::Merged));
    }
    // The terms are locked but the automatic approval at lock did not happen:
    // a cancelled unlock, or a "protx shared_sign" that failed
    const MnShareSession frozen{FrozenSession(draft, owner_keys, /*sign_all=*/false)};
    QCOMPARE(int(frozen.stage()), int(MnShareSession::Stage::Frozen));

    const auto wallet{MakeSpendingWallet(m_node, context, "coord", owner_keys[0])};
    QVERIFY(wallet != nullptr);
    WalletGuard guard{context, wallet};
    MasternodeTestUtil::GuiModels models{m_node};
    QVERIFY2(models.ok, qPrintable(QString::fromStdString(models.error.translated)));
    WalletModel wallet_model(interfaces::MakeWallet(context, wallet), models.client);

    SharedMnCreateDialog dialog(m_node, &wallet_model, /*parent=*/nullptr);
    dialog.m_session = frozen;
    dialog.m_role = SharedMnCreateDialog::Role::Coordinator;
    dialog.m_my_share = 0;
    dialog.rebuildOrder();
    dialog.goToPage(SharedMnCreateDialog::PageApprovals);

    // Approving again must stay offered: "Unlock terms" is the only other way
    // on, and it discards every approval already collected
    QVERIFY(dialog.needsOwnApproval());
    QVERIFY(!dialog.m_next_button->isHidden());
    QVERIFY2(dialog.m_next_button->isEnabled(), qPrintable(dialog.m_next_button->toolTip()));

    // Once our own approval is in, the page waits for the others again
    std::vector<unsigned char> signature;
    QVERIFY(CHashSigner::SignHash(uint256S(dialog.m_session.consentHash().toStdString()), owner_keys[0], signature));
    QVERIFY2(dialog.m_session.addSignature(0, QString::fromStdString(EncodeBase64(signature)), error),
             qPrintable(error));
    dialog.updateButtons();
    QVERIFY(!dialog.needsOwnApproval());
    QVERIFY(dialog.m_next_button->isHidden());

    // With every approval in, the button is the combine/sign retry again
    for (int i = 1; i < 3; ++i) {
        std::vector<unsigned char> reply;
        QVERIFY(CHashSigner::SignHash(uint256S(dialog.m_session.consentHash().toStdString()), owner_keys[i], reply));
        QVERIFY2(dialog.m_session.addSignature(i, QString::fromStdString(EncodeBase64(reply)), error),
                 qPrintable(error));
    }
    dialog.updateButtons();
    QVERIFY(dialog.allApproved());
    QVERIFY(!dialog.m_next_button->isHidden());
    QVERIFY2(dialog.m_next_button->isEnabled(), qPrintable(dialog.m_next_button->toolTip()));
}

void SharedMnWizardTests::participantsPageGating()
{
    TestingSetup test{CBaseChainParams::REGTEST};
    m_node.setContext(&test.m_node);
    SharedMnCreateDialog dialog(m_node, /*wallet_model=*/nullptr, /*parent=*/nullptr);
    dialog.m_role = SharedMnCreateDialog::Role::Coordinator;
    dialog.m_session = InvitationSession();
    dialog.m_my_share = -1;
    dialog.rebuildOrder();
    dialog.goToPage(SharedMnCreateDialog::PageParticipants);

    QString error;
    // Everything adds up, but nobody said which row is this wallet
    QVERIFY(!dialog.validatePage(SharedMnCreateDialog::PageParticipants, error));
    QVERIFY(!error.isEmpty());
    dialog.m_my_share = 0;
    QVERIFY2(dialog.validatePage(SharedMnCreateDialog::PageParticipants, error), qPrintable(error));

    // An unnamed participant is named in the error, not silently accepted
    dialog.m_session.shares()[1].label.clear();
    dialog.refreshShareTable();
    QVERIFY(!dialog.validatePage(SharedMnCreateDialog::PageParticipants, error));
    QVERIFY(error.contains(QStringLiteral("2")));

    // Two participants cannot share a name: the contributions are keyed by it
    dialog.m_session.shares()[1].label = dialog.m_session.shares()[0].label;
    dialog.refreshShareTable();
    QVERIFY(!dialog.validatePage(SharedMnCreateDialog::PageParticipants, error));
    QVERIFY(error.contains(dialog.m_session.shares()[0].label));

    // Amounts that do not add up to the collateral are refused with both totals
    dialog.m_session.shares()[1].label = QStringLiteral("bob");
    dialog.m_session.shares()[2].amount = 100 * COIN;
    dialog.refreshShareTable();
    QVERIFY(!dialog.validatePage(SharedMnCreateDialog::PageParticipants, error));
    QVERIFY(!error.isEmpty());
}

void SharedMnWizardTests::statusBoardAbsorbsRepliesInAnyOrder()
{
    TestingSetup test{CBaseChainParams::REGTEST};
    m_node.setContext(&test.m_node);
    SharedMnCreateDialog dialog(m_node, /*wallet_model=*/nullptr, /*parent=*/nullptr);

    MnShareSession invitation{InvitationSession()};
    const MnShareSession alice{DraftReply(invitation, 0, FakeTxid('1'))};
    dialog.m_session = alice; // the coordinator answered its own invitation first
    dialog.m_role = SharedMnCreateDialog::Role::Coordinator;
    dialog.m_my_share = 0;
    dialog.rebuildOrder();
    dialog.goToPage(SharedMnCreateDialog::PageInvite);

    QVERIFY(!dialog.m_boards.empty());
    SharedMnStatusBoard* board{dialog.m_boards.front().second};
    QCOMPARE(board->rowCount(), 3);
    QCOMPARE(board->columnCount(), 4);
    QCOMPARE(int(board->cellState(0, 0)), int(SharedMnStatusBoard::State::Done));
    QCOMPARE(int(board->cellState(1, 0)), int(SharedMnStatusBoard::State::Pending));
    QVERIFY(board->rowName(0).contains(QStringLiteral("(you)")));

    // Carol answers before Bob: the board must not care
    dialog.handleImportedText(DraftReply(invitation, 2, FakeTxid('3')).toJsonString());
    QCOMPARE(int(board->cellState(2, 0)), int(SharedMnStatusBoard::State::Done));
    QCOMPARE(int(board->cellState(2, 1)), int(SharedMnStatusBoard::State::Done));
    QCOMPARE(int(board->cellState(1, 0)), int(SharedMnStatusBoard::State::Pending));
    QVERIFY(!dialog.allDetailsCollected());
    // The board names who answered; the line is not repeated under the page
    QVERIFY(board->lastReceived().contains(QStringLiteral("carol")));
    QVERIFY(board->lastReceived().contains(dialog.m_session.fingerprint()));
    QVERIFY(dialog.m_status_label->text().isEmpty());

    const MnShareSession bob{DraftReply(invitation, 1, FakeTxid('2'))};
    dialog.handleImportedText(bob.toJsonString());
    QCOMPARE(int(board->cellState(1, 0)), int(SharedMnStatusBoard::State::Done));
    QVERIFY(dialog.allDetailsCollected());
    QCOMPARE(int(dialog.currentPage()), int(SharedMnCreateDialog::PageInvite));

    // Re-pasting a reply already in hand changes nothing and is not an error:
    // every other copy of the session would look stale if it bumped the
    // revision
    const int revision{dialog.m_session.revision()};
    dialog.handleImportedText(bob.toJsonString());
    QVERIFY(dialog.m_error_label->text().isEmpty());
    QCOMPARE(dialog.m_session.revision(), revision);

    // A second, different answer for a row that already has one is refused by
    // name rather than silently overwriting what the first reply sent
    dialog.handleImportedText(DraftReply(invitation, 1, FakeTxid('4')).toJsonString());
    QVERIFY(dialog.m_error_label->text().contains(QStringLiteral("bob")));
    QCOMPARE(dialog.m_session.revision(), revision);
}

void SharedMnWizardTests::pastedMessageRouting()
{
    TestingSetup test{CBaseChainParams::REGTEST};
    m_node.setContext(&test.m_node);
    SharedMnCreateDialog dialog(m_node, /*wallet_model=*/nullptr, /*parent=*/nullptr);

    // Nothing recognisable: one sentence, and the dialog stays where it was
    dialog.handleImportedText(QStringLiteral("not a shared masternode message"));
    QCOMPARE(int(dialog.currentPage()), int(SharedMnCreateDialog::PageLanding));
    QVERIFY(!dialog.m_error_label->text().isEmpty());
    QCOMPARE(int(dialog.m_role), int(SharedMnCreateDialog::Role::Undecided));

    // Somebody else's message kind: this dialog says where it belongs rather
    // than trying to parse it
    UniValue sigs(UniValue::VOBJ);
    sigs.pushKV("type", "dash-shared-mn-sigs");
    sigs.pushKV("kind", "dissolve");
    sigs.pushKV("proTxHash", uint256::ONE.ToString());
    dialog.handleImportedText(QString::fromStdString(sigs.write()));
    QCOMPARE(int(dialog.currentPage()), int(SharedMnCreateDialog::PageLanding));
    QVERIFY(!dialog.m_error_label->text().isEmpty());
    QCOMPARE(int(dialog.m_role), int(SharedMnCreateDialog::Role::Undecided));

    // A message far larger than any envelope is refused before it is parsed
    dialog.handleImportedText(QString(3 * 1024 * 1024, QLatin1Char('x')));
    QVERIFY(dialog.m_error_label->text().contains(QStringLiteral("too large")));
    QCOMPARE(int(dialog.m_role), int(SharedMnCreateDialog::Role::Undecided));

    // Locked terms land on the approvals page, not on the drafting pages
    MnShareSession invitation{InvitationSession()};
    MnShareSession draft{invitation};
    std::vector<CKey> owner_keys(3);
    QString error;
    for (int i = 0; i < 3; ++i) {
        QVERIFY(draft.absorbDraftReply(DraftReply(invitation, i, FakeTxid('1' + i), &owner_keys[i]), error) ==
                MnShareSession::MergeResult::Merged);
    }
    const MnShareSession frozen{FrozenSession(draft, owner_keys, /*sign_all=*/false)};
    QCOMPARE(int(frozen.stage()), int(MnShareSession::Stage::Frozen));
    dialog.handleImportedText(frozen.toJsonString());
    QCOMPARE(int(dialog.currentPage()), int(SharedMnCreateDialog::PageApprovals));
    QCOMPARE(int(dialog.m_role), int(SharedMnCreateDialog::Role::Participant));
    QVERIFY(dialog.m_approvals_terms->text().contains(frozen.sessionCode()));

    // An envelope whose text was edited after it was copied still imports, but
    // the warning survives the page change: it stays on the status line while
    // the "Received …" line lives on the board
    UniValue edited{frozen.toJson()};
    edited.pushKV("fingerprint", "0000-0000");
    SharedMnCreateDialog warned(m_node, /*wallet_model=*/nullptr, /*parent=*/nullptr);
    warned.handleImportedText(QString::fromStdString(edited.write(/*prettyIndent=*/2)));
    QCOMPARE(int(warned.currentPage()), int(SharedMnCreateDialog::PageApprovals));
    QVERIFY2(warned.m_status_label->text().contains(QStringLiteral("edited after it was copied")),
             qPrintable(warned.m_status_label->text()));
    const auto board{std::find_if(warned.m_boards.begin(), warned.m_boards.end(), [](const auto& entry) {
        return entry.first == SharedMnCreateDialog::PageApprovals;
    })};
    QVERIFY(board != warned.m_boards.end());
    QVERIFY(board->second->lastReceived().contains(QStringLiteral("Received")));
}

void SharedMnWizardTests::copyPutsFingerprintedEnvelopeOnClipboard()
{
    TestingSetup test{CBaseChainParams::REGTEST};
    m_node.setContext(&test.m_node);
    SharedMnCreateDialog dialog(m_node, /*wallet_model=*/nullptr, /*parent=*/nullptr);
    dialog.m_session = DraftReply(InvitationSession(), 0, FakeTxid('1'));
    dialog.m_role = SharedMnCreateDialog::Role::Coordinator;
    dialog.m_my_share = 0;
    dialog.rebuildOrder();
    dialog.goToPage(SharedMnCreateDialog::PageInvite);

    dialog.copySession(QStringLiteral("Invitation"));
    const QString code{dialog.m_session.fingerprint()};
    QVERIFY(!code.isEmpty());
    QCOMPARE(dialog.m_sent_code, code);

    // The line under the buttons names the session and the exact message, so
    // two people can check by voice that they hold the same one
    QVERIFY(dialog.m_status_label->text().contains(dialog.m_session.sessionCode()));
    QVERIFY(dialog.m_status_label->text().contains(code));

    // What was copied is a complete envelope that parses back to this session
    MnShareSession round_trip;
    QString error;
    QVERIFY2(round_trip.fromJson(QApplication::clipboard()->text().toStdString(), error), qPrintable(error));
    QCOMPARE(round_trip.sessionId(), dialog.m_session.sessionId());
    QCOMPARE(round_trip.fingerprint(), code);
    QVERIFY(round_trip.importWarning().isEmpty());
}

void SharedMnWizardTests::unlockingDiscardsApprovals()
{
    TestingSetup test{CBaseChainParams::REGTEST};
    m_node.setContext(&test.m_node);
    SharedMnCreateDialog dialog(m_node, /*wallet_model=*/nullptr, /*parent=*/nullptr);

    MnShareSession invitation{InvitationSession()};
    MnShareSession draft{invitation};
    std::vector<CKey> owner_keys(3);
    QString error;
    for (int i = 0; i < 3; ++i) {
        QVERIFY(draft.absorbDraftReply(DraftReply(invitation, i, FakeTxid('1' + i), &owner_keys[i]), error) ==
                MnShareSession::MergeResult::Merged);
    }
    dialog.m_session = FrozenSession(draft, owner_keys, /*sign_all=*/true);
    dialog.m_role = SharedMnCreateDialog::Role::Coordinator;
    dialog.m_my_share = 0;
    dialog.rebuildOrder();
    dialog.goToPage(SharedMnCreateDialog::PageApprovals);

    QCOMPARE(dialog.m_session.signedCount(), 3);
    QVERIFY(dialog.allApproved());
    SharedMnStatusBoard* board{dialog.m_boards.at(1).second};
    for (int row = 0; row < 3; ++row) {
        QCOMPARE(int(board->cellState(row, 2)), int(SharedMnStatusBoard::State::Done));
    }

    // Unlocking is the one way back, and it costs every approval collected so
    // far: they were made over a transaction that no longer exists
    dialog.m_session.unfreeze();
    dialog.refreshAll();
    QCOMPARE(dialog.m_session.signedCount(), 0);
    QVERIFY(!dialog.allApproved());
    QCOMPARE(int(dialog.m_session.stage()), int(MnShareSession::Stage::Draft));
    for (int row = 0; row < 3; ++row) {
        QCOMPARE(int(board->cellState(row, 2)), int(SharedMnStatusBoard::State::Pending));
        // The details and coins everybody sent survive; only consent is gone
        QCOMPARE(int(board->cellState(row, 0)), int(SharedMnStatusBoard::State::Done));
        QCOMPARE(int(board->cellState(row, 1)), int(SharedMnStatusBoard::State::Done));
    }
    QVERIFY(dialog.allDetailsCollected());
}
