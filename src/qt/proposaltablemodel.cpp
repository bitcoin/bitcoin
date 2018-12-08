// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/proposaltablemodel.h>

#include <qt/clientmodel.h>
#include <qt/guiconstants.h>
#include <qt/guiutil.h>

#include <interfaces/node.h>
#include <key_io.h>
 
#include <QColor>
#include <QDateTime>
#include <QDebug>

static int column_alignments[] = {
        Qt::AlignRight|Qt::AlignVCenter,
        Qt::AlignCenter|Qt::AlignVCenter,
        Qt::AlignCenter|Qt::AlignVCenter,
        Qt::AlignRight|Qt::AlignVCenter,
        Qt::AlignRight|Qt::AlignVCenter,
        Qt::AlignRight|Qt::AlignVCenter,
        Qt::AlignLeft|Qt::AlignVCenter,
        Qt::AlignCenter|Qt::AlignVCenter,
        Qt::AlignCenter|Qt::AlignVCenter,
        Qt::AlignRight|Qt::AlignVCenter
    };

struct ProposalRecord
{
    uint256 hash;
    CAmount amount;
    qint32 start_epoch;
    qint32 end_epoch;
    int64_t yesVotes;
    int64_t noVotes;
    int64_t absoluteYesVotes;
    QString name;
    QString url;
    bool endorsed;
    bool funding;
    int64_t percentage;

    /** Return the unique identifier for this proposal (part) */
    QString getObjHash() const
    {
        return QString::fromStdString(hash.ToString());
    }


    ProposalRecord() {}
    ProposalRecord(const uint256 _hash, const CAmount& _amount, qint32 _start_epoch, qint32 _end_epoch,
                const int64_t& _yesVotes, const int64_t& _noVotes, const int64_t& _absoluteYesVotes,
                QString _name, QString _url, const bool _funding, const bool _endorsed, const int64_t& _percentage):
            hash(_hash), amount(_amount), start_epoch(_start_epoch), end_epoch(_end_epoch), yesVotes(_yesVotes),
            noVotes(_noVotes), absoluteYesVotes(_absoluteYesVotes), name(_name), url(_url), endorsed(_endorsed),
            funding(_funding), percentage(_percentage) {}
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
        parent(_parent) {}

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
            if (node.getMasternodeCount().enabled > 0)
                percentage = round(prop.abs_yes * 100 / node.getMasternodeCount().enabled);

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
                            prop.endorsed,
                            prop.funding,
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
        bool add = true;
        bool update = true;
        switch(status)
        {
        case CT_NEW:
            if(inModel)
            {
                // should never happen but attemt to update
                qWarning() << "ProposalTablePriv::updateProposal: Warning: Got CT_NEW, but proposal is already in model";
                status = CT_UPDATED;
                add = false;
            }
            if(add)
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
                if (node.getMasternodeCount().enabled > 0)
                    percentage = round(prop.abs_yes * 100 / node.getMasternodeCount().enabled);

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
                            prop.endorsed,
                            prop.funding,
                            percentage);

