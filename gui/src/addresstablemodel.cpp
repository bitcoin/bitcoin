#include "addresstablemodel.h"
#include "main.h"

const QString AddressTableModel::Send = "S";
const QString AddressTableModel::Receive = "R";

AddressTableModel::AddressTableModel(QObject *parent) :
    QAbstractTableModel(parent)
{

}

int AddressTableModel::rowCount(const QModelIndex &parent) const
{
    return 5;
}

int AddressTableModel::columnCount(const QModelIndex &parent) const
{
    return 2;
}

QVariant AddressTableModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();

    if(role == Qt::DisplayRole)
    {
        /* index.row(), index.column() */
        /* Return QString */
        if(index.column() == Address)
            return "1PC9aZC4hNX2rmmrt7uHTfYAS3hRbph4UN" + QString::number(index.row());
        else
            return "Description";
    } else if (role == TypeRole)
    {
        switch(index.row() % 2)
        {
        case 0: return Send;
        case 1: return Receive;
        }
    }
    return QVariant();
}

QVariant AddressTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    return QVariant();
}

Qt::ItemFlags AddressTableModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemIsEnabled;

    return QAbstractTableModel::flags(index);
}
