// Copyright (c) 2021-2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/proposalmodel.h>

#include <governance/vote.h>

#include <interfaces/node.h>

#include <qt/clientmodel.h>

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

QString Proposal::toJson() const
{
    const auto json = govObj.GetInnerJson();
    return QString::fromStdString(json.write(2));
}

bool Proposal::isActive() const
{
    std::string strError;
    return clientModel->node().gov().getObjLocalValidity(govObj, strError, false);
}

int Proposal::GetAbsoluteYesCount() const
{
    return clientModel->node().gov().getObjAbsYesCount(govObj, VOTE_SIGNAL_FUNDING);
}

QString Proposal::votingStatus(const int nAbsVoteReq) const
{
    // Voting status...
    // TODO: determine if voting is in progress vs. funded or not funded for past proposals.
    // see CSuperblock::GetNearestSuperblocksHeights(nBlockHeight, nLastSuperblock, nNextSuperblock);
    const int absYesCount = clientModel->node().gov().getObjAbsYesCount(govObj, VOTE_SIGNAL_FUNDING);
    if (absYesCount >= nAbsVoteReq) {
        // Could use govObj.IsSetCachedFunding here, but need nAbsVoteReq to display numbers anyway.
        return QObject::tr("Passing +%1").arg(absYesCount - nAbsVoteReq);
    } else {
        return QObject::tr("Needs additional %1 votes").arg(nAbsVoteReq - absYesCount);
    }
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
    if (role != Qt::DisplayRole && role != Qt::EditRole) {
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
        case Column::VOTING_STATUS:
            return proposal->votingStatus(nAbsVoteReq);
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
            return proposal->GetAbsoluteYesCount();
        default:
            return {};
        };
        break;
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
        return tr("Status");
    default:
        return {};
    }
}

int ProposalModel::columnWidth(int section)
{
    switch (section) {
    case Column::HASH:
        return 80;
    case Column::TITLE:
        return 220;
    case Column::START_DATE:
    case Column::END_DATE:
    case Column::PAYMENT_AMOUNT:
        return 110;
    case Column::IS_ACTIVE:
        return 80;
    case Column::VOTING_STATUS:
        return 220;
    default:
        return 80;
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
            if ((*it)->GetAbsoluteYesCount() != proposal->GetAbsoluteYesCount()) {
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
