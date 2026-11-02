// Copyright (c) 2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/sharedmnwidgets.h>

#include <core_io.h>
#include <evo/providertx.h>
#include <interfaces/wallet.h>
#include <node/transaction.h>
#include <policy/policy.h>
#include <primitives/transaction.h>
#include <script/standard.h>
#include <util/error.h>
#include <util/strencodings.h>

#include <qt/masternodeoperationrunner.h>
#include <qt/masternodewidgets.h>
#include <qt/optionsmodel.h>
#include <qt/walletmodel.h>

#include <univalue.h>

#include <QCoreApplication>
#include <QDialog>
#include <QFont>
#include <QGuiApplication>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QScreen>
#include <QScrollArea>
#include <QScrollBar>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

#include <algorithm>
#include <string>

namespace {
//! Column layout of the board: the step columns follow these three
constexpr int COLUMN_NUMBER{0};
constexpr int COLUMN_NAME{1};
constexpr int COLUMN_AMOUNT{2};
constexpr int FIXED_COLUMNS{3};

QString StateGlyph(SharedMnStatusBoard::State state)
{
    switch (state) {
    case SharedMnStatusBoard::State::Done: return QStringLiteral("✓");
    case SharedMnStatusBoard::State::Problem: return QStringLiteral("⚠");
    case SharedMnStatusBoard::State::Pending: break;
    }
    return QStringLiteral("—");
}

QString Escaped(const QString& text)
{
    return text.toHtmlEscaped();
}

//! `text`, or an italic placeholder when it is empty
QString EscapedOr(const QString& text, const QString& placeholder)
{
    if (text.isEmpty()) return QStringLiteral("<i>%1</i>").arg(placeholder.toHtmlEscaped());
    return text.toHtmlEscaped();
}

QString Card(const QString& title, const QString& body)
{
    return QStringLiteral("<p><b>%1</b><br>%2</p>").arg(title.toHtmlEscaped(), body);
}

QString LabelledRow(const QString& label, const QString& value)
{
    return QStringLiteral("%1 %2").arg(label.toHtmlEscaped(), value);
}
} // anonymous namespace

BitcoinUnits::Unit SharedMnDisplayUnit(const WalletModel* wallet_model)
{
    if (wallet_model && wallet_model->getOptionsModel()) return wallet_model->getOptionsModel()->getDisplayUnit();
    return BitcoinUnits::Unit::DASH;
}

bool SharedMnWalletOwnsShare(const WalletModel* wallet_model, const interfaces::MnShare& share)
{
    return wallet_model != nullptr && wallet_model->wallet().isSpendable(PKHash(share.keyIDOwner));
}

QString SharedMnV24InactiveMessage()
{
    return QCoreApplication::translate("SharedMnDialog",
                                       "Shared masternodes need the v24 upgrade, which is not active on this network "
                                       "yet.");
}

QString SharedMnBroadcast(interfaces::Node& node, const CTransactionRef& tx)
{
    bilingual_str message;
    const TransactionError error{
        node.broadcastTransaction(tx, node::DEFAULT_MAX_RAW_TX_FEE_RATE.GetFee(GetVirtualTransactionSize(*tx)), message)};
    if (error == TransactionError::OK) return {};
    if (message.empty()) message = TransactionErrorString(error);
    return MasternodeOperationRunner::errorText({interfaces::ProviderTxErrorCode::BROADCAST_ERROR, message, {}, error});
}

