// Copyright (c) 2011-2013 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "proposalfilterproxy.h"
#include "proposaltablemodel.h"

#include <cstdlib>

#include <QDateTime>

const QDateTime ProposalFilterProxy::MIN_DATE = QDateTime::fromTime_t(0);
const QDateTime ProposalFilterProxy::MAX_DATE = QDateTime::fromTime_t(0xFFFFFFFF);

ProposalFilterProxy::ProposalFilterProxy(QObject *parent) :
    QSortFilterProxyModel(parent),
    startDate(MIN_DATE),
    endDate(MAX_DATE),
    proposalName(),
    minAmount(0),
    minPercentage(-100),
    minYesVotes(0),
    minNoVotes(0),
    minAbsoluteYesVotes(INT_MIN)
{
}

bool ProposalFilterProxy::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);

    int proposalStartDate = index.data(ProposalTableModel::StartDateRole).toInt();
    int proposalEndDate = index.data(ProposalTableModel::EndDateRole).toInt();
    QString propName = index.data(ProposalTableModel::ProposalRole).toString();
    qint64 amount = llabs(index.data(ProposalTableModel::AmountRole).toLongLong());
    int yesVotes = index.data(ProposalTableModel::YesVotesRole).toInt();
    int noVotes = index.data(ProposalTableModel::NoVotesRole).toInt();
    int absoluteYesVotes = index.data(ProposalTableModel::AbsoluteYesVotesRole).toInt();
    int percentage = index.data(ProposalTableModel::PercentageRole).toInt();

    if(proposalStartDate < (startDate.toMSecsSinceEpoch() / 1000))
       return false;
    if(proposalEndDate > (endDate.toMSecsSinceEpoch() / 1000))
       return false;
    if(!propName.contains(proposalName, Qt::CaseInsensitive))
        return false;
    if(amount < minAmount)
        return false;
    if(yesVotes < minYesVotes)
        return false;
    if(noVotes < minNoVotes)
        return false;
    if(absoluteYesVotes < minAbsoluteYesVotes)
        return false;
    if(percentage < minPercentage)
        return false;

    return true;
}

void ProposalFilterProxy::setProposalStart(const QDateTime &date)
{
    this->startDate = date;
    invalidateFilter();
}

void ProposalFilterProxy::setProposalEnd(const QDateTime &date)
{
    this->endDate = date;
    invalidateFilter();
}

void ProposalFilterProxy::setProposal(const QString &proposal)
{
    this->proposalName = proposal;
    invalidateFilter();
}

void ProposalFilterProxy::setMinAmount(const CAmount& minimum)
{
    this->minAmount = minimum;
    invalidateFilter();
}

void ProposalFilterProxy::setMinPercentage(const CAmount& minimum)
{
    this->minPercentage = minimum;
    invalidateFilter();
}

void ProposalFilterProxy::setMinYesVotes(const CAmount& minimum)
{
    this->minYesVotes = minimum;
    invalidateFilter();
}

void ProposalFilterProxy::setMinNoVotes(const CAmount& minimum)
{
    this->minNoVotes = minimum;
    invalidateFilter();
}

void ProposalFilterProxy::setMinAbsoluteYesVotes(const CAmount& minimum)
{
    this->minAbsoluteYesVotes = minimum;
    invalidateFilter();
}

int ProposalFilterProxy::rowCount(const QModelIndex &parent) const
{
    return QSortFilterProxyModel::rowCount(parent);
}