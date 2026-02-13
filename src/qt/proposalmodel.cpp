// Copyright (c) 2021-2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/proposalmodel.h>

#include <governance/classes.h>
#include <governance/vote.h>

#include <qt/guiutil_font.h>

#include <chainparams.h>
#include <interfaces/node.h>

#include <qt/clientmodel.h>
#include <qt/guiutil.h>

#include <univalue.h>

#include <algorithm>
#include <cmath>

Proposal::Proposal(ClientModel* _clientModel, const CGovernanceObject& _govObj,
                   const interfaces::GOV::GovernanceInfo& govInfo, int collateral_confs,
                   bool is_broadcast) :
    clientModel{_clientModel},
    m_is_broadcast{is_broadcast},
    m_block_height{_clientModel ? _clientModel->getNumBlocks() : 0},
    m_collateral_confs{collateral_confs},
    m_gov_info{govInfo},
    govObj{_govObj},
    m_date_collateral{QDateTime::fromSecsSinceEpoch(govObj.GetCreationTime())},
    m_hash_collateral{QString::fromStdString(govObj.GetCollateralHash().ToString())},
    m_hash_object{QString::fromStdString(govObj.GetHash().ToString())},
    m_hash_parent{QString::fromStdString(govObj.Object().hashParent.ToString())},
    m_objHash{govObj.GetHash()}
{
    UniValue prop_data;
    if (!prop_data.read(govObj.GetDataAsPlainString())) {
        return;
    }

    if (clientModel) {
        m_funded_height = clientModel->node().gov().getProposalFundedHeight(govObj.GetHash());
        m_votes = clientModel->node().gov().getObjVotes(govObj, VOTE_SIGNAL_FUNDING);
    }

    if (const UniValue& titleValue = prop_data.find_value("name"); titleValue.isStr()) {
        m_title = QString::fromStdString(titleValue.get_str());
    }

    if (const UniValue& paymentStartValue = prop_data.find_value("start_epoch"); paymentStartValue.isNum()) {
        m_startDate = QDateTime::fromSecsSinceEpoch(paymentStartValue.getInt<int64_t>());
    }

    if (const UniValue& paymentEndValue = prop_data.find_value("end_epoch"); paymentEndValue.isNum()) {
        m_endDate = QDateTime::fromSecsSinceEpoch(paymentEndValue.getInt<int64_t>());
    }

    if (const UniValue& addressValue = prop_data.find_value("payment_address"); addressValue.isStr()) {
        m_address = QString::fromStdString(addressValue.get_str());
    }

    if (const UniValue& amountValue = prop_data.find_value("payment_amount"); amountValue.isNum()) {
        m_paymentAmount = llround(amountValue.get_real() * COIN);
    }

    if (const UniValue& urlValue = prop_data.find_value("url"); urlValue.isStr()) {
        m_url = QString::fromStdString(urlValue.get_str());
    }
}

int Proposal::paymentsRequested() const
{
    if (!m_startDate.isValid() || !m_endDate.isValid()) {
        return 1;
    }
    const auto& consensus_params = Params().GetConsensus();
    const int64_t superblock_cycle = consensus_params.nPowTargetSpacing * consensus_params.nSuperblockCycle;
    const int64_t proposal_duration = m_endDate.toSecsSinceEpoch() - m_startDate.toSecsSinceEpoch();
    return std::max<int>(1, ((proposal_duration + superblock_cycle - 1) / superblock_cycle));
}

