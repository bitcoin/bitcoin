// Copyright (c) 2021-2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/proposalmodel.h>

#include <governance/vote.h>

#include <qt/guiutil_font.h>

#include <interfaces/node.h>

#include <qt/clientmodel.h>
#include <qt/guiutil.h>

#include <QDesktopServices>
#include <QUrl>

#include <univalue.h>

#include <algorithm>
#include <cmath>

Proposal::Proposal(ClientModel* _clientModel, const CGovernanceObject& _govObj) :
    clientModel{_clientModel},
    govObj{_govObj},
    m_hash{QString::fromStdString(govObj.GetHash().ToString())}
{
    UniValue prop_data;
    if (!prop_data.read(govObj.GetDataAsPlainString())) {
        return;
    }

    if (clientModel) {
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

    if (const UniValue& amountValue = prop_data.find_value("payment_amount"); amountValue.isNum()) {
        m_paymentAmount = llround(amountValue.get_real() * COIN);
    }

    if (const UniValue& urlValue = prop_data.find_value("url"); urlValue.isStr()) {
        m_url = QString::fromStdString(urlValue.get_str());
    }
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
    ret += "<b>" + QObject::tr("Payment Amount") + ":</b> " + BitcoinUnits::formatHtmlWithUnit(unit, m_paymentAmount) + "<br>";
    ret += "<b>" + QObject::tr("Payment Start") + ":</b> " + GUIUtil::dateTimeStr(m_startDate) + "<br>";
    ret += "<b>" + QObject::tr("Payment End") + ":</b> " + GUIUtil::dateTimeStr(m_endDate) + "<br>";
    ret += "<b>" + QObject::tr("Object Hash") + ":</b> " + m_hash + "<br>";
    ret += "<br>";
    ret += "</html>";
    return ret;
}

QString Proposal::toJson() const
{
    const auto json = govObj.GetInnerJson();
    return QString::fromStdString(json.write(2));
}

bool Proposal::isActive() const
{
    if (!clientModel) {
        return false;
    }
    std::string strError;
    return clientModel->node().gov().getObjLocalValidity(govObj, strError, false);
}

void Proposal::openUrl() const
{
    QDesktopServices::openUrl(QUrl(m_url));
}

///
/// Proposal Model
///

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
    if (role != Qt::DisplayRole && role != Qt::EditRole && role != Qt::ToolTipRole) {
        return {};
    }

    const auto* proposal = m_data[index.row()].get();
    switch(role) {
    case Qt::DisplayRole:
    {
        switch (index.column()) {
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
        case Column::IS_ACTIVE:
            return proposal->isActive() ? tr("Yes") : tr("No");
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
        case Column::IS_ACTIVE:
            return proposal->isActive();
        case Column::VOTING_STATUS:
            return proposal->getAbsoluteYesCount();
        default:
            return {};
        };
        break;
    }
    case Qt::ToolTipRole:
    {
        if (index.column() == Column::VOTING_STATUS) {
            const int margin = proposal->getAbsoluteYesCount() - nAbsVoteReq;
            return tr("%1 Yes, %2 No, %3 Abstain, %4").arg(proposal->getYesCount()).arg(proposal->getNoCount())
                                                      .arg(proposal->getAbstainCount())
                                                      .arg((margin >= 0 ? tr("passing with %1 votes") : tr("needs %1 more votes")).arg(std::abs(margin)));
        }
        return {};
    }
    };
    return {};
}

QVariant ProposalModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
        return {};
    }

    switch (section) {
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
    case Column::IS_ACTIVE:
        return tr("Active");
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

void ProposalModel::reconcile(ProposalList&& proposals)
{
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
            if ((*it)->getAbsoluteYesCount() != proposal->getAbsoluteYesCount()) {
                // Replace proposal to update vote count
                *it = std::move(proposal);
                Q_EMIT dataChanged(createIndex(idx, Column::VOTING_STATUS), createIndex(idx, Column::VOTING_STATUS));
            }
            // else: no changes, proposal unique_ptr goes out of scope and gets deleted
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
