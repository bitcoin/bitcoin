// Copyright (c) 2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/mnsharesession.h>

#include <chainparams.h>
#include <consensus/validation.h>
#include <core_io.h>
#include <crypto/sha256.h>
#include <evo/dmn_types.h>
#include <evo/netinfo.h>
#include <evo/providertx.h>
#include <evo/sharedcollateral.h>
#include <evo/specialtx.h>
#include <key_io.h>
#include <messagesigner.h>
#include <primitives/transaction.h>
#include <random.h>
#include <script/standard.h>
#include <uint256.h>
#include <util/strencodings.h>

#include <QCoreApplication>
#include <QRegularExpression>

#include <algorithm>
#include <limits>
#include <optional>
#include <utility>
#include <variant>

namespace {

constexpr const char* ENVELOPE_TYPE{"dash-shared-mn-session"};
constexpr int ENVELOPE_VERSION{1};
constexpr const char* FINGERPRINT_KEY{"fingerprint"};
//! A participant funds their share from a handful of coins; a transaction with
//! thousands of inputs could never be relayed anyway, and every one of them is
//! rendered and re-checked on the GUI thread
constexpr size_t MAX_CONTRIBUTION_INPUTS{64};
//! The engine has no options model, so its own messages quote plain DASH
constexpr BitcoinUnits::Unit kEngineUnit{BitcoinUnits::Unit::DASH};

//! Copy of `json` with any "fingerprint" key removed, preserving key order
UniValue WithoutFingerprint(const UniValue& json)
{
    if (!json.isObject()) return json;
    UniValue ret(UniValue::VOBJ);
    const std::vector<std::string>& keys{json.getKeys()};
    const std::vector<UniValue>& values{json.getValues()};
    for (size_t i = 0; i < keys.size(); ++i) {
        if (keys[i] == FINGERPRINT_KEY) continue;
        ret.pushKV(keys[i], values[i]);
    }
    return ret;
}

//! Untranslated stage keys used in the envelope JSON
const char* StageKey(MnShareSession::Stage stage)
{
    switch (stage) {
    case MnShareSession::Stage::Draft: return "draft";
    case MnShareSession::Stage::Frozen: return "frozen";
    case MnShareSession::Stage::Signing: return "signing";
    case MnShareSession::Stage::Combined: return "combined";
    case MnShareSession::Stage::FundingSigned: return "fundingSigned";
    case MnShareSession::Stage::Broadcast: return "broadcast";
    }
    return "draft";
}

std::optional<MnShareSession::Stage> StageFromKey(const std::string& key)
{
    for (const auto stage : {MnShareSession::Stage::Draft, MnShareSession::Stage::Frozen,
                             MnShareSession::Stage::Signing, MnShareSession::Stage::Combined,
                             MnShareSession::Stage::FundingSigned, MnShareSession::Stage::Broadcast}) {
        if (key == StageKey(stage)) return stage;
    }
    return std::nullopt;
}

//! Display name of share `index`: its label, or "Share k" when unnamed
QString ShareName(const std::vector<MnShareSession::Share>& shares, size_t index)
{
    if (index < shares.size() && !shares[index].label.isEmpty()) return shares[index].label;
    return QCoreApplication::translate("MnShareSession", "Share %1").arg(index + 1);
}

//! Index of the share named `label`, or shares.size() when no share matches
size_t ShareIndexOfLabel(const std::vector<MnShareSession::Share>& shares, const QString& label)
{
    for (size_t i = 0; i < shares.size(); ++i) {
        if (shares[i].label == label) return i;
    }
    return shares.size();
}

//! Same consensus content. Labels are local annotations, not terms, and are
//! ignored.
bool SameShare(const MnShareSession::Share& a, const MnShareSession::Share& b)
{
    return a.amount == b.amount && a.ownerAddress == b.ownerAddress && a.refundAddress == b.refundAddress &&
           a.rewardAddress == b.rewardAddress;
}

bool SameContribution(const MnShareSession::Contribution& a, const MnShareSession::Contribution& b)
{
    if (a.inputs.size() != b.inputs.size() || a.hasChange != b.hasChange) return false;
    if (a.hasChange && (a.changeAddress != b.changeAddress || a.changeAmount != b.changeAmount)) return false;
    for (size_t i = 0; i < a.inputs.size(); ++i) {
        if (a.inputs[i].txid.compare(b.inputs[i].txid, Qt::CaseInsensitive) != 0 ||
            a.inputs[i].vout != b.inputs[i].vout || a.inputs[i].sequence != b.inputs[i].sequence) {
            return false;
        }
    }
    return true;
}

bool IsTxidHex(const QString& txid)
{
    return txid.size() == 64 && IsHex(txid.toStdString());
}

//! The inputs and change outputs `contributions` describe, in the order the
//! funding transaction uses: every contribution's inputs first, then one change
//! output per contribution that has one. `error` names the contribution at
//! fault when an input id or a change address is malformed.
bool BuildFundingParts(const std::vector<MnShareSession::Contribution>& contributions, std::vector<CTxIn>& vin,
                       std::vector<CTxOut>& vout, QString& error)
{
    for (const auto& contribution : contributions) {
        for (const auto& input : contribution.inputs) {
            if (!IsTxidHex(input.txid)) {
                error = QCoreApplication::translate("MnShareSession",
                                                    "Contribution \"%1\" has a malformed input transaction id.")
                            .arg(contribution.label);
                return false;
            }
            vin.emplace_back(COutPoint(uint256S(input.txid.toStdString()), input.vout), CScript(), input.sequence);
        }
    }
    for (const auto& contribution : contributions) {
        if (!contribution.hasChange) continue;
        const CTxDestination dest{DecodeDestination(contribution.changeAddress.toStdString())};
        if (!IsValidDestination(dest)) {
            error = QCoreApplication::translate("MnShareSession", "Contribution \"%1\" has an invalid change address.")
                        .arg(contribution.label);
            return false;
        }
        vout.emplace_back(contribution.changeAmount, GetScriptForDestination(dest));
    }
    return true;
}

//! True when `payload` registers exactly the service addresses `addrs` (the
//! comma-joined list the envelope displays) would register. Both sides are run
//! through NetInfoInterface::AddEntry, so the comparison is between canonical
//! forms rather than between strings.
bool NetInfoMatches(const CProRegTx& payload, const QString& addrs)
{
    if (!payload.netInfo) return false;
    const auto expected{NetInfoInterface::MakeNetInfo(payload.nVersion)};
    if (!expected) return false;
    static const QRegularExpression separator{QStringLiteral("[,\\s]+")};
    for (const QString& entry : addrs.split(separator, Qt::SkipEmptyParts)) {
        if (expected->AddEntry(NetInfoPurpose::CORE_P2P, entry.toStdString()) != NetInfoStatus::Success) return false;
    }
    return *payload.netInfo == *expected;
}

//! Decode a frozen shared registration (funding tx + CProRegTx payload with a
//! share table) from the envelope's protx hex
bool DecodeSharedProTx(const QString& protx_hex, CMutableTransaction& tx, CProRegTx& payload, QString& error)
{
    if (!DecodeHexTx(tx, protx_hex.toStdString())) {
        error = QCoreApplication::translate("MnShareSession",
                                            "The session's prepared transaction is not valid transaction hex.");
        return false;
    }
    // assert_type=false: a malformed file must produce an error, not a debug abort
    auto opt_payload = GetTxPayload<CProRegTx>(tx, /*assert_type=*/false);
    if (!opt_payload || !opt_payload->IsShared()) {
        error = QCoreApplication::translate("MnShareSession", "The session's prepared transaction is not a shared "
                                                              "masternode registration.");
        return false;
    }
    payload = std::move(*opt_payload);
    return true;
}

} // anonymous namespace

namespace {
//! U+202F NARROW NO-BREAK SPACE. BitcoinUnits groups thousands with a plain
//! thin space, which a layout may break a number across.
constexpr char16_t NARROW_NO_BREAK_SPACE{0x202F};
} // anonymous namespace

QString SharedMnFormatAmount(BitcoinUnits::Unit unit, CAmount amount)
{
    // Eight fixed decimals on a 1000 DASH collateral are noise the reader has
    // to count through to compare two rows, so a whole-coin amount is shown as
    // one. Both separators are made non-breaking: a collateral that wraps as
    // "1" / "000.00000000" / "tDASH" across three lines is unreadable, and one
    // confirmation dialog did exactly that.
    QString number{BitcoinUnits::format(unit, amount, /*plussign=*/false, BitcoinUnits::SeparatorStyle::ALWAYS)};
    if (const int point{number.indexOf(QLatin1Char('.'))}; point >= 0) {
        int end{number.size()};
        while (end > point + 1 && number.at(end - 1) == QLatin1Char('0')) {
            --end;
        }
        number.truncate(end == point + 1 ? point : end);
    }
    number.replace(QChar(THIN_SP_CP), QChar(NARROW_NO_BREAK_SPACE));
    return number + QChar(QChar::Nbsp) + BitcoinUnits::name(unit);
}