QString Proposal::toHtml(const BitcoinUnit& unit) const
{
    QString ret;
    ret.reserve(4000);
    ret += "<html>";
    ret += "<b>" + QObject::tr("Title") + ":</b> " + GUIUtil::HtmlEscape(m_title) + "<br>";
    if (!m_url.isEmpty()) {
        ret += "<b>" + QObject::tr("URL") + ":</b> " + GUIUtil::HtmlEscape(m_url) + "<br>";
    }
    ret += "<b>" + QObject::tr("Destination Address") + ":</b> " + GUIUtil::HtmlEscape(m_address) + "<br>";
    ret += "<b>" + QObject::tr("Payment Amount") + ":</b> " + BitcoinUnits::formatHtmlWithUnit(unit, m_paymentAmount) + "<br>";
    ret += "<b>" + QObject::tr("Payments Requested") + ":</b> " + QString::number(paymentsRequested()) + "<br>";
    ret += "<b>" + QObject::tr("Payment Start") + ":</b> " + GUIUtil::dateTimeStr(m_startDate) + "<br>";
    ret += "<b>" + QObject::tr("Payment End") + ":</b> " + GUIUtil::dateTimeStr(m_endDate) + "<br>";
    ret += "<b>" + QObject::tr("Object Hash") + ":</b> " + m_hash_object + "<br>";
    ret += "<b>" + QObject::tr("Parent Hash") + ":</b> " + m_hash_parent + "<br>";
    ret += "<b>" + QObject::tr("Collateral Date") + ":</b> " + GUIUtil::dateTimeStr(m_date_collateral) + "<br>";
    ret += "<b>" + QObject::tr("Collateral Hash") + ":</b> " + m_hash_collateral + "<br>";
    ret += "<br>";
    ret += "</html>";
    return ret;
}

QString Proposal::toJson() const
{
    const auto json = govObj.GetInnerJson();
    return QString::fromStdString(json.write(2));
}

int Proposal::blocksUntilSuperblock() const
{
    return m_gov_info.nextsuperblock - m_block_height;
}

int Proposal::collateralConfs() const
{
    return m_collateral_confs;
}

int Proposal::requiredConfs() const
{
    return m_gov_info.requiredConfs;
}

ProposalStatus Proposal::status(bool is_fundable) const
{
    if (getFundedHeight().has_value()) {
        return ProposalStatus::Funded;
    }
    if (QDateTime::currentDateTime() >= endDate()) {
        return ProposalStatus::Lapsed;
    }
    if (m_collateral_confs < m_gov_info.requiredConfs) {
        return ProposalStatus::Confirming;
    }
    if (!m_is_broadcast) {
        return ProposalStatus::Pending;
    }
    if (m_gov_info.superblockcycle <= 0) {
        return ProposalStatus::Voting;
    }
    if (m_block_height % m_gov_info.superblockcycle >= m_gov_info.superblockcycle - m_gov_info.superblockmaturitywindow) {
        return is_fundable ? ProposalStatus::Passing : ProposalStatus::Failing;
    }
    if (getAbsoluteYesCount() < m_gov_info.fundingthreshold) {
        return ProposalStatus::Voting;
    }
    return is_fundable ? ProposalStatus::Passing : ProposalStatus::Unfunded;
}

bool Proposal::isActive() const
{
    if (!clientModel) {
        return false;
    }
    std::string strError;
    return clientModel->node().gov().getObjLocalValidity(govObj, strError, false);
}

///
/// Proposal Model
///

ProposalModel::ProposalModel(QObject* parent) :
    QAbstractTableModel(parent)
{
    refreshIcons();
}

void ProposalModel::refreshIcons()
{
    m_icon_confirming = {
        GUIUtil::getIcon("transaction_0", GUIUtil::ThemedColor::ORANGE),
        GUIUtil::getIcon("transaction_1", GUIUtil::ThemedColor::ORANGE),
        GUIUtil::getIcon("transaction_2", GUIUtil::ThemedColor::ORANGE),
        GUIUtil::getIcon("transaction_3", GUIUtil::ThemedColor::ORANGE),
        GUIUtil::getIcon("transaction_4", GUIUtil::ThemedColor::ORANGE),
        GUIUtil::getIcon("transaction_5", GUIUtil::ThemedColor::ORANGE)
    };
    m_icon_failing = GUIUtil::getIcon("voting", GUIUtil::ThemedColor::RED);
    m_icon_lapsed = GUIUtil::getIcon("transaction_6", GUIUtil::ThemedColor::RED);
    m_icon_passing = GUIUtil::getIcon("synced", GUIUtil::ThemedColor::GREEN);
    m_icon_pending = GUIUtil::getIcon("voting", GUIUtil::ThemedColor::BLUE);
    m_icon_unfunded = GUIUtil::getIcon("voting", GUIUtil::ThemedColor::RED);
    m_icon_voting = GUIUtil::getIcon("voting", GUIUtil::ThemedColor::ORANGE);
}

int ProposalModel::rowCount(const QModelIndex& index) const
{
    return static_cast<int>(m_data.size());
}

int ProposalModel::columnCount(const QModelIndex& index) const
{
    return Column::_COUNT;
}

