#include "myofferwhitelisttablemodel.h"

#include "guiutil.h"
#include "walletmodel.h"

#include "wallet/wallet.h"
#include "base58.h"

#include <QFont>
using namespace std;

struct MyOfferWhitelistTableEntry
{
	QString offer;
    QString alias;
	QString expires;
	QString discount;	

    MyOfferWhitelistTableEntry() {}
    MyOfferWhitelistTableEntry(const QString &offer, const QString &alias, const QString &expires,const QString &discount):
        offer(offer), alias(alias), expires(expires),discount(discount) {}
};

struct MyOfferWhitelistTableEntryLessThan
{
    bool operator()(const MyOfferWhitelistTableEntry &a, const MyOfferWhitelistTableEntry &b) const
    {
        return a.offer < b.offer;
    }
    bool operator()(const MyOfferWhitelistTableEntry &a, const QString &b) const
    {
        return a.offer < b;
    }
    bool operator()(const QString &a, const MyOfferWhitelistTableEntry &b) const
    {
        return a < b.offer;
    }
};

// Private implementation
class MyOfferWhitelistTablePriv
{
public:
    QList<MyOfferWhitelistTableEntry> cachedEntryTable;
    MyOfferWhitelistTableModel *parent;

    MyOfferWhitelistTablePriv(MyOfferWhitelistTableModel *parent):
        parent(parent) {}


    void updateEntry(const QString &offer, const QString &alias, const QString &expires,const QString &discount, int status)
    {
		if(!parent)
		{
			return;
		}
        // Find offer / value in model
        QList<MyOfferWhitelistTableEntry>::iterator lower = qLowerBound(
            cachedEntryTable.begin(), cachedEntryTable.end(), offer, MyOfferWhitelistTableEntryLessThan());
        QList<MyOfferWhitelistTableEntry>::iterator upper = qUpperBound(
            cachedEntryTable.begin(), cachedEntryTable.end(), offer, MyOfferWhitelistTableEntryLessThan());
        int lowerIndex = (lower - cachedEntryTable.begin());
        int upperIndex = (upper - cachedEntryTable.begin());
        bool inModel = (lower != upper);

        switch(status)
        {
        case CT_NEW:

            parent->beginInsertRows(QModelIndex(), lowerIndex, lowerIndex);
            cachedEntryTable.insert(lowerIndex, MyOfferWhitelistTableEntry(offer, alias, expires, discount));
            parent->endInsertRows();
            break;
        case CT_UPDATED:
			lower->offer = offer;
			lower->alias = alias;
			lower->expires = expires;
			lower->discount = discount;
            parent->emitDataChanged(lowerIndex);
            break;
        case CT_DELETED:

            parent->beginRemoveRows(QModelIndex(), lowerIndex, upperIndex-1);
            cachedEntryTable.erase(lower, upper);
            parent->endRemoveRows();
            break;
        }
    }

    int size()
    {
        return cachedEntryTable.size();
    }

    MyOfferWhitelistTableEntry *index(int idx)
    {
        if(idx >= 0 && idx < cachedEntryTable.size())
        {
            return &cachedEntryTable[idx];
        }
        else
        {
            return 0;
        }
    }
};

MyOfferWhitelistTableModel::MyOfferWhitelistTableModel(WalletModel *parent) :
    QAbstractTableModel(parent)
{
    columns << tr("Alias") << tr("Discount") << tr("Expires In");
    priv = new MyOfferWhitelistTablePriv(this);

}

MyOfferWhitelistTableModel::~MyOfferWhitelistTableModel()
{
    delete priv;
}
int MyOfferWhitelistTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return priv->size();
}

int MyOfferWhitelistTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return columns.length();
}

QVariant MyOfferWhitelistTableModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();

    MyOfferWhitelistTableEntry *rec = static_cast<MyOfferWhitelistTableEntry*>(index.internalPointer());

    if(role == Qt::DisplayRole || role == Qt::EditRole)
    {
        switch(index.column())
        {
        case Offer:
            return rec->offer;
        case Alias:
            return rec->alias;
        case Discount:
            return rec->discount;
        case Expires:
            return rec->expires;
        }
    }
    else if (role == AliasRole)
    {
        return rec->alias;
    }
    return QVariant();
}

bool MyOfferWhitelistTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if(!index.isValid())
        return false;
    MyOfferWhitelistTableEntry *rec = static_cast<MyOfferWhitelistTableEntry*>(index.internalPointer());

    editStatus = OK;

    if(role == Qt::EditRole)
    {
        switch(index.column())
        {
        case Offer:
            // Do nothing, if old value == new value
            if(rec->offer == value.toString())
            {
                editStatus = NO_CHANGES;
                return false;
            }
           
            break;
        case Alias:
            // Do nothing, if old value == new value
            if(rec->alias == value.toString())
            {
                editStatus = NO_CHANGES;
                return false;
            }
           
            break;
        case Discount:
            // Do nothing, if old value == new value
            if(rec->discount == value.toString())
            {
                editStatus = NO_CHANGES;
                return false;
            }
           
            break;
       case Expires:
            // Do nothing, if old value == new value
            if(rec->expires == value.toString())
            {
                editStatus = NO_CHANGES;
                return false;
            }
            break;
        return true;
		}
    }
    return false;
}

QVariant MyOfferWhitelistTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation == Qt::Horizontal)
    {
        if(role == Qt::DisplayRole)
        {
            return columns[section];
        }
    }
    return QVariant();
}

Qt::ItemFlags MyOfferWhitelistTableModel::flags(const QModelIndex &index) const
{
    if(!index.isValid())
        return 0;
    Qt::ItemFlags retval = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    return retval;
}

QModelIndex MyOfferWhitelistTableModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    MyOfferWhitelistTableEntry *data = priv->index(row);
    if(data)
    {
        return createIndex(row, column, priv->index(row));
    }
    else
    {
        return QModelIndex();
    }
}

void MyOfferWhitelistTableModel::updateEntry(const QString &offer, const QString &alias, const QString &expires,const QString &discount, int status)
{
    priv->updateEntry(offer, alias, expires, discount, status);
}

QString MyOfferWhitelistTableModel::addRow(const QString &offer, const QString &alias, const QString &expires,const QString &discount)
{
    std::string strAlias = alias.toStdString();
    editStatus = OK;
    return QString::fromStdString(strAlias);
}
void MyOfferWhitelistTableModel::clear()
{
	beginResetModel();
    priv->cachedEntryTable.clear();
	endResetModel();
}


void MyOfferWhitelistTableModel::emitDataChanged(int idx)
{
    Q_EMIT dataChanged(index(idx, 0, QModelIndex()), index(idx, columns.length()-1, QModelIndex()));
}