QString SharedMnPlural(qint64 count, const char* singular, const char* plural)
{
    return count == 1 ? QCoreApplication::translate("MnShareSession", singular)
                      : QCoreApplication::translate("MnShareSession", plural).arg(count);
}

QString shared_mn::EnvelopeFingerprint(const UniValue& json)
{
    const std::string text{WithoutFingerprint(json).write(/*prettyIndent=*/2)};
    unsigned char digest[CSHA256::OUTPUT_SIZE];
    CSHA256().Write(reinterpret_cast<const unsigned char*>(text.data()), text.size()).Finalize(digest);
    const QString hex{QString::fromStdString(HexStr(Span<const unsigned char>(digest, 4))).toUpper()};
    return hex.left(4) + QLatin1Char('-') + hex.mid(4);
}

void shared_mn::AppendFingerprint(UniValue& json)
{
    json.pushKV(FINGERPRINT_KEY, EnvelopeFingerprint(json).toStdString());
}

bool shared_mn::CheckFingerprint(const UniValue& json, QString& warning)
{
    if (!json.isObject()) return true;
    const UniValue& stamped{json.find_value(FINGERPRINT_KEY)};
    if (!stamped.isStr()) return true;
    if (QString::fromStdString(stamped.get_str()).compare(EnvelopeFingerprint(json), Qt::CaseInsensitive) == 0) {
        return true;
    }
    warning = QCoreApplication::translate("MnShareSession", "This message was edited after it was copied.");
    return false;
}

MnShareSession::MnShareSession()
    : m_network{QString::fromStdString(Params().NetworkIDString())},
      m_session_id{QString::fromStdString(GetRandHash().ToString())}
{
}

QString MnShareSession::StageName(Stage stage)
{
    switch (stage) {
    case Stage::Draft:
        return QCoreApplication::translate("MnShareSession", "Draft");
    case Stage::Frozen:
        return QCoreApplication::translate("MnShareSession", "Terms locked");
    case Stage::Signing:
        return QCoreApplication::translate("MnShareSession", "Collecting approvals");
    case Stage::Combined:
        return QCoreApplication::translate("MnShareSession", "Approvals combined");
    case Stage::FundingSigned:
        return QCoreApplication::translate("MnShareSession", "Contributions signed");
    case Stage::Broadcast:
        return QCoreApplication::translate("MnShareSession", "Broadcast");
    }
    return {};
}

UniValue MnShareSession::toJsonBody() const
{
    UniValue ret(UniValue::VOBJ);
    ret.pushKV("type", ENVELOPE_TYPE);
    ret.pushKV("version", ENVELOPE_VERSION);
    ret.pushKV("network", m_network.toStdString());
    ret.pushKV("sessionId", m_session_id.toStdString());
    ret.pushKV("revision", m_revision);
    ret.pushKV("stage", StageKey(m_stage));
    ret.pushKV("fundingTx", m_funding_tx.toStdString());

    UniValue shares(UniValue::VARR);
    for (const auto& share : m_shares) {
        UniValue entry(UniValue::VOBJ);
        entry.pushKV("label", share.label.toStdString());
        entry.pushKV("amount", share.amount);
        entry.pushKV("ownerAddress", share.ownerAddress.toStdString());
        entry.pushKV("refundAddress", share.refundAddress.toStdString());
        entry.pushKV("rewardAddress", share.rewardAddress.toStdString());
        shares.push_back(entry);
    }
    ret.pushKV("shares", shares);

    UniValue terms(UniValue::VOBJ);
    terms.pushKV("coreP2PAddrs", m_terms.coreP2PAddrs.toStdString());
    terms.pushKV("operatorPubKey", m_terms.operatorPubKey.toStdString());
    terms.pushKV("votingAddress", m_terms.votingAddress.toStdString());
    terms.pushKV("operatorReward", m_terms.operatorReward);
    terms.pushKV("earlyPeriodBlocks", static_cast<int64_t>(m_terms.earlyPeriodBlocks));
    terms.pushKV("earlyPenalty", m_terms.earlyPenalty);
    ret.pushKV("terms", terms);

    UniValue contributions(UniValue::VARR);
    for (const auto& contribution : m_contributions) {
        UniValue entry(UniValue::VOBJ);
        entry.pushKV("label", contribution.label.toStdString());
        UniValue inputs(UniValue::VARR);
        for (const auto& input : contribution.inputs) {
            UniValue in(UniValue::VOBJ);
            in.pushKV("txid", input.txid.toStdString());
            in.pushKV("vout", static_cast<int64_t>(input.vout));
            in.pushKV("sequence", static_cast<int64_t>(input.sequence));
            inputs.push_back(in);
        }
        entry.pushKV("inputs", inputs);
        if (contribution.hasChange) {
            UniValue change(UniValue::VOBJ);
            change.pushKV("address", contribution.changeAddress.toStdString());
            change.pushKV("amount", contribution.changeAmount);
            entry.pushKV("change", change);
        }
        entry.pushKV("changeIndex", contribution.changeIndex);
        contributions.push_back(entry);
    }
    ret.pushKV("contributions", contributions);

    ret.pushKV("protx", m_protx.toStdString());
    ret.pushKV("consentHash", m_consent_hash.toStdString());
    ret.pushKV("collateralIndex", m_collateral_index);

    ret.pushKV("sigs", signaturesJson());

    ret.pushKV("prepareWallet", m_prepare_wallet.toStdString());
    ret.pushKV("operatorSecretHolder", m_operator_secret_holder.toStdString());
    ret.pushKV("coordinatorLabel", m_coordinator_label.toStdString());
    return ret;
}

UniValue MnShareSession::toJson() const
{
    UniValue ret{toJsonBody()};
    shared_mn::AppendFingerprint(ret);
    return ret;
}

QString MnShareSession::fingerprint() const
{
    return shared_mn::EnvelopeFingerprint(toJsonBody());
}

QString MnShareSession::toJsonString() const
{
    return QString::fromStdString(toJson().write(/*prettyIndent=*/2));
}

bool MnShareSession::fromJson(const std::string& text, QString& error)
{
    UniValue json;
    if (!json.read(text)) {
        error = QCoreApplication::translate("MnShareSession", "The file is not valid JSON.");
        return false;
    }
    return fromJson(json, error);
}

