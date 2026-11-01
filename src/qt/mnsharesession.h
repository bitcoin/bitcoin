// Copyright (c) 2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_MNSHARESESSION_H
#define BITCOIN_QT_MNSHARESESSION_H

#include <consensus/amount.h>

#include <qt/bitcoinunits.h>

#include <univalue.h>

#include <QString>
#include <QStringList>

#include <string>
#include <vector>

class CProRegTx;
struct CMutableTransaction;

//! The one amount formatter for every shared-masternode screen. It lives next
//! to the engine rather than in qt/sharedmnwidgets.h because the engine's own
//! validation messages quote amounts and must not depend on the widgets.
//! Callers with a wallet should pass SharedMnDisplayUnit(wallet_model); the
//! engine has no options model and formats in DASH.
QString SharedMnFormatAmount(BitcoinUnits::Unit unit, CAmount amount);

//! Pick the singular or the plural wording for `count`, and substitute it into
//! the plural form's %1.
//!
//! Qt's own "%n form(s)" needs a loaded translation catalogue to choose a form;
//! with none installed it emits the source text verbatim, so the user reads a
//! literal "1 coin(s)". Mark both literals with QT_TRANSLATE_NOOP in the
//! "MnShareSession" context so lupdate still collects them.
QString SharedMnPlural(qint64 count, const char* singular, const char* plural);

//! Helpers shared by every shared-masternode envelope: the session envelope
//! owned by MnShareSession and the maintenance "dash-shared-mn-sigs" envelope
//! built by the shared masternode dialogs. A fingerprint is a short code two
//! people can read out to each other to confirm they are looking at the same
//! message, without comparing multi-kilobyte JSON by eye.
namespace shared_mn {
//! Fingerprint of `json`: the first four bytes of SHA256 over the UTF-8 of
//! `json.write(/*prettyIndent=*/2)`, rendered as eight uppercase hex digits
//! with a hyphen after the first four ("AB12-CD34"). A "fingerprint" key
//! already on `json` is excluded from the hash, so stamping is idempotent and
//! an envelope can be re-checked as received.
QString EnvelopeFingerprint(const UniValue& json);
//! Append "fingerprint" as the last key of `json`, computed over `json` as it
//! stands. Call this once, after every other key has been pushed.
void AppendFingerprint(UniValue& json);
//! Compare the "fingerprint" key of `json` against the value recomputed from
//! the rest of the object. True when they agree or when the key is absent (an
//! envelope written by a build that did not stamp one); false with `warning`
//! set to a user-facing sentence when they differ, which means the text was
//! altered between copy and paste.
bool CheckFingerprint(const UniValue& json, QString& warning);
} // namespace shared_mn

//! Pure-logic engine (no widgets, no QObject) for a shared-masternode
//! multi-party session. It owns the session envelope: a JSON file passed
//! between participants that fully describes the session, so any participant
//! holding the latest copy can continue it.
//!
//! Envelope shape (UniValue JSON, version 1):
//! {type, version, network, sessionId, revision, stage, fundingTx, shares[],
//!  terms{}, contributions[], protx, consentHash, collateralIndex, sigs[],
//!  prepareWallet, operatorSecretHolder}
//!
//! All amounts are duffs (CAmount). All user-displayable errors come back as
//! translated QStrings. Signature verification is native (recompute the
//! consent digest from the envelope's protx hex and check each compact
//! signature against the share owner key). fromJson() verifies every
//! signature once the envelope claims Stage::Combined or later; before that
//! it stores them as-is and callers run dropUnverifiedSignatures() before
//! counting, which is what the dialogs do on import.
class MnShareSession
{
public:
    enum class Stage {
        Draft,         //!< terms and contributions still editable
        Frozen,        //!< shared_register_prepare done; consent hash fixed
        Signing,       //!< collecting share owner consent signatures
        Combined,      //!< shared_combine done; join sigs embedded
        FundingSigned, //!< every funding input carries its signature
        Broadcast,     //!< sent to the network
    };

    enum class MergeResult {
        Merged,     //!< envelopes were compatible; new data was absorbed
        OtherNewer, //!< the imported envelope supersedes ours; caller decides
        OtherOlder, //!< ours supersedes the imported one; nothing absorbed
        Conflict,   //!< incompatible envelopes; see the error string
    };

