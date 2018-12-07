// Copyright (c) 2011-2018 The Bitcoin Core developers
// Copyright (c) 2014-2017 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/masternodetablemodel.h>

#include <qt/clientmodel.h>
#include <qt/guiconstants.h>
#include <qt/guiutil.h>

#include <interfaces/node.h>
#include <key_io.h>

#include <QColor>
#include <QFont>
#include <QDebug>

const QString MasternodeTableModel::MyNodes = "M";
const QString MasternodeTableModel::AllNodes = "A";

static int column_alignments[] = {
        Qt::AlignLeft|Qt::AlignVCenter,
        Qt::AlignLeft|Qt::AlignVCenter,
        Qt::AlignCenter|Qt::AlignVCenter,
        Qt::AlignLeft|Qt::AlignVCenter,
        Qt::AlignCenter|Qt::AlignVCenter,
        Qt::AlignRight|Qt::AlignVCenter,
        Qt::AlignRight|Qt::AlignVCenter,
        Qt::AlignLeft|Qt::AlignVCenter
    };

struct MasternodeTableEntry
{
    enum Type {
        MyNodeList,
        AllNodeList,
        Hidden /* QSortFilterProxyModel will filter these out */
    };

    Type type;
    QString txhash;
    qint64 n;
    QString alias;
    QString address;
    qint64 protocol;
    qint64 daemon;
    qint64 sentinel;
    QString status;
    qint64 active;
    qint64 lastseen;
    QString payee;
    qint64 banscore;

    QString outpoint() const
    {
        return QString::fromStdString(strprintf("%s-%u", txhash.toStdString().substr(0,64), n));
    }

    MasternodeTableEntry() {}
    MasternodeTableEntry(Type _type, const QString & _txhash, const qint64& _n, const QString &_alias, const QString &_address,
                         const qint64 &_protocol, const qint64 &_daemon, const qint64 &_sentinel, const QString &_status,
                         const qint64 &_active, const qint64 &_lastseen, const QString &_payee, const qint64 &_banscore) :
        type(_type), txhash(_txhash), n(_n), alias(_alias), address(_address), protocol(_protocol), daemon (_daemon),
        sentinel (_sentinel), status(_status), active(_active), lastseen(_lastseen), payee(_payee), banscore(_banscore) {}
};

/* Determine if our own masternode */
static MasternodeTableEntry::Type translateMasternodeType(const QString &strAlias)
{
    MasternodeTableEntry::Type addressType = MasternodeTableEntry::Hidden;
    // "refund" addresses aren't shown, and change addresses aren't in mapAddressBook at all.
    if (strAlias != "")
        addressType = MasternodeTableEntry::MyNodeList;
    else
        addressType = MasternodeTableEntry::AllNodeList;
    return addressType;
}

struct outpointEntryLessThan
{
    bool operator()(const MasternodeTableEntry &a, const MasternodeTableEntry &b) const
    {
        return a.outpoint() < b.outpoint();
    }
    bool operator()(const MasternodeTableEntry &a, const QString &b) const
    {
        return a.outpoint() < b;
    }
    bool operator()(const QString &a, const MasternodeTableEntry &b) const
    {
        return a < b.outpoint();
    }
};

// Private implementation
class MasternodeTablePriv
{
public:
    explicit MasternodeTablePriv(MasternodeTableModel *_parent):
        parent(_parent) {}

    MasternodeTableModel *parent;

    QList<MasternodeTableEntry> cachedMasternodeTable;

    void refreshMasternodeTable(interfaces::Node& node)
    {
        cachedMasternodeTable.clear();
        {
            for (const auto& masternode : node.getMasternodes())
            {
                MasternodeTableEntry::Type addressType = translateMasternodeType(
                        QString::fromStdString(masternode.alias));
                cachedMasternodeTable.append(MasternodeTableEntry(
                                                 addressType,
                                                 QString::fromStdString(masternode.outpoint.hash.ToString()),
                                                 masternode.outpoint.n,
                                                 QString::fromStdString(masternode.alias),
                                                 QString::fromStdString(masternode.address),
                                                 masternode.protocol,
                                                 masternode.daemon,
                                                 masternode.sentinel,
                                                 QString::fromStdString(masternode.status),
                                                 masternode.active,
                                                 masternode.last_seen,
                                                 QString::fromStdString(masternode.payee),
                                                 masternode.banscore));
            }
        }
        // qLowerBound() and qUpperBound() require our cachedAddressTable list to be sorted in asc order
        // Even though the map is already sorted this re-sorting step is needed because the originating map
        // is sorted by binary address, not by base58() address.
        qSort(cachedMasternodeTable.begin(), cachedMasternodeTable.end(), outpointEntryLessThan());
    }