QVariant ProposalModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || !isValidRow(index.row())) {
        return {};
    }
    if (index.column() == Column::HASH) {
        if (role == Qt::FontRole) {
            return GUIUtil::fixedPitchFont();
        }
        if (role == Qt::ForegroundRole) {
            return QVariant::fromValue(GUIUtil::getThemedQColor(GUIUtil::ThemedColor::UNCONFIRMED));
        }
    }
    if (role != Qt::DisplayRole && role != Qt::DecorationRole && role != Qt::EditRole && role != Qt::ToolTipRole) {
        return {};
    }

    const auto* proposal = m_data[index.row()].get();
    const bool isFundable = m_fundable_hashes.count(proposal->objHash()) > 0;
    switch(role) {
    case Qt::DisplayRole:
    {
        switch (index.column()) {
        case Column::STATUS:
            return {};
        case Column::HASH:
            return proposal->hash();
        case Column::TITLE:
            return proposal->title();
        case Column::START_DATE:
            return proposal->startDate().date();
        case Column::END_DATE:
            return proposal->endDate().date();
        case Column::PAYMENT_AMOUNT: {
            return BitcoinUnits::floorWithUnit(m_display_unit, proposal->paymentAmount(), false,
                                               BitcoinUnits::SeparatorStyle::ALWAYS);
        }
        case Column::VOTING_STATUS: {
            const int margin = proposal->getAbsoluteYesCount() - nAbsVoteReq;
            return QString("%1Y, %2N, %3A (%4%5)").arg(proposal->getYesCount()).arg(proposal->getNoCount())
                                                  .arg(proposal->getAbstainCount()).arg(margin > 0 ? "+" : "")
                                                  .arg(margin);

        }
        default:
            return {};
        };
        break;
    }
    case Qt::EditRole:
    {
        // Edit role is used for sorting, so return the raw values where possible
        switch (index.column()) {
        case Column::STATUS:
            return static_cast<int>(proposal->status(isFundable));
        case Column::HASH:
            return proposal->hash();
        case Column::TITLE:
            return proposal->title();
        case Column::START_DATE:
            return proposal->startDate();
        case Column::END_DATE:
            return proposal->endDate();
        case Column::PAYMENT_AMOUNT:
            return qlonglong(proposal->paymentAmount());
        case Column::VOTING_STATUS:
            return proposal->getAbsoluteYesCount();
        default:
            return {};
        };
        break;
    }
    case Qt::ToolTipRole:
    {
        if (index.column() == Column::STATUS) {
            switch (proposal->status(isFundable)) {
            case ProposalStatus::Confirming: {
                return tr("Pending, %1 of %2 confirmations").arg(proposal->collateralConfs()).arg(proposal->requiredConfs());
            }
            case ProposalStatus::Voting: {
                const int margin = nAbsVoteReq - proposal->getAbsoluteYesCount();
                return tr("Voting, needs %1 more votes for funding").arg(std::max(0, margin));
            }
            case ProposalStatus::Passing: {
                return tr("Passing with %1 votes").arg(proposal->getAbsoluteYesCount());
            }
            case ProposalStatus::Unfunded: {
                return tr("Passing with %1 votes but budget saturated, may not be funded").arg(proposal->getAbsoluteYesCount());
            }
            case ProposalStatus::Failing: {
                const int margin = nAbsVoteReq - proposal->getAbsoluteYesCount();
                return tr("Failed, needed %1 more votes").arg(std::max(0, margin));
            }
            case ProposalStatus::Funded: {
                const auto height{proposal->getFundedHeight()};
                return height.has_value() ? tr("Funded at block %1").arg(height.value()) : tr("Funded");
            }
            case ProposalStatus::Lapsed: {
                return tr("Lapsed, past proposal end date");
            }
            case ProposalStatus::Pending: {
                return tr("Ready to broadcast, check \"Resume Proposal\" dialog");
            }
            } // no default case, so the compiler can warn about missing cases
        }
        if (index.column() == Column::VOTING_STATUS) {
            const int margin = proposal->getAbsoluteYesCount() - nAbsVoteReq;
            return tr("%1 Yes, %2 No, %3 Abstain, %4").arg(proposal->getYesCount()).arg(proposal->getNoCount())
                                                      .arg(proposal->getAbstainCount())
                                                      .arg((margin >= 0 ? tr("passing with %1 votes") : tr("needs %1 more votes")).arg(std::abs(margin)));
        }
        return {};
    }
    case Qt::DecorationRole:
    {
        if (index.column() == Column::STATUS) {
            switch (proposal->status(isFundable)) {
            case ProposalStatus::Confirming:
                return m_icon_confirming[std::clamp<int>(proposal->collateralConfs(), 0, 5)];
            case ProposalStatus::Voting:
                return m_icon_voting;
            case ProposalStatus::Unfunded:
                return m_icon_unfunded;
            case ProposalStatus::Failing:
                return m_icon_failing;
            case ProposalStatus::Passing:
            case ProposalStatus::Funded:
                return m_icon_passing;
            case ProposalStatus::Pending:
                return m_icon_pending;
            case ProposalStatus::Lapsed:
                return m_icon_lapsed;
            } // no default case, so the compiler can warn about missing cases
        }
        return {};
    }
    }
    return {};
}