bool MnShareSession::fromJson(const UniValue& json, QString& error)
{
    try {
        if (!json.isObject()) {
            error = QCoreApplication::translate("MnShareSession", "The file is not a shared masternode session file.");
            return false;
        }
        const UniValue& type{json.find_value("type")};
        if (!type.isStr() || type.get_str() != ENVELOPE_TYPE) {
            error = QCoreApplication::translate("MnShareSession", "The file is not a shared masternode session file.");
            return false;
        }
        const UniValue& version{json.find_value("version")};
        if (!version.isNum() || version.getInt<int>() != ENVELOPE_VERSION) {
            error = QCoreApplication::translate("MnShareSession", "The session file uses an unsupported format "
                                                                  "version. Update Dash Core to open it.");
            return false;
        }
        const UniValue& network{json.find_value("network")};
        const std::string our_network{Params().NetworkIDString()};
        if (!network.isStr() || network.get_str() != our_network) {
            error = QCoreApplication::translate("MnShareSession", "The session file is for the \"%1\" network but this "
                                                                  "node runs \"%2\". It cannot be used here.")
                        .arg(network.isStr() ? QString::fromStdString(network.get_str())
                                             : QCoreApplication::translate("MnShareSession", "unknown"),
                             QString::fromStdString(our_network));
            return false;
        }
        const UniValue& session_id{json.find_value("sessionId")};
        if (!session_id.isStr() || session_id.get_str().empty()) {
            error = QCoreApplication::translate("MnShareSession", "The session file has no session id.");
            return false;
        }
        const UniValue& revision{json.find_value("revision")};
        if (!revision.isNum() || revision.getInt<int>() < 1) {
            error = QCoreApplication::translate("MnShareSession", "The session file has an invalid revision.");
            return false;
        }
        const UniValue& stage_value{json.find_value("stage")};
        if (!stage_value.isStr()) {
            error = QCoreApplication::translate("MnShareSession", "The session file has no stage.");
            return false;
        }
        const auto stage{StageFromKey(stage_value.get_str())};
        if (!stage) {
            error = QCoreApplication::translate("MnShareSession", "The session file is at the unknown stage \"%1\". It "
                                                                  "was probably created by a newer Dash Core.")
                        .arg(QString::fromStdString(stage_value.get_str()));
            return false;
        }

        MnShareSession parsed;
        // A mismatch means the text was altered between copy and paste. That is
        // not fatal on its own — every consensus-relevant field is re-validated
        // below — so parsing continues and the caller surfaces the warning.
        shared_mn::CheckFingerprint(json, parsed.m_import_warning);
        parsed.m_network = QString::fromStdString(network.get_str());
        parsed.m_session_id = QString::fromStdString(session_id.get_str());
        parsed.m_revision = revision.getInt<int>();
        parsed.m_stage = *stage;

        if (const UniValue& v{json.find_value("fundingTx")}; v.isStr()) {
            parsed.m_funding_tx = QString::fromStdString(v.get_str());
        }

        if (const UniValue& shares{json.find_value("shares")}; shares.isArray()) {
            // Refuse an oversized table before allocating anything for it: the
            // dialog builds three widgets and a radio button per share row on
            // the GUI thread, and consensus caps the table anyway
            if (shares.size() > CProRegTx::MAX_SHARES) {
                error = QCoreApplication::translate("MnShareSession",
                                                    "The session file lists %1 shares; a shared masternode can have "
                                                    "at most %2.")
                            .arg(shares.size())
                            .arg(CProRegTx::MAX_SHARES);
                return false;
            }
            for (size_t i = 0; i < shares.size(); ++i) {
                const UniValue& entry{shares[i]};
                const bool well_formed{entry.isObject() && entry.find_value("amount").isNum() &&
                                       entry.find_value("ownerAddress").isStr() &&
                                       entry.find_value("refundAddress").isStr()};
                if (!well_formed) {
                    error = QCoreApplication::translate("MnShareSession",
                                                        "Share entry %1 in the session file is malformed.")
                                .arg(i + 1);
                    return false;
                }
                Share share;
                if (const UniValue& v{entry.find_value("label")}; v.isStr()) share.label = QString::fromStdString(v.get_str());
                share.amount = entry.find_value("amount").getInt<int64_t>();
                if (!MoneyRange(share.amount)) {
                    error = QCoreApplication::translate("MnShareSession",
                                                        "Share entry %1 in the session file is malformed.")
                                .arg(i + 1);
                    return false;
                }
                share.ownerAddress = QString::fromStdString(entry.find_value("ownerAddress").get_str());
                share.refundAddress = QString::fromStdString(entry.find_value("refundAddress").get_str());
                if (const UniValue& v{entry.find_value("rewardAddress")}; v.isStr()) {
                    share.rewardAddress = QString::fromStdString(v.get_str());
                }
                parsed.m_shares.push_back(share);
            }
        }

        if (const UniValue& terms{json.find_value("terms")}; terms.isObject()) {
            if (const UniValue& v{terms.find_value("coreP2PAddrs")}; v.isStr()) parsed.m_terms.coreP2PAddrs = QString::fromStdString(v.get_str());
            if (const UniValue& v{terms.find_value("operatorPubKey")}; v.isStr()) parsed.m_terms.operatorPubKey = QString::fromStdString(v.get_str());
            if (const UniValue& v{terms.find_value("votingAddress")}; v.isStr()) parsed.m_terms.votingAddress = QString::fromStdString(v.get_str());
            if (const UniValue& v{terms.find_value("operatorReward")}; v.isNum()) parsed.m_terms.operatorReward = v.getInt<int>();
            if (const UniValue& v{terms.find_value("earlyPeriodBlocks")}; v.isNum()) {
                const int64_t blocks{v.getInt<int64_t>()};
                if (blocks < 0 || blocks > std::numeric_limits<uint32_t>::max()) {
                    error = QCoreApplication::translate("MnShareSession",
                                                        "The session file has an invalid early period.");
                    return false;
                }
                parsed.m_terms.earlyPeriodBlocks = static_cast<uint32_t>(blocks);
            }
            if (const UniValue& v{terms.find_value("earlyPenalty")}; v.isNum()) {
                parsed.m_terms.earlyPenalty = v.getInt<int64_t>();
                if (!MoneyRange(parsed.m_terms.earlyPenalty)) {
                    error = QCoreApplication::translate("MnShareSession",
                                                        "The session file has an invalid early-exit penalty.");
                    return false;
                }
            }
        }

        if (const UniValue& contributions{json.find_value("contributions")}; contributions.isArray()) {
            if (contributions.size() > CProRegTx::MAX_SHARES) {
                error = QCoreApplication::translate("MnShareSession",
                                                    "The session file lists %1 contributions; there can be at most "
                                                    "one per share, and at most %2 shares.")
                            .arg(contributions.size())
                            .arg(CProRegTx::MAX_SHARES);
                return false;
            }
            const auto malformed = [&error](size_t i) {
                error = QCoreApplication::translate("MnShareSession",
                                                    "Contribution entry %1 in the session file is malformed.")
                            .arg(i + 1);
                return false;
            };
            for (size_t i = 0; i < contributions.size(); ++i) {
                const UniValue& entry{contributions[i]};
                if (!entry.isObject() || !entry.find_value("label").isStr() || !entry.find_value("inputs").isArray()) {
                    return malformed(i);
                }
                Contribution contribution;
                contribution.label = QString::fromStdString(entry.find_value("label").get_str());
                const UniValue& inputs{entry.find_value("inputs")};
                if (inputs.size() > MAX_CONTRIBUTION_INPUTS) {
                    error = QCoreApplication::translate("MnShareSession",
                                                        "Contribution entry %1 in the session file has %2 inputs; at "
                                                        "most %3 can be funded in one contribution.")
                                .arg(i + 1)
                                .arg(inputs.size())
                                .arg(MAX_CONTRIBUTION_INPUTS);
                    return false;
                }
                for (size_t j = 0; j < inputs.size(); ++j) {
                    const UniValue& in{inputs[j]};
                    const bool well_formed{in.isObject() && in.find_value("txid").isStr() &&
                                           in.find_value("vout").isNum() && in.find_value("sequence").isNum()};
                    if (!well_formed) {
                        return malformed(i);
                    }
                    const int64_t vout{in.find_value("vout").getInt<int64_t>()};
                    const int64_t sequence{in.find_value("sequence").getInt<int64_t>()};
                    if (vout < 0 || vout > std::numeric_limits<uint32_t>::max() ||
                        sequence < 0 || sequence > std::numeric_limits<uint32_t>::max()) {
                        return malformed(i);
                    }
                    Input input;
                    input.txid = QString::fromStdString(in.find_value("txid").get_str());
                    input.vout = static_cast<uint32_t>(vout);
                    input.sequence = static_cast<uint32_t>(sequence);
                    if (!IsTxidHex(input.txid)) {
                        return malformed(i);
                    }
                    contribution.inputs.push_back(input);
                }
                if (const UniValue& change{entry.find_value("change")}; change.isObject()) {
                    if (!change.find_value("address").isStr() || !change.find_value("amount").isNum()) {
                        return malformed(i);
                    }
                    contribution.hasChange = true;
                    contribution.changeAddress = QString::fromStdString(change.find_value("address").get_str());
                    contribution.changeAmount = change.find_value("amount").getInt<int64_t>();
                    if (!MoneyRange(contribution.changeAmount)) {
                        return malformed(i);
                    }
                }
                if (const UniValue& v{entry.find_value("changeIndex")}; v.isNum()) contribution.changeIndex = v.getInt<int>();
                parsed.m_contributions.push_back(contribution);
            }
        }

        if (const UniValue& v{json.find_value("protx")}; v.isStr()) parsed.m_protx = QString::fromStdString(v.get_str());
        if (const UniValue& v{json.find_value("consentHash")}; v.isStr()) {
            const std::string& hash{v.get_str()};
            if (!hash.empty() && !(hash.size() == 64 && IsHex(hash))) {
                error = QCoreApplication::translate("MnShareSession", "The session file has a malformed consent hash.");
                return false;
            }
            parsed.m_consent_hash = QString::fromStdString(hash).toLower();
        }
        if (const UniValue& v{json.find_value("collateralIndex")}; v.isNum()) parsed.m_collateral_index = v.getInt<int>();

        if (const UniValue& sigs{json.find_value("sigs")}; sigs.isArray()) {
            for (size_t i = 0; i < sigs.size(); ++i) {
                const UniValue& entry{sigs[i]};
                if (!entry.isObject() || !entry.find_value("shareIndex").isNum() || !entry.find_value("signature").isStr()) {
                    error = QCoreApplication::translate("MnShareSession",
                                                        "Signature entry %1 in the session file is malformed.")
                                .arg(i + 1);
                    return false;
                }
                Signature sig;
                sig.shareIndex = entry.find_value("shareIndex").getInt<int>();
                sig.signatureB64 = QString::fromStdString(entry.find_value("signature").get_str());
                if (std::any_of(parsed.m_sigs.begin(), parsed.m_sigs.end(),
                                [&](const Signature& s) { return s.shareIndex == sig.shareIndex; })) {
                    error = QCoreApplication::translate("MnShareSession",
                                                        "The session file lists more than one signature for share %1.")
                                .arg(sig.shareIndex + 1);
                    return false;
                }
                parsed.m_sigs.push_back(sig);
            }
        }

        if (const UniValue& v{json.find_value("prepareWallet")}; v.isStr()) parsed.m_prepare_wallet = QString::fromStdString(v.get_str());
        if (const UniValue& v{json.find_value("operatorSecretHolder")}; v.isStr()) parsed.m_operator_secret_holder = QString::fromStdString(v.get_str());
        if (const UniValue& v{json.find_value("coordinatorLabel")}; v.isStr()) parsed.m_coordinator_label = QString::fromStdString(v.get_str());

        if (parsed.m_stage != Stage::Draft) {
            if (parsed.m_protx.isEmpty() || parsed.m_consent_hash.isEmpty()) {
                error = QCoreApplication::translate("MnShareSession", "The session file is marked \"%1\" but is "
                                                                      "missing its frozen transaction or consent hash.")
                            .arg(StageName(parsed.m_stage));
                return false;
            }
            // A frozen file must register exactly the shares/terms it displays,
            // and its prepared transaction must match its consent hash
            CMutableTransaction frozen_tx;
            CProRegTx frozen_payload;
            if (!parsed.decodeFrozen(parsed.m_protx, frozen_tx, frozen_payload, error)) return false;
            if (!parsed.payloadMatchesEnvelope(error)) return false;
            for (const auto& sig : parsed.m_sigs) {
                if (sig.shareIndex < 0 || static_cast<size_t>(sig.shareIndex) >= parsed.m_shares.size()) {
                    error = QCoreApplication::translate("MnShareSession", "The session file has a signature for a "
                                                                          "share that does not exist.");
                    return false;
                }
            }
            if (parsed.m_stage >= Stage::Combined &&
                parsed.signedCount() != static_cast<int>(parsed.m_shares.size())) {
                error = QCoreApplication::translate("MnShareSession", "The session file claims the approvals are "
                                                                      "combined, but not every share has an approval.");
                return false;
            }
            if (parsed.m_stage >= Stage::Combined) {
                for (const auto& sig : parsed.m_sigs) {
                    QString sig_error;
                    if (!parsed.verifySignature(sig.shareIndex, sig.signatureB64, sig_error)) {
                        error = QCoreApplication::translate("MnShareSession",
                                                            "The session file claims the approvals are combined, but "
                                                            "an approval is invalid: %1")
                                    .arg(sig_error);
                        return false;
                    }
                }
            }
            if (parsed.m_stage >= Stage::FundingSigned &&
                std::any_of(frozen_tx.vin.begin(), frozen_tx.vin.end(),
                            [](const CTxIn& input) { return input.scriptSig.empty(); })) {
                error = QCoreApplication::translate("MnShareSession", "The session file claims every contribution is "
                                                                      "signed, but one or more funding inputs "
                                                                      "are unsigned.");
                return false;
            }
        }

        *this = std::move(parsed);
        return true;
    } catch (const std::exception& e) {
        error = QCoreApplication::translate("MnShareSession", "The session file could not be parsed: %1")
                    .arg(QString::fromUtf8(e.what()));
        return false;
    }
}