    void updateMasternode(interfaces::Node& node, const COutPoint& outpoint, int status)
    {
        qDebug() << "MasternodeTablePriv::updateMasternode" + QString::fromStdString(strprintf("%s-%d", outpoint.hash.ToString(), outpoint.n)) + " " + QString::number(status);

        // Find bounds of this masternode in model
        QString strOutpoint = QString::fromStdString(outpoint.ToStringShort());
        qWarning() << "MasternodeTablePriv::updateMasternode: strOutpoint: " + strOutpoint + " " + QString::number(status);
        QList<MasternodeTableEntry>::iterator lower = qLowerBound(
            cachedMasternodeTable.begin(), cachedMasternodeTable.end(), strOutpoint, outpointEntryLessThan());
        QList<MasternodeTableEntry>::iterator upper = qUpperBound(
            cachedMasternodeTable.begin(), cachedMasternodeTable.end(), strOutpoint, outpointEntryLessThan());
        int lowerIndex = (lower - cachedMasternodeTable.begin());
        int upperIndex = (upper - cachedMasternodeTable.begin());
        bool inModel = (lower != upper);
        switch(status)
        {
        case CT_NEW:
            if(inModel)
            {
                qWarning() << "MasternodeTablePriv::updateMasternode: Warning: Got CT_NEW, but masternode is already in model: " + QString::fromStdString(strprintf("%s-%d", outpoint.hash.ToString(), outpoint.n)) + " " + QString::number(status);
                break;
            }
        {
            // Find proposal on platform
            interfaces::Masternode masternode = node.getMasternode(outpoint);
            if(masternode.outpoint == COutPoint())
            {
                qWarning() << "MasternodeTablePriv::updateMasternode: Warning: Got CT_NEW, but masternode is not on platform: " + QString::fromStdString(strprintf("%s-%d", outpoint.hash.ToString(), outpoint.n)) + " " + QString::number(status);
                break;
            }
            // Added -- insert at the right position
            MasternodeTableEntry::Type addressType = translateMasternodeType(
                        QString::fromStdString(masternode.alias));

            MasternodeTableEntry toInsert =
                    MasternodeTableEntry(
                        addressType,
                        QString::fromStdString(masternode.outpoint.hash.ToString()),
                        masternode.outpoint.n,
                        QString::fromStdString(masternode.alias),
                        QString::fromStdString(masternode.address),
                        masternode.protocol,
                        masternode.daemon,
                        masternode.sentinel,
                        QString::fromStdString(masternode.status),
                        masternode.active,
                        masternode.last_seen,
                        QString::fromStdString(masternode.payee),
                        masternode.banscore);

            parent->beginInsertRows(QModelIndex(), lowerIndex, lowerIndex);
            cachedMasternodeTable.insert(lowerIndex, toInsert);
            parent->endInsertRows();
        }
            break;
        case CT_DELETED:
            if(!inModel)
            {
                qWarning() << "MasternodeTablePriv::updateMasternode: Warning: Got CT_DELETED, but masternode is not in model: " + QString::fromStdString(strprintf("%s-%d", outpoint.hash.ToString(), outpoint.n)) + " " + QString::number(status);
                break;
            }
            // Removed -- remove entire transaction from table
            parent->beginRemoveRows(QModelIndex(), lowerIndex, upperIndex-1);
            cachedMasternodeTable.erase(lower, upper);
            parent->endRemoveRows();
            break;
        case CT_UPDATED:
            if(!inModel)
            {
                qWarning() << "MasternodeTablePriv::updateMasternode: Warning: Got CT_UPDATED, but entry is not in model: "+ QString::fromStdString(strprintf("%s-%d", outpoint.hash.ToString(), outpoint.n)) + " " + QString::number(status);
                break;
            }
        {
            // Find masternode on platform
            interfaces::Masternode masternode = node.getMasternode(outpoint);
            if(masternode.outpoint == COutPoint())
            {
                qWarning() << "MasternodeTablePriv::updateMasternode: Warning: Got CT_UPDATED, but masternode is not on platform: " + QString::fromStdString(strprintf("%s-%d", outpoint.hash.ToString(), outpoint.n)) + " " + QString::number(status);
                break;
            }
            MasternodeTableEntry::Type addressType = translateMasternodeType(
                        QString::fromStdString(masternode.alias));
            lower->type = addressType;
            lower->txhash = QString::fromStdString(masternode.outpoint.hash.ToString());
            lower->n = masternode.outpoint.n;
            lower->alias = QString::fromStdString(masternode.alias);
            lower->address = QString::fromStdString(masternode.address);
            lower->protocol = masternode.protocol;
            lower->daemon = masternode.daemon;
            lower->sentinel = masternode.sentinel;
            lower->status = QString::fromStdString(masternode.status);
            lower->active = masternode.active;
            lower->lastseen = masternode.last_seen;
            lower->payee = QString::fromStdString(masternode.payee);
            lower->banscore = masternode.banscore;
            parent->emitDataChanged(lowerIndex);
        }
            break;
        }
    }