                parent->beginInsertRows(QModelIndex(), lowerIndex, lowerIndex);
                cachedProposals.insert(lowerIndex, toInsert);
                parent->endInsertRows();
                break;
            }
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
                //strange, not there... warn and try to add it
                qWarning() << "ProposalTablePriv::updateProposal: Warning: Got CT_UPDATED, but entry is not in model";
                status = CT_NEW;
                update = false;
            }
            if (update)
            {
                // Find proposal on platform
                interfaces::Proposal prop = node.getProposal(hash);
                if(prop.hash == uint256())
                {
                    // did we receive a wrong signal? The node got highes priority, delete the entry
                    qWarning() << "ProposalTablePriv::updateProposal: Warning: Got CT_UPDATED, but proposal is not on platform";
                    parent->beginRemoveRows(QModelIndex(), lowerIndex, upperIndex-1);
                    cachedProposals.erase(lower, upper);
                    parent->endRemoveRows();
                    break;
                }
                int percentage = 0;
                if (node.getMasternodeCount().enabled > 0)
                    percentage = round(prop.abs_yes * 100 / node.getMasternodeCount().enabled);
                lower->hash = prop.hash;
                lower->start_epoch = prop.start;
                lower->end_epoch = prop.end;
                lower->yesVotes = prop.yes;
                lower->noVotes = prop.no;
                lower-> absoluteYesVotes = prop.abs_yes;
                lower->amount = prop.amount;
                lower->name = QString::fromStdString(prop.name);
                lower->url = QString::fromStdString(prop.url);
                lower->endorsed = prop.endorsed;
                lower->funding = prop.funding;
                lower->percentage = percentage;
                break;
            }
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
    columns << tr("Amount") << tr("Start Date") << tr("End Date") << tr("Yes") << tr("No") << tr("Absolute") << tr("Proposal") << tr("Endorsed") << tr("Funding") << tr("Percentage");
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

    QString theme = GUIUtil::getThemeName();

    ProposalRecord *rec = static_cast<ProposalRecord*>(index.internalPointer());

    switch(role)
    {
    case Qt::DisplayRole:
        switch(index.column())
        {
        case Proposal:
            return QString(rec->name);
        case YesVotes:
            return qint64(rec->yesVotes);
        case NoVotes:
            return qint64(rec->noVotes);
        case AbsoluteYesVotes:
            return qint64(rec->absoluteYesVotes);
        case StartDate:
            return (QDateTime::fromTime_t(rec->start_epoch)).date().toString(Qt::SystemLocaleLongDate);
        case EndDate:
            return (QDateTime::fromTime_t(rec->end_epoch)).date().toString(Qt::SystemLocaleLongDate);
        case Percentage:
            return QString("%1\%").arg(rec->percentage);
        case Amount:
            return BitcoinUnits::format(BitcoinUnits::chuffs, rec->amount);
        case Funding:
        {
            QString ret;
            rec->funding == true ?  ret = "Yes" : ret = "No";
            return QString(ret);
        }
        case Endorsed:
            if (rec->funding == true)
                return QIcon(":/icons/" + theme + "/transaction_confirmed");
        }
        break;
    case Qt::TextAlignmentRole:
        return column_alignments[index.column()];
    case Qt::ForegroundRole:
        if(index.column() == Percentage) {
            if(rec->percentage < 10) {
                return COLOR_NEGATIVE;
            } else {
                return COLOR_GREEN;
            }
        } 
        break;
        if(index.column() == Funding) {
            if(rec->funding) {
                return COLOR_GREEN;
            } else {
                return COLOR_NEGATIVE;
            }
        }
        return COLOR_BLACK;
        break;
    case ProposalHashRole:
        return rec->getObjHash();
    case ProposalUrlRole:
        return rec->url;
    case ProposalDateFilterRole:
        return QDateTime::fromTime_t(rec->end_epoch);
    default: break;
    }
    return QVariant();
}

QVariant ProposalTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation == Qt::Horizontal)
    {
        if(role == Qt::DisplayRole && section < columns.size())
        {
            return columns[section];
        }
        else if (role == Qt::ToolTipRole)
        {
            switch(section)
            {
            case Proposal:
                return tr("Name and short description");
            case StartDate:
                return tr("Starting date of the proposal");
            case EndDate:
                return tr("Expiry date of the proposal");
            case YesVotes:
                return tr("Obtained funding yes votes");
            case NoVotes:
                return tr("Obtained funding no votes");
            case AbsoluteYesVotes:
                return tr("Obtained absolute yes votes (difference funding yes / no)");
            case Amount:
                return tr("Proposed amount");
            case Funding:
                return tr("Current funding state based on votes");
            case Percentage:
                return tr("Current vote percentage");
            default: break;
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

void ProposalTableModel::emitDataChanged(int idx)
{
    Q_EMIT dataChanged(index(idx, 0, QModelIndex()), index(idx, columns.length()-1, QModelIndex()));
}