    struct Share {
        QString label;
        CAmount amount{0};
        QString ownerAddress;  //!< P2PKH only (immutable share owner key)
        QString refundAddress; //!< P2PKH or P2SH, immutable
        QString rewardAddress; //!< empty = fall back to the refund address
    };

    struct Input {
        QString txid; //!< 64 hex chars
        uint32_t vout{0};
        //! Fixed at contribution time and never rewritten afterwards: the
        //! consent digest covers every input's nSequence, so a rewrite after
        //! freeze would invalidate all collected signatures.
        uint32_t sequence{0xffffffff};
    };

    struct Contribution {
        QString label;
        std::vector<Input> inputs;
        bool hasChange{false};
        QString changeAddress;
        CAmount changeAmount{0};
        //! Output index of the change in the built funding transaction;
        //! recomputed whenever the funding transaction is rebuilt
        int changeIndex{-1};
    };

    struct Terms {
        QString coreP2PAddrs;   //!< ADDR:PORT, or empty for a later ProUpServTx
        QString operatorPubKey; //!< hex BLS public key (basic scheme)
        QString votingAddress;  //!< P2PKH
        int operatorReward{0};  //!< 0..10000, in 1/100 of a percent
        uint32_t earlyPeriodBlocks{0};
        CAmount earlyPenalty{0};
    };

    struct Signature {
        int shareIndex{-1};
        QString signatureB64; //!< base64 65-byte compact signature
    };

    //! Mirror of the BuildProDisTx output math for previewing a unilateral
    //! dissolution before it is built
    struct PenaltyPreview {
        bool valid{false};
        QString error;
        bool early{false};
        CAmount penalty{0};
        //! First block height at which a unilateral dissolution is penalty-free
        int penaltyFreeHeight{0};
        //! Refund per share index. The actor's entry is principal minus
        //! penalty minus fee (a zero entry is omitted from the real
        //! transaction); every other entry is principal plus its pro-rata
        //! penalty bonus (sequential floor, remainder to the last non-actor).
        std::vector<CAmount> payouts;
    };

    //! New Draft session: random sessionId, revision 1, current network
    MnShareSession();

    Stage stage() const { return m_stage; }
    static QString StageName(Stage stage);

    const QString& sessionId() const { return m_session_id; }
    //! Short, human-checkable form of the session id: its first six hex
    //! characters, uppercased ("3F9A2C"). It names the session for the whole
    //! of its life and never changes.
    QString sessionCode() const { return m_session_id.left(6).toUpper(); }
    const QString& network() const { return m_network; }
    int revision() const { return m_revision; }

    //! Fingerprint of this session's current envelope. It changes whenever any
    //! field changes, so it identifies one exact message rather than the
    //! session as a whole.
    QString fingerprint() const;
    //! Non-empty when the last fromJson() found a "fingerprint" that did not
    //! match the rest of the message. Parsing still succeeded — the caller
    //! should show this next to whatever it imported.
    const QString& importWarning() const { return m_import_warning; }

    std::vector<Share>& shares() { return m_shares; }
    const std::vector<Share>& shares() const { return m_shares; }
    Terms& terms() { return m_terms; }
    const Terms& terms() const { return m_terms; }
    const std::vector<Contribution>& contributions() const { return m_contributions; }

    const QString& fundingTxHex() const { return m_funding_tx; }
    const QString& protxHex() const { return m_protx; }
    const QString& consentHash() const { return m_consent_hash; }
    int collateralIndex() const { return m_collateral_index; }

    const QString& prepareWallet() const { return m_prepare_wallet; }
    void setPrepareWallet(const QString& name) { m_prepare_wallet = name; }
    const QString& operatorSecretHolder() const { return m_operator_secret_holder; }
    void setOperatorSecretHolder(const QString& label) { m_operator_secret_holder = label; }
    //! Display name of the participant coordinating the session, so a waiting
    //! participant can be told "Waiting for Alice" rather than "the
    //! coordinator". Purely informational and optional on import.
    const QString& coordinatorLabel() const { return m_coordinator_label; }
    void setCoordinatorLabel(const QString& label) { m_coordinator_label = label; }

    //! Record a direct edit to draft shares or terms so a circulated baton
    //! supersedes older copies even when no funding contribution changed.
    void noteDraftChange();