QString MnShareSession::ShareListErrorText(const std::string& reject_reason, BitcoinUnits::Unit unit,
                                           CAmount min_share_amount, uint32_t early_period_blocks)
{
    const CAmount required_collateral{GetMnType(MnType::Regular).collat_amount};
    if (reject_reason == "bad-protx-shares-count") {
        return QCoreApplication::translate("MnShareSession", "A shared masternode needs between %1 and %2 shares.")
            .arg(CProRegTx::MIN_SHARES)
            .arg(CProRegTx::MAX_SHARES);
    }
    if (reject_reason == "bad-protx-shares-early-period") {
        return QCoreApplication::translate("MnShareSession",
                                           "The early period must be at most %1 blocks (about two years).")
            .arg(CProRegTx::MAX_EARLY_PERIOD_BLOCKS);
    }
    if (reject_reason == "bad-protx-shares-amount") {
        return QCoreApplication::translate("MnShareSession", "Every share amount must be between %1 and %2.")
            .arg(SharedMnFormatAmount(unit, CCollateralShare::MIN_AMOUNT),
                 SharedMnFormatAmount(unit, required_collateral));
    }
    if (reject_reason == "bad-protx-shares-amount-sum") {
        return QCoreApplication::translate("MnShareSession", "The share amounts must sum to exactly %1.")
            .arg(SharedMnFormatAmount(unit, required_collateral));
    }
    if (reject_reason == "bad-protx-shares-key-null" || reject_reason == "bad-protx-shares-dup-key") {
        return QCoreApplication::translate("MnShareSession", "Every share needs its own, non-empty owner address.");
    }
    if (reject_reason == "bad-protx-shares-dup-refund") {
        return QCoreApplication::translate("MnShareSession", "Every refund address must be unique.");
    }
    if (reject_reason == "bad-protx-shares-payee" || reject_reason == "bad-protx-shares-payee-dest") {
        return QCoreApplication::translate("MnShareSession",
                                           "Every refund and reward address must be a valid P2PKH or P2SH address.");
    }
    if (reject_reason == "bad-protx-shares-payee-reuse") {
        return QCoreApplication::translate("MnShareSession", "A refund or reward address must not be the voting "
                                                             "address or any share's owner address.");
    }
    if (reject_reason == "bad-protx-shares-penalty") {
        if (early_period_blocks == 0) {
            return QCoreApplication::translate("MnShareSession",
                                               "Set the early penalty to 0 when there is no early period.");
        }
        return QCoreApplication::translate("MnShareSession",
                                           "The early penalty must be below the smallest share amount (%1).")
            .arg(SharedMnFormatAmount(unit, min_share_amount));
    }
    return QCoreApplication::translate("MnShareSession", "The share table is not valid (%1).")
        .arg(QString::fromStdString(reject_reason));
}

QString MnShareSession::CheckEarlyPenalty(const std::vector<CAmount>& share_amounts, BitcoinUnits::Unit unit,
                                          uint32_t early_period_blocks, CAmount early_penalty)
{
    CAmount smallest{GetMnType(MnType::Regular).collat_amount};
    for (const CAmount amount : share_amounts) {
        if (amount > 0) smallest = std::min(smallest, amount);
    }
    TxValidationState state;
    if (IsEarlyPenaltyValid(early_period_blocks, early_penalty, smallest, state)) return {};
    return ShareListErrorText(state.GetRejectReason(), unit, smallest, early_period_blocks);
}

QString MnShareSession::CheckShareAmounts(const std::vector<CAmount>& share_amounts, BitcoinUnits::Unit unit)
{
    TxValidationState state;
    if (AreShareAmountsValid(share_amounts, GetMnType(MnType::Regular).collat_amount, state)) return {};
    return ShareListErrorText(state.GetRejectReason(), unit, CCollateralShare::MIN_AMOUNT, /*early_period_blocks=*/0);
}

QStringList MnShareSession::validateShares() const
{
    QStringList errors;

    // What consensus cannot see: whether each address string decodes to a destination of the right type
    CollateralShares shares;
    CKeyID voting_key_id;
    if (CTxDestination dest{DecodeDestination(m_terms.votingAddress.toStdString())}; const auto* pkhash = std::get_if<PKHash>(&dest)) {
        voting_key_id = ToKeyID(*pkhash);
    } else {
        errors << QCoreApplication::translate("MnShareSession", "The voting address must be a valid P2PKH address.");
    }

    for (size_t i = 0; i < m_shares.size(); ++i) {
        const Share& share{m_shares[i]};
        const QString row{QCoreApplication::translate("MnShareSession", "Share %1:").arg(i + 1)};
        const auto row_error{[&](const QString& text) { errors << row + QLatin1Char(' ') + text; }};

        CKeyID owner_key_id;
        if (CTxDestination dest{DecodeDestination(share.ownerAddress.toStdString())}; const auto* pkhash = std::get_if<PKHash>(&dest)) {
            owner_key_id = ToKeyID(*pkhash);
        } else {
            row_error(QCoreApplication::translate("MnShareSession", "the owner address must be a valid P2PKH address."));
        }

        CScript scripts[2];
        const std::pair<const QString*, QString> payout_addrs[]{
            {&share.refundAddress, QCoreApplication::translate("MnShareSession", "refund")},
            {&share.rewardAddress, QCoreApplication::translate("MnShareSession", "reward")},
        };
        for (size_t j = 0; j < std::size(payout_addrs); ++j) {
            const auto& [addr, kind] = payout_addrs[j];
            if (addr == &share.rewardAddress && addr->isEmpty()) continue; // empty reward = fall back to refund
            const CTxDestination dest{DecodeDestination(addr->toStdString())};
            if (!std::holds_alternative<PKHash>(dest) && !std::holds_alternative<ScriptHash>(dest)) {
                row_error(QCoreApplication::translate("MnShareSession",
                                                      "the %1 address must be a valid P2PKH or P2SH address.")
                              .arg(kind));
                continue;
            }
            scripts[j] = GetScriptForDestination(dest);
        }
        shares.emplace_back(share.amount, scripts[0], scripts[1], owner_key_id);
    }
    if (!errors.isEmpty()) return errors;

    TxValidationState state;
    if (!IsShareListTriviallyValid(shares, m_terms.earlyPeriodBlocks, m_terms.earlyPenalty,
                                   GetMnType(MnType::Regular).collat_amount, voting_key_id, state)) {
        CAmount min_amount{std::numeric_limits<CAmount>::max()};
        for (const auto& share : shares) min_amount = std::min(min_amount, share.amount);
        errors << ShareListErrorText(state.GetRejectReason(), kEngineUnit, min_amount, m_terms.earlyPeriodBlocks);
    }
    return errors;
}

