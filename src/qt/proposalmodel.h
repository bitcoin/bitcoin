// Copyright (c) 2021-2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_PROPOSALMODEL_H
#define BITCOIN_QT_PROPOSALMODEL_H

#include <governance/object.h>

#include <qt/bitcoinunits.h>

#include <QAbstractTableModel>
#include <QDateTime>
#include <QObject>

#include <memory>
#include <vector>

class ClientModel;

class Proposal : public QObject
{
private:
    Q_OBJECT

    ClientModel* clientModel;
    const CGovernanceObject govObj;

    double m_paymentAmount{0.0};
    QDateTime m_endDate{};
    QDateTime m_startDate{};
    QString m_title{};
    QString m_url{};

public:
    explicit Proposal(ClientModel* _clientModel, const CGovernanceObject& _govObj, QObject* parent = nullptr);

    bool isActive() const;
    double paymentAmount() const { return m_paymentAmount; }
    int GetAbsoluteYesCount() const;
    QDateTime endDate() const { return m_endDate; }
    QDateTime startDate() const { return m_startDate; }
    QString hash() const { return QString::fromStdString(govObj.GetHash().ToString()); }
    QString title() const { return m_title; }
    QString toJson() const;
    QString url() const { return m_url; }
    QString votingStatus(int nAbsVoteReq) const;

    void openUrl() const;
};

using ProposalList = std::vector<std::unique_ptr<Proposal>>;

class ProposalModel : public QAbstractTableModel
{
    Q_OBJECT

private:
    ProposalList m_data;
    int nAbsVoteReq = 0;
    BitcoinUnit m_display_unit{BitcoinUnit::DASH};

public:
    explicit ProposalModel(QObject* parent = nullptr) :
        QAbstractTableModel(parent){};

    enum Column : int {
        HASH = 0,
        TITLE,
        START_DATE,
        END_DATE,
        PAYMENT_AMOUNT,
        IS_ACTIVE,
        VOTING_STATUS,
        _COUNT // for internal use only
    };

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    static int columnWidth(int section);
    void append(std::unique_ptr<Proposal>&& proposal);
    void remove(int row);
    void reconcile(ProposalList&& proposals);
    void setDisplayUnit(const BitcoinUnit& display_unit) { m_display_unit = display_unit; }
    void setVotingParams(int nAbsVoteReq);
    const Proposal* getProposalAt(const QModelIndex& index) const;

private:
    bool isValidRow(int row) const { return !m_data.empty() && row >= 0 && row < rowCount(); }
};

#endif // BITCOIN_QT_PROPOSALMODEL_H