QVariant ProposalModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
        return {};
    }

    switch (section) {
    case Column::STATUS:
        return {};
    case Column::HASH:
        return tr("Hash");
    case Column::TITLE:
        return tr("Title");
    case Column::START_DATE:
        return tr("Start");
    case Column::END_DATE:
        return tr("End");
    case Column::PAYMENT_AMOUNT:
        return tr("Amount");
    case Column::VOTING_STATUS:
        return tr("Votes");
    default:
        return {};
    }
}

void ProposalModel::append(std::unique_ptr<Proposal>&& proposal)
{
    beginInsertRows({}, rowCount(), rowCount());
    m_data.push_back(std::move(proposal));
    endInsertRows();
}

void ProposalModel::remove(int row)
{
    if (!isValidRow(row)) {
        return;
    }
    beginRemoveRows({}, row, row);
    m_data.erase(m_data.begin() + row);
    endRemoveRows();
}

void ProposalModel::reconcile(ProposalList&& proposals, std::unordered_set<uint256, StaticSaltedHasher>&& fundable_hashes)
{
    m_fundable_hashes = std::move(fundable_hashes);

    // Track which existing proposals to keep. After processing new proposals,
    // remove any existing proposals that weren't found in the new set.
    const int original_sz{rowCount()};
    std::vector<bool> keep_index(original_sz, false);

    for (auto& proposal : proposals) {
        auto it{std::ranges::find_if(m_data, [&proposal](const auto& existing) {
            return existing->hash() == proposal->hash();
        })};

        if (it != m_data.end()) {
            const auto idx{static_cast<int>(std::distance(m_data.begin(), it))};
            keep_index[static_cast<size_t>(idx)] = true;
            // Always replace: block height, vote breakdown, and broadcast status
            // can all change between cycles without a single cheap sentinel.
            *it = std::move(proposal);
            Q_EMIT dataChanged(createIndex(idx, Column::STATUS), createIndex(idx, Column::VOTING_STATUS));
        } else {
            append(std::move(proposal));
        }
    }

    // Remove in reverse order to preserve indices during removal
    for (int idx{original_sz}; idx-- > 0;) {
        if (!keep_index[static_cast<size_t>(idx)]) {
            remove(idx);
        }
    }

}

void ProposalModel::setDisplayUnit(const BitcoinUnit& display_unit)
{
    m_display_unit = display_unit;
    if (!m_data.empty()) {
        Q_EMIT dataChanged(createIndex(0, Column::PAYMENT_AMOUNT), createIndex(rowCount() - 1, Column::PAYMENT_AMOUNT));
    }
}

void ProposalModel::setVotingParams(int newAbsVoteReq)
{
    if (this->nAbsVoteReq == newAbsVoteReq) {
        return;
    }

    // Changing either of the voting params may change the voting status
    // column. Emit signal to force recalculation.
    this->nAbsVoteReq = newAbsVoteReq;
    if (!m_data.empty()) {
        Q_EMIT dataChanged(createIndex(0, Column::VOTING_STATUS), createIndex(rowCount() - 1, Column::VOTING_STATUS));
    }
}

const Proposal* ProposalModel::getProposalAt(const QModelIndex& index) const
{
    if (!index.isValid() || !isValidRow(index.row())) {
        return nullptr;
    }
    return m_data[index.row()].get();
}