void MnShareSession::noteDraftChange()
{
    if (m_stage == Stage::Draft) ++m_revision;
}

bool MnShareSession::rebuildFundingTx(QString& error)
{
    CMutableTransaction tx;
    tx.nVersion = 2;
    if (!BuildFundingParts(m_contributions, tx.vin, tx.vout, error)) return false;
    int change_index{0};
    for (auto& contribution : m_contributions) {
        contribution.changeIndex = contribution.hasChange ? change_index++ : -1;
    }
    m_funding_tx = QString::fromStdString(EncodeHexTx(CTransaction(tx)));
    return true;
}

bool MnShareSession::addContribution(const Contribution& contribution, QString& error)
{
    if (m_stage != Stage::Draft) {
        error = QCoreApplication::translate("MnShareSession", "Funding can only be changed while the session is "
                                                              "editable. Unlock the terms first (this discards "
                                                              "all collected approvals).");
        return false;
    }
    if (contribution.label.isEmpty()) {
        error = QCoreApplication::translate("MnShareSession", "The contribution needs a participant label.");
        return false;
    }
    if (contribution.inputs.empty()) {
        error = QCoreApplication::translate("MnShareSession", "The contribution has no funding inputs.");
        return false;
    }
    for (const auto& existing : m_contributions) {
        if (existing.label == contribution.label) {
            error = QCoreApplication::translate("MnShareSession", "\"%1\" has already contributed. Remove the existing "
                                                                  "contribution first.")
                        .arg(contribution.label);
            return false;
        }
        for (const auto& in : existing.inputs) {
            for (const auto& new_in : contribution.inputs) {
                if (in.txid.compare(new_in.txid, Qt::CaseInsensitive) == 0 && in.vout == new_in.vout) {
                    error = QCoreApplication::translate("MnShareSession",
                                                        "Input %1:%2 is already contributed by \"%3\".")
                                .arg(new_in.txid)
                                .arg(new_in.vout)
                                .arg(existing.label);
                    return false;
                }
            }
        }
    }
    if (contribution.hasChange && contribution.changeAmount <= 0) {
        error = QCoreApplication::translate("MnShareSession", "The change amount must be positive.");
        return false;
    }

    m_contributions.push_back(contribution);
    if (!rebuildFundingTx(error)) {
        m_contributions.pop_back();
        return false;
    }
    ++m_revision;
    return true;
}

bool MnShareSession::removeContribution(const QString& label, QString& error)
{
    if (m_stage != Stage::Draft) {
        error = QCoreApplication::translate("MnShareSession", "Funding can only be changed while the session is "
                                                              "editable. Unlock the terms first (this discards "
                                                              "all collected approvals).");
        return false;
    }
    for (auto it = m_contributions.begin(); it != m_contributions.end(); ++it) {
        if (it->label == label) {
            m_contributions.erase(it);
            if (!rebuildFundingTx(error)) return false;
            ++m_revision;
            return true;
        }
    }
    error = QCoreApplication::translate("MnShareSession", "No contribution from \"%1\" was found.").arg(label);
    return false;
}

bool MnShareSession::freeze(const QString& protx_hex, const QString& consent_hash_hex, int collateral_index, QString& error)
{
    if (m_stage != Stage::Draft) {
        error = QCoreApplication::translate("MnShareSession", "Only an editable session can be locked.");
        return false;
    }
    CMutableTransaction tx;
    CProRegTx payload;
    if (!DecodeSharedProTx(protx_hex, tx, payload, error)) return false;
    if (!(consent_hash_hex.size() == 64 && IsHex(consent_hash_hex.toStdString()))) {
        error = QCoreApplication::translate("MnShareSession", "The consent hash is malformed.");
        return false;
    }
    const uint256 recomputed{payload.MakeSharedRegConsentHash(CTransaction(tx))};
    if (recomputed != uint256S(consent_hash_hex.toStdString())) {
        error = QCoreApplication::translate("MnShareSession",
                                            "The consent hash does not match the prepared transaction. Do not sign; "
                                            "restart the preparation step.");
        return false;
    }
    if (collateral_index < 0 || payload.collateralOutpoint.n != static_cast<uint32_t>(collateral_index)) {
        error = QCoreApplication::translate("MnShareSession",
                                            "The collateral output index does not match the prepared transaction.");
        return false;
    }
    // The share table and terms the user reviewed must be exactly what the
    // prepared transaction registers, otherwise the displayed session lies
    m_protx = protx_hex;
    m_collateral_index = collateral_index;
    if (!payloadMatchesEnvelope(error)) {
        m_protx.clear();
        m_collateral_index = -1;
        return false;
    }
    m_consent_hash = consent_hash_hex.toLower();
    m_sigs.clear();
    m_stage = Stage::Frozen;
    return true;
}

void MnShareSession::unfreeze()
{
    m_protx.clear();
    m_consent_hash.clear();
    m_collateral_index = -1;
    m_sigs.clear();
    m_stage = Stage::Draft;
    // Signatures collected for the old consent hash can never apply to the
    // reworked session; a new revision makes every circulating copy stale
    ++m_revision;
}

bool MnShareSession::verifySignature(int share_index, const QString& sig_b64, QString& error) const
{
    if (m_protx.isEmpty() || m_consent_hash.isEmpty()) {
        error = QCoreApplication::translate("MnShareSession",
                                            "The terms are not locked yet; there is nothing to approve.");
        return false;
    }
    CMutableTransaction tx;
    CProRegTx payload;
    if (!decodeFrozen(m_protx, tx, payload, error)) return false;
    if (share_index < 0 || static_cast<size_t>(share_index) >= payload.shares.size()) {
        error = QCoreApplication::translate("MnShareSession", "Share %1 of %2 does not exist.")
                    .arg(share_index + 1)
                    .arg(payload.shares.size());
        return false;
    }
    const uint256 consent_hash{uint256S(m_consent_hash.toStdString())};
    // A signature over the payload digest is meaningless if the payload is not
    // the share table the participant reviewed
    if (!payloadMatchesEnvelope(error)) return false;
    const auto sig{DecodeBase64(sig_b64.toStdString())};
    if (!sig) {
        error = QCoreApplication::translate("MnShareSession", "The signature is not valid base64.");
        return false;
    }
    const QString label{ShareName(m_shares, static_cast<size_t>(share_index))};
    if (std::string str_error; !CHashSigner::VerifyHashCanonical(consent_hash, payload.shares[share_index].keyIDOwner, *sig, str_error)) {
        error = QCoreApplication::translate("MnShareSession",
                                            "The signature from %1 does not verify against the current terms. It was "
                                            "most likely made for an older version of this session — ask %1 to re-sign "
                                            "the current version.")
                    .arg(label);
        return false;
    }
    return true;
}

const MnShareSession::Signature* MnShareSession::findSignature(int share_index) const
{
    for (const auto& sig : m_sigs) {
        if (sig.shareIndex == share_index) return &sig;
    }
    return nullptr;
}

