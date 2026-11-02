// Copyright (c) 2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_SHAREDMNWIDGETS_H
#define BITCOIN_QT_SHAREDMNWIDGETS_H

#include <consensus/amount.h>
#include <interfaces/node.h>
#include <primitives/transaction.h>

#include <qt/bitcoinunits.h>
#include <qt/mnsharesession.h>

#include <QString>
#include <QStringList>
#include <QWidget>

#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

struct CMutableTransaction;
class WalletModel;

QT_BEGIN_NAMESPACE
class QDialog;
class QLabel;
class QPushButton;
class QTableWidget;
QT_END_NAMESPACE

//! Display unit of `wallet_model`, falling back to DASH when there is no
//! wallet or no options model. Pass the result to SharedMnFormatAmount() and
//! SharedMnTermSheetHtml() so every shared-masternode screen shows amounts in
//! the unit the user picked.
//!
//! Amounts themselves are formatted by SharedMnFormatAmount(), declared in
//! qt/mnsharesession.h: the session engine needs it for its own validation
//! messages and cannot include this header without a dependency cycle.
BitcoinUnits::Unit SharedMnDisplayUnit(const WalletModel* wallet_model);

//! True when `wallet_model` holds `share`'s owner key, which is what makes a
//! share this wallet's on every shared-masternode screen. A watch-only wallet
//! holds keys too; anything that signs must also check canSign().
bool SharedMnWalletOwnsShare(const WalletModel* wallet_model, const interfaces::MnShare& share);

//! The one wording for the v24 activation gate
QString SharedMnV24InactiveMessage();

//! Broadcast a fully signed transaction under the same fee cap "sendrawtransaction"
//! applies. Returns why it was rejected, ready to show to the user, or an empty
//! string once it is in the mempool.
QString SharedMnBroadcast(interfaces::Node& node, const CTransactionRef& tx);

//! What a ProDisTx pays out, read against the share table of the masternode it
//! dissolves.
struct SharedMnDissolution {
    //! True when the transaction is exactly the penalty-free template "protx
    //! shared_dissolve_prepare" builds: one input, one output per non-actor
    //! share in share order paying its full principal to its own refund
    //! script, and an optional final output returning the actor's principal
    //! minus the fee.
    bool returnsPrincipal{false};
    //! Why it is not that template, in words the person asked to approve it
    //! can act on; empty when it is
    QString error;
    //! Total share principal minus total outputs
    CAmount fee{0};
    //! What each share receives, in share order; empty unless `returnsPrincipal`
    std::vector<CAmount> payouts;
};

//! Read `tx` as a dissolution of the masternode whose share table is `shares`,
//! with `actor_index` taken from its payload.
//!
//! In unanimous mode consensus allows any payout the share owners all sign for,
//! so a preparer is free to build a "dissolution" that pays a participant
//! nothing and everything to somebody else. Every unanimous dissolution the
//! GUI and the RPCs build is the penalty-free principal-returning template, so
//! a wallet asked to approve anything else refuses instead of showing numbers
//! nobody reads.
SharedMnDissolution SharedMnReadDissolution(const CMutableTransaction& tx, uint16_t actor_index,
                                            const std::vector<interfaces::MnShare>& shares);

//! One row per participant with a tick column per step of the session, so
//! everybody can see at a glance who has answered and who has not.
//!
//! The board is filled by the owning dialog: setShares() defines the rows,
//! setColumns() the steps, and setCell() the state of one step for one
//! participant. Participant names arrive in an envelope written by somebody
//! else, so they are untrusted text and are only ever rendered as
//! QTableWidgetItem text - never as rich text.
class SharedMnStatusBoard : public QWidget
{
    Q_OBJECT

public:
    //! State of one step for one participant
    enum class State {
        Pending, //!< nothing received yet ("—")
        Done,    //!< received and accepted ("✓")
        Problem, //!< received but unusable, or refused ("⚠"); the tooltip says why
    };

    struct Row {
        QString name;    //!< participant's display name (untrusted text)
        CAmount amount{0};
        QString tooltip; //!< full text behind a shortened name, empty for none
    };

    explicit SharedMnStatusBoard(QWidget* parent = nullptr);

    //! Unit used to render the Amount column (default DASH)
    void setDisplayUnit(BitcoinUnits::Unit unit);
    //! Titles of the step columns, in order, e.g. {"Details", "Funded",
    //! "Approved", "Signed"}. Every cell resets to Pending.
    void setColumns(const QStringList& titles);
    //! Participants, in share order. Every cell resets to Pending.
    void setShares(const std::vector<Row>& rows);
    //! Mark the row this wallet holds: its name gains " (you)" and the row is
    //! bold. Pass -1 for "none of these are me".
    void setYouRow(int row);
    //! State of step `column` (an index into setColumns(), not a table column)
    //! for participant `row`. Out-of-range indexes are ignored.
    void setCell(int row, int column, State state, const QString& tooltip = QString());
    //! One-line progress summary under the table, e.g. "2 of 3 approved"
    void setSummary(const QString& text);
    //! One-line note about the most recent import, e.g.
    //! "Received Carol's Approval - Code 9A0B-11C2". Empty hides the line.
    void setLastReceived(const QString& text);

