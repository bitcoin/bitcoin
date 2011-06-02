#include "addresstablemodel.h"
#include "guiutil.h"
#include "main.h"

#include <QFont>

const QString AddressTableModel::Send = "S";
const QString AddressTableModel::Receive = "R";

struct AddressTableEntry
{
    enum Type {
        Sending,
        Receiving
    };
    Type type;
    QString label;
    QString address;

    AddressTableEntry() {}
    AddressTableEntry(Type type, const QString &label, const QString &address):
        type(type), label(label), address(address) {}
};

/* Private implementation */
struct AddressTablePriv
{
    QList<AddressTableEntry> cachedAddressTable;

    void refreshAddressTable()
    {
        cachedAddressTable.clear();

        CRITICAL_BLOCK(cs_mapKeys)
        CRITICAL_BLOCK(cs_mapAddressBook)
        {
            BOOST_FOREACH(const PAIRTYPE(std::string, std::string)& item, mapAddressBook)
            {
                std::string strAddress = item.first;
                std::string strName = item.second;
                uint160 hash160;
                bool fMine = (AddressToHash160(strAddress, hash160) && mapPubKeys.count(hash160));
                cachedAddressTable.append(AddressTableEntry(fMine ? AddressTableEntry::Receiving : AddressTableEntry::Sending,
                                  QString::fromStdString(strName),
                                  QString::fromStdString(strAddress)));
            }
        }
    }

    int size()
    {
        return cachedAddressTable.size();
    }

    AddressTableEntry *index(int idx)
    {
        if(idx >= 0 && idx < cachedAddressTable.size())
        {
            return &cachedAddressTable[idx];
        } else {
            return 0;
        }
    }
};

AddressTableModel::AddressTableModel(QObject *parent) :
    QAbstractTableModel(parent),priv(0)
{
    columns << tr("Label") << tr("Address");
    priv = new AddressTablePriv();
    priv->refreshAddressTable();
}

AddressTableModel::~AddressTableModel()
{
    delete priv;
}

int AddressTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return priv->size();
}

int AddressTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return columns.length();
}

QVariant AddressTableModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();

    AddressTableEntry *rec = static_cast<AddressTableEntry*>(index.internalPointer());

    if(role == Qt::DisplayRole || role == Qt::EditRole)
    {
        switch(index.column())
        {
        case Label:
            return rec->label;
        case Address:
            return rec->address;
        }
    } else if (role == Qt::FontRole)
    {
        if(index.column() == Address)
        {
            return GUIUtil::bitcoinAddressFont();
        }
    } else if (role == TypeRole)
    {
        switch(rec->type)
        {
        case AddressTableEntry::Sending:
            return Send;
        case AddressTableEntry::Receiving:
            return Receive;
        default: break;
        }
    }
    return QVariant();
}

bool AddressTableModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
    if(!index.isValid())
        return false;

    if(role == Qt::EditRole)
    {
        switch(index.column())
        {
        case Label:
            /* TODO */
            break;
        case Address:
            /* TODO */
            /* Double-check that we're not overwriting receiving address */
            /* Note that changing address changes index in map */
            break;
        }
        /* emit dataChanged(index, index); */
        return true;
    }
    return false;
}

QVariant AddressTableModel::headerData(int section, Qt::Orientation orientation, int role) const
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

QModelIndex AddressTableModel::index(int row, int column, const QModelIndex & parent) const
{
    Q_UNUSED(parent);
    AddressTableEntry *data = priv->index(row);
    if(data)
    {
        return createIndex(row, column, priv->index(row));
    } else {
        return QModelIndex();
    }
}

void AddressTableModel::updateList()
{
    /* Update internal model from Bitcoin core */
    beginResetModel();
    priv->refreshAddressTable();
    endResetModel();
}