bool MnShareSession::payloadMatchesEnvelope(QString& error) const
{
    CMutableTransaction tx;
    CProRegTx payload;
    if (!DecodeSharedProTx(m_protx, tx, payload, error)) return false;

    if (payload.shares.size() != m_shares.size()) {
        error = QCoreApplication::translate("MnShareSession",
                                            "The session file's share table does not match its prepared transaction. "
                                            "Do not sign; request a fresh copy.");
        return false;
    }
    for (size_t i = 0; i < m_shares.size(); ++i) {
        const Share& share{m_shares[i]};
        const CCollateralShare& signed_share{payload.shares[i]};

        const CTxDestination owner{DecodeDestination(share.ownerAddress.toStdString())};
        const auto* owner_pkhash{std::get_if<PKHash>(&owner)};
        if (!owner_pkhash || ToKeyID(*owner_pkhash) != signed_share.keyIDOwner ||
            share.amount != signed_share.amount ||
            GetScriptForDestination(DecodeDestination(share.refundAddress.toStdString())) != signed_share.scriptRefund) {
            error = QCoreApplication::translate("MnShareSession",
                                                "Share %1 in the session file does not match its prepared transaction. "
                                                "Do not sign; request a fresh copy.")
                        .arg(i + 1);
            return false;
        }
        const CScript expected_reward{share.rewardAddress.isEmpty()
                                          ? CScript()
                                          : GetScriptForDestination(DecodeDestination(share.rewardAddress.toStdString()))};
        if (expected_reward != signed_share.scriptReward) {
            error = QCoreApplication::translate("MnShareSession",
                                                "Share %1's reward address in the session file does not match its "
                                                "prepared transaction. Do not sign; request a fresh copy.")
                        .arg(i + 1);
            return false;
        }
    }

    const CTxDestination voting{DecodeDestination(m_terms.votingAddress.toStdString())};
    const auto* voting_pkhash{std::get_if<PKHash>(&voting)};
    const bool operator_matches{QString::fromStdString(payload.pubKeyOperator.Get().ToString(/*specificLegacyScheme=*/false))
                                    .compare(m_terms.operatorPubKey, Qt::CaseInsensitive) == 0};
    if (!voting_pkhash || ToKeyID(*voting_pkhash) != payload.keyIDVoting ||
        !operator_matches ||
        m_terms.operatorReward != payload.nOperatorReward ||
        m_terms.earlyPeriodBlocks != payload.nEarlyPeriodBlocks ||
        m_terms.earlyPenalty != payload.nEarlyPenalty ||
        !NetInfoMatches(payload, m_terms.coreP2PAddrs)) {
        error = QCoreApplication::translate("MnShareSession",
                                            "The terms shown in the session file do not match its prepared "
                                            "transaction. Do not sign; request a fresh copy.");
        return false;
    }

    // The share table is only half of what the wallet is asked to sign. The
    // other half is where the money goes: the Funding card and the "change
    // returned" line are read out of m_contributions, so a transaction whose
    // change output pays somebody else, or which is missing one, would be
    // approved against a file that promises the opposite.
    const QString funding_mismatch{QCoreApplication::translate(
        "MnShareSession", "The funding shown in the session file does not match its prepared transaction. Do not "
                          "sign; request a fresh copy.")};
    std::vector<CTxIn> expected_vin;
    std::vector<CTxOut> expected_vout;
    if (QString build_error; !BuildFundingParts(m_contributions, expected_vin, expected_vout, build_error)) {
        error = funding_mismatch;
        return false;
    }
    // The collateral is the last output, appended to the funding transaction by
    // "protx shared_register_prepare"
    if (m_collateral_index < 0 || static_cast<size_t>(m_collateral_index) != expected_vout.size() ||
        static_cast<uint32_t>(m_collateral_index) != payload.collateralOutpoint.n) {
        error = funding_mismatch;
        return false;
    }
    // The wizard only ever builds regular masternodes, and the funding check
    // and share validation both assume the regular collateral
    if (payload.nType != MnType::Regular) {
        error = funding_mismatch;
        return false;
    }
    expected_vout.emplace_back(GetMnType(MnType::Regular).collat_amount, SharedCollateralScript());

    if (tx.vin.size() != expected_vin.size() || tx.vout.size() != expected_vout.size() || tx.nLockTime != 0) {
        error = funding_mismatch;
        return false;
    }
    for (size_t i = 0; i < expected_vin.size(); ++i) {
        // scriptSig is filled in as the contributions are signed; everything
        // the consent digest covers must already be final
        if (tx.vin[i].prevout != expected_vin[i].prevout || tx.vin[i].nSequence != expected_vin[i].nSequence) {
            error = funding_mismatch;
            return false;
        }
    }
    for (size_t i = 0; i < expected_vout.size(); ++i) {
        if (tx.vout[i].nValue != expected_vout[i].nValue ||
            tx.vout[i].scriptPubKey != expected_vout[i].scriptPubKey) {
            error = funding_mismatch;
            return false;
        }
    }
    return true;
}

bool MnShareSession::sameDraftState(const MnShareSession& other) const
{
    if (m_funding_tx != other.m_funding_tx) return false;
    if (m_shares.size() != other.m_shares.size()) return false;
    for (size_t i = 0; i < m_shares.size(); ++i) {
        if (!SameShare(m_shares[i], other.m_shares[i])) return false;
    }
    return sameTerms(other);
}

bool MnShareSession::sameTerms(const MnShareSession& other) const
{
    return m_terms.coreP2PAddrs == other.m_terms.coreP2PAddrs &&
           m_terms.operatorPubKey == other.m_terms.operatorPubKey &&
           m_terms.votingAddress == other.m_terms.votingAddress &&
           m_terms.operatorReward == other.m_terms.operatorReward &&
           m_terms.earlyPeriodBlocks == other.m_terms.earlyPeriodBlocks &&
           m_terms.earlyPenalty == other.m_terms.earlyPenalty;
}

bool MnShareSession::addSignature(int share_index, const QString& sig_b64, QString& error)
{
    if (!verifySignature(share_index, sig_b64, error)) return false;
    if (const Signature* existing = findSignature(share_index)) {
        // RFC6979 signatures are deterministic: the same key signing the same
        // digest always yields the same bytes, so a byte-identical duplicate
        // is a harmless re-import and anything else signed a stale version
        if (DecodeBase64(existing->signatureB64.toStdString()) == DecodeBase64(sig_b64.toStdString())) {
            return true;
        }
        error = QCoreApplication::translate(
            "MnShareSession", "A different signature for this share is already recorded. One of the copies signed a "
                              "stale version of the session — agree on the latest revision and have it re-signed.");
        return false;
    }
    Signature sig;
    sig.shareIndex = share_index;
    sig.signatureB64 = sig_b64;
    const auto pos = std::find_if(m_sigs.begin(), m_sigs.end(),
                                  [share_index](const Signature& s) { return s.shareIndex > share_index; });
    m_sigs.insert(pos, sig);
    if (m_stage == Stage::Frozen) m_stage = Stage::Signing;
    return true;
}

int MnShareSession::signedCount() const
{
    int count{0};
    for (const auto& sig : m_sigs) {
        if (sig.shareIndex >= 0 && static_cast<size_t>(sig.shareIndex) < m_shares.size()) ++count;
    }
    return count;
}

UniValue MnShareSession::signaturesJson() const
{
    UniValue sigs(UniValue::VARR);
    for (const auto& sig : m_sigs) {
        UniValue entry(UniValue::VOBJ);
        entry.pushKV("shareIndex", sig.shareIndex);
        entry.pushKV("signature", sig.signatureB64.toStdString());
        sigs.push_back(entry);
    }
    return sigs;
}

QStringList MnShareSession::dropUnverifiedSignatures()
{
    QStringList dropped;
    std::vector<Signature> kept;
    for (const auto& sig : m_sigs) {
        if (QString error; verifySignature(sig.shareIndex, sig.signatureB64, error)) {
            kept.push_back(sig);
        } else {
            dropped << error;
        }
    }
    m_sigs = std::move(kept);
    return dropped;
}

bool MnShareSession::replaceProTx(const QString& tx_hex, QString& error)
{
    if (m_stage != Stage::Combined) {
        error = QCoreApplication::translate("MnShareSession",
                                            "Funding inputs can only be signed once the approvals are combined.");
        return false;
    }
    MnShareSession candidate{*this};
    candidate.m_protx = tx_hex;
    CMutableTransaction tx;
    CProRegTx payload;
    if (!candidate.decodeFrozen(tx_hex, tx, payload, error) || !candidate.payloadMatchesEnvelope(error)) return false;
    m_protx = tx_hex;
    return true;
}

bool MnShareSession::decodeFrozen(const QString& protx_hex, CMutableTransaction& tx, CProRegTx& payload,
                                  QString& error) const
{
    if (!DecodeSharedProTx(protx_hex, tx, payload, error)) return false;
    if (payload.MakeSharedRegConsentHash(CTransaction(tx)) != uint256S(m_consent_hash.toStdString())) {
        error = QCoreApplication::translate("MnShareSession",
                                            "The prepared transaction does not match the terms everyone approved. Do "
                                            "not sign or broadcast it; request a fresh copy.");
        return false;
    }
    return true;
}

QString MnShareSession::signatureFor(int share_index) const
{
    const Signature* sig{findSignature(share_index)};
    return sig ? sig->signatureB64 : QString();
}