SharedMnStatusBoard::SharedMnStatusBoard(QWidget* parent) : QWidget(parent)
{
    setObjectName(QStringLiteral("sharedMnStatusBoard"));

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(MasternodeWidgetUtil::ROW_SPACING);

    m_table = new QTableWidget(this);
    m_table->setObjectName(QStringLiteral("sharedMnStatusTable"));
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionMode(QAbstractItemView::NoSelection);
    m_table->setFocusPolicy(Qt::NoFocus);
    m_table->setAlternatingRowColors(true);
    // Every cell is a short label; wrapping only inflates the row heights when
    // the table is measured before it has its final width
    m_table->setWordWrap(false);
    m_table->verticalHeader()->setVisible(false);
    m_table->horizontalHeader()->setStretchLastSection(false);
    layout->addWidget(m_table);

    m_summary = MasternodeWidgetUtil::makeHint(QString(), this);
    m_summary->setVisible(false);
    layout->addWidget(m_summary);

    m_last_received = MasternodeWidgetUtil::makeHint(QString(), this);
    m_last_received->setVisible(false);
    layout->addWidget(m_last_received);

    // Both lines quote participant names, which are written by somebody else.
    // makeHint() leaves the default Qt::AutoText, under which "<a href=…>" or
    // "<h1>" in a name would be rendered as markup.
    for (QLabel* const label : {m_summary, m_last_received}) {
        label->setTextFormat(Qt::PlainText);
    }

    rebuild();
}

void SharedMnStatusBoard::setDisplayUnit(BitcoinUnits::Unit unit)
{
    m_unit = unit;
    rebuild();
}

void SharedMnStatusBoard::setColumns(const QStringList& titles)
{
    m_columns = titles;
    rebuild();
}

void SharedMnStatusBoard::setShares(const std::vector<Row>& rows)
{
    m_rows = rows;
    rebuild();
}

void SharedMnStatusBoard::setYouRow(int row)
{
    m_you_row = row;
    rebuild();
}

void SharedMnStatusBoard::setCell(int row, int column, State state, const QString& tooltip)
{
    if (row < 0 || static_cast<size_t>(row) >= m_states.size()) return;
    if (column < 0 || column >= m_columns.size()) return;
    m_states[row][column] = state;
    if (QTableWidgetItem* item = m_table->item(row, FIXED_COLUMNS + column)) {
        item->setText(StateGlyph(state));
        item->setToolTip(tooltip);
    }
}

void SharedMnStatusBoard::setSummary(const QString& text)
{
    m_summary->setText(text);
    m_summary->setVisible(!text.isEmpty());
}

void SharedMnStatusBoard::setLastReceived(const QString& text)
{
    m_last_received->setText(text);
    m_last_received->setVisible(!text.isEmpty());
}

int SharedMnStatusBoard::rowCount() const { return static_cast<int>(m_rows.size()); }

int SharedMnStatusBoard::columnCount() const { return m_columns.size(); }

SharedMnStatusBoard::State SharedMnStatusBoard::cellState(int row, int column) const
{
    if (row < 0 || static_cast<size_t>(row) >= m_states.size()) return State::Pending;
    if (column < 0 || column >= m_columns.size()) return State::Pending;
    return m_states[row][column];
}

QString SharedMnStatusBoard::rowName(int row) const
{
    if (row < 0 || static_cast<size_t>(row) >= m_rows.size()) return QString();
    const QTableWidgetItem* item{m_table->item(row, COLUMN_NAME)};
    return item ? item->text() : QString();
}

QString SharedMnStatusBoard::lastReceived() const { return m_last_received->text(); }

