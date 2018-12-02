// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/proposaltablemodel.h>

#include <qt/clientmodel.h>
#include <qt/guiconstants.h>
#include <qt/guiutil.h>
#include <qt/optionsmodel.h>
#include <qt/proposalrecord.h>

#include <core_io.h>
#include <interfaces/node.h>
#include <sync.h>
#include <uint256.h>
#include <univalue.h>
#include <util/system.h>
 
#include <QColor>
#include <QDateTime>
#include <QDebug>
#include <QIcon>
#include <QList>

static int column_alignments[] = {
        Qt::AlignRight|Qt::AlignVCenter,
        Qt::AlignCenter|Qt::AlignVCenter,
        Qt::AlignCenter|Qt::AlignVCenter,
        Qt::AlignRight|Qt::AlignVCenter,
        Qt::AlignRight|Qt::AlignVCenter,
        Qt::AlignRight|Qt::AlignVCenter,
        Qt::AlignLeft|Qt::AlignVCenter,
        Qt::AlignRight|Qt::AlignVCenter
    };

// Comparison operator for sort/binary search of model proposal list
struct HashLessThan
{
    bool operator()(const ProposalRecord &a, const ProposalRecord &b) const
    {
        return a.hash < b.hash;
    }
    bool operator()(const ProposalRecord &a, const uint256 &b) const
    {
        return a.hash < b;
    }
    bool operator()(const uint256 &a, const ProposalRecord &b) const
    {
        return a < b.hash;
    }
};

// Private implementation
class ProposalTablePriv
{
public:
    explicit ProposalTablePriv(ProposalTableModel *_parent) :
        parent(_parent)
    {
    }

    ProposalTableModel *parent;

    QList<ProposalRecord> cachedProposals;

    /* Query entire proposal list from core.
     */
    void refreshProposalList(interfaces::Node& node)
    {
        qDebug() << "ProposalTablePriv::refreshProposalList";
        cachedProposals.clear();

        for (const auto& prop : node.getProposals())
        {
            int percentage = 0;
            if (node.getMNcount().enabled > 0)
                percentage = round(prop.abs_yes * 100 / node.getMNcount().enabled);

            cachedProposals.append(ProposalRecord(
                            prop.hash,
                            prop.amount,
                            prop.start,
                            prop.end,
                            prop.yes,
                            prop.no,
                            prop.abs_yes,
                            QString::fromStdString(prop.name),
                            QString::fromStdString(prop.url),
                            percentage));
        }

        // qLowerBound() and qUpperBound() require our cachedAddressTable list to be sorted in asc order
        qSort(cachedProposals.begin(), cachedProposals.end(), HashLessThan());
    }

    void updateProposal(interfaces::Node& node, const uint256& hash, int status)
    {
        qDebug() << "ProposalTablePriv::updateProposal"+ QString::fromStdString(hash.ToString()) + " " + QString::number(status);

        // Find bounds of this proposal in model
        QList<ProposalRecord>::iterator lower = qLowerBound(
            cachedProposals.begin(), cachedProposals.end(), hash, HashLessThan());
        QList<ProposalRecord>::iterator upper = qUpperBound(
            cachedProposals.begin(), cachedProposals.end(), hash, HashLessThan());
        int lowerIndex = (lower - cachedProposals.begin());
        int upperIndex = (upper - cachedProposals.begin());
        bool inModel = (lower != upper);
        switch(status)
        {
        case CT_NEW:
            if(inModel)
            {
                qWarning() << "ProposalTablePriv::updateProposal: Warning: Got CT_NEW, but proposal is already in model";
                break;
            }
            if(true /*showProposal*/)
            {
                // Find proposal on platform
                interfaces::Proposal prop = node.getProposal(hash);
                if(prop.hash == uint256())
                {
                    qWarning() << "ProposalTablePriv::updateProposal: Warning: Got CT_NEW, but proposal is not on platform";
                    break;
                }
                // Added -- insert at the right position
                int percentage = 0;
                if (node.getMNcount().enabled > 0)
                    percentage = round(prop.abs_yes * 100 / node.getMNcount().enabled);

                ProposalRecord toInsert =
                        ProposalRecord(
                            prop.hash,
                            prop.amount,
                            prop.start,
                            prop.end,
                            prop.yes,
                            prop.no,
                            prop.abs_yes,
                            QString::fromStdString(prop.name),
                            QString::fromStdString(prop.url),
                            percentage);

                parent->beginInsertRows(QModelIndex(), lowerIndex, lowerIndex);
                cachedProposals.insert(lowerIndex, toInsert);
                parent->endInsertRows();
            }
            break;
        case CT_DELETED:
            if(!inModel)
            {
                qWarning() << "ProposalTablePriv::updateProposal: Warning: Got CT_DELETED, but proposal is not in model";
                break;
            }
            // Removed -- remove entire transaction from table
            parent->beginRemoveRows(QModelIndex(), lowerIndex, upperIndex-1);
            cachedProposals.erase(lower, upper);
            parent->endRemoveRows();
            break;
        case CT_UPDATED:
            if(!inModel)
            {
                qWarning() << "ProposalTablePriv::updateProposal: Warning: Got CT_UPDATED, but entry is not in model";
                break;
            }
            // Find proposal on platform
            interfaces::Proposal prop = node.getProposal(hash);
            if(prop.hash == uint256())
            {
                qWarning() << "ProposalTablePriv::updateProposal: Warning: Got CT_UPDATED, but proposal is not on platform";
                break;
            }
            int percentage = 0;
            if (node.getMNcount().enabled > 0)
                percentage = round(prop.abs_yes * 100 / node.getMNcount().enabled);
            lower->hash = prop.hash;
            lower->start_epoch = prop.start;
            lower->end_epoch = prop.end;
            lower->yesVotes = prop.yes;
            lower->noVotes = prop.no;
            lower-> absoluteYesVotes = prop.abs_yes;
            lower->amount = prop.amount;
            lower->name = QString::fromStdString(prop.name);
            lower->url = QString::fromStdString(prop.url);
            lower->percentage = percentage;
            break;
        }
    }