MnShareSession::MergeResult MnShareSession::absorbDraftReply(const MnShareSession& other, QString& error)
{
    if (other.m_session_id != m_session_id) {
        error = QCoreApplication::translate("MnShareSession",
                                            "This message is for a different session (Session %1). You are working on "
                                            "Session %2.")
                    .arg(other.sessionCode(), sessionCode());
        return MergeResult::Conflict;
    }
    if (m_stage != Stage::Draft || other.m_stage != Stage::Draft) {
        error = QCoreApplication::translate("MnShareSession",
                                            "Replies can only be absorbed while the terms are still editable.");
        return MergeResult::Conflict;
    }
    // The reply is an answer to one exact invitation. If the sender changed
    // the table or the terms, absorbing their addresses would silently mix two
    // different agreements, so it is a conflict rather than a merge.
    const QString changed_draft{QCoreApplication::translate(
        "MnShareSession", "The reply does not match your current draft: the participants, amounts or terms were "
                          "changed. Ask the sender to paste your latest invitation and reply again.")};
    if (m_shares.size() != other.m_shares.size() || !sameTerms(other)) {
        error = changed_draft;
        return MergeResult::Conflict;
    }
    for (size_t i = 0; i < m_shares.size(); ++i) {
        if (m_shares[i].label != other.m_shares[i].label || m_shares[i].amount != other.m_shares[i].amount) {
            error = changed_draft;
            return MergeResult::Conflict;
        }
    }

    MnShareSession merged{*this};
    bool absorbed{false};
    for (size_t i = 0; i < merged.m_shares.size(); ++i) {
        Share& ours{merged.m_shares[i]};
        const Share& theirs{other.m_shares[i]};
        const std::pair<QString*, const QString*> fields[]{
            {&ours.ownerAddress, &theirs.ownerAddress},
            {&ours.refundAddress, &theirs.refundAddress},
            {&ours.rewardAddress, &theirs.rewardAddress},
        };
        for (const auto& [mine, sent] : fields) {
            if (sent->isEmpty() || *mine == *sent) continue;
            if (!mine->isEmpty()) {
                error = QCoreApplication::translate("MnShareSession",
                                                    "%1's share already has details from another reply.")
                            .arg(ShareName(merged.m_shares, i));
                return MergeResult::Conflict;
            }
            *mine = *sent;
            absorbed = true;
        }
    }

    for (const auto& contribution : other.m_contributions) {
        const auto known = std::find_if(merged.m_contributions.begin(), merged.m_contributions.end(),
                                        [&](const Contribution& c) { return c.label == contribution.label; });
        if (known != merged.m_contributions.end()) continue;
        if (!merged.addContribution(contribution, error)) return MergeResult::Conflict;
        absorbed = true;
    }
    if (!absorbed) {
        // Re-pasting a reply that is already in hand changes nothing, and must
        // not bump the revision: that would make every other copy look stale.
        error.clear();
        return MergeResult::Merged;
    }
    // Order the contributions by the share they fund. The funding transaction
    // is built from this order, so without it the transaction would depend on
    // the order the replies happened to arrive in.
    std::stable_sort(merged.m_contributions.begin(), merged.m_contributions.end(),
                     [&](const Contribution& a, const Contribution& b) {
                         return ShareIndexOfLabel(merged.m_shares, a.label) <
                                ShareIndexOfLabel(merged.m_shares, b.label);
                     });
    if (!merged.rebuildFundingTx(error)) return MergeResult::Conflict;
    merged.m_revision = m_revision + 1;
    *this = std::move(merged);
    error.clear();
    return MergeResult::Merged;
}

bool MnShareSession::adoptLockedTerms(const MnShareSession& other, QString& error)
{
    if (other.m_session_id != m_session_id) {
        error = QCoreApplication::translate("MnShareSession",
                                            "This message is for a different session (Session %1). You are working on "
                                            "Session %2.")
                    .arg(other.sessionCode(), sessionCode());
        return false;
    }
    if (m_stage != Stage::Draft || (other.m_stage != Stage::Frozen && other.m_stage != Stage::Signing)) {
        error = QCoreApplication::translate("MnShareSession", "This message does not carry locked terms.");
        return false;
    }

    const QString mismatch{
        QCoreApplication::translate("MnShareSession", "The locked terms do not contain the details you sent.")};
    if (m_shares.size() != other.m_shares.size()) {
        error = mismatch;
        return false;
    }

    // The reply this copy sent answered one exact invitation. A coordinator can
    // still edit every masternode-wide term and every other row after the last
    // reply arrives, so the sheet being locked may be an agreement this
    // participant never saw - and `*this = other` below leaves nothing to
    // compare against afterwards. absorbDraftReply refuses the same divergence
    // in the other direction.
    const QString coordinator{!m_coordinator_label.isEmpty()          ? m_coordinator_label
                              : !other.m_coordinator_label.isEmpty()  ? other.m_coordinator_label
                                                                      : QCoreApplication::translate(
                                                                            "MnShareSession", "the coordinator")};
    const QString changed_terms{
        QCoreApplication::translate("MnShareSession", "The terms changed since you sent your details. Ask %1 to send "
                                                     "the current invitation and reply again.")
            .arg(coordinator)};
    if (!sameTerms(other)) {
        error = changed_terms;
        return false;
    }
    for (size_t i = 0; i < m_shares.size(); ++i) {
        if (m_shares[i].label != other.m_shares[i].label || m_shares[i].amount != other.m_shares[i].amount) {
            error = changed_terms;
            return false;
        }
    }

    for (size_t i = 0; i < m_shares.size(); ++i) {
        if (m_shares[i].ownerAddress.isEmpty()) continue; // a row somebody else fills in
        if (!SameShare(m_shares[i], other.m_shares[i])) {
            error = mismatch;
            return false;
        }
    }
    for (const auto& ours : m_contributions) {
        const auto theirs = std::find_if(other.m_contributions.begin(), other.m_contributions.end(),
                                         [&](const Contribution& c) { return c.label == ours.label; });
        if (theirs == other.m_contributions.end() || !SameContribution(ours, *theirs)) {
            error = mismatch;
            return false;
        }
    }

    *this = other;
    error.clear();
    return true;
}

MnShareSession::MergeResult MnShareSession::mergeEnvelope(const MnShareSession& other, QString& error)
{
    if (other.m_session_id != m_session_id) {
        error = QCoreApplication::translate("MnShareSession",
                                            "This message is for a different session (Session %1). You are working on "
                                            "Session %2.")
                    .arg(other.sessionCode(), sessionCode());
        return MergeResult::Conflict;
    }
    if (other.m_revision > m_revision) {
        error = QCoreApplication::translate("MnShareSession", "The imported copy is revision %1 of this session; this "
                                                              "copy is revision %2 and is superseded by it.")
                    .arg(other.m_revision)
                    .arg(m_revision);
        return MergeResult::OtherNewer;
    }
    if (other.m_revision < m_revision) {
        error = QCoreApplication::translate("MnShareSession",
                                            "The imported copy is revision %1 of this session; this copy is already at "
                                            "revision %2. Send the sender the newer file.")
                    .arg(other.m_revision)
                    .arg(m_revision);
        return MergeResult::OtherOlder;
    }

    const bool frozen{!m_consent_hash.isEmpty()};
    const bool other_frozen{!other.m_consent_hash.isEmpty()};
    if (frozen != other_frozen) {
        // Freezing does not bump the revision, so a frozen copy supersedes a
        // draft copy of the same revision
        error = other_frozen
                    ? QCoreApplication::translate("MnShareSession", "The imported copy has locked terms and supersedes "
                                                                    "this editable copy.")
                    : QCoreApplication::translate("MnShareSession", "This session's terms are already locked; the "
                                                                    "imported copy is still editable. Send "
                                                                    "the sender the newer update.");
        return other_frozen ? MergeResult::OtherNewer : MergeResult::OtherOlder;
    }
    if (frozen && m_consent_hash.compare(other.m_consent_hash, Qt::CaseInsensitive) != 0) {
        error = QCoreApplication::translate("MnShareSession",
                                            "Both copies of Session %1 locked different terms and cannot be combined. "
                                            "Agree on one copy, send it out again, and have everybody approve that "
                                            "one.")
                    .arg(sessionCode());
        return MergeResult::Conflict;
    }
    if (!frozen) {
        if (!sameDraftState(other)) {
            error = QCoreApplication::translate("MnShareSession",
                                                "Both copies of Session %1 changed the same details and cannot be "
                                                "combined. Agree on one copy and send it out again.")
                        .arg(sessionCode());
            return MergeResult::Conflict;
        }
        return MergeResult::Merged;
    }

    // Stage all changes in a copy so a conflicting funding signature or
    // advanced transaction cannot partially mutate the current session.
    MnShareSession merged{*this};

    // Same frozen terms: absorb the union of the other copy's verified signatures
    QStringList warnings;
    for (const auto& sig : other.m_sigs) {
        if (QString sig_error; !merged.addSignature(sig.shareIndex, sig.signatureB64, sig_error)) {
            warnings << sig_error;
        }
    }

    // Contributors sign the same Combined transaction independently. Merge
    // those copies input-by-input so this is one parallel signing round,
    // rather than forcing participants to pass one partially signed copy
    // around serially. Nothing except scriptSig may differ between copies.
    if (merged.m_stage == Stage::Combined && other.m_stage == Stage::Combined) {
        CMutableTransaction ours;
        CMutableTransaction theirs;
        CProRegTx ours_payload;
        CProRegTx theirs_payload;
        if (!decodeFrozen(merged.m_protx, ours, ours_payload, error) ||
            !decodeFrozen(other.m_protx, theirs, theirs_payload, error)) {
            return MergeResult::Conflict;
        }

        CMutableTransaction unsigned_ours{ours};
        CMutableTransaction unsigned_theirs{theirs};
        for (auto& input : unsigned_ours.vin)
            input.scriptSig.clear();
        for (auto& input : unsigned_theirs.vin)
            input.scriptSig.clear();
        if (EncodeHexTx(CTransaction(unsigned_ours)) != EncodeHexTx(CTransaction(unsigned_theirs))) {
            error = QCoreApplication::translate("MnShareSession", "The funding-signed copies are not the same "
                                                                  "transaction. They cannot be merged.");
            return MergeResult::Conflict;
        }

        for (size_t i = 0; i < ours.vin.size(); ++i) {
            const CScript& incoming{theirs.vin[i].scriptSig};
            if (incoming.empty()) continue;
            if (!ours.vin[i].scriptSig.empty() && ours.vin[i].scriptSig != incoming) {
                error = QCoreApplication::translate("MnShareSession", "Funding input %1 has two different signatures. "
                                                                      "The copies cannot be merged.")
                            .arg(i + 1);
                return MergeResult::Conflict;
            }
            ours.vin[i].scriptSig = incoming;
        }
        merged.m_protx = QString::fromStdString(EncodeHexTx(CTransaction(ours)));
        if (std::all_of(ours.vin.begin(), ours.vin.end(), [](const CTxIn& input) { return !input.scriptSig.empty(); })) {
            merged.m_stage = Stage::FundingSigned;
        }
    }

    // Adopt a further-advanced stage through the normal validated transitions.
    // The protx field evolves along the stages: combined join sigs, then
    // funding signatures live in the same transaction hex.
    if (other.m_stage > merged.m_stage) {
        QString transition_error;
        if (merged.m_stage < Stage::Combined && other.m_stage >= Stage::Combined &&
            !merged.setCombinedTx(other.m_protx, transition_error)) {
            error = QCoreApplication::translate("MnShareSession", "The imported copy claims the approvals are "
                                                                  "combined, but that state is invalid: %1")
                        .arg(transition_error);
            return MergeResult::Conflict;
        }
        if (other.m_stage >= Stage::FundingSigned &&
            !merged.setFundingSignedTx(other.m_protx, transition_error)) {
            error = QCoreApplication::translate("MnShareSession", "The imported copy claims every contribution is "
                                                                  "signed, but that state is invalid: %1")
                        .arg(transition_error);
            return MergeResult::Conflict;
        }
        if (merged.m_stage < Stage::Broadcast && other.m_stage == Stage::Broadcast &&
            !merged.setBroadcast(transition_error)) {
            error = QCoreApplication::translate("MnShareSession", "The imported copy claims the registration was "
                                                                  "broadcast, but that state is invalid: %1")
                        .arg(transition_error);
            return MergeResult::Conflict;
        }
    }
    *this = std::move(merged);
    error = warnings.join(QLatin1Char('\n'));
    return MergeResult::Merged;
}