    int size()
    {
        return cachedMasternodeTable.size();
    }

    MasternodeTableEntry *index(int idx)
    {
        if(idx >= 0 && idx < cachedMasternodeTable.size())
        {
            MasternodeTableEntry *rec = &cachedMasternodeTable[idx];
            return rec;
        }
        return 0;
    }
};

MasternodeTableModel::MasternodeTableModel(ClientModel *parent) :
    QAbstractTableModel(parent),
    clientModel(parent),
    priv(new MasternodeTablePriv(this))

{
    columns << tr("Alias") << tr("Address") << tr("Version") << tr("Status") << tr("Score") << tr("Active") << tr("Last Seen") << tr("Payee");
    priv->refreshMasternodeTable(clientModel->node());

    // Subscribe for updates
    connect(clientModel, &ClientModel::updateMasternode, this, &MasternodeTableModel::updateMasternode);
}

MasternodeTableModel::~MasternodeTableModel()
{
    delete priv;
}

void MasternodeTableModel::updateMasternode(const QString &_hash, const int &_n, int status)
{
    COutPoint outpoint = COutPoint(uint256S(_hash.toStdString()), _n);
    priv->updateMasternode(clientModel->node(), outpoint, status);
}

int MasternodeTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return priv->size();
}

int MasternodeTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return columns.length();
}

QVariant MasternodeTableModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();

    MasternodeTableEntry *rec = static_cast<MasternodeTableEntry*>(index.internalPointer());

    switch(role)
    {
    case Qt::DisplayRole:
        switch(index.column())
        {
        case Alias:
            return rec->alias;
        case Address:
            return rec->address;
        case Daemon:
            return rec->daemon;
        case Status:
            return rec->status;
        case Score:
            return rec->banscore;
        case Active:
            return QString::fromStdString(DurationToDHMS(rec->active));
        case Last_Seen:
            if (rec->lastseen > 0)
                return (QDateTime::fromTime_t((qint32)rec->lastseen)).date().toString(Qt::SystemLocaleLongDate);
            else {
                QString ret = tr("never");
                return ret;
            }
        case Payee:
            return rec->payee;
        default: break;
        }
    case TypeRole:
        switch(rec->type)
        {
        case MasternodeTableEntry::MyNodeList:
            return MyNodes;
        case MasternodeTableEntry::AllNodeList:
            return AllNodes;
        default: break;
        }
    case TxHashRole:
        return rec->txhash;
    case TxOutIndexRole:
        return rec->n;
    case Qt::TextAlignmentRole:
        return column_alignments[index.column()];
    case Qt::ForegroundRole:
        if(index.column() == Status) {
            if(rec->status == "MISSING" || rec->status == "NEW_START_REQUIRED") {
                return COLOR_NEGATIVE;
            } else if (rec->status == "ENABLED"){
                return COLOR_GREEN;
            }
        }
        return COLOR_BLACK;
    default: break;
    }
    return QVariant();
}

QVariant MasternodeTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation == Qt::Horizontal)
    {
        if(role == Qt::DisplayRole && section < columns.size())
        {
            return columns[section];
        }
    }
    return QVariant();
}

QModelIndex MasternodeTableModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    MasternodeTableEntry *data = priv->index(row);
    if(data)
    {
        return createIndex(row, column, priv->index(row));
    }
    else
    {
        return QModelIndex();
    }
}

void MasternodeTableModel::emitDataChanged(int idx)
{
    Q_EMIT dataChanged(index(idx, 0, QModelIndex()), index(idx, columns.length()-1, QModelIndex()));
}