    int size()
    {
        return cachedProposals.size();
    }

    ProposalRecord *index(int idx)
    {
        if(idx >= 0 && idx < cachedProposals.size())
        {
            ProposalRecord *rec = &cachedProposals[idx];
            return rec;
        }
        return 0;
    }
};

ProposalTableModel::ProposalTableModel(ClientModel *parent):
        QAbstractTableModel(parent),
        clientModel(parent),
        priv(new ProposalTablePriv(this))
{
    columns << tr("Amount (CHC)") << tr("Start Date") << tr("End Date") << tr("Yes") << tr("No") << tr("Absolute") << tr("Proposal") << tr("Percentage");
    priv->refreshProposalList(clientModel->node());

    // Subscribe for updates
    connect(clientModel, &ClientModel::updateProposal, this, &ProposalTableModel::updateProposal);
}

ProposalTableModel::~ProposalTableModel()
{
    delete priv;
}

void ProposalTableModel::updateProposal(const QString &hash, int status)
{
    uint256 updated;
    updated.SetHex(hash.toStdString());

    priv->updateProposal(clientModel->node(), updated, status);
}

int ProposalTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return priv->size();
}

int ProposalTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return columns.length();
}

QVariant ProposalTableModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();
    ProposalRecord *rec = static_cast<ProposalRecord*>(index.internalPointer());

    switch(role)
    {
    case Qt::DisplayRole:
        switch(index.column())
        {
        case Proposal:
            return rec->name;
        case YesVotes:
            return rec->yesVotes;
        case NoVotes:
            return rec->noVotes;
        case AbsoluteYesVotes:
            return rec->absoluteYesVotes;
        case StartDate:
            return (QDateTime::fromTime_t((qint32)rec->start_epoch)).date().toString(Qt::SystemLocaleLongDate);
        case EndDate:
            return (QDateTime::fromTime_t((qint32)rec->end_epoch)).date().toString(Qt::SystemLocaleLongDate);
        case Percentage:
            return QString("%1\%").arg(rec->percentage);
        case Amount:
            return BitcoinUnits::format(BitcoinUnits::chuffs, rec->amount);
        }
        break;
    case Qt::EditRole:
        switch(index.column())
        {
        case Proposal:
            return rec->name;
        case StartDate:
            return rec->start_epoch;
        case EndDate:
            return rec->end_epoch;
        case YesVotes:
            return qint64(rec->yesVotes);
        case NoVotes:
            return qint64(rec->noVotes);
        case AbsoluteYesVotes:
            return qint64(rec->absoluteYesVotes);
        case Amount:
            return qint64(rec->amount);
        case Percentage:
            return qint64(rec->percentage);
        }
        break;
    case Qt::TextAlignmentRole:
        return column_alignments[index.column()];
    case Qt::ForegroundRole:
        if(index.column() == Percentage) {
            if(rec->percentage < 10) {
                return COLOR_NEGATIVE;
            } else {
                return QColor(23, 168, 26);
            }
        } 

        return COLOR_BAREADDRESS;
        break;
    case ProposalRole:
        return rec->name;
    case AmountRole:
        return qint64(rec->amount);
    case StartDateRole:
        return QDateTime::fromTime_t(static_cast<uint>(rec->start_epoch));
    case EndDateRole:
        return QDateTime::fromTime_t(static_cast<uint>(rec->end_epoch));
    case YesVotesRole:
        return qint64(rec->yesVotes);
    case NoVotesRole:
        return qint64(rec->noVotes);
    case AbsoluteYesVotesRole:
        return qint64(rec->absoluteYesVotes);
    case PercentageRole:
        return qint64(rec->percentage);
    case ProposalUrlRole:
        return rec->url;
    case ProposalHashRole:
        return rec->getObjHash();
    }
    return QVariant();
}

QVariant ProposalTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation == Qt::Horizontal)
    {
        if(role == Qt::DisplayRole)
        {
            return columns[section];
        }
        else if (role == Qt::TextAlignmentRole)
        {
            return Qt::AlignCenter;
        } 
        else if (role == Qt::ToolTipRole)
        {
            switch(section)
            {
            case Proposal:
                return tr("Proposal Name");
            case StartDate:
                return tr("Date and time the proposal starts.");
            case EndDate:
                return tr("Date and time the proposal expires.");
            case YesVotes:
                return tr("Obtained yes votes.");
            case NoVotes:
                return tr("Obtained no votes.");
            case AbsoluteYesVotes:
                return tr("Obtained absolute yes votes.");
            case Amount:
                return tr("Proposed amount.");
            case Percentage:
                return tr("Current vote percentage.");
            }
        }
    }
    return QVariant();
}

QModelIndex ProposalTableModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    ProposalRecord *data = priv->index(row);
    if(data)
    {
        return createIndex(row, column, priv->index(row));
    }
    return QModelIndex();
}
