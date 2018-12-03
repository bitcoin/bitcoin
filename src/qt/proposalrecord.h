// Copyright (c) 2011-2013 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_PROPOSALRECORD_H
#define BITCOIN_QT_PROPOSALRECORD_H

#include <amount.h>
#include <uint256.h>

#include <QList>
#include <QString>

namespace interfaces {
class Node;
struct Proposal;
}

class ProposalRecord
{
public:
    ProposalRecord():
            hash(), amount(0), start_epoch(0), end_epoch(0), yesVotes(0), noVotes(0), absoluteYesVotes(0), name(""), url(""), percentage(0)
    {
    }

    ProposalRecord(uint256 hash, const CAmount& amount, qint64 start_epoch, qint64 end_epoch,
                const int64_t& yesVotes, const int64_t& noVotes, const int64_t& absoluteYesVotes,
                QString name, QString url, const int64_t& percentage):
            hash(hash), amount(amount), start_epoch(start_epoch), end_epoch(end_epoch), yesVotes(yesVotes), noVotes(noVotes),
            absoluteYesVotes(absoluteYesVotes), name(name), url(url), percentage(percentage)
    {
    }

    /** Decompose CGovernanceObject to model proposal records.
     */
    static bool showProposal();

    uint256 hash;
    CAmount amount;
    qint64 start_epoch;
    qint64 end_epoch;
    int64_t yesVotes;
    int64_t noVotes;
    int64_t absoluteYesVotes;
    QString name;
    QString url;
    int64_t percentage;

    /** Return the unique identifier for this proposal (part) */
    QString getObjHash() const;
};

#endif // BITCOIN_QT_PROPOSALRECORD_H