    int rowCount() const;
    int columnCount() const; //!< number of step columns
    //! State of step `column` for participant `row`; Pending when out of range
    State cellState(int row, int column) const;
    //! Name shown for `row`, including the " (you)" suffix; empty out of range
    QString rowName(int row) const;
    //! Text of the last-received line, empty when no message has been shown
    QString lastReceived() const;

private:
    void rebuild();

    QTableWidget* m_table{nullptr};
    QLabel* m_summary{nullptr};
    QLabel* m_last_received{nullptr};
    QStringList m_columns;
    std::vector<Row> m_rows;
    std::vector<std::vector<State>> m_states;
    int m_you_row{-1};
    BitcoinUnits::Unit m_unit{BitcoinUnits::Unit::DASH};
};

//! Largest message any shared-masternode screen will read from the clipboard
//! or from a file. Every envelope is a few kilobytes of JSON; the cap is there
//! so a hostile or truncated paste cannot freeze the GUI thread in the parser
//! or in widget construction.
constexpr qint64 MAX_ENVELOPE_FILE_BYTES{2 * 1024 * 1024};

//! Recognising a pasted message so the user never has to say what it is.
//!
//! Everything a participant can be handed - a session envelope, a maintenance
//! signing envelope, a standby dissolution - is self-describing, so one Paste
//! button can route all of them.
namespace SharedMnImport {
enum class Kind {
    Session,    //!< a "dash-shared-mn-session" envelope (MnShareSession)
    Sigs,       //!< a "dash-shared-mn-sigs" envelope (dissolve or registrar)
    StandbyHex, //!< a raw ProDisTx: a standby dissolution to broadcast
    Unknown,    //!< nothing recognisable; `error` says so in one sentence
};

struct Detected {
    Kind kind{Kind::Unknown};
    QString proTxHash; //!< Sigs only: the masternode the envelope is about
    QString sigKind;   //!< Sigs only: "dissolve" or "registrar"
    QString error;     //!< Unknown only: one user-facing sentence
};

//! Classify pasted or opened text. This only identifies the message; the
//! owning dialog still parses and validates it (MnShareSession::fromJson(),
//! SharedSigCollector::importEnvelope(), the dissolution preview).
Detected Detect(const QString& text);
} // namespace SharedMnImport

//! Mark `button` as a secondary action: the themes paint it in the light
//! button style instead of the filled primary one, so a page has exactly one
//! filled button and Cancel is never the loudest control on screen. A
//! checkable secondary button paints filled again while it is checked, which
//! is what makes a row of them read as a segmented control.
void SharedMnMakeSecondary(QPushButton* button);

//! Let every word-wrapped QLabel below `root` report its real height.
//!
//! QLabel implements heightForWidth() but does not advertise it in its size
//! policy, so a layout reserves a single line for a wrapped label and paints
//! the next widget over the rest of the text. Opting the labels in makes a
//! page as tall as it actually is, which is what the enclosing scroll areas
//! and dialogs size themselves from.
void SharedMnFitWrappedLabels(QWidget* root);

//! Resize `dialog` to what its scrolled content asks for, never past 85% of
//! the screen and never below `minimum_width`.
//!
//! QScrollArea caps its own sizeHint at 36x24 text lines, so a dialog built
//! around one opens with its table, checkbox and submit button below the fold.
//! Add back what the scrolled widgets really want.
void SharedMnSizeFromContent(QDialog* dialog, int minimum_width);

//! What this wallet puts into the funding transaction, for the term sheet
struct SharedMnOwnFunding {
    CAmount spent{0};  //!< value of this wallet's coins among the recorded inputs
    CAmount change{0}; //!< change paid back to addresses of this wallet
    int unvalued{0};   //!< inputs of this wallet's contribution that cannot be valued here
};

//! The term sheet everyone reads before approving: participants, masternode
//! settings, exit terms, funding and identity, as five HTML cards.
//!
//! `you_share_index` is the share this wallet holds (-1 for none); that row is
//! shown bold with a "(you)" marker. `unit` formats every amount - use
//! SharedMnDisplayUnit(). `in_this_wallet`, when given, says whether this
//! wallet holds the key for an address; those addresses are marked so the
//! reader can confirm which ones are theirs. `own_funding`, when given with a
//! share of our own, adds what this wallet pays and warns when that is more
//! than 1 DASH above the share. Every value coming out of the envelope is
//! HTML-escaped, because participant names are written by other people.
QString SharedMnTermSheetHtml(const MnShareSession& session, int you_share_index, BitcoinUnits::Unit unit,
                              const std::function<bool(const QString& address)>& in_this_wallet = {},
                              const std::optional<SharedMnOwnFunding>& own_funding = std::nullopt);

#endif // BITCOIN_QT_SHAREDMNWIDGETS_H