void SharedMnStatusBoard::rebuild()
{
    // Cell states belong to the board, not to the table widget, so they
    // survive a column or unit change and can be read back by the caller
    m_states.assign(m_rows.size(), std::vector<State>(m_columns.size(), State::Pending));

    QStringList headers{tr("#"), tr("Name"), tr("Amount")};
    headers += m_columns;
    m_table->clear();
    m_table->setColumnCount(headers.size());
    m_table->setHorizontalHeaderLabels(headers);
    m_table->setRowCount(static_cast<int>(m_rows.size()));

    for (int row = 0; row < static_cast<int>(m_rows.size()); ++row) {
        const Row& entry{m_rows[row]};
        const bool you{row == m_you_row};
        // Names come from an envelope somebody else wrote. Setting them as
        // item text keeps them literal; a rich-text path would let a name
        // rewrite the board.
        QString name{entry.name};
        if (you) name += QLatin1Char(' ') + tr("(you)");

        auto* number_item = new QTableWidgetItem(QString::number(row + 1));
        auto* name_item = new QTableWidgetItem(name);
        if (!entry.tooltip.isEmpty()) name_item->setToolTip(entry.tooltip);
        auto* amount_item = new QTableWidgetItem(SharedMnFormatAmount(m_unit, entry.amount));
        number_item->setTextAlignment(Qt::AlignCenter);
        amount_item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_table->setItem(row, COLUMN_NUMBER, number_item);
        m_table->setItem(row, COLUMN_NAME, name_item);
        m_table->setItem(row, COLUMN_AMOUNT, amount_item);

        for (int column = 0; column < m_columns.size(); ++column) {
            auto* item = new QTableWidgetItem(StateGlyph(State::Pending));
            item->setTextAlignment(Qt::AlignCenter);
            m_table->setItem(row, FIXED_COLUMNS + column, item);
        }
        if (you) {
            for (int column = 0; column < headers.size(); ++column) {
                QTableWidgetItem* item{m_table->item(row, column)};
                QFont font{item->font()};
                font.setBold(true);
                item->setFont(font);
            }
        }
    }

    QHeaderView* header{m_table->horizontalHeader()};
    header->setSectionResizeMode(COLUMN_NUMBER, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(COLUMN_NAME, QHeaderView::Stretch);
    header->setSectionResizeMode(COLUMN_AMOUNT, QHeaderView::ResizeToContents);
    for (int column = 0; column < m_columns.size(); ++column) {
        header->setSectionResizeMode(FIXED_COLUMNS + column, QHeaderView::ResizeToContents);
    }
    m_table->resizeRowsToContents();
}

SharedMnDissolution SharedMnReadDissolution(const CMutableTransaction& tx, uint16_t actor_index,
                                            const std::vector<interfaces::MnShare>& shares)
{
    // One wording for every way the transaction can fail to be the template:
    // the distinction between "pays the wrong amount", "pays somebody else"
    // and "keeps too much as fee" is not one the reader can act on differently
    const QString refuse{QCoreApplication::translate(
        "SharedMnWidgets", "This request does not pay every share its full principal. Do not approve it; ask "
                           "whoever prepared it for a fresh request.")};

    SharedMnDissolution result;
    if (shares.empty() || actor_index >= shares.size()) {
        result.error = QCoreApplication::translate("SharedMnWidgets",
                                                   "This request names a share this masternode does not have.");
        return result;
    }
    if (tx.vin.size() != 1) {
        result.error = QCoreApplication::translate(
            "SharedMnWidgets",
            "This request does not spend exactly the masternode's collateral. Do not approve it.");
        return result;
    }

    CAmount principal_total{0};
    for (const interfaces::MnShare& share : shares) {
        if (!MoneyRange(share.amount) || !MoneyRange(principal_total + share.amount)) {
            result.error = refuse;
            return result;
        }
        principal_total += share.amount;
    }
    CAmount output_total{0};
    for (const CTxOut& out : tx.vout) {
        if (!MoneyRange(out.nValue) || !MoneyRange(output_total + out.nValue)) {
            result.error = refuse;
            return result;
        }
        output_total += out.nValue;
    }
    result.fee = principal_total - output_total;

    const size_t actor{actor_index};
    size_t next_output{0};
    for (size_t i = 0; i < shares.size(); ++i) {
        if (i == actor) continue;
        if (next_output >= tx.vout.size() || tx.vout[next_output].nValue != shares[i].amount ||
            tx.vout[next_output].scriptPubKey != shares[i].scriptRefund) {
            result.error = refuse;
            return result;
        }
        ++next_output;
    }
    // The actor's own output is optional: "protx shared_dissolve_prepare"
    // leaves it out when the fee consumes the whole share
    if (tx.vout.size() > next_output + 1 ||
        (tx.vout.size() == next_output + 1 && tx.vout[next_output].scriptPubKey != shares[actor].scriptRefund)) {
        result.error = refuse;
        return result;
    }
    if (result.fee <= 0) {
        result.error = refuse;
        return result;
    }
    if (result.fee > CProDisTx::MAX_FEE) {
        result.error = QCoreApplication::translate(
                           "SharedMnWidgets",
                           "This request keeps %1 of the shares' principal as a transaction fee. Do not approve it; "
                           "ask whoever prepared it for a fresh request.")
                           .arg(SharedMnFormatAmount(BitcoinUnits::Unit::DASH, result.fee));
        return result;
    }

    result.returnsPrincipal = true;
    result.payouts.assign(shares.size(), 0);
    for (size_t i = 0, out = 0; i < shares.size(); ++i) {
        if (i == actor) continue;
        result.payouts[i] = tx.vout[out++].nValue;
    }
    result.payouts[actor] = shares[actor].amount - result.fee;
    return result;
}

SharedMnImport::Detected SharedMnImport::Detect(const QString& text)
{
    Detected detected;
    const QString trimmed{text.trimmed()};
    if (trimmed.isEmpty()) {
        detected.error = QCoreApplication::translate("SharedMnWidgets",
                                                     "The clipboard does not contain a shared masternode message.");
        return detected;
    }

    if (UniValue json; json.read(trimmed.toStdString()) && json.isObject()) {
        const UniValue& type{json.find_value("type")};
        const std::string type_str{type.isStr() ? type.get_str() : std::string{}};
        if (type_str == "dash-shared-mn-session") {
            detected.kind = Kind::Session;
            return detected;
        }
        if (type_str == "dash-shared-mn-sigs") {
            detected.kind = Kind::Sigs;
            if (const UniValue& v{json.find_value("proTxHash")}; v.isStr()) {
                detected.proTxHash = QString::fromStdString(v.get_str());
            }
            if (const UniValue& v{json.find_value("kind")}; v.isStr()) {
                detected.sigKind = QString::fromStdString(v.get_str());
            }
            return detected;
        }
        detected.error = QCoreApplication::translate("SharedMnWidgets",
                                                     "This message is not a shared masternode message.");
        return detected;
    }

    if (CMutableTransaction tx; IsHex(trimmed.toStdString()) && DecodeHexTx(tx, trimmed.toStdString())) {
        if (tx.nType == TRANSACTION_PROVIDER_DISSOLVE) {
            detected.kind = Kind::StandbyHex;
            return detected;
        }
        detected.error = QCoreApplication::translate("SharedMnWidgets",
                                                     "This transaction is not a shared masternode dissolution.");
        return detected;
    }

    detected.error = QCoreApplication::translate("SharedMnWidgets",
                                                 "The clipboard does not contain a shared masternode message.");
    return detected;
}

void SharedMnMakeSecondary(QPushButton* button)
{
    if (button == nullptr) return;
    button->setProperty("mnSecondary", true);
}

void SharedMnFitWrappedLabels(QWidget* root)
{
    if (root == nullptr) return;
    for (QLabel* const label : root->findChildren<QLabel*>()) {
        if (!label->wordWrap()) continue;
        QSizePolicy policy{label->sizePolicy()};
        policy.setHeightForWidth(true);
        label->setSizePolicy(policy);
    }
}

void SharedMnSizeFromContent(QDialog* dialog, int minimum_width)
{
    if (dialog == nullptr) return;
    int extra_width{0};
    int extra_height{0};
    for (QScrollArea* const scroll : dialog->findChildren<QScrollArea*>()) {
        const QWidget* const content{scroll->widget()};
        if (content == nullptr) continue;
        const QSize wanted{content->sizeHint()};
        const QSize offered{scroll->sizeHint()};
        // Room for the vertical bar too, or the content that just fits pulls a
        // horizontal bar in behind it
        extra_width = std::max(extra_width,
                               wanted.width() - offered.width() + scroll->verticalScrollBar()->sizeHint().width());
        extra_height = std::max(extra_height, wanted.height() - offered.height());
    }
    QSize size{dialog->sizeHint() + QSize(std::max(extra_width, 0), std::max(extra_height, 0))};
    size.setWidth(std::max(size.width(), minimum_width));
    if (const QScreen* const screen = QGuiApplication::primaryScreen(); screen != nullptr) {
        // Headless platforms report no usable geometry at all; only a screen
        // that could really show a dialog gets to cap one
        const QSize available{screen->availableGeometry().size()};
        if (available.width() >= 640 && available.height() >= 480) {
            size = size.boundedTo(QSize(available.width() * 85 / 100, available.height() * 85 / 100));
        }
    }
    // The minimum must never exceed what the window ends up being, or the
    // dialog cannot be resized down on a small display
    dialog->setMinimumWidth(std::min(minimum_width, size.width()));
    dialog->resize(size);
}

QString SharedMnTermSheetHtml(const MnShareSession& session, int you_share_index, BitcoinUnits::Unit unit,
                              const std::function<bool(const QString& address)>& in_this_wallet,
                              const std::optional<SharedMnOwnFunding>& own_funding)
{
    const auto& shares{session.shares()};
    const auto& terms{session.terms()};
    const auto address = [&](const QString& value, const QString& placeholder) {
        QString html{EscapedOr(value, placeholder)};
        if (!value.isEmpty() && in_this_wallet && in_this_wallet(value)) {
            html += QStringLiteral(" <i>%1</i>")
                        .arg(QCoreApplication::translate("SharedMnWidgets", "✓ in this wallet").toHtmlEscaped());
        }
        return html;
    };
    CAmount total{0};
    for (const auto& share : shares) {
        total += share.amount;
    }

    QStringList cards;

    // 1. Participants. One block per share rather than a six-column table: an
    // address is a single unbreakable word, and six of them side by side make a
    // sheet far wider than the page that asks the reader to read every line.
    QString participants{QStringLiteral("<table cellspacing='0' cellpadding='0'>")};
    for (size_t i = 0; i < shares.size(); ++i) {
        const auto& share{shares[i]};
        const bool you{static_cast<int>(i) == you_share_index};
        QString name{EscapedOr(share.label, QCoreApplication::translate("SharedMnWidgets", "unnamed"))};
        if (you) name += QLatin1Char(' ') + QCoreApplication::translate("SharedMnWidgets", "(you)").toHtmlEscaped();
        const QString amount{
            total > 0
                ? QCoreApplication::translate("SharedMnWidgets", "%1 (%2%)")
                      .arg(SharedMnFormatAmount(unit, share.amount),
                           QString::number(100.0 * static_cast<double>(share.amount) / static_cast<double>(total),
                                           'f', 1))
                      .toHtmlEscaped()
                : SharedMnFormatAmount(unit, share.amount).toHtmlEscaped()};
        QString heading{QStringLiteral("%1 — %2").arg(name, amount)};
        if (you) heading = QStringLiteral("<b>%1</b>").arg(heading);

        QStringList detail;
        detail << LabelledRow(QCoreApplication::translate("SharedMnWidgets", "Owner:"),
                              address(share.ownerAddress,
                                      QCoreApplication::translate("SharedMnWidgets", "not set yet")));
        detail << LabelledRow(QCoreApplication::translate("SharedMnWidgets", "Refund:"),
                              address(share.refundAddress,
                                      QCoreApplication::translate("SharedMnWidgets", "not set yet")));
        detail << LabelledRow(QCoreApplication::translate("SharedMnWidgets", "Reward:"),
                              share.rewardAddress.isEmpty()
                                  ? QCoreApplication::translate("SharedMnWidgets", "same as refund").toHtmlEscaped()
                                  : address(share.rewardAddress, QString()));
        participants += QStringLiteral("<tr><td valign='top' style='padding-right:8px'>%1</td>"
                                       "<td style='padding-bottom:6px'>%2<br>%3</td></tr>")
                            .arg(QString::number(i + 1), heading, detail.join(QStringLiteral("<br>")));
    }
    participants += QStringLiteral("</table>");
    cards << Card(QCoreApplication::translate("SharedMnWidgets", "Participants"), participants);

    // 2. Masternode
    QStringList masternode;
    masternode << LabelledRow(QCoreApplication::translate("SharedMnWidgets", "Service:"),
                              terms.coreP2PAddrs.isEmpty()
                                  ? QCoreApplication::translate("SharedMnWidgets",
                                                                "Not set — set later with a service update")
                                        .toHtmlEscaped()
                                  : Escaped(terms.coreP2PAddrs));
    masternode << LabelledRow(QCoreApplication::translate("SharedMnWidgets", "Operator key:"),
                              EscapedOr(MasternodeWidgetUtil::chunked(terms.operatorPubKey),
                                        QCoreApplication::translate("SharedMnWidgets", "not set")));
    masternode << LabelledRow(QCoreApplication::translate("SharedMnWidgets", "Node run by:"),
                              EscapedOr(session.operatorSecretHolder(),
                                        QCoreApplication::translate("SharedMnWidgets", "not recorded")));
    masternode << LabelledRow(QCoreApplication::translate("SharedMnWidgets", "Voting address:"),
                              address(terms.votingAddress,
                                      QCoreApplication::translate("SharedMnWidgets", "not set")));
    masternode << LabelledRow(QCoreApplication::translate("SharedMnWidgets", "Operator reward:"),
                              QStringLiteral("%1%").arg(QString::number(terms.operatorReward / 100.0, 'f', 2)));
    cards << Card(QCoreApplication::translate("SharedMnWidgets", "Masternode"),
                  masternode.join(QStringLiteral("<br>")));

    // 3. Exit terms
    QStringList exit_terms;
    exit_terms << LabelledRow(QCoreApplication::translate("SharedMnWidgets", "Early period:"),
                              QCoreApplication::translate("SharedMnWidgets", "%1 blocks (%2)")
                                  .arg(terms.earlyPeriodBlocks)
                                  .arg(MnShareSession::HumanEarlyPeriod(terms.earlyPeriodBlocks))
                                  .toHtmlEscaped());
    exit_terms << LabelledRow(QCoreApplication::translate("SharedMnWidgets", "Early-exit penalty:"),
                              SharedMnFormatAmount(unit, terms.earlyPenalty).toHtmlEscaped());
    exit_terms << (terms.earlyPeriodBlocks == 0 || terms.earlyPenalty == 0
                       ? QCoreApplication::translate("SharedMnWidgets", "Anyone can leave at any time for free.")
                             .toHtmlEscaped()
                       : QCoreApplication::translate("SharedMnWidgets", "Leaving alone during the early period costs "
                                                                        "%1, split among the others; "
                                                                        "leaving together is free.")
                             .arg(SharedMnFormatAmount(unit, terms.earlyPenalty))
                             .toHtmlEscaped());
    cards << Card(QCoreApplication::translate("SharedMnWidgets", "Exit terms"),
                  exit_terms.join(QStringLiteral("<br>")));

    // 4. Funding
    CAmount change_total{0};
    int input_count{0};
    for (const auto& contribution : session.contributions()) {
        input_count += static_cast<int>(contribution.inputs.size());
        if (contribution.hasChange) change_total += contribution.changeAmount;
    }
    QStringList funding;
    if (own_funding && you_share_index >= 0 && static_cast<size_t>(you_share_index) < shares.size()) {
        const CAmount share{shares[you_share_index].amount};
        const CAmount paid{own_funding->spent - own_funding->change};
        QString yours;
        if (own_funding->unvalued > 0) {
            yours = QCoreApplication::translate("SharedMnWidgets", "cannot be valued on this node yet").toHtmlEscaped();
        } else if (own_funding->spent == 0) {
            yours = QCoreApplication::translate("SharedMnWidgets", "no coins reserved yet").toHtmlEscaped();
        } else {
            yours = QCoreApplication::translate("SharedMnWidgets", "%1 (%2 in, %3 change back)")
                        .arg(SharedMnFormatAmount(unit, paid), SharedMnFormatAmount(unit, own_funding->spent),
                             SharedMnFormatAmount(unit, own_funding->change))
                        .toHtmlEscaped();
        }
        // The coins that can be valued are a lower bound, so they can already
        // prove too much is being taken while others are still unknown
        if (paid - share > COIN) {
            yours += QStringLiteral("<br><b>%1</b>")
                         .arg(QCoreApplication::translate("SharedMnWidgets",
                                                          "⚠ That is at least %1 more than your %2 share. Do not "
                                                          "approve unless you meant to pay it.")
                                  .arg(SharedMnFormatAmount(unit, paid - share), SharedMnFormatAmount(unit, share))
                                  .toHtmlEscaped());
        }
        funding << LabelledRow(QCoreApplication::translate("SharedMnWidgets", "Your contribution:"), yours);
    }
    funding << LabelledRow(
        QCoreApplication::translate("SharedMnWidgets", "Contributions:"),
        QCoreApplication::translate("SharedMnWidgets", "%1, %2")
            .arg(SharedMnPlural(static_cast<qint64>(session.contributions().size()),
                                QT_TRANSLATE_NOOP("MnShareSession", "1 participant"),
                                QT_TRANSLATE_NOOP("MnShareSession", "%1 participants")),
                 SharedMnPlural(input_count, QT_TRANSLATE_NOOP("MnShareSession", "1 input"),
                                QT_TRANSLATE_NOOP("MnShareSession", "%1 inputs")))
            .toHtmlEscaped());
    funding << LabelledRow(QCoreApplication::translate("SharedMnWidgets", "Change returned:"),
                           SharedMnFormatAmount(unit, change_total).toHtmlEscaped());
    funding << LabelledRow(QCoreApplication::translate("SharedMnWidgets", "Collateral:"),
                           session.collateralIndex() < 0
                               ? QCoreApplication::translate("SharedMnWidgets", "%1, once the terms are locked")
                                     .arg(SharedMnFormatAmount(unit, total))
                                     .toHtmlEscaped()
                               : QCoreApplication::translate("SharedMnWidgets",
                                                             "output #%1 pays the %2 shared collateral script")
                                     .arg(session.collateralIndex())
                                     .arg(SharedMnFormatAmount(unit, total))
                                     .toHtmlEscaped());
    cards << Card(QCoreApplication::translate("SharedMnWidgets", "Funding"), funding.join(QStringLiteral("<br>")));

    // 5. Identity
    QStringList identity;
    identity << LabelledRow(QCoreApplication::translate("SharedMnWidgets", "Session:"), Escaped(session.sessionCode()));
    identity << LabelledRow(QCoreApplication::translate("SharedMnWidgets", "Code:"), Escaped(session.fingerprint()));
    identity << LabelledRow(QCoreApplication::translate("SharedMnWidgets", "Prepared by wallet:"),
                            EscapedOr(session.prepareWallet(),
                                      QCoreApplication::translate("SharedMnWidgets", "not recorded")));
    cards << Card(QCoreApplication::translate("SharedMnWidgets", "Identity"), identity.join(QStringLiteral("<br>")));

    return cards.join(QString());
}
