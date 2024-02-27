// Copyright (c) 2021-2023 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_GOVERNANCELIST_H
#define BITCOIN_QT_GOVERNANCELIST_H

#include <governance/object.h>
#include <primitives/transaction.h>
#include <sync.h>
#include <util/system.h>

#include <QAbstractTableModel>
#include <QDateTime>
#include <QMenu>
#include <QSortFilterProxyModel>
#include <QTimer>
#include <QWidget>

inline constexpr int GOVERNANCELIST_UPDATE_SECONDS = 10;

namespace Ui {
class GovernanceList;
}

class CDeterministicMNList;
class ClientModel;
class ProposalModel;

/** Governance Manager page widget */
class GovernanceList : public QWidget
{
    Q_OBJECT

public:
    explicit GovernanceList(QWidget* parent = nullptr);
    ~GovernanceList() override;
    void setClientModel(ClientModel* clientModel);

private:
    ClientModel* clientModel{nullptr};

    std::unique_ptr<Ui::GovernanceList> ui;
    ProposalModel* proposalModel;
    QSortFilterProxyModel* proposalModelProxy;

    QMenu* proposalContextMenu;
    QTimer* timer;

private Q_SLOTS:
    void updateProposalList();
    void updateProposalCount() const;
    void showProposalContextMenu(const QPoint& pos);
    void showAdditionalInfo(const QModelIndex& index);
};

class Proposal : public QObject
{
private:
    Q_OBJECT

    ClientModel* clientModel;
    const CGovernanceObject govObj;

    QString m_title;
    QDateTime m_startDate;
    QDateTime m_endDate;
    float m_paymentAmount;
    QString m_url;

public:
    explicit Proposal(ClientModel* _clientModel, const CGovernanceObject& _govObj, QObject* parent = nullptr);
    QString title() const;
    QString hash() const;
    QDateTime startDate() const;
    QDateTime endDate() const;
    float paymentAmount() const;
    QString url() const;
    bool isActive() const;
    QString votingStatus(int nAbsVoteReq) const;
    int GetAbsoluteYesCount() const;

    void openUrl() const;

    QString toJson() const;
};

class ProposalModel : public QAbstractTableModel
{
    Q_OBJECT

private:
    QList<const Proposal*> m_data;
    int nAbsVoteReq = 0;

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
    void append(const Proposal* proposal);
    void remove(int row);
    void reconcile(Span<const Proposal*> proposals);
    void setVotingParams(int nAbsVoteReq);

    const Proposal* getProposalAt(const QModelIndex& index) const;
};

#endif // BITCOIN_QT_GOVERNANCELIST_H