    //! Serialize the envelope. "fingerprint" is appended last and covers every
    //! other key, so it changes with any edit to the message.
    UniValue toJson() const;
    QString toJsonString() const;

    //! Strict parse of an envelope. The network must match the running node
    //! (hard error naming both networks) and the stage must be known. A
    //! "fingerprint" that disagrees with the rest of the message is not a
    //! parse failure; it is reported through importWarning().
    //! Signatures are stored as-is before Stage::Combined; callers must run
    //! dropUnverifiedSignatures() (or rely on addSignature/mergeEnvelope,
    //! which verify) before counting them.
    bool fromJson(const UniValue& json, QString& error);
    bool fromJson(const std::string& text, QString& error);

    //! The draft share table checked by IsShareListTriviallyValid itself, after
    //! the address strings are parsed (empty list = valid). Parse problems are
    //! reported per share; a consensus rejection is reported as one message.
    QStringList validateShares() const;

    //! User-facing text for a reject reason of IsShareListTriviallyValid or
    //! IsEarlyPenaltyValid. min_share_amount and early_period_blocks pick the
    //! wording of the penalty message.
    static QString ShareListErrorText(const std::string& reject_reason, BitcoinUnits::Unit unit,
                                      CAmount min_share_amount, uint32_t early_period_blocks);
    //! Early-exit penalty rules for the share amounts alone (empty = valid)
    static QString CheckEarlyPenalty(const std::vector<CAmount>& share_amounts, BitcoinUnits::Unit unit,
                                     uint32_t early_period_blocks, CAmount early_penalty);
    //! AreShareAmountsValid for the amounts entered before any address
    //! exists (empty = valid); validateShares() runs the full list later
    static QString CheckShareAmounts(const std::vector<CAmount>& share_amounts, BitcoinUnits::Unit unit);

    //! Draft only: record a participant's funding contribution and rebuild
    //! the funding transaction deterministically (inputs, then change
    //! outputs, both in contribution order). Bumps the revision.
    bool addContribution(const Contribution& contribution, QString& error);
    //! Draft only: remove a contribution by label and rebuild. Bumps the revision.
    bool removeContribution(const QString& label, QString& error);

    //! Record the shared_register_prepare result and advance Draft -> Frozen.
    //! Cross-checks that the protx hex parses as a shared registration and
    //! that its recomputed consent digest equals consent_hash_hex.
    bool freeze(const QString& protx_hex, const QString& consent_hash_hex, int collateral_index, QString& error);
    //! Discard the frozen transaction and all signatures, return to Draft and
    //! bump the revision (previously collected signatures can never apply again)
    void unfreeze();

    //! Verify one base64 compact signature against the frozen transaction:
    //! recompute the consent digest from the protx hex, hard-compare it to
    //! the stored consentHash, then check the signature against the share
    //! owner key (canonical encoding required)
    bool verifySignature(int share_index, const QString& sig_b64, QString& error) const;

    //! Store a signature after verifying it. A byte-identical duplicate is a
    //! silent success; a different signature for an already-signed index is
    //! rejected as a stale-version conflict.
    bool addSignature(int share_index, const QString& sig_b64, QString& error);
    int signedCount() const;
    //! Base64 signature for a share index, or empty if not signed yet
    QString signatureFor(int share_index) const;
    //! The stored signatures as the envelope's [{shareIndex, signature}] array
    UniValue signaturesJson() const;
    //! Drop every stored signature that no longer verifies (an imported copy
    //! stores them as-is). Returns one sentence per dropped signature.
    QStringList dropUnverifiedSignatures();
    //! Replace the registration transaction with a copy whose funding inputs
    //! carry more signatures, keeping the stage. Everything the consent digest
    //! and the displayed session cover must be unchanged.
    bool replaceProTx(const QString& tx_hex, QString& error);

    //! Absorb one participant's reply to a circulated invitation: their share
    //! row (owner, refund and reward addresses) and their funding
    //! contribution. Both copies must be Draft copies of the same session
    //! whose share labels, share amounts and terms agree; anything else is a
    //! Conflict with `error` set, and *this is left untouched.
    //!
    //! This is what makes the details round parallel. Every participant
    //! answers the same invitation independently, and the coordinator absorbs
    //! the replies in whatever order they arrive: a share row is only ever
    //! filled in from empty, so two replies can never overwrite each other,
    //! and the resulting funding transaction is the same regardless of the
    //! order the replies were pasted in (contributions are ordered by their
    //! share, not by arrival).
    //!
    //! Returns Merged (the only non-Conflict result) and bumps the revision
    //! once when anything was absorbed. Re-absorbing the same reply is a
    //! no-op success.
    MergeResult absorbDraftReply(const MnShareSession& other, QString& error);