bool MnShareSession::setCombinedTx(const QString& tx_hex, QString& error)
{
    if (m_stage != Stage::Frozen && m_stage != Stage::Signing) {
        error = QCoreApplication::translate("MnShareSession", "The session is not collecting approvals.");
        return false;
    }
    if (signedCount() < static_cast<int>(m_shares.size())) {
        error = QCoreApplication::translate("MnShareSession", "Not every share has signed yet (%1 of %2). Combine only "
                                                              "a fully signed session.")
                    .arg(signedCount())
                    .arg(m_shares.size());
        return false;
    }
    CMutableTransaction tx;
    CProRegTx payload;
    // Combining embeds the join signatures into the payload; everything the
    // consent digest covers must be unchanged
    if (!decodeFrozen(tx_hex, tx, payload, error)) return false;
    m_protx = tx_hex;
    m_stage = Stage::Combined;
    return true;
}

bool MnShareSession::setFundingSignedTx(const QString& tx_hex, QString& error)
{
    if (m_stage != Stage::Combined && m_stage != Stage::FundingSigned) {
        error = QCoreApplication::translate("MnShareSession", "The owner approvals have to be combined before the "
                                                              "contributions are signed.");
        return false;
    }
    CMutableTransaction tx;
    CProRegTx payload;
    if (!decodeFrozen(tx_hex, tx, payload, error)) return false;
    int unsigned_inputs{0};
    for (const auto& in : tx.vin) {
        if (in.scriptSig.empty()) ++unsigned_inputs;
    }
    if (unsigned_inputs > 0) {
        error = SharedMnPlural(unsigned_inputs,
                               QT_TRANSLATE_NOOP("MnShareSession", "One funding input is still unsigned."),
                               QT_TRANSLATE_NOOP("MnShareSession", "%1 funding inputs are still unsigned."));
        return false;
    }
    m_protx = tx_hex;
    m_stage = Stage::FundingSigned;
    return true;
}

bool MnShareSession::setBroadcast(QString& error)
{
    if (m_stage != Stage::FundingSigned) {
        error = QCoreApplication::translate("MnShareSession", "The session cannot be broadcast before every consent "
                                                              "and funding signature is in place.");
        return false;
    }
    m_stage = Stage::Broadcast;
    return true;
}

QString MnShareSession::HumanEarlyPeriod(uint32_t blocks)
{
    if (blocks == 0) return QCoreApplication::translate("MnShareSession", "none");
    const qint64 minutes{static_cast<qint64>(blocks) * 5 / 2}; // 2.5-minute blocks
    if (minutes < 120) {
        return SharedMnPlural(minutes, QT_TRANSLATE_NOOP("MnShareSession", "about 1 minute"),
                          QT_TRANSLATE_NOOP("MnShareSession", "about %1 minutes"));
    }
    if (minutes < 72 * 60) {
        return SharedMnPlural(minutes / 60, QT_TRANSLATE_NOOP("MnShareSession", "about 1 hour"),
                          QT_TRANSLATE_NOOP("MnShareSession", "about %1 hours"));
    }
    return SharedMnPlural((minutes + 720) / 1440, QT_TRANSLATE_NOOP("MnShareSession", "about 1 day"),
                      QT_TRANSLATE_NOOP("MnShareSession", "about %1 days"));
}

MnShareSession::PenaltyPreview MnShareSession::PenaltyPreviewFor(const std::vector<CAmount>& share_amounts,
                                                                 int actor_index, CAmount early_penalty,
                                                                 uint32_t early_period_blocks, CAmount fee,
                                                                 int at_height, int registered_height)
{
    PenaltyPreview preview;
    if (actor_index < 0 || static_cast<size_t>(actor_index) >= share_amounts.size()) {
        preview.error = QCoreApplication::translate("MnShareSession", "Share %1 of %2 does not exist.")
                            .arg(actor_index + 1)
                            .arg(share_amounts.size());
        return preview;
    }
    if (fee <= 0) {
        preview.error = QCoreApplication::translate("MnShareSession", "The fee must be positive.");
        return preview;
    }

    // Mirrors the early-period decision in "protx shared_dissolve": the transaction
    // would confirm at at_height + 1
    preview.penaltyFreeHeight = registered_height + static_cast<int>(early_period_blocks);
    preview.early = static_cast<int64_t>(at_height) + 1 - registered_height < static_cast<int64_t>(early_period_blocks);
    preview.penalty = preview.early ? early_penalty : 0;

    const CAmount actor_output{share_amounts[actor_index] - preview.penalty - fee};
    if (actor_output < 0) {
        preview.error = QCoreApplication::translate("MnShareSession",
                                                    "The penalty and fee exceed the dissolving participant's share.");
        return preview;
    }

    // Mirrors BuildProDisTx: pro-rata bonus with sequential floor, remainder
    // to the last non-actor share, so the payouts always sum exactly
    CAmount non_actor_total{0};
    size_t last_non_actor{0};
    bool have_non_actor{false};
    for (size_t i = 0; i < share_amounts.size(); ++i) {
        if (static_cast<int>(i) != actor_index) {
            non_actor_total += share_amounts[i];
            last_non_actor = i;
            have_non_actor = true;
        }
    }
    // A valid shared masternode always has at least two shares, but guard the
    // division anyway so a malformed table cannot divide by zero
    if (!have_non_actor || non_actor_total <= 0) {
        preview.error = QCoreApplication::translate("MnShareSession", "A unilateral dissolution needs at least one "
                                                                      "other share to receive the penalty.");
        return preview;
    }
    preview.payouts.assign(share_amounts.size(), 0);
    CAmount distributed{0};
    for (size_t i = 0; i < share_amounts.size(); ++i) {
        if (static_cast<int>(i) == actor_index) {
            preview.payouts[i] = actor_output;
            continue;
        }
        const CAmount bonus{i == last_non_actor ? preview.penalty - distributed
                                                : ProRataFloor(preview.penalty, share_amounts[i], non_actor_total)};
        distributed += bonus;
        preview.payouts[i] = share_amounts[i] + bonus;
    }
    preview.valid = true;
    return preview;
}
