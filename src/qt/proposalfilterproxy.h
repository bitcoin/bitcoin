// Copyright (c) 2011-2013 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_PROPOSALFILTERPROXY_H
#define BITCOIN_QT_PROPOSALFILTERPROXY_H

#include "amount.h"

#include <QDateTime>
#include <QSortFilterProxyModel>

class ProposalFilterProxy : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit ProposalFilterProxy(QObject *parent = 0);


    static const QDateTime MIN_DATE;

    static const QDateTime MAX_DATE;

    void setProposalStart(const QDateTime &date);
    void setProposalEnd(const QDateTime &date);
    void setProposal(const QString &proposal);
    
    void setMinAmount(const CAmount& minimum);
    void setMinPercentage(const CAmount& minimum);
    void setMinYesVotes(const CAmount& minimum);
    void setMinNoVotes(const CAmount& minimum);
    void setMinAbsoluteYesVotes(const CAmount& minimum);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex & source_parent) const;

private:
    QDateTime startDate;
    QDateTime endDate;
    QString proposalName;
    CAmount minAmount;
    CAmount minPercentage;
    CAmount minYesVotes;
    CAmount minNoVotes;
    CAmount minAbsoluteYesVotes;
};

#endif // BITCOIN_QT_PROPOSALFILTERPROXY_H