    //! Replace this Draft copy with the coordinator's locked terms, after
    //! checking that they are the agreement this copy answered: the
    //! masternode-wide terms and every row's label and amount must be
    //! unchanged, and every share row this copy filled in (any row with a
    //! non-empty owner address) and every contribution this copy holds must
    //! appear unchanged in `other`. `other` must be Frozen or Signing and
    //! belong to the same session.
    //!
    //! A participant must never be nudged into approving terms that quietly
    //! changed their own addresses or coins, so a mismatch is refused rather
    //! than merged.
    bool adoptLockedTerms(const MnShareSession& other, QString& error);

    //! Absorb another copy of the same session. Never merges across
    //! diverging terms: a higher/lower revision or a consent-hash mismatch is
    //! reported to the caller (supersede dialog) instead. On Merged, the
    //! union of the other envelope's *verified* signatures is absorbed and a
    //! further-advanced stage (with its transaction hex) is adopted; error
    //! carries any skipped-signature warnings even on success.
    MergeResult mergeEnvelope(const MnShareSession& other, QString& error);

    //! Record the shared_combine result and advance to Combined
    bool setCombinedTx(const QString& tx_hex, QString& error);
    //! Record the fully signed funding transaction and advance to FundingSigned
    bool setFundingSignedTx(const QString& tx_hex, QString& error);
    //! Mark the session broadcast
    bool setBroadcast(QString& error);

    //! Approximate duration of a block count at 2.5 minutes per block
    static QString HumanEarlyPeriod(uint32_t blocks);

    //! Preview of a unilateral dissolution built at chain height at_height
    //! (the transaction would confirm at at_height + 1)
    static PenaltyPreview PenaltyPreviewFor(const std::vector<CAmount>& share_amounts, int actor_index,
                                            CAmount early_penalty, uint32_t early_period_blocks, CAmount fee,
                                            int at_height, int registered_height);

private:
    //! The envelope without its "fingerprint" key; toJson() stamps the
    //! fingerprint onto this and fingerprint() hashes it
    UniValue toJsonBody() const;
    //! Rebuild m_funding_tx from m_contributions (deterministic order) and
    //! refresh each contribution's changeIndex
    bool rebuildFundingTx(QString& error);
    const Signature* findSignature(int share_index) const;
    //! Decode `protx_hex` as a shared registration whose recomputed consent
    //! digest is this session's consentHash, i.e. a transaction everyone's
    //! approval still covers
    bool decodeFrozen(const QString& protx_hex, CMutableTransaction& tx, CProRegTx& payload, QString& error) const;
    //! Decode m_protx and require it to be exactly what the envelope displays:
    //! its CProRegTx payload's share table and terms must match
    //! m_shares/m_terms, and its inputs and outputs must be the funding
    //! m_contributions describes plus the shared collateral output at
    //! m_collateral_index. The displayed session is what a participant reviews
    //! before signing, but the wallet signs the digest of the *payload* and the
    //! scripts of the *transaction*, so a mismatch means the file shows one
    //! agreement while a different one would be registered — it must never
    //! verify.
    bool payloadMatchesEnvelope(QString& error) const;
    //! True when the draft-editable state (shares, terms, contributions) of two
    //! copies is identical; used to reject silent merges of diverging drafts
    bool sameDraftState(const MnShareSession& other) const;
    //! True when two copies agree on every masternode-wide term
    bool sameTerms(const MnShareSession& other) const;

    QString m_network;
    QString m_session_id;
    int m_revision{1};
    Stage m_stage{Stage::Draft};
    QString m_funding_tx;
    std::vector<Share> m_shares;
    Terms m_terms;
    std::vector<Contribution> m_contributions;
    QString m_protx;
    QString m_consent_hash;
    int m_collateral_index{-1};
    std::vector<Signature> m_sigs;
    QString m_prepare_wallet;
    QString m_operator_secret_holder;
    QString m_coordinator_label;
    QString m_import_warning;
};

#endif // BITCOIN_QT_MNSHARESESSION_H
